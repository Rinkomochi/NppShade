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

#include "dlg_preferences.h"
#include "preferences.h"
#include "resource.h"
#include "commctrl.h"
#include "cryptopp/osrng.h"
#include "cryptopp/base64.h"
#include "managed_extensions.h"
#include "localization.h"

INT_PTR CALLBACK DlgPreferences::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        original_language = preferences.ui_language;
        localization::setLanguage(preferences.ui_language);
        const wchar_t* languages[] = { L"跟随系统", L"简体中文", L"English", L"日本語", L"한국어" };
        for (int i = 0; i < 5; ++i) {
            const std::wstring label = localization::translate(languages[i]);
            ::SendDlgItemMessage(_hSelf, IDC_PREF_LANGUAGE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
        ::SendDlgItemMessage(_hSelf, IDC_PREF_LANGUAGE, CB_SETCURSEL, preferences.ui_language, 0);
        ::SendDlgItemMessage(_hSelf, IDC_PREF_FILES_ENABLE, BM_SETCHECK, preferences.files.enable, 0);
        ::SendDlgItemMessage(_hSelf, IDC_PREF_FILES_ASK, BM_SETCHECK, preferences.files.askonsave, 0);
        ::SendDlgItemMessage(_hSelf, IDC_PREF_FILES_EXT, EM_LIMITTEXT, NPPC_FILE_EXT_MAXLENGTH, 0);
        ::SetDlgItemText(_hSelf, IDC_PREF_FILES_EXT, preferences.files.extension.c_str());

        ::EnableWindow(::GetDlgItem(_hSelf, IDC_PREF_FILES_ASK), preferences.files.enable);
        ::EnableWindow(::GetDlgItem(_hSelf, IDC_PREF_FILES_EXT), preferences.files.enable);
        ::EnableWindow(::GetDlgItem(_hSelf, IDC_PREF_FILES_STATIC_EXT), preferences.files.enable);

        ::SendDlgItemMessage(_hSelf, IDC_PREF_KEYS_VALUE, EM_LIMITTEXT, 24, 0);
        ::SendDlgItemMessage(_hSelf, IDC_PREF_KEYS_LABEL, EM_LIMITTEXT, NPPC_MAX_PRESET_LABELLENGTH, 0);
        for (size_t i = 0; i < preferences.getKeyNum(); i++) {
            ::SendDlgItemMessage(_hSelf, IDC_PREF_KEYS_LIST, LB_ADDSTRING, 0, (LPARAM)preferences.getKeyLabel(i));
        }

        if (preferences.failed()) {
            ::SetDlgItemText(_hSelf, IDC_PREF_ERROR, TEXT("（配置文件加载失败）"));
            url_error.init(_hInst, _hSelf);
            url_error.create(::GetDlgItem(_hSelf, IDC_PREF_ERROR), NPPC_ABOUT_GITHUB, RGB(255, 0, 0), false);
        }

        goToCenter();
        return TRUE;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_OK:
        {
            TCHAR tstr[NPPC_FILE_EXT_MAXLENGTH + 1];
            ::GetDlgItemText(_hSelf, IDC_PREF_FILES_EXT, tstr, NPPC_FILE_EXT_MAXLENGTH + 1);
            std::vector<std::wstring> extensions;
            if (!parseManagedExtensions(tstr, extensions)) {
                const std::wstring text = localization::translate(L"请输入有效扩展名，多个扩展名用英文分号分隔，例如 nppshade;enc;crypt。");
                const std::wstring title = localization::translate(L"扩展名无效");
                ::MessageBox(_hSelf, text.c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
                ::SetFocus(::GetDlgItem(_hSelf, IDC_PREF_FILES_EXT));
                return TRUE;
            }
            std::wstring normalized;
            for (const auto& item : extensions) {
                if (!normalized.empty()) normalized += L";";
                normalized += item;
            }
            preferences.files.enable = !!::SendDlgItemMessage(_hSelf, IDC_PREF_FILES_ENABLE, BM_GETCHECK, 0, 0);
            preferences.files.askonsave = !!::SendDlgItemMessage(_hSelf, IDC_PREF_FILES_ASK, BM_GETCHECK, 0, 0);

            preferences.files.extension = normalized;
            preferences.ui_language = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_PREF_LANGUAGE, CB_GETCURSEL, 0, 0));
            localization::setLanguage(preferences.ui_language);

            EndDialog(_hSelf, IDC_OK);
            return TRUE;
        }
        case IDC_CANCEL : case IDCANCEL:
        {
            preferences.ui_language = original_language;
            localization::setLanguage(original_language);
            EndDialog(_hSelf, IDC_CANCEL);
            return TRUE;
        }
        case IDC_PREF_LANGUAGE:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                const int selected = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_PREF_LANGUAGE, CB_GETCURSEL, 0, 0));
                if (selected >= 0 && selected <= 4) {
                    preferences.ui_language = selected;
                    localization::setLanguage(selected);
                    localization::translateDialog(_hSelf);
                }
            }
            return TRUE;
        }
        case IDC_PREF_FILES_ENABLE:
        {
            bool c = ::SendDlgItemMessage(_hSelf, IDC_PREF_FILES_ENABLE, BM_GETCHECK, 0, 0) ? true : false;
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_PREF_FILES_ASK), c);
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_PREF_FILES_EXT), c);
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_PREF_FILES_STATIC_EXT), c);
            break;
        }
        case IDC_PREF_KEYS_ADD:
        {
            if (HIWORD(wParam) == BN_CLICKED) {
                if (preferences.getKeyNum() >= 20) {
                    const std::wstring text = localization::translate(L"最多只能保存 20 个预设密钥。");
                    const std::wstring title = localization::translate(L"错误");
                    ::MessageBox(_hSelf, text.c_str(), title.c_str(), MB_OK);
                    return FALSE;
                }
                CPreferences::KeyPreset temp;
                ::GetDlgItemText(_hSelf, IDC_PREF_KEYS_LABEL, temp.label, 31);
                if (!lstrlen(temp.label)) {
                    const std::wstring text = localization::translate(L"请输入密钥名称。");
                    const std::wstring title = localization::translate(L"错误");
                    ::MessageBox(_hSelf, text.c_str(), title.c_str(), MB_OK);
                    return FALSE;
                }
                const std::wstring default_label = localization::translate(L"nppshade");
                if (lstrcmp(temp.label, default_label.c_str()) == 0) {
                    const std::wstring text = localization::translate(L"请使用其他密钥名称。");
                    const std::wstring title = localization::translate(L"错误");
                    ::MessageBox(_hSelf, text.c_str(), title.c_str(), MB_OK);
                    return FALSE;
                }
                TCHAR   tvalue[25];
                byte    tstr[24];
                size_t  i;
                memset(temp.data, 0, 16);
                ::GetDlgItemText(_hSelf, IDC_PREF_KEYS_VALUE, tvalue, 25);
                for (i = 0; i < 24; i++) {
                    tstr[i] = static_cast<byte>(tvalue[i]);
                }

                using namespace CryptoPP;
                StringSource((const byte*)tstr, 24, true, new Base64Decoder(new ArraySink(temp.data, 16)));

                for (i = 0; i < 16; i++) {
                    if (temp.data[i] != 0) {
                        break;
                    }
                }
                if (i == 16) {
                    const std::wstring text = localization::translate(L"密钥值必须是 Base64 编码的 16 字节数据。");
                    const std::wstring title = localization::translate(L"错误");
                    ::MessageBox(_hSelf, text.c_str(), title.c_str(), MB_OK); break;
                }

                preferences.addKey(temp);
                ::SendDlgItemMessage(_hSelf, IDC_PREF_KEYS_LIST, LB_ADDSTRING, 0, (LPARAM)temp.label);
                ::SendDlgItemMessage(_hSelf, IDC_PREF_KEYS_LIST, LB_SETCURSEL, preferences.getKeyNum() - 1, 0);
            }
            break;
        }
        case IDC_PREF_KEYS_DEL:
        {
            if (HIWORD(wParam) == BN_CLICKED) {
                int sel = (int)::SendDlgItemMessage(_hSelf, IDC_PREF_KEYS_LIST, LB_GETCURSEL, 0, 0);
                if (sel > 0 && preferences.delKey((size_t)sel)) {
                    ::SendDlgItemMessage(_hSelf, IDC_PREF_KEYS_LIST, LB_DELETESTRING, sel, 0);
                    ::SetDlgItemText(_hSelf, IDC_PREF_KEYS_LABEL, TEXT(""));
                    ::SetDlgItemText(_hSelf, IDC_PREF_KEYS_VALUE, TEXT(""));
                }
            }
            break;
        }
        case IDC_PREF_KEYS_RANDOM:
        {
            if (HIWORD(wParam) == BN_CLICKED) {
                using namespace CryptoPP;
                unsigned char t_rand[16];
                OS_GenerateRandomBlock(true, t_rand, 16);
                TCHAR tvalue[25];
                char t_rand_s[24];
                ArraySource(t_rand, 16, true, new Base64Encoder(new ArraySink((byte*)t_rand_s, 24)));
                for (size_t i = 0; i < 24; i++) {
                    tvalue[i] = static_cast<TCHAR>(t_rand_s[i]);
                }
                tvalue[24] = 0;
                ::SetDlgItemText(_hSelf, IDC_PREF_KEYS_VALUE, tvalue);
            }
            break;
        }
        case IDC_PREF_KEYS_LIST:
        {
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                int sel = (int)::SendDlgItemMessage(_hSelf, IDC_PREF_KEYS_LIST, LB_GETCURSEL, 0, 0);
                if (sel >= 0) {
                    using namespace CryptoPP;
                    char tstr[24];
                    TCHAR tvalue[25];
                    ArraySource(preferences.getKey((size_t)sel), 16, true, new Base64Encoder(new ArraySink((byte*)tstr, 24)));
                    for (size_t i = 0; i < 24; i++) {
                        tvalue[i] = tstr[i];
                    }
                    tvalue[24] = 0;
                    ::SetDlgItemText(_hSelf, IDC_PREF_KEYS_VALUE, tvalue);
                    const std::wstring label = localization::translate(preferences.getKeyLabel((size_t)sel));
                    ::SetDlgItemText(_hSelf, IDC_PREF_KEYS_LABEL, label.c_str());
                }
            }
            break;
        }
        }
        break;
    }
    }
    return FALSE;
}

