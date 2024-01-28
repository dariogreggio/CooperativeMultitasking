/* 
 * File:   windows.h
 * Author: katia
 *
 * Created on 3 novembre 2021, 11.34
 * 
 * per flag e strutture: https://pkg.go.dev/github.com/xpzed/win32#section-readme
 * e https://github.com/tpn/winsdk-7/blob/master/v7.1A/Include/CommCtrl.h
 * https://github.com/tpn/winsdk-10/blob/master/Include/10.0.10240.0/um/WinUser.h
 */

#ifndef _BREAKTHROUGH_H
#define	_BREAKTHROUGH_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../genericTypedefs.h"
#include <stdint.h>
#include "../pc_pic_video.X/adafruit_colors.h"
#include "../pc_pic_video.X/gfxfont.h"
#include "fat_sd/fsio.h"
#include "fat_ram/ramfsio.h"
#include "fat_ide/idefsio.h"
#include "pc_pic_cpu.h"
//#include <setjmp.h>
    
#define BREAKTHROUGH_VERSION_L 13
#define BREAKTHROUGH_VERSION_H 2

#define IS_BREAKTHROUGH() GetDesktopWindow()    // o mInstance->hWnd ?? ma occhio a hInstance :D

typedef char *LPSTR;
typedef const char *LPCSTR;
typedef void *HANDLE;

//extern UGRAPH_COORD_T cursor_x,cursor_y;
extern BYTE textsize;

void _swap(UGRAPH_COORD_T *a, UGRAPH_COORD_T *b);

#define BUILD_BUG_ON(condition) ((int)(sizeof(struct { int:(-!!(condition)); })))  // ((int)sizeof(char[1 - 2*!!(condition)]))		// per align struct
    // porcodio non va, errore di sintassi
#define ASSERT_CONCAT_(a, b) a##b		//https://www.microchip.com/forums/m1126290.aspx
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/* These can't be used after statements in c89. */
#ifdef __COUNTER__
  #define STATIC_ASSERT(e,m) \
    ;enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/(int)(!!(e)) }
#else
  /* This can't be used twice on the same line so ensure if using in headers
   * that the headers are not included twice (by wrapping in #ifndef...#endif)
   * Note it doesn't cause an issue when used on same line of separate modules
   * compiled with gcc -combine -fwhole-program.  */
  #define STATIC_ASSERT(e,m) \
    ;enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(int)(!!(e)) }
#endif

    
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
   ((DWORD)(uint8_t)(ch0) | ((DWORD)(uint8_t)(ch1) << 8) |         \
   ((DWORD)(uint8_t)(ch2) << 16) | ((DWORD)(uint8_t)(ch3) << 24 ))
#define MAKECLASS(c) ((CLASS)c)
    
#define BORDER_SIZE ((hWnd->style & WS_THICKFRAME) ? 2 : 1)
#define SHADOW_SIZE 1
#define TITLE_HEIGHT 10
#define MENU_HEIGHT 9
#define TITLE_ICON_WIDTH 8
#define SCROLL_SIZE 8

typedef uint32_t WPARAM;
typedef uint32_t LPARAM;
typedef uint32_t LRESULT;
typedef uint32_t HINSTANCE;
typedef uint16_t ATOM;
typedef const GFX_COLOR *ICON;
typedef const GFX_COLOR *CURSOR;



enum __attribute__ ((__packed__)) ORIENTATION {		// https://docs.microsoft.com/en-us/uwp/api/windows.graphics.display.displayorientations?view=winrt-22000
	LANDSCAPE=1,//Specifies that the monitor is oriented in landscape mode where the width of the display viewing area is greater than the height.
	LANSCAPE_FLIPPED=4, //Specifies that the monitor rotated another 90 degrees in the clockwise direction (to equal 180 degrees) to orient the display in landscape mode where the width of the display viewing area is greater than the height. This landscape mode is flipped 180 degrees from the Landscape mode.
	NONE=0,		// No display orientation is specified.
	PORTRAIT=2,	//Specifies that the monitor rotated 90 degrees in the clockwise direction to orient the display in portrait mode where the height of the display viewing area is greater than the width.
	PORTRAIT_FLIPPED=8
	};

void handleWinTimers(void);
int8_t manageWindows(BYTE);
BYTE manageTouchScreen(UGRAPH_COORD_T *x,UGRAPH_COORD_T *y,uint8_t *z);
int8_t InitWindows(GFX_COLOR bgcolor,enum ORIENTATION,BYTE extra,BYTE mode,const char *sfondo);
void DrawWindow(UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2,GFX_COLOR f,GFX_COLOR b);
void MovePointer(UGRAPH_COORD_T x,UGRAPH_COORD_T y);


#define MAKEINTATOM(a) ((void *)a)
#define MAKEWPARAM(l,h) MAKELONG(l,h)
#define MAKELPARAM(l,h) MAKELONG(l,h)
#define MAKELRESULT(l,h) MAKELONG(l,h)
#define GET_X_LPARAM(a) LOWORD(a)
#define GET_Y_LPARAM(a) HIWORD(a)
#define MAKEPOINTS(l) (*((POINTS *)&(l))) 

#define HWND_BROADCAST ((HWND)0xffff)

#define WS_POPUP        0x80000000L
#define WS_CHILD        0x40000000L
#define WS_CHILDWINDOW  0x40000000L
#define WS_ICONIC       0x20000000L
#define WS_VISIBLE      0x10000000L
#define WS_DISABLED     0x08000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_MINIMIZE     0x01000000L
#define WS_MINIMIZEBOX  0x01000000L
#define WS_MAXIMIZE     0x00800000L
#define WS_MAXIMIZEBOX  0x00800000L
#define WS_BORDER       0x00400000L
#define WS_DLGFRAME     0x00200000L
#define WS_THICKFRAME   0x00200000L
#define WS_VSCROLL      0x00100000L
#define WS_HSCROLL      0x00080000L
#define WS_CAPTION      0x00040000L
#define WS_SYSMENU      0x00020000L
#define WS_SIZEBOX      0x00010000L
#define WS_GROUP        0x00008000L
#define WS_TABSTOP      0x00004000L
#define WS_OVERLAPPED   0x00000000L
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | /*WS_THICKFRAME | */ WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_TILED        0x00000000L
#define WS_TILEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_EX_TOPMOST   0x00001000L     // sarebbe ex-style ma per ora faccio così!
//da 0x800 in giu ci sono stili controlli...
    
#define HWND_BOTTOM (HWND)1
#define HWND_NOTOPMOST (HWND)-2
#define HWND_TOP (HWND)0
#define HWND_TOPMOST (HWND)-1

#define SWP_ASYNCWINDOWPOS 0x4000
#define SWP_DEFERERASE 0x2000
#define SWP_NOSENDCHANGING 0x0400
#define SWP_NOREPOSITION 0x0200
#define SWP_NOOWNERZORDER 0x0200
#define SWP_NOCOPYBITS 0x0100
#define SWP_HIDEWINDOW 0x0080
#define SWP_DRAWFRAME  0x0020
#define SWP_FRAMECHANGED 0x0020
#define SWP_NOACTIVATE 0x0010
#define SWP_SHOWWINDOW 0x0040
#define SWP_NOREDRAW 0x0008
#define SWP_NOZORDER 0x0004
#define SWP_NOMOVE 0x0002
#define SWP_NOSIZE 0x0001

#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_NORMAL 1
#define SW_SHOWMINIMIZED 2      // n.b. ANCHE se le iconette sono disattivate in STYLE, questi vanno lo stesso! v. Windows
#define SW_SHOWMAXIMIZED 3
#define SW_MAXIMIZE 3
#define SW_SHOWNOACTIVATE 4
#define SW_SHOW 5
#define SW_MINIMIZE 6
#define SW_SHOWMINNOACTIVE 7
#define SW_SHOWNA 8
#define SW_RESTORE 9
#define SW_SHOWDEFAULT 10
#define SW_FORCEMINIMIZE 11
  
#define SIZE_MAXHIDE 4
#define SIZE_MAXIMIZED 2
#define SIZE_MAXSHOW 3
#define SIZE_MINIMIZED 1
#define SIZE_RESTORED 0

#define SW_OTHERUNZOOM 4
#define SW_OTHERZOOM 2
#define SW_PARENTCLOSING 1
#define SW_PARENTOPENING 3

#define SB_HORZ 0
#define SB_VERT 1
#define SB_CTL 2
#define SB_BOTH 3
#define SIF_RANGE 0x0001
#define SIF_PAGE 0x0002
#define SIF_POS 0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS 0x0010

#define SBS_HORZ   0x1000
#define SBS_VERT   0x2000 
    
#define SB_LINEUP           0
#define SB_LINELEFT         0
#define SB_LINEDOWN         1
#define SB_LINERIGHT        1
#define SB_PAGEUP           2
#define SB_PAGELEFT         2
#define SB_PAGEDOWN         3
#define SB_PAGERIGHT        3
#define SB_THUMBPOSITION    4
#define SB_THUMBTRACK       5
#define SB_TOP              6
#define SB_LEFT             6
#define SB_BOTTOM           7
#define SB_RIGHT            7
#define SB_ENDSCROLL        8

#define ESB_ENABLE_BOTH     0x0000
#define ESB_DISABLE_BOTH    0x0003
#define ESB_DISABLE_LEFT 0x0001
#define ESB_DISABLE_RIGHT 0x0002
#define ESB_DISABLE_UP 0x0001
#define ESB_DISABLE_DOWN 0x0002
#define ESB_DISABLE_LTUP ESB_DISABLE_LEFT
#define ESB_DISABLE_RTDN ESB_DISABLE_RIGHT


#define CW_USEDEFAULT 0x80000000        // con GRAPH_COORD = int

// da winuser.h
#define WM_NULL                         0x0000
#define WM_CREATE                       0x0001
#define WM_DESTROY                      0x0002
#define WM_MOVE                         0x0003
#define WM_SIZE                         0x0005
/* * WM_SIZE message wParam values */
#define SIZE_RESTORED       0
#define SIZE_MINIMIZED      1
#define SIZE_MAXIMIZED      2
#define SIZE_MAXSHOW        3
#define SIZE_MAXHIDE        4

#define WM_ACTIVATE                     0x0006
/* * WM_ACTIVATE state values */
#define     WA_INACTIVE     0
#define     WA_ACTIVE       1
#define     WA_CLICKACTIVE  2

#define WM_SETFOCUS                     0x0007
#define WM_KILLFOCUS                    0x0008
#define WM_ENABLE                       0x000A
#define WM_SETREDRAW                    0x000B
#define WM_SETTEXT                      0x000C
#define WM_GETTEXT                      0x000D
#define WM_GETTEXTLENGTH                0x000E
#define WM_PAINT                        0x000F
#define WM_CLOSE                        0x0010
#define WM_QUERYENDSESSION              0x0011
#define WM_QUIT                         0x0012
#define WM_QUERYOPEN                    0x0013
#define WM_ERASEBKGND                   0x0014
#define WM_SYSCOLORCHANGE               0x0015
#define WM_ENDSESSION                   0x0016
#define WM_SHOWWINDOW                   0x0018
#define WM_CTLCOLOR         0x0019       // deprecated, v. altri simili
#define WM_WININICHANGE                 0x001A
#define WM_SETTINGCHANGE                WM_WININICHANGE

#define WM_DEVMODECHANGE                0x001B
#define WM_ACTIVATEAPP                  0x001C
#define WM_FONTCHANGE                   0x001D
#define WM_TIMECHANGE                   0x001E
#define WM_CANCELMODE                   0x001F
#define WM_SETCURSOR                    0x0020
#define WM_MOUSEACTIVATE                0x0021
#define WM_CHILDACTIVATE                0x0022
#define WM_QUEUESYNC                    0x0023

#define WM_GETMINMAXINFO                0x0024

#define WM_PAINTICON                    0x0026
#define WM_ICONERASEBKGND               0x0027
#define WM_NEXTDLGCTL                   0x0028
#define WM_SPOOLERSTATUS                0x002A
#define WM_DRAWITEM                     0x002B
#define WM_MEASUREITEM                  0x002C
#define WM_DELETEITEM                   0x002D
#define WM_VKEYTOITEM                   0x002E
#define WM_CHARTOITEM                   0x002F
#define WM_SETFONT                      0x0030
#define WM_GETFONT                      0x0031
#define WM_SETHOTKEY                    0x0032
#define WM_GETHOTKEY                    0x0033
#define WM_QUERYDRAGICON                0x0037
#define WM_COMPAREITEM                  0x0039
#define WM_GETOBJECT                    0x003D
#define WM_COMPACTING                   0x0041
#define WM_COMMNOTIFY                   0x0044  /* no longer suported */
#define WM_WINDOWPOSCHANGING            0x0046
#define WM_WINDOWPOSCHANGED             0x0047

#define WM_POWER                        0x0048
/* wParam for WM_POWER window message and DRV_POWER driver notification */
#define PWR_OK              1
#define PWR_FAIL            (-1)
#define PWR_SUSPENDREQUEST  1
#define PWR_SUSPENDRESUME   2
#define PWR_CRITICALRESUME  3

#define WM_COPYDATA                     0x004A
#define WM_CANCELJOURNAL                0x004B

/* * lParam of WM_COPYDATA message points to... */
typedef struct __attribute((packed)) {
    DWORD dwData;
    DWORD cbData;
    VOID *lpData;
  } COPYDATASTRUCT;

#define WM_NOTIFY                       0x004E
#define WM_INPUTLANGCHANGEREQUEST       0x0050
#define WM_INPUTLANGCHANGE              0x0051
#define WM_TCARD                        0x0052
#define WM_HELP                         0x0053
#define WM_USERCHANGED                  0x0054
#define WM_NOTIFYFORMAT                 0x0055
#define NFR_ANSI                             1
#define NFR_UNICODE                          2
#define NF_QUERY                             3
#define NF_REQUERY                           4

#define WM_CONTEXTMENU                  0x007B
#define WM_STYLECHANGING                0x007C
#define WM_STYLECHANGED                 0x007D
#define WM_DISPLAYCHANGE                0x007E
#define WM_GETICON                      0x007F
#define WM_SETICON                      0x0080

#define WM_NCCREATE                     0x0081
#define WM_NCDESTROY                    0x0082
#define WM_NCCALCSIZE                   0x0083
#define WM_NCHITTEST                    0x0084
#define WM_NCPAINT                      0x0085
#define WM_NCACTIVATE                   0x0086
#define WM_GETDLGCODE                   0x0087
#define WM_SYNCPAINT                    0x0088
#define WM_NCMOUSEMOVE                  0x00A0
#define WM_NCLBUTTONDOWN                0x00A1
#define WM_NCLBUTTONUP                  0x00A2
#define WM_NCLBUTTONDBLCLK              0x00A3
#define WM_NCRBUTTONDOWN                0x00A4
#define WM_NCRBUTTONUP                  0x00A5
#define WM_NCRBUTTONDBLCLK              0x00A6
#define WM_NCMBUTTONDOWN                0x00A7
#define WM_NCMBUTTONUP                  0x00A8
#define WM_NCMBUTTONDBLCLK              0x00A9
  
#define WM_MOUSEHOVER                   0x02A1
#define WM_MOUSELEAVE                   0x02A3
#define WM_NCMOUSEHOVER                 0x02A0
#define WM_NCMOUSELEAVE                 0x02A2
  
#define WM_KEYFIRST                     0x0100
#define WM_KEYDOWN                      0x0100
#define WM_KEYUP                        0x0101
#define WM_CHAR                         0x0102
#define WM_DEADCHAR                     0x0103
#define WM_SYSKEYDOWN                   0x0104
#define WM_SYSKEYUP                     0x0105
#define WM_SYSCHAR                      0x0106
#define WM_SYSDEADCHAR                  0x0107
#define WM_KEYLAST                      0x0108

#define WM_INITDIALOG                   0x0110
#define WM_COMMAND                      0x0111
#define WM_SYSCOMMAND                   0x0112
#define WM_TIMER                        0x0113
#define WM_HSCROLL                      0x0114
#define WM_VSCROLL                      0x0115
#define WM_INITMENU                     0x0116
#define WM_INITMENUPOPUP                0x0117
#define WM_MENUSELECT                   0x011F
#define WM_MENUCHAR                     0x0120
#define WM_ENTERIDLE                    0x0121
#define WM_MENURBUTTONUP                0x0122
#define WM_MENUDRAG                     0x0123
#define WM_MENUGETOBJECT                0x0124
#define WM_UNINITMENUPOPUP              0x0125
#define WM_MENUCOMMAND                  0x0126
#define WM_APPCOMMAND                   0x0319


#define WM_CTLCOLORMSGBOX               0x0132
#define WM_CTLCOLOREDIT                 0x0133
#define WM_CTLCOLORLISTBOX              0x0134
#define WM_CTLCOLORBTN                  0x0135
#define WM_CTLCOLORDLG                  0x0136
#define WM_CTLCOLORSCROLLBAR            0x0137
#define WM_CTLCOLORSTATIC               0x0138

#define WM_MOUSEFIRST                   0x0200
#define WM_MOUSEMOVE                    0x0200
#define WM_LBUTTONDOWN                  0x0201
#define WM_LBUTTONUP                    0x0202
#define WM_LBUTTONDBLCLK                0x0203
#define WM_RBUTTONDOWN                  0x0204
#define WM_RBUTTONUP                    0x0205
#define WM_RBUTTONDBLCLK                0x0206
#define WM_MBUTTONDOWN                  0x0207
#define WM_MBUTTONUP                    0x0208
#define WM_MBUTTONDBLCLK                0x0209

#define WM_MOUSEWHEEL                   0x020A
#define WM_MOUSELAST                    0x020A
#define WHEEL_DELTA                     120     /* Value for rolling one detent */
#define WHEEL_PAGESCROLL                (UINT_MAX) /* Scroll one page */
  
#define WM_SIZING                       0x0214
#define WM_CAPTURECHANGED               0x0215
#define WM_MOVING                       0x0216
  
#define WM_MOUSELEAVE                   0x02A3
  
#define WM_PARENTNOTIFY                 0x0210
#define WM_ENTERMENULOOP                0x0211
#define WM_EXITMENULOOP                 0x0212
#define WM_NEXTMENU                     0x0213
#define WM_ENTERSIZEMOVE                0x0231
#define WM_EXITSIZEMOVE                 0x0232
  
#define WM_CUT                          0x0300
#define WM_COPY                         0x0301
#define WM_PASTE                        0x0302
#define WM_CLEAR                        0x0303
#define WM_UNDO                         0x0304
#define WM_RENDERFORMAT                 0x0305
#define WM_DESTROYCLIPBOARD             0x0307
#define WM_PAINTCLIPBOARD               0x0309
#define WM_CLIPBOARDUPDATE              0x031D
  
#define WM_PRINT                        0x0317
#define WM_PRINTCLIENT                  0x0318
  
#define WM_GETTITLEBARINFOEX            0x033F
  
#define WM_KICKIDLE 0x036A              // solo MFC
#define WM_SETMESSAGESTRING 0x0362
#define WM_FILECHANGE 0x0390            // scelto io...
#define WM_PRINTCHAR  0x0391            // scelto io...
          
#define WM_USER                         0x0400

#define WM_SURF_NAVIGATE                WM_USER+1
#define WM_VIEWER_OPEN                  WM_USER+1
#define WM_PLAYER_OPEN                  WM_USER+1
#define WM_NOTEPAD_OPEN                 WM_USER+1

  
#define HTBORDER 18 //	In the border of a window that does not have a sizing border.
#define HTBOTTOM 15 //	In the lower-horizontal border of a resizable window (the user can click the mouse to resize the window vertically).
#define HTBOTTOMLEFT 16 //	In the lower-left corner of a border of a resizable window (the user can click the mouse to resize the window diagonally).
#define HTBOTTOMRIGHT 17 //	In the lower-right corner of a border of a resizable window (the user can click the mouse to resize the window diagonally).
#define HTCAPTION 2	//In a title bar.
#define HTCLIENT 1 //	In a client area.
#define HTCLOSE 20 //In a Close button.
#define HTERROR -2 //	On the screen background or on a dividing line between windows (same as HTNOWHERE, except that the DefWindowProc function produces a system beep to indicate an error).
#define HTGROWBOX 4 //	In a size box (same as HTSIZE).
#define HTHELP 21 //	In a Help button.
#define HTHSCROLL 6 //	In a horizontal scroll bar.
#define HTLEFT 10 //	In the left border of a resizable window (the user can click the mouse to resize the window horizontally).
#define HTMENU 5 //	In a menu.
#define HTMAXBUTTON 9 //	In a Maximize button.
#define HTMINBUTTON 8 //	In a Minimize button.
#define HTNOWHERE 0 //	On the screen background or on a dividing line between windows.
#define HTREDUCE 8 //	In a Minimize button.
#define HTRIGHT 11 //	In the right border of a resizable window (the user can click the mouse to resize the window horizontally).
#define HTSIZE 4 //	In a size box (same as HTGROWBOX).
#define HTSYSMENU 3 //	In a window menu or in a Close button in a child window.
#define HTTOP 12 //	In the upper-horizontal border of a window.
#define HTTOPLEFT 13 //	In the upper-left corner of a window border.
#define HTTOPRIGHT 14 //	In the upper-right corner of a window border.
#define HTTRANSPARENT -1 //	In a window currently covered by another window in the same thread (the message will be sent to underlying windows in the same thread until one of them returns a code that is not HTTRANSPARENT).
#define HTVSCROLL 7 //	In the vertical scroll bar.
#define HTZOOM 9 //	In a Maximize button

#define WMSZ_BOTTOM 6
#define WMSZ_BOTTOMLEFT 7
#define WMSZ_BOTTOMRIGHT 8
#define WMSZ_LEFT 1
#define WMSZ_RIGHT 2
#define WMSZ_TOP 3
#define WMSZ_TOPLEFT 4
#define WMSZ_TOPRIGHT 5

#define MA_ACTIVATE 1 //	Activates the window, and does not discard the mouse message.
#define MA_ACTIVATEANDEAT 2 //	Activates the window, and discards the mouse message.
#define MA_NOACTIVATE 3 //	Does not activate the window, and does not discard the mouse message.
#define MA_NOACTIVATEANDEAT //4
  
#define COLOR_SCROLLBAR         0
#define COLOR_BACKGROUND        1
#define COLOR_ACTIVECAPTION     2
#define COLOR_INACTIVECAPTION   3
#define COLOR_MENU              4
#define COLOR_WINDOW            5
#define COLOR_WINDOWFRAME       6
#define COLOR_MENUTEXT          7
#define COLOR_WINDOWTEXT        8
#define COLOR_CAPTIONTEXT       9
#define COLOR_ACTIVEBORDER      10
#define COLOR_INACTIVEBORDER    11
#define COLOR_APPWORKSPACE      12
#define COLOR_HIGHLIGHT         13
#define COLOR_HIGHLIGHTTEXT     14
#define COLOR_BTNFACE           15
#define COLOR_BTNSHADOW         16
#define COLOR_GRAYTEXT          17
#define COLOR_BTNTEXT           18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT      20
#define COLOR_3DDKSHADOW        21
#define COLOR_3DLIGHT           22
#define COLOR_INFOTEXT          23
#define COLOR_INFOBK            24
#define COLOR_HOTLIGHT                  26
#define COLOR_GRADIENTACTIVECAPTION     27
#define COLOR_GRADIENTINACTIVECAPTION   28
#define COLOR_MENUBAR                 30
#define COLOR_DESKTOP           COLOR_BACKGROUND
#define COLOR_3DFACE            COLOR_BTNFACE
#define COLOR_3DSHADOW          COLOR_BTNSHADOW
#define COLOR_3DHIGHLIGHT       COLOR_BTNHIGHLIGHT
#define COLOR_3DHILIGHT         COLOR_BTNHIGHLIGHT
#define COLOR_BTNHILIGHT        COLOR_BTNHIGHLIGHT

#define CLR_NONE                0xFFFFFFFFL
#define CLR_DEFAULT             0xFF000000L

#define SM_CXSCREEN             0
#define SM_CYSCREEN             1
#define SM_CXVSCROLL            2
#define SM_CYHSCROLL            3
#define SM_CYCAPTION            4
#define SM_CXBORDER             5
#define SM_CYBORDER             6
#define SM_CXDLGFRAME           7
#define SM_CYDLGFRAME           8
#define SM_CYVTHUMB             9
#define SM_CXHTHUMB             10
#define SM_CXICON               11
#define SM_CYICON               12
#define SM_CXCURSOR             13
#define SM_CYCURSOR             14
#define SM_CYMENU               15
#define SM_CXFULLSCREEN         16
#define SM_CYFULLSCREEN         17
#define SM_CYKANJIWINDOW        18
#define SM_MOUSEPRESENT         19
#define SM_CYVSCROLL            20
#define SM_CXHSCROLL            21
#define SM_DEBUG                22
#define SM_SWAPBUTTON           23
#define SM_RESERVED1            24
#define SM_RESERVED2            25
#define SM_RESERVED3            26
#define SM_RESERVED4            27
#define SM_CXMIN                28
#define SM_CYMIN                29
#define SM_CXSIZE               30
#define SM_CYSIZE               31
#define SM_CXFRAME              32
#define SM_CYFRAME              33
#define SM_CXMINTRACK           34
#define SM_CYMINTRACK           35
#define SM_CXDOUBLECLK          36
#define SM_CYDOUBLECLK          37
#define SM_CXICONSPACING        38
#define SM_CYICONSPACING        39
#define SM_MENUDROPALIGNMENT    40
#define SM_PENWINDOWS           41
#define SM_DBCSENABLED          42
#define SM_CMOUSEBUTTONS        43
          
#define SM_CXFIXEDFRAME           SM_CXDLGFRAME  /* ;win40 name change */
#define SM_CYFIXEDFRAME           SM_CYDLGFRAME  /* ;win40 name change */
#define SM_CXSIZEFRAME            SM_CXFRAME     /* ;win40 name change */
#define SM_CYSIZEFRAME            SM_CYFRAME     /* ;win40 name change */

#define SM_SECURE               44
#define SM_CXEDGE               45
#define SM_CYEDGE               46
#define SM_CXMINSPACING         47
#define SM_CYMINSPACING         48
#define SM_CXSMICON             49
#define SM_CYSMICON             50
#define SM_CYSMCAPTION          51
#define SM_CXSMSIZE             52
#define SM_CYSMSIZE             53
#define SM_CXMENUSIZE           54
#define SM_CYMENUSIZE           55
#define SM_ARRANGE              56
#define SM_CXMINIMIZED          57
#define SM_CYMINIMIZED          58
#define SM_CXMAXTRACK           59
#define SM_CYMAXTRACK           60
#define SM_CXMAXIMIZED          61
#define SM_CYMAXIMIZED          62
#define SM_NETWORK              63
#define SM_CLEANBOOT            67
#define SM_CXDRAG               68
#define SM_CYDRAG               69
#define SM_SHOWSOUNDS           70
#define SM_CXMENUCHECK          71   /* Use instead of GetMenuCheckMarkDimensions()! */
#define SM_CYMENUCHECK          72
#define SM_SLOWMACHINE          73
#define SM_MIDEASTENABLED       74
#define SM_MOUSEWHEELPRESENT    75
#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#define SM_CMONITORS            80
#define SM_SAMEDISPLAYFORMAT    81
#define SM_IMMENABLED 82
#define SM_CMETRICS             83
#define SM_CYFOCUSBORDER 84
#define SM_TABLETPC 86
#define SM_MEDIACENTER 87
#define SM_STARTER 88
#define SM_SERVERR2 89
#define SM_DIGITIZER 94
#define SM_MAXIMUMTOUCHES 95


#define GW_CHILD 5
#define GW_ENABLEDPOPUP 6
#define GW_HWNDFIRST 0
#define GW_HWNDLAST 1
#define GW_HWNDNEXT 2
#define GW_HWNDPREV 3
#define GW_OWNER 4

 
  

#define MF_BYCOMMAND 0x0100
#define MF_BYPOSITION 0x0400
#define MNS_NOTIFYBYPOS     	(0x0080)        // cambiati per farli stare in 16bit :)
#define MNS_CHECKORBMP      	(0x0200)
#define MF_ENABLED  0x0000
#define MF_GRAYED   0x0001
#define MF_DISABLED 0x0002
#define MF_BITMAP 0x0004
#define MF_CHECKED 0x0008
#define MF_MENUBARBREAK 0x0020
#define MF_MENUBREAK 0x0040
#define MF_OWNERDRAW 0x0100
#define MF_POPUP 0x0010
#define MF_SEPARATOR 0x0800
#define MF_STRING 0x0000
#define MF_UNCHECKED 0x0000
#define MF_SYSMENU 0x2000
#define MF_MOUSESELECT 0x8000
  
#define MNC_CLOSE 1
#define MNC_EXECUTE 2
#define MNC_IGNORE 0
#define MNC_SELECT 3

    
typedef struct __attribute((packed)) {
//  UINT cbSize;
  UINT fMask;
  short int  nMin;
  short int  nMax;
  UINT nPage;
  short int  nPos;
  short int  nTrackPos;
  } SCROLLINFO;
typedef struct  __attribute((packed)) {
// vabbe'  DWORD cbSize;
  RECT  rcScrollBar;
  short int   dxyLineButton;
  short int   xyThumbTop;
  short int   xyThumbBottom;
//  int   reserved;
//  DWORD rgstate[CCHILDREN_SCROLLBAR + 1];
  } SCROLLBARINFO;
typedef struct __attribute((packed)) {
  union __attribute((packed)) {
    const char text[16];
    BITMAP *bitmap;
    };
  uint16_t command;
  WORD flags;
  struct _MENU *menu;
  } MENUITEM;
#define MAX_MENUITEMS 15
typedef struct __attribute((packed)) {
  char key[2];
// implicito  uint16_t command;
  } ACCELERATOR;
typedef struct __attribute((packed)) _MENU {
  MENUITEM menuItems[MAX_MENUITEMS];
  ACCELERATOR accelerators[MAX_MENUITEMS];
//#warning togliere da qua e metterlo in HWND? forse, più coerente
  } MENU;
typedef MENU *HMENU;
typedef struct __attribute((packed)) {
  const GFXfont *font;           //
  union __attribute((packed)) {
    struct __attribute((packed)) {
      unsigned char base64:6;
      unsigned char multiplier4:2;
      };
    struct __attribute((packed)) {
      unsigned char base16:4;
      unsigned char multiplier16:4;
      };
    BYTE size;
    };
  BYTE weight;      // come windows /10 ossia da 100 a 1000 ~
  BYTE inclination;
  union {
    struct __attribute((packed)) {
      unsigned int bold:1;
      unsigned int italic:1;
      unsigned int underline:1;
      unsigned int strikethrough:1;
      };
    BYTE attributes;
    };
  } FONT;
typedef FONT *HFONT;
typedef struct __attribute((packed)) {
  BYTE style;
  BYTE size;
  GFX_COLOR color;
  } PEN;
typedef struct __attribute((packed)) {
  BYTE style;
  BYTE size;
  GFX_COLOR color;
  } BRUSH;
typedef struct __attribute((packed)) {
  RECT area;            // in coordinate schermo (o relative al parent)
                    // servirebbero anche coordinate ViewPort... e coordinate cursore testo
  POINT cursor,textCursor;
  GFX_COLOR foreColor,backColor;
  PEN pen;
  BRUSH brush;
  FONT font;
  struct _WINDOW *hWnd;
  void *mem;
  struct __attribute((packed)) {        // aggiungere text alignment mode?
    unsigned char flags:2;
    unsigned char isMemoryDC:1;
    unsigned char unused:1;
    unsigned char rop:4;
    unsigned char mapping:4;
    };
  unsigned char textAlignment;
  uint8_t filler[1];
  } DC,*HDC;
STATIC_ASSERT(!(sizeof(DC) % 4),0);
typedef struct __attribute((packed)) {
//  union {
    RECT rcPaint;       // questa è in coordinate finestra (0,0,...)
    DC  DC;
//    };
  BOOL fErase;
  BOOL fRestore;
  BOOL fIncUpdate;
// per ora tolti!  BYTE rgbReserved[32];
} PAINTSTRUCT;
typedef union __attribute((packed)) {
  char class4[4];
  DWORD class;
  } CLASS;
typedef LRESULT (WINDOWPROC(void *,uint16_t,WPARAM,LPARAM));
typedef struct _MSG {
  struct _WINDOW *hWnd;
  WPARAM wParam;
  LPARAM lParam;
  DWORD  time;
//  DWORD  lPrivate;
  POINTS  pt;
  uint16_t  message;
} MSG;
typedef struct __attribute((packed)) _WINDOW {
	// HINSTANCE hInstance;		GWL_HINSTANCE -116
  RECT nonClientArea;       // in coordinate schermo (o relative al parent)
  RECT clientArea;          // in coordinate schermo (o relative al parent)
  RECT paintArea;           // in coordinate finestra 
  RECT savedArea;       // per maximized o icon
  DWORD style;      // di solito stile nei 24 bit alti; negli 8-12 bit bassi i control speciali  GWL_STYLE -96
//  DWORD styleEx;      
  WORD scrollSizeX,scrollSizeY;     
  WORD scrollPosX,scrollPosY;
  // ci sarebbe anche scrollRangeX ecc, nonché i minimi (per ora=0
  char caption[32];
  CLASS class;
  DWORD userdata;				// GWL_USERDATA=-48
  HMENU menu;       // vale anche come child ID! GWL_ID=-44
  ICON icon;
  CURSOR cursor;
  FONT  font;
  WINDOWPROC *windowProc;       // GWL_WNDPROC=-24
  MSG *messageQueue;
  struct _WINDOW *parent;		// GWL_HWNDPARENT=-16 .. pare che Parent sia diverso da Owner...
  struct _WINDOW *children;
  struct _WINDOW *next;     // ordinate per zOrder crescente; l'ultima dovrebbe sempre valere maxWindows
  BYTE internalState;        // 
  BYTE internalStateTemp;        // 
  union __attribute((packed)) {
    struct __attribute((packed)) {
        unsigned int active:1;
        unsigned int enabled:1;
        unsigned int visible:1;
        unsigned int maximized:1;
        unsigned int minimized:1;
    //      unsigned int iconic:1;
        unsigned int topmost:1;
        unsigned int focus:1;
        unsigned int locked:1;      // usato per menu
        };
    BYTE status;
    };
  BYTE zOrder;    // più alto = più davanti // OCCHIO PACKing! e alignment, per bubblesort... e per byte extra!
//  BYTE filler[0];
  } WINDOW;
STATIC_ASSERT(!(sizeof(struct _WINDOW) % 4),0);
typedef WINDOW *HWND;
typedef struct __attribute((packed)) {
  HWND   hwndFrom;
  DWORD  idFrom;
  UINT   code;
} NMHDR;
#define NM_FIRST                (0U-  0U)       // generic to all controls
#define NM_LAST                 (0U- 99U)
#define LVN_FIRST               (0U-100U)       // listview
#define LVN_LAST                (0U-199U)
// Property sheet reserved      (0U-200U) -  (0U-299U) - see prsht.h
#define HDN_FIRST               (0U-300U)       // header
#define HDN_LAST                (0U-399U)
#define TVN_FIRST               (0U-400U)       // treeview
#define TVN_LAST                (0U-499U)
#define TTN_FIRST               (0U-520U)       // tooltips
#define TTN_LAST                (0U-549U)
#define TCN_FIRST               (0U-550U)       // tab control
#define TCN_LAST                (0U-580U)
// Shell reserved               (0U-580U) -  (0U-589U)
#define CDN_FIRST               (0U-601U)       // common dialog (new)
#define CDN_LAST                (0U-699U)
#define TBN_FIRST               (0U-700U)       // toolbar
#define TBN_LAST                (0U-720U)
#define UDN_FIRST               (0U-721U)        // updown
#define UDN_LAST                (0U-729U)
#define DTN_FIRST               (0U-740U)       // datetimepick
#define DTN_LAST                (0U-745U)       // DTN_FIRST - 5
#define MCN_FIRST               (0U-746U)       // monthcal
#define MCN_LAST                (0U-752U)       // MCN_FIRST - 6
#define DTN_FIRST2              (0U-753U)       // datetimepick2
#define DTN_LAST2               (0U-799U)
#define CBEN_FIRST              (0U-800U)       // combo box ex
#define CBEN_LAST               (0U-830U)
#define RBN_FIRST               (0U-831U)       // rebar
#define RBN_LAST                (0U-859U)
#define IPN_FIRST               (0U-860U)       // internet address
#define IPN_LAST                (0U-879U)       // internet address
#define SBN_FIRST               (0U-880U)       // status bar
#define SBN_LAST                (0U-899U)
#define PGN_FIRST               (0U-900U)       // Pager Control
#define PGN_LAST                (0U-950U)
#define WMN_FIRST               (0U-1000U)
#define WMN_LAST                (0U-1200U)
#define BCN_FIRST               (0U-1250U)
#define BCN_LAST                (0U-1350U)
#define TRBN_FIRST              (0U-1501U)       // trackbar
#define TRBN_LAST               (0U-1519U)
#define MSGF_COMMCTRL_BEGINDRAG     0x4200
#define MSGF_COMMCTRL_SIZEHEADER    0x4201
#define MSGF_COMMCTRL_DRAGSELECT    0x4202
#define MSGF_COMMCTRL_TOOLBARCUST   0x4203

#define NM_OUTOFMEMORY          (NM_FIRST-1)
#define NM_CLICK                (NM_FIRST-2)    // uses NMCLICK struct
#define NM_DBLCLK               (NM_FIRST-3)
#define NM_RETURN               (NM_FIRST-4)
#define NM_RCLICK               (NM_FIRST-5)    // uses NMCLICK struct
#define NM_RDBLCLK              (NM_FIRST-6)
#define NM_SETFOCUS             (NM_FIRST-7)
#define NM_KILLFOCUS            (NM_FIRST-8)
#define NM_CUSTOMDRAW           (NM_FIRST-12)
#define NM_HOVER                (NM_FIRST-13)
#define NM_NCHITTEST            (NM_FIRST-14)   // uses NMMOUSE struct
#define NM_KEYDOWN              (NM_FIRST-15)   // uses NMKEY struct
#define NM_RELEASEDCAPTURE      (NM_FIRST-16)
#define NM_SETCURSOR            (NM_FIRST-17)   // uses NMMOUSE struct
#define NM_CHAR                 (NM_FIRST-18)   // uses NMCHAR struct
#define NM_TOOLTIPSCREATED      (NM_FIRST-19)   // notify of when the tooltips window is create
#define NM_LDOWN                (NM_FIRST-20)
#define NM_RDOWN                (NM_FIRST-21)
#define NM_THEMECHANGED         (NM_FIRST-22)
#define NM_FONTCHANGED          (NM_FIRST-23)
#define NM_CUSTOMTEXT           (NM_FIRST-24)   // uses NMCUSTOMTEXT struct
#define NM_TVSTATEIMAGECHANGING (NM_FIRST-24)   // uses NMTVSTATEIMAGECHANGING struct, defined after HTREEITEM

inline HWND __attribute__((always_inline)) WindowFromDC(HDC hDC);
enum MAPMODES {
   MM_TEXT = 1,
   MM_LOMETRIC = 2,
   MM_HIMETRIC = 3,
   MM_LOENGLISH = 4,
   MM_HIENGLISH = 5,
   MM_TWIPS = 6,
   MM_ISOTROPIC = 7,
   MM_ANISOTROPIC = 8,
   //Minimum and Maximum Mapping Mode values
   MM_MIN = MM_TEXT,
   MM_MAX = MM_ANISOTROPIC,
   MM_MAX_FIXEDSCALE = MM_TWIPS
	};
HDC GetWindowDC(HWND hWnd,HDC hDC);
HDC GetDC(HWND hWnd,HDC hDC);
GFX_COLOR GetDCBrushColor(HDC hdc);
GFX_COLOR SetDCBrushColor(HDC hdc,GFX_COLOR color);
GFX_COLOR GetDCPenColor(HDC hdc);
GFX_COLOR SetDCPenColor(HDC hdc,GFX_COLOR color);
BOOL GetDCOrgEx(HDC hdc,POINT *lppt);
BOOL SetViewportExtEx(HDC hdc,int x,int y,SIZE *lpsz);
BOOL SetWindowExtEx(HDC hdc,int x,int y,SIZE *lpsz);
BOOL SetWindowOrgEx(HDC hdc,int x,int y,POINT *lppt);
BYTE SetMapMode(HDC hdc,BYTE iMode);

typedef RECT *HRGN;      // per ora!
extern RECT theRgn;        // faccio così! una RGN statica globale...


#define DCX_WINDOW 1
#define DCX_CACHE 2
#define DCX_PARENTCLIP 32
#define DCX_CLIPSIBLINGS 16
#define DCX_CLIPCHILDREN 8
#define DCX_NORESETATTRS 4
#define DCX_LOCKWINDOWUPDATE 0x400
#define DCX_EXCLUDERGN 64
#define DCX_INTERSECTRGN 128
#define DCX_VALIDATE 0x200000
HDC GetDCEx(HWND hWnd,HDC hDC,HRGN hrgnClip,DWORD flags);
//HDC CreateDC(const char *pwszDriver,const char *pwszDevice,const char *pszPort,const DEVMODE *pdm);
HDC CreateCompatibleDC(HDC hdcOrig,HDC hdc);
BOOL ReleaseDC(HWND hWnd,HDC hDC);
int8_t SaveDC(HDC hdc);
BOOL RestoreDC(HDC hdc,int8_t nSavedDC);
HDC BeginPaint(HWND hWnd,PAINTSTRUCT *lpPaint);
BOOL EndPaint(HWND hWnd,const PAINTSTRUCT *lpPaint);
BOOL LockWindowUpdate(HWND hWndLock);
typedef struct _DISPLAY_DEVICE {
//  DWORD cb;
  char  DeviceName[32];
  char  DeviceString[128];
  DWORD StateFlags;
  char  DeviceID[128];
  char  DeviceKey[128];
} DISPLAY_DEVICE;
#define DISPLAY_DEVICE_ATTACHED_TO_DESKTOP 0x00000001
#define DISPLAY_DEVICE_MULTI_DRIVER        0x00000002
#define DISPLAY_DEVICE_PRIMARY_DEVICE      0x00000004
#define DISPLAY_DEVICE_MIRRORING_DRIVER    0x00000008
#define DISPLAY_DEVICE_VGA_COMPATIBLE      0x00000010
#define DISPLAY_DEVICE_REMOVABLE           0x00000020
#define DISPLAY_DEVICE_UNSAFE_MODES_ON     0x00080000
#define DISPLAY_DEVICE_TS_COMPATIBLE       0x00200000
#define DISPLAY_DEVICE_DISCONNECT          0x02000000
#define DISPLAY_DEVICE_REMOTE              0x04000000
#define DISPLAY_DEVICE_MODESPRUNED         0x08000000
BOOL EnumDisplayDevices(const char *lpDevice,BYTE iDevNum,DISPLAY_DEVICE *lpDisplayDevice,uint16_t dwFlags);
#define CCHDEVICENAME 32
#define CCHFORMNAME 32
#define DM_GRAYSCALE 1
#define DM_INTERLACED 2
#define DM_BITSPERPEL 0x40000
#define DM_PELSWIDTH 0x80000
#define DM_PELSHEIGHT 0x100000
#define DM_DISPLAYFLAGS 0x200000
#define CDS_UPDATEREGISTRY 1
#define CDS_TEST 2
#define CDS_FULLSCREEN 4
#define CDS_GLOBAL 8
#define CDS_SET_PRIMARY 10
#define CDS_NORESET 0x10000000
#define CDS_SETRECT 0x20000000
#define CDS_RESET 0x40000000
#define CDS_FORCE 0x80000000
enum {
    DISP_CHANGE_SUCCESSFUL = 0,
    DISP_CHANGE_RESTART = 1,
    DISP_CHANGE_FAILED = -1,
    DISP_CHANGE_BADMODE = -2,
    DISP_CHANGE_NOTUPDATED = -3,
    DISP_CHANGE_BADFLAGS = -4,
    DISP_CHANGE_BADPARAM = -5
    };
typedef struct _devicemode {
  BYTE  dmDeviceName[CCHDEVICENAME];
  WORD  dmSpecVersion;
  WORD  dmDriverVersion;
  WORD  dmSize;
  WORD  dmDriverExtra;
  DWORD dmFields;
  union {
    struct {
      short dmOrientation;
      short dmPaperSize;
      short dmPaperLength;
      short dmPaperWidth;
      short dmScale;
      short dmCopies;
      short dmDefaultSource;
      short dmPrintQuality;
    } DUMMYSTRUCTNAME;
    POINT dmPosition;
    struct {
      POINT dmPosition;
      DWORD  dmDisplayOrientation;
      DWORD  dmDisplayFixedOutput;
    } DUMMYSTRUCTNAME2;
  } DUMMYUNIONNAME;
  short dmColor;
  short dmDuplex;
  short dmYResolution;
  short dmTTOption;
  short dmCollate;
  BYTE  dmFormName[CCHFORMNAME];
  WORD  dmLogPixels;
  DWORD dmBitsPerPel;
  DWORD dmPelsWidth;
  DWORD dmPelsHeight;
  union {
    DWORD dmDisplayFlags;
    DWORD dmNup;
  } DUMMYUNIONNAME2;
  DWORD dmDisplayFrequency;
  DWORD dmICMMethod;
  DWORD dmICMIntent;
  DWORD dmMediaType;
  DWORD dmDitherType;
  DWORD dmReserved1;
  DWORD dmReserved2;
  DWORD dmPanningWidth;
  DWORD dmPanningHeight;
} DEVMODE;
LONG ChangeDisplaySettings(DEVMODE *lpDevMode, DWORD dwFlags);
LONG ChangeDisplaySettingsEx(LPCSTR lpszDeviceName,DEVMODE *lpDevMode,
  HWND hWnd,DWORD dwFlags,void *lParam);

typedef struct __attribute((packed)) {
  void*    lpCreateParams;
  HINSTANCE hInstance;
  MENU     *menu;
  HWND      hwndParent;
  UGRAPH_COORD_T       cy;
  UGRAPH_COORD_T       cx;
  UGRAPH_COORD_T       y;
  UGRAPH_COORD_T       x;
  DWORD      style;
  const char *lpszName;
  CLASS class;
//  DWORD     dwExStyle;
  } CREATESTRUCT;
typedef struct __attribute((packed)) {
  HWND hwnd;
  HWND hwndInsertAfter;
  UGRAPH_COORD_T  x;
  UGRAPH_COORD_T  y;
  UGRAPH_COORD_T  cx;
  UGRAPH_COORD_T  cy;
  UINT flags;
  } WINDOWPOS;
typedef struct __attribute((packed)) {
  RECT       rgrc[3];
  WINDOWPOS *lppos;
  } NCCALCSIZE_PARAMS;
#define WVR_VREDRAW 512
#define WVR_HREDRAW 256
#define WVR_REDRAW  (WVR_HREDRAW | WVR_VREDRAW)

typedef LRESULT (TIMERPROC(HWND,WORD,WORD,DWORD));
typedef struct __attribute((packed)) {
  HWND hWnd;
  DWORD timeout;
  DWORD time_cnt;
  WORD uEvent;
  TIMERPROC *tproc;
  } TIMER_DATA;

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

LRESULT SendMessage(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
#define PostMessage(a,b,c,d) SendMessage(a,b,c,d)       // per ora :)
BOOL IsWindowUnicode(HWND hWnd);

HWND WindowFromPoint(POINT Point);
HWND ChildWindowFromPoint(HWND parent,POINT Point);
HWND GetWindow(HWND hWnd,UINT uCmd);
typedef BOOL (WNDENUMPROC(HWND,LPARAM));
BOOL EnumChildWindows(HWND hWndParent,WNDENUMPROC lpEnumFunc,LPARAM lParam);
BOOL EnumWindows(WNDENUMPROC lpEnumFunc,LPARAM lParam);
BOOL IsChild(HWND hWndParent,HWND hWnd);
BOOL EnumDesktopWindows(HWND hWndDesk,WNDENUMPROC lpEnumFunc,LPARAM lParam);
HWND /*HDESK*/ GetThreadDesktop(DWORD dwThreadId);
HWND /*HDESK*/ OpenDesktop(const char *lpszDesktop,DWORD dwFlags,BOOL fInherit,DWORD /*ACCESS_MASK*/ dwDesiredAccess);
HWND GetDesktopWindow(void);
HWND GetShellWindow(void);
HWND GetRootWindow(void);
#define OCR_NORMAL 32512
#define OCR_IBEAM 32513
#define OCR_WAIT 32514
#define OCR_CROSS 32515
#define OCR_UP 32516
#define OCR_SIZE 32640
#define OCR_ICON 32641
#define OCR_SIZENWSE 32642
#define OCR_SIZENESW 32643
#define OCR_SIZEWE 32644
#define OCR_SIZENS 32645
#define OCR_SIZEALL 32646
#define OCR_NO 32648
#define OCR_APPSTARTING 32650
BOOL SetCursorPos(UGRAPH_COORD_T X,UGRAPH_COORD_T Y);
BOOL SetPhysicalCursorPos(UGRAPH_COORD_T X,UGRAPH_COORD_T Y);
BOOL GetCursorPos(POINT *lpPoint);
CURSOR SetCursor(CURSOR hCursor);
BOOL SetSystemCursor(CURSOR hCursor,DWORD id);
BOOL ClipCursor(const RECT *lpRect);
BOOL SwapMouseButton(BOOL fSwap);
typedef struct __attribute((packed)) {
//  DWORD cbSize;
  DWORD dwFlags;
  HWND  hwndTrack;
  WORD dwHoverTime;
} TRACKMOUSEEVENT;
BOOL TrackMouseEvent(TRACKMOUSEEVENT *lpEventTrack);
HWND SetCapture(HWND hWnd);
BOOL ReleaseCapture(HWND hWnd);
BOOL SetDoubleClickTime(uint16_t uMSeconds);
BOOL SetCaretPos(UGRAPH_COORD_T X,UGRAPH_COORD_T Y);
BOOL SetCaretBlinkTime(uint16_t uMSeconds);
BOOL HideCaret(HWND hWnd);
int GetDeviceCaps(HDC hDC,int index);
GFX_COLOR GetSysColor(int nIndex);
HWND CreateWindow(CLASS Class,const char *lpWindowName,
  DWORD dwStyle,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,
  HWND hWndParent,HMENU Menu,void *lpParam);
HWND CreateWindowEx(DWORD dwExStyle,CLASS Class,const char *lpWindowName,
  DWORD dwStyle,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,
  HWND hWndParent,HMENU Menu,void *lpParam);
BOOL DestroyWindow(HWND hWnd);
BOOL CloseWindow(HWND hWnd);
BOOL EnableWindow(HWND hWnd,BOOL bEnable);
HWND GetActiveWindow();
BOOL IsWindowVisible(HWND hWnd);
BOOL IsWindowEnabled(HWND hWnd);
BOOL RectVisible(HDC hDC,const RECT *lprect);
BOOL PtVisible(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y);

#define RDW_INVALIDATE   0x0001
#define RDW_INTERNALPAINT   0x0002
#define RDW_ERASE       0x0004
#define RDW_VALIDATE    0x0008
#define RDW_NOINTERNALPAINT 0x0010
#define RDW_NOERASE     0x0020
#define RDW_NOCHILDREN      0x0040
#define RDW_ALLCHILDREN     0x0080
#define RDW_UPDATENOW       0x0100
#define RDW_ERASENOW    0x0200
#define RDW_FRAME       0x0400
#define RDW_NOFRAME     0x0800
BOOL RedrawWindow(HWND hWnd,const RECT *lprcUpdate,HRGN hrgnUpdate,WORD flags);
#define SW_SCROLLCHILDREN   0x0001  /* Scroll children within *lprcScroll. */
#define SW_INVALIDATE       0x0002  /* Invalidate after scrolling */
#define SW_ERASE            0x0004  /* If SW_INVALIDATE, don't send WM_ERASEBACKGROUND */
#define SW_SMOOTHSCROLL     0x0010  /* Use smooth scrolling */
BOOL ScrollWindow(HWND hWnd,int16_t XAmount,int16_t YAmount,const RECT *lpRect,const RECT *lpClipRect);
int ScrollWindowEx(HWND hWnd,int16_t dx,int16_t dy,const RECT *prcScroll,const RECT *prcClip,
  HRGN hrgnUpdate,RECT *prcUpdate,WORD flags);
BOOL UpdateWindow(HWND hWnd);
BOOL OpenIcon(HWND hWnd);
BOOL PhysicalToLogicalPoint(HWND hWnd,POINT *lpPoint);
BOOL LPtoDP(HWND hWnd,POINT *lpPoint,int c);
typedef struct __attribute((packed)) {
  POINT ptReserved;
  POINT ptMaxSize;
  POINT ptMaxPosition;
  POINT ptMinTrackSize;
  POINT ptMaxTrackSize;
  } MINMAXINFO;
WORD TileWindows(HWND hwndParent,BYTE wHow,const RECT *lpRect,UINT cKids,const HWND *lpKids);
WORD CascadeWindows(HWND hwndParent,BYTE wHow,const RECT *lpRect,UINT cKids,const HWND *lpKids);
WORD MinimizeWindows(HWND hwndParent,const RECT *lpRect);

typedef struct __attribute((packed)) {
//  DWORD cbSize;
  RECT  rcWindow;
  RECT  rcClient;
  DWORD dwStyle;
  DWORD dwExStyle;
  DWORD dwWindowStatus;
  UINT  cxWindowBorders;
  UINT  cyWindowBorders;
  ATOM  atomWindowType;
  WORD  wCreatorVersion;
  } WINDOWINFO;
BOOL GetWindowInfo(HWND hwnd,WINDOWINFO *pwi);

#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_DBLCLKS          0x0008
#define CS_OWNDC            0x0020
#define CS_CLASSDC          0x0040
#define CS_PARENTDC         0x0080
#define CS_NOCLOSE          0x0200
#define CS_SAVEBITS         0x0800
#define CS_BYTEALIGNCLIENT  0x1000
#define CS_BYTEALIGNWINDOW  0x2000
#define CS_GLOBALCLASS      0x4000
#define CS_DROPSHADOW       0x8000      // diverso in windows ma ottimizzo!
typedef struct __attribute((packed)) _WNDCLASS {
//  HINSTANCE hInstance;
  CLASS class;							// GCW_ATOM -36
  DWORD style;							// GCL_STYLE -32
  ICON icon;								// GCL_HICON -28
  CURSOR cursor;						// GCL_HCURSOR -24
  WINDOWPROC *lpfnWndProc;	// GCL_WNDPROC -20
  HMENU menu;								// GCL_MENUNAME -16
  uint16_t cbClsExtra;			// GCL_CBCLSEXTRA -12
  uint16_t cbWndExtra;			// GCL_CBWNDEXTRA -10
  struct _WNDCLASS *next;       // occhio a alignment se si fa sort!
  BRUSH hbrBackground;              // e anche per alignment byte extra! GCW_HBRBACKGROUND -4
//  BYTE filler[0];
  } WNDCLASS;
  //se tra i flag c'è CS_CLASSDC, aggiungere un HDC completo al fondo della struct (come cbClsExtra), così come in WINDOW per CS_OWNDC
STATIC_ASSERT(!(sizeof(struct _WNDCLASS) % 4),0);
int RegisterClass(const WNDCLASS *lpWndClass);
BOOL UnregisterClass(CLASS Class, HINSTANCE hInstance);
HWND FindWindow(const CLASS *Class, const char *lpWindowName);
HWND FindWindowEx(HWND hWndParent,HWND hWndChildAfter,const CLASS *Class, const char *lpWindowName);
BOOL GetClassInfo(HINSTANCE hInstance, CLASS Class, WNDCLASS **lpWndClass);
int GetClassName(HWND hWnd, CLASS *Class);

#define GCW_HBRBACKGROUND -4
#define GCL_CBWNDEXTRA -10
#define GCL_CBCLSEXTRA -12
#define GCL_MENUNAME -16
#define GCL_WNDPROC -20
#define GCL_HCURSOR -24
#define GCL_HICON -28
#define GCL_STYLE -32
#define GCW_ATOM -36 // qua class... direi che ha senso
//#define GCL_HICONSM -34
//#define GCL_HMODULE -16
#define GWL_HWNDPARENT -16
#define GWLP_HWNDPARENT
#define GWL_WNDPROC -24
#define GWLP_WNDPROC   //METTERE!
#define GWL_ID -44
#define GWL_USERDATA -48
#define GWLP_USERDATA
//#define GWL_EXSTYLE -84
#define GWL_STYLE -96
#define GWL_HINSTANCE -116
#define GWLP_HINSTANCE
DWORD GetClassLong(HWND hWnd,int nIndex);
WORD GetClassWord(HWND hWnd,int nIndex);
DWORD GetWindowLong(HWND hWnd,int nIndex);
void SetClassLong(HWND hWnd,int nIndex,DWORD value);
void SetWindowLong(HWND hWnd,int nIndex,DWORD value);
BYTE GetClassByte(HWND hWnd,int nIndex);
BYTE GetWindowByte(HWND hWnd,int nIndex);
WORD GetWindowWord(HWND hWnd,int nIndex);       // mie estensioni!
void SetClassByte(HWND hWnd,int nIndex,BYTE value);
void SetWindowByte(HWND hWnd,int nIndex,BYTE value);
void SetWindowWord(HWND hWnd,int nIndex,WORD value);
uint64_t SetWindowLongPtr(HWND hWnd,int nIndex,uint64_t dwNewLong);     // da 64bit...
uint64_t GetWindowLongPtr(HWND hWnd,int nIndex);
#define GET_WINDOW_OFFSET(w,n) (((unsigned char *)w)+sizeof(struct _WINDOW)+n)
#define GET_WINDOW_DLG_OFFSET(w,n) (((unsigned char *)hWnd)+sizeof(struct _WINDOW)+DWL_USER+n)
#define GET_CLASS_OFFSET(c,n) (((unsigned char *)c)+sizeof(WNDCLASS)+n)

BOOL SetWindowPos(HWND hWnd,HWND hWndInsertAfter,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy, DWORD uFlags);
BOOL MoveWindow(HWND hWnd,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,BOOL bRepaint);
typedef struct __attribute((packed)) {
//  UINT  length /*cbSize*/;
  UINT  flags;
  uint16_t showCmd;
  POINT ptMinPosition;
  POINT ptMaxPosition;
  RECT  rcNormalPosition;
  RECT  rcDevice;
  } WINDOWPLACEMENT;
BOOL SetWindowPlacement(HWND hWnd,const WINDOWPLACEMENT *lpwndpl);
BOOL GetWindowPlacement(HWND hWnd,WINDOWPLACEMENT *lpwndpl);
BOOL ShowWindow(HWND hWnd,BYTE nCmdShow);
BOOL ShowOwnedPopups(HWND hWnd,BOOL fShow);
BOOL BringWindowToTop(HWND hWnd);
HWND SetActiveWindow(HWND hWnd);
BOOL SetForegroundWindow(HWND hWnd);
BOOL LockSetForegroundWindow(BYTE uLockCode);
void SwitchToThisWindow(HWND hWnd,BOOL fUnknown);
HWND SetFocus(HWND hWnd);
HWND GetFocus(void);
HWND SetParent(HWND hWndChild,HWND hWndNewParent);
HWND GetParent(HWND hWnd);
HWND GetTopWindow(HWND hWnd);
HWND GetForegroundWindow(void);
BOOL GetClientRect(HWND hWnd,RECT *lpRect);
BOOL GetWindowRect(HWND hWnd,RECT *lpRect);
BOOL IsWindow(HWND hWnd);
BOOL IsZoomed(HWND hWnd);
BOOL IsIconic(HWND hWnd);
BOOL AdjustWindowRect(RECT *lpRect,DWORD dwStyle,BOOL bMenu);
BOOL AdjustWindowRectEx(RECT *lpRect,DWORD dwStyle,BOOL bMenu,DWORD dwStyleEx);
UINT ArrangeIconicWindows(HWND hWnd);
BOOL SetIcon(HWND hWnd,const GFX_COLOR *icon,BYTE type);
typedef struct __attribute((packed)) {
  BOOL    fIcon;
  UGRAPH_COORD_T xHotspot;
  UGRAPH_COORD_T yHotspot;
  BITMAP *hbmMask;
  BITMAP *hbmColor;
  } ICONINFO;
BOOL GetIconInfo(ICON hIcon,ICONINFO *piconinfo);
BOOL SetFont(HWND hWnd,FONT font);
int SetScrollPos(HWND hWnd,BYTE nBar,uint16_t nPos,BOOL bRedraw);
int SetScrollInfo(HWND hwnd,BYTE nBar,SCROLLINFO *lpsi,BOOL redraw);
BOOL SetScrollRange(HWND hWnd,BYTE nBar,uint16_t nMinPos,uint16_t nMaxPos,BOOL bRedraw);
BOOL ShowScrollBar(HWND hWnd,BYTE wBar,BOOL bShow);
BOOL EnableScrollBar(HWND hWnd,UINT wSBflags,UINT wArrows);

typedef struct __attribute((packed)) {
//  DWORD cbSize;
  RECT  rcTitleBar;
  DWORD rgstate[5 /*CCHILDREN_TITLEBAR*/ + 1];
  } TITLEBARINFO;
typedef struct __attribute((packed))  {
//  DWORD cbSize;
  RECT  rcTitleBar;
  DWORD rgstate[5 /*CCHILDREN_TITLEBAR*/ + 1];
  RECT  rgrect[5 /*CCHILDREN_TITLEBAR*/ + 1];
  } TITLEBARINFOEX;
#define STATE_SYSTEM_FOCUSABLE 0x00100000
#define STATE_SYSTEM_INVISIBLE 0x00008000
#define STATE_SYSTEM_OFFSCREEN 0x00010000
#define STATE_SYSTEM_UNAVAILABLE 0x00000001
#define STATE_SYSTEM_PRESSED 0x00000008
BOOL SetWindowText(HWND hWnd,const char *title);
int GetWindowText(HWND hWnd,char *lpString,int nMaxCount);
BOOL GetTitleBarInfo(HWND hWnd,TITLEBARINFO *pti);
void SetWindowTextCursor(HDC ,BYTE,BYTE);
#define TRANSPARENT  1
#define OPAQUE       2
#define TA_LEFT      0
#define TA_TOP      0
#define TA_NOUPDATECP 0
#define TA_UPDATECP 1
#define TA_CENTER    6
#define TA_RIGHT     2
#define TA_BASELINE 24
#define TA_RTLREADING     128   // mia modifica tanto non lo uso!
#define TA_BOTTOM 8
#define TA_MASK (TA_BASELINE + TA_CENTER + TA_UPDATECP)
#define VTA_BASELINE TA_BASELINE
#define VTA_LEFT     TA_BOTTOM
#define VTA_RIGHT    TA_TOP
#define VTA_CENTER   TA_CENTER
#define VTA_BOTTOM   TA_RIGHT
#define VTA_TOP      TA_LEFT
GFX_COLOR SetTextColor(HDC hDC,GFX_COLOR color);
GFX_COLOR SetBkColor(HDC hDC,GFX_COLOR color);
GFX_COLOR GetTextColor(HDC hDC);
GFX_COLOR GetBkColor(HDC hDC);
#define GetRValue(rgb) ColorR(rgb)
#define GetGValue(rgb) ColorG(rgb)
#define GetBValue(rgb) ColorB(rgb)
int SetBkMode(HDC hDC,int mode);
int GetBkMode(HDC hDC);
int GetTextFace(HDC hDC,WORD c,char *lpName);
DWORD GetFontData(HDC hDC,DWORD dwTable,DWORD dwOffset,VOID *pvBuffer,DWORD cjBuffer);
int SetTextCharacterExtra(HDC hDC,int extra);
BYTE SetTextAlign(HDC hDC,BYTE align);
BYTE GetTextAlign(HDC hDC);
BOOL GetTextExtentExPoint(HDC hdc,const char *lpszString,int cchString,
  int nMaxExtent,int *lpnFit,int *lpnDx,SIZE *lpSize);
BOOL GetTextExtentPoint(HDC hDC,const char *lpString,WORD c,SIZE *psize);
BOOL GetTextExtentPoint32(HDC hDC,const char *lpString,WORD n,SIZE *psizl);
int GetTextCharacterExtra(HDC hdc);
BOOL GetCharWidth(HDC hDC,uint16_t iFirst,uint16_t iLast,uint16_t *lpBuffer);
#define GetCharWidth32(a,b,c,d) GetCharWidth(a,b,c,d)
BOOL GetCharABCWidths(HDC hDC,uint16_t wFirst,uint16_t wLast,uint16_t *lpBuffer /*ABC *lpABC*/); // qua così!
BOOL TextOut(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const char *s,uint16_t c);
BOOL ExtTextOut(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,UINT options,const RECT *lprect,const char *lpString,uint16_t n,const INT *lpDx);
BOOL ExtTextOutWrap(HDC hDC,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UINT uoptions,const RECT *lprc,const char *lpString,uint16_t cbCount,const INT *lpDx);
LONG TabbedTextOut(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,const char *lpString,uint16_t chCount,BYTE nTabPositions,const BYTE *lpnTabStopPositions,BYTE nTabOrigin);
typedef struct __attribute((packed)) {
  UGRAPH_COORD_T x;
  UGRAPH_COORD_T y;
  UINT   n;
  const char *lpstr;
  UINT   uiFlags;
  RECT   rcl;
  int    *pdx;
} POLYTEXT;
BOOL PolyTextOut(HDC hdc,const POLYTEXT *ppt,WORD nstrings);
#define DT_TOP                      0x00000000
#define DT_LEFT                     0x00000000
#define DT_CENTER                   0x00000001
#define DT_RIGHT                    0x00000002
#define DT_VCENTER                  0x00000004
#define DT_BOTTOM                   0x00000008
#define DT_WORDBREAK                0x00000010
#define DT_SINGLELINE               0x00000020
#define DT_EXPANDTABS               0x00000040
#define DT_TABSTOP                  0x00000080
#define DT_NOCLIP                   0x00000100
#define DT_EXTERNALLEADING          0x00000200
#define DT_CALCRECT                 0x00000400
#define DT_NOPREFIX                 0x00000800
#define DT_INTERNAL                 0x00001000
#define DT_EDITCONTROL              0x00002000
#define DT_PATH_ELLIPSIS            0x00004000
#define DT_END_ELLIPSIS             0x00008000
#define DT_MODIFYSTRING             0x00010000
#define DT_RTLREADING               0x00020000
#define DT_WORD_ELLIPSIS            0x00040000
#define DT_NOFULLWIDTHCHARBREAK     0x00080000
#define DT_HIDEPREFIX               0x00100000
#define DT_PREFIXONLY               0x00200000
int DrawText(HDC hDC,const char *lpchText,int cchText,RECT *lprc,UINT format);
typedef enum {
   ETO_OPAQUE = 0x00000002,
   ETO_CLIPPED = 0x00000004,
   ETO_GLYPH_INDEX = 0x00000010,
   ETO_RTLREADING = 0x00000080,
   ETO_NO_RECT = 0x00000100,            // questa roba sembra per windows + recenti.. "EMR"
   ETO_SMALL_CHARS = 0x00000200,        // idem
   ETO_NUMERICSLOCAL = 0x00000400,
   ETO_NUMERICSLATIN = 0x00000800,
   ETO_IGNORELANGUAGE = 0x00001000,
   ETO_PDY = 0x00002000,
   ETO_REVERSE_INDEX_MAP = 0x00010000
    } ExtTextOutOptions;
GFX_COLOR SetPixel(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR color);
BOOL SetPixelV(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR color);
GFX_COLOR GetPixel(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y);
BOOL MoveToEx(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,POINT *lppt);
#define MoveTo(a,b,c) MoveToEx(a,b,c,NULL)
BOOL LineTo(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y);
BOOL Chord(HDC hDC,UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2,
  UGRAPH_COORD_T x3,UGRAPH_COORD_T y3,UGRAPH_COORD_T x4,UGRAPH_COORD_T y4);
BOOL Ellipse(HDC hDC,UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2);
BOOL Arc(HDC hDC,UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2,
  UGRAPH_COORD_T x3,UGRAPH_COORD_T y3,UGRAPH_COORD_T x4,UGRAPH_COORD_T y4);
BOOL FillRgn(HDC hDC,HRGN hrgn,BRUSH hbr);
BOOL Rectangle(HDC hDC,UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2);
BOOL FillRect(HDC hDC,const RECT *lprc,BRUSH hbr);
BOOL InvertRect(HDC hDC,const RECT *lprc);
BOOL FrameRect(HDC hDC,const RECT *lprc,BRUSH hbr);
BOOL Polygon(HDC hDC,const POINT *apt,int cpt);
BOOL Polyline(HDC hDC,const POINT *apt,int cpt);
BOOL PolylineTo(HDC hDC,const POINT *apt,DWORD cpt);
BOOL FloodFill(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR color);
BOOL ExtFloodFill(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR color,UINT type);
#define  FLOODFILLBORDER   0
#define  FLOODFILLSURFACE  1
int SetPolyFillMode(HDC hDC,int mode);
#define  ALTERNATE 0
#define  WINDING 1

enum TERNARY_OP GetROP2(HDC hdc);
enum TERNARY_OP SetROP2(HDC hdc,enum TERNARY_OP rop2);

BOOL BitBlt(HDC hdc,UGRAPH_COORD_T x,UGRAPH_COORD_T y,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy,HDC hdcSrc,UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,DWORD rop);
BOOL StretchBlt(HDC hdcDest,UGRAPH_COORD_T xDest,UGRAPH_COORD_T yDest,GRAPH_COORD_T wDest,GRAPH_COORD_T hDest,
  HDC hdcSrc,UGRAPH_COORD_T xSrc,UGRAPH_COORD_T ySrc,GRAPH_COORD_T wSrc,GRAPH_COORD_T hSrc,DWORD rop);
int StretchDIBits(HDC hdc,UGRAPH_COORD_T xDest,UGRAPH_COORD_T yDest,GRAPH_COORD_T DestWidth,GRAPH_COORD_T DestHeight,
  UGRAPH_COORD_T xSrc,UGRAPH_COORD_T ySrc,GRAPH_COORD_T SrcWidth,GRAPH_COORD_T SrcHeight,const void *lpBits,
  const GFX_COLOR *lpbmi,BYTE iUsage,enum TERNARY_OP rop);
GFX_COLOR *CopyImage(GFX_COLOR *h,BYTE type,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy,UINT flags);
int SetDIBits(HDC hdc,GFX_COLOR hbm[],int16_t start,uint16_t cLines,const VOID *lpBits,const GFX_COLOR *lpbmi,UINT ColorUse);
BYTE SetStretchBltMode(HDC hdc,BYTE mode);
BOOL GetBitmapDimensionEx(BITMAP *hbit,S_SIZE *lpsize);
long GetBitmapBits(BITMAP *hbit,long cb,void *lpvBits);

/*sono solo MFC ... fare?*/
void FillSolidRect(HDC hDC,const RECT *lpRect, GFX_COLOR clr);
void FillSolidRectCoords(HDC hDC,int x, int y, int cx, int cy, GFX_COLOR clr);      // C++
BOOL ModifyStyle(HWND hWnd,DWORD dwRemove, DWORD dwAdd, UINT nFlags /*=0*/); // http://www.icodeguru.com/vc&mfc/mfcreference/html/_mfc_cwnd.3a3a.modifystyle.htm
//BOOL ModifyStyleEx(HWND hWnd,DWORD dwRemove, DWORD dwAdd, UINT nFlags /*=0*/); 
void Draw3dRect(HDC hDC,const RECT *lpRect,
    GFX_COLOR clrTopLeft,GFX_COLOR clrBottomRight);
void Draw3dRectCoords(HDC hDC,int x,int y,int cx,int cy,        // mio nome! C++
    GFX_COLOR clrTopLeft,GFX_COLOR clrBottomRight);
void DrawDragRect(HDC hDC,const RECT *lpRect,SIZE size,const RECT *lpRectLast,SIZE sizeLast,
    BRUSH pBrush /*=NULL*/,BRUSH pBrushLast /*=NULL*/);

HRGN CreateRectRgn(int x1,int y1,int x2,int y2);
HRGN CreateRectRgnIndirect(const RECT *rect);
HRGN CreatePolygonRgn(const POINT *pptl,int cPoint,int iMode);
HRGN CreatePolyPolygonRgn(const POINT *pptl,const INT *pc,int cPoly,int iMode);
HRGN CreateRoundRectRgn(int x1,int y1,int x2,int y2,int w,int h);
int GetWindowRgn(HWND hWnd,HRGN hRgn);
int SetWindowRgn(HWND hWnd,HRGN hRgn,BOOL bRedraw);
int GetWindowRgnBox(HWND hWnd,RECT *lprc);
#define ERRORREGION		0
#define NULLREGION		1
#define SIMPLEREGION	2
#define COMPLEXREGION	3


#define PRF_CHECKVISIBLE    0x00000001L
#define PRF_NONCLIENT       0x00000002L
#define PRF_CLIENT          0x00000004L
#define PRF_ERASEBKGND      0x00000008L
#define PRF_CHILDREN        0x00000010L
#define PRF_OWNED           0x00000020L

enum __attribute__ ((__packed__)) {          // mi servono per la mia SelectObject
    OBJ_PEN,
    OBJ_BRUSH,
    OBJ_FONT,
    OBJ_ICON,
    OBJ_BITMAP,
    OBJ_REGION
    };
#define PS_SOLID            0
#define PS_DASH             1
#define PS_DOT              2
#define PS_DASHDOT          3
#define PS_DASHDOTDOT       4
#define PS_NULL             5
#define PS_INSIDEFRAME      6
PEN CreatePen(BYTE iStyle,BYTE cWidth,GFX_COLOR color);
PEN ExtCreatePen(BYTE iPenStyle,BYTE cWidth,const BRUSH *plbrush,BYTE cStyle,const BYTE *pstyle);
GFX_COLOR SetDCPenColor(HDC hDC,GFX_COLOR color);
// Brush Styles
#define BS_SOLID            0
#define BS_NULL             1
#define BS_HOLLOW           BS_NULL
#define BS_HATCHED          2
#define BS_PATTERN          3
#define BS_INDEXED          4
#define BS_DIBPATTERN       5
// Hatch Styles
#define HS_HORIZONTAL       0
#define HS_VERTICAL         1
#define HS_FDIAGONAL        2
#define HS_BDIAGONAL        3
#define HS_CROSS            4
#define HS_DIAGCROSS        5
BRUSH CreateSolidBrush(GFX_COLOR color);
typedef struct __attribute__ ((__packed__)) {
  BYTE      lbStyle;
  GFX_COLOR lbColor;
  DWORD   *lbHatch;
} LOGBRUSH;
BRUSH CreateBrushIndirect(const LOGBRUSH *plbrush);
BRUSH CreateHatchBrush(BYTE iHatch,GFX_COLOR color);
BRUSH CreatePatternBrush(BITMAP *hbm);
BRUSH GetSysColorBrush(int nIndex);
GFX_COLOR SetDCBrushColor(HDC hDC,GFX_COLOR color);
enum __attribute__ ((__packed__)) FONTWEIGHT {
    FW_DONTCARE = 0,
    FW_THIN = 100,
    FW_EXTRALIGHT = 200,
    FW_LIGHT = 300,
    FW_NORMAL = 400,
    FW_MEDIUM = 500,
    FW_SEMIBOLD = 600,
    FW_BOLD = 700,
    FW_EXTRABOLD = 800,
    FW_HEAVY = 900,
    };
#define TMPF_FIXED_PITCH    0x0	/* win32 bug: 1 means variable*/
#define TMPF_VARIABLE_PITCH 0x01
#define TMPF_VECTOR         0x02
#define TMPF_TRUETYPE       0x04
#define TMPF_DEVICE         0x08
typedef struct __attribute((packed)) {
  WORD tmHeight;
  WORD tmAscent;
  WORD tmDescent;
  WORD tmInternalLeading;
  WORD tmExternalLeading;
  WORD tmAveCharWidth;
  WORD tmMaxCharWidth;
  WORD tmWeight;
  WORD tmOverhang;
  WORD tmDigitizedAspectX;
  WORD tmDigitizedAspectY;
  BYTE tmFirstChar;
  BYTE tmLastChar;
  BYTE tmDefaultChar;
  BYTE tmBreakChar;
  BYTE tmItalic;
  BYTE tmUnderlined;
  BYTE tmStruckOut;
  BYTE tmPitchAndFamily;
  BYTE tmCharSet;
  } TEXTMETRIC;
typedef struct __attribute((packed)) {
  WORD lfHeight;
  WORD lfWidth;
  WORD lfEscapement;
  WORD lfOrientation;
  WORD lfWeight;
  BYTE lfItalic;
  BYTE lfUnderline;
  BYTE lfStrikeOut;
  BYTE lfCharSet;
  BYTE lfOutPrecision;
  BYTE lfClipPrecision;
  BYTE lfQuality;
  BYTE lfPitchAndFamily;
  char lfFaceName[16 /*LF_FACESIZE*/];
  } LOGFONT;
enum __attribute__ ((__packed__)) FONT_MASK {
    RASTER_FONTTYPE = 0x0001,
    DEVICE_FONTTYPE = 0x0002,
    TRUETYPE_FONTTYPE = 0x0004
	};
enum __attribute__ ((__packed__)) FONTPITCHANDFAMILY {
    DEFAULT_PITCH = 0,
    FIXED_PITCH = 1,
    VARIABLE_PITCH = 2,
    FF_DONTCARE = (0 << 4),
    FF_ROMAN = (1 << 4),
    FF_SWISS = (2 << 4),
    FF_MODERN = (3 << 4),
    FF_SCRIPT = (4 << 4),
    FF_DECORATIVE = (5 << 4),
    };
enum __attribute__ ((__packed__)) FONTCHARSET {
    ANSI_CHARSET = 0,
    DEFAULT_CHARSET = 1,
    SYMBOL_CHARSET = 2,
    SHIFTJIS_CHARSET = 128,
    HANGEUL_CHARSET = 129,
    HANGUL_CHARSET = 129,
    GB2312_CHARSET = 134,
    CHINESEBIG5_CHARSET = 136,
    OEM_CHARSET = 255,
    JOHAB_CHARSET = 130,
    HEBREW_CHARSET = 177,
    ARABIC_CHARSET = 178,
    GREEK_CHARSET = 161,
    TURKISH_CHARSET = 162,
    VIETNAMESE_CHARSET = 163,
    THAI_CHARSET = 222,
    EASTEUROPE_CHARSET = 238,
    RUSSIAN_CHARSET = 204,
    MAC_CHARSET = 77,
    BALTIC_CHARSET = 186,
    };
enum __attribute__ ((__packed__)) FONTPRECISION {
    OUT_DEFAULT_PRECIS = 0,
    OUT_STRING_PRECIS = 1,
    OUT_CHARACTER_PRECIS = 2,
    OUT_STROKE_PRECIS = 3,
    OUT_TT_PRECIS = 4,
    OUT_DEVICE_PRECIS = 5,
    OUT_RASTER_PRECIS = 6,
    OUT_TT_ONLY_PRECIS = 7,
    OUT_OUTLINE_PRECIS = 8,
    OUT_SCREEN_OUTLINE_PRECIS = 9,
    OUT_PS_ONLY_PRECIS = 10,
    };
enum __attribute__ ((__packed__)) FONTCLIPPRECISION {
    CLIP_DEFAULT_PRECIS = 0,
    CLIP_CHARACTER_PRECIS = 1,
    CLIP_STROKE_PRECIS = 2,
    CLIP_MASK = 0xf,
    CLIP_LH_ANGLES = (1 << 4),
    CLIP_TT_ALWAYS = (2 << 4),
    CLIP_DFA_DISABLE = (4 << 4),
    CLIP_EMBEDDED = (8 << 4),
    };
enum __attribute__ ((__packed__)) FONTQUALITY {
    DEFAULT_QUALITY = 0,
    DRAFT_QUALITY = 1,
    PROOF_QUALITY = 2,
    NONANTIALIASED_QUALITY = 3,
    ANTIALIASED_QUALITY = 4,
    CLEARTYPE_QUALITY = 5,
    CLEARTYPE_NATURAL_QUALITY = 6,
    };
FONT CreateFont(BYTE cHeight,BYTE cWidth,BYTE cEscapement,BYTE cOrientation,WORD cWeight,
  BYTE bItalic,BYTE bUnderline,BYTE bStrikeOut,enum FONTCHARSET iCharSet,enum FONTPRECISION iOutPrecision,
  enum FONTCLIPPRECISION iClipPrecision, WORD iQuality, enum FONTPITCHANDFAMILY iPitchAndFamily,const char *pszFaceName);
inline uint8_t getFontHeight(HFONT font);
inline uint8_t getFontWidth(HFONT font);
BOOL GetTextMetrics(HDC hDC,TEXTMETRIC *lptm);
typedef LRESULT (FONTENUMPROC(const LOGFONT *lplf,const TEXTMETRIC *lptm,DWORD dwType,LPARAM lpData));
int EnumFonts(HDC hDC,const char *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam);
int EnumFontFamilies(HDC hDC,const char *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam);
int EnumFontFamiliesEx(HDC hDC,const char /*LOGFONT*/ *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam,DWORD dwFlags);

#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define HOLLOW_BRUSH        NULL_BRUSH
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16
#define DEFAULT_GUI_FONT    17
#define DC_BRUSH            18
#define DC_PEN              19
#define BORDERX_PEN 32
#define BORDERY_PEN 33 
#define STOCK_LAST          33
typedef union __attribute((packed)) {
  FONT  font;
  PEN   pen;
  BRUSH brush;
//    REGION region;
    ICON icon;
    BITMAP bitmap;
  uint64_t v;       // bah sembrava un'idea... per ora lo lascio
  } GDIOBJ;
GDIOBJ GetStockObject(int i);
GDIOBJ SelectObject(HDC hDC,BYTE type,GDIOBJ g);		// io faccio così
BOOL DeleteObject(BYTE type,GDIOBJ g);		// io faccio così

CURSOR LoadCursor(HINSTANCE hInstance,const char *lpCursorName);
ICON LoadIcon(HINSTANCE hInstance,const char *lpIconName);
HANDLE LoadImage(HINSTANCE hInst,const char *name,BYTE type,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy,BYTE fuLoad);
uint16_t LoadString(HINSTANCE hInstance,UINT uID,char *lpBuffer,uint16_t cchBufferMax);
HMENU LoadMenu(HINSTANCE hInstance,const char *lpMenuName);


#define DFC_CAPTION   1		
#define DFC_MENU      2		
#define DFC_SCROLL    3		
#define DFC_BUTTON    4		
#define DFC_POPUPMENU 5		

#define DFCS_CAPTIONCLOSE   0
#define DFCS_CAPTIONMIN     1
#define DFCS_CAPTIONMAX     2
#define DFCS_CAPTIONRESTORE 3
#define DFCS_CAPTIONHELP    4

#define DFCS_MENUARROW      0
#define DFCS_MENUCHECK      1
#define DFCS_MENUBULLET     2
#define DFCS_MENUARROWRIGHT 4

#define DFCS_BUTTONCHECK      0
#define DFCS_BUTTONRADIOIMAGE 1
#define DFCS_BUTTONRADIOMASK  2
#define DFCS_BUTTONRADIO      4
#define DFCS_BUTTON3STATE     8
#define DFCS_BUTTONPUSH    0x10

#define DFCS_SCROLLUP         0
#define DFCS_SCROLLDOWN       1
#define DFCS_SCROLLLEFT       2
#define DFCS_SCROLLRIGHT      3
#define DFCS_SCROLLCOMBOBOX   5
#define DFCS_SCROLLSIZEGRIP   8
#define DFCS_SCROLLSIZEGRIPRIGHT 0x10

#define DFCS_INACTIVE    0x100
#define DFCS_PUSHED      0x200
#define DFCS_CHECKED     0x400
#define DFCS_TRANSPARENT 0x800
#define DFCS_HOT         0x1000
#define DFCS_ADJUSTRECT  0x2000
#define DFCS_FLAT        0x4000
#define DFCS_MONO        0x8000

//---------- ---------- ---------- ---------- ----------
BOOL DrawFrameControl(HDC,RECT *,BYTE,uint16_t);
/* flags for DrawCaption */
#define DC_ACTIVE           0x01
#define DC_SMALLCAP         0x02
#define DC_ICON             0x04
#define DC_TEXT             0x08
#define DC_INBUTTON         0x10
#define DC_GRADIENT         0x20
#define DC_BUTTONS          0x80    // 0x1000 faccio byte
BOOL DrawCaption(HWND hwnd,HDC hdc,const RECT *lprect,BYTE flags);
#define BDR_RAISEDOUTER 1
#define BDR_SUNKENOUTER 2
#define BDR_RAISEDINNER 4
#define BDR_SUNKENINNER 8
#define EDGE_BUMP ( BDR_RAISEDOUTER | BDR_SUNKENINNER )
#define EDGE_ETCHED ( BDR_SUNKENOUTER | BDR_RAISEDINNER )
#define EDGE_RAISED ( BDR_RAISEDOUTER | BDR_RAISEDINNER )
#define EDGE_SUNKEN ( BDR_SUNKENOUTER | BDR_SUNKENINNER )
/* Border flags */
#define BF_LEFT         0x0001
#define BF_TOP          0x0002
#define BF_RIGHT        0x0004
#define BF_BOTTOM       0x0008
#define BF_TOPLEFT      (BF_TOP | BF_LEFT)
#define BF_TOPRIGHT     (BF_TOP | BF_RIGHT)
#define BF_BOTTOMLEFT   (BF_BOTTOM | BF_LEFT)
#define BF_BOTTOMRIGHT  (BF_BOTTOM | BF_RIGHT)
#define BF_RECT         (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)
#define BF_DIAGONAL     0x0010
// For diagonal lines, the BF_RECT flags specify the end point of the
// vector bounded by the rectangle parameter.
#define BF_DIAGONAL_ENDTOPRIGHT     (BF_DIAGONAL | BF_TOP | BF_RIGHT)
#define BF_DIAGONAL_ENDTOPLEFT      (BF_DIAGONAL | BF_TOP | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMLEFT   (BF_DIAGONAL | BF_BOTTOM | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMRIGHT  (BF_DIAGONAL | BF_BOTTOM | BF_RIGHT)
#define BF_MIDDLE       0x0800  /* Fill in the middle */
#define BF_SOFT         0x1000  /* For softer buttons */
#define BF_ADJUST       0x2000  /* Calculate the space left over */
#define BF_FLAT         0x4000  /* For flat rather than 3D borders */
#define BF_MONO         0x8000  /* For monochrome borders */
BOOL DrawEdge(HDC hdc,RECT *qrc,BYTE edge,uint16_t grfFlags);
BOOL DrawFocusRect(HDC hDC,const RECT *lprc);
BOOL InvalidateRect(HWND hWnd,const RECT *lpRect,BOOL bErase);
BOOL GetUpdateRect(HWND hWnd,RECT *lpRect,BOOL bErase);
BOOL GetUpdateRgn(HWND hWnd,HRGN hRgn,BOOL bErase);
int DrawIcon(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const ICON icon);

enum {
	DI_COMPAT      = 0x04,
	DI_DEFAULTSIZE = 0x08,
	DI_IMAGE       = 0x02,
	DI_MASK        = 0x01,
	DI_NOMIRROR    = 0x10,
	DI_NORMAL      = DI_IMAGE | DI_MASK
    };
BOOL DrawIconEx(HDC hDC,UGRAPH_COORD_T xLeft,UGRAPH_COORD_T yTop,const ICON hIcon,UGRAPH_COORD_T cxWidth,UGRAPH_COORD_T cyWidth,
  UINT istepIfAniCur,BRUSH hbrFlickerFreeDraw,UINT diFlags);
/* Image type */
#define DST_COMPLEX     0x00
#define DST_TEXT        0x01
#define DST_PREFIXTEXT  0x02
#define DST_ICON        0x03
#define DST_BITMAP      0x04
/* State type */
#define DSS_NORMAL      0x0000
#define DSS_UNION       0x0010  /* Gray string appearance */
#define DSS_DISABLED    0x0020
#define DSS_MONO        0x0080
#define DSS_HIDEPREFIX  0x0200
#define DSS_PREFIXONLY  0x0400
#define DSS_RIGHT       0x8000
typedef BOOL (DRAWSTATEPROC(HDC hdc,LPARAM lData,WPARAM wData,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy));
BOOL DrawState(HDC hdc,BRUSH hbrFore,DRAWSTATEPROC qfnCallBack,LPARAM lData,
  WPARAM wData,UGRAPH_COORD_T x,UGRAPH_COORD_T y,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy,UINT uFlags);
int ShowCursor(BOOL);
static int drawBitmap(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const GFX_COLOR bitmap[],UGRAPH_COORD_T xs, UGRAPH_COORD_T ys);
int PaintDesktop(HDC , int8_t);
BOOL MessageBeep(BYTE uType);
#define MB_OK                 0x0000
#define MB_OKCANCEL           0x0001
#define MB_ABORTRETRYIGNORE   0x0002
#define MB_YESNOCANCEL        0x0003
#define MB_YESNO              0x0004
#define MB_RETRYCANCEL        0x0005
#define MB_CANCELTRYCONTINUE  0x0006
#define MB_ICONEXCLAMATION  0x0030 //	An exclamation-point icon appears in the message box.
#define MB_ICONWARNING		  0x0030 //	An exclamation-point icon appears in the message box.
#define MB_ICONINFORMATION  0x0040  //	An icon consisting of a lowercase letter i in a circle appears in the message box.
#define MB_ICONASTERISK			0x0040  //	An icon consisting of a lowercase letter i in a circle appears in the message box.
#define MB_ICONQUESTION			0x0020  //	A question-mark icon appears in the message box. The question-mark message icon is no longer recommended because it does not clearly represent a specific type of message and because the phrasing of a message as a question could apply to any message type. In addition, users can confuse the message symbol question mark with Help information. Therefore, do not use this question mark message symbol in your message boxes. The system continues to support its inclusion only for backward compatibility.
#define MB_ICONSTOP					0x0010  //	A stop-sign icon appears in the message box.
#define MB_ICONERROR				0x0010  //	A stop-sign icon appears in the message box.
#define MB_ICONHAND					0x0010 
#define MB_DEFBUTTON1       0x0000
#define MB_DEFBUTTON2       0x0100
#define MB_DEFBUTTON3       0x0200
#define MB_DEFBUTTON4       0x0300
#define MB_APPLMODAL        0x0000L
#define MB_SYSTEMMODAL      0x1000L
#define MB_TASKMODAL        0x2000L
#define MB_HELP             0x4000L // Help Button
int MessageBox(HWND hWnd,const char *lpText,const char *lpCaption,WORD uType);
BOOL AnyPopup();

WORD SetTimer(HWND hWnd,WORD nIDEvent,DWORD uElapse,TIMERPROC *lpTimerFunc);
BOOL KillTimer(HWND hWnd,WORD uIDEvent);
#define USER_TIMER_MINIMUM 0x000A
#define USER_TIMER_MAXIMUM 0x7FFFFFFF 
void handleWinTimers(void);

BYTE handle_filechanges(char drive,BYTE mode);


BOOL SetRect(RECT *lprc,UGRAPH_COORD_T xLeft,UGRAPH_COORD_T yTop,UGRAPH_COORD_T xRight,UGRAPH_COORD_T yBottom);
BOOL SetRectEmpty(RECT *lprc);
BOOL UnionRect(RECT *lprcDst,const RECT *lprcSrc1,const RECT *lprcSrc2);
void inline OffsetRect(RECT *lprc,int16_t dx,int16_t dy);
void inline OffsetRect2(RECT *lprcd,RECT *lprcs,int16_t dx,int16_t dy);
void inline OffsetPoint(POINT *lprc,int16_t dx,int16_t dy);
BOOL IntersectRect(RECT *lprcDst,const RECT *lprcSrc1,const RECT *lprcSrc2);
void InflateRect(RECT *lprc,int dx,int dy);
BOOL CopyRect(RECT *lprcDst,const RECT *lprcSrc);
BOOL ValidateRect(HWND hWnd,const RECT *lpRect);
BOOL ValidateRgn(HWND hWnd,HRGN hRgn);


#define MN_GETHMENU     0x01E1
#define MSGF_DIALOGBOX 1    // era 0 ma io uso 0 per indicare "niente" (ovviamente...)
#define MSGF_MENU 2
#define MSGF_MOVE 3     // mio!
#define MSGF_SIZE 4     // mio!

#define SC_SIZE         0xF000
#define SC_SEPARATOR    0xf00f
#define SC_MOVE         0xF010
#define SC_MINIMIZE     0xF020
#define SC_MAXIMIZE     0xF030
#define SC_NEXTWINDOW   0xF040
#define SC_PREVWINDOW   0xF050
#define SC_CLOSE        0xF060
#define SC_VSCROLL      0xF070
#define SC_HSCROLL      0xF080
#define SC_MOUSEMENU    0xF090
#define SC_KEYMENU      0xF100
#define SC_ARRANGE      0xF110
#define SC_RESTORE      0xF120
#define SC_TASKLIST     0xF130
#define SC_SCREENSAVE   0xF140
#define SC_HOTKEY       0xF150
#define SC_DEFAULT      0xF160
#define SC_MONITORPOWER 0xF170
#define SC_CONTEXTHELP  0xF180
#define SCF_ISSECURE 0x0001
BOOL SetMenu(HWND hWnd,HMENU Menu);
HMENU GetMenu(HWND hWnd);
#define OBJID_CLIENT ((LONG)0xFFFFFFFC)
#define OBJID_MENU ((LONG)0xFFFFFFFD)
#define OBJID_SYSMENU ((LONG)0xFFFFFFFF)
typedef struct __attribute((packed)) {
//  DWORD cbSize;
  RECT  rcBar;
  HMENU hMenu;
  HWND  hwndMenu;
  BOOL  fBarFocused : 1;
  BOOL  fFocused : 1;
  BOOL  fUnused : 6;
  } MENUBARINFO;
BOOL GetMenuBarInfo(HWND hwnd,LONG idObject,uint16_t idItem,MENUBARINFO *pmbi);
typedef struct __attribute((packed)) {
//  UINT      cbSize;
  uint16_t fMask;
  uint16_t fType;
  uint16_t fState;
  uint16_t wID;
  MENU     hSubMenu;
  BITMAP   hbmpChecked;
  BITMAP   hbmpUnchecked;
  unsigned long *dwItemData;
  char     *dwTypeData;
  UINT     cch;
  BITMAP   hbmpItem;
  } MENUITEMINFO;
typedef struct {
//  DWORD     cbSize;
  uint16_t  fMask;
  uint16_t  dwStyle;
  uint16_t  cyMax;
  BRUSH     hbrBack;
  DWORD     dwContextHelpID;
  DWORD     dwMenuData;
  } MENUINFO;
BOOL SetMenuItemInfo(HMENU hMenu,uint16_t item,BOOL fByPositon,MENUITEMINFO *lpmii);
BOOL GetMenuInfo(HMENU,MENUINFO *);
int GetMenuItemCount(HMENU Menu);
uint16_t GetMenuItemID(HMENU Menu,BYTE nPos);
MENUITEM *GetMenuItemFromCommand(HMENU Menu,uint16_t cmd);      // https://www.codeguru.com/cplusplus/finding-a-menuitem-from-command-id/
BYTE MenuItemFromPoint(HWND  hWnd,HMENU hMenu,POINT ptScreen);
uint16_t GetMenuDefaultItem(HMENU hMenu,BYTE fByPos,uint16_t gmdiFlags);
BOOL SetMenuDefaultItem(HMENU hMenu,uint16_t uItem,BYTE fByPos);
UGRAPH_COORD_T GetMenuCheckMarkDimensions(void);
BOOL EnableMenuItem(HMENU hMenu,uint16_t uIDEnableItem,BYTE uEnable);
DWORD CheckMenuItem(HMENU hMenu,uint16_t uIDEnableItem,uint16_t uCheck);
BOOL AppendMenuA(HMENU Menu,uint16_t uFlags,uint16_t uIDNewItem,const char *lpNewItem);
BOOL ModifyMenu(HMENU Menu,uint16_t uPosition,uint16_t uFlags,uint16_t uIDNewItem,const char *lpNewItem);
HMENU CreatePopupMenu(HMENU);     // no memoria dinamica ;)
BOOL DestroyMenu(HMENU Menu);
BOOL DeleteMenu(HMENU Menu,BYTE uPosition,uint16_t uFlags);
BOOL InsertMenu(HMENU Menu,uint16_t uPosition,uint16_t uFlags,uint16_t uIDNewItem,const char *lpNewItem);
BOOL InsertMenuItem(HMENU Menu,UINT item,BOOL fByPosition,MENUITEMINFO *lpmi);
BOOL DrawMenuBar(HWND hWnd);
enum __attribute__ ((__packed__)) {
	TPM_CENTERALIGN     = 0x0004,
	TPM_LEFTALIGN       = 0x0000,
	TPM_RIGHTALIGN      = 0x0008,
	TPM_BOTTOMALIGN     = 0x0020,
	TPM_TOPALIGN        = 0x0000,
	TPM_VCENTERALIGN    = 0x0010,
	TPM_NONOTIFY        = 0x0080,
	TPM_RETURNCMD       = 0x0100,
	TPM_LEFTBUTTON      = 0x0000,
	TPM_RIGHTBUTTON     = 0x0002,
	TPM_HORNEGANIMATION = 0x0800,
	TPM_HORPOSANIMATION = 0x0400,
	TPM_NOANIMATION     = 0x4000,
	TPM_VERNEGANIMATION = 0x2000,
	TPM_VERPOSANIMATION = 0x1000,
	TPM_HORIZONTAL      = 0x0000,
	TPM_VERTICAL        = 0x0040
};
typedef struct tagTPMPARAMS {
//  UINT cbSize;
  RECT rcExclude;
} TPMPARAMS;
BOOL TrackPopupMenu(HMENU hMenu,UINT uFlags,UGRAPH_COORD_T x,UGRAPH_COORD_T y,int reserved,HWND hwnd,const RECT *prcRect);
BOOL TrackPopupMenuEx(HMENU hMenu,UINT uFlags,UGRAPH_COORD_T x,UGRAPH_COORD_T y,HWND hwnd,TPMPARAMS *lptpm);
typedef ACCELERATOR *HACCEL;
int TranslateAccelerator(HWND hWnd,HACCEL hAccTable,MSG *lpMsg);
HACCEL LoadAccelerators(HINSTANCE hInstance,LPCSTR lpTableName);
HMENU GetSystemMenu(HWND hWnd,BOOL bRevert);
HMENU GetActiveMenu(void);
HMENU GetSubMenu(HMENU hMenu,uint16_t nPos);
int GetMenuString(HMENU hMenu,UINT uIDItem,LPSTR lpString,uint16_t cchMax,UINT flags);

BOOL RegisterHotKey(HWND hWnd,uint16_t id,BYTE fsModifiers,BYTE vk);
BOOL UnregisterHotKey(HWND hWnd,uint16_t id);


/* GDI  https://github.com/tpn/winsdk-7/blob/master/v7.1A/Include/WinGDI.h */
/* Ternary raster operations; per motivi ignoti ci son 2 set di define... io le unisco e fottetevi! */
#ifndef TERNARY_OP_DEFINED
#define R2_COPYPEN          0   /* P        */ /* preferisco così ;) */
#define R2_BLACK            1   /*  0       */
#define R2_NOTMERGEPEN      2   /* DPon     */
#define R2_MASKNOTPEN       3   /* DPna     */
#define R2_NOTCOPYPEN       4   /* PN       */
#define R2_MASKPENNOT       5   /* PDna     */
#define R2_NOT              6   /* Dn       */
#define R2_XORPEN           7   /* DPx      */
#define R2_NOTMASKPEN       8   /* DPan     */
#define R2_MASKPEN          9   /* DPa      */
#define R2_NOTXORPEN        10  /* DPxn     */
#define R2_NOP              11  /* D        */
#define R2_MERGENOTPEN      12  /* DPno     */
#define R2_MERGEPENNOT      13  /* PDno     */
#define R2_MERGEPEN         14  /* DPo      */
#define R2_WHITE            15  /*  1       */
#define R2_LAST             15
enum TERNARY_OP {       // FINIRE!
    /* Ternary raster operations */
    SRCCOPY=R2_COPYPEN    ,//0x00CC0020 /* dest = source                   */
    SRCPAINT=R2_MERGEPEN  ,//0x00EE0086 /* dest = source OR dest           */
    SRCAND=R2_MASKPEN     ,//0x008800C6 /* dest = source AND dest          */
    SRCINVERT=R2_XORPEN   ,//0x00660046 /* dest = source XOR dest          */
    SRCERASE=R2_MASKPENNOT,//0x00440328 /* dest = source AND (NOT dest )   */
    NOTSRCCOPY=R2_NOTCOPYPEN,//0x00330008 /* dest = (NOT source)             */
    NOTSRCERASE=R2_NOTMERGEPEN,//0x001100A6 /* dest = (NOT src) AND (NOT dest) */
    MERGECOPY=R2_MERGEPEN     ,//0x00C000CA /* dest = (source AND pattern)     */
    MERGEPAINT=R2_XORPEN      ,//0x00BB0226 /* dest = (NOT source) OR dest     */
    PATCOPY             ,//0x00F00021 /* dest = pattern                  */
    PATPAINT            ,//0x00FB0A09 /* dest = DPSnoo                   */
    PATINVERT=R2_XORPEN ,//0x005A0049 /* dest = pattern XOR dest         */
    DSTINVERT=R2_NOT           ,//0x00550009 /* dest = (NOT dest)               */
    BLACKNESS=R2_BLACK           ,//0x00000042 /* dest = BLACK                    */
    WHITENESS=R2_WHITE           ,//0x00FF0062 /* dest = WHITE                    */
    NOMIRRORBITMAP      ,//0x80000000 /* Do not Mirror the bitmap in this call */
    CAPTUREBLT          ,//0x40000000 /* Include layered windows */
    };
#define TERNARY_OP_DEFINED
#endif

/* CombineRgn() Styles */
#define RGN_AND             1
#define RGN_OR              2
#define RGN_XOR             3
#define RGN_DIFF            4
#define RGN_COPY            5
#define RGN_MIN             RGN_AND
#define RGN_MAX             RGN_COPY

/* Device Parameters for GetDeviceCaps() */
#define DRIVERVERSION 0     /* Device driver version                    */
#define TECHNOLOGY    2     /* Device classification                    */
#define HORZSIZE      4     /* Horizontal size in millimeters           */
#define VERTSIZE      6     /* Vertical size in millimeters             */
#define HORZRES       8     /* Horizontal width in pixels               */
#define VERTRES       10    /* Vertical height in pixels                */
#define BITSPIXEL     12    /* Number of bits per pixel                 */
#define PLANES        14    /* Number of planes                         */
#define NUMBRUSHES    16    /* Number of brushes the device has         */
#define NUMPENS       18    /* Number of pens the device has            */
#define NUMMARKERS    20    /* Number of markers the device has         */
#define NUMFONTS      22    /* Number of fonts the device has           */
#define NUMCOLORS     24    /* Number of colors the device supports     */
#define PDEVICESIZE   26    /* Size required for device descriptor      */
#define CURVECAPS     28    /* Curve capabilities                       */
#define LINECAPS      30    /* Line capabilities                        */
#define POLYGONALCAPS 32    /* Polygonal capabilities                   */
#define TEXTCAPS      34    /* Text capabilities                        */
#define CLIPCAPS      36    /* Clipping capabilities                    */
#define RASTERCAPS    38    /* Bitblt capabilities                      */
#define ASPECTX       40    /* Length of the X leg                      */
#define ASPECTY       42    /* Length of the Y leg                      */
#define ASPECTXY      44    /* Length of the hypotenuse                 */

#define LOGPIXELSX    88    /* Logical pixels/inch in X                 */
#define LOGPIXELSY    90    /* Logical pixels/inch in Y                 */

#define SIZEPALETTE  104    /* Number of entries in physical palette    */
#define NUMRESERVED  106    /* Number of reserved entries in palette    */
#define COLORRES     108    /* Actual color resolution                  */

// Printing related DeviceCaps. These replace the appropriate Escapes
#define PHYSICALWIDTH   110 /* Physical Width in device units           */
#define PHYSICALHEIGHT  111 /* Physical Height in device units          */
#define PHYSICALOFFSETX 112 /* Physical Printable Area x margin         */
#define PHYSICALOFFSETY 113 /* Physical Printable Area y margin         */
#define SCALINGFACTORX  114 /* Scaling factor x                         */
#define SCALINGFACTORY  115 /* Scaling factor y                         */

// Display driver specific
#define VREFRESH        116  /* Current vertical refresh rate of the    */
                             /* display device (for displays only) in Hz*/
#define DESKTOPVERTRES  117  /* Horizontal width of entire desktop in pixels                                  */
#define DESKTOPHORZRES  118  /* Vertical height of entire desktop in pixels                                  */
#define BLTALIGNMENT    119  /* Preferred blt alignment                 */
#define SHADEBLENDCAPS  120  /* Shading and blending caps               */
#define COLORMGMTCAPS   121  /* Color Management caps                   */


typedef struct __attribute((packed)) {
  BYTE      CtlType;
  BYTE      CtlID;
  BYTE      itemID;
  BYTE      itemAction;
  HWND      hwndItem;
  HDC       hDC;
  RECT      rcItem;
  DWORD    *itemData;
  uint16_t  itemState;
    } DRAWITEMSTRUCT;
typedef struct __attribute((packed)) {
  BYTE      CtlType;
  BYTE      CtlID;
  BYTE      itemID;
  UGRAPH_COORD_T itemWidth;
  UGRAPH_COORD_T itemHeight;
  DWORD     *itemData;
    } MEASUREITEMSTRUCT;
typedef struct __attribute((packed)) {
  BYTE      CtlType;
  BYTE      CtlID;
  HWND      hwndItem;
  uint16_t  itemID1;
  DWORD     *itemData1;
  uint16_t  itemID2;
  DWORD     *itemData2;
  DWORD     dwLocaleId;
} COMPAREITEMSTRUCT;

/* * Owner draw control types */
#define ODT_MENU        1
#define ODT_LISTBOX     2
#define ODT_COMBOBOX    3
#define ODT_BUTTON      4
#define ODT_STATIC      5

/* * Owner draw actions */
#define ODA_DRAWENTIRE  0x01
#define ODA_SELECT      0x02
#define ODA_FOCUS       0x04

/* * Owner draw state */
#define ODS_SELECTED    0x0001
#define ODS_GRAYED      0x0002
#define ODS_DISABLED    0x0004
#define ODS_CHECKED     0x0008
#define ODS_FOCUS       0x0010
#define ODS_DEFAULT     0x0020
#define ODS_COMBOBOXEDIT    0x1000
#define ODS_HOTLIGHT    0x0040
#define ODS_INACTIVE    0x0080
#define ODS_NOACCEL     0x0100
#define ODS_NOFOCUSRECT 0x0200



#define MK_CONTROL  0x08
#define MK_LBUTTON  0x01
#define MK_MBUTTON  0x10
#define MK_RBUTTON  0x02
#define MK_SHIFT    0x04
#define MK_XBUTTON1 0x20
#define MK_XBUTTON2 0x40

/* * Virtual Keys, Standard Set */
#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_CANCEL         0x03
#define VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */

#define VK_BACK           0x08
#define VK_TAB            0x09

#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D

#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14

#define VK_KANA           0x15
#define VK_HANGEUL        0x15  /* old name - should be here for compatibility */
#define VK_HANGUL         0x15
#define VK_JUNJA          0x17
#define VK_FINAL          0x18
#define VK_HANJA          0x19
#define VK_KANJI          0x19

#define VK_ESCAPE         0x1B

#define VK_CONVERT        0x1C
#define VK_NONCONVERT     0x1D
#define VK_ACCEPT         0x1E
#define VK_MODECHANGE     0x1F

#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
#define VK_A 0x41
#define VK_B 0x42
#define VK_E 0x45
#define VK_H 0x48
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_N 0x4E
#define VK_O 0x4F
#define VK_R 0x52
#define VK_S 0x53
#define VK_T 0x54
#define VK_U 0x55
#define VK_V 0x56
#define VK_W 0x57
#define VK_X 0x58
#define VK_Y 0x59
#define VK_Z 0x5A
#define VK_0 0x30
#define VK_1 0x31
#define VK_2 0x32
#define VK_3 0x33
#define VK_4 0x34
#define VK_5 0x35
#define VK_6 0x36
#define VK_7 0x37
#define VK_8 0x38
#define VK_9 0x39

#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D

#define VK_SLEEP          0x5F

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87

#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
SHORT GetKeyState(int nVirtKey);
SHORT GetAsyncKeyState(int vKey);
BOOL SetKeyboardState(BYTE *lpKeyState);
BOOL GetKeyboardState(BYTE *lpKeyState);
SHORT VkKeyScan(char ch);
SHORT VkKeyScanEx(char ch,DWORD /*HKL*/ dwhkl);
UINT MapVirtualKey(UINT uCode,BYTE uMapType);
enum {
	MAPVK_VK_TO_VSC = 0x00,
	MAPVK_VSC_TO_VK = 0x01,
	MAPVK_VK_TO_CHAR = 0x02,
	MAPVK_VSC_TO_VK_EX = 0x03,
	MAPVK_VK_TO_VSC_EX = 0x04
    };
    
#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5

#define VK_PROCESSKEY     0xE5

#define VK_BROWSER_BACK        0xA6
#define VK_BROWSER_FORWARD     0xA7
#define VK_BROWSER_REFRESH     0xA8
#define VK_BROWSER_STOP        0xA9
#define VK_BROWSER_SEARCH      0xAA
#define VK_BROWSER_FAVORITES   0xAB
#define VK_BROWSER_HOME        0xAC

#define VK_VOLUME_MUTE         0xAD
#define VK_VOLUME_DOWN         0xAE
#define VK_VOLUME_UP           0xAF
#define VK_MEDIA_NEXT_TRACK    0xB0
#define VK_MEDIA_PREV_TRACK    0xB1
#define VK_MEDIA_STOP          0xB2
#define VK_MEDIA_PLAY_PAUSE    0xB3
#define VK_LAUNCH_MAIL         0xB4
#define VK_LAUNCH_MEDIA_SELECT 0xB5
#define VK_LAUNCH_APP1         0xB6
#define VK_LAUNCH_APP2         0xB7

#define VK_OEM_1          0xBA   // ';:' for US
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define VK_OEM_2          0xBF   // '/?' for US
#define VK_OEM_3          0xC0   // '`~' for US


#define VK_GAMEPAD_A                         0xC3
#define VK_GAMEPAD_B                         0xC4
#define VK_GAMEPAD_X                         0xC5
#define VK_GAMEPAD_Y                         0xC6
#define VK_GAMEPAD_RIGHT_SHOULDER            0xC7
#define VK_GAMEPAD_LEFT_SHOULDER             0xC8
#define VK_GAMEPAD_LEFT_TRIGGER              0xC9
#define VK_GAMEPAD_RIGHT_TRIGGER             0xCA
#define VK_GAMEPAD_DPAD_UP                   0xCB
#define VK_GAMEPAD_DPAD_DOWN                 0xCC
#define VK_GAMEPAD_DPAD_LEFT                 0xCD
#define VK_GAMEPAD_DPAD_RIGHT                0xCE
#define VK_GAMEPAD_MENU                      0xCF
#define VK_GAMEPAD_VIEW                      0xD0
#define VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON    0xD1
#define VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON   0xD2
#define VK_GAMEPAD_LEFT_THUMBSTICK_UP        0xD3
#define VK_GAMEPAD_LEFT_THUMBSTICK_DOWN      0xD4
#define VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT     0xD5
#define VK_GAMEPAD_LEFT_THUMBSTICK_LEFT      0xD6
#define VK_GAMEPAD_RIGHT_THUMBSTICK_UP       0xD7
#define VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN     0xD8
#define VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT    0xD9
#define VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT     0xDA

#define VK_OEM_4          0xDB  //  '[{' for US
#define VK_OEM_5          0xDC  //  '\|' for US
#define VK_OEM_6          0xDD  //  ']}' for US
#define VK_OEM_7          0xDE  //  ''"' for US
#define VK_OEM_8          0xDF

#define VK_OEM_AX         0xE1  //  'AX' key on Japanese AX kbd
#define VK_OEM_102        0xE2  //  "<>" or "\|" on RT 102-key kbd.
#define VK_ICO_HELP       0xE3  //  Help key on ICO
#define VK_ICO_00         0xE4  //  00 key on ICO

#define VK_PROCESSKEY     0xE5
#define VK_ICO_CLEAR      0xE6
#define VK_PACKET         0xE7

#define VK_ATTN           0xF6
#define VK_CRSEL          0xF7
#define VK_EXSEL          0xF8
#define VK_EREOF          0xF9
#define VK_PLAY           0xFA
#define VK_ZOOM           0xFB
#define VK_NONAME         0xFC
#define VK_PA1            0xFD
#define VK_OEM_CLEAR      0xFE


#define WC_STATIC       MAKEFOURCC('S','T','T','C')
#define WC_BUTTON 	MAKEFOURCC('B','T','N',' ')
#define WC_COMBOBOX 	MAKEFOURCC('C','M','B','X')
#define DropDown 	  	  	MAKEFOURCC('D','R','D','N')
#define WC_EDIT 	MAKEFOURCC('E','D','I','T')
#define ImageDrag 	  	  	MAKEFOURCC('E','D','I','T')
#define WC_DIALOG       MAKEFOURCC('D','L','G',' ')
#define WC_LISTBOX 	MAKEFOURCC('L','B','O','X')
#define WC_HOTKEY 	MAKEFOURCC('H','O','T','K')
#define WC_PROGRESS 	MAKEFOURCC('P','R','O','G')
#define WC_STATUSBAR 	MAKEFOURCC('S','T','A','T')
#define WC_TRACKBAR 	MAKEFOURCC('T','R','C','K')
#define WC_SLIDER 	MAKEFOURCC('S','L','I','D')
#define WC_UPDOWN    	MAKEFOURCC('U','P','D','N')     // io lo faccio con il text incorporato!
#define WC_NATIVEFONTCTL 	  	MAKEFOURCC('E','D','I','T')
#define ReaderModeCtl 	  	  	MAKEFOURCC('E','D','I','T')
#define REBARCLASSNAME 	MAKEFOURCC('E','D','I','T')
#define WC_SCROLLBAR 	MAKEFOURCC('S','C','R','L')
#define ANIMATE_CLASS 	MAKEFOURCC('E','D','I','T')
#define DATETIMEPICK_CLASS 	MAKEFOURCC('E','D','I','T')
#define WC_HEADER 	MAKEFOURCC('E','D','I','T')
#define WC_IPADDRESS 	MAKEFOURCC('E','D','I','T')
#define WC_LINK 	MAKEFOURCC('E','D','I','T')
#define WC_LISTVIEW 	MAKEFOURCC('L','S','T','V')
#define MONTHCAL_CLASS 	MAKEFOURCC('E','D','I','T')
#define WC_PAGESCROLLER 	MAKEFOURCC('E','D','I','T')
#define WC_TABCONTROL 	MAKEFOURCC('T','A','B','C')
#define WC_TREEVIEW 	MAKEFOURCC('T','R','E','E')
#define WC_TOOLBAR   	MAKEFOURCC('T','O','O','L')
#define TOOLTIPS_CLASS  MAKEFOURCC('T','T','I','P')
#define WC_FILEDLG   	MAKEFOURCC('D','I','R','D')
#define WC_DIRCLASS   	MAKEFOURCC('D','S','K','D')
#define WC_CMDSHELL   	MAKEFOURCC('C','M','D','S')
#define WC_DEFAULTCLASS MAKEFOURCC(0,0,0,0)
#define WC_DESKTOPCLASS MAKEFOURCC(1,0,0,0)         //When Windows first starts, the desktop's Class is "Progman"
#define WC_TASKBARCLASS MAKEFOURCC('T','S','K','B')
#define WC_CONTROLPANELCLASS MAKEFOURCC('C','T','L','P')
#define WC_TASKMANCLASS MAKEFOURCC('T','S','K','M')
#define WC_VIDEOCAPCLASS MAKEFOURCC('V','C','A','P')
#define WC_VIEWERCLASS 	MAKEFOURCC('V','I','E','W')
#define WC_PLAYERCLASS 	MAKEFOURCC('P','L','A','Y')



/*
 * Dialog Box Command IDs
 */
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7
#define IDCLOSE         8
#define IDHELP          9



/*
 * Control Manager Structures and Definitions
 */

/*
 * Edit Control Styles
 */
#define ES_LEFT             0x0000L
#define ES_CENTER           0x0001L
#define ES_RIGHT            0x0002L
#define ES_MULTILINE        0x0004L
#define ES_UPPERCASE        0x0008L
#define ES_LOWERCASE        0x0010L
#define ES_PASSWORD         0x0020L
#define ES_AUTOVSCROLL      0x0040L
#define ES_AUTOHSCROLL      0x0080L
#define ES_NOHIDESEL        0x0100L
#define ES_OEMCONVERT       0x0400L
#define ES_READONLY         0x0800L
#define ES_WANTRETURN       0x1000L
#define ES_NUMBER           0x2000L


/*
 * Edit Control Notification Codes
 */
#define EN_SETFOCUS         0x0100
#define EN_KILLFOCUS        0x0200
#define EN_CHANGE           0x0300
#define EN_UPDATE           0x0400
#define EN_ERRSPACE         0x0500
#define EN_MAXTEXT          0x0501
#define EN_HSCROLL          0x0601
#define EN_VSCROLL          0x0602

/* Edit control EM_SETMARGIN parameters */
#define EC_LEFTMARGIN       0x0001
#define EC_RIGHTMARGIN      0x0002
#define EC_USEFONTINFO      0xffff

/* wParam of EM_GET/SETIMESTATUS  */
#define EMSIS_COMPOSITIONSTRING        0x0001

/* lParam for EMSIS_COMPOSITIONSTRING  */
#define EIMES_GETCOMPSTRATONCE         0x0001
#define EIMES_CANCELCOMPSTRINFOCUS     0x0002
#define EIMES_COMPLETECOMPSTRKILLFOCUS 0x0004


/*
 * Edit Control Messages
 */
#define EM_GETSEL               0x00B0
#define EM_SETSEL               0x00B1
#define EM_GETRECT              0x00B2
#define EM_SETRECT              0x00B3
#define EM_SETRECTNP            0x00B4
#define EM_SCROLL               0x00B5
#define EM_LINESCROLL           0x00B6
#define EM_SCROLLCARET          0x00B7
#define EM_GETMODIFY            0x00B8
#define EM_SETMODIFY            0x00B9
#define EM_GETLINECOUNT         0x00BA
#define EM_LINEINDEX            0x00BB
#define EM_SETHANDLE            0x00BC
#define EM_GETHANDLE            0x00BD
#define EM_GETTHUMB             0x00BE
#define EM_LINELENGTH           0x00C1
#define EM_REPLACESEL           0x00C2
#define EM_GETLINE              0x00C4
#define EM_LIMITTEXT            0x00C5
#define EM_CANUNDO              0x00C6
#define EM_UNDO                 0x00C7
#define EM_FMTLINES             0x00C8
#define EM_LINEFROMCHAR         0x00C9
#define EM_SETTABSTOPS          0x00CB
#define EM_SETPASSWORDCHAR      0x00CC
#define EM_EMPTYUNDOBUFFER      0x00CD
#define EM_GETFIRSTVISIBLELINE  0x00CE
#define EM_SETREADONLY          0x00CF
#define EM_SETWORDBREAKPROC     0x00D0
#define EM_GETWORDBREAKPROC     0x00D1
#define EM_GETPASSWORDCHAR      0x00D2
#define EM_SETMARGINS           0x00D3
#define EM_GETMARGINS           0x00D4
#define EM_SETLIMITTEXT         EM_LIMITTEXT   /* ;win40 Name change */
#define EM_GETLIMITTEXT         0x00D5
#define EM_POSFROMCHAR          0x00D6
#define EM_CHARFROMPOS          0x00D7
#define EM_TAKEFOCUS            0x00D7

#define EM_SETIMESTATUS         0x00D8
#define EM_GETIMESTATUS         0x00D9
#define EM_SETEVENTMASK         (WM_USER + 69)

/*
 * EDITWORDBREAKPROC code values
 */
#define WB_LEFT            0
#define WB_RIGHT           1
#define WB_ISDELIMITER     2


/*
 * Button Control Styles
 */
#define BS_PUSHBUTTON       0x00000000L
#define BS_DEFPUSHBUTTON    0x00000001L
#define BS_CHECKBOX         0x00000002L
#define BS_AUTOCHECKBOX     0x00000003L
#define BS_RADIOBUTTON      0x00000004L
#define BS_3STATE           0x00000005L
#define BS_AUTO3STATE       0x00000006L
#define BS_GROUPBOX         0x00000007L
#define BS_USERBUTTON       0x00000008L
#define BS_AUTORADIOBUTTON  0x00000009L
#define BS_OWNERDRAW        0x0000000BL
#define BS_LEFTTEXT         0x00000020L
#define BS_TEXT             0x00000000L
#define BS_ICON             0x00000040L
#define BS_BITMAP           0x00000080L
#define BS_LEFT             0x00000100L
#define BS_RIGHT            0x00000200L
#define BS_CENTER           0x00000300L
#define BS_TOP              0x00000400L
#define BS_BOTTOM           0x00000800L
#define BS_VCENTER          0x00000C00L
#define BS_PUSHLIKE         0x00001000L
#define BS_MULTILINE        0x00002000L
#define BS_NOTIFY           0x00004000L
#define BS_FLAT             0x00008000L
#define BS_RIGHTBUTTON      BS_LEFTTEXT

/*
 * User Button Notification Codes
 */
#define BN_CLICKED          0
#define BN_PAINT            1
#define BN_HILITE           2
#define BN_UNHILITE         3
#define BN_DISABLE          4
#define BN_DOUBLECLICKED    5
#define BN_PUSHED           BN_HILITE
#define BN_UNPUSHED         BN_UNHILITE
#define BN_DBLCLK           BN_DOUBLECLICKED
#define BN_SETFOCUS         6
#define BN_KILLFOCUS        7

/*
 * Button Control Messages
 */
#define BM_GETCHECK        0x00F0
#define BM_SETCHECK        0x00F1
#define BM_GETSTATE        0x00F2
#define BM_SETSTATE        0x00F3
#define BM_SETSTYLE        0x00F4
#define BM_CLICK           0x00F5
#define BM_GETIMAGE        0x00F6
#define BM_SETIMAGE        0x00F7

#define BST_UNCHECKED      0x0000
#define BST_CHECKED        0x0001
#define BST_INDETERMINATE  0x0002
#define BST_PUSHED         0x0004
#define BST_FOCUS          0x0008

/*
 * Static Control Constants
 */
#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L
#define SS_GRAYRECT         0x00000005L
#define SS_WHITERECT        0x00000006L
#define SS_BLACKFRAME       0x00000007L
#define SS_GRAYFRAME        0x00000008L
#define SS_WHITEFRAME       0x00000009L
#define SS_USERITEM         0x0000000AL
#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL
#define SS_OWNERDRAW        0x0000000DL
#define SS_BITMAP           0x0000000EL
#define SS_ENHMETAFILE      0x0000000FL
#define SS_ETCHEDHORZ       0x00000010L
#define SS_ETCHEDVERT       0x00000011L
#define SS_ETCHEDFRAME      0x00000012L
#define SS_TYPEMASK         0x0000001FL
#define SS_NOPREFIX         0x00000080L /* Don't do "&" character translation */
#define SS_NOTIFY           0x00000100L
#define SS_CENTERIMAGE      0x00000200L
#define SS_RIGHTJUST        0x00000400L
#define SS_REALSIZEIMAGE    0x00000800L
#define SS_SUNKEN           0x00001000L
#define SS_ENDELLIPSIS      0x00004000L
#define SS_PATHELLIPSIS     0x00008000L
#define SS_WORDELLIPSIS     0x0000C000L
#define SS_ELLIPSISMASK     0x0000C000L

/*
 * Static Control Messages
 */
#define STM_SETICON         0x0170
#define STM_GETICON         0x0171
#define STM_SETIMAGE        0x0172
#define STM_GETIMAGE        0x0173
#define STN_CLICKED         0
#define STN_DBLCLK          1
#define STN_ENABLE          2
#define STN_DISABLE         3
#define STM_MSGMAX          0x0174

/*
 * Dialog window class
 */
//#define WC_DIALOG       (MAKEINTATOM(0x8002)) v. la mia ;)

/*
 * Get/SetWindowWord/Long offsets for use with WC_DIALOG windows
 */
#define DWL_MSGRESULT   0
#define DWLP_MSGRESULT   
#define DWL_DLGPROC     -24
#define DWLP_DLGPROC    
#define DWL_INTERNAL    4       // il primo byte è control che ha il focus; il 2° quale control è DEFPUSHBUTTON
#define DWL_USER        8
#define DWLP_USER       
#define DLGWINDOWEXTRA 30           // 48 su Mac?? ! e cmq qua... 

typedef struct _DLGITEMTEMPLATE {
  CLASS class;
  DWORD style;
//  DWORD dwExtendedStyle;
  uint16_t x;
  uint16_t y;
  uint16_t cx;
  uint16_t cy;
  char *caption;          // mia estensione...
  WORD  id;
//  struct _DLGITEMTEMPLATE *item;
} DLGITEMTEMPLATE;
typedef struct {
  DWORD style;
//  DWORD dwExtendedStyle;
  WORD  cdit;
  uint16_t x;
  uint16_t y;
  uint16_t cx;
  uint16_t cy;
  FONT font;                   // idem
  char *caption;          // mia estensione...
  DLGITEMTEMPLATE *item[];
} DLGTEMPLATE;

/*
 * Dialog Manager Routines
 */
typedef LRESULT (DIALOGPROC(void *,uint16_t,WPARAM,LPARAM));
int DialogBox(HINSTANCE hInstance,const void *lpTemplate,HWND hWndParent,DIALOGPROC *lpDialogFunc);
#define DialogBoxIndirect(hInstance,lpTemplate,hWndParent,lpDialogFunc) DialogBox(hInstance,lpTemplate,hWndParent,lpDialogFunc)
int DialogBoxParam(HINSTANCE hInstance,const void *lpTemplate,HWND hWndParent,DIALOGPROC *lpDialogFunc,LPARAM dwInitParam);
#define DialogBoxIndirectParam(hInstance,lpTemplate,hWndParent,lpDialogFunc,dwInitParam) DialogBoxParam(hInstance,lpTemplate,hWndParent,lpDialogFunc,dwInitParam)
// faccio define perché qua template e indirect sonoi lo stesso
BOOL EndDialog(HWND hDlg,int nResult);
HWND CreateDialog(HINSTANCE hInstance,const char *lpName,HWND hWndParent,DIALOGPROC *lpDialogFunc);
#define CreateDialogIndirect(hInstance,lpName,hWndParent,lpDialogFunc) CreateDialog(hInstance,lpName,hWndParent,lpDialogFunc)
HWND CreateDialogParam(HINSTANCE hInstance,const char *lpName,HWND hWndParent,DIALOGPROC *lpDialogFunc,LPARAM dwInitParam);
#define CreateDialogIndirectParam(hInstance,lpName,hWndParent,lpDialogFunc,dwInitParam) CreateDialogParam(hInstance,lpName,hWndParent,lpDialogFunc,dwInitParam)
LRESULT DefDlgProc(HWND hDlg,uint16_t Msg,WPARAM wParam,LPARAM lParam);
//BOOL IsDialogMessage(HWND hDlg, LPMSG lpMsg);
HWND GetDlgItem(HWND hDlg,uint16_t nIDDlgItem);
uint16_t GetDlgCtrlID(HWND hWnd);
UINT SetDlgItemInt(HWND hDlg,uint16_t nIDDlgItem,UINT uValue,BOOL bSigned);
UINT GetDlgItemInt(HWND hDlg,uint16_t nIDDlgItem,BOOL *lpTranslated,BOOL bSigned);
UINT SetDlgItemText(HWND hDlg,uint16_t nIDDlgItem,const char *lpString);
UINT GetDlgItemText(HWND hDlg,uint16_t nIDDlgItem,char *lpString,uint16_t cchMax);
BOOL CheckDlgButton(HWND hDlg,uint16_t nIDButton,BYTE uCheck);
BOOL CheckRadioButton(HWND hDlg,uint16_t nIDFirstButton,uint16_t nIDLastButton,uint16_t nIDCheckButton);
UINT IsDlgButtonChecked(HWND hDlg,uint16_t nIDButton);
LRESULT SendDlgItemMessage(HWND hDlg,uint16_t nIDDlgItem,UINT Msg,WPARAM wParam,LPARAM lParam);
BOOL MapDialogRect(HWND hDlg, RECT *lpRect);
HWND GetNextDlgTabItem(HWND hDlg,HWND hCtl,BOOL bPrevious);
HWND GetNextDlgGroupItem(HWND hDlg,HWND hCtl,BOOL bPrevious);

/*
 * DlgDirList, DlgDirListComboBox flags values
 */
#define DDL_READWRITE       0x0000
#define DDL_READONLY        0x0001
#define DDL_HIDDEN          0x0002
#define DDL_SYSTEM          0x0004
#define DDL_DIRECTORY       0x0010
#define DDL_ARCHIVE         0x0020

#define DDL_POSTMSGS        0x2000
#define DDL_DRIVES          0x4000
#define DDL_EXCLUSIVE       0x8000

typedef struct __attribute((packed)) _FILETIME {        // v. anche FAT timestamp
  WORD dwLowDateTime;
  WORD dwHighDateTime;
  } FILETIME;
typedef struct __attribute((packed)) _SYSTEMTIME {      // v. anche gTimeCrtTime ecc
  WORD wYear;
  WORD wMilliseconds;
  BYTE wMonth;
  BYTE wDayOfWeek;
  BYTE wDay;
  BYTE wHour;
  BYTE wMinute;
  BYTE wSecond;
  } SYSTEMTIME;
BOOL FileTimeToSystemTime(const FILETIME *lpFileTime,SYSTEMTIME *lpSystemTime);
int DlgDirList(HWND hDlg, char *lpPathSpec, int nIDListBox, int nIDStaticPath,
    UINT uFileType);
BOOL DlgDirSelectEx(HWND hDlg, char *lpString, int nCount, int nIDListBox);
int DlgDirListComboBox(HWND hDlg, char *lpPathSpec, int nIDComboBox, int nIDStaticPath,
    UINT uFiletype);
BOOL DlgDirSelectComboBoxEx(HWND hDlg, char *lpString, int nCount, int nIDComboBox);

BYTE GetTempPath(BYTE nBufferLength,char *lpBuffer);
UINT16 GetTempFileName(char *lpPathName,const char *lpPrefixString,
  UINT16 uUnique,char *lpTempFileName);
DWORD GetLogicalDrives();
BOOL GetVolumeInformation(const char *lpRootPathName,char *lpVolumeNameBuffer,
  DWORD nVolumeNameSize, DWORD *lpVolumeSerialNumber,
  DWORD *lpMaximumComponentLength,DWORD *lpFileSystemFlags,
  char *lpFileSystemNameBuffer,BYTE nFileSystemNameSize);


/*
 * Dialog Styles
 */
#define DS_ABSALIGN         0x01L
#define DS_SYSMODAL         0x02L
#define DS_LOCALEDIT        0x20L   /* Edit items get Local storage. */
#define DS_SETFONT          0x40L   /* User specified font for Dlg controls */
#define DS_MODALFRAME       0x80L   /* Can be combined with WS_CAPTION  */
#define DS_NOIDLEMSG        0x100L  /* WM_ENTERIDLE message will not be sent */
#define DS_SETFOREGROUND    0x200L  /* not in win3.1 */


#define DS_3DLOOK           0x0004L
#define DS_FIXEDSYS         0x0008L
#define DS_NOFAILCREATE     0x0010L
#define DS_CONTROL          0x0400L
#define DS_CENTER           0x0800L
#define DS_CENTERMOUSE      0x1000L
#define DS_CONTEXTHELP      0x2000L


#define DM_GETDEFID         (WM_USER+0)
#define DM_SETDEFID         (WM_USER+1)

#define DM_REPOSITION       (WM_USER+2)
/*
 * Returned in HIWORD() of DM_GETDEFID result if msg is supported
 */
#define DC_HASDEFID         0x534B

/*
 * Dialog Codes
 */
#define DLGC_WANTARROWS     0x0001      /* Control wants arrow keys         */
#define DLGC_WANTTAB        0x0002      /* Control wants tab keys           */
#define DLGC_WANTALLKEYS    0x0004      /* Control wants all keys           */
#define DLGC_WANTMESSAGE    0x0004      /* Pass message to control          */
#define DLGC_HASSETSEL      0x0008      /* Understands EM_SETSEL message    */
#define DLGC_DEFPUSHBUTTON  0x0010      /* Default pushbutton               */
#define DLGC_UNDEFPUSHBUTTON 0x0020     /* Non-default pushbutton           */
#define DLGC_RADIOBUTTON    0x0040      /* Radio button                     */
#define DLGC_WANTCHARS      0x0080      /* Want WM_CHAR messages            */
#define DLGC_STATIC         0x0100      /* Static item: don't include       */
#define DLGC_BUTTON         0x2000      /* Button item: can be checked      */

#define LB_CTLCODE          0L

/*
 * Listbox Return Values
 */
#define LB_OKAY             0
#define LB_ERR              (-1)
#define LB_ERRSPACE         (-2)

/*
**  The idStaticPath parameter to DlgDirList can have the following values
**  ORed if the list box should show other details of the files along with
**  the name of the files;
*/
            /* all other details also will be returned */


/*
 * Listbox Notification Codes
 */
#define LBN_ERRSPACE        (-2)
#define LBN_SELCHANGE       1
#define LBN_DBLCLK          2
#define LBN_SELCANCEL       3
#define LBN_SETFOCUS        4
#define LBN_KILLFOCUS       5


/*
 * Listbox messages
 */
#define LB_ADDSTRING            0x0180
#define LB_INSERTSTRING         0x0181
#define LB_DELETESTRING         0x0182
#define LB_SELITEMRANGEEX       0x0183
#define LB_RESETCONTENT         0x0184
#define LB_SETSEL               0x0185
#define LB_SETCURSEL            0x0186
#define LB_GETSEL               0x0187
#define LB_GETCURSEL            0x0188
#define LB_GETTEXT              0x0189
#define LB_GETTEXTLEN           0x018A
#define LB_GETCOUNT             0x018B
#define LB_SELECTSTRING         0x018C
#define LB_DIR                  0x018D
#define LB_GETTOPINDEX          0x018E
#define LB_FINDSTRING           0x018F
#define LB_GETSELCOUNT          0x0190
#define LB_GETSELITEMS          0x0191
#define LB_SETTABSTOPS          0x0192
#define LB_GETHORIZONTALEXTENT  0x0193
#define LB_SETHORIZONTALEXTENT  0x0194
#define LB_SETCOLUMNWIDTH       0x0195
#define LB_ADDFILE              0x0196
#define LB_SETTOPINDEX          0x0197
#define LB_GETITEMRECT          0x0198
#define LB_GETITEMDATA          0x0199
#define LB_SETITEMDATA          0x019A
#define LB_SELITEMRANGE         0x019B
#define LB_SETANCHORINDEX       0x019C
#define LB_GETANCHORINDEX       0x019D
#define LB_SETCARETINDEX        0x019E
#define LB_GETCARETINDEX        0x019F
#define LB_SETITEMHEIGHT        0x01A0
#define LB_GETITEMHEIGHT        0x01A1
#define LB_FINDSTRINGEXACT      0x01A2
#define LB_SETLOCALE            0x01A5
#define LB_GETLOCALE            0x01A6
#define LB_SETCOUNT             0x01A7
#define LB_INITSTORAGE          0x01A8
#define LB_ITEMFROMPOINT        0x01A9
#define LB_MSGMAX               0x01B0
#define LB_MULTIPLEADDSTRING    0x01B1
#define LB_GETLISTBOXINFO       0x01B2

/*
 * Listbox Styles
 */
#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_NOREDRAW          0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL       0x0800L
#define LBS_DISABLENOSCROLL   0x1000L
#define LBS_NODATA            0x2000L
#define LBS_NOSEL             0x4000L
#define LBS_STANDARD          (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

typedef struct __attribute((packed)) _LISTITEM {
    char data[64];
    void *tag;
//    struct _LISTITEM *prev;   // volendo :)
    struct _LISTITEM *next;
    uint16_t id;
    BYTE state;     // b0=selected
} LISTITEM;


/*
 * Combo Box return Values
 */
#define CB_OKAY             0
#define CB_ERR              (-1)
#define CB_ERRSPACE         (-2)


/*
 * Combo Box Notification Codes
 */
#define CBN_ERRSPACE        (-1)
#define CBN_SELCHANGE       1
#define CBN_DBLCLK          2
#define CBN_SETFOCUS        3
#define CBN_KILLFOCUS       4
#define CBN_EDITCHANGE      5
#define CBN_EDITUPDATE      6
#define CBN_DROPDOWN        7
#define CBN_CLOSEUP         8
#define CBN_SELENDOK        9
#define CBN_SELENDCANCEL    10

/*
 * Combo Box styles
 */
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L
#define CBS_UPPERCASE           0x2000L
#define CBS_LOWERCASE           0x4000L


/*
 * Combo Box messages
 */
#define CB_GETEDITSEL               0x0140
#define CB_LIMITTEXT                0x0141
#define CB_SETEDITSEL               0x0142
#define CB_ADDSTRING                0x0143
#define CB_DELETESTRING             0x0144
#define CB_DIR                      0x0145
#define CB_GETCOUNT                 0x0146
#define CB_GETCURSEL                0x0147
#define CB_GETLBTEXT                0x0148
#define CB_GETLBTEXTLEN             0x0149
#define CB_INSERTSTRING             0x014A
#define CB_RESETCONTENT             0x014B
#define CB_FINDSTRING               0x014C
#define CB_SELECTSTRING             0x014D
#define CB_SETCURSEL                0x014E
#define CB_SHOWDROPDOWN             0x014F
#define CB_GETITEMDATA              0x0150
#define CB_SETITEMDATA              0x0151
#define CB_GETDROPPEDCONTROLRECT    0x0152
#define CB_SETITEMHEIGHT            0x0153
#define CB_GETITEMHEIGHT            0x0154
#define CB_SETEXTENDEDUI            0x0155
#define CB_GETEXTENDEDUI            0x0156
#define CB_GETDROPPEDSTATE          0x0157
#define CB_FINDSTRINGEXACT          0x0158
#define CB_SETLOCALE                0x0159
#define CB_GETLOCALE                0x015A
#define CB_GETTOPINDEX              0x015b
#define CB_SETTOPINDEX              0x015c
#define CB_GETHORIZONTALEXTENT      0x015d
#define CB_SETHORIZONTALEXTENT      0x015e
#define CB_GETDROPPEDWIDTH          0x015f
#define CB_SETDROPPEDWIDTH          0x0160
#define CB_INITSTORAGE              0x0161
#define CB_MSGMAX                   0x0162


#define CCM_FIRST               0x2000      // Common control shared messages
#define CCM_LAST                (CCM_FIRST + 0x200)
#define CCM_SETBKCOLOR          (CCM_FIRST + 1) // lParam is bkColor

#define PBM_SETRANGE            (WM_USER+1)
#define PBM_SETPOS              (WM_USER+2)
#define PBM_DELTAPOS            (WM_USER+3)
#define PBM_SETSTEP             (WM_USER+4)
#define PBM_STEPIT              (WM_USER+5)
#define PBM_SETRANGE32          (WM_USER+6)  // lParam = high, wParam = low
typedef struct __attribute((packed)) {
   int iLow;
   int iHigh;
    } PBRANGE;
#define PBM_GETRANGE            (WM_USER+7)  // wParam = return (TRUE ? low : high). lParam = PPBRANGE or NULL
#define PBM_GETPOS              (WM_USER+8)
#define PBM_SETBARCOLOR         (WM_USER+9)             // lParam = bar color
#define PBM_SETBKCOLOR          CCM_SETBKCOLOR  // lParam = bkColor
#define PBM_SETMARQUEE          (WM_USER+10)
#define PBM_GETSTEP             (WM_USER+13)
#define PBM_GETBKCOLOR          (WM_USER+14)
#define PBM_GETBARCOLOR         (WM_USER+15)
#define PBM_SETSTATE            (WM_USER+16) // wParam = PBST_[State] (NORMAL, ERROR, PAUSED)
#define PBM_GETSTATE            (WM_USER+17)
#define PBS_SMOOTH              0x1
#define PBS_CAPTION             0x2     // mio!
#define PBS_VERTICAL            0x4
#define PBS_MARQUEE             0x08
#define PBS_SMOOTHREVERSE       0x10

#define PBST_NORMAL             0x0001
#define PBST_ERROR              0x0002
#define PBST_PAUSED             0x0003


#define TBS_AUTOTICKS           0x0001
#define TBS_VERT                0x0002
#define TBS_HORZ                0x0000
#define TBS_TOP                 0x0004
#define TBS_BOTTOM              0x0000
#define TBS_LEFT                0x0004
#define TBS_RIGHT               0x0000
#define TBS_BOTH                0x0008
#define TBS_NOTICKS             0x0010
#define TBS_ENABLESELRANGE      0x0020
#define TBS_FIXEDLENGTH         0x0040
#define TBS_NOTHUMB             0x0080
#define TBS_TOOLTIPS            0x0100
#define TBS_REVERSED            0x0200  // Accessibility hint: the smaller number (usually the min value) means "high" and the larger number (usually the max value) means "low"
#define TBS_DOWNISLEFT          0x0400  // Down=Left and Up=Right (default is Down=Right and Up=Left)
#define TBS_NOTIFYBEFOREMOVE    0x0800  // Trackbar should notify parent before repositioning the slider due to user action (enables snapping)
#define TBS_TRANSPARENTBKGND    0x1000  // Background is painted by the parent via WM_PRINTCLIENT

#define TBM_GETPOS              (WM_USER)
#define TBM_GETRANGEMIN         (WM_USER+1)
#define TBM_GETRANGEMAX         (WM_USER+2)
#define TBM_GETTIC              (WM_USER+3)
#define TBM_SETTIC              (WM_USER+4)
#define TBM_SETPOS              (WM_USER+5)
#define TBM_SETRANGE            (WM_USER+6)
#define TBM_SETRANGEMIN         (WM_USER+7)
#define TBM_SETRANGEMAX         (WM_USER+8)
#define TBM_CLEARTICS           (WM_USER+9)
#define TBM_SETSEL              (WM_USER+10)
#define TBM_SETSELSTART         (WM_USER+11)
#define TBM_SETSELEND           (WM_USER+12)
#define TBM_GETPTICS            (WM_USER+14)
#define TBM_GETTICPOS           (WM_USER+15)
#define TBM_GETNUMTICS          (WM_USER+16)
#define TBM_GETSELSTART         (WM_USER+17)
#define TBM_GETSELEND           (WM_USER+18)
#define TBM_CLEARSEL            (WM_USER+19)
#define TBM_SETTICFREQ          (WM_USER+20)
#define TBM_SETPAGESIZE         (WM_USER+21)
#define TBM_GETPAGESIZE         (WM_USER+22)
#define TBM_SETLINESIZE         (WM_USER+23)
#define TBM_GETLINESIZE         (WM_USER+24)
#define TBM_GETTHUMBRECT        (WM_USER+25)
#define TBM_GETCHANNELRECT      (WM_USER+26)
#define TBM_SETTHUMBLENGTH      (WM_USER+27)
#define TBM_GETTHUMBLENGTH      (WM_USER+28)
#define TBM_SETTOOLTIPS         (WM_USER+29)
#define TBM_GETTOOLTIPS         (WM_USER+30)
#define TBM_SETTIPSIDE          (WM_USER+31)
// TrackBar Tip Side flags
#define TBTS_TOP                0
#define TBTS_LEFT               1
#define TBTS_BOTTOM             2
#define TBTS_RIGHT              3

#define TBM_SETBUDDY            (WM_USER+32) // wparam = BOOL fLeft; (or right)
#define TBM_GETBUDDY            (WM_USER+33) // wparam = BOOL fLeft; (or right)
#define TBM_SETPOSNOTIFY        (WM_USER+34)
#define TBM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
#define TBM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT


#define TB_LINEUP               0
#define TB_LINEDOWN             1
#define TB_PAGEUP               2
#define TB_PAGEDOWN             3
#define TB_THUMBPOSITION        4
#define TB_THUMBTRACK           5
#define TB_TOP                  6
#define TB_BOTTOM               7
#define TB_ENDTRACK             8

// custom draw item specs
#define TBCD_TICS    0x0001
#define TBCD_THUMB   0x0002
#define TBCD_CHANNEL 0x0003

#define TRBN_THUMBPOSCHANGING       (TRBN_FIRST-1)

// Structure for Trackbar's TRBN_THUMBPOSCHANGING notification
typedef struct {
    NMHDR hdr;
    DWORD dwPos;
    int nReason;
} NMTRBTHUMBPOSCHANGING;


#define SBM_SETPOS                  0x00E0 /*not in win3.1 */
#define SBM_GETPOS                  0x00E1 /*not in win3.1 */
#define SBM_SETRANGE                0x00E2 /*not in win3.1 */
#define SBM_SETRANGEREDRAW          0x00E6 /*not in win3.1 */
#define SBM_GETRANGE                0x00E3 /*not in win3.1 */
#define SBM_ENABLE_ARROWS           0x00E4 /*not in win3.1 */
#define SBM_SETSCROLLINFO           0x00E9
#define SBM_GETSCROLLINFO           0x00EA
#define SBM_GETSCROLLBARINFO        0x00EB      // win 5.0


typedef struct _UDACCEL {
    UINT nSec;
    UINT nInc;
} UDACCEL;

#define UD_MAXVAL               0x7f        // mio
#define UD_MINVAL               (-UD_MAXVAL)

#define UDS_WRAP                0x0001
#define UDS_SETBUDDYINT         0x0002
#define UDS_ALIGNRIGHT          0x0004
#define UDS_ALIGNLEFT           0x0008
#define UDS_AUTOBUDDY           0x0010
#define UDS_ARROWKEYS           0x0020
#define UDS_HORZ                0x0040
#define UDS_NOTHOUSANDS         0x0080
#define UDS_HOTTRACK            0x0100

#define UDM_SETRANGE            (WM_USER+101)
#define UDM_GETRANGE            (WM_USER+102)
#define UDM_SETPOS              (WM_USER+103)
#define UDM_GETPOS              (WM_USER+104)
#define UDM_SETBUDDY            (WM_USER+105)
#define UDM_GETBUDDY            (WM_USER+106)
#define UDM_SETACCEL            (WM_USER+107)
#define UDM_GETACCEL            (WM_USER+108)
#define UDM_SETBASE             (WM_USER+109)
#define UDM_GETBASE             (WM_USER+110)
#define UDM_SETRANGE32          (WM_USER+111)
#define UDM_GETRANGE32          (WM_USER+112) // wParam & lParam are LPINT
#define UDM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
#define UDM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT
#define UDM_SETPOS32            (WM_USER+113)
#define UDM_GETPOS32            (WM_USER+114)

HWND CreateUpDownControl(DWORD dwStyle, int x, int y, int cx, int cy,
                                HWND hParent, int nID, HINSTANCE hInst,
                                HWND hBuddy,
                                int nUpper, int nLower, int nPos);

#define NM_UPDOWN      NMUPDOWN
#define LPNM_UPDOWN  LPNMUPDOWN

typedef struct _NM_UPDOWN {
    NMHDR hdr;
    int iPos;
    int iDelta;
} NMUPDOWN;

#define UDN_DELTAPOS            (UDN_FIRST - 1)


#define SB_SETTEXTA             (WM_USER+1)
#define SB_SETTEXTW             (WM_USER+11)
#define SB_GETTEXTA             (WM_USER+2)
#define SB_GETTEXTW             (WM_USER+13)
#define SB_GETTEXTLENGTHA       (WM_USER+3)
#define SB_GETTEXTLENGTHW       (WM_USER+12)

#define SB_GETTEXT              SB_GETTEXTA
#define SB_SETTEXT              SB_SETTEXTA
#define SB_GETTEXTLENGTH        SB_GETTEXTLENGTHA

#define SB_SETPARTS             (WM_USER+4)
#define SB_GETPARTS             (WM_USER+6)
#define SB_GETBORDERS           (WM_USER+7)
#define SB_SETMINHEIGHT         (WM_USER+8)
#define SB_SIMPLE               (WM_USER+9)
#define SB_GETRECT              (WM_USER+10)
#define SB_ISSIMPLE             (WM_USER+14)
#define SB_SETICON              (WM_USER+15)
#define SB_SETTIPTEXT          (WM_USER+16)
#define SB_GETTIPTEXT          (WM_USER+18)
#define SB_GETICON              (WM_USER+20)
//#define SB_SETUNICODEFORMAT     CCM_SETUNICODEFORMAT
//#define SB_GETUNICODEFORMAT     CCM_GETUNICODEFORMAT
#define SBT_OWNERDRAW            0x1000
#define SBT_NOBORDERS            0x0100
#define SBT_POPOUT               0x0200
#define SBT_RTLREADING           0x0400
#define SBT_NOTABPARSING         0x0800
#define SB_SETBKCOLOR           CCM_SETBKCOLOR      // lParam = bkColor
#define SBN_SIMPLEMODECHANGE    (SBN_FIRST - 0)
#define SB_SIMPLEID  0x00ff

#define MAKEINTRESOURCE(n) (n)
#define IDC_ARROW           MAKEINTRESOURCE(32512)
#define IDC_IBEAM           MAKEINTRESOURCE(32513)
#define IDC_WAIT            MAKEINTRESOURCE(32514)
#define IDC_CROSS           MAKEINTRESOURCE(32515)
#define IDC_UPARROW         MAKEINTRESOURCE(32516)
#define IDC_SIZE            MAKEINTRESOURCE(32640)  /* OBSOLETE: use IDC_SIZEALL */
#define IDC_ICON            MAKEINTRESOURCE(32641)  /* OBSOLETE: use IDC_ARROW */
#define IDC_SIZENWSE        MAKEINTRESOURCE(32642)
#define IDC_SIZENESW        MAKEINTRESOURCE(32643)
#define IDC_SIZEWE          MAKEINTRESOURCE(32644)
#define IDC_SIZENS          MAKEINTRESOURCE(32645)
#define IDC_SIZEALL         MAKEINTRESOURCE(32646)
#define IDC_NO              MAKEINTRESOURCE(32648) /*not in win3.1 */
#define IDC_HAND            MAKEINTRESOURCE(32649)
#define IDC_APPSTARTING     MAKEINTRESOURCE(32650) /*not in win3.1 */
#define IDC_HELP            MAKEINTRESOURCE(32651)
#define IDI_APPLICATION     MAKEINTRESOURCE(32512)
#define IDI_HAND            MAKEINTRESOURCE(32513)
#define IDI_QUESTION        MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION     MAKEINTRESOURCE(32515)
#define IDI_ASTERISK        MAKEINTRESOURCE(32516)
#define IDI_WINLOGO         MAKEINTRESOURCE(32517)
#define IDI_SHIELD          MAKEINTRESOURCE(32518)
#define IDI_WARNING         IDI_EXCLAMATION
#define IDI_ERROR           IDI_HAND
#define IDI_INFORMATION     IDI_ASTERISK


typedef struct __attribute((packed)) _OPENFILENAME {
//  DWORD         lStructSize;
//  HWND          hwndOwner;
//  DWORD/*HINSTANCE*/     hInstance;
  char path[32];
//  SearchRec rec;
  BYTE attribMask;         // sempre occhio allineamento..
//  BYTE tipoVis;             // messi in DWL del dialog..
  FS_DISK_PROPERTIES fsdp;
  char disk;
  } OPENFILENAME;
STATIC_ASSERT(!(sizeof(struct _OPENFILENAME) % 4),0);
typedef struct __attribute((packed)) _DIRLIST {
//  DWORD         lStructSize;
//  HWND          hwndOwner;
//  DWORD/*HINSTANCE*/     hInstance;
  char path[32];
//  SearchRec rec;
//  BYTE attribMask;         // sempre occhio allineamento..
//  BYTE tipoVis;             // messi in DWL del dialog..
  FS_DISK_PROPERTIES fsdp;      // ofs 32
  char disk;                    // ofs 66    
  char filler[1];
  } DIRLIST;
STATIC_ASSERT(!(sizeof(struct _DIRLIST) % 4),0);

#define CDN_FIRST               (0U-601U)
#define CDN_INITDONE            (CDN_FIRST - 0x0000)
#define CDN_FOLDERCHANGE        (CDN_FIRST - 0x0002)
#define CDN_FILEOK              (CDN_FIRST - 0x0005)
#define CDN_TYPECHANGE          (CDN_FIRST - 0x0006)
#define CDN_INCLUDEITEM         (CDN_FIRST - 0x0007)


// threads
#define THREAD_ID_INVALID	(-1)
#define MAX_THREADS			(20)
#define TIMESLICE_MS		(1)
#define ENV_POSITION_RETURNADDRESS (11*2)      // sperimentale (non ci sono i sorgenti e il debugger mostra int64!! @#£$% #cancrokubler
#define ENV_POSITION_RETURNCODE (23)      // a caso, v. .. oppure si potrebbero usare 8-9 dove c'è lo stack, vicinanze
typedef enum __attribute__ ((__packed__)) {
	THREAD_NEW=0, 
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_SLEEPING,
    THREAD_BLOCKED,
    THREAD_DONE
    } THREAD_STATE;
typedef enum __attribute__ ((__packed__)) {
//	THREAD_PTY_IDLE=0,
//	THREAD_PTY_VERYLOW,
    THREAD_PTY_LOW=1,
    THREAD_PTY_MIDDLE,
    THREAD_PTY_HIGH,
//    THREAD_PTY_VERYHIGH,
//    THREAD_PTY_REALTIME
    } THREAD_PRIORITY;
#define _JB_LEN 24      // incongruenza (coglioni) in setjmp.h https://forum.microchip.com/s/topic/a5C3l000000MSHUEA4/t338772
typedef int jmp_buf[_JB_LEN]; // sembra andare... 28/1/24 #porcamadonnalenuvole #leucemiafiglicarabinieri
typedef struct __attribute((packed)) _THREAD {
	void *context;
    struct _THREAD *next;
	jmp_buf env;
    THREAD_PRIORITY priority;
    THREAD_STATE state;
  	uint16_t sleepCount;		// sleeptime in ms
    uint16_t signals;
    unsigned char ID; //... ovvero usiamo ordine prog. getThreadID
    BYTE pendingIO;
    } THREAD;
STATIC_ASSERT(!(sizeof(struct _THREAD) % 4),0);
#define	ATOMIC_START()
#define	ATOMIC_END()
THREAD *BeginThread(void *function,BYTE state,DWORD parm);      // SIZE_T  dwStackSize
BOOL EndThread(THREAD *);
void ExitThread(DWORD);
BOOL TerminateThread(THREAD *,DWORD);
void SuspendThread(THREAD *);
void ResumeThread(THREAD *);
BOOL GetExitCodeThread(THREAD *,DWORD *lpExitCode);
void ThreadSleep(uint16_t);
BOOL SwitchToThread(THREAD *);
BYTE GetThreadID(THREAD *);
THREAD_PRIORITY GetThreadPriority(THREAD *);
BOOL SetThreadPriority(THREAD *,THREAD_PRIORITY);
BOOL GetThreadPriorityBoost(THREAD *,BOOL *pDisablePriorityBoost);
BOOL SetThreadPriorityBoost(THREAD *,BOOL pDisablePriorityBoost);
int GetThreadDescription(THREAD *,char *ppszThreadDescription);
int SetThreadDescription(THREAD *,char *lpThreadDescription);
//BOOL GetThreadInformation(THREAD *,THREAD_INFORMATION_CLASS ThreadInformationClass,
//  void *ThreadInformation,DWORD ThreadInformationSize);
//BOOL SetThreadInformation(THREAD *,THREAD_INFORMATION_CLASS ThreadInformationClass,
//       void *ThreadInformation,DWORD ThreadInformationSize);
BOOL GetThreadTimes(THREAD *,FILETIME *lpCreationTime,FILETIME *lpExitTime,
  FILETIME *lpKernelTime,FILETIME *lpUserTime);
BOOL GetThreadIOPendingFlag(THREAD *,BOOL *lpIOIsPending);
BYTE GetCurrentThreadID(void);
#define GetCurrentThread() getActiveThread()        // per forza...
THREAD *GetThreadFromID(BYTE );
THREAD *GetThreadFromContext(void *);
THREAD *getNextThread(void);
THREAD *getNextThreadSeq(void);
THREAD *getActiveThread(void);
THREAD *setActiveThread(THREAD *);
//extern THREAD *rootThreads,*winManagerThreadID;
//https://stackoverflow.com/questions/14685406/practical-usage-of-setjmp-and-longjmp-in-c/14685524
//https://en.wikipedia.org/wiki/Setjmp.h
// DEVO spacioccare con l'active Thread perché non c'è modo di ricavare il thread da dentro la funzione...
//  IDEM deve essere una Macro per salvare/ricevere la posizione attuale
//void Yield(void *);
#ifdef USA_BREAKTHROUGH
#define Yield() \
      {\
      ATOMIC_START();\
      THREAD *nextThread=getNextThread();\
      THREAD *myThread=getActiveThread();\
      switch(myThread->state) {\
        case THREAD_NEW:\
          if(setjmp(myThread->env) == 0) {\
            myThread->state = THREAD_READY;\
            return;\
            }\
          else\
            myThread->state = THREAD_RUNNING;\
          break;\
        case THREAD_READY:\
          Nop();\
          break;\
        case THREAD_RUNNING:\
          if(setjmp(myThread->env) == 0) {\
            if(myThread->state==THREAD_RUNNING) myThread->state = THREAD_READY;\
            setActiveThread(nextThread);\
            longjmp(nextThread->env, GetThreadID(nextThread));\
            return;\
            }\
          else\
            setActiveThread(myThread);\
          break;\
        default:\
          Nop();\
          break;\
        }\
      ATOMIC_END();\
      }
#else
#define Yield()
#endif
//https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html

#define WAIT_OBJECT_0 0
#define WAIT_ABANDONED_0 0x00000080L
#define WAIT_TIMEOUT 0x00000102L
#define WAIT_FAILED (DWORD)0xFFFFFFFF
#define INFINITE -1
BOOL SetEvent(THREAD *,BYTE hEvent);        // circa :)
BOOL ResetEvent(THREAD *,BYTE hEvent);
DWORD WaitForSingleObject(THREAD *,BYTE hEvent,DWORD dwMilliseconds);
DWORD WaitForSingleObjectEx(THREAD *,BYTE hEvent,DWORD dwMilliseconds,BOOL bAutoReset); // mia modifica
DWORD WaitForMultipleObjects(THREAD *,BYTE eventMask,BOOL bWaitAll,DWORD dwMilliseconds);
// queste appartengono al C... int raise(int sig);
//void (* signal(int sig, void (*func)(int)) )(int);

typedef LRESULT (* HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);
typedef uint32_t HHOOK;
HHOOK SetWindowsHookEx(int idHook,HOOKPROC lpfn,HINSTANCE hmod,DWORD dwThreadId);
BOOL UnhookWindowsHookEx(HHOOK hhk);
LRESULT CallNextHookEx(HHOOK hhk,int nCode,WPARAM wParam,LPARAM lParam);


extern volatile uint32_t millis;
DWORD timeGetTime(void);

extern const char *PROFILE_FILE,*profileFile2;
#define GetProfileString(section,key,defval,val,size) GetPrivateProfileString(section,key,defval,val,size,PROFILE_FILE)
#define GetProfileInt(section,key,defval) GetPrivateProfileInt(section,key,defval,PROFILE_FILE)
#define WriteProfileString(section,key,val) WritePrivateProfileString(section,key,val,PROFILE_FILE)
#define WriteProfileInt(section,key,val) WritePrivateProfileInt(section,key,val,PROFILE_FILE)
BYTE GetPrivateProfileString(const char *section,const char *key,const char *defval,char *val,WORD size,const char *file);
int GetPrivateProfileInt(const char *section,const char *key,int defval,const char *file);
BYTE WritePrivateProfileString(const char *section,const char *key,const char *val,const char *file);
BYTE WritePrivateProfileInt(const char *section,const char *key,int val,const char *file);

BOOL SetFirmwareEnvironmentVariable(const char *lpName,const char *lpGuid,void *pValue,DWORD nSize);
typedef enum {
  FirmwareTypeUnknown,
  FirmwareTypeBios,
  FirmwareTypeUefi,
  FirmwareTypeMax
} FIRMWARE_TYPE;
BOOL GetFirmwareType(FIRMWARE_TYPE *FirmwareType);
typedef struct {
//  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  CHAR  szCSDVersion[128];
  WORD  wServicePackMajor;
  WORD  wServicePackMinor;
  WORD  wSuiteMask;
  BYTE  wProductType;
  BYTE  wReserved;
} OSVERSIONINFOEX;
BOOL VerifyVersionInfo(OSVERSIONINFOEX *lpVersionInformation,DWORD dwTypeMask,uint64_t dwlConditionMask);

UINT GetWindowModuleFileName(HWND hwnd,char *pszFileName,uint16_t cchFileNameMax);
BOOL GetUserName(LPSTR lpBuffer,BYTE *pcbBuffer);
BOOL GetComputerName(LPSTR lpBuffer,BYTE *nSize);
/* per ora no typedef struct {
  struct _IP_ADAPTER_INFO *Next;
  DWORD                   ComboIndex;
  char                    AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
  char                    Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
  UINT                    AddressLength;
  BYTE                    Address[MAX_ADAPTER_ADDRESS_LENGTH];
  DWORD                   Index;
  UINT                    Type;
  UINT                    DhcpEnabled;
  PIP_ADDR_STRING         CurrentIpAddress;
  IP_ADDR_STRING          IpAddressList;
  IP_ADDR_STRING          GatewayList;
  IP_ADDR_STRING          DhcpServer;
  BOOL                    HaveWins;
  IP_ADDR_STRING          PrimaryWinsServer;
  IP_ADDR_STRING          SecondaryWinsServer;
  time_t                  LeaseObtained;
  time_t                  LeaseExpires;
} IP_ADAPTER_INFO;
DWORD GetAdaptersInfo(PIP_ADAPTER_INFO AdapterInfo,DWORD *SizePointer);*/
IP_ADDR GetIPAddress(int8_t); 

char *CharLower(char *lpsz);
char *CharUpper(char *lpsz);
uint16_t CharLowerBuff(char *lpsz,uint16_t cchLength);
uint16_t CharUpperBuff(char *lpsz,uint16_t cchLength);
char *CharNext(const char *lpsz);
char *CharPrev(const char *lpszStart,const char *lpszCurrent);
BOOL CharToOem(const char *pSrc,char *pDst);

BOOL GetMessage(MSG *lpMsg,HWND hWnd,uint16_t wMsgFilterMin,uint16_t wMsgFilterMax);
BOOL PeekMessage(MSG *lpMsg,HWND hWnd,uint16_t wMsgFilterMin,uint16_t wMsgFilterMax);
BOOL TranslateMessage(const MSG *lpMsg);
LRESULT DispatchMessage(const MSG *lpMsg);
BOOL IsDialogMessage(HWND hDlg,MSG *lpMsg);
BOOL IsHungAppWindow(HWND hwnd);
BOOL GetInputState();
BOOL InSendMessage();
DWORD InSendMessageEx();
UINT RegisterWindowMessage(const char *lpString);

BOOL OpenClipboard(HWND hWndNewOwner);
BOOL CloseClipboard();
BOOL EmptyClipboard();
HANDLE SetClipboardData(UINT uFormat,HANDLE hMem);
HANDLE GetClipboardData(UINT uFormat);
HWND GetClipboardOwner();
int CountClipboardFormats();
UINT EnumClipboardFormats(UINT format);
int GetClipboardFormatName(UINT format,LPSTR lpszFormatName,int cchMaxCount);
enum {
	CF_BITMAP          = 2,
	CF_DIB             = 8,
	CF_DIBV5           = 17,
	CF_DIF             = 5,
	CF_DSPBITMAP       = 0x0082,
	CF_DSPENHMETAFILE  = 0x008E,
	CF_DSPMETAFILEPICT = 0x0083,
	CF_DSPTEXT         = 0x0081,
	CF_ENHMETAFILE     = 14,
	CF_GDIOBJFIRST     = 0x0300,
	CF_GDIOBJLAST      = 0x03FF,
	CF_HDROP           = 15,
	CF_LOCALE          = 16,
	CF_METAFILEPICT    = 3,
	CF_OEMTEXT         = 7,
	CF_OWNERDISPLAY    = 0x0080,
	CF_PALETTE         = 9,
	CF_PENDATA         = 10,
	CF_PRIVATEFIRST    = 0x0200,
	CF_PRIVATELAST     = 0x02FF,
	CF_RIFF            = 11,
	CF_SYLK            = 4,
	CF_TEXT            = 1,
	CF_TIFF            = 6,
	CF_UNICODETEXT     = 13,
	CF_WAVE            = 12
    };
struct CLIPBOARD {
    HWND owner;
    void *data;
    UINT format;
    };

struct __attribute__ ((__packed__)) BITMAPFILEHEADER {
  WORD  bfType;
  DWORD bfSize;
  WORD  bfReserved1;
  WORD  bfReserved2;
  DWORD bfOffBits;
  } BITMAPFILEHEADER;
struct __attribute__ ((__packed__)) BITMAPINFOHEADER {
  DWORD biSize;
  DWORD biWidth;
  DWORD biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  DWORD biXPelsPerMeter;
  DWORD biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
  };

HINSTANCE ShellExecute(HWND hwnd,const char *lpOperation,const char *lpFile,
  const char *lpParameters,const char *lpDirectory,BYTE nShowCmd);

int MulDiv(int nNumber,int nNumerator,int nDenominator);
uint64_t UnsignedMultiply128(uint64_t Multiplier,uint64_t Multiplicand,uint64_t *HighProduct);
uint64_t Multiply128(int64_t Multiplier,int64_t Multiplicand,int64_t *HighProduct);
uint64_t UnsignedMultiplyExtract128(uint64_t Multiplier,uint64_t Multiplicand,BYTE Shift);
uint64_t PopulationCount64(uint64_t operand);
uint64_t ShiftLeft128(uint64_t LowPart,uint64_t HighPart,BYTE Shift);
uint64_t ShiftRight128(uint64_t LowPart,uint64_t HighPart,BYTE Shift);
#define UInt32x32To64(a,b)  ((uint64_t)a * b)
#define Int32x32To64(a,b)  ((int64_t)a * b)
#define Int64ShllMod32(a,b) {(uint64_t)a <<= b;}
#define Int64ShrlMod32(a,b) {(uint64_t)a >>= b;}
#define Int64ShraMod32(a,b) {(int64_t)a >>= b;}
uint64_t UnsignedMultiplyHigh(uint64_t Multiplier,uint64_t Multiplicand);

BOOL StrTrim(LPSTR psz, LPCSTR pszTrimChars);

BOOL ScreenToClient(HWND hWnd,POINT *lpPoint);
BOOL ClientToScreen(HWND hWnd,POINT *lpPoint);
int MapWindowPoints(HWND hWndFrom,HWND hWndTo,POINT *lpPoints,BYTE cPoints);

    
LRESULT DefWindowProc(HWND,uint16_t,WPARAM ,LPARAM);
LRESULT SendMessage(HWND,uint16_t,WPARAM ,LPARAM);
LRESULT CallWindowProc(WINDOWPROC lpPrevWndFunc,HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);

LRESULT DefWindowProcStaticWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcToolbarWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcEditWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcListboxWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcButtonWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcScrollBarWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcStatusBar(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcProgressWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcSpinWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcSliderWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcDlgMessageBox(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcFileDlgWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcDirWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcCmdShellWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcDC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcTaskBar(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcTaskManager(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcControlPanel(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT DefWindowProcVideoCap(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT viewerWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
LRESULT playerWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);


typedef struct {
  DWORD dwSize;
  DWORD dwICC;
} INITCOMMONCONTROLSEX;
BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX *picce);

// typedef _TIME_T_ time_t; per altri mplabx col cancro ai figli dei suoi creatori

enum TIPI_FILE {
    TIPOFILE_UNKNOWN=0,
    TIPOFILE_PROGRAMMA,
    TIPOFILE_ZIP,
    TIPOFILE_INI,
    TIPOFILE_BASIC,
    TIPOFILE_REXX,
    TIPOFILE_FORTH,
    TIPOFILE_LINK,
    TIPOFILE_TESTO,
    TIPOFILE_IMMAGINE,
    TIPOFILE_AUDIO,
    TIPOFILE_INTERNET,
    };

struct __attribute__ ((__packed__)) HTMLinfo {
	char sTitle[64];
	char baseAddr[32];
	char bkImage[64],bkSound[64];
	char sDescription[64],sKeywords[64],sGenerator[64];
	time_t sExpDate;
  struct HYPER_LINK *link1;
	GFX_COLOR bkColor,textColor,vlinkColor,hlinkColor,ulinkColor;
	WORD fLen;
	WORD rows,cols;
  };

#define SURF_TIMEOUT 2000
#define HTML_SIZE 4000

extern const char *tagContentLength;
extern const char *tagContentType;
extern const char *tagConnection;


#ifdef	__cplusplus
}
#endif

#endif	/* WINDOWS_H */

