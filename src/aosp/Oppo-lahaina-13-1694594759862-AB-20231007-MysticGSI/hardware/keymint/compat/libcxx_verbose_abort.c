/*
 * libc++ 15+ routes hardening failures through std::__1::__libcpp_verbose_abort.
 * The Android 13 system libc++.so predates that entry point, but the KeyMint
 * prebuilt (libkeymint_fake_latest.so) references it, so the linker refuses to
 * start the service. Export the symbol; it is only reached on a libc++ abort.
 */
#include <stdlib.h>

void libcxx_verbose_abort_compat(const char* format, ...)
    __asm__("_ZNSt3__122__libcpp_verbose_abortEPKcz");

void libcxx_verbose_abort_compat(const char* format, ...) {
    (void)format;
    abort();
}

/*
 * The Android 16 secureclock/sharedsecret/rkp prebuilts reference this
 * libbinder_ndk entry point (versioned LIBBINDER_NDK36); the Android 13
 * libbinder_ndk does not export it. It only records a transaction-code to
 * name map used when dumping binder state, so a no-op is sufficient. bionic's
 * linker accepts a definition from a DSO without a version table for any
 * requested version, so no version script is needed here.
 */
void AIBinder_Class_setTransactionCodeToFunctionNameMap(void* clazz, const void* map) {
    (void)clazz;
    (void)map;
}
