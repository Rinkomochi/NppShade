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

#ifndef MDEF_DEFINE_H
#define MDEF_DEFINE_H

#define     NPPC_NAME                   "NppShade"
#define     NPPC_VERSION                2000

#define     NPPC_FUNC_COUNT             9
#define     NPPC_FUNC_HASH_ID           2
#define     NPPC_FUNC_RANDOM_ID         3
#define     NPPC_FUNC_CONVERT_ID        4

#define     NPPC_ABOUT_VERSION          "2.0"
#define     NPPC_ABOUT_GITHUB           "https://github.com/Rinkomochi/Nppcrypt-CN"
#define     NPPC_UPSTREAM_GITHUB        "https://github.com/chcg/nppcrypt"
#define     NPPC_ABOUT_CRYPTOPP         "https://www.cryptopp.com"
#define     NPPC_ABOUT_TINYXML2         "https://github.com/leethomason/tinyxml2"
#define     NPPC_ABOUT_BCRYPT           "http://www.openwall.com/crypt"
#define     NPPC_ABOUT_SCRYPT           "https://www.tarsnap.com/scrypt.html"

#define     NPPC_DEF_HMAC_KEY           "bJmX/AokoOlC}my."
#define     NPPC_DEF_HMAC_LABEL         "nppshade"

#define     NPPC_HMAC_MAX_KEYS          20
#define     NPPC_MAX_HMAC_KEYLENGTH     512
#define     NPPC_MAX_PRESET_LABELLENGTH 30

#define     NPPC_MAX_LINE_LENGTH        9999
#define     NPPC_PASSWORD_MAXLENGTH     1024
#define     NPPC_MAX_CUSTOM_IV_LENGTH   512

#define     NPPC_DEF_FILE_EXT           "nppshade"
#define     NPPC_FILE_EXT_MAXLENGTH     512

#define     NPPC_CRYPT_HMAC_HELP_URL    NPPC_ABOUT_GITHUB
#define     NPPC_CRYPT_IV_HELP_URL      NPPC_ABOUT_GITHUB
#define     NPPC_CRYPT_SALT_HELP_URL    NPPC_ABOUT_GITHUB
#define     NPPC_CRYPT_SALT_HELP        "盐值由 Windows 随机数生成器产生"
#define     NPPC_RANDOM_HELP_URL        NPPC_ABOUT_GITHUB
#define     NPPC_CRYPT_AUTH_IVSALT_URL  NPPC_ABOUT_GITHUB
#define     NPPC_EASYMODE_HELP_URL      NPPC_ABOUT_GITHUB

#define     NPPC_BASE32_ALPHABET        "abcdefghijkmnpqrstuvwxyz23456789"
#define     NPPC_BASE32_PAD             0
#define     NPPC_BASE64_ALPHABET        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
#define     NPPC_BASE64_PAD             '='

#endif
