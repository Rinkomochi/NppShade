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

#include "exception.h"

ExcError::ExcError(ID id, const char* func, unsigned int line) noexcept : id(id), line(line)
{
    std::ostringstream o;
    o << "[" << func << ":" << line << "] " << messages[(unsigned)id];
    msg.assign(o.str());
};

const char* ExcError::messages[] = {
    "发生意外错误。",
    "无法获取 Scintilla 字符指针。",
    "无法获取 Scintilla 窗口句柄。",
    "无法获取文件路径。",
    "转换为 UTF-8 失败。",
    "转换为宽字符失败。",
    "无法读取首选项文件。",
    "无法解析首选项文件。",
    "无法生成文件头。",
    "无法写入输出文件。",
    "无法读取输入文件。"
};

const char* ExcInvalid::messages[] = {
    "预设密钥编号无效。",
    "密码模式无效。",
    "密钥长度无效。",
    "PBKDF2 参数无效。",
    "bcrypt 参数无效。",
    "scrypt 参数无效。",
    "盐值长度无效。",
    "bcrypt 的盐值必须为 16 字节。",
    "每行长度无效。",
    "该哈希算法不支持所选摘要长度。",
    "文件头无效。",    
    "无法解析文件头版本。",
    "HMAC 数据无效。",
    "HMAC 哈希算法无效。",
    "密码算法无效。",
    "编码方式无效。",
    "密钥派生算法无效。",
    "无法解析盐值。",
    "无法解析初始向量。",
    "无法解析认证标签。",
    "无法解析附加认证数据标志。",
    "哈希算法无效。",
    "无法解析哈希密钥。",
    "操作参数无效。",
    "不能转换为相同的编码。",
    "行结束符无效。",
    "大小写选项无效。",
    "未找到文件头。",
    "缺少密钥长度。",
    "缺少密码模式。",
    "缺少 HMAC 密钥。",
    "缺少初始向量。",
    "缺少盐值。",
    "缺少密码。",
    "该哈希算法不支持密钥。",
    "该哈希算法需要密钥。",
    "仅支持解密 UTF-8 文件。"
};

const char* ExcInfo::messages[] = {
    "文件为空。",
    "HMAC 验证失败。",
    "此文件头版本已不受支持。",
    "请选择要解密的文本。加密文件可直接打开，并在输入密码后自动解密。",
    "请选择要转换的文本。"
};

const char* ExcInfo::urls[] = {
    NULL,
    "https://github.com/Rinkomochi/Nppcrypt-CN",
    "https://github.com/Rinkomochi/Nppcrypt-CN",
    "https://github.com/Rinkomochi/Nppcrypt-CN",
    NULL
};

const char* ExcInfo::url_captions[] = {
    NULL,
    "了解更多",
    "获取帮助",
    "了解更多",
    NULL
};
