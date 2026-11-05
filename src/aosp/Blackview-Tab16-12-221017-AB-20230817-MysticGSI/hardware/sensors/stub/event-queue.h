#pragma once

#include <aidl/android/hardware/common/fmq/MQDescriptor.h>
#include <aidl/android/hardware/common/fmq/SynchronizedReadWrite.h>
#include <algorithm>
#include <atomic>
#include <climits>
#include <cstring>
#include <linux/futex.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

// Import the framework's synchronized FMQ. Grantors can live in separate FDs.
// Counters measure bytes, and EventFlag wakes use shared FUTEX_WAKE_BITSET.
template<class T> class EventQueue {
    struct Region { void* address; size_t size; };
    std::vector<Region> regions_;
    std::atomic<uint64_t>* read_ = nullptr;
    std::atomic<uint64_t>* write_ = nullptr;
    std::atomic<uint32_t>* flag_ = nullptr;
    uint8_t* ring_ = nullptr;
    size_t bytes_ = 0;

  public:
    using Descriptor = aidl::android::hardware::common::fmq::MQDescriptor<
        T, aidl::android::hardware::common::fmq::SynchronizedReadWrite>;
    EventQueue() = default;
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;
    ~EventQueue() {
        for (const auto& region : regions_) munmap(region.address, region.size);
    }

    bool initialize(const Descriptor& desc) {
        static_assert(std::atomic<uint64_t>::is_always_lock_free);
        if (!regions_.empty() || desc.quantum != sizeof(T) || desc.flags != 1 ||
            desc.grantors.size() != 4) return false;
        const auto& grantors = desc.grantors;
        if (grantors[0].extent != 8 || grantors[1].extent != 8 ||
            grantors[2].extent < sizeof(T) || grantors[2].extent % sizeof(T) ||
            grantors[3].extent < 4) return false;
        void* pointers[4];
        const size_t page = sysconf(_SC_PAGESIZE);
        for (size_t i = 0; i < 4; ++i) {
            const auto& grantor = grantors[i];
            if (grantor.fdIndex < 0 || size_t(grantor.fdIndex) >= desc.handle.fds.size() ||
                grantor.offset < 0 || grantor.offset % 8 || grantor.extent <= 0 ||
                grantor.extent > 16 * 1024 * 1024) return false;
            int fd = desc.handle.fds[grantor.fdIndex].get();
            struct stat st;
            if (fstat(fd, &st) || st.st_size < int64_t(grantor.offset) + grantor.extent)
                return false;
            size_t offset = size_t(grantor.offset) / page * page;
            size_t delta = size_t(grantor.offset) - offset;
            size_t size = delta + size_t(grantor.extent);
            void* address = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
            if (address == MAP_FAILED) return false;
            regions_.push_back({address, size});
            pointers[i] = static_cast<uint8_t*>(address) + delta;
        }
        read_ = static_cast<std::atomic<uint64_t>*>(pointers[0]);
        write_ = static_cast<std::atomic<uint64_t>*>(pointers[1]);
        ring_ = static_cast<uint8_t*>(pointers[2]);
        flag_ = static_cast<std::atomic<uint32_t>*>(pointers[3]);
        bytes_ = size_t(grantors[2].extent);
        read_->store(0, std::memory_order_release);
        write_->store(0, std::memory_order_release);
        return true;
    }

    // The HAL serializes all writers, including flush, with its service mutex.
    bool write(const T& event, uint32_t wakeBits) {
        if (!write_) return false;
        uint64_t w = write_->load(std::memory_order_relaxed);
        uint64_t r = read_->load(std::memory_order_acquire);
        if (w - r > bytes_ || bytes_ - (w - r) < sizeof(T)) return false;
        size_t offset = w % bytes_;
        size_t first = std::min(sizeof(T), bytes_ - offset);
        memcpy(ring_ + offset, &event, first);
        memcpy(ring_, reinterpret_cast<const uint8_t*>(&event) + first, sizeof(T) - first);
        write_->store(w + sizeof(T), std::memory_order_release);
        flag_->fetch_or(wakeBits, std::memory_order_release);
        syscall(__NR_futex, flag_, FUTEX_WAKE_BITSET, INT_MAX, nullptr, nullptr, wakeBits);
        return true;
    }
};
