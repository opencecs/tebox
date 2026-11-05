#pragma once

#include <aidl/android/hardware/media/c2/BnConfigurable.h>

#include <mutex>
#include <string>

namespace gki {

class Configurable : public aidl::android::hardware::media::c2::BnConfigurable {
public:
    // |decoder|: a decoder of |mime| (bitstream in, RGBA pictures out);
    // otherwise the AVC encoder (pictures in, bitstream out).
    Configurable(int32_t id, std::string name, bool decoder = false,
                 std::string mime = "video/avc");

    ndk::ScopedAStatus config(
            const aidl::android::hardware::media::c2::Params& inParams, bool mayBlock,
            aidl::android::hardware::media::c2::IConfigurable::ConfigResult* _aidl_return)
            override;
    ndk::ScopedAStatus getId(int32_t* _aidl_return) override;
    ndk::ScopedAStatus getName(std::string* _aidl_return) override;
    ndk::ScopedAStatus query(
            const std::vector<int32_t>& indices, bool mayBlock,
            aidl::android::hardware::media::c2::IConfigurable::QueryResult* _aidl_return)
            override;
    ndk::ScopedAStatus querySupportedParams(
            int32_t start, int32_t count,
            std::vector<aidl::android::hardware::media::c2::ParamDescriptor>* _aidl_return)
            override;
    ndk::ScopedAStatus querySupportedValues(
            const std::vector<aidl::android::hardware::media::c2::FieldSupportedValuesQuery>&
                    inFields,
            bool mayBlock,
            aidl::android::hardware::media::c2::IConfigurable::QuerySupportedValuesResult*
                    _aidl_return) override;

    int32_t id() const { return mId; }
    uint32_t width() const;
    uint32_t height() const;
    void setSize(uint32_t w, uint32_t h);

private:
    const int32_t mId;
    const std::string mName;
    const bool mDecoder;
    const std::string mMime;
    mutable std::mutex mLock;
    uint32_t mWidth = 1080;
    uint32_t mHeight = 2400;
};

}  // namespace gki
