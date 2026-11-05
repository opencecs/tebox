/*
 **
 ** Copyright 2019, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef HIDL_android_hardware_keymaster_V4_1_AndroidKeymaster4Device_H_
#define HIDL_android_hardware_keymaster_V4_1_AndroidKeymaster4Device_H_

#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>
#include <android/hardware/keymaster/4.1/types.h>
#include <hidl/Status.h>

#include "AndroidKeymaster4Device.h"

namespace keymaster {
class AndroidKeymaster;
class KeymasterContext;

namespace V4_1 {

using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::android::hardware::keymaster::V4_0::ErrorCode;
using ::android::hardware::keymaster::V4_0::HardwareAuthenticatorType;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;
using ::android::hardware::keymaster::V4_0::HmacSharingParameters;
using ::android::hardware::keymaster::V4_0::KeyCharacteristics;
using ::android::hardware::keymaster::V4_0::KeyFormat;
using ::android::hardware::keymaster::V4_0::KeyParameter;
using ::android::hardware::keymaster::V4_0::KeyPurpose;
using ::android::hardware::keymaster::V4_0::OperationHandle;
using ::android::hardware::keymaster::V4_0::SecurityLevel;
using ::android::hardware::keymaster::V4_0::VerificationToken;
using ::android::hardware::keymaster::V4_1::IKeymasterDevice;
using ::android::hardware::keymaster::V4_1::Tag;

using V41ErrorCode = ::android::hardware::keymaster::V4_1::ErrorCode;

inline V41ErrorCode convert(ErrorCode error_code) {
    return static_cast<V41ErrorCode>(error_code);
}

inline ErrorCode convert(V41ErrorCode error_code) {
    return static_cast<ErrorCode>(error_code);
}

class AndroidKeymaster41Device : public IKeymasterDevice, public V4_0::ng::AndroidKeymaster4Device {
    using super = V4_0::ng::AndroidKeymaster4Device;

  public:
    explicit AndroidKeymaster41Device(SecurityLevel securityLevel)
        : super(KmVersion::KEYMASTER_4_1, securityLevel) {}
    virtual ~AndroidKeymaster41Device() {}

    Return<V41ErrorCode> deviceLocked(bool /* passwordOnly */,
                                      const VerificationToken& /* verificationToken */) override;
    Return<V41ErrorCode> earlyBootEnded() override;

    Return<void> getHardwareInfo(super::getHardwareInfo_cb _hidl_cb) override {
        return super::getHardwareInfo(_hidl_cb);
    }

    Return<void> getHmacSharingParameters(super::getHmacSharingParameters_cb _hidl_cb) override {
        return super::getHmacSharingParameters(_hidl_cb);
    }

    Return<void> computeSharedHmac(const hidl_vec<HmacSharingParameters>& params,
                                   super::computeSharedHmac_cb _hidl_cb) override {
        return super::computeSharedHmac(params, _hidl_cb);
    }

    Return<void> verifyAuthorization(uint64_t challenge,
                                     const hidl_vec<KeyParameter>& parametersToVerify,
                                     const HardwareAuthToken& authToken,
                                     super::verifyAuthorization_cb _hidl_cb) override {
        return super::verifyAuthorization(challenge, parametersToVerify, authToken, _hidl_cb);
    }

    Return<ErrorCode> addRngEntropy(const hidl_vec<uint8_t>& data) override {
        return super::addRngEntropy(data);
    }

    Return<void> generateKey(const hidl_vec<KeyParameter>& keyParams,
                             super::generateKey_cb _hidl_cb) override {
        return super::generateKey(keyParams, _hidl_cb);
    }

    Return<void> getKeyCharacteristics(const hidl_vec<uint8_t>& keyBlob,
                                       const hidl_vec<uint8_t>& clientId,
                                       const hidl_vec<uint8_t>& appData,
                                       super::getKeyCharacteristics_cb _hidl_cb) override {
        return super::getKeyCharacteristics(keyBlob, clientId, appData, _hidl_cb);
    }

    Return<void> importKey(const hidl_vec<KeyParameter>& params, KeyFormat keyFormat,
                           const hidl_vec<uint8_t>& keyData,
                           super::importKey_cb _hidl_cb) override {
        return super::importKey(params, keyFormat, keyData, _hidl_cb);
    }

    Return<void> importWrappedKey(const hidl_vec<uint8_t>& wrappedKeyData,
                                  const hidl_vec<uint8_t>& wrappingKeyBlob,
                                  const hidl_vec<uint8_t>& maskingKey,
                                  const hidl_vec<KeyParameter>& unwrappingParams,
                                  uint64_t passwordSid, uint64_t biometricSid,
                                  super::importWrappedKey_cb _hidl_cb) override {
        return super::importWrappedKey(wrappedKeyData, wrappingKeyBlob, maskingKey,
                                       unwrappingParams, passwordSid, biometricSid, _hidl_cb);
    }

    Return<void> exportKey(KeyFormat exportFormat, const hidl_vec<uint8_t>& keyBlob,
                           const hidl_vec<uint8_t>& clientId, const hidl_vec<uint8_t>& appData,
                           super::exportKey_cb _hidl_cb) override {
        return super::exportKey(exportFormat, keyBlob, clientId, appData, _hidl_cb);
    }

    Return<void> attestKey(const hidl_vec<uint8_t>& keyToAttest,
                           const hidl_vec<KeyParameter>& attestParams,
                           super::attestKey_cb _hidl_cb) override {
        return super::attestKey(keyToAttest, attestParams, _hidl_cb);
    }

    Return<void> upgradeKey(const hidl_vec<uint8_t>& keyBlobToUpgrade,
                            const hidl_vec<KeyParameter>& upgradeParams,
                            super::upgradeKey_cb _hidl_cb) override {
        return super::upgradeKey(keyBlobToUpgrade, upgradeParams, _hidl_cb);
    }

    Return<ErrorCode> deleteKey(const hidl_vec<uint8_t>& keyBlob) override {
        return super::deleteKey(keyBlob);
    }

    Return<ErrorCode> deleteAllKeys() override { return super::deleteAllKeys(); }

    Return<ErrorCode> destroyAttestationIds() override { return super::destroyAttestationIds(); }

    Return<void> begin(KeyPurpose purpose, const hidl_vec<uint8_t>& key,
                       const hidl_vec<KeyParameter>& inParams, const HardwareAuthToken& authToken,
                       super::begin_cb _hidl_cb) override {
        return super::begin(purpose, key, inParams, authToken, _hidl_cb);
    }

    Return<void> update(uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
                        const hidl_vec<uint8_t>& input, const HardwareAuthToken& authToken,
                        const VerificationToken& verificationToken,
                        super::update_cb _hidl_cb) override {
        return super::update(operationHandle, inParams, input, authToken, verificationToken,
                             _hidl_cb);
    }

    Return<void> finish(uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
                        const hidl_vec<uint8_t>& input, const hidl_vec<uint8_t>& signature,
                        const HardwareAuthToken& authToken,
                        const VerificationToken& verificationToken,
                        super::finish_cb _hidl_cb) override {
        return super::finish(operationHandle, inParams, input, signature, authToken,
                             verificationToken, _hidl_cb);
    }

    Return<ErrorCode> abort(uint64_t operationHandle) override {
        return super::abort(operationHandle);
    }
};

IKeymasterDevice* CreateKeymasterDevice(SecurityLevel securityLevel);

}  // namespace V4_1
}  // namespace keymaster

#endif  // HIDL_android_hardware_keymaster_V4_1_AndroidKeymaster4Device_H_
