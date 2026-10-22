#pragma once

#include <aidl/android/hardware/media/c2/BnComponentInterface.h>

#include <memory>
#include <string>

namespace gki {

class Configurable;

class ComponentInterface : public aidl::android::hardware::media::c2::BnComponentInterface {
public:
    ComponentInterface(int32_t id, std::string name,
                       std::shared_ptr<Configurable> configurable);

    ndk::ScopedAStatus getConfigurable(
            std::shared_ptr<aidl::android::hardware::media::c2::IConfigurable>* _aidl_return)
            override;

    std::shared_ptr<Configurable> configurable() const { return mConfigurable; }

private:
    const int32_t mId;
    const std::string mName;
    std::shared_ptr<Configurable> mConfigurable;
};

}  // namespace gki
