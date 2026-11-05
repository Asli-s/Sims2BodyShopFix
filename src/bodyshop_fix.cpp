#include <windows.h>
#include <bcrypt.h>

#include <commdlg.h>

#include <string>
#include <vector>

namespace {

constexpr ULONGLONG kFileSize = 8052736;
constexpr ULONGLONG kPatchOffset = 0x1FA7CF;
constexpr BYTE kOriginalBytes[6] = {0x8B, 0x06, 0x56, 0xFF, 0x50, 0x10};
// mov eax, 0x10000000 (256 MB), then nop.
constexpr BYTE kPatchedBytes[6] = {0xB8, 0x00, 0x00, 0x00, 0x10, 0x90};
constexpr char kOriginalSha256[] = "FB8D6039A68D27D335524DE267C205B4E457546FAE421D4879831E2D5055EE4A";
constexpr char kPatchedSha256[] = "3C6DA84117B08527E75638E45D7C6C5397DD8E40CEA686FF72E2F993CDF9F61B";

constexpr wchar_t kExeInInstall[] = L"\\Fun with Pets\\SP9\\CSBin\\TS2BodyShop.exe";
constexpr wchar_t kBackupSuffix[] = L".backup";

constexpr wchar_t kTitle[] = L"Sims 2 Body Shop Patcher";

void showMessage(const wchar_t* text, UINT icon) {
    MessageBoxW(nullptr, text, kTitle, MB_OK | icon);
}

bool askYes(const wchar_t* text) {
    return MessageBoxW(nullptr, text, kTitle, MB_YESNO | MB_ICONQUESTION) == IDYES;
}

std::wstring browseForBodyShop() {
    wchar_t file[MAX_PATH] = {};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = L"Body Shop (TS2BodyShop.exe)\0TS2BodyShop.exe\0";
    dialog.lpstrFile = file;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrTitle = L"Find TS2BodyShop.exe";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    return GetOpenFileNameW(&dialog) ? std::wstring(file) : std::wstring();
}

bool readExe(const std::wstring& path, std::vector<BYTE>& data) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(file, &size) && static_cast<ULONGLONG>(size.QuadPart) == kFileSize;
    if (ok) {
        data.resize(static_cast<size_t>(kFileSize));
        DWORD read = 0;
        ok = ReadFile(file, data.data(), static_cast<DWORD>(kFileSize), &read, nullptr) &&
             read == kFileSize;
    }
    CloseHandle(file);
    return ok;
}

bool writeExe(const std::wstring& path, const std::vector<BYTE>& data) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) &&
              written == data.size();
    ok = FlushFileBuffers(file) && ok;
    CloseHandle(file);
    return ok;
}

// Returns the SHA-256 of the data as 64 upper-case hex digits, or an empty string on failure.
std::string sha256Hex(const std::vector<BYTE>& data) {
    BYTE hash[32] = {};
    if (!BCRYPT_SUCCESS(BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
                                   const_cast<BYTE*>(data.data()),
                                   static_cast<ULONG>(data.size()), hash, sizeof(hash)))) {
        return {};
    }
    static const char digits[] = "0123456789ABCDEF";
    std::string hex;
    for (BYTE b : hash) {
        hex += digits[b >> 4];
        hex += digits[b & 15];
    }
    return hex;
}

std::wstring findBodyShop() {
    wchar_t dir[MAX_PATH] = {};
    DWORD bytes = sizeof(dir);
    LSTATUS status = RegGetValueW(HKEY_LOCAL_MACHINE,
                                  L"SOFTWARE\\WOW6432Node\\Maxis\\The Sims 2 Ultimate Collection",
                                  L"Install Dir", RRF_RT_REG_SZ, nullptr, dir, &bytes);
    if (status != ERROR_SUCCESS) return {};
    std::wstring path = dir;
    while (!path.empty() && (path.back() == L'\\' || path.back() == L'/')) path.pop_back();
    path += kExeInInstall;
    return GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES ? std::wstring() : path;
}

void setBytes(std::vector<BYTE>& data, const BYTE (&bytes)[6]) {
    for (size_t i = 0; i < 6; ++i) data[static_cast<size_t>(kPatchOffset) + i] = bytes[i];
}

int run(int argc, wchar_t** argv) {
    std::wstring path = argc > 1 ? argv[1] : findBodyShop();
    if (path.empty()) {
        showMessage(L"Body Shop could not be found automatically.\n\n"
                    L"Please select TS2BodyShop.exe manually.\n"
                    L"It is located in your installation folder under Fun with Pets\\SP9\\CSBin.",
                    MB_ICONINFORMATION);
        path = browseForBodyShop();
        if (path.empty()) return 1;
    }

    std::vector<BYTE> data;
    if (!readExe(path, data)) {
        showMessage(L"Could not read Body Shop, or the version is incompatible.\n\n"
                    L"Close Body Shop and try again. No changes were applied.",
                    MB_ICONWARNING);
        return 1;
    }

    const std::string hash = sha256Hex(data);
    const std::wstring backup = path + kBackupSuffix;

    if (hash == kOriginalSha256) {
        if (!askYes(L"Body Shop is not patched yet.\n\nApply the patch?")) return 0;
        if (!CopyFileW(path.c_str(), backup.c_str(), FALSE)) {
            showMessage(L"Could not save the backup. No changes were applied.", MB_ICONERROR);
            return 1;
        }
        setBytes(data, kPatchedBytes);
        if (sha256Hex(data) != kPatchedSha256 || !writeExe(path, data)) {
            showMessage(L"Writing failed. Your original file is in TS2BodyShop.exe.backup.",
                        MB_ICONERROR);
            return 1;
        }
        showMessage(L"Patched!\n\n"
                    L"A backup was saved as TS2BodyShop.exe.backup.",
                    MB_ICONINFORMATION);
        return 0;
    }

    if (hash == kPatchedSha256) {
        if (!askYes(L"Body Shop is already patched.\n\nRevert the patch?")) return 0;
        setBytes(data, kOriginalBytes);
        if (sha256Hex(data) != kOriginalSha256 || !writeExe(path, data)) {
            showMessage(L"Revert failed. Your original file is in TS2BodyShop.exe.backup.",
                        MB_ICONERROR);
            return 1;
        }
        showMessage(L"Done! Body Shop is reverted to the original.", MB_ICONINFORMATION);
        return 0;
    }

    showMessage(L"This Body Shop is a different version or is already modified by something else.\n\n"
                L"No changes were applied.",
                MB_ICONWARNING);
    return 1;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    return run(__argc, __wargv);
}