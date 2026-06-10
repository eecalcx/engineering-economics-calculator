#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <winreg.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDC_NUMBER 1001
#define IDC_PERCENT 1002
#define IDC_OPERATION 1003
#define IDC_CALCULATE 1004
#define IDC_RESULT 1005
#define IDC_SETTINGS 1006
#define IDC_VALUE3 1007
#define IDC_VALUE4 1008
#define IDC_LABEL1 1009
#define IDC_LABEL2 1010
#define IDC_OPERATION_LABEL 1011
#define IDC_LABEL3 1012
#define IDC_LABEL4 1013
#define IDI_APPICON 101

typedef BOOL (WINAPI *PFN_SHOULDAPPSUSEDARKMODE)(void);

static HINSTANCE g_hInst = NULL;
static HFONT g_hFont = NULL;
static HFONT g_hTitleFont = NULL;
static HBRUSH g_bgBrush = NULL;
static HBRUSH g_panelBrush = NULL;
static HBRUSH g_accentBrush = NULL;
static HICON g_hWindowIcon = NULL;
static PFN_SHOULDAPPSUSEDARKMODE g_pShouldAppsUseDarkMode = NULL;

static LRESULT CALLBACK ControlTabSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WNDPROC oldProc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if (msg == WM_KEYDOWN && wParam == VK_TAB) {
        HWND parent = GetParent(hwnd);
        if (parent) {
            BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            HWND next = GetNextDlgTabItem(parent, hwnd, shiftPressed);
            if (next) {
                SetFocus(next);
                return 0;
            }
        }
    }

    if (oldProc) {
        return CallWindowProcA(oldProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void AttachTabNavigation(HWND hwnd) {
    WNDPROC oldProc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
    if (!oldProc) {
        return;
    }

    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldProc);
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)ControlTabSubclassProc);
}

static BOOL g_darkMode = TRUE;
static int g_themeMode = 0;

static void SaveThemeModePreference(int mode) {
    char exePath[MAX_PATH];
    char configPath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return;
    }

    char *slash = strrchr(exePath, '\\');
    if (!slash) {
        return;
    }
    *slash = '\0';

    snprintf(configPath, sizeof(configPath), "%s\\theme_mode.txt", exePath);
    FILE *file = fopen(configPath, "w");
    if (!file) {
        return;
    }

    fprintf(file, "%d\n", mode);
    fclose(file);
}

static int LoadThemeModePreference(void) {
    char exePath[MAX_PATH];
    char configPath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return 0;
    }

    char *slash = strrchr(exePath, '\\');
    if (!slash) {
        return 0;
    }
    *slash = '\0';

    snprintf(configPath, sizeof(configPath), "%s\\theme_mode.txt", exePath);
    FILE *file = fopen(configPath, "r");
    if (!file) {
        return 0;
    }

    int mode = 0;
    if (fscanf(file, "%d", &mode) != 1 || mode < 0 || mode > 2) {
        mode = 0;
    }
    fclose(file);

    return mode;
}

static COLORREF GetWindowBgColor(void) {
    return g_darkMode ? RGB(31, 31, 31) : RGB(248, 248, 248);
}

static COLORREF GetPanelColor(void) {
    return g_darkMode ? RGB(45, 45, 48) : RGB(255, 255, 255);
}

static COLORREF GetBorderColor(void) {
    return g_darkMode ? RGB(68, 68, 71) : RGB(217, 217, 217);
}

static COLORREF GetThemeTextColor(void) {
    return g_darkMode ? RGB(240, 240, 240) : RGB(24, 24, 24);
}

static COLORREF GetAccentColor(void) {
    return g_darkMode ? RGB(0, 120, 212) : RGB(0, 99, 177);
}

static void DrawDarkButton(HDC hdc, const RECT *rect, BOOL primary, const char *text) {
    COLORREF fill = primary ? GetAccentColor() : GetPanelColor();
    COLORREF border = primary ? GetAccentColor() : GetBorderColor();
    COLORREF textColor = primary ? RGB(255, 255, 255) : GetThemeTextColor();

    HBRUSH fillBrush = CreateSolidBrush(fill);
    HPEN borderPen = CreatePen(PS_SOLID, 1, border);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, fillBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    Rectangle(hdc, rect->left, rect->top, rect->right, rect->bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(fillBrush);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);

    RECT textRect = *rect;
    InflateRect(&textRect, -4, -4);
    DrawTextA(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void DrawComboBoxItem(HDC hdc, const RECT *rect, BOOL selected, const char *text) {
    COLORREF fill = selected ? GetAccentColor() : GetPanelColor();
    COLORREF textColor = selected ? RGB(255, 255, 255) : GetThemeTextColor();
    COLORREF border = selected ? GetAccentColor() : GetBorderColor();

    HBRUSH fillBrush = CreateSolidBrush(fill);
    HPEN borderPen = CreatePen(PS_SOLID, 1, border);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, fillBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    Rectangle(hdc, rect->left, rect->top, rect->right, rect->bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(fillBrush);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);

    RECT textRect = *rect;
    InflateRect(&textRect, -8, -2);
    DrawTextA(hdc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

static void ReleaseBrushes(void) {
    if (g_bgBrush) {
        DeleteObject(g_bgBrush);
        g_bgBrush = NULL;
    }
    if (g_panelBrush) {
        DeleteObject(g_panelBrush);
        g_panelBrush = NULL;
    }
    if (g_accentBrush) {
        DeleteObject(g_accentBrush);
        g_accentBrush = NULL;
    }
}

static void RecreateBrushes(void) {
    ReleaseBrushes();
    g_bgBrush = CreateSolidBrush(GetWindowBgColor());
    g_panelBrush = CreateSolidBrush(GetPanelColor());
    g_accentBrush = CreateSolidBrush(GetAccentColor());
}

static BOOL ReadDoubleFromControl(HWND hwnd, int controlId, double *value) {
    char text[64] = {0};
    GetDlgItemTextA(hwnd, controlId, text, sizeof(text));
    char *end = NULL;
    double parsed = strtod(text, &end);
    if (text[0] == '\0' || end == text || *end != '\0') {
        return FALSE;
    }
    *value = parsed;
    return TRUE;
}

static void UpdateInputLabels(HWND hwnd) {
    HWND hLabel1 = GetDlgItem(hwnd, IDC_LABEL1);
    HWND hLabel2 = GetDlgItem(hwnd, IDC_LABEL2);
    HWND hLabel3 = GetDlgItem(hwnd, IDC_LABEL3);
    HWND hLabel4 = GetDlgItem(hwnd, IDC_LABEL4);
    HWND hValue3 = GetDlgItem(hwnd, IDC_VALUE3);
    HWND hValue4 = GetDlgItem(hwnd, IDC_VALUE4);

    int operation = (int)SendMessageA(GetDlgItem(hwnd, IDC_OPERATION), CB_GETCURSEL, 0, 0);
    const char *label1 = "Original number";
    const char *label2 = "Percentage";
    const char *label3 = "";
    const char *label4 = "";

    switch (operation) {
        case 0:
        case 1:
            label1 = "Original number";
            label2 = "Percentage";
            break;
        case 2:
            label1 = "Present worth (P)";
            label2 = "Interest rate (i)";
            label3 = "Periods (n)";
            break;
        case 3:
            label1 = "Future worth (F)";
            label2 = "Interest rate (i)";
            label3 = "Periods (n)";
            break;
        case 4:
            label1 = "Annual series (A)";
            label2 = "Interest rate (i)";
            label3 = "Periods (n)";
            break;
        case 5:
            label1 = "Annual series (A)";
            label2 = "Interest rate (i)";
            label3 = "Periods (n)";
            break;
        case 6:
            label1 = "Present worth (P)";
            label2 = "Interest rate (i)";
            label3 = "Periods (n)";
            break;
        case 7:
            label1 = "Future worth (F)";
            label2 = "Interest rate (i)";
            label3 = "Periods (n)";
            break;
        case 8:
            label1 = "Gradient (G)";
            label2 = "Interest rate (i)";
            label3 = "Periods (n)";
            break;
        case 9:
            label1 = "Nominal rate (r)";
            label2 = "Compounding periods (m)";
            break;
        case 10:
            label1 = "Present worth (P)";
            label2 = "Rate (r)";
            label3 = "Periods (n)";
            break;
        case 11:
            label1 = "Interest rate (i)";
            label2 = "Inflation rate (f)";
            break;
        case 12:
            label1 = "Benefits (B)";
            label2 = "Costs (C)";
            break;
    }

    SetWindowTextA(hLabel1, label1);
    SetWindowTextA(hLabel2, label2);
    SetWindowTextA(hLabel3, label3);
    SetWindowTextA(hLabel4, label4);

    BOOL showValue3 = (operation >= 2 && operation <= 10 && operation != 9);
    BOOL showValue4 = FALSE;
    ShowWindow(hLabel3, showValue3 ? SW_SHOW : SW_HIDE);
    ShowWindow(hValue3, showValue3 ? SW_SHOW : SW_HIDE);
    ShowWindow(hLabel4, showValue4 ? SW_SHOW : SW_HIDE);
    ShowWindow(hValue4, showValue4 ? SW_SHOW : SW_HIDE);
}

static void calculate_and_show(HWND hwnd) {
    char resultText[240] = {0};
    double value1 = 0.0;
    double value2 = 0.0;
    double value3 = 0.0;
    double value4 = 0.0;
    BOOL hasValue1 = ReadDoubleFromControl(hwnd, IDC_NUMBER, &value1);
    BOOL hasValue2 = ReadDoubleFromControl(hwnd, IDC_PERCENT, &value2);
    BOOL hasValue3 = ReadDoubleFromControl(hwnd, IDC_VALUE3, &value3);
    ReadDoubleFromControl(hwnd, IDC_VALUE4, &value4);

    int operation = (int)SendMessageA(GetDlgItem(hwnd, IDC_OPERATION), CB_GETCURSEL, 0, 0);

    if (operation == 0) {
        if (!hasValue1 || !hasValue2) {
            MessageBoxA(hwnd, "Please enter a valid original number and percentage.", "Input Error", MB_ICONWARNING | MB_OK);
            return;
        }

        double result = value1 + (value1 * value2 / 100.0);
        snprintf(resultText, sizeof(resultText), "Result after %.2f%% increase: %.2f", value2, result);
    }
    else if (operation == 1) {
        if (!hasValue1 || !hasValue2) {
            MessageBoxA(hwnd, "Please enter a valid original number and percentage.", "Input Error", MB_ICONWARNING | MB_OK);
            return;
        }

        double result = value1 - (value1 * value2 / 100.0);
        snprintf(resultText, sizeof(resultText), "Result after %.2f%% decrease: %.2f", value2, result);
    }
    else {
        if (!hasValue1 || !hasValue2) {
            MessageBoxA(hwnd, "Please enter the first two required values.", "Input Error", MB_ICONWARNING | MB_OK);
            return;
        }

        switch (operation) {
            case 2:
                if (!hasValue3) {
                    MessageBoxA(hwnd, "Please enter the number of periods (n).", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Interest rate cannot be zero for this formula.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double result = value1 * pow(1.0 + value2, value3);
                    snprintf(resultText, sizeof(resultText), "F = P(1+i)^n = %.6f", result);
                }
                break;
            case 3:
                if (!hasValue3) {
                    MessageBoxA(hwnd, "Please enter the number of periods (n).", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Interest rate cannot be zero for this formula.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double result = value1 / pow(1.0 + value2, value3);
                    snprintf(resultText, sizeof(resultText), "P = F/(1+i)^n = %.6f", result);
                }
                break;
            case 4:
                if (!hasValue3) {
                    MessageBoxA(hwnd, "Please enter the number of periods (n).", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Interest rate cannot be zero for this formula.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double result = value1 * ((pow(1.0 + value2, value3) - 1.0) / value2);
                    snprintf(resultText, sizeof(resultText), "F = A(F/A,i,n) = %.6f", result);
                }
                break;
            case 5:
                if (!hasValue3) {
                    MessageBoxA(hwnd, "Please enter the number of periods (n).", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Interest rate cannot be zero for this formula.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double denom = value2 * pow(1.0 + value2, value3);
                    double result = value1 * ((pow(1.0 + value2, value3) - 1.0) / denom);
                    snprintf(resultText, sizeof(resultText), "P = A(P/A,i,n) = %.6f", result);
                }
                break;
            case 6:
                if (!hasValue3) {
                    MessageBoxA(hwnd, "Please enter the number of periods (n).", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Interest rate cannot be zero for this formula.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double result = value1 * ((value2 * pow(1.0 + value2, value3)) / (pow(1.0 + value2, value3) - 1.0));
                    snprintf(resultText, sizeof(resultText), "A = P(A/P,i,n) = %.6f", result);
                }
                break;
            case 7:
                if (!hasValue3) {
                    MessageBoxA(hwnd, "Please enter the number of periods (n).", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Interest rate cannot be zero for this formula.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double result = value1 * (value2 / (pow(1.0 + value2, value3) - 1.0));
                    snprintf(resultText, sizeof(resultText), "A = F(A/F,i,n) = %.6f", result);
                }
                break;
            case 8:
                if (!hasValue3) {
                    MessageBoxA(hwnd, "Please enter the number of periods (n).", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Interest rate cannot be zero for this formula.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double result = value1 * ((pow(1.0 + value2, value3) - value2 * value3 - 1.0) / (value2 * value2 * pow(1.0 + value2, value3)));
                    snprintf(resultText, sizeof(resultText), "P = G(P/G,i,n) = %.6f", result);
                }
                break;
            case 9:
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Compounding periods cannot be zero.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                if (value2 < 1.0) {
                    MessageBoxA(hwnd, "Compounding periods should be at least 1.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double result = pow(1.0 + value1 / value2, value2) - 1.0;
                    snprintf(resultText, sizeof(resultText), "i_eff = (1 + r/m)^m - 1 = %.6f", result);
                }
                break;
            case 10:
                if (!hasValue3) {
                    MessageBoxA(hwnd, "Please enter the number of periods (n).", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double result = value1 * exp(value2 * value3);
                    snprintf(resultText, sizeof(resultText), "F = P e^(rn) = %.6f", result);
                }
                break;
            case 11:
                {
                    double result = value1 + value2 + (value1 * value2);
                    snprintf(resultText, sizeof(resultText), "d = i + f + i*f = %.6f", result);
                }
                break;
            case 12:
                if (fabs(value2) < 1e-12) {
                    MessageBoxA(hwnd, "Costs cannot be zero.", "Input Error", MB_ICONWARNING | MB_OK);
                    return;
                }
                {
                    double ratio = value1 / value2;
                    snprintf(resultText, sizeof(resultText), "B/C ratio = %.6f (%s)", ratio, ratio >= 1.0 ? "acceptable" : "not acceptable");
                }
                break;
            default:
                snprintf(resultText, sizeof(resultText), "Select a supported formula.");
                break;
        }
    }

    SetWindowTextA(GetDlgItem(hwnd, IDC_RESULT), resultText);
}

static BOOL CALLBACK RefreshChildWindows(HWND hwndChild, LPARAM lParam) {
    (void)lParam;
    RedrawWindow(hwndChild, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    return TRUE;
}

static BOOL IsWindowsDarkModeEnabled(void) {
    if (!g_pShouldAppsUseDarkMode) {
        HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
        if (hUxtheme) {
            g_pShouldAppsUseDarkMode = (PFN_SHOULDAPPSUSEDARKMODE)GetProcAddress(hUxtheme, "ShouldAppsUseDarkMode");
        }
    }

    if (g_pShouldAppsUseDarkMode) {
        return g_pShouldAppsUseDarkMode() != FALSE;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    LONG reg = RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_DWORD,
        NULL,
        &value,
        &size);
    if (reg == ERROR_SUCCESS) {
        return value == 0;
    }

    return FALSE;
}

static void UpdateSettingsButtonText(HWND hwnd) {
    HWND hSettings = GetDlgItem(hwnd, IDC_SETTINGS);
    if (!hSettings) {
        return;
    }

    const char *text = (g_themeMode == 0) ? "Mode: Auto" : (g_themeMode == 1 ? "Mode: Dark" : "Mode: Light");
    SetWindowTextA(hSettings, text);
}

static void ApplyThemeToWindow(HWND hwnd) {
    RecreateBrushes();
    SetClassLongPtrA(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)g_bgBrush);

    BOOL dark = g_darkMode;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    COLORREF titleColor = g_darkMode ? RGB(31, 31, 31) : RGB(250, 250, 250);
    COLORREF titleTextColor = g_darkMode ? RGB(243, 243, 243) : RGB(24, 24, 24);
    COLORREF borderColor = g_darkMode ? RGB(55, 55, 55) : RGB(220, 220, 220);
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &titleColor, sizeof(titleColor));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &titleTextColor, sizeof(titleTextColor));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));

    DWM_WINDOW_CORNER_PREFERENCE cornerPreference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));

    HWND hCalculate = GetDlgItem(hwnd, IDC_CALCULATE);
    HWND hSettings = GetDlgItem(hwnd, IDC_SETTINGS);
    if (hCalculate) {
        SetWindowTheme(hCalculate, g_darkMode ? L"DarkMode_Explorer" : L"Explorer", NULL);
    }
    if (hSettings) {
        SetWindowTheme(hSettings, g_darkMode ? L"DarkMode_Explorer" : L"Explorer", NULL);
    }

    EnumChildWindows(hwnd, RefreshChildWindows, 0);
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

static void SetThemeMode(HWND hwnd, int mode) {
    g_themeMode = mode;
    if (mode == 0) {
        g_darkMode = IsWindowsDarkModeEnabled();
    }
    else {
        g_darkMode = (mode == 1);
    }
    ApplyThemeToWindow(hwnd);
    UpdateSettingsButtonText(hwnd);
    SaveThemeModePreference(mode);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            if (!g_hFont) {
                g_hFont = CreateFontA(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            }
            if (!g_hTitleFont) {
                g_hTitleFont = CreateFontA(-22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            }

            HWND hTitle = CreateWindowExA(0, "STATIC", "Engineering Economics Calculator", WS_CHILD | WS_VISIBLE | SS_LEFT,
                24, 20, 320, 28, hwnd, NULL, g_hInst, NULL);
            HWND hSubtitle = CreateWindowExA(0, "STATIC", "Adjust values and calculate the result instantly.", WS_CHILD | WS_VISIBLE | SS_LEFT,
                24, 50, 420, 24, hwnd, NULL, g_hInst, NULL);
            SendMessageA(hTitle, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);
            SendMessageA(hSubtitle, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hLabel1 = CreateWindowExA(0, "STATIC", "Original number", WS_CHILD | WS_VISIBLE | SS_LEFT,
                24, 95, 180, 24, hwnd, (HMENU)IDC_LABEL1, g_hInst, NULL);
            HWND hNumber = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                220, 90, 280, 32, hwnd, (HMENU)IDC_NUMBER, g_hInst, NULL);
            SendMessageA(hLabel1, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageA(hNumber, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SetWindowTheme(hNumber, L"Explorer", NULL);
            AttachTabNavigation(hNumber);

            HWND hLabel2 = CreateWindowExA(0, "STATIC", "Percentage", WS_CHILD | WS_VISIBLE | SS_LEFT,
                24, 145, 180, 24, hwnd, (HMENU)IDC_LABEL2, g_hInst, NULL);
            HWND hPercent = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                220, 140, 280, 32, hwnd, (HMENU)IDC_PERCENT, g_hInst, NULL);
            SendMessageA(hLabel2, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageA(hPercent, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SetWindowTheme(hPercent, L"Explorer", NULL);
            AttachTabNavigation(hPercent);

            HWND hOperationLabel = CreateWindowExA(0, "STATIC", "Calculation", WS_CHILD | WS_VISIBLE | SS_LEFT,
                24, 195, 180, 24, hwnd, (HMENU)IDC_OPERATION_LABEL, g_hInst, NULL);
            HWND hOperation = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED,
                220, 190, 280, 120, hwnd, (HMENU)IDC_OPERATION, g_hInst, NULL);
            SendMessageA(hOperationLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageA(hOperation, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SetWindowTheme(hOperation, g_darkMode ? L"DarkMode_Explorer" : L"Explorer", NULL);
            AttachTabNavigation(hOperation);
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Percentage Increase");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Percentage Decrease");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Future Worth from Present");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Present Worth from Future");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Future Worth of Uniform Series");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Present Worth of Uniform Series");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Annual Worth from Present");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Annual Worth from Future");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Present Worth of Gradient");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Effective Annual Rate");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Continuous Compounding");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Inflation-Adjusted Rate");
            SendMessageA(hOperation, CB_ADDSTRING, 0, (LPARAM)"Benefit-Cost Ratio");
            SendMessageA(hOperation, CB_SETCURSEL, 0, 0);

            HWND hLabel3 = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
                24, 245, 180, 24, hwnd, (HMENU)IDC_LABEL3, g_hInst, NULL);
            HWND hValue3 = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                220, 240, 280, 32, hwnd, (HMENU)IDC_VALUE3, g_hInst, NULL);
            SendMessageA(hLabel3, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageA(hValue3, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SetWindowTheme(hValue3, L"Explorer", NULL);
            AttachTabNavigation(hValue3);

            HWND hLabel4 = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
                24, 295, 180, 24, hwnd, (HMENU)IDC_LABEL4, g_hInst, NULL);
            HWND hValue4 = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                220, 290, 280, 32, hwnd, (HMENU)IDC_VALUE4, g_hInst, NULL);
            SendMessageA(hLabel4, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageA(hValue4, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SetWindowTheme(hValue4, L"Explorer", NULL);
            AttachTabNavigation(hValue4);

            HWND hButton = CreateWindowExA(0, "BUTTON", "Calculate", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_NOTIFY | BS_OWNERDRAW,
                24, 350, 160, 42, hwnd, (HMENU)IDC_CALCULATE, g_hInst, NULL);
            HWND hSettings = CreateWindowExA(0, "BUTTON", "Mode: Auto", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_NOTIFY | BS_OWNERDRAW,
                200, 350, 160, 42, hwnd, (HMENU)IDC_SETTINGS, g_hInst, NULL);
            SetWindowTheme(hButton, g_darkMode ? L"DarkMode_Explorer" : L"Explorer", NULL);
            SetWindowTheme(hSettings, g_darkMode ? L"DarkMode_Explorer" : L"Explorer", NULL);
            SendMessageA(hButton, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageA(hSettings, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            AttachTabNavigation(hButton);
            AttachTabNavigation(hSettings);

            HWND hResult = CreateWindowExA(0, "STATIC", "Result will appear here.", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_LEFTNOWORDWRAP,
                24, 412, 480, 70, hwnd, (HMENU)IDC_RESULT, g_hInst, NULL);
            SendMessageA(hResult, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SetWindowTheme(hResult, L"Explorer", NULL);

            UpdateInputLabels(hwnd);
            SetThemeMode(hwnd, LoadThemeModePreference());
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CALCULATE && HIWORD(wParam) == BN_CLICKED) {
                calculate_and_show(hwnd);
            }
            else if (LOWORD(wParam) == IDC_SETTINGS && HIWORD(wParam) == BN_CLICKED) {
                int nextMode = (g_themeMode + 1) % 3;
                SetThemeMode(hwnd, nextMode);
            }
            else if (LOWORD(wParam) == IDC_OPERATION && HIWORD(wParam) == CBN_SELCHANGE) {
                UpdateInputLabels(hwnd);
            }
            return 0;

        case WM_MEASUREITEM: {
            LPMEASUREITEMSTRUCT measureItem = (LPMEASUREITEMSTRUCT)lParam;
            if (measureItem->CtlType == ODT_LISTBOX || measureItem->CtlType == ODT_COMBOBOX) {
                measureItem->itemHeight = 28;
                return TRUE;
            }
            break;
        }

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;
            if (drawItem->CtlType == ODT_BUTTON) {
                char text[64] = {0};
                SendMessageA(drawItem->hwndItem, WM_GETTEXT, sizeof(text), (LPARAM)text);
                DrawDarkButton((HDC)drawItem->hDC, &drawItem->rcItem,
                    (GetDlgCtrlID(drawItem->hwndItem) == IDC_CALCULATE), text);
                return TRUE;
            }

            if (drawItem->CtlType == ODT_COMBOBOX) {
                char text[128] = {0};
                SendMessageA(drawItem->hwndItem, CB_GETLBTEXT, drawItem->itemID, (LPARAM)text);
                if (text[0] == '\0') {
                    strcpy(text, "Select a calculation");
                }
                DrawComboBoxItem((HDC)drawItem->hDC, &drawItem->rcItem,
                    (drawItem->itemState & ODS_SELECTED) != 0, text);
                return TRUE;
            }

            if (drawItem->CtlType == ODT_LISTBOX) {
                char text[128] = {0};
                SendMessageA(drawItem->hwndItem, LB_GETTEXT, drawItem->itemID, (LPARAM)text);
                DrawComboBoxItem((HDC)drawItem->hDC, &drawItem->rcItem,
                    (drawItem->itemState & ODS_SELECTED) != 0, text);
                return TRUE;
            }
            break;
        }

        case WM_KEYDOWN:
            if (wParam == VK_TAB) {
                HWND hFocus = GetFocus();
                if (hFocus && GetParent(hFocus) == hwnd) {
                    BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    HWND hNext = GetNextDlgTabItem(hwnd, hFocus, shiftPressed);
                    if (hNext) {
                        SetFocus(hNext);
                        return 0;
                    }
                }
            }
            break;

        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            HWND hButton = (HWND)lParam;
            int controlId = GetDlgCtrlID(hButton);
            BOOL primary = (controlId == IDC_CALCULATE);
            SetBkMode(hdc, OPAQUE);
            SetTextColor(hdc, primary ? RGB(255, 255, 255) : (g_darkMode ? RGB(242, 242, 242) : RGB(24, 24, 24)));
            SetBkColor(hdc, primary ? GetAccentColor() : GetPanelColor());
            return (LRESULT)(primary ? g_accentBrush : g_panelBrush);
        }

        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, OPAQUE);
            SetTextColor(hdc, GetThemeTextColor());
            SetBkColor(hdc, GetPanelColor());
            return (LRESULT)g_panelBrush;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, OPAQUE);
            SetTextColor(hdc, GetThemeTextColor());
            SetBkColor(hdc, GetPanelColor());
            return (LRESULT)g_panelBrush;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, OPAQUE);
            SetTextColor(hdc, GetThemeTextColor());
            SetBkColor(hdc, GetWindowBgColor());
            return (LRESULT)g_bgBrush;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, g_bgBrush);
            return 1;
        }

        case WM_THEMECHANGED:
        case WM_SETTINGCHANGE:
            if (g_themeMode == 0) {
                SetThemeMode(hwnd, 0);
            }
            return 0;

        case WM_DESTROY: {
            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL;
            }
            if (g_hTitleFont) {
                DeleteObject(g_hTitleFont);
                g_hTitleFont = NULL;
            }
            if (g_hWindowIcon) {
                DestroyIcon(g_hWindowIcon);
                g_hWindowIcon = NULL;
            }
            ReleaseBrushes();
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    SetProcessDPIAware();

    INITCOMMONCONTROLSEX icc = {0};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    g_hInst = hInstance;

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PercentageCalculatorWindow";

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Window registration failed.", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    HWND hwnd = CreateWindowExA(WS_EX_CONTROLPARENT, "PercentageCalculatorWindow", "Engineering Economics Calculator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 560, 560,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxA(NULL, "Window creation failed.", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    g_hWindowIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_APPICON));
    if (g_hWindowIcon) {
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hWindowIcon);
        SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hWindowIcon);
        SetClassLongPtrA(hwnd, GCLP_HICON, (LONG_PTR)g_hWindowIcon);
        SetClassLongPtrA(hwnd, GCLP_HICONSM, (LONG_PTR)g_hWindowIcon);
    }

    ApplyThemeToWindow(hwnd);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return (int)msg.wParam;
}
