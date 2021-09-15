// sniphelper2.cpp : Defines the entry point for the application.
//

#include "sniphelper2.h"

#include <shlobj.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "framework.h"

#define MAX_LOADSTRING 100

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

void error(const char* msg) {}

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

/*
LPWSTR desktop_directory_unicode() {
  static wchar_t path[MAX_PATH + 1] = {};
  if (SHGetSpecialFolderPathW(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE))
    return path;
  return path;
}
*/

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  static wchar_t desktop_path[MAX_PATH + 1] = {};
  static wchar_t path[MAX_PATH + 1] = {};

  time_t t = time(NULL);
  tm tm;

  switch (msg) {
    case WM_CREATE:
      nextClipboardViewer = SetClipboardViewer(hwnd);
      MessageBeep(MB_ICONINFORMATION);
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

      if (!SHGetSpecialFolderPathW(HWND_DESKTOP, desktop_path, CSIDL_DESKTOP,
                                   FALSE))
        error("can't get desktop path");

      localtime_s(&tm, &t);

      swprintf(path, MAX_PATH,
               L"%s\\snapshot - %d-%02d-%02d %02d-%02d-%02d.bmp", desktop_path,
               tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
               tm.tm_min, tm.tm_sec);

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

            CreateBMPFile(hwnd, LPTSTR(path), info, cb_hnd, hdc);

            // SelectObject(hdc, oldobj);
            DeleteDC(hdc);
          }

          CloseClipboard();
        }
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
