#pragma once

#include <aidl/android/hardware/common/fmq/MQDescriptor.h>
#include <aidl/android/hardware/common/fmq/SynchronizedReadWrite.h>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <linux/futex.h>
#include <linux/memfd.h>
#include <new>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

// Synchronized libfmq ABI: byte counters, ring storage, EventFlag word.
// The guest framework uses libfmq as the other endpoint. All shared counters
// use acquire/release ordering, and futexes are shared across processes.
template<class T> class Fmq {
    // AIDL @FixedSize unions have generated copy operators but a fixed,
    // pointer-free memory ABI (the same raw representation libfmq transports).
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    int fd_ = -1;
    void* mapping_ = MAP_FAILED;
    size_t bytes_ = 0, size_ = 0, eventOffset_ = 0;
    std::atomic<uint64_t>* read_ = nullptr;
    std::atomic<uint64_t>* write_ = nullptr;
    std::atomic<uint32_t>* event_ = nullptr;
    uint8_t* ring_ = nullptr;
  public:
    using Descriptor = aidl::android::hardware::common::fmq::MQDescriptor<
        T, aidl::android::hardware::common::fmq::SynchronizedReadWrite>;
    Fmq() = default;
    Fmq(const Fmq&) = delete;
    Fmq& operator=(const Fmq&) = delete;
    ~Fmq() { if (mapping_ != MAP_FAILED) munmap(mapping_, size_); if (fd_ >= 0) ::close(fd_); }
    bool initialize(size_t count) {
        if (!count || count > 1024 * 1024 / sizeof(T) || fd_ >= 0) return false;
        bytes_ = count * sizeof(T);
        eventOffset_ = (16 + bytes_ + 7) & ~size_t(7);
        size_ = eventOffset_ + 8;
        fd_ = syscall(__NR_memfd_create, "qemu-audio-fmq", MFD_CLOEXEC);
        if (fd_ < 0 || ftruncate(fd_, size_)) return false;
        mapping_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (mapping_ == MAP_FAILED) return false;
        auto* p = static_cast<uint8_t*>(mapping_);
        read_ = new(p) std::atomic<uint64_t>(0);
        write_ = new(p + 8) std::atomic<uint64_t>(0);
        ring_ = p + 16;
        event_ = new(p + eventOffset_) std::atomic<uint32_t>(0);
        return true;
    }
    bool descriptor(Descriptor* out) const {
        int copy = fcntl(fd_, F_DUPFD_CLOEXEC, 0);
        if (copy < 0) return false;
        out->handle.fds.clear();
        out->handle.fds.emplace_back(copy);
        out->quantum = sizeof(T);
        out->flags = 1; // kSynchronizedReadWrite
        out->grantors.resize(4);
        size_t offsets[] = {0, 8, 16, eventOffset_};
        size_t extents[] = {8, 8, bytes_, 4};
        for (size_t i = 0; i < 4; ++i) {
            out->grantors[i].fdIndex = 0;
            out->grantors[i].offset = offsets[i];
            out->grantors[i].extent = extents[i];
        }
        return true;
    }
    size_t available() const {
        uint64_t r = read_->load(std::memory_order_acquire);
        uint64_t w = write_->load(std::memory_order_acquire);
        return w - r <= bytes_ ? (w - r) / sizeof(T) : 0;
    }
    bool read(T* dest, size_t count = 1) {
        uint64_t r = read_->load(std::memory_order_relaxed);
        uint64_t w = write_->load(std::memory_order_acquire);
        if (count > bytes_ / sizeof(T) || w - r > bytes_ || count * sizeof(T) > w - r) return false;
        size_t n = count * sizeof(T), offset = r % bytes_;
        size_t first = std::min(n, bytes_ - offset);
        memcpy(static_cast<void*>(dest), ring_ + offset, first);
        memcpy(reinterpret_cast<uint8_t*>(dest) + first, ring_, n - first);
        read_->store(r + n, std::memory_order_release);
        wake(1); // FMQ_NOT_FULL
        return true;
    }
    bool write(const T* src, size_t count = 1) {
        uint64_t w = write_->load(std::memory_order_relaxed);
        uint64_t r = read_->load(std::memory_order_acquire);
        if (count > bytes_ / sizeof(T) || w - r > bytes_ || count * sizeof(T) > bytes_ - (w - r)) return false;
        size_t n = count * sizeof(T), offset = w % bytes_;
        size_t first = std::min(n, bytes_ - offset);
        memcpy(ring_ + offset, src, first);
        memcpy(ring_, reinterpret_cast<const uint8_t*>(src) + first, n - first);
        write_->store(w + n, std::memory_order_release);
        wake(2); // FMQ_NOT_EMPTY
        return true;
    }
    void wake(uint32_t bits) {
        if (!event_) return;
        event_->fetch_or(bits, std::memory_order_release);
        syscall(__NR_futex, event_, FUTEX_WAKE_BITSET, INT_MAX, nullptr, nullptr, bits);
    }
    void waitReadable() {
        if (available()) return;
        if (event_->fetch_and(~2U, std::memory_order_acq_rel) & 2U) return;
        uint32_t expected = event_->load(std::memory_order_acquire);
        if (expected & 2U) return;
        timespec deadline;
        clock_gettime(CLOCK_MONOTONIC, &deadline);
        deadline.tv_nsec += 100000000; // bounded so closing never hangs
        if (deadline.tv_nsec >= 1000000000) { ++deadline.tv_sec; deadline.tv_nsec -= 1000000000; }
        syscall(__NR_futex, event_, FUTEX_WAIT_BITSET, expected, &deadline, nullptr, 2U);
    }
};
