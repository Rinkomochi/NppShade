#pragma once

#include <string>
#include <vector>
#include <cwctype>

// Keep the original XML attribute: one extension is also a valid list.
inline bool parseManagedExtensions(const std::wstring& text, std::vector<std::wstring>& result)
{
    result.clear();
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find(L';', start);
        std::wstring item = text.substr(start, end == std::wstring::npos ? end : end - start);
        const size_t first = item.find_first_not_of(L" \t");
        if (first != std::wstring::npos) {
            item.erase(0, first);
            item.erase(item.find_last_not_of(L" \t") + 1);
            if (item[0] == L'.') item.erase(0, 1);
            if (item.empty() || item.find_first_of(L"\\/:*?\"<>| .\t\r\n") != std::wstring::npos) {
                result.clear();
                return false;
            }
            for (auto& c : item) c = static_cast<wchar_t>(std::towlower(c));
            bool duplicate = false;
            for (const auto& existing : result) duplicate = duplicate || existing == item;
            if (!duplicate) result.push_back(item);
        }
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    return !result.empty();
}
