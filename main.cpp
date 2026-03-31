#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <vector>
#include <cstdio>
#include "resource.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
#define WM_TRAYICON  (WM_APP + 1)
#define IDM_EXIT     0xFFFF

// Menu ID encoding: pack monitor index + rotation into one UINT
#define MENU_ID(mon, rot)    ((UINT)((mon) * 4 + (rot) + 1))
#define MONITOR_FROM_ID(id)  (((id) - 1) / 4)
#define ROTATION_FROM_ID(id) (((id) - 1) % 4)

static const WCHAR* APP_NAME     = L"Screen Rotator";
static const WCHAR* WINDOW_CLASS = L"ScreenRotatorClass";

static const WCHAR* ROTATION_LABELS[] = {
    L"0\x00B0 (Landscape)",
    L"90\x00B0 (Portrait)",
    L"180\x00B0 (Landscape flipped)",
    L"270\x00B0 (Portrait flipped)"
};

// ---------------------------------------------------------------------------
// Monitor info
// ---------------------------------------------------------------------------
struct MonitorInfo {
    WCHAR deviceName[32];     // e.g. \\.\DISPLAY1 — API identifier
    WCHAR friendlyName[256];  // e.g. "DELL U2722D (Display 1)" — menu label
    DWORD currentRotation;    // DMDO_DEFAULT / DMDO_90 / DMDO_180 / DMDO_270
    bool  isPrimary;
};

static std::vector<MonitorInfo> g_monitors;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static HINSTANCE      g_hInstance;
static HWND           g_hWnd;
static NOTIFYICONDATA g_nid;

// ---------------------------------------------------------------------------
// EnumerateMonitors — refresh the global monitor list
// ---------------------------------------------------------------------------
static void EnumerateMonitors()
{
    g_monitors.clear();

    DISPLAY_DEVICE adapter = {};
    adapter.cb = sizeof(adapter);

    for (DWORD i = 0; EnumDisplayDevices(NULL, i, &adapter, 0); i++) {
        if (!(adapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        // Get current display settings for rotation
        DEVMODE dm = {};
        dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsEx(adapter.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
            continue;

        // Get friendly monitor name (second-level enumeration)
        DISPLAY_DEVICE monitor = {};
        monitor.cb = sizeof(monitor);
        const WCHAR* friendly = adapter.DeviceName; // fallback
        if (EnumDisplayDevices(adapter.DeviceName, 0, &monitor, 0)) {
            friendly = monitor.DeviceString;
        }

        MonitorInfo info = {};
        wcscpy_s(info.deviceName, adapter.DeviceName);
        swprintf_s(info.friendlyName, L"%s (%s)", friendly, adapter.DeviceName);
        info.currentRotation = dm.dmDisplayOrientation;
        info.isPrimary = (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;

        g_monitors.push_back(info);
        adapter.cb = sizeof(adapter); // reset for next iteration
    }
}

// ---------------------------------------------------------------------------
// IsPortrait — true for 90° or 270°
// ---------------------------------------------------------------------------
static bool IsPortrait(DWORD orientation)
{
    return orientation == DMDO_90 || orientation == DMDO_270;
}

// ---------------------------------------------------------------------------
// SupportsRotation — check if a given orientation is supported by the driver
// ---------------------------------------------------------------------------
static bool SupportsRotation(LPCWSTR deviceName, DWORD currentOrientation, DWORD targetOrientation)
{
    // Current orientation is always supported
    if (targetOrientation == currentOrientation)
        return true;

    // Use CDS_TEST to ask the driver if it would accept this rotation
    DEVMODE dm = {};
    dm.dmSize = sizeof(dm);
    if (!EnumDisplaySettingsEx(deviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
        return false;

    // Swap width/height if crossing landscape/portrait boundary
    if (IsPortrait(currentOrientation) != IsPortrait(targetOrientation)) {
        DWORD temp = dm.dmPelsWidth;
        dm.dmPelsWidth = dm.dmPelsHeight;
        dm.dmPelsHeight = temp;
    }

    dm.dmDisplayOrientation = targetOrientation;
    dm.dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT;

    return ChangeDisplaySettingsEx(deviceName, &dm, NULL, CDS_TEST, NULL) == DISP_CHANGE_SUCCESSFUL;
}

// ---------------------------------------------------------------------------
// SetRotation — change rotation for a specific display
// ---------------------------------------------------------------------------
static void SetRotation(LPCWSTR deviceName, DWORD newOrientation)
{
    DEVMODE dm = {};
    dm.dmSize = sizeof(dm);
    if (!EnumDisplaySettingsEx(deviceName, ENUM_CURRENT_SETTINGS, &dm, 0)) {
        MessageBox(NULL, L"Failed to read current display settings.",
                   APP_NAME, MB_OK | MB_ICONERROR);
        return;
    }

    DWORD oldOrientation = dm.dmDisplayOrientation;

    // Swap width/height only when crossing the landscape/portrait boundary
    if (IsPortrait(oldOrientation) != IsPortrait(newOrientation)) {
        DWORD temp = dm.dmPelsWidth;
        dm.dmPelsWidth = dm.dmPelsHeight;
        dm.dmPelsHeight = temp;
    }

    dm.dmDisplayOrientation = newOrientation;
    dm.dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT;

    LONG result = ChangeDisplaySettingsEx(deviceName, &dm, NULL, CDS_UPDATEREGISTRY, NULL);
    if (result != DISP_CHANGE_SUCCESSFUL) {
        WCHAR msg[256];
        swprintf_s(msg, L"Failed to change rotation (error %ld).\n"
                        L"Your display driver may not support this orientation.", result);
        MessageBox(NULL, msg, APP_NAME, MB_OK | MB_ICONERROR);
    }
}

// ---------------------------------------------------------------------------
// BuildContextMenu — create the right-click popup menu
// ---------------------------------------------------------------------------
static HMENU BuildContextMenu()
{
    EnumerateMonitors();

    HMENU hMenu = CreatePopupMenu();

    for (int m = 0; m < (int)g_monitors.size(); m++) {
        HMENU hSub = CreatePopupMenu();

        for (DWORD rot = 0; rot < 4; rot++) {
            UINT flags = MF_STRING;

            // Checkmark on current rotation
            if (rot == g_monitors[m].currentRotation)
                flags |= MF_CHECKED;

            // Gray out unsupported rotations
            if (!SupportsRotation(g_monitors[m].deviceName, g_monitors[m].currentRotation, rot))
                flags |= MF_GRAYED;

            AppendMenu(hSub, flags, MENU_ID(m, rot), ROTATION_LABELS[rot]);
        }

        // Mark primary monitor in label
        WCHAR label[300];
        if (g_monitors[m].isPrimary)
            swprintf_s(label, L"%s  [Primary]", g_monitors[m].friendlyName);
        else
            wcscpy_s(label, g_monitors[m].friendlyName);

        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSub, label);
    }

    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");

    return hMenu;
}

// ---------------------------------------------------------------------------
// Tray icon management
// ---------------------------------------------------------------------------
static void AddTrayIcon(HWND hWnd)
{
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = hWnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon            = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APPICON));
    if (!g_nid.hIcon)
        g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // fallback
    wcscpy_s(g_nid.szTip, APP_NAME);
    Shell_NotifyIcon(NIM_ADD, &g_nid);

    g_nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &g_nid);
}

static void RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        AddTrayIcon(hWnd);
        return 0;

    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_CONTEXTMENU || LOWORD(lParam) == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = BuildContextMenu();
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            PostMessage(hWnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
        }
        return 0;

    case WM_COMMAND: {
        UINT id = LOWORD(wParam);
        if (id == IDM_EXIT) {
            DestroyWindow(hWnd);
        } else if (id >= 1 && id < IDM_EXIT) {
            int monIdx = MONITOR_FROM_ID(id);
            DWORD rot  = ROTATION_FROM_ID(id);
            if (monIdx >= 0 && monIdx < (int)g_monitors.size()) {
                SetRotation(g_monitors[monIdx].deviceName, rot);
            }
        }
        return 0;
    }

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    g_hInstance = hInstance;

    // Prevent multiple instances
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"ScreenRotatorMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, L"Screen Rotator is already running.", APP_NAME, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    WNDCLASS wc    = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance   = hInstance;
    wc.lpszClassName = WINDOW_CLASS;
    RegisterClass(&wc);

    g_hWnd = CreateWindowEx(0, WINDOW_CLASS, APP_NAME, 0,
                            0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!g_hWnd)
        return 1;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return (int)msg.wParam;
}
