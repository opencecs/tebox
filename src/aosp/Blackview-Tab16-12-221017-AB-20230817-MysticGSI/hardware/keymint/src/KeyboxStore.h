#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <hardware/keymaster_defs.h>
#include <keymaster/android_keymaster_utils.h>

namespace keymint_vendor {

// Runtime store for AndroidAttestation keybox.xml (TrickyStore schema).
// Parsed once at process start; PEM material kept as DER for SoftKeymaster.
class KeyboxStore {
  public:
    static KeyboxStore& get();

    // Load / parse keybox. Returns false on hard failure (missing file / bad XML).
    bool load(const std::string& path);

    bool loaded() const;

    // SoftKeymaster attestation hooks (EC / RSA).
    const keymaster::KeymasterKeyBlob* attestationKey(keymaster_algorithm_t algorithm,
                                                      keymaster_error_t* error) const;
    keymaster::CertificateChain attestationChain(keymaster_algorithm_t algorithm,
                                                 keymaster_error_t* error) const;

    const std::string& deviceId() const { return device_id_; }

  private:
    KeyboxStore() = default;

    struct AlgoMaterial {
        keymaster::KeymasterKeyBlob private_key;           // DER
        keymaster::CertificateChain chain;                 // leaf → … → root (DER)
    };

    bool parseXml(const std::string& xml);
    static std::optional<std::vector<uint8_t>> pemToDer(const std::string& pem);

    mutable std::mutex mu_;
    bool loaded_ = false;
    std::string device_id_;
    AlgoMaterial ec_;
    AlgoMaterial rsa_;
};

}  // namespace keymint_vendor
