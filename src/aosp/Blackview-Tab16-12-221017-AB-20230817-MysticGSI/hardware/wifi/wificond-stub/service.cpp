// Soft wificond (binder name: wifinl80211). No nl80211 / no kernel wifi.
// Provides client iface + fixed fake scan results for Settings SSID list.
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_parcel.h>
#include <android/binder_parcel_utils.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <android/log.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr const char* kTag = "qemu-wificond";
constexpr const char* kService = "wifinl80211";

// Matches IBinder.FIRST_CALL_TRANSACTION
constexpr transaction_code_t kFirst = 1;

// IWificond method order (aidl)
enum WificondTxn : transaction_code_t {
    WC_CREATE_AP = kFirst,           // 1
    WC_CREATE_CLIENT,                // 2
    WC_TEARDOWN_AP,                  // 3
    WC_TEARDOWN_CLIENT,              // 4
    WC_TEARDOWN_ALL,                 // 5
    WC_GET_CLIENTS,                  // 6
    WC_GET_APS,                      // 7
    WC_CH_2G,                        // 8
    WC_CH_5G,                        // 9
    WC_CH_DFS,                       // 10
    WC_CH_6G,                        // 11
    WC_CH_60G,                       // 12
    WC_REG_IFACE_CB,                 // 13
    WC_UNREG_IFACE_CB,               // 14
    WC_REG_EVENT_CB,                 // 15
    WC_UNREG_EVENT_CB,               // 16
    WC_GET_WIPHY,                    // 17
    WC_NOTIFY_CC,                    // 18
};

// IClientInterface
enum ClientTxn : transaction_code_t {
    CL_PKT = kFirst,        // 1 getPacketCounters
    CL_SIGNAL,              // 2 signalPoll
    CL_MAC,                 // 3 getMacAddress
    CL_NAME,                // 4 getInterfaceName
    CL_SCANNER,             // 5 getWifiScannerImpl
    CL_MGMT,                // 6 SendMgmtFrame
};

// IWifiScannerImpl
enum ScanTxn : transaction_code_t {
    SC_GET = kFirst,         // 1 getScanResults
    SC_GET_PNO,              // 2 getPnoScanResults
    SC_MAX_SSID,             // 3 getMaxSsidsPerScan
    SC_SCAN,                 // 4 scan (bool)
    SC_SCAN_REQ,             // 5 scanRequest (int)
    SC_SUB,                  // 6 subscribeScanEvents
    SC_UNSUB,                // 7 unsubscribeScanEvents
    SC_SUB_PNO,              // 8 subscribePnoScanEvents
    SC_UNSUB_PNO,            // 9 unsubscribePnoScanEvents
    SC_START_PNO,            // 10 startPnoScan
    SC_STOP_PNO,             // 11 stopPnoScan
    SC_ABORT,                // 12 abortScan
};

// IScanEvent
enum ScanEventTxn : transaction_code_t {
    SE_READY = kFirst,  // OnScanResultReady
    SE_FAILED,          // OnScanFailed
};

struct FakeAp {
    std::string ssid;
    std::array<uint8_t, 6> bssid{};
    int32_t freq = 2412;
    int32_t rssi_dbm = -55;
};

std::vector<FakeAp> gAps;
std::once_flag gApsOnce;

uint32_t mix32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

void ensureAps() {
    std::call_once(gApsOnce, [] {
        // Realistic consumer SSIDs + vendor OUIs (BSSID looks like real gear).
        struct Template {
            const char* fmt;  // snprintf with one unsigned arg (hex suffix)
            unsigned arg_mask;
            uint8_t oui[3];
            int freq;
        };
        const Template templates[] = {
                {"TP-LINK_%04X", 0xffffu, {0x50, 0xC7, 0xBF}, 2412},
                {"TP-LINK_5G_%04X", 0xffffu, {0x50, 0xC7, 0xBF}, 5180},
                {"Xiaomi-%04X", 0xffffu, {0x64, 0xCC, 0x2E}, 2437},
                {"Xiaomi_5G-%04X", 0xffffu, {0x64, 0xCC, 0x2E}, 5745},
                {"ChinaNet-%04X", 0xffffu, {0xD4, 0xEE, 0x07}, 2462},
                {"CMCC-%04X", 0xffffu, {0x00, 0x1E, 0x10}, 2417},
                {"MERCURY_%04X", 0xffffu, {0xD8, 0x15, 0x0D}, 2437},
                {"Tenda_%04X", 0xffffu, {0xC8, 0x3A, 0x35}, 2412},
                {"HUAWEI-%06X", 0xffffffu, {0x48, 0x46, 0xFB}, 5200},
                {"HONOR_5G-%04X", 0xffffu, {0x20, 0xAB, 0x48}, 5220},
                {"ASUS_%04X", 0xffffu, {0x04, 0xD4, 0xC4}, 5785},
                {"NETGEAR%02X", 0xffu, {0xA0, 0x04, 0x60}, 5765},
                {"ChinaUnicom-%04X", 0xffffu, {0xB0, 0x95, 0x8E}, 2462},
                {"Redmi-%04X", 0xffffu, {0x28, 0x6C, 0x07}, 2417},
                {"cu_%04X", 0xffffu, {0x00, 0x1A, 0x2B}, 2437},
                {"DIRECT-%02X-HP DeskJet", 0xffu, {0x98, 0xE7, 0xF4}, 2412},
        };
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        uint32_t seed = mix32(static_cast<uint32_t>(now) ^ 0xc0ffeeu);
        const int n = static_cast<int>(sizeof(templates) / sizeof(templates[0]));
        for (int i = 0; i < n; ++i) {
            seed = mix32(seed + static_cast<uint32_t>(i) * 0x9e3779b9u);
            const Template& t = templates[i];
            FakeAp ap;
            char ssid[64];
            std::snprintf(ssid, sizeof(ssid), t.fmt, seed & t.arg_mask);
            ap.ssid = ssid;
            ap.bssid = {t.oui[0], t.oui[1], t.oui[2],
                        static_cast<uint8_t>((seed >> 16) & 0xff),
                        static_cast<uint8_t>((seed >> 8) & 0xff),
                        static_cast<uint8_t>(seed & 0xff)};
            ap.freq = t.freq;
            ap.rssi_dbm = -38 - static_cast<int>(seed % 45);
            gAps.push_back(ap);
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "fixed SSID catalog %zu", gAps.size());
    });
}

void tryCreateWlan0() {
    // Best-effort dummy iface so netd observers see wlan0. Ignore failures.
    (void)std::system("ip link add wlan0 type dummy 2>/dev/null");
    (void)std::system("ip link set wlan0 address 02:00:00:00:00:01 up 2>/dev/null");
}

// Minimal RSN IE → framework shows WPA2-PSK (padlock / needs password).
// EID=0x30, len=20, ver=1, group=CCMP, pairwise=CCMP, AKM=PSK.
constexpr uint8_t kRsnWpa2Psk[] = {
        0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f,
        0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00,
};

// Write one NativeScanResult in the legacy Java Parcelable format.
binder_status_t writeNativeScanResult(AParcel* p, const FakeAp& ap) {
    const int8_t* ssid = reinterpret_cast<const int8_t*>(ap.ssid.data());
    binder_status_t st = AParcel_writeByteArray(p, ssid, static_cast<int32_t>(ap.ssid.size()));
    if (st != STATUS_OK) return st;
    int8_t bssid[6];
    for (int i = 0; i < 6; ++i) bssid[i] = static_cast<int8_t>(ap.bssid[i]);
    st = AParcel_writeByteArray(p, bssid, 6);
    if (st != STATUS_OK) return st;
    st = AParcel_writeByteArray(p, reinterpret_cast<const int8_t*>(kRsnWpa2Psk),
                                static_cast<int32_t>(sizeof(kRsnWpa2Psk)));
    if (st != STATUS_OK) return st;
    st = AParcel_writeUint32(p, static_cast<uint32_t>(ap.freq));
    if (st != STATUS_OK) return st;
    st = AParcel_writeInt32(p, ap.rssi_dbm * 100);  // signal_mbm
    if (st != STATUS_OK) return st;
    auto tsf = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count());
    st = AParcel_writeUint64(p, tsf);
    if (st != STATUS_OK) return st;
    // ESS | Privacy — Privacy marks the BSS as secured.
    st = AParcel_writeUint32(p, 0x11u);
    if (st != STATUS_OK) return st;
    st = AParcel_writeInt32(p, 0);  // not associated
    if (st != STATUS_OK) return st;
    return AParcel_writeInt32(p, 0);  // empty radio chain list
}

binder_status_t writeScanResultArray(AParcel* reply) {
    ensureAps();
    binder_status_t st = AParcel_writeInt32(reply, static_cast<int32_t>(gAps.size()));
    if (st != STATUS_OK) return st;
    for (const auto& ap : gAps) {
        // writeTypedArray: non-null marker then object
        st = AParcel_writeInt32(reply, 1);
        if (st != STATUS_OK) return st;
        st = writeNativeScanResult(reply, ap);
        if (st != STATUS_OK) return st;
    }
    return STATUS_OK;
}

binder_status_t writeNoException(AParcel* reply) {
    return AParcel_writeInt32(reply, 0);
}

binder_status_t writeIntArray(AParcel* reply, const int32_t* data, int32_t n) {
    binder_status_t st = writeNoException(reply);
    if (st != STATUS_OK) return st;
    return AParcel_writeInt32Array(reply, data, n);
}

// ---------- Scanner ----------
struct ScannerState {
    AIBinder* scanEvent = nullptr;  // IScanEvent
    std::mutex mu;
};

ScannerState* scannerOf(AIBinder* b) {
    return static_cast<ScannerState*>(AIBinder_getUserData(b));
}

void* scannerCreate(void* args) { return args; }

void scannerDestroy(void* userData) {
    auto* s = static_cast<ScannerState*>(userData);
    if (s->scanEvent) AIBinder_decStrong(s->scanEvent);
    delete s;
}

AIBinder_Class* gScanEventClass = nullptr;

void ensureScanEventClass() {
    if (gScanEventClass) return;
    // Local class so AIBinder_associateClass writes the AIDL interface token
    // when we call back into the Java IScanEvent Stub.
    gScanEventClass = AIBinder_Class_define(
            "android.net.wifi.nl80211.IScanEvent",
            [](void* args) -> void* { return args; },
            [](void*) {},
            [](AIBinder*, transaction_code_t, const AParcel*, AParcel*) -> binder_status_t {
                return STATUS_UNKNOWN_TRANSACTION;
            });
}

void fireScanReady(ScannerState* s) {
    AIBinder* cb = nullptr;
    {
        std::lock_guard lock(s->mu);
        if (!s->scanEvent) {
            __android_log_print(ANDROID_LOG_WARN, kTag, "scan ready but no IScanEvent subscriber");
            return;
        }
        cb = s->scanEvent;
        AIBinder_incStrong(cb);
    }
    // Defer so scanRequest can return before the framework re-enters getScanResults.
    std::thread([cb]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        AParcel* in = nullptr;
        AParcel* out = nullptr;
        binder_status_t st = AIBinder_prepareTransaction(cb, &in);
        if (st == STATUS_OK) {
            st = AIBinder_transact(cb, SE_READY, &in, &out, FLAG_ONEWAY);
        }
        if (out) AParcel_delete(out);
        __android_log_print(st == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                            "OnScanResultReady: %d", st);
        AIBinder_decStrong(cb);
    }).detach();
}

binder_status_t scannerOnTransact(AIBinder* binder, transaction_code_t code, const AParcel* in,
                                  AParcel* out) {
    auto* s = scannerOf(binder);
    __android_log_print(ANDROID_LOG_INFO, kTag, "scanner txn %u", static_cast<unsigned>(code));
    switch (code) {
        case SC_GET:
        case SC_GET_PNO: {
            binder_status_t st = writeNoException(out);
            if (st != STATUS_OK) return st;
            st = writeScanResultArray(out);
            __android_log_print(ANDROID_LOG_INFO, kTag, "getScanResults -> %zu aps st=%d",
                                gAps.size(), st);
            return st;
        }
        case SC_MAX_SSID:
            writeNoException(out);
            return AParcel_writeInt32(out, 16);
        case SC_SCAN: {
            (void)in;
            writeNoException(out);
            AParcel_writeBool(out, true);
            fireScanReady(s);
            return STATUS_OK;
        }
        case SC_SCAN_REQ: {
            (void)in;
            writeNoException(out);
            AParcel_writeInt32(out, 0);  // SCAN_STATUS_SUCCESS
            fireScanReady(s);
            return STATUS_OK;
        }
        case SC_SUB: {
            // Inbound interface token is stripped by AIBinder_Class before onTransact.
            AIBinder* cb = nullptr;
            binder_status_t st = AParcel_readStrongBinder(in, &cb);
            if (st != STATUS_OK) {
                __android_log_print(ANDROID_LOG_ERROR, kTag, "subscribeScanEvents read cb: %d", st);
                return st;
            }
            ensureScanEventClass();
            st = AIBinder_associateClass(cb, gScanEventClass);
            if (st != STATUS_OK) {
                __android_log_print(ANDROID_LOG_WARN, kTag, "associateClass IScanEvent: %d", st);
            }
            std::lock_guard lock(s->mu);
            if (s->scanEvent) AIBinder_decStrong(s->scanEvent);
            s->scanEvent = cb;  // takes ownership of strong ref from read
            __android_log_print(ANDROID_LOG_INFO, kTag, "subscribeScanEvents ok");
            return STATUS_OK;   // oneway
        }
        case SC_UNSUB: {
            std::lock_guard lock(s->mu);
            if (s->scanEvent) {
                AIBinder_decStrong(s->scanEvent);
                s->scanEvent = nullptr;
            }
            return STATUS_OK;
        }
        case SC_SUB_PNO:
        case SC_UNSUB_PNO:
            return STATUS_OK;
        case SC_START_PNO:
            writeNoException(out);
            return AParcel_writeBool(out, true);
        case SC_STOP_PNO:
            writeNoException(out);
            return AParcel_writeBool(out, true);
        case SC_ABORT:
            return STATUS_OK;
        default:
            return STATUS_UNKNOWN_TRANSACTION;
    }
}

AIBinder_Class* gScannerClass = nullptr;

AIBinder* newScanner() {
    if (!gScannerClass) {
        gScannerClass = AIBinder_Class_define("android.net.wifi.nl80211.IWifiScannerImpl",
                                              scannerCreate, scannerDestroy, scannerOnTransact);
    }
    return AIBinder_new(gScannerClass, new ScannerState());
}

// ---------- Client ----------
struct ClientState {
    std::string name = "wlan0";
    AIBinder* scanner = nullptr;
};

ClientState* clientOf(AIBinder* b) {
    return static_cast<ClientState*>(AIBinder_getUserData(b));
}

void* clientCreate(void* args) { return args; }

void clientDestroy(void* userData) {
    auto* c = static_cast<ClientState*>(userData);
    if (c->scanner) AIBinder_decStrong(c->scanner);
    delete c;
}

binder_status_t clientOnTransact(AIBinder* binder, transaction_code_t code, const AParcel* in,
                                 AParcel* out) {
    auto* c = clientOf(binder);
    switch (code) {
        case CL_PKT:
        case CL_SIGNAL:
            writeNoException(out);
            return AParcel_writeInt32Array(out, nullptr, 0);
        case CL_MAC: {
            writeNoException(out);
            const int8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
            return AParcel_writeByteArray(out, mac, 6);
        }
        case CL_NAME:
            writeNoException(out);
            return AParcel_writeString(out, c->name.c_str(), static_cast<int32_t>(c->name.size()));
        case CL_SCANNER: {
            if (!c->scanner) c->scanner = newScanner();
            __android_log_print(ANDROID_LOG_INFO, kTag, "getWifiScannerImpl");
            writeNoException(out);
            return AParcel_writeStrongBinder(out, c->scanner);
        }
        case CL_MGMT:
            (void)in;
            return STATUS_OK;  // oneway
        default:
            return STATUS_UNKNOWN_TRANSACTION;
    }
}

AIBinder_Class* gClientClass = nullptr;

AIBinder* newClient(const std::string& name) {
    if (!gClientClass) {
        gClientClass = AIBinder_Class_define("android.net.wifi.nl80211.IClientInterface",
                                             clientCreate, clientDestroy, clientOnTransact);
    }
    auto* state = new ClientState();
    state->name = name;
    return AIBinder_new(gClientClass, state);
}

// ---------- Wificond ----------
struct WificondState {
    AIBinder* client = nullptr;
    std::mutex mu;
};

WificondState* wcOf(AIBinder* b) {
    return static_cast<WificondState*>(AIBinder_getUserData(b));
}

void* wcCreate(void* args) { return args; }

void wcDestroy(void* userData) {
    auto* w = static_cast<WificondState*>(userData);
    if (w->client) AIBinder_decStrong(w->client);
    delete w;
}

bool stdStringAllocator(void* stringData, int32_t length, char** buffer) {
    auto* str = static_cast<std::string*>(stringData);
    if (length < 0) {
        str->clear();
        *buffer = nullptr;
        return true;
    }
    str->resize(static_cast<size_t>(length));
    *buffer = str->empty() ? nullptr : str->data();
    return true;
}

binder_status_t readUtf8String(const AParcel* in, std::string* out) {
    return AParcel_readString(in, out, stdStringAllocator);
}

binder_status_t wcOnTransact(AIBinder* binder, transaction_code_t code, const AParcel* in,
                             AParcel* out) {
    auto* w = wcOf(binder);
    static const int32_t ch2g[] = {2412, 2417, 2422, 2437, 2462};
    static const int32_t ch5g[] = {5180, 5200, 5220, 5745, 5765, 5785};

    switch (code) {
        case WC_CREATE_AP:
            writeNoException(out);
            return AParcel_writeStrongBinder(out, nullptr);
        case WC_CREATE_CLIENT: {
            std::string name;
            binder_status_t st = readUtf8String(in, &name);
            if (st != STATUS_OK) return st;
            if (name.empty()) name = "wlan0";
            tryCreateWlan0();
            std::lock_guard lock(w->mu);
            if (w->client) AIBinder_decStrong(w->client);
            w->client = newClient(name);
            __android_log_print(ANDROID_LOG_INFO, kTag, "createClientInterface(%s)", name.c_str());
            writeNoException(out);
            return AParcel_writeStrongBinder(out, w->client);
        }
        case WC_TEARDOWN_AP:
        case WC_TEARDOWN_CLIENT:
            writeNoException(out);
            return AParcel_writeBool(out, true);
        case WC_TEARDOWN_ALL: {
            std::lock_guard lock(w->mu);
            if (w->client) {
                AIBinder_decStrong(w->client);
                w->client = nullptr;
            }
            return writeNoException(out);
        }
        case WC_GET_CLIENTS: {
            writeNoException(out);
            std::lock_guard lock(w->mu);
            if (w->client) {
                AParcel_writeInt32(out, 1);
                return AParcel_writeStrongBinder(out, w->client);
            }
            return AParcel_writeInt32(out, 0);
        }
        case WC_GET_APS:
            writeNoException(out);
            return AParcel_writeInt32(out, 0);
        case WC_CH_2G:
            return writeIntArray(out, ch2g, 5);
        case WC_CH_5G:
            return writeIntArray(out, ch5g, 6);
        case WC_CH_DFS:
        case WC_CH_6G:
        case WC_CH_60G:
            writeNoException(out);
            return AParcel_writeInt32Array(out, nullptr, 0);
        case WC_REG_IFACE_CB:
        case WC_UNREG_IFACE_CB:
        case WC_REG_EVENT_CB:
        case WC_UNREG_EVENT_CB:
            return STATUS_OK;  // oneway
        case WC_GET_WIPHY:
            writeNoException(out);
            // null DeviceWiphyCapabilities
            return AParcel_writeInt32(out, 0);
        case WC_NOTIFY_CC:
            return STATUS_OK;
        default:
            return STATUS_UNKNOWN_TRANSACTION;
    }
}

}  // namespace

int main() {
    ensureAps();
    tryCreateWlan0();
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();

    static AIBinder_Class* cls = AIBinder_Class_define("android.net.wifi.nl80211.IWificond",
                                                       wcCreate, wcDestroy, wcOnTransact);
    AIBinder* binder = AIBinder_new(cls, new WificondState());
    binder_status_t st = AServiceManager_addService(binder, kService);
    __android_log_print(st == STATUS_OK ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                        "register %s: %d", kService, st);
    AIBinder_decStrong(binder);
    if (st != STATUS_OK) return 1;
    ABinderProcess_joinThreadPool();
    return 1;
}
