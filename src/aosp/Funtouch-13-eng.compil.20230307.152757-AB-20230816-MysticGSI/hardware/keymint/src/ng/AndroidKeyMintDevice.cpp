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

#define LOG_TAG "android.hardware.security.keymint-impl"
#include <android-base/logging.h>

#include "AndroidKeyMintDevice.h"

#include <aidl/android/hardware/security/keymint/ErrorCode.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/keymaster_configuration.h>

#include "AndroidKeyMintOperation.h"
#include "KeyMintUtils.h"
#include "../KeyboxSoftKeymasterContext.h"

namespace aidl::android::hardware::security::keymint {

using namespace keymaster;  // NOLINT(google-build-using-namespace)

using km_utils::authToken2AidlVec;
using km_utils::kmBlob2vector;
using km_utils::kmError2ScopedAStatus;
using km_utils::kmParam2Aidl;
using km_utils::KmParamSet;
using km_utils::kmParamSet2Aidl;
using km_utils::legacy_enum_conversion;
using secureclock::TimeStampToken;

namespace {

vector<KeyCharacteristics> convertKeyCharacteristics(SecurityLevel keyMintSecurityLevel,
                                                     const AuthorizationSet& requestParams,
                                                     const AuthorizationSet& sw_enforced,
                                                     const AuthorizationSet& hw_enforced,
                                                     bool include_keystore_enforced = true) {
    KeyCharacteristics keyMintEnforced{keyMintSecurityLevel, {}};

    if (keyMintSecurityLevel != SecurityLevel::SOFTWARE) {
        // We're pretending to be TRUSTED_ENVIRONMENT or STRONGBOX.
        keyMintEnforced.authorizations = kmParamSet2Aidl(hw_enforced);
        if (include_keystore_enforced && !sw_enforced.empty()) {
            // Put all the software authorizations in the keystore list.
            KeyCharacteristics keystoreEnforced{SecurityLevel::KEYSTORE,
                                                kmParamSet2Aidl(sw_enforced)};
            return {std::move(keyMintEnforced), std::move(keystoreEnforced)};
        } else {
            return {std::move(keyMintEnforced)};
        }
    }

    KeyCharacteristics keystoreEnforced{SecurityLevel::KEYSTORE, {}};
    CHECK(hw_enforced.empty()) << "Hardware-enforced list is non-empty for pure SW KeyMint";

    // This is a pure software implementation, so all tags are in sw_enforced.
    // We need to walk through the SW-enforced list and figure out which tags to
    // return in the software list and which in the keystore list.

    for (auto& entry : sw_enforced) {
        switch (entry.tag) {
        /* Invalid and unused */
        case KM_TAG_ECIES_SINGLE_HASH_MODE:
        case KM_TAG_INVALID:
        case KM_TAG_KDF:
        case KM_TAG_ROLLBACK_RESISTANCE:
            CHECK(false) << "We shouldn't see tag " << entry.tag;
            break;

        /* Unimplemented */
        case KM_TAG_ALLOW_WHILE_ON_BODY:
        case KM_TAG_BOOTLOADER_ONLY:
        case KM_TAG_ROLLBACK_RESISTANT:
        case KM_TAG_STORAGE_KEY:
            break;

        /* Keystore-enforced if not locally generated. */
        case KM_TAG_CREATION_DATETIME:
            // A KeyMaster implementation is required to add this tag to generated/imported keys.
            // A KeyMint implementation is not required to create this tag, only to echo it back if
            // it was included in the key generation/import request.
            if (requestParams.Contains(KM_TAG_CREATION_DATETIME)) {
                keystoreEnforced.authorizations.push_back(kmParam2Aidl(entry));
            }
            break;

        /* Disallowed in KeyCharacteristics */
        case KM_TAG_APPLICATION_DATA:
        case KM_TAG_ATTESTATION_APPLICATION_ID:
            break;

        /* Not key characteristics */
        case KM_TAG_ASSOCIATED_DATA:
        case KM_TAG_ATTESTATION_CHALLENGE:
        case KM_TAG_ATTESTATION_ID_BRAND:
        case KM_TAG_ATTESTATION_ID_DEVICE:
        case KM_TAG_ATTESTATION_ID_IMEI:
        case KM_TAG_ATTESTATION_ID_SECOND_IMEI:
        case KM_TAG_ATTESTATION_ID_MANUFACTURER:
        case KM_TAG_ATTESTATION_ID_MEID:
        case KM_TAG_ATTESTATION_ID_MODEL:
        case KM_TAG_ATTESTATION_ID_PRODUCT:
        case KM_TAG_ATTESTATION_ID_SERIAL:
        case KM_TAG_AUTH_TOKEN:
        case KM_TAG_CERTIFICATE_SERIAL:
        case KM_TAG_CERTIFICATE_SUBJECT:
        case KM_TAG_CERTIFICATE_NOT_AFTER:
        case KM_TAG_CERTIFICATE_NOT_BEFORE:
        case KM_TAG_CONFIRMATION_TOKEN:
        case KM_TAG_DEVICE_UNIQUE_ATTESTATION:
        case KM_TAG_IDENTITY_CREDENTIAL_KEY:
        case KM_TAG_INCLUDE_UNIQUE_ID:
        case KM_TAG_MAC_LENGTH:
        case KM_TAG_NONCE:
        case KM_TAG_RESET_SINCE_ID_ROTATION:
        case KM_TAG_ROOT_OF_TRUST:
        case KM_TAG_UNIQUE_ID:
        case KM_TAG_MODULE_HASH:
            break;

        /* KeyMint-enforced */
        case KM_TAG_ALGORITHM:
        case KM_TAG_APPLICATION_ID:
        case KM_TAG_AUTH_TIMEOUT:
        case KM_TAG_BLOB_USAGE_REQUIREMENTS:
        case KM_TAG_BLOCK_MODE:
        case KM_TAG_BOOT_PATCHLEVEL:
        case KM_TAG_CALLER_NONCE:
        case KM_TAG_DIGEST:
        case KM_TAG_EARLY_BOOT_ONLY:
        case KM_TAG_EC_CURVE:
        case KM_TAG_EXPORTABLE:
        case KM_TAG_KEY_SIZE:
        case KM_TAG_MAX_USES_PER_BOOT:
        case KM_TAG_MIN_MAC_LENGTH:
        case KM_TAG_MIN_SECONDS_BETWEEN_OPS:
        case KM_TAG_NO_AUTH_REQUIRED:
        case KM_TAG_ORIGIN:
        case KM_TAG_OS_PATCHLEVEL:
        case KM_TAG_OS_VERSION:
        case KM_TAG_PADDING:
        case KM_TAG_PURPOSE:
        case KM_TAG_RSA_OAEP_MGF_DIGEST:
        case KM_TAG_RSA_PUBLIC_EXPONENT:
        case KM_TAG_TRUSTED_CONFIRMATION_REQUIRED:
        case KM_TAG_TRUSTED_USER_PRESENCE_REQUIRED:
        case KM_TAG_UNLOCKED_DEVICE_REQUIRED:
        case KM_TAG_USER_AUTH_TYPE:
        case KM_TAG_USER_SECURE_ID:
        case KM_TAG_VENDOR_PATCHLEVEL:
            keyMintEnforced.authorizations.push_back(kmParam2Aidl(entry));
            break;

        /* Keystore-enforced */
        case KM_TAG_ACTIVE_DATETIME:
        case KM_TAG_ALL_APPLICATIONS:
        case KM_TAG_ALL_USERS:
        case KM_TAG_MAX_BOOT_LEVEL:
        case KM_TAG_ORIGINATION_EXPIRE_DATETIME:
        case KM_TAG_USAGE_EXPIRE_DATETIME:
        case KM_TAG_USER_ID:
        case KM_TAG_USAGE_COUNT_LIMIT:
            keystoreEnforced.authorizations.push_back(kmParam2Aidl(entry));
            break;
        }
    }

    vector<KeyCharacteristics> retval;
    retval.reserve(2);
    if (!keyMintEnforced.authorizations.empty()) retval.push_back(std::move(keyMintEnforced));
    if (include_keystore_enforced && !keystoreEnforced.authorizations.empty()) {
        retval.push_back(std::move(keystoreEnforced));
    }

    return retval;
}

Certificate convertCertificate(const keymaster_blob_t& cert) {
    return {std::vector<uint8_t>(cert.data, cert.data + cert.data_length)};
}

vector<Certificate> convertCertificateChain(const CertificateChain& chain) {
    vector<Certificate> retval;
    retval.reserve(chain.entry_count);
    std::transform(chain.begin(), chain.end(), std::back_inserter(retval), convertCertificate);
    return retval;
}

void addClientAndAppData(const std::vector<uint8_t>& appId, const std::vector<uint8_t>& appData,
                         ::keymaster::AuthorizationSet* params) {
    params->Clear();
    if (appId.size()) {
        params->push_back(::keymaster::TAG_APPLICATION_ID, appId.data(), appId.size());
    }
    if (appData.size()) {
        params->push_back(::keymaster::TAG_APPLICATION_DATA, appData.data(), appData.size());
    }
}

}  // namespace

constexpr size_t kOperationTableSize = 16;

AndroidKeyMintDevice::AndroidKeyMintDevice(SecurityLevel securityLevel)
    : impl_(new(std::nothrow)::keymaster::AndroidKeymaster(
          [&]() -> auto {
              auto context = new (std::nothrow) KeyboxSoftKeymasterContext(
                  KmVersion::KEYMINT_4, static_cast<keymaster_security_level_t>(securityLevel));
              context->SetSystemVersion(::keymaster::GetOsVersion(),
                                        ::keymaster::GetOsPatchlevel());
              context->SetVendorPatchlevel(::keymaster::GetVendorPatchlevel());
              // Software devices cannot be configured by the boot loader but they have
              // to return a boot patch level. So lets just return the OS patch level.
              // The OS patch level only has a year and a month so we just add the 1st
              // of the month as day field.
              context->SetBootPatchlevel(GetOsPatchlevel() * 100 + 1);
              auto digest = ::keymaster::GetVbmetaDigest();
              if (digest) {
                  std::string bootState = ::keymaster::GetVerifiedBootState();
                  std::string bootloaderState = ::keymaster::GetBootloaderState();
                  context->SetVerifiedBootInfo(bootState, bootloaderState, *digest);
              } else {
                  LOG(ERROR) << "Unable to read vb_meta digest";
              }
              return context;
          }(),
          kOperationTableSize)),
      securityLevel_(securityLevel) {}

AndroidKeyMintDevice::~AndroidKeyMintDevice() {}

ScopedAStatus AndroidKeyMintDevice::getHardwareInfo(KeyMintHardwareInfo* info) {
    info->versionNumber = 4;
    info->securityLevel = securityLevel_;
    info->keyMintName = "TrustyKeyMintDevice";
    info->keyMintAuthorName = "Google";
    info->timestampTokenRequired = false;
    return ScopedAStatus::ok();
}

ScopedAStatus AndroidKeyMintDevice::addRngEntropy(const vector<uint8_t>& data) {
    if (data.size() == 0) {
        return ScopedAStatus::ok();
    }

    AddEntropyRequest request(impl_->message_version());
    request.random_data.Reinitialize(data.data(), data.size());

    AddEntropyResponse response(impl_->message_version());
    impl_->AddRngEntropy(request, &response);

    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus AndroidKeyMintDevice::generateKey(const vector<KeyParameter>& keyParams,
                                                const optional<AttestationKey>& attestationKey,
                                                KeyCreationResult* creationResult) {

    GenerateKeyRequest request(impl_->message_version());
    request.key_description.Reinitialize(KmParamSet(keyParams));
    if (attestationKey) {
        request.attestation_signing_key_blob =
            KeymasterKeyBlob(attestationKey->keyBlob.data(), attestationKey->keyBlob.size());
        request.attest_key_params.Reinitialize(KmParamSet(attestationKey->attestKeyParams));
        request.issuer_subject = KeymasterBlob(attestationKey->issuerSubjectName.data(),
                                               attestationKey->issuerSubjectName.size());
    }

    GenerateKeyResponse response(impl_->message_version());
    impl_->GenerateKey(request, &response);

    if (response.error != KM_ERROR_OK) {
        // Note a key difference between this current aidl and previous hal, is
        // that hal returns void where as aidl returns the error status.  If
        // aidl returns error, then aidl will not return any change you may make
        // to the out parameters.  This is quite different from hal where all
        // output variable can be modified due to hal returning void.
        //
        // So the caller need to be aware not to expect aidl functions to clear
        // the output variables for you in case of error.  If you left some
        // wrong data set in the out parameters, they will stay there.
        return kmError2ScopedAStatus(response.error);
    }

    creationResult->keyBlob = kmBlob2vector(response.key_blob);
    creationResult->keyCharacteristics = convertKeyCharacteristics(
        securityLevel_, request.key_description, response.unenforced, response.enforced);
    creationResult->certificateChain = convertCertificateChain(response.certificate_chain);
    return ScopedAStatus::ok();
}

ScopedAStatus AndroidKeyMintDevice::importKey(const vector<KeyParameter>& keyParams,
                                              KeyFormat keyFormat, const vector<uint8_t>& keyData,
                                              const optional<AttestationKey>& attestationKey,
                                              KeyCreationResult* creationResult) {

    ImportKeyRequest request(impl_->message_version());
    request.key_description.Reinitialize(KmParamSet(keyParams));
    request.key_format = legacy_enum_conversion(keyFormat);
    request.key_data = KeymasterKeyBlob(keyData.data(), keyData.size());
    if (attestationKey) {
        request.attestation_signing_key_blob =
            KeymasterKeyBlob(attestationKey->keyBlob.data(), attestationKey->keyBlob.size());
        request.attest_key_params.Reinitialize(KmParamSet(attestationKey->attestKeyParams));
        request.issuer_subject = KeymasterBlob(attestationKey->issuerSubjectName.data(),
                                               attestationKey->issuerSubjectName.size());
    }

    ImportKeyResponse response(impl_->message_version());
    impl_->ImportKey(request, &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    creationResult->keyBlob = kmBlob2vector(response.key_blob);
    creationResult->keyCharacteristics = convertKeyCharacteristics(
        securityLevel_, request.key_description, response.unenforced, response.enforced);
    creationResult->certificateChain = convertCertificateChain(response.certificate_chain);

    return ScopedAStatus::ok();
}

ScopedAStatus
AndroidKeyMintDevice::importWrappedKey(const vector<uint8_t>& wrappedKeyData,         //
                                       const vector<uint8_t>& wrappingKeyBlob,        //
                                       const vector<uint8_t>& maskingKey,             //
                                       const vector<KeyParameter>& unwrappingParams,  //
                                       int64_t passwordSid, int64_t biometricSid,     //
                                       KeyCreationResult* creationResult) {

    ImportWrappedKeyRequest request(impl_->message_version());
    request.SetWrappedMaterial(wrappedKeyData.data(), wrappedKeyData.size());
    request.SetWrappingMaterial(wrappingKeyBlob.data(), wrappingKeyBlob.size());
    request.SetMaskingKeyMaterial(maskingKey.data(), maskingKey.size());
    request.additional_params.Reinitialize(KmParamSet(unwrappingParams));
    request.password_sid = static_cast<uint64_t>(passwordSid);
    request.biometric_sid = static_cast<uint64_t>(biometricSid);

    ImportWrappedKeyResponse response(impl_->message_version());
    impl_->ImportWrappedKey(request, &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    creationResult->keyBlob = kmBlob2vector(response.key_blob);
    creationResult->keyCharacteristics = convertKeyCharacteristics(
        securityLevel_, request.additional_params, response.unenforced, response.enforced);
    creationResult->certificateChain = convertCertificateChain(response.certificate_chain);

    return ScopedAStatus::ok();
}

ScopedAStatus AndroidKeyMintDevice::upgradeKey(const vector<uint8_t>& keyBlobToUpgrade,
                                               const vector<KeyParameter>& upgradeParams,
                                               vector<uint8_t>* keyBlob) {

    UpgradeKeyRequest request(impl_->message_version());
    request.SetKeyMaterial(keyBlobToUpgrade.data(), keyBlobToUpgrade.size());
    request.upgrade_params.Reinitialize(KmParamSet(upgradeParams));

    UpgradeKeyResponse response(impl_->message_version());
    impl_->UpgradeKey(request, &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    *keyBlob = kmBlob2vector(response.upgraded_key);
    return ScopedAStatus::ok();
}

ScopedAStatus AndroidKeyMintDevice::deleteKey(const vector<uint8_t>& keyBlob) {
    DeleteKeyRequest request(impl_->message_version());
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());

    DeleteKeyResponse response(impl_->message_version());
    impl_->DeleteKey(request, &response);

    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus AndroidKeyMintDevice::deleteAllKeys() {
    // There's nothing to be done to delete software key blobs.
    DeleteAllKeysRequest request(impl_->message_version());
    DeleteAllKeysResponse response(impl_->message_version());
    impl_->DeleteAllKeys(request, &response);

    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus AndroidKeyMintDevice::destroyAttestationIds() {
    return kmError2ScopedAStatus(KM_ERROR_UNIMPLEMENTED);
}

ScopedAStatus AndroidKeyMintDevice::begin(KeyPurpose purpose, const vector<uint8_t>& keyBlob,
                                          const vector<KeyParameter>& params,
                                          const optional<HardwareAuthToken>& authToken,
                                          BeginResult* result) {

    BeginOperationRequest request(impl_->message_version());
    request.purpose = legacy_enum_conversion(purpose);
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());
    request.additional_params.Reinitialize(KmParamSet(params));

    vector<uint8_t> vector_token = authToken2AidlVec(authToken);
    request.additional_params.push_back(
        TAG_AUTH_TOKEN, reinterpret_cast<uint8_t*>(vector_token.data()), vector_token.size());

    BeginOperationResponse response(impl_->message_version());
    impl_->BeginOperation(request, &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    result->params = kmParamSet2Aidl(response.output_params);
    result->challenge = response.op_handle;
    result->operation =
        ndk::SharedRefBase::make<AndroidKeyMintOperation>(impl_, response.op_handle);
    return ScopedAStatus::ok();
}

ScopedAStatus AndroidKeyMintDevice::deviceLocked(
    bool passwordOnly, const std::optional<secureclock::TimeStampToken>& timestampToken) {
    DeviceLockedRequest request(impl_->message_version());
    request.passwordOnly = passwordOnly;
    if (timestampToken.has_value()) {
        request.token.challenge = timestampToken->challenge;
        request.token.mac = {timestampToken->mac.data(), timestampToken->mac.size()};
        request.token.timestamp = timestampToken->timestamp.milliSeconds;
    }
    DeviceLockedResponse response = impl_->DeviceLocked(request);
    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus AndroidKeyMintDevice::earlyBootEnded() {
    EarlyBootEndedResponse response = impl_->EarlyBootEnded();
    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus
AndroidKeyMintDevice::convertStorageKeyToEphemeral(const std::vector<uint8_t>& /* storageKeyBlob */,
                                                   std::vector<uint8_t>* /* ephemeralKeyBlob */) {
    return kmError2ScopedAStatus(KM_ERROR_UNIMPLEMENTED);
}

ScopedAStatus AndroidKeyMintDevice::getKeyCharacteristics(
    const std::vector<uint8_t>& keyBlob, const std::vector<uint8_t>& appId,
    const std::vector<uint8_t>& appData, std::vector<KeyCharacteristics>* keyCharacteristics) {
    GetKeyCharacteristicsRequest request(impl_->message_version());
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());
    addClientAndAppData(appId, appData, &request.additional_params);

    GetKeyCharacteristicsResponse response(impl_->message_version());
    impl_->GetKeyCharacteristics(request, &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    AuthorizationSet emptySet;
    *keyCharacteristics =
        convertKeyCharacteristics(securityLevel_, emptySet, response.unenforced, response.enforced,
                                  /* include_keystore_enforced = */ false);

    return ScopedAStatus::ok();
}

ScopedAStatus AndroidKeyMintDevice::getRootOfTrustChallenge(array<uint8_t, 16>* /* challenge */) {
    return kmError2ScopedAStatus(KM_ERROR_UNIMPLEMENTED);
}

ScopedAStatus AndroidKeyMintDevice::getRootOfTrust(const array<uint8_t, 16>& /* challenge */,
                                                   vector<uint8_t>* /* rootOfTrust */) {
    return kmError2ScopedAStatus(KM_ERROR_UNIMPLEMENTED);
}

ScopedAStatus AndroidKeyMintDevice::sendRootOfTrust(const vector<uint8_t>& /* rootOfTrust */) {
    return kmError2ScopedAStatus(KM_ERROR_UNIMPLEMENTED);
}

ScopedAStatus AndroidKeyMintDevice::setAdditionalAttestationInfo(const vector<KeyParameter>& info) {
    SetAdditionalAttestationInfoRequest request(impl_->message_version());
    request.info.Reinitialize(KmParamSet(info));

    SetAdditionalAttestationInfoResponse response = impl_->SetAdditionalAttestationInfo(request);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    } else {
        return ScopedAStatus::ok();
    }
}

std::shared_ptr<IKeyMintDevice> CreateKeyMintDevice(SecurityLevel securityLevel) {
    return ndk::SharedRefBase::make<AndroidKeyMintDevice>(securityLevel);
}

}  // namespace aidl::android::hardware::security::keymint
