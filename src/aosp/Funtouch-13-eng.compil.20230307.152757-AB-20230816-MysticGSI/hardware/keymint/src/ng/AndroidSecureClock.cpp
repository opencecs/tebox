/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.security.secureclock-impl"
#include <log/log.h>

#include "AndroidSecureClock.h"

#include <aidl/android/hardware/security/keymint/ErrorCode.h>

#include "KeyMintUtils.h"
#include <keymaster/android_keymaster.h>
#include <keymaster/keymaster_configuration.h>

namespace aidl::android::hardware::security::secureclock {

using keymaster::GenerateTimestampTokenRequest;
using keymaster::GenerateTimestampTokenResponse;
using keymint::km_utils::kmBlob2vector;
using keymint::km_utils::kmError2ScopedAStatus;

AndroidSecureClock::AndroidSecureClock(
    const std::shared_ptr<keymint::AndroidKeyMintDevice>& keymint)
    : impl_(keymint->getKeymasterImpl()) {}

AndroidSecureClock::~AndroidSecureClock() {}

ScopedAStatus AndroidSecureClock::generateTimeStamp(int64_t challenge, TimeStampToken* token) {
    GenerateTimestampTokenRequest request(impl_->message_version());
    request.challenge = challenge;
    GenerateTimestampTokenResponse response(request.message_version);
    impl_->GenerateTimestampToken(request, &response);
    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }
    token->challenge = response.token.challenge;
    token->timestamp.milliSeconds = static_cast<int64_t>(response.token.timestamp);
    token->mac = kmBlob2vector(response.token.mac);
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::security::secureclock
