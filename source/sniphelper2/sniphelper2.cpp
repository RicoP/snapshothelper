// sniphelper2.cpp : Defines the entry point for the application.
//

#include "sniphelper2.h"

#include <shlobj.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#include <algorithm>
#include <string_view>
#include <utility>

#include "framework.h"

#define CUTE_PNG_IMPLEMENTATION
#include "cute_png.h"

#define MAX_LOADSTRING 100

HINSTANCE g_hInst = NULL;

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const WMAPP_HIDEFLYOUT = WM_APP + 2;

// Global Variables:
HINSTANCE hInst;                      // current instance
WCHAR szTitle[MAX_LOADSTRING];        // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  g_hInst = hInstance;

  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // TODO: Place code here.

  // Initialize global strings
  LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadStringW(hInstance, IDC_SNIPHELPER2, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  // Perform application initialization:
  if (!InitInstance(hInstance, nCmdShow)) {
    return FALSE;
  }

  HACCEL hAccelTable =
      LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SNIPHELPER2));

  MSG msg;

  // Main message loop:
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
  WNDCLASSEXW wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SNIPHELPER2));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SNIPHELPER2);
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

HWND windowHandler;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
  hInst = hInstance;  // Store instance handle in our global variable

  windowHandler =
      CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                    0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

  if (!windowHandler) {
    return FALSE;
  }

  ShowWindow(windowHandler, nCmdShow);
  UpdateWindow(windowHandler);

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

HWND nextClipboardViewer;
#define MSG_UPDATECLIPBOARD (WM_APP + 1)
static bool g_fIgnoreClipboardChange = true;

void error(const char* msg) {
  MessageBox(NULL, L"Please read the ReadMe.txt file for troubleshooting",
             L"Error adding icon", MB_OK);
  exit(-1);
}

void errhandler(const char* msg, HWND hwnd) { error(msg); }

// https://docs.microsoft.com/en-us/windows/win32/gdi/storing-an-image?redirectedfrom=MSDN
PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp) {
  BITMAP bmp;
  PBITMAPINFO pbmi;
  WORD cClrBits;

  // Retrieve the bitmap color format, width, and height.
  if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) error("GetObject");

  // Convert the color format to a count of bits.
  cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
  if (cClrBits == 1)
    cClrBits = 1;
  else if (cClrBits <= 4)
    cClrBits = 4;
  else if (cClrBits <= 8)
    cClrBits = 8;
  else if (cClrBits <= 16)
    cClrBits = 16;
  else if (cClrBits <= 24)
    cClrBits = 24;
  else
    cClrBits = 32;

  // Allocate memory for the BITMAPINFO structure. (This structure
  // contains a BITMAPINFOHEADER structure and an array of RGBQUAD
  // data structures.)

  if (cClrBits < 24)
    pbmi = (PBITMAPINFO)LocalAlloc(
        LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits));

  // There is no RGBQUAD array for these formats: 24-bit-per-pixel or
  // 32-bit-per-pixel

  else
    pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));

  // Initialize the fields in the BITMAPINFO structure.

  pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pbmi->bmiHeader.biWidth = bmp.bmWidth;
  pbmi->bmiHeader.biHeight = bmp.bmHeight;
  pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
  pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
  if (cClrBits < 24) pbmi->bmiHeader.biClrUsed = (1 << cClrBits);

  // If the bitmap is not compressed, set the BI_RGB flag.
  pbmi->bmiHeader.biCompression = BI_RGB;

  // Compute the number of bytes in the array of color
  // indices and store the result in biSizeImage.
  // The width must be DWORD aligned unless the bitmap is RLE
  // compressed.
  pbmi->bmiHeader.biSizeImage =
      ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8 *
      pbmi->bmiHeader.biHeight;
  // Set biClrImportant to 0, indicating that all of the
  // device colors are important.
  pbmi->bmiHeader.biClrImportant = 0;
  return pbmi;
}

void CreateBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP,
                   HDC hDC) {
  HANDLE hf;               // file handle
  BITMAPFILEHEADER hdr;    // bitmap file-header
  PBITMAPINFOHEADER pbih;  // bitmap info-header
  LPBYTE lpBits;           // memory pointer
  DWORD dwTotal;           // total count of bytes
  DWORD cb;                // incremental count of bytes
  BYTE* hp;                // byte pointer
  DWORD dwTmp;

  pbih = (PBITMAPINFOHEADER)pbi;
  lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

  if (!lpBits) errhandler("GlobalAlloc", hwnd);

  // Retrieve the color table (RGBQUAD array) and the bits
  // (array of palette indices) from the DIB.
  if (!GetDIBits(hDC, hBMP, 0, (WORD)pbih->biHeight, lpBits, pbi,
                 DIB_RGB_COLORS)) {
    errhandler("GetDIBits", hwnd);
  }

  // Create the .BMP file.
  hf = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE, (DWORD)0, NULL,
                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
  if (hf == INVALID_HANDLE_VALUE) errhandler("CreateFile", hwnd);
  hdr.bfType = 0x4d42;  // 0x42 = "B" 0x4d = "M"
  // Compute the size of the entire file.
  hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbih->biSize +
                       pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
  hdr.bfReserved1 = 0;
  hdr.bfReserved2 = 0;

  // Compute the offset to the array of color indices.
  hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + pbih->biSize +
                  pbih->biClrUsed * sizeof(RGBQUAD);

  // Copy the BITMAPFILEHEADER into the .BMP file.
  if (!WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), (LPDWORD)&dwTmp,
                 NULL)) {
    errhandler("WriteFile", hwnd);
  }

  // Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
  if (!WriteFile(hf, (LPVOID)pbih,
                 sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD),
                 (LPDWORD)&dwTmp, (NULL)))
    errhandler("WriteFile", hwnd);

  // Copy the array of color indices into the .BMP file.
  dwTotal = cb = pbih->biSizeImage;
  hp = lpBits;
  if (!WriteFile(hf, (LPSTR)hp, (int)cb, (LPDWORD)&dwTmp, NULL))
    errhandler("WriteFile", hwnd);

  // Close the .BMP file.
  if (!CloseHandle(hf)) errhandler("CloseHandle", hwnd);

  // Free memory.
  GlobalFree((HGLOBAL)lpBits);
}

void CreatePNGFile(HWND hwnd, char* path, PBITMAPINFO pbi, HBITMAP hBMP,
                   HDC hDC) {
  PBITMAPINFOHEADER pbih;  // bitmap info-header
  LPBYTE lpBits;           // memory pointer

  pbih = (PBITMAPINFOHEADER)pbi;
  lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

  if (!lpBits) errhandler("GlobalAlloc", hwnd);

  // Retrieve the color table (RGBQUAD array) and the bits
  // (array of palette indices) from the DIB.
  if (!GetDIBits(hDC, hBMP, 0, (WORD)pbih->biHeight, lpBits, pbi,
                 DIB_RGB_COLORS)) {
    errhandler("GetDIBits", hwnd);
  }

  cp_image_t img;
  img.w = pbi->bmiHeader.biWidth;
  img.h = pbi->bmiHeader.biHeight;

  cp_pixel_t* pix_upside_down = (cp_pixel_t*)lpBits;
  cp_pixel_t* pix = (cp_pixel_t*)malloc(sizeof(cp_pixel_t) * img.w * img.h);

  for (int y = 0; y < img.h; y++) {
    cp_pixel_t* row_source = &pix_upside_down[y * img.w];
    cp_pixel_t* row = &pix[(img.h - y - 1) * img.w];

    for (int x = 0; x < img.w; x++) {
      row[x] = row_source[x];
      std::swap(row[x].r, row[x].b);
    }
  }

  img.pix = pix;

  cp_save_png(path, &img);

  // Free memory.
  free(pix);
  GlobalFree((HGLOBAL)lpBits);
}

// Use a guid to uniquely identify our icon
class __declspec(uuid("8D0B8B92-4E1C-488e-A1E1-2331AFCE2CB5")) PrinterIcon;

BOOL AddNotificationIcon(HWND hwnd) {
  NOTIFYICONDATA nid = {sizeof(nid)};
  nid.hWnd = hwnd;
  // add the icon, setting the icon, tooltip, and callback message.
  // the icon will be identified with the GUID
  nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
  nid.guidItem = __uuidof(PrinterIcon);
  nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
  auto result = LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON),
                               LIM_SMALL, &nid.hIcon);
  LoadString(g_hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
  Shell_NotifyIcon(NIM_ADD, &nid);

  // NOTIFYICON_VERSION_4 is prefered
  nid.uVersion = NOTIFYICON_VERSION_4;
  return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL DeleteNotificationIcon() {
  NOTIFYICONDATA nid = {sizeof(nid)};
  nid.uFlags = NIF_GUID;
  nid.guidItem = __uuidof(PrinterIcon);
  return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd, POINT pt) {
  HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_CONTEXTMENU));
  if (hMenu) {
    HMENU hSubMenu = GetSubMenu(hMenu, 0);
    if (hSubMenu) {
      // our window must be foreground before calling TrackPopupMenu or the menu
      // will not disappear when the user clicks away
      SetForegroundWindow(hwnd);

      // respect menu drop alignment
      UINT uFlags = TPM_RIGHTBUTTON;
      if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
        uFlags |= TPM_RIGHTALIGN;
      } else {
        uFlags |= TPM_LEFTALIGN;
      }

      TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
    }
    DestroyMenu(hMenu);
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  time_t t;
  tm tm;

  switch (msg) {
    case WM_CREATE:
      nextClipboardViewer = SetClipboardViewer(hwnd);
      // MessageBeep(MB_ICONINFORMATION);
      // add the notification icon
      if (!AddNotificationIcon(hwnd)) {
        MessageBox(hwnd, L"Please read the ReadMe.txt file for troubleshooting",
                   L"Error adding icon", MB_OK);
        return -1;
      }

      {
        wchar_t desktop_path[MAX_PATH + 1] = {};
        if (!SHGetSpecialFolderPathW(HWND_DESKTOP, desktop_path, CSIDL_DESKTOP,
                                     FALSE))
          error("can't get desktop path");

        // Working directory is desktop
        SetCurrentDirectory(desktop_path);
      }
      break;
    case WM_CHANGECBCHAIN:
      if ((HWND)wParam == nextClipboardViewer)
        nextClipboardViewer == (HWND)lParam;
      else if (nextClipboardViewer != NULL)
        SendMessage(nextClipboardViewer, msg, wParam, lParam);
      break;

    case WM_DRAWCLIPBOARD:
      if (g_fIgnoreClipboardChange) {
        g_fIgnoreClipboardChange = false;
        break;
      }

      t = time(NULL);
      localtime_s(&tm, &t);

      // https://github.com/dspinellis/outwit/blob/master/winclip/winclip.c
      // DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);

      if (IsClipboardFormatAvailable(CF_BITMAP)) {
        if (OpenClipboard(hwnd)) {
          HBITMAP cb_hnd = (HBITMAP)GetClipboardData(CF_BITMAP);
          if (cb_hnd != nullptr) {
            // BITMAP bmp;
            HDC hdc;
            int x, y;
            COLORREF cref;
            HGDIOBJ oldobj;

            if ((hdc = CreateCompatibleDC(NULL)) == NULL) {
              error("Unable to create a compatible device context");
            }

            auto info = CreateBitmapInfoStruct(hwnd, cb_hnd);

            {
              /*
              wchar_t path[MAX_PATH + 1];
              swprintf(path, MAX_PATH,
                       L"snapshot - %d-%02d-%02d %02d-%02d-%02d.bmp",
                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
                       tm.tm_min, tm.tm_sec);

              CreateBMPFile(hwnd, LPTSTR(path), info, cb_hnd, hdc);
              */

              char path[MAX_PATH];
              sprintf_s(path, MAX_PATH,
                        "snapshot - %d-%02d-%02d %02d-%02d-%02d.png",
                        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);

              CreatePNGFile(hwnd, path, info, cb_hnd, hdc);
            }

            // SelectObject(hdc, oldobj);
            DeleteDC(hdc);
          }

          CloseClipboard();
        }
      }
      break;

    case WMAPP_NOTIFYCALLBACK:
      switch (LOWORD(lParam)) {
        case NIN_SELECT:
          break;

        case NIN_BALLOONTIMEOUT:
          break;

        case NIN_BALLOONUSERCLICK:
          break;

        case WM_CONTEXTMENU: {
          POINT const pt = {LOWORD(wParam), HIWORD(wParam)};
          ShowContextMenu(hwnd, pt);
        } break;
      }
      break;

    case WM_COMMAND: {
      int wmId = LOWORD(wParam);
      // Parse the menu selections:
      switch (wmId) {
        case IDM_ABOUT:
          DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
          break;
        case IDM_EXIT:
          DestroyWindow(hwnd);
          break;
        default:
          return DefWindowProc(hwnd, msg, wParam, lParam);
      }
    } break;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      // TODO: Add any drawing code that uses hdc here...
      EndPaint(hwnd, &ps);
    } break;

    case WM_DESTROY:
      ChangeClipboardChain(windowHandler, nextClipboardViewer);
      DeleteNotificationIcon();
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);
  switch (message) {
    case WM_INITDIALOG:
      return (INT_PTR)TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
      }
      break;
  }
  return (INT_PTR)FALSE;
}

#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")
