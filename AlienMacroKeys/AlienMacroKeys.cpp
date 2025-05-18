// AlienMacroKeys.cpp : Defines the entry point for the application.
//

#include <regex>
#include <thread>
#include "framework.h"
#include "AlienMacroKeys.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hMainDialog = nullptr;
NOTIFYICONDATA nid = { 0 };
HMENU hTrayMenu = nullptr;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    MainDlgProc(HWND, UINT, WPARAM, LPARAM);
void                ResizeMainWindowToDialog(HWND, HWND);
void                AddTrayIcon(HWND);
void                RemoveTrayIcon();


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALIENMACROKEYS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	// Start the monitor thread
    std::string vid = AW_KB_VID;
    std::string pid = AW_KB_PID;

    std::regex re("(?:0x)[0-9a-fA-F]{4}");

	bool vid_valid = std::regex_match(vid, re);
	bool pid_valid = std::regex_match(pid, re);
    if (!vid_valid || !pid_valid)
    {
        std::cerr << "VID/PID is invalid. Please make sure it is in the format 0xXXXX where XXXX is the hexadecimal VID/PID." << std::endl;
    }

    WORD targetVID = (WORD)std::stoi(vid, nullptr, 16);
    WORD targetPID = (WORD)std::stoi(pid, nullptr, 16);

    std::thread([=] {
        StartMonitor(targetVID, targetPID);
        }).detach();

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALIENMACROKEYS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENMACROKEYS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ALIENMACROKEYS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, //WS_OVERLAPPEDWINDOW,
       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // No WS_THICKFRAME, no maximize
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    // existing cases
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(hWnd, SW_HIDE); // Hide window and taskbar icon
            return 0; // Prevent default minimize
        }
        // For all other system commands, call the default handler
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
			ShowWindow(hWnd, SW_HIDE); // Hide window and taskbar icon
            return 0;
        }
        break;
    case WM_CREATE:
        // Create the modeless dialog as a child of the main window
        hMainDialog = CreateDialogParam(
            hInst,
            MAKEINTATOM(IDD_MAINWINDOW),
            hWnd,       // parent is main window
            MainDlgProc,
            0
        );
        if (hMainDialog)
        {
            // Position and show the dialog as needed
			SetWindowPos(hMainDialog, nullptr, 10, 10, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
        }
        ResizeMainWindowToDialog(hWnd, hMainDialog);
        
        AddTrayIcon(hWnd);

        break;
    case WM_USER + 1: // Tray icon callback
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            if (!hTrayMenu) {
                hTrayMenu = CreatePopupMenu();
                AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
            }
            SetForegroundWindow(hWnd); // Required for menu to disappear correctly
            TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        else if (lParam == WM_LBUTTONUP) {
			// Restore the window if it was hidden
            ShowWindow(hWnd, SW_SHOW);
			SetForegroundWindow(hWnd); // Bring the window to the front
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (wmId == ID_TRAY_EXIT) {
                DestroyWindow(hWnd);
                break;
			}
            // Forward commands to the child dialog if needed
            if (hMainDialog && IsDialogMessage(hMainDialog, &(*(MSG*)&message)))
            {
                break;
			}
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        RemoveTrayIcon();
        if (hTrayMenu) {
            DestroyMenu(hTrayMenu);
            hTrayMenu = nullptr;
        }
        if (hMainDialog)
        {
            DestroyWindow(hMainDialog); // Destroy the dialog when the main window is destroyed
			hMainDialog = nullptr; // Reset the dialog handle
        }
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        // Handle dialog controls here
        break;
    }
    return (INT_PTR)FALSE;
}

// Helper function to resize the main window's client area to match the dialog resource size
void ResizeMainWindowToDialog(HWND hWnd, HWND hDialog)
{
    if (!hWnd || !hDialog) return;

    // Get the dialog's size in screen coordinates
    RECT dlgRect;
    GetWindowRect(hDialog, &dlgRect);
    int dlgWidthPx = dlgRect.right - dlgRect.left;
    int dlgHeightPx = dlgRect.bottom - dlgRect.top;

    // Adjust for window borders so the client area matches the dialog size
    RECT reClient, reWindow;
    GetClientRect(hWnd, &reClient);
    GetWindowRect(hWnd, &reWindow);

    int borderWidth = (reWindow.right - reWindow.left) - (reClient.right - reClient.left);
    int borderHeight = (reWindow.bottom - reWindow.top) - (reClient.bottom - reClient.top);

    SetWindowPos(hWnd, nullptr, 0, 0,
        dlgWidthPx + borderWidth,
        dlgHeightPx + borderHeight,
        SWP_NOMOVE | SWP_NOZORDER);

    // Position the dialog at (0,0) in the client area
    SetWindowPos(hMainDialog, nullptr, 0, 0,
        dlgWidthPx, dlgHeightPx,
        SWP_NOZORDER | SWP_SHOWWINDOW);
}

void AddTrayIcon(HWND hWnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ALIENMACROKEYS));
    wcscpy_s(nid.szTip, L"AlienMacroKeys");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}