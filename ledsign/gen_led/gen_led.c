#include <ctype.h>
#include <windows.h>

#include "gen.h"
#include "libsigntcp.h"

#define TRKNMAX 100

BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved);
BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

static void config(void);
static void quit(void);
static int init(void);
static char szAppName[] = "LED Sign Plugin for Winamp";
static HHOOK WAHook = NULL;

static winampGeneralPurposePlugin plugin =
{
    GPPHDR_VER,
    szAppName,
    init,
    config,
    quit,
};

static void config()
{

}

static void quit()
{
  sc_close();
}

static char trkname[TRKNMAX + 1];

static char *findwa(char *s) {
  char *cand;
  register char *p;
  const char *waname = "- winamp";
  register const char *p2;

  cand = s;
  p = s;
  p2 = waname;

  while (1) {
    if (*p == 0) break;

    if (tolower(*p) != *p2) {
      p2 = waname;
    } else {
      p2++;
      if (*p2 == 0) {
        cand = p - 7;
        p2 = waname;
      }
    }
    p++;
  }

  p = cand;
  if (p == s) return p;
  while (p > s && *(--p) == ' ') { }

  return p + 1;
}

static void updtrkname(char *s) {
  int n;

  char *p;

  p = findwa(s);
  n = p - s;
  if (n > TRKNMAX) n = TRKNMAX;

  if (strncmp(&(trkname[0]), s, n)) {
    memcpy(&(trkname[0]), s, n);
    trkname[n] = 0;
    sc_sprint(trkname, SFLAG_QUEUE | SFLAG_TRANS, 500, 0);
  }
}

/* Winamp window hook for obtaining updates */
static LRESULT WINAPI HookWinampWnd(int nCode, WPARAM wParam, LPARAM lParam) {
    /* The useful parameters are:
     * ((CWPRETSTRUCT *)lParam)->hwnd, ((CWPRETSTRUCT *)lParam)->message,
     * ((CWPRETSTRUCT *)lParam)->wParam, ((CWPRETSTRUCT *)lParam)->lParam)
     */

    /* Process only if HC_ACTION according to MS docs. */
    if (nCode == HC_ACTION && ((CWPRETSTRUCT *)lParam)->hwnd == plugin.hwndParent) {
        switch(((CWPRETSTRUCT *)lParam)->message) {
        case WM_SETTEXT:
            updtrkname((char *)(((CWPRETSTRUCT *)lParam)->lParam));
            break;

        default:
            break;
      }
    }

    return CallNextHookEx(WAHook, nCode,
        wParam, lParam);
}

static int init() {
  trkname[0] = 0;

  sc_open();

  WAHook = SetWindowsHookEx(WH_CALLWNDPROCRET, HookWinampWnd, NULL,
                            GetWindowThreadProcessId(plugin.hwndParent, NULL));

  return 0;
}

__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin(void);
__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin(void)
{
    return &plugin;
}
