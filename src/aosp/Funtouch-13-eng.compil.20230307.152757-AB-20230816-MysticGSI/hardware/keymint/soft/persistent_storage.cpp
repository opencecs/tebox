// Replace the GSI's RAM-only software secure-key index. Key blobs remain in
// keystore2; only their stable SHA-256-derived IDs are stored here.
#include <keymaster/pure_soft_secure_key_storage.h>
#include <android/log.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace {
constexpr const char* kDirectory = "/data/misc/qemu-keymint";
constexpr size_t kSlots = 64;  // PureSoftKeymasterContext's software slot limit.
std::mutex storage_mutex;

std::string path(uint64_t id) {
    char name[32];
    snprintf(name, sizeof(name), "/%016llx.key", static_cast<unsigned long long>(id));
    return std::string(kDirectory) + name;
}

bool isKeyName(const char* name) {
    if (strlen(name) != 20 || strcmp(name + 16, ".key") != 0) return false;
    return strspn(name, "0123456789abcdef") == 16;
}

bool listKeys(std::vector<std::string>* names) {
    DIR* dir = opendir(kDirectory);
    if (!dir) return false;
    errno = 0;
    while (auto* entry = readdir(dir)) {
        if (isKeyName(entry->d_name)) names->emplace_back(entry->d_name);
    }
    bool ok = errno == 0;
    closedir(dir);
    return ok;
}

bool syncDirectory() {
    int fd = open(kDirectory, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    bool ok = fsync(fd) == 0;
    close(fd);
    return ok;
}

keymaster_error_t storageError(const char* operation) {
    __android_log_print(ANDROID_LOG_ERROR, "qemu-keymint", "persistent storage %s: %s",
                        operation, strerror(errno));
    return KM_ERROR_UNKNOWN_ERROR;
}
}  // namespace

namespace keymaster {
// Export these definitions from the launcher so the existing GSI vtable binds
// to them. Keep the GSI constructor, destructor and ABI unchanged.
keymaster_error_t PureSoftSecureKeyStorage::WriteKey(km_id_t id, const KeymasterKeyBlob&) {
    std::lock_guard<std::mutex> lock(storage_mutex);
    std::vector<std::string> names;
    if (!listKeys(&names)) return storageError("list");
    struct stat info;
    const std::string filename = path(id);
    if (lstat(filename.c_str(), &info) == 0) {
        if (!S_ISREG(info.st_mode)) return KM_ERROR_UNKNOWN_ERROR;
        return syncDirectory() ? KM_ERROR_OK : storageError("sync");
    }
    if (errno != ENOENT) return storageError("stat");
    if (names.size() >= kSlots) return KM_ERROR_UNKNOWN_ERROR;
    // A zero-length regular file is the complete record. No partial contents
    // can become visible, and the directory fsync commits creation before return.
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return storageError("create");
    bool ok = fsync(fd) == 0;
    close(fd);
    if (!ok || !syncDirectory()) return storageError("sync");
    __android_log_print(ANDROID_LOG_INFO, "qemu-keymint", "persisted software secure-key slot");
    return KM_ERROR_OK;
}

keymaster_error_t PureSoftSecureKeyStorage::KeyExists(km_id_t id, bool* exists) {
    std::lock_guard<std::mutex> lock(storage_mutex);
    struct stat info;
    if (lstat(path(id).c_str(), &info) == 0) {
        *exists = S_ISREG(info.st_mode);
        return KM_ERROR_OK;
    }
    if (errno != ENOENT) return storageError("stat");
    // A missing/unmounted state directory is a storage error, not deleted keys.
    if (stat(kDirectory, &info) != 0 || !S_ISDIR(info.st_mode)) return storageError("directory");
    *exists = false;
    return KM_ERROR_OK;
}

keymaster_error_t PureSoftSecureKeyStorage::DeleteKey(km_id_t id) {
    std::lock_guard<std::mutex> lock(storage_mutex);
    if (unlink(path(id).c_str()) != 0 && errno != ENOENT) return storageError("delete");
    return syncDirectory() ? KM_ERROR_OK : storageError("sync");
}

keymaster_error_t PureSoftSecureKeyStorage::DeleteAllKeys() {
    std::lock_guard<std::mutex> lock(storage_mutex);
    std::vector<std::string> names;
    if (!listKeys(&names)) return storageError("list");
    for (const auto& name : names) {
        if (unlink((std::string(kDirectory) + "/" + name).c_str()) != 0)
            return storageError("delete");
    }
    return syncDirectory() ? KM_ERROR_OK : storageError("sync");
}

keymaster_error_t PureSoftSecureKeyStorage::HasSlot(bool* has_slot) {
    std::lock_guard<std::mutex> lock(storage_mutex);
    std::vector<std::string> names;
    if (!listKeys(&names)) return storageError("list");
    *has_slot = names.size() < kSlots;
    return KM_ERROR_OK;
}
}  // namespace keymaster
