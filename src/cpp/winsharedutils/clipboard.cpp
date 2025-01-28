﻿
bool tryopenclipboard(HWND hwnd = 0)
{
    bool success = false;
    for (int i = 0; i < 50; i++)
    {
        if (OpenClipboard(hwnd))
        {
            success = true;
            break;
        }
        else
        {
            Sleep(10);
        }
    }
    return success;
}

std::optional<std::wstring> clipboard_get_internal()
{
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
        return {};
    if (tryopenclipboard() == false)
        return {};
    std::optional<std::wstring> data = {};
    do
    {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData == 0)
            break;
        LPWSTR pszText = static_cast<LPWSTR>(GlobalLock(hData));
        if (pszText == 0)
            break;
        data = std::move(std::wstring(pszText));
        GlobalUnlock(hData);
    } while (false);
    CloseClipboard();
    return data;
}

DECLARE_API bool clipboard_get(void (*cb)(const wchar_t *))
{
    auto data = std::move(clipboard_get_internal());
    if (!data)
        return false;
    cb(data.value().c_str());
    return true;
}

DECLARE_API bool clipboard_set(HWND hwnd, wchar_t *text)
{
    bool success = false;
    if (tryopenclipboard(hwnd) == false)
        return false;
    EmptyClipboard();
    do
    {
        HGLOBAL hClipboardData;
        size_t len = wcslen(text) + 1;
        hClipboardData = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
        if (hClipboardData == 0)
            break;
        auto pchData = (wchar_t *)GlobalLock(hClipboardData);
        if (pchData == 0)
        {
            GlobalFree(hClipboardData);
            break;
        }
        wcscpy_s(pchData, len, text);
        GlobalUnlock(hClipboardData);
        if (SetClipboardData(CF_UNICODETEXT, hClipboardData))
            success = true;
        else
        {
            GlobalFree(hClipboardData);
        }

    } while (false);
    CloseClipboard();
    return success;
}
inline bool iscurrentowndclipboard()
{
    auto ohwnd = GetClipboardOwner();
    DWORD pid;
    GetWindowThreadProcessId(ohwnd, &pid);
    return pid == GetCurrentProcessId();
}

#ifndef WINXP
#define addClipboardFormatListener AddClipboardFormatListener
#define removeClipboardFormatListener RemoveClipboardFormatListener
#else
#ifndef __C65AB19A_FFA2_4BB6_B2D2_508A6509AD7A__
#define __C65AB19A_FFA2_4BB6_B2D2_508A6509AD7A__
struct INFO_EFB07CE8_C478_475C_9B6D_1862D6400474
{
    HWND hwndNextViewer = NULL;
    HWND Listener = NULL;
};
auto KLASS_4420FB75_2931_459E_BA36_B2484F6DC9EE = TEXT("4420FB75_2931_459E_BA36_B2484F6DC9EE");
auto PROP_5B7BE8DE_E87B_4FC7_8562_48E4E42880DB = TEXT("5B7BE8DE_E87B_4FC7_8562_48E4E42880DB");
LRESULT CALLBACK WNDPROC_8DDD0332_D337_4F76_AF3C_D0CF23E94191(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DRAWCLIPBOARD:
    case WM_CHANGECBCHAIN:
    case WM_DESTROY:
    {
        auto info = reinterpret_cast<INFO_EFB07CE8_C478_475C_9B6D_1862D6400474 *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!info)
            return DefWindowProc(hwnd, msg, wParam, lParam);
        switch (msg)
        {
        case WM_DRAWCLIPBOARD:
        {
            if (info->hwndNextViewer)
                SendMessage(info->hwndNextViewer, msg, wParam, lParam);
            PostMessage(info->Listener, WM_CLIPBOARDUPDATE, 0, 0);
        }
        break;
        case WM_CHANGECBCHAIN:
        {
            if (((HWND)wParam == info->hwndNextViewer))
                info->hwndNextViewer = (HWND)lParam;
            else if (info->hwndNextViewer)
                SendMessage(info->hwndNextViewer, msg, wParam, lParam);
        }
        break;
        case WM_DESTROY:
        {
            if (info->hwndNextViewer)
                ChangeClipboardChain(hwnd, info->hwndNextViewer);
            delete info;
        }
        break;
        }
    }
    break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
#endif
BOOL addClipboardFormatListener(HWND _hWnd)
{
    WNDCLASS wc = {};
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc = WNDPROC_8DDD0332_D337_4F76_AF3C_D0CF23E94191;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)wc.lpfnWndProc, &wc.hInstance);
    wc.lpszClassName = KLASS_4420FB75_2931_459E_BA36_B2484F6DC9EE;
    RegisterClass(&wc);
    auto hwnd = CreateWindowEx(0, KLASS_4420FB75_2931_459E_BA36_B2484F6DC9EE, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!hwnd)
        return FALSE;
    auto info = new INFO_EFB07CE8_C478_475C_9B6D_1862D6400474{};
    info->Listener = _hWnd;
    info->hwndNextViewer = SetClipboardViewer(hwnd); // WM_DRAWCLIPBOARD会立即通知一次，但AddClipboardFormatListener不会
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)info);
    SetProp(_hWnd, PROP_5B7BE8DE_E87B_4FC7_8562_48E4E42880DB, hwnd);
    return TRUE;
}
BOOL removeClipboardFormatListener(HWND _hWnd)
{
    auto hwnd = (HWND)GetProp(_hWnd, PROP_5B7BE8DE_E87B_4FC7_8562_48E4E42880DB);
    if (!hwnd)
        return TRUE;
    DestroyWindow(hwnd);
    RemoveProp(_hWnd, PROP_5B7BE8DE_E87B_4FC7_8562_48E4E42880DB);
    return TRUE;
}
#endif

static auto LUNA_UPDATE_PREPARED_OK = RegisterWindowMessage(L"LUNA_UPDATE_PREPARED_OK");
static auto WM_MAGPIE_SCALINGCHANGED = RegisterWindowMessage(L"MagpieScalingChanged");
static auto WM_SYS_HOTKEY = RegisterWindowMessage(L"SYS_HOTKEY_REG_UNREG");
bool IsColorSchemeChangeMessage(LPARAM lParam)
{
    return lParam && CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL;
}
typedef void (*callbackT)(int, bool, const wchar_t *);
static int unique_id = 1;
typedef void (*hotkeycallback_t)();
static std::map<int, hotkeycallback_t> keybinds;
struct hotkeymessageLP
{
    UINT fsModifiers;
    UINT vk;
    hotkeycallback_t callback;
};
static LRESULT CALLBACK WNDPROC_1(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto callback = (callbackT)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    if (callback)
    {
        if (WM_SETTINGCHANGE == message)
        {
            if (IsColorSchemeChangeMessage(lParam))
                callback(0, false, NULL);
        }
        else if (message == WM_MAGPIE_SCALINGCHANGED)
        {
            callback(1, (bool)wParam, NULL);
        }
        else if (message == LUNA_UPDATE_PREPARED_OK)
        {
            callback(2, false, NULL);
        }
        else if (WM_CLIPBOARDUPDATE == message)
        {
            auto data = clipboard_get_internal();
            if (data)
                callback(3, iscurrentowndclipboard(), data.value().c_str());
        }
        else if (WM_HOTKEY == message)
        {
            auto _unique_id = (int)(wParam);
            auto _ = keybinds.find(_unique_id);
            if (_ == keybinds.end())
                return 0;
            _->second();
        }
        else if (WM_SYS_HOTKEY == message)
        {
            if ((UINT)wParam == 1)
            {
                auto info = (hotkeymessageLP *)(lParam);
                unique_id += 1;
                auto succ = RegisterHotKey(hWnd, unique_id, info->fsModifiers, info->vk);
                if (succ)
                {
                    keybinds[unique_id] = info->callback;
                    return unique_id;
                }
                return 0;
            }
            else
            {
                UnregisterHotKey(hWnd, (int)lParam);
                auto _ = keybinds.find((int)lParam);
                if (_ != keybinds.end())
                    keybinds.erase(_);
            }
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
static HWND globalmessagehwnd;
DECLARE_API void startclipboardlisten()
{
    addClipboardFormatListener(globalmessagehwnd);
}
DECLARE_API void stopclipboardlisten()
{
    removeClipboardFormatListener(globalmessagehwnd);
}
DECLARE_API void globalmessagelistener(callbackT callback)
{
    const wchar_t CLASS_NAME[] = L"globalmessagelistener";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WNDPROC_1;
    wc.lpszClassName = CLASS_NAME;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)wc.lpfnWndProc, &wc.hInstance);
    RegisterClass(&wc);
    HWND hWnd = CreateWindowEx(0, CLASS_NAME, NULL, 0, 0, 0, 0, 0, 0, nullptr, wc.hInstance, nullptr); // HWND_MESSAGE会收不到。
    globalmessagehwnd = hWnd;
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)callback);
    ChangeWindowMessageFilterEx(hWnd, LUNA_UPDATE_PREPARED_OK, MSGFLT_ALLOW, nullptr);
    ChangeWindowMessageFilterEx(hWnd, WM_MAGPIE_SCALINGCHANGED, MSGFLT_ALLOW, nullptr);
}
DECLARE_API void dispatchcloseevent()
{
    PostMessage(HWND_BROADCAST, LUNA_UPDATE_PREPARED_OK, 0, 0);
}
DECLARE_API bool clipboard_set_image(HWND hwnd, void *ptr, size_t size)
{
    size -= sizeof(BITMAPFILEHEADER);
    HGLOBAL hDib = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hDib)
        return false;
    void *pDib = GlobalLock(hDib);
    if (!pDib)
    {
        GlobalFree(hDib);
        return false;
    }
    memcpy((char *)pDib, (char *)ptr + sizeof(BITMAPFILEHEADER), size);
    if (tryopenclipboard(hwnd) == false)
        return false;
    EmptyClipboard();
    if (!SetClipboardData(CF_DIB, hDib))
    {
        GlobalFree(hDib);
        CloseClipboard();
        return false;
    }
    CloseClipboard();
    return true;
}

DECLARE_API int registhotkey(UINT fsModifiers, UINT vk, hotkeycallback_t callback)
{
    auto info = new hotkeymessageLP{fsModifiers, vk, callback};
    auto ret = SendMessage(globalmessagehwnd, WM_SYS_HOTKEY, 1, (LPARAM)info);
    delete info;
    return ret;
}
DECLARE_API void unregisthotkey(int _id)
{
    SendMessage(globalmessagehwnd, WM_SYS_HOTKEY, 0, (LPARAM)_id);
}