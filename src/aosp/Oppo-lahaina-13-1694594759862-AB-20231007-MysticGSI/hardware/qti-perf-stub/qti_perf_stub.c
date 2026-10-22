/*
 * Stub replacement for the ColorOS GSI's
 * /system/system_ext/lib64/libqti_performance.so.
 *
 * boot-QPerformance.art declares 16 native methods on
 * com.qualcomm.qti.Performance and never calls System.loadLibrary, so ART
 * binds them lazily by symbol name.  ZygoteInit.preloadSharedLibraries()
 * calls System.loadLibrary("qti_performance") only under
 * Build.isQcomPlatform(), which is
 * SystemProperties.get("ro.boot.hardware", "unknow").toLowerCase()
 * .startsWith("qcom") -- false here (androidboot.hardware=ranchu, and qcom
 * also activates the SoundTrigger qcom retry loop, see qemu/cmdline/boot).
 * Any call then throws UnsatisfiedLinkError from a system_server thread,
 * which is a "FATAL EXCEPTION IN SYSTEM PROCESS" and soft-reboots the
 * framework (Performance$SendGameLaunchBroadcast, e.g. when the wallpaper
 * picker launches an app).
 *
 * ART does NOT search dlsym(RTLD_DEFAULT): JavaVMExt::FindNativeMethodInternal
 * walks only the JNI libraries registered for the declaring class loader and
 * calls SharedLibrary::FindSymbol -> dlsym(handle_, <mangled name>).  A plain
 * LD_PRELOAD export is therefore invisible to it (verified: the stub was
 * mapped in zygote/system_server, the linkerconfig whitelist was in effect,
 * and the natives still failed).
 *
 * So this library preloads into zygote (LD_PRELOAD in
 * qemu/init/system-overlay/.../init.zygote64_32.rc) and interposes the
 * public dlsym() that libart.so imports as dlsym@LIBC.  At relocation time
 * bionic searches a library's lookup list in this order: the primary
 * namespace's global group (DF_1_GLOBAL libraries) first, then the local
 * group.  The loader force-sets DF_1_GLOBAL on LD_PRELOAD libraries and adds
 * them as secondary members to every initial namespace from ld.config.txt,
 * including "com_android_art", so this definition of dlsym wins over the one
 * in libdl.so (which sits in libart's local group).  The versioned
 * dlsym@LIBC reference still matches because a definer without a verdef
 * table satisfies any version.
 *
 * For the Performance natives the interposer returns the no-op below; every
 * other lookup is delegated to the loader's __loader_dlsym (exported by
 * ld-android.so, reached through libdl.so's DT_NEEDED) with the real caller
 * address, so RTLD_DEFAULT/RTLD_NEXT and namespace selection behave exactly
 * as before.
 *
 * This vendor tree has no QTI perf HAL, so every hint is a no-op.  The
 * library is bind-mounted over the GSI file by the initramfs system-overlay.
 */
#include <jni.h>
#include <stddef.h>

/* The loader entry point behind libdl's dlsym(); declared weak because the
 * NDK's link-time libdl stub does not export it.  At runtime the definition
 * in ld-android.so is found through libdl.so's DT_NEEDED. */
extern void* __loader_dlsym(void* handle, const char* symbol,
                            const void* caller_addr) __attribute__((weak));

/* No-op implementations.  Names are the JNI-mangled forms of the Java
 * declarations in Performance.java (private instance methods, so the second
 * argument is the jobject receiver); return values mirror the real library's
 * "no HAL" behaviour. */

JNIEXPORT void JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1event(JNIEnv* env, jobject self,
                                                      jint a, jstring b,
                                                      jint c, jintArray d) {
    (void)env; (void)self; (void)a; (void)b; (void)c; (void)d;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1get_1feedback(JNIEnv* env,
                                                              jobject self,
                                                              jint a,
                                                              jstring b) {
    (void)env; (void)self; (void)a; (void)b;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1get_1feedback_1extn(
    JNIEnv* env, jobject self, jint a, jstring b, jint c, jintArray d) {
    (void)env; (void)self; (void)a; (void)b; (void)c; (void)d;
    return 0;
}

JNIEXPORT jdouble JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1get_1hal_1ver(JNIEnv* env,
                                                              jobject self) {
    (void)env; (void)self;
    return 0.0;
}

JNIEXPORT jstring JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1get_1prop(JNIEnv* env,
                                                          jobject self,
                                                          jstring name,
                                                          jstring def_value) {
    (void)env; (void)self; (void)name;
    /* Mirror the real library: return the caller's default when unknown. */
    return def_value;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1hint(JNIEnv* env, jobject self,
                                                     jint a, jstring b, jint c,
                                                     jint d) {
    (void)env; (void)self; (void)a; (void)b; (void)c; (void)d;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1hint_1acq_1rel(
    JNIEnv* env, jobject self, jint a, jint b, jstring c, jint d, jint e,
    jint f, jintArray g) {
    (void)env; (void)self; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    (void)g;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1hint_1renew(
    JNIEnv* env, jobject self, jint a, jint b, jstring c, jint d, jint e,
    jint f, jintArray g) {
    (void)env; (void)self; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    (void)g;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1io_1prefetch_1start(
    JNIEnv* env, jobject self, jint a, jstring b, jstring c) {
    (void)env; (void)self; (void)a; (void)b; (void)c;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1io_1prefetch_1stop(
    JNIEnv* env, jobject self) {
    (void)env; (void)self;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1lock_1acq(JNIEnv* env,
                                                          jobject self,
                                                          jint a, jint b,
                                                          jintArray c) {
    (void)env; (void)self; (void)a; (void)b; (void)c;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1lock_1acq_1rel(
    JNIEnv* env, jobject self, jint a, jint b, jint c, jint d, jintArray e) {
    (void)env; (void)self; (void)a; (void)b; (void)c; (void)d; (void)e;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1lock_1rel(JNIEnv* env,
                                                          jobject self,
                                                          jint handle) {
    (void)env; (void)self; (void)handle;
    return 0;
}

JNIEXPORT jstring JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1sync_1request(JNIEnv* env,
                                                              jobject self,
                                                              jint opcode) {
    (void)env; (void)self; (void)opcode;
    return NULL;
}

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1uxEngine_1events(
    JNIEnv* env, jobject self, jint a, jint b, jstring c, jint d) {
    (void)env; (void)self; (void)a; (void)b; (void)c; (void)d;
    return 0;
}

JNIEXPORT jstring JNICALL
Java_com_qualcomm_qti_Performance_native_1perf_1uxEngine_1trigger(
    JNIEnv* env, jobject self, jint opcode) {
    (void)env; (void)self; (void)opcode;
    return NULL;
}

/* Short JNI names as ART builds them: ART first tries
 * Java_<mangled class>_<mangled method>, then the long form with "__" and the
 * argument signature appended. */
struct stub_export {
    const char* name;
    void* fn;
};

#define STUB_EXPORT(fn) { #fn, (void*)(fn) }

static const struct stub_export stub_exports[] = {
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1event),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1get_1feedback),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1get_1feedback_1extn),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1get_1hal_1ver),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1get_1prop),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1hint),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1hint_1acq_1rel),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1hint_1renew),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1io_1prefetch_1start),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1io_1prefetch_1stop),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1lock_1acq),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1lock_1acq_1rel),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1lock_1rel),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1sync_1request),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1uxEngine_1events),
    STUB_EXPORT(Java_com_qualcomm_qti_Performance_native_1perf_1uxEngine_1trigger),
};

static int stub_name_matches(const char* symbol, const char* export_name) {
    size_t i = 0;
    while (export_name[i] != '\0') {
        if (symbol[i] != export_name[i]) return 0;
        i++;
    }
    return symbol[i] == '\0' || (symbol[i] == '_' && symbol[i + 1] == '_');
}

/* Interposes libdl's dlsym for every library whose relocation search reaches
 * this preload's global-group entry (notably libart.so in com_android_art). */
__attribute__((visibility("default"), noinline))
void* dlsym(void* handle, const char* symbol) {
    /* Caller address as seen by the interposed call: this is the code that
     * called dlsym (e.g. libart.so), exactly what libdl would have passed. */
    const void* caller = __builtin_return_address(0);
    if (symbol != NULL) {
        size_t i;
        for (i = 0; i < sizeof(stub_exports) / sizeof(stub_exports[0]); i++) {
            if (stub_name_matches(symbol, stub_exports[i].name)) {
                return stub_exports[i].fn;
            }
        }
    }
    if (__loader_dlsym == NULL) return NULL;
    void* result = __loader_dlsym(handle, symbol, caller);
    /* The build disables sibling calls: the caller address is x30 at entry
     * and must not be clobbered by a tail call. */
    __asm__ __volatile__("" ::: "memory");
    return result;
}
