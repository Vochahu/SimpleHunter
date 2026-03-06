#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <string>
#include <vector>
#include <locale.h>
#include <shellapi.h>
#include <commctrl.h> // ComboBox

#define ID_EDIT_INPUT 101
#define ID_BTN_SEARCH 102
#define ID_LIST_RESULTS 103
#define ID_COMBO_FILTER 104
#define ID_MENU_DELETE 201
#define ID_MENU_OPEN 202

HWND hEdit, hBtn, hList, hCombo;

// Check
bool CheckSize(unsigned long long fileSize, int filterIndex) {
    const unsigned long long MB = 1024 * 1024;
    switch (filterIndex) {
        case 1: return fileSize < (1 * MB);           // Small: < 1MB
        case 2: return fileSize >= (1 * MB) && fileSize <= (100 * MB); // Medium: 1-100MB
        case 3: return fileSize > (100 * MB);         // Large: > 100MB
        default: return true;                         // None: All sizes
    }
}

void SearchFiles(std::wstring directory, std::wstring target, HWND listbox, int filterIndex) {
    std::wstring query = directory + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(query.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::wstring fileName = findData.cFileName;
        if (fileName == L"." || fileName == L"..") continue;

        std::wstring fullPath = directory + L"\\" + fileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            SearchFiles(fullPath, target, listbox, filterIndex);
        } else {
            std::wstring lowF = fileName; for(auto &c : lowF) c = towlower(c);
            std::wstring lowT = target;   for(auto &c : lowT) c = towlower(c);
            
            if (lowF.find(lowT) != std::wstring::npos) {
                // File
                unsigned long long fileSize = ((unsigned long long)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
                
                if (CheckSize(fileSize, filterIndex)) {
                    SendMessageW(listbox, LB_ADDSTRING, 0, (LPARAM)fullPath.c_str());
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

void ShowContextMenu(HWND hwnd, LPARAM lParam) {
    POINT pt = { LOWORD(lParam), HIWORD(lParam) };
    POINT ptClient = pt;
    ScreenToClient(hList, &ptClient);

    LRESULT itemIndex = SendMessage(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(ptClient.x, ptClient.y));
    if (HIWORD(itemIndex) == 0) {
        SendMessage(hList, LB_SETCURSEL, LOWORD(itemIndex), 0);
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, ID_MENU_OPEN, L"Locate in Explorer");
        AppendMenuW(hMenu, MF_STRING, ID_MENU_DELETE, L"Delete Permanent");
        TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            CreateWindowExW(0, L"STATIC", L"Search string:", WS_VISIBLE | WS_CHILD, 10, 10, 150, 20, hwnd, NULL, NULL, NULL);
            hEdit = CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 30, 200, 25, hwnd, (HMENU)ID_EDIT_INPUT, NULL, NULL);
            
            CreateWindowExW(0, L"STATIC", L"Size filter:", WS_VISIBLE | WS_CHILD, 220, 10, 100, 20, hwnd, NULL, NULL, NULL);
            hCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 220, 30, 100, 200, hwnd, (HMENU)ID_COMBO_FILTER, NULL, NULL);
            const wchar_t* filters[] = { L"None", L"Small", L"Medium", L"Large" };
            for(int i=0; i<4; i++) SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)filters[i]);
            SendMessage(hCombo, CB_SETCURSEL, 0, 0);

            hBtn = CreateWindowExW(0, L"BUTTON", L"Search Now", WS_VISIBLE | WS_CHILD, 330, 30, 140, 25, hwnd, (HMENU)ID_BTN_SEARCH, NULL, NULL);
            hList = CreateWindowExW(0, L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 10, 70, 460, 280, hwnd, (HMENU)ID_LIST_RESULTS, NULL, NULL);

            SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hList, WM_SETFONT, (WPARAM)hFont, TRUE);
            break;
        }
        case WM_CONTEXTMENU: if ((HWND)wParam == hList) ShowContextMenu(hwnd, lParam); break;
        case WM_COMMAND: {
            int sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
            wchar_t path[MAX_PATH] = {0};
            if (sel != LB_ERR) SendMessageW(hList, LB_GETTEXT, sel, (LPARAM)path);

            if (LOWORD(wParam) == ID_BTN_SEARCH) {
                wchar_t buf[256]; GetWindowTextW(hEdit, buf, 256);
                if (wcslen(buf) < 2) { MessageBoxW(hwnd, L"Keyword too short!", L"Info", MB_OK); break; }
                int filter = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                SendMessageW(hList, LB_RESETCONTENT, 0, 0);
                SearchFiles(L"C:\\Users", buf, hList, filter);
                MessageBoxW(hwnd, L"Search complete!", L"Done", MB_OK);
            }
            else if (LOWORD(wParam) == ID_MENU_OPEN) {
                std::wstring cmd = L"/select,\"" + std::wstring(path) + L"\"";
                ShellExecuteW(NULL, L"open", L"explorer.exe", cmd.c_str(), NULL, SW_SHOWNORMAL);
            }
            else if (LOWORD(wParam) == ID_MENU_DELETE) {
                if (MessageBoxW(hwnd, L"Delete this file permanently?", L"Confirm", MB_YESNO | MB_ICONWARNING) == IDYES) {
                    if (DeleteFileW(path)) SendMessage(hList, LB_DELETESTRING, sel, 0);
                }
            }
            break;
        }
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int wmain() {
    WNDCLASSW wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"FileHunter";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(0, L"FileHunter", L"Universal File Hunter v1.1", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400, NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, SW_SHOW);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}