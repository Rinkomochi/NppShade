/*
This file is part of nppcrypt
(http://www.github.com/jeanpaulrichter/nppcrypt)
a plugin for notepad++ [ Copyright (C)2003 Don HO <don.h@free.fr> ]
(https://notepad-plus-plus.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <vector>
#include <windows.h>
#include <commdlg.h>
#include "npp/Definitions.h"
#include "nppshade.h"
#include "exception.h"
#include "preferences.h"
#include "crypt.h"
#include "crypt_help.h"
#include "dlg_crypt.h"
#include "dlg_hash.h"
#include "dlg_random.h"
#include "dlg_about.h"
#include "dlg_preferences.h"
#include "dlg_auth.h"
#include "dlg_convert.h"
#include "dlg_initdata.h"
#include "messagebox.h"
#include "cryptheader.h"
#include "resource.h"
#include "managed_extensions.h"
#include "help.h"
#include "localization.h"

typedef std::map<std::wstring, CryptInfo> cryptfilemap;
enum class NppShadeFileSave : unsigned { DEFAULT, OLD_ENCRYPTION, NO_ENCRYPTION };

#pragma comment(lib, "comdlg32.lib")

extern NppData nppData;

struct PendingWholeFileSave {
    bool active = false;
    bool replaced = false;
    uptr_t buffer_id = 0;
    std::wstring target_path;
    std::basic_string<byte> ciphertext;
};

static std::wstring managedExtension()
{
    std::vector<std::wstring> extensions;
    return parseManagedExtensions(preferences.files.extension, extensions) ? extensions.front() : L"";
}

static bool isManagedExtension(const std::wstring& extension)
{
    std::vector<std::wstring> extensions;
    if (!preferences.files.enable || !parseManagedExtensions(preferences.files.extension, extensions)) return false;
    for (const auto& configured : extensions) {
        if (_wcsicmp(configured.c_str(), extension.c_str()) == 0) return true;
    }
    return false;
}

static bool isAbsolutePath(const std::wstring& path)
{
    return (path.size() >= 3 && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/')) ||
        (path.size() >= 2 && path[0] == L'\\' && path[1] == L'\\');
}

static bool pathExistsOrUncertain(const std::wstring& path)
{
    if (::GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return true;
    }
    const DWORD error = ::GetLastError();
    return error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND;
}

static bool chooseEncryptedPath(const std::wstring& suggested, const std::wstring& extension, std::wstring& chosen)
{
    std::vector<wchar_t> path(32768, L'\0');
    if (suggested.size() >= path.size()) {
        msgbox::error(nppData._nppHandle, "文件路径过长。");
        return false;
    }
    std::copy(suggested.begin(), suggested.end(), path.begin());

    std::vector<std::wstring> extensions;
    if (!parseManagedExtensions(preferences.files.extension, extensions)) return false;
    std::wstring patterns;
    for (const auto& item : extensions) {
        if (!patterns.empty()) patterns += L";";
        patterns += L"*." + item;
    }
    std::wstring filter = localization::translate(L"加密文件") + L" (" + patterns + L")";
    filter.push_back(L'\0');
    filter += patterns;
    filter.push_back(L'\0');
    filter += L"所有文件 (*.*)";
    filter.push_back(L'\0');
    filter += L"*.*";
    filter.push_back(L'\0');
    filter.push_back(L'\0');

    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = nppData._nppHandle;
    dialog.lpstrFilter = filter.c_str();
    dialog.lpstrFile = &path[0];
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.lpstrDefExt = extension.c_str();
    dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;
    if (!::GetSaveFileNameW(&dialog)) {
        return false;
    }

    chosen.assign(&path[0]);
    const size_t separator = chosen.find_last_of(L"\\/");
    const std::wstring filename = chosen.substr(separator == std::wstring::npos ? 0 : separator + 1);
    const size_t dot = filename.find_last_of(L'.');
    if (dot == std::wstring::npos || !isManagedExtension(filename.substr(dot + 1))) {
        chosen += L"." + extension;
    }
    if (pathExistsOrUncertain(chosen)) {
        msgbox::info(nppData._nppHandle, "目标文件已存在，请选择其他文件名。");
        return false;
    }
    return true;
}

const TCHAR             NPP_PLUGIN_NAME[] = TEXT(NPPC_NAME);
const int               NPPCRYPT_VERSION = NPPC_VERSION;

FuncItem                funcItem[NPPC_FUNC_COUNT];
NppData                 nppData;
HINSTANCE               m_hInstance;
CurrentOptions          current;
cryptfilemap            crypt_files;
NppShadeFileSave        crypt_file_cursave;
PendingWholeFileSave    pending_whole_file_save;
UINT_PTR                last_known_scintilla_first_line = 0;
UINT_PTR                last_known_scintilla_pos = 0;
UINT_PTR                last_known_scintilla_anchor = 0;

DlgCrypt                dlg_crypt;
DlgHash                 dlg_hash(current.hash);
DlgRandom               dlg_random(current.random);
DlgAuth                 dlg_auth;
DlgPreferences          dlg_preferences;
DlgAbout                dlg_about;
DlgConvert              dlg_convert(current.convert);
DlgInitdata             dlg_initdata;

BOOL APIENTRY DllMain( HANDLE hModule, DWORD reasonForCall, LPVOID lpReserved )
{
    switch (reasonForCall)
    {
    case DLL_PROCESS_ATTACH:
    {
        #ifdef _DEBUG
            _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        #endif
        m_hInstance = (HINSTANCE)hModule;
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        preferences.save(current);
        dlg_random.destroy();
        dlg_hash.destroy();
        dlg_crypt.destroy();
        dlg_about.destroy();
        dlg_preferences.destroy();
        dlg_auth.destroy();
        dlg_convert.destroy();
        dlg_initdata.destroy();
        break;
    }
    case DLL_THREAD_ATTACH:
    {
        break;
    }
    case DLL_THREAD_DETACH:
    {
        break;
    }
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    nppData = notpadPlusData;

    dlg_random.init(m_hInstance, nppData._nppHandle, IDD_RANDOM, IDC_OK);
    dlg_hash.init(m_hInstance, nppData._nppHandle, IDD_HASH, IDC_OK);
    dlg_crypt.init(m_hInstance, nppData._nppHandle, IDD_CRYPT, IDC_OK);
    dlg_about.init(m_hInstance, nppData._nppHandle, IDD_ABOUT, IDC_OK);
    dlg_preferences.init(m_hInstance, nppData._nppHandle, IDD_PREFERENCES, IDC_OK);
    dlg_auth.init(m_hInstance, nppData._nppHandle, IDD_AUTH, IDC_OK);
    dlg_convert.init(m_hInstance, nppData._nppHandle, IDD_CONVERT, IDC_OK);
    dlg_initdata.init(m_hInstance, nppData._nppHandle, IDD_INITDATA, IDC_OK);

    TCHAR config_dir[MAX_PATH];
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)config_dir);
    const int config_dir_len = lstrlen(config_dir);
    if (config_dir_len + 15 < MAX_PATH) {
        TCHAR preference_path[MAX_PATH];
        TCHAR legacy_path[MAX_PATH];
        lstrcpy(preference_path, config_dir);
        lstrcat(preference_path, TEXT("\\nppshade.xml"));
        lstrcpy(legacy_path, config_dir);
        lstrcat(legacy_path, TEXT("\\nppcrypt.xml"));

        const bool has_preferences = ::GetFileAttributes(preference_path) != INVALID_FILE_ATTRIBUTES;
        const bool has_legacy_preferences = ::GetFileAttributes(legacy_path) != INVALID_FILE_ATTRIBUTES;
        if (!has_preferences && has_legacy_preferences) {
            preferences.load(legacy_path, current);
            if (!preferences.failed()) {
                if (preferences.files.extension == TEXT("nppcrypt")) {
                    preferences.files.extension = TEXT(NPPC_DEF_FILE_EXT);
                }
            }
            preferences.setFilePath(preference_path);
        } else {
            preferences.load(preference_path, current);
        }
    }

    const wchar_t* source_names[] = { L"加密", L"解密", L"哈希", L"随机数据", L"编码转换", L"首选项", L"关于" };
    PFUNCPLUGINCMD callbacks[] = { EncryptDlg, DecryptDlg, HashDlg, RandomDlg, ConvertDlg, PreferencesDlg, AboutDlg };
    const size_t indices[] = { 0, 1, 2, 3, 4, 6, 8 };
    for (size_t i = 0; i < 7; ++i) {
        std::wstring name = localization::translate(source_names[i]);
        help::npp::setCommand(indices[i], &name[0], callbacks[i], NULL, false);
    }
}

extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
    return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
    *nbF = int(NPPC_FUNC_COUNT);
    return funcItem;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
    switch (notifyCode->nmhdr.code)
    {
    case NPPN_FILEOPENED:
    {
        if (!preferences.files.enable) {
            return;
        }
        try {
            std::wstring path, filename, extension;
            help::buffer::getPath(notifyCode->nmhdr.idFrom, path, filename, extension);

            if (isManagedExtension(extension)) {
                ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)path.c_str());
                HWND hCurScintilla = help::scintilla::getCurrent();

                int data_length = (int)::SendMessage(hCurScintilla, SCI_GETLENGTH , 0, 0);
                if (data_length <= 0) {
                    throwInfo(file_empty);
                }
                byte* pData = (byte*)::SendMessage(hCurScintilla, SCI_GETCHARACTERPOINTER, 0, 0);
                if (!pData) {
                    throwError(no_scintilla_pointer);
                }

                CryptInfo               crypt;
                nppcrypt::InitData         initdata;
                CryptHeaderReader       header(crypt.hmac);

                // A file with the configured extension may be plain text until
                // its first save. Only files with a valid header need decrypting.
                if (!header.parse(crypt.options, initdata, pData, data_length)) {
                    return;
                }

                if(crypt.hmac.enable) {
                    if (crypt.hmac.keypreset_id < 0) {
                        if (!dlg_auth.doDialog()) {
                            return;
                        }
                        crypt.hmac.hash.key.set(dlg_auth.getInput());
                        if (!header.checkHMAC()) {
                            throwInfo(hmac_auth_failed);
                        }
                    } else {
                        if (crypt.hmac.keypreset_id >= preferences.getKeyNum()) {
                            throwInvalid(invalid_keypreset_id);
                        } else {
                            crypt.hmac.hash.key.set(preferences.getKey(crypt.hmac.keypreset_id), 16);
                            if (!header.checkHMAC()) {
                                throwInfo(hmac_auth_failed);
                            }
                        }
                    }
                }

                std::basic_string<byte> buffer;
                const std::basic_string<byte> ciphertext(header.getEncrypted(), header.getEncryptedLength());
                if (dlg_crypt.decryptDialog(&crypt, &initdata.iv, [&]() {
                    buffer.clear();
                    nppcrypt::decrypt(ciphertext.data(), ciphertext.size(), buffer, crypt.options, crypt.password, initdata);
                    return true;
                }, &filename)) {

                    ::SendMessage(hCurScintilla, SCI_CLEARALL, 0, 0);
                    ::SendMessage(hCurScintilla, SCI_APPENDTEXT, buffer.size(), (LPARAM)&buffer[0]);
                    ::SendMessage(hCurScintilla, SCI_GOTOPOS, 0, 0);
                    ::SendMessage(hCurScintilla, SCI_EMPTYUNDOBUFFER, 0, 0);
                    ::SendMessage(hCurScintilla, SCI_SETSAVEPOINT, 0, 0);

                    crypt_files.insert(std::pair<std::wstring, CryptInfo>(path, crypt));
                }
            }
        } catch (ExcInfo& exc) {
            if (exc.getID() != ExcInfo::ID::file_empty) {
                msgbox::info(nppData._nppHandle, exc.what(), exc.getURL(), exc.getURLCaption());
            }
        } catch(std::exception& exc) {
            msgbox::error(nppData._nppHandle, exc.what());
        } catch(...) {
            msgbox::error(nppData._nppHandle, "未知错误！");
        }
        break;
    }
    case NPPN_FILESAVED:
    {
        try {
            if (pending_whole_file_save.active && pending_whole_file_save.buffer_id == notifyCode->nmhdr.idFrom) {
                if (pending_whole_file_save.replaced) {
                    HWND hCurScintilla = help::scintilla::getCurrent();
                    ::SendMessage(hCurScintilla, SCI_UNDO, 0, 0);
                    ::SendMessage(hCurScintilla, SCI_SETANCHOR, last_known_scintilla_anchor, 0);
                    ::SendMessage(hCurScintilla, SCI_SETCURRENTPOS, last_known_scintilla_pos, 0);
                    ::SendMessage(hCurScintilla, SCI_SETFIRSTVISIBLELINE, last_known_scintilla_first_line, 0);
                    ::SendMessage(hCurScintilla, SCI_EMPTYUNDOBUFFER, 0, 0);
                    ::SendMessage(hCurScintilla, SCI_SETSAVEPOINT, 0, 0);
                    pending_whole_file_save.replaced = false;
                }
                return;
            }
            if (!preferences.files.enable) {
                return;
            }
            std::wstring path, filename, extension;
            help::buffer::getPath(notifyCode->nmhdr.idFrom, path, filename, extension);

            if (isManagedExtension(extension)) {

                if (crypt_file_cursave != NppShadeFileSave::NO_ENCRYPTION) {
                    HWND hCurScintilla = help::scintilla::getCurrent();
                    ::SendMessage(hCurScintilla, SCI_UNDO, 0, 0);
                    ::SendMessage(hCurScintilla, SCI_SETANCHOR, last_known_scintilla_anchor, 0);
                    ::SendMessage(hCurScintilla, SCI_SETCURRENTPOS, last_known_scintilla_pos, 0);
                    ::SendMessage(hCurScintilla, SCI_SETFIRSTVISIBLELINE, last_known_scintilla_first_line, last_known_scintilla_first_line);
                    ::SendMessage(hCurScintilla, SCI_EMPTYUNDOBUFFER, 0, 0);
                    ::SendMessage(hCurScintilla, SCI_SETSAVEPOINT, 0, 0);
                }
                if (crypt_file_cursave == NppShadeFileSave::OLD_ENCRYPTION) {
                    msgbox::info(nppData._nppHandle, "文件已使用原有加密方式保存。");
                } else if (crypt_file_cursave == NppShadeFileSave::NO_ENCRYPTION) {
                    msgbox::error(nppData._nppHandle, "文件保存时未加密。");
                }
            }
        } catch (ExcInfo& exc) {
            if (exc.getID() != ExcInfo::ID::file_empty) {
                msgbox::info(nppData._nppHandle, exc.what(), exc.getURL(), exc.getURLCaption());
            }
        } catch (std::exception& exc) {
            msgbox::error(nppData._nppHandle, exc.what());
        } catch (...) {
            msgbox::error(nppData._nppHandle, "未知错误！");
        }
        break;
    }
    case NPPN_FILEBEFORESAVE:
    {
        try {
            if (pending_whole_file_save.active && pending_whole_file_save.buffer_id == notifyCode->nmhdr.idFrom) {
                HWND hCurScintilla = help::scintilla::getCurrent();
                last_known_scintilla_first_line = (UINT_PTR)::SendMessage(hCurScintilla, SCI_GETFIRSTVISIBLELINE, 0, 0);
                last_known_scintilla_pos = (UINT_PTR)::SendMessage(hCurScintilla, SCI_GETCURRENTPOS, 0, 0);
                last_known_scintilla_anchor = (UINT_PTR)::SendMessage(hCurScintilla, SCI_GETANCHOR, 0, 0);
                ::SendMessage(hCurScintilla, SCI_BEGINUNDOACTION, 0, 0);
                ::SendMessage(hCurScintilla, SCI_CLEARALL, 0, 0);
                ::SendMessage(hCurScintilla, SCI_APPENDTEXT, pending_whole_file_save.ciphertext.size(),
                    (LPARAM)pending_whole_file_save.ciphertext.data());
                ::SendMessage(hCurScintilla, SCI_ENDUNDOACTION, 0, 0);
                pending_whole_file_save.replaced = true;
                return;
            }
            if (!preferences.files.enable) {
                return;
            }
            std::wstring path, filename, extension;
            help::buffer::getPath(notifyCode->nmhdr.idFrom, path, filename, extension);

            if (isManagedExtension(extension)) {

                cryptfilemap::iterator  fiter = crypt_files.find(path);
                bool                    old_encryption_available = (fiter != crypt_files.end());
                CryptInfo& crypt = old_encryption_available ? fiter->second : current.crypt;

                ::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)path.c_str());
                HWND hCurScintilla = help::scintilla::getCurrent();

                last_known_scintilla_first_line = (UINT_PTR)::SendMessage(hCurScintilla, SCI_GETFIRSTVISIBLELINE, 0, 0);
                last_known_scintilla_pos = (UINT_PTR)::SendMessage(hCurScintilla, SCI_GETCURRENTPOS, 0, 0);
                last_known_scintilla_anchor = (UINT_PTR)::SendMessage(hCurScintilla, SCI_GETANCHOR, 0, 0);

                int data_length = (int)::SendMessage(hCurScintilla, SCI_GETLENGTH , 0, 0);
                if (data_length <= 0) {
                    throwInfo(file_empty);
                }

                byte* pData = (byte*)::SendMessage(hCurScintilla, SCI_GETCHARACTERPOINTER , 0, 0);
                if (!pData) {
                    throwError(no_scintilla_pointer);
                }

                nppcrypt::InitData         initdata;
                CryptHeaderWriter       header(crypt.hmac);
                std::basic_string<byte> buffer;
                bool                    autoencrypt = false;
                crypt_file_cursave = NppShadeFileSave::DEFAULT;

                if(old_encryption_available) {
                    if(preferences.files.askonsave) {
                        std::wstring asksave_msg = localization::translate(L"更改以下文件的加密方式：");
                        if (filename.size() > 32) {
                            asksave_msg += filename.substr(0,32) + TEXT("...?");
                        } else {
                            asksave_msg += filename + TEXT("?");
                        }
                        if (::MessageBox(nppData._nppHandle, asksave_msg.c_str(), TEXT("NppShade"), MB_YESNO | MB_ICONQUESTION) != IDYES) {
                            autoencrypt = true;
                        }
                    } else {
                        autoencrypt = true;
                    }
                }
                if(!autoencrypt) {
                    int encoding = (int)::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, notifyCode->nmhdr.idFrom, 0);
                    bool no_ascii = (encoding != uni8Bit && encoding != uniUTF8 && encoding != uniCookie) ? true : false;

                    if(!dlg_crypt.encryptDialog(&crypt, &initdata.iv, &filename)) {
                        if (old_encryption_available) {
                            crypt = fiter->second;
                            crypt_file_cursave = NppShadeFileSave::OLD_ENCRYPTION;
                        } else {
                            crypt_file_cursave = NppShadeFileSave::NO_ENCRYPTION;
                            return;
                        }
                    }
                }

                nppcrypt::encrypt(pData, data_length, buffer, crypt.options, crypt.password, initdata);
                header.create(crypt.options, initdata, &buffer[0], buffer.size());
                if (!old_encryption_available) {
                    crypt_files.insert(std::pair<std::wstring, CryptInfo>(path, crypt));
                }

                ::SendMessage(hCurScintilla, SCI_BEGINUNDOACTION, 0, 0);
                ::SendMessage(hCurScintilla, SCI_CLEARALL, 0, 0);
                ::SendMessage(hCurScintilla, SCI_APPENDTEXT, header.size(), (LPARAM)header.c_str());
                ::SendMessage(hCurScintilla, SCI_APPENDTEXT, buffer.size(), (LPARAM)&buffer[0]);
                ::SendMessage(hCurScintilla, SCI_ENDUNDOACTION, 0, 0);
            }
        } catch (ExcInfo& exc) {
            if (exc.getID() != ExcInfo::ID::file_empty) {
                msgbox::info(nppData._nppHandle, exc.what(), exc.getURL(), exc.getURLCaption());
            }
        } catch(std::exception& exc) {
            msgbox::error(nppData._nppHandle, exc.what());
        } catch(...) {
            msgbox::error(nppData._nppHandle, "未知错误！");
        }
        break;
    }
    case NPPN_FILEBEFORECLOSE:
    {
        std::wstring path, filename, extension;
        help::buffer::getPath(notifyCode->nmhdr.idFrom, path, filename, extension);
        if (isManagedExtension(extension)) {
            crypt_files.erase(path);
        }
        break;
    }
    }
}

// ====================================================================================================================================================================

static void EncryptWholeFile()
{
    const std::wstring extension = managedExtension();
    if (!preferences.files.enable || extension.empty() ||
        extension.find_first_of(L"\\/:*?\"<>| \t.") != std::wstring::npos) {
        msgbox::info(nppData._nppHandle,
            "请在“首选项 → 整文件加密”中启用整文件加密，并填写有效扩展名，多个扩展名用英文分号分隔。");
        return;
    }

    HWND hCurScintilla = help::scintilla::getCurrent();
    const int data_length = (int)::SendMessage(hCurScintilla, SCI_GETLENGTH, 0, 0);
    if (data_length <= 0) {
        throwInfo(file_empty);
    }
    if (::SendMessage(hCurScintilla, SCI_GETREADONLY, 0, 0)) {
        msgbox::info(nppData._nppHandle, "当前文件为只读，无法加密保存。");
        return;
    }

    const uptr_t buffer_id = help::buffer::getCurrent();
    std::wstring path, filename, current_extension;
    try {
        help::buffer::getPath(buffer_id, path, filename, current_extension);
    } catch (const ExcError&) {
        filename = localization::translate(L"新建文件");
    }

    const bool same_extension = isAbsolutePath(path) && isManagedExtension(current_extension);
    if (same_extension && crypt_files.find(path) != crypt_files.end()) {
        // Save As with the same name always invokes the save notifications,
        // including when the document has no unsaved edits.
        if (!::SendMessage(nppData._nppHandle, NPPM_SAVECURRENTFILEAS, FALSE, (LPARAM)path.c_str())) {
            msgbox::error(nppData._nppHandle, "整文件加密保存失败。");
        }
        return;
    }

    const byte* pData = (const byte*)::SendMessage(hCurScintilla, SCI_GETCHARACTERPOINTER, 0, 0);
    if (!pData) {
        throwError(no_scintilla_pointer);
    }
    if (data_length >= 9 && (memcmp(pData, "<nppshade", 9) == 0 || memcmp(pData, "<nppcrypt", 9) == 0)) {
        msgbox::info(nppData._nppHandle, localization::translateUtf8("当前内容已包含 NppShade 加密文件头，请先解密后再编辑或保存。").c_str());
        return;
    }

    std::wstring target_path;
    if (same_extension) {
        target_path = path;
    } else if (isAbsolutePath(path)) {
        target_path = path + L"." + extension;
        if (pathExistsOrUncertain(target_path)) {
            const std::wstring suggested = path + L".encrypted." + extension;
            if (!chooseEncryptedPath(suggested, extension, target_path)) {
                return;
            }
        }
    } else {
        const std::wstring suggested = (filename.empty() ? localization::translate(L"新建文件") : filename) + L"." + extension;
        if (!chooseEncryptedPath(suggested, extension, target_path)) {
            return;
        }
    }

    std::wstring target_filename = target_path.substr(target_path.find_last_of(L"\\/") + 1);
    CryptInfo crypt = current.crypt;
    nppcrypt::InitData initdata;
    if (!dlg_crypt.encryptDialog(&crypt, &initdata.iv, &target_filename)) {
        return;
    }

    std::basic_string<byte> encrypted;
    nppcrypt::encrypt(pData, data_length, encrypted, crypt.options, crypt.password, initdata);
    CryptHeaderWriter header(crypt.hmac);
    header.create(crypt.options, initdata, &encrypted[0], encrypted.size());

    pending_whole_file_save.replaced = false;
    pending_whole_file_save.buffer_id = buffer_id;
    pending_whole_file_save.target_path = target_path;
    pending_whole_file_save.ciphertext.assign((const byte*)header.c_str(), header.size());
    pending_whole_file_save.ciphertext.append(encrypted.data(), encrypted.size());
    pending_whole_file_save.active = true;

    const bool was_modified = !!::SendMessage(hCurScintilla, SCI_GETMODIFY, 0, 0);
    const bool saved = !!::SendMessage(nppData._nppHandle, NPPM_SAVECURRENTFILEAS, FALSE,
        (LPARAM)pending_whole_file_save.target_path.c_str());
    pending_whole_file_save.ciphertext.clear();
    pending_whole_file_save.target_path.clear();
    pending_whole_file_save.active = false;
    if (saved) {
        crypt_files[target_path] = crypt;
    } else {
        if (pending_whole_file_save.replaced) {
            ::SendMessage(hCurScintilla, SCI_UNDO, 0, 0);
        }
        if (was_modified && !::SendMessage(hCurScintilla, SCI_GETMODIFY, 0, 0)) {
            const byte first = *(const byte*)::SendMessage(hCurScintilla, SCI_GETCHARACTERPOINTER, 0, 0);
            ::SendMessage(hCurScintilla, SCI_SETTARGETSTART, 0, 0);
            ::SendMessage(hCurScintilla, SCI_SETTARGETEND, 1, 0);
            ::SendMessage(hCurScintilla, SCI_REPLACETARGET, 1, (LPARAM)&first);
        }
        msgbox::error(nppData._nppHandle, "整文件加密保存失败，原始内容仍在编辑器中。");
    }
    pending_whole_file_save.replaced = false;
    crypt.password.clear();
}

void EncryptDlg()
{
    try {
        const byte* pData;
        size_t      data_length;
        size_t      sel_start;

        if (!help::scintilla::getSelection(&pData, &data_length, &sel_start)) {
            EncryptWholeFile();
            return;
        }

        nppcrypt::InitData             initdata;

        if(dlg_crypt.encryptDialog(&current.crypt, &initdata.iv)) {

            std::basic_string<byte>     buffer;
            nppcrypt::encrypt(pData, data_length, buffer, current.crypt.options, current.crypt.password, initdata);

            CryptHeaderWriter header(current.crypt.hmac);
            header.create(current.crypt.options, initdata, &buffer[0], buffer.size());

            HWND hCurScintilla = help::scintilla::getCurrent();
            ::SendMessage(hCurScintilla, SCI_BEGINUNDOACTION, 0, 0);
            ::SendMessage(hCurScintilla, SCI_TARGETFROMSELECTION, 0, 0);
            ::SendMessage(hCurScintilla, SCI_REPLACETARGET, header.size(), (LPARAM)header.c_str());
            ::SendMessage(hCurScintilla, SCI_SETSEL, sel_start + header.size(), sel_start + header.size());
            ::SendMessage(hCurScintilla, SCI_TARGETFROMSELECTION, 0, 0);
            ::SendMessage(hCurScintilla, SCI_REPLACETARGET, buffer.size(), (LPARAM)&buffer[0]);
            ::SendMessage(hCurScintilla, SCI_SETSEL, sel_start, sel_start + header.size() + buffer.size());
            ::SendMessage(hCurScintilla, SCI_ENDUNDOACTION, 0, 0);

            current.crypt.password.clear();
        }
    } catch (ExcInfo& exc) {
        msgbox::info(nppData._nppHandle, exc.what(), exc.getURL(), exc.getURLCaption());
    } catch (std::exception& exc) {
        msgbox::error(nppData._nppHandle, exc.what());
    } catch (...) {
        msgbox::error(nppData._nppHandle, "未知错误！");
    }
}

void DecryptDlg()
{
    try
    {
        const byte* pData;
        size_t      data_length;
        size_t      sel_start;

        if (!help::scintilla::getSelection(&pData, &data_length, &sel_start)) {
            throwInfo(no_text_selected);
        }

        nppcrypt::InitData     initdata;
        CryptHeaderReader   header(current.crypt.hmac);

        /* parse header and check hmac if present */
        bool header_found = header.parse(current.crypt.options, initdata, pData, data_length);
        if (header_found) {
            if (current.crypt.hmac.enable) {
                if (current.crypt.hmac.keypreset_id < 0) {
                    if (!dlg_auth.doDialog()) {
                        return;
                    }
                    current.crypt.hmac.hash.key.set(dlg_auth.getInput());
                    if (!header.checkHMAC()) {
                        throwInfo(hmac_auth_failed);
                    }
                } else {
                    if (current.crypt.hmac.keypreset_id >= preferences.getKeyNum()) {
                        throwInvalid(invalid_keypreset_id);
                    } else {
                        current.crypt.hmac.hash.key.set(preferences.getKey(current.crypt.hmac.keypreset_id), 16);
                        if (!header.checkHMAC()) {
                            throwInfo(hmac_auth_failed);
                        }
                    }
                }
            }
        }

        std::basic_string<byte> buffer;
        const std::basic_string<byte> ciphertext(header.getEncrypted(), header.getEncryptedLength());
        if (dlg_crypt.decryptDialog(&current.crypt, &initdata.iv, [&]() {

            /* check if salt or tag data is mising */
            size_t need_salt_len = (current.crypt.options.key.salt_bytes > 0 && initdata.salt.size() != current.crypt.options.key.salt_bytes) ? current.crypt.options.key.salt_bytes : 0;
            size_t need_tag_len = 0;
            if (nppcrypt::help::checkProperty(current.crypt.options.cipher, nppcrypt::BLOCK)) {
                if (current.crypt.options.mode == nppcrypt::Mode::gcm && initdata.tag.size() != nppcrypt::Constants::gcm_tag_size) {
                    need_tag_len = nppcrypt::Constants::gcm_tag_size;
                } else if (current.crypt.options.mode == nppcrypt::Mode::ccm && initdata.tag.size() != nppcrypt::Constants::ccm_tag_size) {
                    need_tag_len = nppcrypt::Constants::ccm_tag_size;
                } else if (current.crypt.options.mode == nppcrypt::Mode::eax && initdata.tag.size() != nppcrypt::Constants::eax_tag_size) {
                    need_tag_len = nppcrypt::Constants::eax_tag_size;
                }
            }
            if (need_salt_len > 0 || need_tag_len > 0) {
                if (!dlg_initdata.doDialog(&initdata, need_salt_len, need_tag_len)) {
                    return false;
                }
            }

            buffer.clear();
            decrypt(ciphertext.data(), ciphertext.size(), buffer, current.crypt.options, current.crypt.password, initdata);
            return true;
        }, NULL, !header_found)) {
            help::scintilla::replaceSelection(buffer);

            current.crypt.password.clear();
        }
    } catch (ExcInfo& exc) {
        msgbox::info(nppData._nppHandle, exc.what(), exc.getURL(), exc.getURLCaption());
    } catch (std::exception& exc) {
        msgbox::error(nppData._nppHandle, exc.what());
    } catch (...) {
        msgbox::error(nppData._nppHandle, "未知错误！");
    }
}

void HashDlg()
{
    try {
        dlg_hash.doDialog();
    } catch (ExcInfo& exc) {
        msgbox::info(nppData._nppHandle, exc.what(), exc.getURL(), exc.getURLCaption());
    } catch (std::exception& exc) {
        msgbox::error(nppData._nppHandle, exc.what());
    } catch (...) {
        msgbox::error(nppData._nppHandle, "未知错误！");
    }
}

void RandomDlg()
{
    try {
        dlg_random.doDialog();
    } catch (ExcInfo& exc) {
        msgbox::info(nppData._nppHandle, exc.what(), exc.getURL(), exc.getURLCaption());
    } catch (std::exception& exc) {
        msgbox::error(nppData._nppHandle, exc.what());
    } catch (...) {
        msgbox::error(nppData._nppHandle, "未知错误！");
    }
}

void ConvertDlg()
{
    try {
        if (help::scintilla::getSelectionLength()) {
            dlg_convert.doDialog();
        } else {
            throwInfo(no_convert_text_selected);
        }
    } catch (ExcInfo& exc) {
        msgbox::info(nppData._nppHandle, exc.what(), exc.getURL(), exc.getURLCaption());
    } catch (std::exception& exc) {
        msgbox::error(nppData._nppHandle, exc.what());
    } catch (...) {
        msgbox::error(nppData._nppHandle, "未知错误！");
    }
}

void PreferencesDlg()
{
    dlg_preferences.doDialog();
}

void AboutDlg()
{
    dlg_about.doDialog();
}
