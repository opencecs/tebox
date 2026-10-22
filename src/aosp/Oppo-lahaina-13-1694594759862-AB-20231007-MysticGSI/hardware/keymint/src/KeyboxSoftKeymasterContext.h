#pragma once

#include <keymaster/contexts/pure_soft_keymaster_context.h>

namespace keymaster {

// Soft KeyMint context that signs attestations with keybox.xml material
// (AndroidAttestation schema) instead of the AOSP demo software keys.
class KeyboxSoftKeymasterContext : public PureSoftKeymasterContext {
  public:
    using PureSoftKeymasterContext::PureSoftKeymasterContext;

    KeymasterKeyBlob GetAttestationKey(keymaster_algorithm_t algorithm,
                                       keymaster_error_t* error) const override;

    CertificateChain GetAttestationChain(keymaster_algorithm_t algorithm,
                                         keymaster_error_t* error) const override;
};

}  // namespace keymaster
