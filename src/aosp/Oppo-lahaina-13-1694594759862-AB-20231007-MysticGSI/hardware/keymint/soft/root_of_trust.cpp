// Report a locked, verified device in key attestation, as TrickyStore does.
// The GSI's PureSoftKeymasterContext hard-codes an unlocked, unverified boot
// with all-zero key and hash. This launcher exports the same symbols, and the
// library's vtable resolves them here (as with persistent_storage.cpp).
#include <sys/system_properties.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace {
// keymaster::AttestationContext::VerifiedBootParams (attestation_context.h),
// spelled out: that header pulls in the legacy hardware/ headers.
struct Blob {
    const uint8_t* data;
    size_t data_length;
};
struct Params {
    Blob verified_boot_key;
    Blob verified_boot_hash;
    int verified_boot_state;  // keymaster_verified_boot_t
    bool device_locked;
};
constexpr int KM_VERIFIED_BOOT_VERIFIED = 0;
constexpr int KM_ERROR_OK = 0;
using keymaster_error_t = int;

uint8_t key[32];
uint8_t hash[32];
Params params;
std::once_flag once;

// Stable per instance: TrickyStore uses random bytes; derive them from the
// serial number instead so the key stays the same across reboots.
void expand(const char* label, const char* seed, uint8_t out[32]) {
    uint64_t state = 14695981039346656037ULL;
    for (const char* s : {label, seed}) {
        for (; *s; ++s) state = (state ^ static_cast<uint8_t>(*s)) * 1099511628211ULL;
    }
    for (int i = 0; i < 4; ++i) {  // splitmix64
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        z ^= z >> 31;
        memcpy(out + i * 8, &z, 8);
    }
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// ro.boot.vbmeta.digest when the bootloader provides a real one.
bool vbmetaDigest(uint8_t out[32]) {
    char value[PROP_VALUE_MAX] = {};
    if (__system_property_get("ro.boot.vbmeta.digest", value) != 64) return false;
    bool zero = true;
    for (int i = 0; i < 32; ++i) {
        int hi = hexValue(value[2 * i]), lo = hexValue(value[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = static_cast<uint8_t>(hi << 4 | lo);
        zero = zero && out[i] == 0;
    }
    return !zero;
}

void init() {
    char serial[PROP_VALUE_MAX] = {};
    if (__system_property_get("ro.serialno", serial) <= 0)
        __system_property_get("ro.boot.serialno", serial);
    expand("tebox verified boot key", serial, key);
    if (!vbmetaDigest(hash)) expand("tebox vbmeta digest", serial, hash);
    params.verified_boot_key = {key, sizeof(key)};
    params.verified_boot_hash = {hash, sizeof(hash)};
    params.verified_boot_state = KM_VERIFIED_BOOT_VERIFIED;
    params.device_locked = true;
}

const Params* verifiedBootParams(keymaster_error_t* error) {
    std::call_once(once, init);
    *error = KM_ERROR_OK;
    return &params;
}
}  // namespace

// PureSoftKeymasterContext::GetVerifiedBootParams and its AttestationContext
// thunk. Neither uses |this|.
extern "C" const Params* teboxGetVerifiedBootParams(const void*, keymaster_error_t* error) __asm__(
        "_ZNK9keymaster24PureSoftKeymasterContext21GetVerifiedBootParamsEP17keymaster_error_t");
extern "C" const Params* teboxGetVerifiedBootParamsThunk(const void*, keymaster_error_t* error) __asm__(
        "_ZThn16_NK9keymaster24PureSoftKeymasterContext21GetVerifiedBootParamsEP17keymaster_error_t");

const Params* teboxGetVerifiedBootParams(const void*, keymaster_error_t* error) {
    return verifiedBootParams(error);
}

const Params* teboxGetVerifiedBootParamsThunk(const void*, keymaster_error_t* error) {
    return verifiedBootParams(error);
}
