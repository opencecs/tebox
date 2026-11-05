#pragma once

#include <aidl/android/hardware/media/c2/BnComponentStore.h>

#include "MesaAvcComponent.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace gki {

class Configurable;
class MesaAvcComponent;

class ComponentStore : public aidl::android::hardware::media::c2::BnComponentStore {
public:
    ComponentStore();

    ndk::ScopedAStatus copyBuffer(const aidl::android::hardware::media::c2::Buffer& src,
                                  const aidl::android::hardware::media::c2::Buffer& dst) override;
    ndk::ScopedAStatus createComponent(
            const std::string& name,
            const std::shared_ptr<aidl::android::hardware::media::c2::IComponentListener>& listener,
            const std::shared_ptr<
                    aidl::android::hardware::media::bufferpool2::IClientManager>& pool,
            std::shared_ptr<aidl::android::hardware::media::c2::IComponent>* _aidl_return)
            override;
    ndk::ScopedAStatus createInterface(
            const std::string& name,
            std::shared_ptr<aidl::android::hardware::media::c2::IComponentInterface>*
                    _aidl_return) override;
    ndk::ScopedAStatus getConfigurable(
            std::shared_ptr<aidl::android::hardware::media::c2::IConfigurable>* _aidl_return)
            override;
    ndk::ScopedAStatus getPoolClientManager(
            std::shared_ptr<aidl::android::hardware::media::bufferpool2::IClientManager>*
                    _aidl_return) override;
    ndk::ScopedAStatus getStructDescriptors(
            const std::vector<int32_t>& indices,
            std::vector<aidl::android::hardware::media::c2::StructDescriptor>* _aidl_return)
            override;
    ndk::ScopedAStatus listComponents(
            std::vector<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits>*
                    _aidl_return) override;
    ndk::ScopedAStatus createInputSurface(
            std::shared_ptr<aidl::android::hardware::media::c2::IInputSurface>* _aidl_return)
            override;

private:
    struct ComponentDesc {
        aidl::android::hardware::media::c2::IComponentStore::ComponentTraits traits;
        MesaCodecKind kind;
        uint32_t hostCodec = 0;  // V4L2 fourcc: decoded by the host (HostDecComponent)
    };

    std::shared_ptr<Configurable> storeConfigurable();
    void addHostDecoders();
    bool hasComponent(const std::string& name);
    std::shared_ptr<Configurable> interfaceFor(const std::string& name);

    std::mutex mLock;
    std::shared_ptr<Configurable> mStoreCfg;
    int32_t mNextId = 1;
    std::map<std::string, std::shared_ptr<Configurable>> mInterfaces;
    std::map<std::string, ComponentDesc> mCatalog;  // guarded by mLock
    bool mHostDecoders = false;
};

}  // namespace gki
