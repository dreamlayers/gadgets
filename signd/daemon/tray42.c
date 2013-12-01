/* Tray42 - System tray example
   Copyright 1997 Michael T. Smith / R.A.M. Technology.
   You are permitted to copy, print and disseminate this code for
   educational purposes. Don't cut and paste sections into your own programs --
   this is so ugly that it hurts, but I didn't feel like taking the time to do
   it right.

   The included RES file contains an icon (16x16x16 and 32x32x16 formats) and
   a menu. I compiled the RES file with the Microsoft Platform SDK rc.exe
   (hopefully still available off http://www.microsoft.com/msdn/ somewhere),
   but the rest of the compiling and linking was done with lcc-win32,
   Jacob Navia's freeware C compiler (based on the lcc compiler system). I
   haven't yet had time to download the latest release of lcc, which probably
   would have fixed the problem I had below (see the MESSED constant).

   If your linker is not MESSED, make sure to comment out that line, and link
   with shell32.lib.

                   - Michael Smith, <aa529@chebucto.ns.ca>, 5 Aug 1997
*/

#include <windows.h>  // main Windows header
#include <windowsx.h> // message cracker macros, etc.
#include <shellapi.h> // Shell32 api
#include "signd.h"
#include "tray42.h"

#define MESSED // lcclnk's implib.exe/shell32.lib won't correctly link to
               // Shell_NotifyIconA
#define UWM_SYSTRAY (WM_USER + 1) // Sent to us by the systray

#ifdef MESSED
#undef Shell_NotifyIcon
BOOL (WINAPI *Shell_NotifyIcon)(DWORD,PNOTIFYICONDATAA);
#endif

#define TOOLTIP "LEDdaemon"

LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HINSTANCE ghInst;

HWND hWnd;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpszCmdLine, int nCmdShow)
{
  WNDCLASSEX wc;
  MSG msg;
  NOTIFYICONDATA nid;
  char *classname = "Noise42.NOTIFYICONDATA.hWnd";

#ifdef MESSED
  HMODULE hshell32;
  if (!(hshell32 = LoadLibrary("SHELL32"))) exit(1);
  if (!(Shell_NotifyIcon = (BOOL (WINAPI *)(DWORD,PNOTIFYICONDATAA))GetProcAddress(hshell32, "Shell_NotifyIconA")))
    exit(1);
#endif

  ghInst = hInstance;
  // Low priority -- don't soak up as many CPU cycles.
  // This won't make a difference unless we are doing background processing
  // in this process.
//  SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

  /* Create a window class for the window that receives systray notifications.
     The window will never be displayed, although there's no reason it couldn't
     be (it just wouldn't be very useful in this program, and in most others).
  */
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = 0;
  wc.lpfnWndProc = wndProc;
  wc.cbClsExtra = wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_42));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = classname;
  wc.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_42), IMAGE_ICON,
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), 0);
  RegisterClassEx(&wc);
  // Create window. Note that WS_VISIBLE is not used, and window is never shown
  hWnd = CreateWindowEx(0, classname, classname, WS_POPUP, CW_USEDEFAULT, 0,
          CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

  // Fill out NOTIFYICONDATA structure
  nid.cbSize = sizeof(NOTIFYICONDATA); // size
  nid.hWnd = hWnd; // window to receive notifications
  nid.uID = 1;     // application-defined ID for icon (can be any UINT value)
  nid.uFlags = NIF_MESSAGE |  // nid.uCallbackMessage is valid, use it
                NIF_ICON |     // nid.hIcon is valid, use it
                NIF_TIP;       // nid.szTip is valid, use it
  nid.uCallbackMessage = UWM_SYSTRAY; // message sent to nid.hWnd
  nid.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_42), IMAGE_ICON,
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON), 0); // 16x16 icon
  // szTip is the ToolTip text (64 byte array including NULL)
  strcpy_s(nid.szTip, sizeof(nid.szTip), TOOLTIP);

  // NIM_ADD: Add icon; NIM_DELETE: Remove icon; NIM_MODIFY: modify icon
  Shell_NotifyIcon(NIM_ADD, &nid); // This adds the icon

  init_socket();

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  cleanup_socket();

#ifdef MESSED
  FreeLibrary(hshell32);
#endif
  return msg.wParam;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  POINT pt;
  HMENU hmenu, hpopup;
  NOTIFYICONDATA nid;

  switch (message) {
    case ACCEPT_EVENT:
      accept_client();
      return TRUE;

    case WM_CREATE:
      SetTimer(hwnd, 0x29a, 10000, NULL); // 2-second timer
      return TRUE;

    case WM_TIMER:
      // Every 2 seconds, we will change something -- in this case, the ToolTip
#if 0
      if (gi >= NUM_TOOLTIPS - 1) gi = 0;
      else gi++;
      strcpy(nid.szTip, tttable[gi]);
      nid.cbSize = sizeof(NOTIFYICONDATA);
      nid.hWnd = hwnd;
      nid.uID = 1;
      nid.uFlags = NIF_TIP; // Only nid.szTip and nid.uID are valid, change tip
      Shell_NotifyIcon(NIM_MODIFY, &nid); // Modify tooltip
#endif
      return TRUE;

    case WM_DESTROY:
      nid.cbSize = sizeof(NOTIFYICONDATA);
      nid.hWnd = hwnd;
      nid.uID = 1;
      nid.uFlags = NIF_TIP; // not really sure what to put here, but it works
      Shell_NotifyIcon(NIM_DELETE, &nid); // This removes the icon
      PostQuitMessage(0);
      KillTimer(hwnd, 0x29a);
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDM_EXIT: DestroyWindow(hwnd); break;
      case IDM_MESSAGEBOX:
        MessageBox(hwnd, "Listening on port 9876", "LED Sign Daemon", MB_TASKMODAL);
        break;
      }
      return TRUE;

    case UWM_SYSTRAY: // We are being notified of mouse activity over the icon
      switch (lParam) {
        case WM_RBUTTONUP: // Let's track a popup menu
          GetCursorPos(&pt);
          hmenu = LoadMenu(ghInst, MAKEINTRESOURCE(IDM_CONTEXTMAIN));
          hpopup = GetSubMenu(hmenu, 0);

          /* SetForegroundWindow and the ensuing null PostMessage is a
             workaround for a Windows 95 bug (see MSKB article Q135788,
             http://www.microsoft.com/kb/articles/q135/7/88.htm, I think).
             In typical Microsoft style this bug is listed as "by design".
             SetForegroundWindow also causes our MessageBox to pop up in front
             of any other application's windows. */
          SetForegroundWindow(hwnd);
          /* We specifiy TPM_RETURNCMD, so TrackPopupMenu returns the menu
             selection instead of returning immediately and our getting a
             WM_COMMAND with the selection. You don't have to do it this way.
          */
          TrackPopupMenu(hpopup,            // Popup menu to track
                         TPM_RIGHTBUTTON,   // Track right mouse button?
                         pt.x, pt.y,        // screen coordinates
                         0,                 // reserved
                         hwnd,              // owner
                         NULL);              // LPRECT user can click in
                                                    // without dismissing menu
          PostMessage(hwnd, 0, 0, 0); // see above
          DestroyMenu(hmenu); // Delete loaded menu and reclaim its resources
          break;

        case WM_LBUTTONDBLCLK:
          SetForegroundWindow(hwnd); // Our MessageBox pops up in front
          MessageBox(hwnd, "You're cool.", "Hey", MB_TASKMODAL);
          break;
        // Other mouse messages: WM_MOUSEMOVE, WM_MBUTTONDOWN, etc.
      }
      return TRUE; // I don't think that it matters what you return.
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}
