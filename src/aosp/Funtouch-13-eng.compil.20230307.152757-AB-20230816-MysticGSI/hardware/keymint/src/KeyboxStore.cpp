#include "KeyboxStore.h"

#include <android-base/logging.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>

namespace keymint_vendor {
namespace {

std::string extractTag(const std::string& xml, const std::string& tag) {
    const std::string open = "<" + tag + ">";
    const std::string close = "</" + tag + ">";
    auto start = xml.find(open);
    if (start == std::string::npos) return {};
    start += open.size();
    auto end = xml.find(close, start);
    if (end == std::string::npos) return {};
    return xml.substr(start, end - start);
}

std::string extractAttr(const std::string& open_tag, const std::string& attr) {
    const std::string key = attr + "=\"";
    auto start = open_tag.find(key);
    if (start == std::string::npos) return {};
    start += key.size();
    auto end = open_tag.find('"', start);
    if (end == std::string::npos) return {};
    return open_tag.substr(start, end - start);
}

std::string trim(std::string s) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

// Collect all <Certificate ...>...</Certificate> bodies in document order.
std::vector<std::string> extractCertificates(const std::string& xml) {
    std::vector<std::string> out;
    const std::string open = "<Certificate";
    size_t pos = 0;
    while (true) {
        auto start = xml.find(open, pos);
        if (start == std::string::npos) break;
        auto gt = xml.find('>', start);
        if (gt == std::string::npos) break;
        auto end = xml.find("</Certificate>", gt);
        if (end == std::string::npos) break;
        out.push_back(trim(xml.substr(gt + 1, end - (gt + 1))));
        pos = end + 14;
    }
    return out;
}

}  // namespace

KeyboxStore& KeyboxStore::get() {
    static KeyboxStore instance;
    return instance;
}

bool KeyboxStore::loaded() const {
    std::lock_guard lock(mu_);
    return loaded_;
}

std::optional<std::vector<uint8_t>> KeyboxStore::pemToDer(const std::string& pem) {
    if (pem.empty()) return std::nullopt;

    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) return std::nullopt;

    std::vector<uint8_t> der;

    if (EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr)) {
        BIO* out = BIO_new(BIO_s_mem());
        if (out && i2d_PrivateKey_bio(out, pkey) > 0) {
            BUF_MEM* mem = nullptr;
            BIO_get_mem_ptr(out, &mem);
            if (mem && mem->data && mem->length > 0) {
                der.assign(reinterpret_cast<uint8_t*>(mem->data),
                           reinterpret_cast<uint8_t*>(mem->data) + mem->length);
            }
        }
        if (out) BIO_free(out);
        EVP_PKEY_free(pkey);
        BIO_free(bio);
        if (!der.empty()) return der;
        return std::nullopt;
    }

    BIO_free(bio);
    bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) return std::nullopt;

    if (X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)) {
        int len = i2d_X509(cert, nullptr);
        if (len > 0) {
            der.resize(static_cast<size_t>(len));
            uint8_t* p = der.data();
            i2d_X509(cert, &p);
        }
        X509_free(cert);
        BIO_free(bio);
        if (!der.empty()) return der;
        return std::nullopt;
    }

    BIO_free(bio);
    return std::nullopt;
}

bool KeyboxStore::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        LOG(ERROR) << "KeyboxStore: cannot open " << path;
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return parseXml(ss.str());
}

bool KeyboxStore::parseXml(const std::string& xml) {
    std::lock_guard lock(mu_);
    loaded_ = false;
    ec_ = {};
    rsa_ = {};
    device_id_.clear();

    if (xml.find("<AndroidAttestation>") == std::string::npos) {
        LOG(ERROR) << "KeyboxStore: missing <AndroidAttestation> root";
        return false;
    }

    // TrickyStore schema: <Keybox DeviceID="...">
    auto kb = xml.find("<Keybox");
    if (kb != std::string::npos) {
        auto gt = xml.find('>', kb);
        if (gt != std::string::npos) {
            device_id_ = extractAttr(xml.substr(kb, gt - kb), "DeviceID");
        }
    }
    if (device_id_.empty()) {
        device_id_ = trim(extractTag(xml, "DeviceID"));
    }
    if (device_id_.empty()) {
        LOG(WARNING) << "KeyboxStore: DeviceID empty";
    }

    auto parseKey = [&](const std::string& algoTag, AlgoMaterial& out) -> bool {
        auto block_start = xml.find("<Key algorithm=\"" + algoTag + "\"");
        if (block_start == std::string::npos) {
            LOG(WARNING) << "KeyboxStore: no <Key algorithm=\"" << algoTag << "\">";
            return false;
        }
        auto block_end = xml.find("</Key>", block_start);
        if (block_end == std::string::npos) return false;
        std::string block = xml.substr(block_start, block_end - block_start);

        std::string priv_pem = trim(extractTag(block, "PrivateKey"));
        auto priv_der = pemToDer(priv_pem);
        if (!priv_der) {
            LOG(ERROR) << "KeyboxStore: failed to decode " << algoTag << " PrivateKey PEM";
            return false;
        }
        out.private_key = keymaster::KeymasterKeyBlob(priv_der->data(), priv_der->size());

        auto certs = extractCertificates(block);
        if (certs.empty()) {
            LOG(ERROR) << "KeyboxStore: no certificates for " << algoTag;
            return false;
        }

        out.chain = keymaster::CertificateChain(certs.size());
        if (!out.chain.entries) {
            LOG(ERROR) << "KeyboxStore: CertificateChain alloc failed";
            return false;
        }
        for (size_t i = 0; i < certs.size(); ++i) {
            auto der = pemToDer(certs[i]);
            if (!der) {
                LOG(ERROR) << "KeyboxStore: bad certificate PEM index " << i << " for " << algoTag;
                out.chain = {};
                return false;
            }
            out.chain.entries[i].data = new (std::nothrow) uint8_t[der->size()];
            out.chain.entries[i].data_length = der->size();
            if (!out.chain.entries[i].data) {
                out.chain = {};
                return false;
            }
            std::memcpy(const_cast<uint8_t*>(out.chain.entries[i].data), der->data(), der->size());
        }
        return true;
    };

    bool ok_ec = parseKey("ecdsa", ec_);
    bool ok_rsa = parseKey("rsa", rsa_);
    if (!ok_ec && !ok_rsa) {
        LOG(ERROR) << "KeyboxStore: neither ecdsa nor rsa key parsed";
        return false;
    }

    loaded_ = true;
    LOG(INFO) << "KeyboxStore: loaded ec=" << ok_ec << " rsa=" << ok_rsa;
    return true;
}

const keymaster::KeymasterKeyBlob* KeyboxStore::attestationKey(keymaster_algorithm_t algorithm,
                                                               keymaster_error_t* error) const {
    std::lock_guard lock(mu_);
    if (!loaded_) {
        if (error) *error = KM_ERROR_UNKNOWN_ERROR;
        return nullptr;
    }
    if (error) *error = KM_ERROR_OK;

    switch (algorithm) {
    case KM_ALGORITHM_EC:
        if (ec_.private_key.key_material_size == 0) {
            if (error) *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
            return nullptr;
        }
        return &ec_.private_key;
    case KM_ALGORITHM_RSA:
        if (rsa_.private_key.key_material_size == 0) {
            if (error) *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
            return nullptr;
        }
        return &rsa_.private_key;
    default:
        if (error) *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return nullptr;
    }
}

keymaster::CertificateChain KeyboxStore::attestationChain(keymaster_algorithm_t algorithm,
                                                          keymaster_error_t* error) const {
    std::lock_guard lock(mu_);
    if (!loaded_) {
        if (error) *error = KM_ERROR_UNKNOWN_ERROR;
        return {};
    }
    if (error) *error = KM_ERROR_OK;

    const AlgoMaterial* mat = nullptr;
    switch (algorithm) {
    case KM_ALGORITHM_EC:
        mat = &ec_;
        break;
    case KM_ALGORITHM_RSA:
        mat = &rsa_;
        break;
    default:
        if (error) *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return {};
    }

    if (!mat->chain.entry_count || !mat->chain.entries) {
        if (error) *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return {};
    }

    auto cloned = keymaster::CertificateChain::clone(mat->chain);
    if (!cloned.entries && error) *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return cloned;
}

}  // namespace keymint_vendor
