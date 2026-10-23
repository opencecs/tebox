// Temporary diagnostic: run the guest's own libvintf against the vendor
// manifest files. Linked (not dlopen'd) so the std::__1 ABI matches.
//
// Usage: vintf-probe [file.xml ...]
//   - reports whether VintfObject can produce a device manifest
//   - reports whether the graphics/keymint AIDL instances are declared
//   - for each file argument, prints fromXml() success and the exact error
#include <vintf/FileSystem.h>
#include <vintf/HalManifest.h>
#include <vintf/VintfObject.h>
#include <vintf/parse_xml.h>

#include <cstdio>
#include <string>

using namespace android::vintf;

static void checkAidl(const HalManifest& m, const char* pkg, const char* iface,
                      const char* inst) {
    printf("aidl %s.%s/%s -> %d\n", pkg, iface, inst,
           m.hasAidlInstance(pkg, iface, inst) ? 1 : 0);
}

int main(int argc, char** argv) {
    auto vintf = VintfObject::GetInstance();
    printf("vintf_object=%s\n", vintf ? "ok" : "NULL");
    if (vintf) {
        auto dm = vintf->getDeviceHalManifest();
        printf("device_manifest=%s\n", dm ? "ok" : "NULL");
        if (dm) {
            checkAidl(*dm, "android.hardware.graphics.composer3", "IComposer", "default");
            checkAidl(*dm, "android.hardware.graphics.allocator", "IAllocator", "default");
            checkAidl(*dm, "android.hardware.security.keymint", "IKeyMintDevice", "default");
            std::string xml = toXml(*dm);
            printf("---device-manifest---\n%s\n---end---\n", xml.c_str());
        }
        auto fm = vintf->getFrameworkHalManifest();
        printf("framework_manifest=%s\n", fm ? "ok" : "NULL");
    }

    details::FileSystemImpl fs;
    for (int i = 1; i < argc; ++i) {
        std::string content;
        std::string ferr;
        int st = fs.fetch(argv[i], &content, &ferr);
        printf("file %s fetch=%d err=[%s]\n", argv[i], st, ferr.c_str());
        if (st != 0) continue;
        HalManifest m;
        std::string perr;
        bool ok = fromXml(&m, content, &perr);
        printf("  fromXml=%d error=[%s]\n", ok ? 1 : 0, perr.c_str());
    }
    printf("probe done\n");
    return 0;
}
