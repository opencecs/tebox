#include "ComponentInterface.h"

#include "Configurable.h"

using ndk::ScopedAStatus;

namespace gki {

ComponentInterface::ComponentInterface(int32_t id, std::string name,
                                       std::shared_ptr<Configurable> configurable)
    : mId(id), mName(std::move(name)), mConfigurable(std::move(configurable)) {}

ScopedAStatus ComponentInterface::getConfigurable(
        std::shared_ptr<aidl::android::hardware::media::c2::IConfigurable>* out) {
    *out = mConfigurable;
    return ScopedAStatus::ok();
}

}  // namespace gki
