#pragma once

#include <windows.h>
#include <vector>
#include <algorithm>
#include <cstring>

// Scale plugin windows only; never change the Notepad++ process DPI policy.
class DialogLayout
{
    struct Control { HWND hwnd; RECT rect; LOGFONT font; bool has_font; };
    std::vector<Control> controls;
    std::vector<HFONT> fonts;
    SIZE client = {};
    UINT source_dpi = 96;
    HWND root = NULL;

    static UINT dpi(HWND hwnd) {
        typedef UINT (WINAPI *GetDpi)(HWND);
        GetDpi get = NULL;
        auto address = GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
        static_assert(sizeof(get) == sizeof(address), "Windows API pointer size");
        std::memcpy(&get, &address, sizeof(get));
        if (get) { UINT value = get(hwnd); if (value) return value; }
        HDC dc = GetDC(hwnd);
        UINT value = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(hwnd, dc);
        return value ? value : 96;
    }
    static BOOL CALLBACK collect(HWND hwnd, LPARAM param) {
        auto self = reinterpret_cast<DialogLayout*>(param);
        Control item = {};
        item.hwnd = hwnd;
        GetWindowRect(hwnd, &item.rect);
        MapWindowPoints(NULL, GetParent(hwnd), reinterpret_cast<POINT*>(&item.rect), 2);
        HFONT font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
        item.has_font = font && GetObject(font, sizeof(item.font), &item.font);
        self->controls.push_back(item);
        disableAutoScale(hwnd);
        return TRUE;
    }
public:
    ~DialogLayout() { clear(); }
    void clear() {
        for (auto font : fonts) DeleteObject(font);
        fonts.clear(); controls.clear(); root = NULL;
    }
    static void disableAutoScale(HWND hwnd) {
        typedef BOOL (WINAPI *SetBehavior)(HWND, int, int);
        auto module = GetModuleHandleW(L"user32.dll");
        SetBehavior dialog = NULL, control = NULL;
        auto address = GetProcAddress(module, "SetDialogDpiChangeBehavior");
        std::memcpy(&dialog, &address, sizeof(dialog));
        address = GetProcAddress(module, "SetDialogControlDpiChangeBehavior");
        std::memcpy(&control, &address, sizeof(control));
        if (dialog) dialog(hwnd, 1, 1); // DDC_DISABLE_ALL
        if (control) control(hwnd, 3, 3); // font and relayout are handled together below
    }
    void initialize(HWND hwnd, HWND owner) {
        clear(); root = hwnd;
        disableAutoScale(hwnd);
        RECT rect;
        GetClientRect(hwnd, &rect);
        client = { rect.right, rect.bottom };
        source_dpi = dpi(hwnd);
        EnumChildWindows(hwnd, collect, reinterpret_cast<LPARAM>(this));
        apply(dpi(owner), NULL, owner);
    }
    void apply(UINT target_dpi, const RECT* suggested, HWND owner) {
        if (!root) return;
        MONITORINFO monitor = { sizeof(monitor) };
        HMONITOR selected = suggested ? MonitorFromRect(suggested, MONITOR_DEFAULTTONEAREST)
                                     : MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
        if (!GetMonitorInfo(selected, &monitor)) return;
        RECT window, current_client;
        GetWindowRect(root, &window); GetClientRect(root, &current_client);
        const int frame_x = window.right - window.left - current_client.right;
        const int frame_y = window.bottom - window.top - current_client.bottom;
        const int margin = MulDiv(8, target_dpi, 96);
        double factor = static_cast<double>(target_dpi) / source_dpi;
        factor = (std::min)(factor, static_cast<double>((std::max)(1L, monitor.rcWork.right - monitor.rcWork.left - frame_x - 2 * margin)) / client.cx);
        factor = (std::min)(factor, static_cast<double>((std::max)(1L, monitor.rcWork.bottom - monitor.rcWork.top - frame_y - 2 * margin)) / client.cy);
        auto scaled = [factor](LONG value) { return static_cast<int>(value * factor + (value < 0 ? -0.5 : 0.5)); };
        int width = scaled(client.cx) + frame_x, height = scaled(client.cy) + frame_y;
        RECT owner_rect; GetWindowRect(owner, &owner_rect);
        int x = suggested ? suggested->left : (owner_rect.left + owner_rect.right - width) / 2;
        int y = suggested ? suggested->top : (owner_rect.top + owner_rect.bottom - height) / 2;
        x = (std::max)(monitor.rcWork.left, (std::min)(static_cast<LONG>(x), monitor.rcWork.right - width));
        y = (std::max)(monitor.rcWork.top, (std::min)(static_cast<LONG>(y), monitor.rcWork.bottom - height));
        SetWindowPos(root, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        std::vector<HFONT> next_fonts;
        for (const auto& item : controls) {
            if (!IsWindow(item.hwnd)) continue;
            const RECT& r = item.rect;
            MoveWindow(item.hwnd, scaled(r.left), scaled(r.top), scaled(r.right) - scaled(r.left), scaled(r.bottom) - scaled(r.top), FALSE);
            if (item.has_font) {
                LOGFONT font = item.font;
                font.lfHeight = scaled(font.lfHeight);
                font.lfWidth = scaled(font.lfWidth);
                HFONT created = CreateFontIndirect(&font);
                if (created) {
                    SendMessage(item.hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(created), FALSE);
                    next_fonts.push_back(created);
                }
            }
        }
        for (auto font : fonts) DeleteObject(font);
        fonts.swap(next_fonts);
        RedrawWindow(root, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
};
