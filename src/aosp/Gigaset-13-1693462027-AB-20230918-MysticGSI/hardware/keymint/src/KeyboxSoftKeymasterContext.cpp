#include "KeyboxSoftKeymasterContext.h"

#include "KeyboxStore.h"

namespace keymaster {

KeymasterKeyBlob KeyboxSoftKeymasterContext::GetAttestationKey(
        keymaster_algorithm_t algorithm, keymaster_error_t* error) const {
    const KeymasterKeyBlob* blob =
            keymint_vendor::KeyboxStore::get().attestationKey(algorithm, error);
    if (blob && blob->key_material_size > 0) {
        return KeymasterKeyBlob(*blob);
    }
    // Fall back to AOSP built-in software attestation keys.
    return SoftAttestationContext::GetAttestationKey(algorithm, error);
}

CertificateChain KeyboxSoftKeymasterContext::GetAttestationChain(
        keymaster_algorithm_t algorithm, keymaster_error_t* error) const {
    CertificateChain chain =
            keymint_vendor::KeyboxStore::get().attestationChain(algorithm, error);
    if (chain.entries && chain.entry_count > 0) {
        return chain;
    }
    return SoftAttestationContext::GetAttestationChain(algorithm, error);
}

}  // namespace keymaster
