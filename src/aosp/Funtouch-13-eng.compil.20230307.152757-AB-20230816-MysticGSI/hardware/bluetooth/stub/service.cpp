// Soft Bluetooth HCI: virtual controller so the stack can reach STATE_ON.
// No RF / no discovery results — inquiry & LE scan complete empty.
#include <aidl/android/hardware/bluetooth/BnBluetoothHci.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/Status.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

using namespace aidl::android::hardware::bluetooth;
using ndk::ScopedAStatus;

namespace {

constexpr const char* kTag = "qemu-bt";
// Fixed public BD_ADDR for this soft controller (locally administered).
constexpr std::array<uint8_t, 6> kBdAddr = {0x02, 0x00, 0x00, 0xBB, 0x77, 0x01};

uint16_t opcodeOf(const std::vector<uint8_t>& cmd) {
    if (cmd.size() < 2) return 0;
    return static_cast<uint16_t>(cmd[0] | (cmd[1] << 8));
}

std::vector<uint8_t> makeCommandComplete(uint16_t opcode, const std::vector<uint8_t>& ret) {
    std::vector<uint8_t> ev;
    ev.reserve(5 + ret.size());
    ev.push_back(0x0e);  // Command Complete
    ev.push_back(static_cast<uint8_t>(3 + ret.size()));
    ev.push_back(0x01);  // num HCI command packets
    ev.push_back(static_cast<uint8_t>(opcode & 0xff));
    ev.push_back(static_cast<uint8_t>((opcode >> 8) & 0xff));
    ev.insert(ev.end(), ret.begin(), ret.end());
    return ev;
}

std::vector<uint8_t> makeCommandStatus(uint16_t opcode, uint8_t status = 0x00) {
    return {0x0f, 0x04, status, 0x01,
            static_cast<uint8_t>(opcode & 0xff),
            static_cast<uint8_t>((opcode >> 8) & 0xff)};
}

std::vector<uint8_t> handleCommand(const std::vector<uint8_t>& cmd) {
    const uint16_t op = opcodeOf(cmd);
    switch (op) {
        case 0x0c03:  // Reset
            return makeCommandComplete(op, {0x00});
        case 0x1001: {  // Read Local Version Information
            // status, hci_ver=0x0D (5.2), hci_rev, lmp_ver, mfr=0x0075 (fake), lmp_subver
            return makeCommandComplete(op, {0x00, 0x0d, 0x00, 0x00, 0x0d, 0x75, 0x00, 0x0d, 0x00});
        }
        case 0x1002: {  // Read Local Supported Commands — all zeros is acceptable
            std::vector<uint8_t> ret(65, 0x00);
            ret[0] = 0x00;  // status
            // Advertise a few common command bits so GD does not refuse the controller.
            ret[1] = 0xff;
            ret[2] = 0xff;
            ret[3] = 0xff;
            return makeCommandComplete(op, ret);
        }
        case 0x1003: {  // Read Local Supported Features
            // status + 8 feature bytes (basic BR/EDR + LE capable bit in byte 4 bit 6 often via page)
            return makeCommandComplete(op, {0x00, 0xff, 0xff, 0x8f, 0xfe, 0xdb, 0xff, 0x7b, 0x87});
        }
        case 0x1004: {  // Read Local Extended Features (page in cmd[3])
            uint8_t page = cmd.size() > 3 ? cmd[3] : 0;
            if (page == 0) {
                return makeCommandComplete(op, {0x00, 0x00, 0x01, 0xff, 0xff, 0x8f, 0xfe, 0xdb, 0xff,
                                               0x7b, 0x87});
            }
            if (page == 1) {
                // LE supported host / simultaneous LE+BR
                return makeCommandComplete(op, {0x00, 0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
                                               0x00, 0x00});
            }
            return makeCommandComplete(op, {0x00, page, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00});
        }
        case 0x1005:  // Read Buffer Size
            // status, acl_len=1021, sco_len=255, acl_num=10, sco_num=8
            return makeCommandComplete(op, {0x00, 0xfd, 0x03, 0xff, 0x0a, 0x00, 0x08, 0x00});
        case 0x1009:  // Read BD_ADDR
            return makeCommandComplete(op, {0x00, kBdAddr[0], kBdAddr[1], kBdAddr[2], kBdAddr[3],
                                           kBdAddr[4], kBdAddr[5]});
        case 0x0c01:  // Set Event Mask
        case 0x0c0c:  // Write PIN Type
        case 0x0c13:  // Write Local Name
        case 0x0c18:  // Write Page Timeout
        case 0x0c1a:  // Write Scan Enable
        case 0x0c24:  // Write Class of Device
        case 0x0c45:  // Write Inquiry Mode
        case 0x0c52:  // Write Extended Inquiry Response
        case 0x0c56:  // Write Simple Pairing Mode
        case 0x0c58:  // Write Inquiry Transmit Power Level
        case 0x0c6d:  // Write LE Host Support
        case 0x0c63:  // Set Event Mask Page 2
        case 0x0c7a:  // Write Secure Connections Host Support
            return makeCommandComplete(op, {0x00});
        case 0x0c14: {  // Read Local Name
            std::vector<uint8_t> ret(249, 0x00);
            ret[0] = 0x00;
            const char* name = "QEMU Soft BT";
            std::memcpy(ret.data() + 1, name, std::strlen(name));
            return makeCommandComplete(op, ret);
        }
        case 0x0c23:  // Read Class of Device
            return makeCommandComplete(op, {0x00, 0x0c, 0x02, 0x5a});  // Smartphone-ish
        case 0x0401: {  // Inquiry — accept then complete with 0 responses
            // Caller must send Command Status first, then Inquiry Complete.
            return {};  // special-cased by sendHciCommand
        }
        case 0x0402:  // Inquiry Cancel
            return makeCommandComplete(op, {0x00});
        // —— LE Controller (OGF 0x08): opcodes verified against Core Spec 5.x ——
        case 0x2001:  // LE Set Event Mask
        case 0x2005:  // LE Set Random Address
        case 0x2006:  // LE Set Advertising Parameters
        case 0x2008:  // LE Set Advertising Data
        case 0x2009:  // LE Set Scan Response Data
        case 0x200a:  // LE Set Advertising Enable
        case 0x200b:  // LE Set Scan Parameters
        case 0x200c:  // LE Set Scan Enable
        case 0x2011:  // LE Clear Filter Accept List
        case 0x2012:  // LE Add Device To Filter Accept List
        case 0x2013:  // LE Remove Device From Filter Accept List
        case 0x2022:  // LE Set Data Length
        case 0x2031:  // LE Set Default PHY
        case 0x2032:  // LE Set PHY
        case 0x2035:  // LE Set Advertising Set Random Address
        case 0x2036:  // LE Set Extended Advertising Parameters
        case 0x2037:  // LE Set Extended Advertising Data
        case 0x2038:  // LE Set Extended Scan Response Data
        case 0x2039:  // LE Set Extended Advertising Enable
        case 0x203c:  // LE Remove Advertising Set
        case 0x203d:  // LE Clear Advertising Sets
        case 0x2041:  // LE Set Extended Scan Parameters
        case 0x2042:  // LE Set Extended Scan Enable
            return makeCommandComplete(op, {0x00});
        case 0x2002:  // LE Read Buffer Size
            return makeCommandComplete(op, {0x00, 0xfb, 0x00, 0x0a});
        case 0x2003:  // LE Read Local Supported Features
            return makeCommandComplete(op, {0x00, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00});
        case 0x2007:  // LE Read Advertising Physical Channel Tx Power
            return makeCommandComplete(op, {0x00, 0x00});
        case 0x200f:  // LE Read Filter Accept List Size
            return makeCommandComplete(op, {0x00, 0x08});
        case 0x2017: {  // LE Encrypt → status + 16-byte ciphertext
            std::vector<uint8_t> ret(17, 0x00);
            return makeCommandComplete(op, ret);
        }
        case 0x2018: {  // LE Rand → status + 8-byte random (GD asserts on size)
            return makeCommandComplete(op, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88});
        }
        case 0x201c:  // LE Read Supported States
            return makeCommandComplete(
                    op, {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff});
        case 0x2023:  // LE Read Suggested Default Data Length
            return makeCommandComplete(op, {0x00, 0x1b, 0x00, 0x48, 0x08});
        case 0x2024:  // LE Read Maximum Data Length
            return makeCommandComplete(op, {0x00, 0xfb, 0x00, 0x48, 0x08, 0xfb, 0x00, 0x48, 0x08});
        case 0x2030:  // LE Read PHY
            return makeCommandComplete(op, {0x00, 0x00, 0x01, 0x01});
        case 0x203a:  // LE Read Maximum Advertising Data Length
            return makeCommandComplete(op, {0x00, 0x1f, 0x00});
        case 0x203b:  // LE Read Number of Supported Advertising Sets
            return makeCommandComplete(op, {0x00, 0x01});
        case 0x204b:  // LE Read Transmit Power
            return makeCommandComplete(op, {0x00, 0x06, 0xf4});
        case 0x2060:  // LE Read Buffer Size [v2]
            return makeCommandComplete(op, {0x00, 0xfb, 0x00, 0x0a, 0xfb, 0x00, 0x0a});
        case 0x080f:  // Write Default Link Policy Settings
        case 0x0c05:  // Set Event Filter
        case 0x0c3a:  // Write Inquiry Scan Type
        case 0x0c43:  // Write Inquiry Mode (some stacks use 0x43)
        case 0x0c47:  // Write Page Scan Type
            return makeCommandComplete(op, {0x00});
        default:
            // Vendor / unknown: status-only Unknown Command (avoid truncated success).
            __android_log_print(ANDROID_LOG_INFO, kTag, "HCI cmd 0x%04x -> UNKNOWN", op);
            return makeCommandComplete(op, {0x01});
    }
}

}  // namespace

class QemuBluetoothHci : public BnBluetoothHci {
  public:
    ScopedAStatus initialize(const std::shared_ptr<IBluetoothHciCallbacks>& cb) override {
        // Stack aborts unless status==SUCCESS. After a crash, close() may never
        // run — accept re-init by replacing the callback and still report SUCCESS.
        {
            std::lock_guard lock(mu_);
            cb_ = cb;
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "initialize()");
        // Defer so initialize() can return before the stack continues HCI traffic.
        std::thread([cb]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            cb->initializationComplete(Status::SUCCESS);
        }).detach();
        return ScopedAStatus::ok();
    }

    ScopedAStatus close() override {
        std::lock_guard lock(mu_);
        cb_.reset();
        __android_log_print(ANDROID_LOG_INFO, kTag, "close()");
        return ScopedAStatus::ok();
    }

    ScopedAStatus sendHciCommand(const std::vector<uint8_t>& command) override {
        std::shared_ptr<IBluetoothHciCallbacks> cb;
        {
            std::lock_guard lock(mu_);
            cb = cb_;
        }
        if (!cb) return ScopedAStatus::ok();

        const uint16_t op = opcodeOf(command);
        if (op == 0x0401) {
            // Inquiry: status then immediate Inquiry Complete (0 responses).
            cb->hciEventReceived(makeCommandStatus(op, 0x00));
            // Inquiry Complete: status=0, num_responses=0
            cb->hciEventReceived({0x01, 0x01, 0x00});
            return ScopedAStatus::ok();
        }

        auto ev = handleCommand(command);
        if (!ev.empty()) cb->hciEventReceived(ev);
        return ScopedAStatus::ok();
    }

    ScopedAStatus sendAclData(const std::vector<uint8_t>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus sendScoData(const std::vector<uint8_t>&) override {
        return ScopedAStatus::ok();
    }
    ScopedAStatus sendIsoData(const std::vector<uint8_t>&) override {
        return ScopedAStatus::ok();
    }

  private:
    std::mutex mu_;
    std::shared_ptr<IBluetoothHciCallbacks> cb_;
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();
    auto service = ndk::SharedRefBase::make<QemuBluetoothHci>();
    constexpr auto instance = "android.hardware.bluetooth.IBluetoothHci/default";
    auto status = AServiceManager_addService(service->asBinder().get(), instance);
    __android_log_print(status == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                        "register %s: %d", instance, status);
    if (status != STATUS_OK) return 1;
    ABinderProcess_joinThreadPool();
    return 1;
}
