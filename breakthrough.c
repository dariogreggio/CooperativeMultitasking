#warning il caret sborra sul menu aperto e/o su finestra davanti
// fare setcursor quando si entra/esce da move/size - è OK ma serve movimento mouse, o "aggiorna"...
//usare strtod "%g" , ftoa anche minibasic??
//il testo OK nel bottone non è centrato CREDO DIPENDA DAL FONT!
//sarebbe carino che la statusbar fosse SOTTO la scrollbar H
//message box esplodono quando chiudi, redraw infinito o crash...
//altre finestre sembrano sborrare sul menu Start aperto
//ovviamente gestire click su scroll, file manager ecc!
//i colori dell'icona "audio8" sembrano ciucchi... mp3 ah no è un problema di "YELLOW"... verificare tinta
//ramdisc... inoltre /L sembra non andare da command line (parm a execConf??
#warning verificare se messagebox schiantano ancora DOPO signed in coordinate RECT SIZE...
// OCCHIO che abbiam messo i MENUCOMMAND... se non si schiantano ! e specialmente la HIWORD (wparam) se da accelerator, tipo in Open di miniBasic
//  alla fine credo che il test HIWORD(wParam) == 0 || HIWORD(wParam) == 1 si potrebbe togliere!
//  e WM_MENUCHAR andrebbe cambiato: pare vada SEMPRE inviato quando si preme un tasto con menu presente, e DOPO si decide il da farsi! https://learn.microsoft.com/en-us/windows/win32/menurc/wm-menuchar


/*
 * BREAKTHROUGH 2.1
 * (C) G.Dar 1987-2024 :) !
 * (portions inspired by Windows, since 1990)
 * 
 * http://nano-x.org/ ahah siete morti :D https://github.com/moovel/microwindows
 * https://minigui.fmsoft.cn/api_ref/3.0.12_threads/index.html boh...?
 * 
 * NB: confermato 19/12/23, le coordinate sono 0..639 ecc, ossia "-1" dalla size!
 */
 
#include <xc.h>
//#include "compiler.h"
#include "pc_pic_cpu.h"
#include <stdio.h>
#include <string.h>


#define Color565(red, green, blue)    (GFX_COLOR) ((((GFX_COLOR)(red) & 0xF8) << 8) | (((GFX_COLOR)(green) & 0xFC) << 3) | (((GFX_COLOR)(blue) & 0xF8) >> 3))
#define ColorBGR(color)    (DWORD) ( ((color & 0xf800) >> 11) | ((color & 0x07e0) << 5) | ((((DWORD)color) & 0x001f) << 16) )
#define ColorRGB(color)    (DWORD) ( ((((DWORD)color) & 0xf800) << 8) | ((((DWORD)color) & 0x07e0) << 5) | ((((DWORD)color) & 0x001f) << 3) )
#define ColorR(color)    ((color & 0xf800) >> 8)        // left-justified, diciamo..
#define ColorG(color)    ((color & 0x07e0) >> 3)
#define ColorB(color)    ((color & 0x001f) << 3)
#define Color565To332(color)    (BYTE) ((WORD)( ((color & 0xe000) >> 8) | ((color & 0x0700) >> 6) | ((color & 0x0018) >> 3) ))
#define Color565To222(color)    (BYTE) ((WORD)( ((color & 0xc000) >> 10) | ((color & 0x0600) >> 7) | ((color & 0x0018) >> 3) ))
//#define Color24To565(color)    ((((color >> 16) & 0xFF) / 8) << 11) | ((((color >> 8) & 0xFF) / 4) << 5) | (((color) &  0xFF) / 8)
  //convert 24bit color into packet 16 bit one (credits for this are all mine) GD made a macro!

#include "breakthrough.h"
#include "breakthrough_private.h"
#include "fat_sd/FSIO.h"
#if defined(USA_USB_HOST_MSD)
#include "../harmony_pic32mz/usb_host_msd.h"
#include "../harmony_pic32mz/usb_host_scsi.h"
#endif
#include "fat_ide/idefsio.h"
#include "superfile.h"

#include "picojpeg.h"
#include "minibasic/minibasic.h"
#include "kommissarRexx/kommissarRexx.h"

#include "mp3/mp3.h"

extern enum FILE_DEVICE m_stdout,m_stdin,m_stderr;
extern const char *ASTERISKS;
extern char prompt[16];
extern signed char currDrive;
extern BYTE ledStatus;

BYTE textsize;
HWND rootWindows=NULL,taskbarWindow=NULL;
HWND desktopWindow=NULL;
static WNDCLASS *rootClasses=NULL;
/*static*/ THREAD *rootThreads=NULL,*winManagerThreadID;
POINTS mousePosition,caretPosition;
CURSOR mouseCursor;
//GFX_COLOR savedCursorArea[16*16];
HMENU activeMenu=NULL,activeMenuParent=NULL;
BYTE activeMenuCntX=0,activeMenuCntY=0;
HWND activeMenuWnd=NULL;
RECT activeMenuRect={0};
TRACKMOUSEEVENT trackMouseEvent;
BYTE eXtra=128;     // per abilitare mouse,suoni e altro.. b0= mouse; b4=audio; b7=screensaver
int8_t cursorCnt=0;
int8_t caretTime=50;    // in 10mS units
const char *PROFILE_FILE="WIN.INI",*profileFile2="SYSTEM.INI";
const char *OPZIONI_STR="OPZIONI",*DESKTOP_STR="DESKTOP",*BOOT_STR="BOOT",*WINDOWS_STR="WINDOWS";

struct CLIPBOARD clipboard;
HWND captureWindow=NULL;
RECT clipCursorRect;
static BYTE internalKeyboardBuffer[256];
static uint16_t doubleClickTime=400;    //ms
uint16_t screenSaverTime=120;		// sec
static uint32_t idleTime=0;    //ms
static uint16_t longClickCnt=0;    //ms
BYTE inScreenSaver=0,quitSignal=0;
BYTE volumeAudio=70;

static BYTE getVolumeAudio() { return volumeAudio & 0x80 ? 0 : volumeAudio; }   // gestisce Mute

GFX_COLOR windowForeColor, windowInactiveForeColor, windowBackColor, desktopColor;
const GFXfont *fontsHelv[]={FreeSans9pt7b,FreeSans12pt7b,FreeSans18pt7b,FreeSans24pt7b};
const GFXfont *fontsTimes[]={FreeSerif9pt7b,FreeSerif12pt7b,FreeSerif18pt7b,FreeSerif24pt7b};

HWND m_WndClock,m_WndCalendar,m_WndCalc,m_WndFileManager[2],m_WndTaskManager,m_WndControlPanel,m_WndBasic /* in effetti se ne potrebbero aprire più d'una...*/,m_WndDOS,
     m_WndSurf,m_WndViewer,m_WndPlayer,m_WndNotepad;

extern volatile unsigned long now;
extern BYTE SDcardOK,USBmemOK,HDOK,FDOK;
extern BYTE *RAMdiscArea;
extern SIZE Screen;
extern struct KEYPRESS keypress;
extern struct MOUSE mouse;
extern BYTE audio_played;
extern BYTE _bpp;
extern Ipv4Addr myIp;

#define CUR_INV 3
#define CUR_WHT WHITE     //2
#define CUR_BLK BLACK +1 /*+1 se usata trasparenza*/    //1
#define CUR_TRNSP BLACK   //  0 


#define MAX_TIMERS 8
TIMER_DATA timerData[MAX_TIMERS];    // diciamo 8

static inline BOOL __attribute__((always_inline)) isPointVisible(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y);
static void calculateClientArea(HWND hWnd,RECT *rc);
static void calculateNonClientArea(HWND hWnd,RECT *rc);
static void activateChildren(HWND hWnd,int8_t mode);
static BYTE countWindows(void);
static HWND getLastChildWindow(HWND hWnd);
static HWND getActiveChildWindow(HWND hWnd);
static HWND getFocusHelper(HWND myWnd);
static BOOL drawMenu(HWND hWnd,HMENU menu,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE mode,int8_t selectedItem);
static S_SIZE drawMenuitemPopup(HDC hDC,MENUITEM *m,UGRAPH_COORD_T x,UGRAPH_COORD_T y,S_SIZE cs,BYTE state); // state: b0=highlight/selected; b1=spazio per check a sx
static BOOL drawMenuPopup(HWND hWnd,HMENU menu,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE mode,int8_t selectedLine);    // mostra check a sx
static S_SIZE drawMenuitem(HDC hDC,MENUITEM *m,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE state);
static uint16_t getMenuFromPoint(HWND hWnd,POINT pt,HMENU*inMenu);
uint16_t getMenuPopupFromPoint(HMENU menu,RECT *rc,POINT pt,HMENU*inMenu);
static POINTS getMenuPosition(HMENU menuorig,HMENU menu);
static BYTE getMenuIndex(HMENU menu,HMENU menuorig);
static BYTE getMenuSize(HMENU menu,S_SIZE *cs,BYTE flags);
static char findMenuLetter(const char *s);
static int matchAccelerator(HMENU menu,char ch,BYTE modifiers);

static BOOL addHeadWindow(HWND,struct _WINDOW **head);
static BOOL addTailWindow(HWND,struct _WINDOW **head);
static BOOL insertWindow(HWND,HWND,struct _WINDOW **head);
static BOOL removeWindow(HWND,struct _WINDOW **head);
//static void SortLinkedList(struct _WINDOW *head);
static void list_bubble_sort(struct _WINDOW **head);
void printWindows(HWND);

static BYTE enterScreenSaver(void);
static BYTE exitScreenSaver(void);

static HWND setActive(HWND hWnd,BOOL state);
static void destroyWindowHelper(HWND hWnd);

int drawIcon8(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const ICON icon);
int drawCaret(HWND hWnd,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const CURSOR caret,BYTE bShow);
static void DrawCursor(UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const CURSOR cursor,BYTE size);

static void setTextSize(BYTE n);

VARIABLESTRING *findEnvVariable(const char *);
char *strupr(char *str);
char *strlwr(char *str);

//#warning SE MSGF_DIALOGBOX NON devo accettare menu! o si perde internalstate (io sarei in modalloop...
#define inDialogLoop() (hWnd->internalState==MSGF_DIALOGBOX)



//-------------------------windows----------------------------------------------
int8_t InitWindows(GFX_COLOR bgcolor /*DARKBLUE*/,enum ORIENTATION orientation,BYTE xtra,
  BYTE mode,const char *sfondo) {
  char buf[16];
  CREATESTRUCT cs,cs2;
  int i;
  
  eXtra=xtra;
  
  windowForeColor=BRIGHTCYAN;
  windowInactiveForeColor=GRAY160;
  windowBackColor=DARKGRAY;
  desktopColor=bgcolor;
  rootWindows=NULL;
  memset(timerData,0,sizeof(timerData));

  rootWindows=taskbarWindow=desktopWindow=NULL;
  rootClasses=NULL;
  rootThreads=NULL;
  activeMenu=activeMenuParent=NULL;
  activeMenuCntX=activeMenuCntY=0;
  activeMenuWnd=NULL;
  SetRectEmpty(&activeMenuRect);
  captureWindow=NULL;
  SetRectEmpty(&clipCursorRect);
  cursorCnt=0;
  caretTime=50;    // in 10mS units
  clipboard.data=NULL;
  memset(&internalKeyboardBuffer,0,sizeof(internalKeyboardBuffer));
  doubleClickTime=400;    //ms
  screenSaverTime=120;		// sec
  idleTime=0;    //ms
  longClickCnt=0;    //ms
  inScreenSaver=quitSignal=0;

  m_WndClock=m_WndCalc=NULL;
  m_WndFileManager[0]=m_WndFileManager[1]=NULL;
  m_WndTaskManager=m_WndControlPanel=NULL;
  m_WndBasic=NULL;

  desktopWindow=malloc(sizeof(struct _WINDOW)+sizeof(POINTS)*16+16+16+4+4+1);    // v. desktop class
  if(!desktopWindow)
    return 0;
  cs.class=desktopWindow->class=MAKECLASS(WC_DESKTOPCLASS);
  desktopWindow->windowProc=(WINDOWPROC*)DefWindowProcDC;
  strcpy(desktopWindow->caption,"desktop");   //When Windows first starts, the desktop's Class is "Progman"
  cs.lpszName=desktopWindow->caption;
  cs.style=desktopWindow->style=WS_VISIBLE;   // active qua boh
  cs.x=cs.y=desktopWindow->nonClientArea.top=desktopWindow->nonClientArea.left=0;
  desktopWindow->nonClientArea.right=Screen.cx-1;
  desktopWindow->nonClientArea.bottom=Screen.cy-1;
  desktopWindow->clientArea.top=desktopWindow->nonClientArea.top;
  desktopWindow->clientArea.left=desktopWindow->nonClientArea.left; 
  desktopWindow->clientArea.right=desktopWindow->nonClientArea.right; 
  desktopWindow->clientArea.bottom=desktopWindow->nonClientArea.bottom; 
  cs.cx=desktopWindow->clientArea.right-desktopWindow->clientArea.left;
  cs.cy=desktopWindow->clientArea.bottom-desktopWindow->clientArea.top;
  SetRectEmpty(&desktopWindow->paintArea);
  desktopWindow->parent=desktopWindow->children=NULL;
  desktopWindow->cursor=standardCursorSm;
  desktopWindow->zOrder=0;
  desktopWindow->next=NULL;
  desktopWindow->messageQueue=NULL;
  desktopWindow->status=desktopWindow->internalState=0; desktopWindow->visible=1; 
  desktopWindow->font=GetStockObject(SYSTEM_FONT).font; 
  desktopWindow->icon=NULL;
  cs.menu=desktopWindow->menu=NULL;
  desktopWindow->scrollSizeX=desktopWindow->scrollSizeY=100;    // default https://learn.microsoft.com/en-us/windows/win32/controls/about-scroll-bars
  desktopWindow->scrollPosX=desktopWindow->scrollPosY=0;
  calculateNonClientArea(desktopWindow,&desktopWindow->nonClientArea);    // serve sia prima che dopo, per corretta gestione in WM_CREATE e per ev. modificare dopo CREATESTRUCT..
  calculateClientArea(desktopWindow,&desktopWindow->clientArea);
  
  taskbarWindow=malloc(sizeof(struct _WINDOW));    // v. taskbar class
  if(!taskbarWindow)
    return 0;
  cs2.class=taskbarWindow->class=MAKECLASS(WC_TASKBARCLASS);
  taskbarWindow->windowProc=(WINDOWPROC*)DefWindowProcTaskBar;
  strcpy(taskbarWindow->caption,"taskbar");   //When Windows first starts, the desktop's Class is "Progman"
  cs2.lpszName=taskbarWindow->caption;
  cs2.style=taskbarWindow->style=WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN;
  cs2.x=taskbarWindow->nonClientArea.left=0;
  cs2.y=taskbarWindow->nonClientArea.top=Screen.cy-(11)-1;
  taskbarWindow->nonClientArea.right=Screen.cx-1;
  taskbarWindow->nonClientArea.bottom=Screen.cy-1;
  taskbarWindow->clientArea.top=taskbarWindow->nonClientArea.top;
  taskbarWindow->clientArea.left=taskbarWindow->nonClientArea.left; 
  taskbarWindow->clientArea.right=taskbarWindow->nonClientArea.right; 
  taskbarWindow->clientArea.bottom=taskbarWindow->nonClientArea.bottom; 
  cs2.cx=taskbarWindow->clientArea.right-taskbarWindow->clientArea.left;
  cs2.cy=taskbarWindow->clientArea.bottom-taskbarWindow->clientArea.top;
  SetRectEmpty(&taskbarWindow->paintArea);
  taskbarWindow->parent=taskbarWindow->children=NULL;   // o figlia di desktop?
  taskbarWindow->cursor=standardCursorSm;
  
  taskbarWindow->next=NULL;
  taskbarWindow->messageQueue=NULL;
  taskbarWindow->status=taskbarWindow->internalState=0; taskbarWindow->visible=1; 
  taskbarWindow->font=GetStockObject(SYSTEM_FONT).font; 
  taskbarWindow->icon=NULL;
  cs2.menu=taskbarWindow->menu=NULL;
  taskbarWindow->scrollSizeX=taskbarWindow->scrollSizeY=100;
  taskbarWindow->scrollPosX=taskbarWindow->scrollPosY=0;
  taskbarWindow->zOrder=1;  // (  0   /* on top, v. attrib */) ma serve a WindowFromPoint
  calculateNonClientArea(taskbarWindow,&taskbarWindow->nonClientArea);    // serve sia prima che dopo, per corretta gestione in WM_CREATE e per ev. modificare dopo CREATESTRUCT..
  calculateClientArea(taskbarWindow,&taskbarWindow->clientArea);

	switch(orientation) {
		case LANDSCAPE:
		case LANSCAPE_FLIPPED:
			break;

		case NONE:
			break;

		case PORTRAIT:
		case PORTRAIT_FLIPPED:
			break;
		}

  
#ifndef USING_SIMULATOR
  Cls();
  SetColors(WHITE,desktopColor,0);    // ok così, 
  DrawRectangle((Screen.cx-24*8)/2,Screen.cy/2-40,(Screen.cx+24*8)/2,Screen.cy/2+40,LIGHTGREEN);
  setTextSize(2);
  SetXY(0,(Screen.cx-12*8)/2,Screen.cy/2-30);
  print("BREAKTHROUGH");
  SetXY(0,(Screen.cx-4*8)/2,Screen.cy/2-5);
  sprintf(buf,"%u.%02u",BREAKTHROUGH_VERSION_H,BREAKTHROUGH_VERSION_L);
  print(buf);
  setTextSize(1);
  SetColors(LIGHTGRAY,desktopColor,0);
  SetXY(0,(Screen.cx-13*8)/2,Screen.cy/2+54);
  print("(C) 1988-2024");
  SetColors(LIGHTGRAY,desktopColor,0);
//  setTextSize(1); fare font + piccolo!
  SetXY(0,(Screen.cx-sizeof(BREAKTHROUGH_COPYRIGHT_STRING)*8)/2,Screen.cy/2+66);
  print(BREAKTHROUGH_COPYRIGHT_STRING);
  SetColors(BRIGHTCYAN,desktopColor,0);    // 
  setTextSize(2);
  sprintf(buf,"%uKB/%uKB available",getTotRAM()/1024,getTotExtRAM()/1024);
  SetXY(0,(Screen.cx-strlen(buf)*8)/2,Screen.cy/2+20);
  print(buf);
  SetXY(0,0,Screen.cy/2+90);
  SetColors(LIGHTRED,desktopColor,0);    // 
	if(!(GetStatus(SOUTH_BRIDGE,NULL) & 0b01000101)) 		// 
		err_puts("Keyboard not present");
	if(eXtra & 1 || !(GetStatus(SOUTH_BRIDGE,NULL) & 0b10001010)) {   //
    err_puts("No mouse found"); 
    eXtra |= 1 /*così funziano frecce tastiera!*/;
//    cursorCnt=-1;   // dice il doc, ma lasciando l'emulazione da tastiera lo tolgo!
    }
  else
    cursorCnt=0;

#endif
  
//        m_stdout=m_stdin=m_stderr=DEVICE_WINDOWS;   // mettere, direi, dopo debug

  __delay_ms(2000);
  KBClear();
  ClrWdt();
  mousePosition.x=Screen.cx/2; mousePosition.y=Screen.cy/2;
  mouseCursor=standardCursorSm;
  CreateSprite(0,8,8,mousePosition.x,mousePosition.y,0,0,1  | 8 /* trasparenza */,SPRITE_SETALL);
  DefineSprite(0,(GFX_COLOR*)standardCursorSm);			//
  
  eXtra=GetProfileInt(WINDOWS_STR,"EXTRA",eXtra);   // va bene qua (v.sopra)
  if(eXtra & 16 && GetID(AUDIO_CARD) == 'A') {
    /// fare musichette :D
    SetAudioWave(0,1,440,0,8,getVolumeAudio()*3/4,0,0);
    __delay_ms(300);
    SetAudioWave(0,1,480,0,8,getVolumeAudio()*3/4,0,0);
    __delay_ms(300);
    SetAudioWave(0,1,540,0,8,getVolumeAudio()*4/5,0,0);
    __delay_ms(300);
    SetAudioWave(0,0,0,0,8,getVolumeAudio(),0,0);
    }
  
//  cs.dwExStyle=hWnd->styleEx;
  char buf1[32];
  BYTE sfondoTiled;
  sfondoTiled=GetProfileInt(DESKTOP_STR,"TILEWALLPAPER",0);
  if(GetProfileString(DESKTOP_STR,"WALLPAPER","",buf1,31)) {
    sfondo=buf1;
    }
  else {
    if(sfondo) {
      switch(*sfondo) {
        case '1':
          sfondo="w95train.jpg";
          break;
        case '2':
          sfondo="w95lion.jpg";
          break;
        case '3':
          sfondo="windowxp.jpg";
          break;
        case '4':
          sfondo="w3tartan.jpg";
          break;
        default:
          sfondo="prova";
          break;
        }
      }
    else {
      i=15;
      GetUserName(buf1,(BYTE*)&i); 
      sfondo=buf1;
      }
    }
  cs.lpCreateParams=(char*)sfondo;
  SetWindowByte(desktopWindow,sizeof(POINTS)*16+16+16+4+4,sfondoTiled);  //v. cmq Create..
  if(!SendMessage(desktopWindow,WM_NCCREATE,0,(LPARAM)&cs))
    return 0;
  if(SendMessage(desktopWindow,WM_CREATE,0,(LPARAM)&cs) < 0)
    return 0;
  GetPrivateProfileString(BOOT_STR,"SCRNSAVE.EXE","BREAKTHROUGH",(char *)GET_WINDOW_OFFSET(desktopWindow,sizeof(POINTS)*16)+16,15,profileFile2);

  desktopWindow->active=1;

  cs2.lpCreateParams=(void*)1;    // 1=orologio, 2=on top
  if(!SendMessage(taskbarWindow,WM_NCCREATE,0,(LPARAM)&cs2))
    return 0;
  if(SendMessage(taskbarWindow,WM_CREATE,0,(LPARAM)&cs2) < 0)
    return 0;
  taskbarWindow->active=1;

#ifndef USING_SIMULATOR
  PaintDesktop(NULL,0);
//  InvalidateRect(desktopWindow,NULL,TRUE);
#endif
  ClrWdt();
  // oppure 
  // rootWindows=desktopWindow=CreateWindow(WC_DESKTOPCLASS,NULL,WS_ACTIVE | WS_VISIBLE,0,0,Screen.cx,Screen.cy,NULL,NULL,0);
  // e anche
  // taskbarWindow=CreateWindow(WC_TASKBARCLASS,"",WS_ACTIVE | WS_VISIBLE,0,Screen.cy-Screen.cy/16,Screen.cx,Screen.cy/16,NULL,NULL,0);
  // NON faccio Create per non inserire nella lista windows!

  DrawCursor(mousePosition.x,mousePosition.y,standardCursorSm,0);

	winManagerThreadID=BeginThread((void *)manageWindows,1,0);

  Yield();
#warning FINIRE AGGIORNARE threads

      // https://jeffpar.github.io/kbarchive/kb/083/Q83433/
//  GetProfileInt(key,"BEEP",1);
//  GetProfileInt(key,"CURSORBLINKRATE",1);
//  GetProfileInt(key,"DOUBLECLICKSPEED",1);
//  GetProfileInt(key,"KEYBOARDDELAY",1);
//  GetProfileInt(key,"KEYBOARDSPEED",1);
//  GetProfileInt(key,"DOUBLECLICKSPEED",1);
  eXtra=(eXtra & 0x7f) | (GetProfileInt(WINDOWS_STR,"SCREENSAVEACTIVE",1) ? 128 : 0);
  screenSaverTime=GetProfileInt(WINDOWS_STR,"SCREENSAVETIMEOUT",120);

	if(!mode) {
		GetProfileString(WINDOWS_STR,"LOAD","",buf1,31);
		if(*buf1) {
			ShellExecute(NULL,"open",buf1,NULL,NULL,SW_SHOWMINIMIZED);   // 
			}
		GetProfileString(WINDOWS_STR,"RUN","",buf1,31);    // più d'una separate da virgola
		if(*buf1) {
			ShellExecute(NULL,"open",buf1,NULL,NULL,SW_SHOWNORMAL);   // coordinate, stato?
			}
		}

  }

void EndWindows(void) {
  HWND w;
  WNDCLASS *c;
  BYTE i;
  
  if(eXtra & 16 && GetID(AUDIO_CARD) == 'A') {
    /// fare musichette :D
    SetAudioWave(0,1,440,0,8,getVolumeAudio()*4/5,0,0);
    __delay_ms(300);
    SetAudioWave(0,1,340,0,8,getVolumeAudio()*3/4,0,0);
    __delay_ms(300);
    SetAudioWave(0,1,280,0,8,getVolumeAudio()*3/4,0,0);
    __delay_ms(300);
    SetAudioWave(0,0,0,0,8,getVolumeAudio(),0,0);
    }
  else {
    StdBeep(200);
    __delay_ms(200);
    StdBeep(100);
    }
  
  CreateSprite(0,0,0,0,0,0,0,0,SPRITE_SETALL);
  for(i=0; i<MAX_TIMERS; i++) {
    timerData[i].uEvent=0;
    timerData[i].hWnd=0;
    timerData[i].timeout=0;
    timerData[i].tproc=NULL;
    timerData[i].time_cnt=0;
    }
  if(desktopWindow) {
    w=rootWindows;
    while(w) {
      HWND w2=w->next;
      DestroyWindow(w);
      w=w2;
      }
    rootWindows=NULL;
    c=rootClasses;
    while(c) {
      WNDCLASS *c2=c->next;
      free(c);
      c=c2;
      }
    rootClasses=NULL;
    free(taskbarWindow->children->next); taskbarWindow->children->next=NULL;  // ok anche se no-orologio, ossia 0!
    free(taskbarWindow->children); taskbarWindow->children=NULL;
    free(taskbarWindow); taskbarWindow=NULL;    // NON si può fare Destoy, non sono in rootWindows...
    free(desktopWindow); desktopWindow=NULL;
    }
  m_WndClock=m_WndCalendar=m_WndCalc=m_WndFileManager[0]=m_WndFileManager[1]=m_WndTaskManager=m_WndControlPanel=
        m_WndBasic=m_WndSurf=m_WndViewer=m_WndNotepad=NULL;
  EndThread(winManagerThreadID);
  }

BOOL IsWindowUnicode(HWND hWnd) { return FALSE; }   //;)

BOOL ScreenToClient(HWND hWnd,POINT *lpPoint) {
  if(hWnd) do {     // mia estensione per Desktop...
    OffsetPoint(lpPoint,-hWnd->nonClientArea.left,-hWnd->nonClientArea.top);
    OffsetPoint(lpPoint,-BORDER_SIZE,-BORDER_SIZE);
    if(hWnd->style & WS_CAPTION)
      OffsetPoint(lpPoint,0,-TITLE_HEIGHT);
    if(hWnd->menu && (hWnd->style & WS_CHILD))
      OffsetPoint(lpPoint,0,-MENU_HEIGHT);
    hWnd=hWnd->parent;
    } while(hWnd);
  return 1;
  }
//v. anche AdjustWindowRect...
BOOL ClientToScreen(HWND hWnd,POINT *lpPoint) {
  if(hWnd) do {     // mia estensione per Desktop...
    OffsetPoint(lpPoint,hWnd->nonClientArea.left,hWnd->nonClientArea.top);
    OffsetPoint(lpPoint,BORDER_SIZE,BORDER_SIZE);
    if(hWnd->style & WS_CAPTION)
      OffsetPoint(lpPoint,0,TITLE_HEIGHT);
    if(hWnd->menu && (hWnd->style & WS_CHILD))
      OffsetPoint(lpPoint,0,MENU_HEIGHT);
    hWnd=hWnd->parent;
    } while(hWnd);
  return 1;
  }
int MapWindowPoints(HWND hWndFrom,HWND hWndTo,POINT *lpPoints,BYTE cPoints) {
  POINT pt={hWndTo->nonClientArea.left-hWndFrom->nonClientArea.left,
    hWndTo->nonClientArea.top-hWndFrom->nonClientArea.top};
  
  while(cPoints--) {
    OffsetPoint(lpPoints++,pt.x,pt.y);
    }
  return MAKELPARAM(pt.x,pt.y);
  }

GFX_COLOR GetSysColor(int nIndex) {
  
  switch(nIndex) {
    case COLOR_ACTIVECAPTION:
    case COLOR_ACTIVEBORDER:
    case COLOR_BTNFACE:
    case COLOR_CAPTIONTEXT:
    case COLOR_BTNTEXT:
    case COLOR_MENU:
    case COLOR_MENUBAR:
    case COLOR_SCROLLBAR:
    case COLOR_WINDOWFRAME:
    case COLOR_WINDOWTEXT:
      return windowForeColor;
      break;
    case COLOR_GRAYTEXT:
    case COLOR_INACTIVEBORDER:
    case COLOR_INACTIVECAPTION:
    case COLOR_INACTIVECAPTIONTEXT:
      return windowInactiveForeColor;
      break;
    case COLOR_WINDOW:
      return windowBackColor;
      break;
//    case COLOR_BACKGROUND:
    case COLOR_DESKTOP:
      return desktopColor;
      break;
    }
  }

GFX_COLOR GetSystemMetrics(int nIndex) {
  
  switch(nIndex) {
    case SM_CXSCREEN:
      return Screen.cx;
      break;
    case SM_CYSCREEN:
      return Screen.cy;
    case SM_CXVSCROLL:
      return SCROLL_SIZE;
      break;
    case SM_CYHSCROLL:
      return SCROLL_SIZE;
      break;
    case SM_CYCAPTION:
      return TITLE_HEIGHT;
      break;
    case SM_CXBORDER:
      return 1;     //BORDER_SIZE
      break;
    case SM_CYBORDER:
      return 1;   //BORDER_SIZE;
      break;
    case SM_CYVTHUMB:
      break;
    case SM_CXHTHUMB:
      break;
    case SM_CXICON:
      return 8;
      break;
    case SM_CYICON:
      return 8;
      break;
    case SM_CXCURSOR:
      return 8;
      break;
    case SM_CYCURSOR:
      return 8;
      break;
    case SM_CYMENU:
      return MENU_HEIGHT;
      break;
    case SM_CXFULLSCREEN:
      return Screen.cx;
      break;
    case SM_CYFULLSCREEN:
      return Screen.cy;
      break;
    case SM_CYKANJIWINDOW:
      return FALSE;
      break;
    case SM_MOUSEPRESENT:
      return TRUE;
      break;
    case SM_CYVSCROLL:
      break;
    case SM_CXHSCROLL:
      break;
    case SM_DEBUG:
#ifdef _DEBUG
      return TRUE;
#else
      return FALSE;
#endif
      break;
    case SM_SWAPBUTTON:
      return FALSE;
      break;
    case SM_RESERVED1:
      return 0;
      break;
    case SM_RESERVED2:
      return 0;
      break;
    case SM_RESERVED3:
      return 0;
      break;
    case SM_RESERVED4:
      return 0;
      break;
    case SM_CXMIN:
      return 20;
      break;
    case SM_CYMIN:
      return 30;
      break;
    case SM_CXSIZE:
      return TITLE_HEIGHT-1-1;
      break;
    case SM_CYSIZE:
      return TITLE_HEIGHT-1-1;
      break;
//    case SM_CXDLGFRAME:
//    case SM_CXSIZEFRAME:
    case SM_CXFRAME:
      return 2;
      break;
//    case SM_CYDLGFRAME:
//    case SM_CYSIZEFRAME:
    case SM_CYFRAME:
      return 2;
      break;
    case SM_CXMINTRACK:
      return 10;
      break;
    case SM_CYMINTRACK:
      return 10;
      break;
    case SM_CXDOUBLECLK:
      break;
    case SM_CYDOUBLECLK:
      break;
    case SM_CXICONSPACING:
      return 16+4;
      break;
    case SM_CYICONSPACING:
      return 16+4;
      break;
    case SM_MENUDROPALIGNMENT:
      break;
    case SM_PENWINDOWS:
    case SM_TABLETPC:
      return FALSE;
      break;
    case SM_DBCSENABLED:
      return FALSE;
      break;
    case SM_CMOUSEBUTTONS:
      return 2;   // v. south bridge per ora
      break;
    case SM_CXFIXEDFRAME:
      break;
    case SM_CYFIXEDFRAME:
      break;
    case SM_SECURE:
      return FALSE;
      break;
    case SM_CXEDGE:
      return SM_CXBORDER+1;   // uguale ma per 3D...
      break;
    case SM_CYEDGE:
      return SM_CYBORDER+1;
      break;
    case SM_CXMINSPACING:
      break;
    case SM_CYMINSPACING:
      break;
    case SM_CXSMICON:
      return 8;
      break;
    case SM_CYSMICON:
      return 8;
      break;
    case SM_CYSMCAPTION:
      break;
    case SM_CXSMSIZE:
      break;
    case SM_CYSMSIZE:
      break;
    case SM_CXMENUSIZE:
      break;
    case SM_CYMENUSIZE:
      return MENU_HEIGHT;
      break;
    case SM_ARRANGE:
      break;
    case SM_CXMINIMIZED:
      break;
    case SM_CYMINIMIZED:
      break;
    case SM_CXMAXTRACK:
      //boh?
      break;
    case SM_CYMAXTRACK:
      break;
    case SM_CXMAXIMIZED:
      return Screen.cx;
      break;
    case SM_CYMAXIMIZED:
      return Screen.cy;
      break;
    case SM_NETWORK:
      return TRUE;
      return FALSE;
      break;
    case SM_CLEANBOOT:
#ifdef _DEBUG
      return TRUE;
#else
      return FALSE;
#endif
      break;
    case SM_CXDRAG:
      break;
    case SM_CYDRAG:
      break;
    case SM_SHOWSOUNDS:
      return FALSE;
      break;
    case SM_CXMENUCHECK:
      return 8;   // fisso...
      break;
    case SM_CYMENUCHECK:
      return 8;
      break;
    case SM_SLOWMACHINE:
      return FALSE; //:D suca
      break;
    case SM_MIDEASTENABLED:
      return FALSE;   // porcoallah :D :D
      break;
    case SM_MOUSEWHEELPRESENT:
      return FALSE;
      break;
    case SM_XVIRTUALSCREEN:
      return FALSE;
      break;
    case SM_YVIRTUALSCREEN:
      return FALSE;
      break;
    case SM_CXVIRTUALSCREEN:
      return FALSE;
      break;
    case SM_CYVIRTUALSCREEN:
      return FALSE;
      break;
    case SM_CMONITORS:
      return 1;
      break;
    case SM_SAMEDISPLAYFORMAT:
      return FALSE;
      break;
    case SM_CMETRICS:
      return ;
      break;
    }
  }

HWND GetDesktopWindow(void) { return desktopWindow; }
HWND GetShellWindow(void) { return taskbarWindow; }   // o forse è sempre il desktop? https://stackoverflow.com/questions/8364758/get-handle-to-desktop-shell-window
HWND GetRootWindow(void) { return rootWindows; }    // uso interno...

int GetDeviceCaps(HDC hDC,int index) {
  
  switch(index) {
    case DRIVERVERSION:
      return MAKEWORD(BREAKTHROUGH_VERSION_L,BREAKTHROUGH_VERSION_H);
      break;
    case TECHNOLOGY:
      break;
    case HORZSIZE:
    case HORZRES:
      return Screen.cx;
      break;
    case VERTRES:
    case VERTSIZE:
      return Screen.cy;
      break;
    case BITSPIXEL:
      return _bpp;
      break;
    case PLANES:
      return 3;
      break;
    case NUMBRUSHES:
      return 1;
      break;
    case NUMPENS:
      return 1;
      break;
    case NUMMARKERS:
      return 0;
      break;
    case NUMFONTS:
//      #ifdef USE_CUSTOM_FONTS 
        return 5;
//      #else
//        return 2;
//      #endif
      break;
    case NUMCOLORS:
      return 1L << _bpp;
      break;
    case CURVECAPS:
      return 1;
      break;
    case LINECAPS:
      return 1;
      break;
    case POLYGONALCAPS:
      return 1;
      break;
    case TEXTCAPS:
      return 1;
      break;
    case CLIPCAPS:
      return 1;
      break;
    case RASTERCAPS:
      return 1;
      break;
    case VREFRESH:
      return 50;
      break;
      
    default:
      break;
    }
  }

BOOL SwapMouseButton(BOOL fSwap) {
  
  }

BOOL SetCursorPos(UGRAPH_COORD_T x,UGRAPH_COORD_T y) {

	//ClipCursor...
  mousePosition.x=x; mousePosition.y=y;
  MovePointer(x,y);
  }
BOOL SetPhysicalCursorPos(UGRAPH_COORD_T X,UGRAPH_COORD_T Y) {
  
  // v. mapping LP DP ecc
  mousePosition.x=X; mousePosition.y=Y;
  MovePointer(X,Y);
  }
BOOL GetCursorPos(POINT *lpPoint) {
  POINT pt;
  pt.x=mousePosition.x;
  pt.y=mousePosition.y;
  *lpPoint=pt;
  }
CURSOR SetCursor(CURSOR hCursor) {
  
  if(mouseCursor != hCursor) {
    mouseCursor=hCursor;
    DefineSprite(0,(GFX_COLOR*)mouseCursor);
    }
  }
BOOL SetSystemCursor(CURSOR hcur,DWORD id) {
  int i=0;
  
  //NO!! non fa questo, https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setsystemcursor
  // serve a cambiare un cursor di sistema...
  switch(id) {
    case OCR_NORMAL:
      SetCursor(standardCursorSm);
      i=1;
      break;
    case OCR_IBEAM:
      i=1;
      break;
    case OCR_WAIT:
      SetCursor(hourglassCursorSm);
      i=1;
      break;
    case OCR_CROSS:
      SetCursor(crossCursorSm);
      i=1;
      break;
    case OCR_UP:
      i=1;
      break;
    case OCR_SIZE:
      i=1;
      break;
    case OCR_ICON:
      i=1;
      break;
    case OCR_SIZENWSE:
      i=1;
      break;
    case OCR_SIZENESW:
      i=1;
      break;
    case OCR_SIZEWE:
      i=1;
      break;
    case OCR_SIZENS:
      i=1;
      break;
    case OCR_SIZEALL:
      i=1;
      break;
    case OCR_NO:
      i=1;
      break;
    case OCR_APPSTARTING:
      i=1;
      break;
    }
  return i;
  }
BOOL ClipCursor(const RECT *lpRect) {
  if(!lpRect)     // fare
    clipCursorRect=*lpRect;
  return TRUE;
  }
BOOL SetDoubleClickTime(uint16_t t) {
  doubleClickTime=t;
  }
HWND SetCapture(HWND hWnd) {
  HWND myWnd=captureWindow;
  captureWindow=hWnd;
  return myWnd;
  }
BOOL ReleaseCapture(HWND hWnd) {
  HWND myWnd=captureWindow;
  captureWindow=NULL;
  return myWnd != NULL;
  }
BOOL SetCaretPos(UGRAPH_COORD_T X,UGRAPH_COORD_T Y) {
  caretPosition.x=X;
  caretPosition.y=Y;
  }
BOOL GetCaretPos(POINT *lpPoint) {
  lpPoint->x=caretPosition.x;
  lpPoint->y=caretPosition.y;
  }
BOOL SetCaretBlinkTime(uint16_t uMSeconds) {
  caretTime=uMSeconds/10;
  }
uint16_t GetCaretBlinkTime() {
  return caretTime*10;
  }
BOOL ShowCaret(HWND hWnd) {
//  drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,TRUE);
  }
BOOL HideCaret(HWND hWnd) {
  // GetFocus() non so se è giusta... verificare o salvare da qualche parte (anche per timer=
//  drawCaret(hWnd,caretPosition.x,caretPosition.y,NULL,FALSE);   // non è proprio così, v. trasparenze ecc
  }
BOOL TrackMouseEvent(TRACKMOUSEEVENT *lpEventTrack) {
  }


static HWND internalCreateWindow(CLASS Class,const char *lpWindowName,
  DWORD dwStyle,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,
  HWND hWndParent,HMENU Menu,void *lpParam,WINDOWPROC wndProc) {
  HWND hWnd;
  WNDCLASS *wndClass;

  if(!GetClassInfo(0,Class,&wndClass))
    return NULL;

  if(wndClass->style & CS_OWNDC)
    hWnd=malloc(sizeof(struct _WINDOW)+wndClass->cbWndExtra+sizeof(HDC));
  else
    hWnd=malloc(sizeof(struct _WINDOW)+wndClass->cbWndExtra);
  if(!hWnd)
    return NULL;
  hWnd->class=Class;
  hWnd->next=NULL;
  hWnd->messageQueue=NULL;
  hWnd->status=hWnd->internalState=0;
  if(X==CW_USEDEFAULT)
    X=16+countWindows()*16;
  if(Y==CW_USEDEFAULT)
    Y=16+countWindows()*16;
  hWnd->nonClientArea.left=X;   // v. MINMAXINFO sotto
  hWnd->nonClientArea.top=Y;
  if(wndClass->style & CS_BYTEALIGNWINDOW) {
    nWidth = (nWidth + 7) & 0xfff8;   // boh credo :)
    }
  hWnd->nonClientArea.right=X+nWidth;   // 
  if(X==CW_USEDEFAULT)
		hWnd->nonClientArea.right=min(hWnd->nonClientArea.right,Screen.cx-1);
  hWnd->nonClientArea.bottom=Y+nHeight;
  if(Y==CW_USEDEFAULT)
		hWnd->nonClientArea.bottom=min(hWnd->nonClientArea.bottom,Screen.cy-1);
  hWnd->scrollPosX=hWnd->scrollPosY=0; hWnd->scrollSizeX=hWnd->scrollSizeY=100;   // default, dice
  hWnd->icon=wndClass->icon;
  hWnd->font=GetStockObject(SYSTEM_FONT).font;  /*cmq lo stock? o anche custom?? */;
  hWnd->cursor=wndClass->cursor;
  hWnd->windowProc=wndProc ? wndProc : wndClass->lpfnWndProc;
  if(lpWindowName) {
    strncpy(hWnd->caption,lpWindowName,sizeof(hWnd->caption)-1);
    hWnd->caption[sizeof(hWnd->caption)-1]=0;
    }
  else
    hWnd->caption[0]=0;
  hWnd->style=dwStyle;    // 
//  hWnd->styleEx=0;
  hWnd->menu=Menu ? Menu : (dwStyle & WS_CHILD ? 0 : wndClass->menu);   // NO se CHILD
  hWnd->children=NULL;
  if(hWndParent) {
//    hWnd->parent=hWndParent->style & WS_CHILD ? hWndParent->parent : hWndParent; // andrebbe fatto ricorsivo a salire.. ??
    hWnd->parent=hWndParent; // però mi incasino! FINIRE! 4/2/23 boh, non so bene cosa volesse essere, mi pare ok
    }
  else
    hWnd->parent=NULL;
  if(dwStyle & WS_CHILD) {    // safety
		dwStyle &= ~(WS_EX_TOPMOST /*| WS_ACTIVE mah, e poi children??*/);
    hWnd->topmost=0;
    }
  
  calculateNonClientArea(hWnd,&hWnd->nonClientArea);    // serve sia prima che dopo, per corretta gestione in WM_CREATE e per ev. modificare dopo CREATESTRUCT..
  calculateClientArea(hWnd,&hWnd->clientArea);
  {CREATESTRUCT cs;
  cs.x=hWnd->nonClientArea.left;
  cs.y=hWnd->nonClientArea.top;
  cs.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
  cs.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
  cs.class=Class;
  cs.lpszName=hWnd->caption;
  cs.menu=hWnd->menu;
  cs.style=hWnd->style;
//  cs.dwExStyle=hWnd->styleEx;
  cs.lpCreateParams=lpParam;
  if(hWnd->style & WS_CHILD && hWnd->style & WS_POPUP) {
    return NULL;// non è consentito...
    }
  MINMAXINFO mmi;
//    mmi.ptMaxPosition
  SendMessage(hWnd,WM_GETMINMAXINFO,0,(LPARAM)&mmi);
  if((int)cs.x<mmi.ptMaxPosition.x)      //  direi ok per il controllo fuori schermo [per il momento...
    cs.x=mmi.ptMaxPosition.x;
  if(cs.cx>=mmi.ptMaxSize.x)
    cs.cx=mmi.ptMaxSize.x-1;
  if((int)cs.y<mmi.ptMaxPosition.y) //
    cs.y=mmi.ptMaxPosition.y;
  if(cs.cy>=mmi.ptMaxSize.y)
    cs.cy=mmi.ptMaxSize.y-1;

  if(!SendMessage(hWnd,WM_NCCREATE,0,(LPARAM)&cs))
    goto no_creata;
  if(SendMessage(hWnd,WM_CREATE,0,(LPARAM)&cs) < 0) {
no_creata:
    free(hWnd);
    return NULL;
    }
  hWnd->style=cs.style;
  hWnd->menu=cs.menu;
  hWnd->nonClientArea.left=cs.x;
  hWnd->nonClientArea.top=cs.y;
  hWnd->nonClientArea.right=hWnd->nonClientArea.left+cs.cx;
  hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+cs.cy;
  }
  calculateNonClientArea(hWnd,&hWnd->nonClientArea);
  calculateClientArea(hWnd,&hWnd->clientArea);
  if(hWnd->style & WS_POPUP) {
    }
  hWnd->enabled=hWnd->style & WS_DISABLED ? 0 : 1;
  if(!(hWnd->style & WS_CHILD)) {
    if(hWnd->enabled) {
      HWND myWnd=GetForegroundWindow();
      hWnd->zOrder= myWnd ? myWnd->zOrder+1 : 1;
      addTailWindow(hWnd,&rootWindows);
      }
    else {
      hWnd->zOrder=1;
      addHeadWindow(hWnd,&rootWindows);    // così rispettiamo lo Z-order subito!
      }
    }
  else {
    HWND myWnd=getLastChildWindow(hWnd->parent);
    hWnd->zOrder= myWnd ? myWnd->zOrder+1 : 1;
    addTailWindow(hWnd,&hWnd->parent->children);
    }
  if(hWnd->style & WS_EX_TOPMOST)
    hWnd->topmost=1;

  if(hWnd->style & WS_VISIBLE) {
    hWnd->visible=1;
    if(!(hWnd->style & WS_CHILD)) {
      if(!(hWnd->style & WS_DISABLED)) {
        HWND myWnd=GetFocus();
        if(myWnd)
          SendMessage(myWnd,WM_KILLFOCUS,(WPARAM)myWnd,0);
        SendMessage(hWnd,WM_SETFOCUS,(WPARAM)hWnd,0);
        // https://www.gamedev.net/forums/topic/323415-first-win32-message-sent/
        ShowWindow(hWnd,SW_SHOW);
        }
      else
        ShowWindow(hWnd,SW_SHOWNOACTIVATE);
      
      // credo così, perché una finestra appena creata NON viene ridisegnata da ShowWindow essendo già top e active
      SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);  // dice anche ERASEBKGND qua, ma io lo faccio in PAINT...
      InvalidateRect(hWnd,NULL,TRUE);
      }
    else {    // ci pensa parentnotify
      }
    }
  
  if(hWnd->style & WS_CHILD) {
  //if(!WS_EX_NOPARENTNOTIFY) mettere
    SendMessage(hWnd->parent,WM_PARENTNOTIFY,MAKELONG(WM_CREATE,(WORD)(DWORD)hWnd->menu),(LPARAM)hWnd);   // tranne WS_EX_NOPARENTNOTIFY  ..
    }
  
  if(!(hWnd->style & WS_CHILD) && taskbarWindow)
    InvalidateRect(taskbarWindow,NULL,TRUE);    // crea icona; e anche se non visibile.. fa da antivirus :)
          
  return hWnd;
  }
HWND CreateWindow(CLASS Class,const char *lpWindowName,
  DWORD dwStyle,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,
  HWND hWndParent,HMENU Menu,void *lpParam) {
  
  return internalCreateWindow(Class,lpWindowName,dwStyle,X,Y,nWidth,nHeight,
    hWndParent,Menu,lpParam,NULL);
  }
HWND CreateWindowEx(DWORD dwExStyle,CLASS Class,const char *lpWindowName,
  DWORD dwStyle,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,
  HWND hWndParent,HMENU Menu,void *lpParam) {   

//If you specify the WS_CHILD style in CreateWindowEx but do not specify a parent window, the system does not create the window.
  if(dwStyle & WS_CHILD && !hWndParent)
    return NULL;
  HWND w=internalCreateWindow(Class,lpWindowName,dwStyle,X,Y,nWidth,nHeight,
    hWndParent,Menu,lpParam,NULL);
  if(w)
    ;     // w->dwExStyle=dwExStyle;
  return w;
  }

static BOOL addTailWindow(HWND hWnd,struct _WINDOW **root) {   // per insertSorted: https://www.geeksforgeeks.org/given-a-linked-list-which-is-sorted-how-will-you-insert-in-sorted-way/
  HWND myWnd;
  
  if(!*root)
    *root=hWnd;
  else {
    myWnd=*root;
    while(myWnd && myWnd->next) {
      myWnd=myWnd->next;
      }
    myWnd->next=hWnd;
    }
  return 1;
  }
static BOOL addHeadWindow(HWND hWnd,struct _WINDOW **root) {
  HWND myWnd;
  
  if(!*root)
    *root=hWnd;
  else {
    myWnd=hWnd->next=*root;
    *root=hWnd;
    (*root)->zOrder=1;    // questa diventa la più indietro...
    while(myWnd) {
      myWnd->zOrder++;    // ...e le scrollo tutte avanti di uno
      myWnd=myWnd->next;
      }
    }
  return 1;
  }
static BOOL insertWindow(HWND hWnd,HWND hWndAfter,struct _WINDOW **root) {
  HWND myWnd,myWnd2;
  
  removeWindow(hWnd,root);
  myWnd=*root;
  while(myWnd) {
    if(myWnd == hWndAfter) {
      myWnd2=hWndAfter->next;
      hWndAfter->next=hWnd;
      hWnd->next=myWnd2;
      return 1;
      }
    myWnd=myWnd->next;
    }
  return 0;
  }
static BOOL removeWindow(HWND hWnd,struct _WINDOW **root) {
  HWND myWnd,myWnd2=NULL;
  // https://stackoverflow.com/questions/47491406/how-to-delete-a-node-in-a-linked-list

  if(hWnd==*root) {
    *root=hWnd->next;
    myWnd=*root;
		goto riordina;
		}

  myWnd=*root;
  while(myWnd && myWnd != hWnd) {
    myWnd2=myWnd;
    myWnd=myWnd->next;
		}
	if(myWnd2) {
		myWnd2->next=myWnd ? myWnd->next : NULL;
		myWnd=myWnd2->next;

riordina:
		while(myWnd) {
			myWnd->zOrder--;
			myWnd=myWnd->next;
			}
	  return 1;
		}

  return 0;
  }

LRESULT SendMessage(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  HWND myWnd;

	if(hWnd != HWND_BROADCAST) {
		return hWnd->windowProc(hWnd,message,wParam,lParam);   
//    __asm__("j": "=c" (hWnd->windowProc));   // VERIFICARE!
    }
  else {
    myWnd=rootWindows;
    while(myWnd) {
			myWnd->windowProc(myWnd,message,wParam,lParam);
      myWnd=myWnd->next;
      }
    if(taskbarWindow)   // mah, diciamo.. NON dovrebbe essere coperto, ma anche solo per aggiornare le icone
      // o controllare ON TOP?? in generale
			taskbarWindow->windowProc(taskbarWindow,message,wParam,lParam);   
    //NO DESKTOP!
    }
  }
void PostQuitMessage(int nExitCode) {
//  EndThread();
  }
LRESULT CallWindowProc(WINDOWPROC lpPrevWndFunc,HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam) {
  (*lpPrevWndFunc)(hWnd,Msg,wParam,lParam); // o GetWindowLong(GWL_WNDPROC ... è lo stesso
  }
LRESULT DefWindowProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_NCPAINT:
      //wParam = region
      return !nonClientPaint(hWnd,(RECT *)wParam,hWnd->active);   // 0 if it processes
      break;
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);    // SERVE cmq per il background
      EndPaint(hWnd,&ps);
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
//#warning			occhio a children si inculano :D
      }
      break;
    case WM_SYNCPAINT:
      SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
      SendMessage(hWnd,WM_PAINT,(WPARAM)NULL,0);
      break;
    case WM_PRINT:
      {HDC hDC=(HDC)wParam; // v. flags lParam PRF_CLIENT ecc
      // stampare la finestra!
      if(lParam & PRF_ERASEBKGND)
        SendMessage(hWnd,WM_ERASEBKGND,wParam,0);
      if(lParam & PRF_CLIENT)
        SendMessage(hWnd,WM_PRINTCLIENT,wParam,0);
      if(lParam & PRF_OWNED)
        ;
      }
      break;
    case WM_PRINTCLIENT:
      {HDC hDC=(HDC)wParam; // v. flags lParam PRF_CLIENT ecc
      // stampare la finestra!
      }
      break;
    case WM_SETREDRAW:
      hWnd->locked= !wParam;
      break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOR:   // è deprecated ma per ora lo uso!
      return 0xffffffff;   // 
      break;
    case WM_DRAWITEM:
      {
      DRAWITEMSTRUCT *dis=(DRAWITEMSTRUCT*)lParam;
      //wparam CtlID; If the message was sent by a menu, this parameter is zero.
      drawRectangleWindowColor(dis->hDC,dis->rcItem.left,dis->rcItem.top,
        dis->rcItem.right,dis->rcItem.bottom,PERU); // tanto per!
      }
      break;
    case WM_MEASUREITEM:
      {
      MEASUREITEMSTRUCT *mis=(MEASUREITEMSTRUCT*)lParam;
      //wparam CtlID;  If the message was sent by a menu, this parameter is zero. If the value is nonzero or the value is zero and the value of the CtlType member of the MEASUREITEMSTRUCT pointed to by lParam is not ODT_MENU, the message was sent by a combo box or by a list box. If the value is nonzero, and the value of the itemID member of the MEASUREITEMSTRUCT pointed to by lParam is (UINT) 1, the message was sent by a combo edit field.
      }
      break;
    case WM_COMPAREITEM:
      {
      COMPAREITEMSTRUCT *cis=(COMPAREITEMSTRUCT*)lParam;
      //wparam CtlID; 
      }
      break;
    case WM_PARENTNOTIFY:   // dovrebbe essere ricorsiva a salire, dice... (o forse solo per i click?)
      switch(LOWORD(wParam)) {
        case WM_CREATE:
          {HWND myWnd=(HWND)lParam;
          if(myWnd->visible) {
            SendMessage(myWnd,WM_NCPAINT,(WPARAM)NULL,0);    // di solito le child non ce l'hanno ma magari MDI
            InvalidateRect(myWnd,NULL,TRUE);
            }
          }
          break;
        case WM_DESTROY:
          {RECT rc;
          HWND myWnd=(HWND)lParam;
          GetWindowRect(myWnd,&rc);
          InvalidateRect(hWnd,NULL /*&rc*/,TRUE); //ovviamente andrebbe ottimizzata, gestita l'area...
          }
          break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
          {POINT pt={LOWORD(lParam),HIWORD(lParam)};   // che froci, ci faceva schifo passare la window qua!
//          HWND myWnd=WindowFromPoint(pt);
          LRESULT r=MA_NOACTIVATE;
          if(hWnd->enabled && hWnd->visible && !hWnd->active) {
//            SendMessage(hWnd,WM_MOUSEACTIVATE,wParam,MAKELONG(HTCLIENT,message));   // bah direi
//#warning serve ancora??
            r=MA_ACTIVATE;
            }
          return r;
          }
          break;
        }
//      if(hWnd->parent)
//        SendMessage(hWnd->parent,message,wParam,lParam);
      break;
    case WM_GETICON:
      //wparam=size, lparam=dpi
      return (LRESULT)hWnd->icon;
      break;
    case WM_SETICON:
      if(wParam)
        ;   // large/small
      hWnd->icon=(GFX_COLOR*)lParam;
      break;
    case WM_SETFONT:
      if(wParam)
        hWnd->font=*(FONT*)wParam;
      else
        hWnd->font.font=NULL;
      if(lParam)
        InvalidateRect(hWnd,NULL,TRUE);
      break;
    case WM_GETFONT:
      return hWnd->font.font ? (DWORD)&hWnd->font : 0;
      break;
    case WM_ERASEBKGND:   // http://www.catch22.net/tuts/win32/flicker-free-drawing#
//     If hbrBackground is NULL, the application should process the WM_ERASEBKGND 
      { HDC hDC=(HDC)wParam;
        int c=hDC->brush.color;
        
        if(!hDC->brush.size && hDC->brush.style==BS_NULL)   // If hbrBackground is NULL, the application should process the WM_ERASEBKGND 
          return 0;
        if(hWnd->style & WS_CHILD) {    // anche qua??
          if((c=SendMessage(hWnd->parent,WM_CTLCOLOR,
            wParam,(LPARAM)hWnd)) == 0xffffffff) {
            c=hDC->brush.color;
            }
          }

        // if hDC->flags &  ?? usare per ottimizzare
        //   DrawWindow(hWnd->paintArea.left,hWnd->paintArea.top,hWnd->paintArea.right,hWnd->paintArea.bottom,b,BLACK /*b*/);		// colore?

//        fillRectangleWindowColor(hDC,hWnd->paintArea.left,hWnd->paintArea.top,hWnd->paintArea.right-1,hWnd->paintArea.bottom-1,c);
fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
        return 1;
      }
      break;
    case WM_KEYDOWN:
      {
      int i;
      if(hWnd->enabled) {
        switch(hWnd->internalState) {
          case MSGF_MENU:          //ossia siamo in MENU LOOP
            if(lParam & 0x20000000) {  // ALT
              goto esc_menu;
              }
            switch(wParam) {
              HMENU m;
              BYTE nmax;
              case VK_LEFT:
                if(activeMenu) {   // se c'è sysmenu allora activeMenuCntX va da 0 a nmax includendo sysmenu, altrimenti da 0 a nmax-1
                  nmax=hWnd->menu ? GetMenuItemCount(hWnd->menu) : 0;
                  nmax += hWnd->style & WS_SYSMENU ? 1 : 0;
                  if(activeMenuCntX > 0) {
                    activeMenuCntX--;
                    m=hWnd->menu;
                    goto newmenupopup;
                    }
                  else {
//                    m=hWnd->style & WS_SYSMENU ? (HMENU)&systemMenu : hWnd->menu;
                    activeMenuCntX=nmax-1;
                    for(;;) {
                      if((hWnd->menu->menuItems[activeMenuCntX].flags & (MF_BITMAP | MF_SEPARATOR | MF_POPUP))
                        || !*hWnd->menu->menuItems[activeMenuCntX].text)
                        break;
                      activeMenuCntX--;
                      }
//                    if(hWnd->style & WS_SYSMENU)
//                      activeMenuCntX++;
                    m=hWnd->menu;
                    goto newmenupopup;
                    }
                  }
                break;
              case VK_RIGHT: // 
                if(activeMenu) {   // 
                  nmax=hWnd->menu ? GetMenuItemCount(hWnd->menu) : 0;
                  nmax += hWnd->style & WS_SYSMENU ? 1 : 0;
                  if(activeMenuCntX < (nmax-1)) {
                    activeMenuCntX++;
                    m=hWnd->menu; // lo lascio cmq... magari per subsubmenu
                    
newmenupopup:       {int n=0;
                    activeMenuCntY=0;
                    if(hWnd->style & WS_SYSMENU) {
                      if(!activeMenuCntX) {
                        m=(HMENU)&systemMenu;
                        n=MF_POPUP;
                        }
                      else {
                        n=m->menuItems[activeMenuCntX-1].flags;
                        m=m->menuItems[activeMenuCntX-1].menu;
                        }
                      }
                    else {
                      n=m->menuItems[activeMenuCntX].flags;
                      m=m->menuItems[activeMenuCntX].menu;
                      }
                    SendMessage(hWnd,WM_MENUSELECT,MAKELONG(activeMenuCntX,n),(LPARAM)m);
                //if(MDI) SendMEssage(hWnd,WM_NEXTMENU,0,0)
                    }
                    }
                  else {
                    activeMenuCntX=0;
                    m=hWnd->menu;
                    goto newmenupopup;
                    }
                  }
                break;
              case VK_UP:
                if(activeMenu) {
                  m=activeMenu;
rifomenuup:
                  if(activeMenuCntY>0) {
                    activeMenuCntY--;
                    }
                  else {
                    activeMenuCntY=MAX_MENUITEMS-1;
                    while(!(m->menuItems[activeMenuCntY].flags & (MF_BITMAP | MF_SEPARATOR | MF_POPUP))
                      && !*m->menuItems[activeMenuCntY].text)
                      activeMenuCntY--;
                    }
                  if(m->menuItems[activeMenuCntY].flags & MF_SEPARATOR)
                    goto rifomenuup;
newmenuline:      {int n;
  /*                if(!drawMenuPopupItem(hDC,m,x,y,0))
                    ;
   * MENUSELECT!
                  if(!drawMenuitem(hDC,m,x,y,1))
                    ;*/
                  if(hWnd->style & WS_SYSMENU) {
                    if(!activeMenuCntX) {
                      m=(HMENU)&systemMenu;
                      n=MF_POPUP;
                      }
                    else {
                      n=m->menuItems[activeMenuCntX-1].flags;
                      m=hWnd->menu->menuItems[activeMenuCntX-1].menu;
                      }
                    }
                  else {
                    n=m->menuItems[activeMenuCntX].flags;
                    m=hWnd->menu->menuItems[activeMenuCntX].menu;
                    }
                  SendMessage(hWnd,WM_MENUSELECT,MAKELONG(activeMenuCntX,n),(LPARAM)activeMenu);
                  }
                  }
                break;
              case VK_DOWN:
                if(activeMenu) {
                  m=activeMenu;
rifomenudown:                  
                  if((activeMenuCntY<MAX_MENUITEMS-1) &&
                    ((m->menuItems[activeMenuCntY+1].flags & (MF_BITMAP | MF_SEPARATOR | MF_POPUP)) ||
                    *m->menuItems[activeMenuCntY+1].text)) {
                    activeMenuCntY++;
                    if(m->menuItems[activeMenuCntY].flags & MF_SEPARATOR)
                      goto rifomenudown;
                    }
                  else {
                    activeMenuCntY=0;
                    }
  /*                if(!drawMenuPopupItem(hDC,m,x,y,0))
                    ;
   * MENUSELECT!
                  if(!drawMenupopupItem(hDC,m,x,y,1))
                    ;*/
                  goto newmenuline;
                  }
                break;
              case VK_RETURN:
                if(activeMenu) {  // 
                  m=activeMenu;
                  hWnd->locked=0;     // per consentire ai COMMAND di scrivere...
                  if(!(m->menuItems[activeMenuCntY].flags & (MF_GRAYED | MF_DISABLED))) {
                    if(m->menuItems[activeMenuCntY].flags & MF_POPUP) {
                      // fare
                      }
                    else {
                      if(m->menuItems[activeMenuCntY].flags & MNS_NOTIFYBYPOS) {   // anche qua syscommand? boh no
                        if(activeMenu==&systemMenu)
                          SendMessage(hWnd,WM_MENUCOMMAND,
                            MAKELONG(activeMenuCntY,0),(LPARAM)&systemMenu);
                        else
                          SendMessage(hWnd,WM_MENUCOMMAND,
                            MAKELONG(activeMenuCntY,0),(LPARAM)&m->menuItems[activeMenuCntY]);
                        }
                      else {
                        if(activeMenu==&systemMenu)
                          SendMessage(hWnd,WM_SYSCOMMAND,
                            MAKELONG(m->menuItems[activeMenuCntY].command,0),
                            MAKELONG(0,0));
                        else
                          SendMessage(hWnd,WM_COMMAND,
                            MAKELONG(m->menuItems[activeMenuCntY].command,0),
                            MAKELONG(0,0));
                        }
                      }
                    } 
                  SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0); 
                  }
                break;
              case VK_ESCAPE:
              case VK_F10:
esc_menu:              
                if(activeMenu) {  // 
                  SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);
                  }
                break;
              }
            break;
          case MSGF_DIALOGBOX:          //ossia siamo in Dialog LOOP
            {HWND myWnd;
            myWnd=getActiveChildWindow(hWnd);
            if(myWnd)
              SendMessage(myWnd,message,wParam,lParam);
//            IsDialogMessage(myWnd,&msg); provare, gestire, e v. main loop
            }
            break;
          default:
            switch(wParam) {
              case VK_F10:
                if(hWnd->menu && !(hWnd->style & WS_CHILD)) {
                  SendMessage(hWnd,WM_SYSCOMMAND,SC_KEYMENU,
                    MAKELONG(findMenuLetter(hWnd->menu->menuItems[0].text),0/*mnemonic?*/));
                  }
                break;
              case VK_F9:    // mettere qua anziché fuori?
                break;
              case VK_TAB:    // mettere qua anziché fuori?
                // credo si debba fare come nella dialog...
                break;
              default:
                {HWND myWnd=getFocusHelper(hWnd->children);
                if(myWnd)
                  SendMessage(myWnd,message,wParam,lParam);
                }
                break;
              }
            break;
          }
        }
      }
      break;
    case WM_KEYUP:
      switch(hWnd->internalState) {
        case MSGF_MENU:          //ossia siamo in MENU LOOP
          break;
        case MSGF_DIALOGBOX:          //ossia siamo in Dialog LOOP
          {HWND myWnd=getActiveChildWindow(hWnd);
          if(myWnd)
            SendMessage(myWnd,message,wParam,lParam);
//            IsDialogMessage(myWnd,&msg); provare, gestire, e v. main loop
            }
          break;
        default:
          {HWND myWnd=getFocusHelper(hWnd->children);
          if(myWnd)
            SendMessage(myWnd,message,wParam,lParam);
          }
          break;
        }
      break;
    case WM_CHAR:
      {
      int i;
      if(hWnd->enabled) {
        switch(hWnd->internalState) {
          case MSGF_MENU:          //ossia siamo in MENU LOOP
            break;
          case MSGF_DIALOGBOX:          //ossia siamo in Dialog LOOP
            {HWND myWnd=getActiveChildWindow(hWnd);
            if(myWnd)
              SendMessage(myWnd,message,wParam,lParam);
//            IsDialogMessage(myWnd,&msg); provare, gestire, e v. main loop
            }
            break;
          default:
            {HWND myWnd=getFocusHelper(hWnd->children);
            if(myWnd)
              SendMessage(myWnd,message,wParam,lParam);
            }
            break;
          }
        }
      }
      return 0;
      break;
    case WM_DEADCHAR:
      break;
    case WM_CHARTOITEM:
      //Sent by a list box with the LBS_WANTKEYBOARDINPUT style to its owner in response to a WM_CHAR message.
      break;
    case WM_VKEYTOITEM:
      //Sent by a list box with the LBS_WANTKEYBOARDINPUT style to its owner in response to a WM_KEYDOWN message.
      break;
    case WM_SYSKEYDOWN:
      if(hWnd->enabled) {
        switch(wParam) {
          case VK_SPACE:
            if(hWnd->style & WS_SYSMENU)
              SendMessage(hWnd,WM_SYSCOMMAND,SC_KEYMENU,MAKELONG(' ',0/*mnemonic?*/));
            break;
          case VK_F4:
            SendMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,0);
            break;
          case VK_F9:
// METTERE QUA togliere da loop... forse            SendMessage(hWnd,WM_SYSCOMMAND,MAKELONG(hWnd->minimized ? SC_MAXIMIZE : SC_MINIMIZE,0),0);
            break;
          case VK_MENU:   // ALT arriva anche da solo, prima di SPACE ecc... w. windows
            break;
          default:
            if(isalnum(LOBYTE(wParam))) {
              if(hWnd->menu && !(hWnd->style & WS_CHILD)) {
                BYTE i;
// fare, forse, circa         if(SendMessage(hWnd,WM_MENUCHAR,MAKELONG(LOBYTE(wParam),MF_SYSMENU),(LPARAM)m) == MNC_SELECT)
                // e spostare il loop di ricerca sotto, in wm_menuchar
/*usare! ecc v. altrove                i=SendMessage(hWnd,WM_MENUCHAR,MAKELONG(LOBYTE(wParam),
                  activeMenu == &systemMenu ? MF_SYSMENU : MF_POPUP),(LPARAM)activeMenu);
                switch(HIWORD(i)) {
                  case MNC_EXECUTE:
                    break;
                  case MNC_CLOSE:
                    break;
                  case MNC_SELCT:
                    break;
                  case MNC_IGNORE:
                    MessageBeep(MB_ICONASTERISK);
                    break;
                  }*/
                
                for(i=0; i<MAX_MENUITEMS; i++) {
                  if(!(hWnd->menu->menuItems[i].flags & MF_BITMAP)) {
                    char ch;
                    ch=findMenuLetter(hWnd->menu->menuItems[i].text);
                    if(ch == LOBYTE(wParam)) {
                      if(hWnd->menu->menuItems[i].flags & MF_POPUP) {
                        SendMessage(hWnd,WM_SYSCOMMAND,SC_KEYMENU,MAKELONG(LOBYTE(wParam),0/*mnemonic?*/));
                        }
                      else {//bah c'era qualche coglione che metteva un menu di una sola riga!
                        if(hWnd->menu->menuItems[i].flags & MNS_NOTIFYBYPOS) 
                          SendMessage(hWnd,WM_MENUCOMMAND,i,(LPARAM)&hWnd->menu->menuItems[i]);
                        else
                          SendMessage(hWnd,WM_COMMAND,MAKELONG(hWnd->menu->menuItems[i].command,0),0);
                        }
                      break;
                      }
                    }
                  }
                if(i==MAX_MENUITEMS)    // se non trovato
                  SendMessage(hWnd,WM_MENUCHAR,MAKELONG(LOBYTE(wParam),
                    activeMenu == &systemMenu ? MF_SYSMENU : MF_POPUP),(LPARAM)activeMenu);

                }

              }
            else {
              // ALT F9 ecc arrivano qua!
              }
            break;
          }
        }
      break;
    case WM_SYSKEYUP:
      break;
    case WM_SYSCHAR:
      if(hWnd->enabled) {
        if(isalnum(LOBYTE(wParam))) {
#warning QUESTO SI PUO TOGLIERE MI PARE! basta keydown
          if(hWnd->menu && !(hWnd->style & WS_CHILD)) {
            BYTE i;
            for(i=0; i<MAX_MENUITEMS; i++) {
              if(!(hWnd->menu->menuItems[i].flags & MF_BITMAP)) {
                char ch;
                ch=findMenuLetter(hWnd->menu->menuItems[i].text);
                if(ch == LOBYTE(wParam)) {
                  if(hWnd->menu->menuItems[i].flags & MF_POPUP) {
                    SendMessage(hWnd,WM_SYSCOMMAND,SC_KEYMENU,MAKELONG(LOBYTE(wParam),0/*mnemonic?*/));
                    }
                  else //bah c'era qualche coglione che metteva un menu di una sola riga!
                    SendMessage(hWnd,WM_COMMAND,MAKELONG(hWnd->menu->menuItems[i].command,0),0);
                  break;
                  }
                }
              }
            if(i==MAX_MENUITEMS)    // se non trovato
              SendMessage(hWnd,WM_MENUCHAR,MAKELONG(LOBYTE(wParam),
                activeMenu == &systemMenu ? MF_SYSMENU : MF_POPUP),(LPARAM)activeMenu);
            }
          }
        }
      else {
        if(hWnd->parent && !hWnd->parent->internalState)  // bah non so mica..
          return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      break;
    case WM_SYSDEADCHAR:
      break;
    case WM_NCMOUSEMOVE:
      break;
    case WM_MOUSEMOVE:
      // c'è una possibilità che questo debba a sua volta inviare WM_SETCURSOR...
      //SendMessage(hWnd,WM_SETCURSOR,hWnd,MAKELONG(,WM_MOUSEMOVE));      // wparam=handle to window, L-lparam=hittest H-lparam=wm_mousemove o altro

      break;
    case WM_MOUSEHOVER:
      break;
    case WM_CAPTURECHANGED:
      break;
    case WM_LBUTTONDOWN:
      if(hWnd->enabled) {  // if ! WS_EX_NOPARENTNOTIFY ...
        if(hWnd->parent && hWnd->style & WS_CHILD) {
          POINT pt={LOWORD(lParam),HIWORD(lParam)};   // che froci, ci faceva schifo passare la window qua!
//          ClientToScreen(hWnd,&pt);
//          SendMessage(hWnd->parent,WM_PARENTNOTIFY,WM_LBUTTONDOWN,MAKELPARAM(pt.x,pt.y));
          }
        }
/*mah, no      else {
        if(hWnd->parent && !hWnd->parent->internalState)
          return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }*/
      break;
    case WM_RBUTTONDOWN:
      if(hWnd->enabled) {
        if(hWnd->parent && hWnd->style & WS_CHILD) {
          POINT pt={LOWORD(lParam),HIWORD(lParam)};   // che froci, ci faceva schifo passare la window qua!
//          ClientToScreen(hWnd,&pt);
//          SendMessage(hWnd->parent,WM_PARENTNOTIFY,WM_RBUTTONDOWN,MAKELPARAM(pt.x,pt.y));
          }
        }
      /* idem*/
/*      else {
        if(hWnd->parent && !hWnd->parent->internalState)
          return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }*/
      break;
    case WM_CONTEXTMENU:
      if(!hWnd->internalState)
        {int i=SendMessage(hWnd,WM_NCHITTEST,0,lParam);
        switch(i) {
          case HTCAPTION:
          case HTSYSMENU:
          case HTCLOSE:
          case HTMINBUTTON:
          case HTMAXBUTTON:
            if(TrackPopupMenu((HMENU)&systemMenu,TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON,
              mousePosition.x-hWnd->nonClientArea.left+4,mousePosition.y-hWnd->nonClientArea.top + TITLE_HEIGHT /*lo ficco entro la client così si pulisce bene :D */,
              0,hWnd,NULL)) {
              }
            break;
          case HTVSCROLL:
          case HTHSCROLL:
            if(TrackPopupMenu((HMENU)&systemMenu2,TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON,
              mousePosition.x-hWnd->nonClientArea.left+4,mousePosition.y-hWnd->nonClientArea.top+4,
              0,hWnd,NULL)) {   // non so se va fuori finestra, .. si potrebbe fare RIGHT ALIGN?
              }
            break;
          }
        } 
      else 
        MessageBeep(MB_ICONASTERISK);
      break;
    case WM_MOUSELEAVE:
      break;
    case WM_SETCURSOR:
      // wparam=handle to window, L-lparam=hittest H-lparam=wm_mousemove o altro
      // impostare cursore a class-cursor se in client area
      switch(LOWORD(lParam)) {
        case HTCLIENT:
          if(hWnd->internalState >= MSGF_MOVE)
            SetCursor(crossCursorSm);
          else if(hWnd->cursor)
            SetCursor(hWnd->cursor);
          break;
        case HTLEFT:      // solo se resizable...
        case HTTOP:
        case HTTOPRIGHT:
        case HTRIGHT:
        case HTBOTTOMRIGHT:
        case HTBOTTOM:
        case HTBOTTOMLEFT:
        case HTTOPLEFT:
          SetCursor(hWnd->internalState >= MSGF_MOVE ? crossCursorSm : crossCursorSm);    // finire con NS WE ecc :)
          break;
         
        default:
          SetCursor(hWnd->internalState >= MSGF_MOVE ? crossCursorSm : standardCursorSm);
          break;
        }
      break;
    case WM_TIMER:
      break;
    case WM_TIMECHANGE:
      break;
    case WM_COMPACTING:
      break;
    case WM_HSCROLL:
      break;
    case WM_VSCROLL:
      break;
    case WM_GETMINMAXINFO:
    { MINMAXINFO *mmi=(MINMAXINFO *)lParam;
      mmi->ptMaxSize.x=Screen.cx-1; mmi->ptMaxSize.y=Screen.cy-1;
      mmi->ptMaxPosition.x=0; mmi->ptMaxPosition.y=0;
      mmi->ptMinTrackSize.x=GetSystemMetrics(SM_CXMINTRACK);
      mmi->ptMinTrackSize.y=GetSystemMetrics(SM_CYMINTRACK);
      mmi->ptMaxTrackSize.x=GetSystemMetrics(SM_CXMAXTRACK);
      mmi->ptMaxTrackSize.y=GetSystemMetrics(SM_CYMAXTRACK);
    }
      break;
    case WM_NCCALCSIZE:
      if(wParam) {
        struct NCCALCSIZE_PARAMS *ncp=(struct NCCALCSIZE_PARAMS *)lParam;
				// client rectangle...
				return WVR_REDRAW   /*| WVR_ALIGNTOP ecc*/;
        }
      else {
        RECT *rc=(RECT *)lParam;
        calculateNonClientArea(hWnd,rc);
        calculateClientArea(hWnd,rc);
				return 0;
        }
      break;
    case WM_NCHITTEST:    //https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest
      {POINT pt;
      RECT rcBorder=hWnd->nonClientArea;
      rcBorder.left+=BORDER_SIZE;
      rcBorder.top+=BORDER_SIZE;
      rcBorder.right-=BORDER_SIZE;
      rcBorder.bottom-=BORDER_SIZE;
      pt.x=GET_X_LPARAM(lParam); pt.y=GET_Y_LPARAM(lParam);
      if(hWnd->minimized) {     // quindi CHILD, forse MDI poi
        if(wndPtInRect(&rcBorder,pt)) {
          return HTCLIENT;
          }
        else {
          return HTNOWHERE /*HTBORDER*/;
          }
        }
      if(hWnd->style & WS_CHILD) {
        HWND myWnd=hWnd;
        while(myWnd->parent) {
          pt.x -= myWnd->parent->clientArea.left;
          pt.y -= myWnd->parent->clientArea.top;
/*          printf("children: pt=%u,%u / %u,%u,%u,%u\r\n",pt.x,pt.y,
                  hWnd->nonClientArea.left ,hWnd->nonClientArea.top,
            hWnd->nonClientArea.right ,hWnd->nonClientArea.bottom);*/
          myWnd=myWnd->parent;
          }
        }

      if(pt.y<=rcBorder.top) {
        if(pt.x<=rcBorder.left)
          return hWnd->style & WS_THICKFRAME ? HTTOPLEFT : HTBORDER;
        else if(pt.x>=rcBorder.right)
          return hWnd->style & WS_THICKFRAME ? HTTOPRIGHT : HTBORDER;
        else
          return hWnd->style & WS_THICKFRAME ? HTTOP : HTBORDER;
        }
      else if(pt.y<rcBorder.bottom) {
        if(pt.x<=rcBorder.left) {
          return hWnd->style & WS_THICKFRAME ? HTLEFT : HTBORDER;   // solo se resizable...
          }
        else if(pt.x>=rcBorder.right) {
          return hWnd->style & WS_THICKFRAME ? HTRIGHT : HTBORDER;   // solo se resizable...
          }
        else if(pt.y<=rcBorder.top+TITLE_HEIGHT) {
          if(hWnd->style & WS_CAPTION) {
            if(pt.x>=rcBorder.left && pt.x<=rcBorder.left+TITLE_ICON_WIDTH) {
              return hWnd->style & WS_SYSMENU ? HTSYSMENU : HTCAPTION;
              }
            else if(pt.x>=rcBorder.right-TITLE_ICON_WIDTH) {
              if(hWnd->style & WS_SYSMENU)
                return HTCLOSE;
              else if(hWnd->style & WS_MAXIMIZEBOX)
                return HTMAXBUTTON;
              else
                return HTCAPTION;
              }
            else if(pt.x>=(rcBorder.right-TITLE_ICON_WIDTH*2+1)) {
              if(hWnd->style & WS_MAXIMIZEBOX)
                return HTMAXBUTTON;
              else if(hWnd->style & WS_MINIMIZEBOX)
                return HTMINBUTTON;
              else
                return HTCAPTION;
              }
            else if(pt.x>=(rcBorder.right-TITLE_ICON_WIDTH*3+2)) {
              if(hWnd->style & WS_MINIMIZEBOX)
                return HTMINBUTTON;
              else
                return HTCAPTION;
              }
            else {
              return HTCAPTION;
              }
            }
          else if(hWnd->menu && !(hWnd->style & WS_CHILD)) {
            goto ismenu;
            }
          else {
            return HTCLIENT;
            }
          }
        else if(activeMenuWnd==hWnd && activeMenu==&systemMenu) {
          S_SIZE cs;
          RECT rc;
          HMENU myMenu;
          getMenuSize(activeMenu,&cs,1);
          rc.left=hWnd->nonClientArea.left;
          rc.top=hWnd->nonClientArea.top+TITLE_HEIGHT;
          rc.right=activeMenuRect.left+cs.cx;
          rc.bottom=activeMenuRect.top+cs.cy;

          getMenuFromPoint(hWnd,pt,&myMenu);
          if(0)
            return HTMENU ;     // FINIRE!
          return HTCLIENT;

          }
        else if(hWnd->menu && !(hWnd->style & WS_CHILD) && 
          pt.y<=rcBorder.top+TITLE_HEIGHT+MENU_HEIGHT) {    //SM_CYMENU
ismenu:
// finire.. .QUALE menu
          if(activeMenu) {
            S_SIZE cs;
            RECT rc;
            HMENU myMenu;
            getMenuSize(activeMenu,&cs,1);
            rc.left=hWnd->nonClientArea.left;
            rc.top=hWnd->nonClientArea.top+TITLE_HEIGHT+MENU_HEIGHT;
            rc.right=activeMenuRect.left+cs.cx;
            rc.bottom=activeMenuRect.top+cs.cy;

            getMenuFromPoint(hWnd,pt,&myMenu);
            }
          //else return HTCLIENT;

        //e ev. contextmenu...

            return HTMENU ;     // FINIRE!
          }
        else if(pt.y<=rcBorder.bottom-SCROLL_SIZE) {
          if(pt.x>=rcBorder.right-SCROLL_SIZE)
            return hWnd->style & WS_VSCROLL ? HTVSCROLL : HTCLIENT;
          else
            return HTCLIENT;
          }
        else if(pt.y<=rcBorder.bottom) {
          if(hWnd->style & WS_SIZEBOX) {
            if(pt.x>=rcBorder.right-SCROLL_SIZE)
              return HTSIZE;
            else
              return hWnd->style & WS_HSCROLL ? HTHSCROLL : HTCLIENT;
            }
          else {
            return hWnd->style & WS_HSCROLL ? HTHSCROLL : HTCLIENT;
            }
          }
        else if(pt.x<rcBorder.right) {
          return HTCLIENT;
          }
        }
      else /*if(pt.y<=(hWnd->nonClientArea.bottom-BORDER_SIZE)) */ {
        if(pt.x<=rcBorder.left) {  // gestire thick
          return hWnd->style & WS_THICKFRAME ? HTBOTTOMLEFT : HTBORDER;   // solo se resizable...
          }
        else if(pt.x>=rcBorder.right) {  // gestire thick
          return hWnd->style & WS_THICKFRAME ? HTBOTTOMRIGHT : HTBORDER;   // solo se resizable...
          }
        else {
          return hWnd->style & WS_THICKFRAME ? HTBOTTOM : HTBORDER;   // solo se resizable...
          }
        }

      return HTNOWHERE;
      }
      break;
    case WM_NCCREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      }
      return 0;
      break;
    case WM_NCDESTROY:
      return 0;
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
      break;
    case WM_MOVE:
      //xPos = (int)(short) GET_X_LPARAM(lParam);   // horizontal position 
      //yPos = (int)(short) GET_Y_LPARAM(lParam);   // vertical position 
      activateChildren(hWnd,-1);
      break;
    case WM_MOVING:
      break;
    case WM_SIZE:
      switch(wParam) {
        case SIZE_MAXHIDE:
          break;
        case SIZE_MAXIMIZED:
          break;
        case SIZE_MAXSHOW:
          break;
        case SIZE_MINIMIZED:
          break;
        case SIZE_RESTORED:
          break;
        }
      //UINT width = LOWORD(lParam);
      //UINT height = HIWORD(lParam);
      activateChildren(hWnd,-1);
      break;
    case WM_SIZING:
      break;
    case WM_WINDOWPOSCHANGING:
// For a window with the WS_OVERLAPPED or WS_THICKFRAME style, the DefWindowProc function sends the WM_GETMINMAXINFO message to the window. This is done to validate the new size and position of the window and to enforce the CS_BYTEALIGNCLIENT and CS_BYTEALIGNWINDOW client styles. By not passing the WM_WINDOWPOSCHANGING message to the DefWindowProc function, an application can override these defaults.
      if(hWnd->style & (WS_OVERLAPPED | WS_THICKFRAME)) {
        MINMAXINFO mmi;
  //    mmi.ptMaxPosition
        SendMessage(hWnd,WM_GETMINMAXINFO,0,(LPARAM)&mmi);
        }
      break;
    case WM_WINDOWPOSCHANGED:
      {WINDOWPOS *wpos=(WINDOWPOS*)lParam;
//The DefWindowProc function sends the WM_SIZE and WM_MOVE messages when it processes the WM_WINDOWPOSCHANGED message. The WM_SIZE and WM_MOVE messages are not sent if an application handles the WM_WINDOWPOSCHANGED message without calling DefWindowProc.    
      SendMessage(hWnd,WM_SIZE,hWnd->maximized ? SIZE_MAXIMIZED : 
        (hWnd->minimized ? SIZE_MINIMIZED : SIZE_RESTORED /*forse, aggiungere altri*/),
              MAKELPARAM(wpos->cx,wpos->cy)); 
      SendMessage(hWnd,WM_MOVE,0,MAKELPARAM(wpos->x,wpos->y)); 
      }
      break;
    case WM_ENTERSIZEMOVE:
//      hWnd->internalState=MOVE;   // non c'è modo di distinguere size e move e a me serve...
      SetCursor(hWnd->internalState == MSGF_MOVE ? crossCursorSm : crossCursorSm);    // :) finire
      // cmq si aggiorna solo se il mouse si muove... ev. finire
      break;
    case WM_EXITSIZEMOVE:
      if(hWnd->internalState>=MSGF_MOVE) {
        hWnd->internalStateTemp=0;
        hWnd->internalState=0; //hWnd->locked=0;
        SetCursor(standardCursorSm);
        }
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    case WM_NCACTIVATE:
//      if(SendMessage(hWnd,WM_NCPAINT,(lParam && (lParam != -1)) ? lParam : NULL,0))
        if(lParam != -1)    // se -1 non ridisegna non client area
          nonClientPaint(hWnd,NULL /*rc, lParam*/,wParam);   // se disegnare attiva opp. inattiva title bar o icona
      // https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-ncactivate
      return wParam ? 1 : 1;   // per ora così
      break;
    case WM_ACTIVATEAPP:    // hwnd, thread..
      break;
    case WM_ACTIVATE:
      if(hWnd->enabled) {
        HWND myWnd=(HWND)lParam;
        BYTE temp;
        if(!(hWnd->style & WS_CHILD)) {
          if(LOWORD(wParam) != WA_INACTIVE) {
//            if(!hWnd->active) {
            if(activeMenu && activeMenuWnd!=hWnd/*ovvio ma ok*/)
              SendMessage(activeMenuWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0); // questo non fa nulla...
              
            if(!HIWORD(wParam))
              SetFocus(hWnd);
            } // ACTIVE
          else {
            if(activeMenuWnd==hWnd/*ovvio ma ok*/) 
              SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);   // questo ok
            }   // INACTIVE
          
          }
        else {    // CHILD

  //        hWnd->active=TRUE;    // boh.. per dialog dentro padre...
#warning non so se è giusto avere una child attiva... verificare (v. calcolatrice)
          if(LOWORD(wParam) != WA_INACTIVE) {
//            if(!hWnd->active) {

            if(!HIWORD(wParam))
              SetFocus(hWnd);
            
            if(!hWnd->parent->visible)
              ShowWindow(hWnd->parent,SW_SHOW);   // questo qua non dovrebbe servire, essendo già attiva per tastiera...
  //#warning  se è dialog deve attivare... e non fare show se già visibile      
            hWnd->parent->active=1;

            }   // ACTIVE, child
          else {
            }   // INACTIVE CHILD

          }
        }
      return 0;
      break;
    case WM_MOUSEACTIVATE:
//mah..      if(!hWnd->active) {
      if(hWnd->enabled) {
//The DefWindowProc function passes the message to a child window's parent window before any processing occurs. The parent window determines whether to activate the child window. If it activates the child window, the parent window should return MA_NOACTIVATE or MA_NOACTIVATEANDEAT to prevent the system from processing the message further.
        if(!(hWnd->style & WS_CHILD)) {
          if(!hWnd->active) {
            if(activeMenu) {    // diciamo
              SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);
              }
            }
          return MA_ACTIVATE;
          }
        else {
          HWND myWnd=(HWND)wParam;    // la top level parent
          if(myWnd) {
            if(SendMessage(myWnd,WM_MOUSEACTIVATE,(WPARAM)myWnd,lParam) == MA_ACTIVATE)
              /*HWND oWnd=*/setActive(myWnd,TRUE);
//#warning The DefWindowProc function passes the message to a child window's parent window before any processing occurs. The parent window determines whether to activate the child window. If it activates the child window, the parent window should return MA_NOACTIVATE or MA_NOACTIVATEANDEAT to prevent the system from processing the message further.
            }
  //            if(SendMessage(myWnd,WM_MOUSEACTIVATE,(WPARAM)myWnd,lParam) == MA_NOACTIVATE)
  //        hWnd->active=TRUE;    // boh.. per dialog dentro padre...
  //#warning non so se è giusto avere una child attiva... verficiare
          

          return MA_ACTIVATE;
          }
        }
      else
        return MA_NOACTIVATE;
      break;
    case WM_CHILDACTIVATE:    // solo MDI cmq!
      if(hWnd->style & WS_CHILD)
        hWnd->active=1;
      return 0;
      break;
    case WM_DISPLAYCHANGE:
      InvalidateRect(NULL,NULL,TRUE);
      break;
    case WM_PAINTICON:    // solo windows 3.x :)
      break;
    case WM_QUERYOPEN:
			return 1;
      break;
    case WM_SHOWWINDOW:
      if(wParam)
        ;
      else
        ;
      switch(lParam) {
        case SW_PARENTOPENING:
          break;
        case SW_PARENTCLOSING:
          break;
        case SW_OTHERZOOM:
          break;
        case SW_OTHERUNZOOM:
          break;
        }
			return 0;
      break;
    case WM_SETFOCUS:
      hWnd->focus=1;
      return 0;
      break;
    case WM_KILLFOCUS:
      {HWND myWnd=hWnd->children;
      while(myWnd) {
        SendMessage(myWnd,WM_KILLFOCUS,wParam,0);    // direi, e a sua volta diventa ricorsiva FORSE BASTEREBBE QUELLA COL FOCUS, se c'è??
        myWnd=myWnd->next;
        }
      if(hWnd->internalState>=MSGF_MOVE) {
        SendMessage(hWnd,WM_EXITSIZEMOVE,0,0);
        }
      if(hWnd->internalState==MSGF_MENU) {    // VERIFICARE!!
        SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);
        }
      hWnd->focus=0;
      }
      return 0;
      break;
    case WM_SETTEXT:
    { RECT rc;
      
      strncpy(hWnd->caption,(const char *)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      rc.left=hWnd->nonClientArea.left;
      rc.right=hWnd->nonClientArea.right;
      rc.top=hWnd->nonClientArea.top;
      rc.bottom=hWnd->nonClientArea.top+TITLE_HEIGHT;
      return SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
    }
      break;
    case WM_GETTEXT:
      {
      int i=min(sizeof(hWnd->caption),wParam);
      strncpy((char *)lParam,hWnd->caption,i-1);
      ((char *)lParam)[i]=0;
      return i;
      }
      break;
    case WM_GETTEXTLENGTH:
      return strlen(hWnd->caption);
      break;
    case WM_ENABLE:
      if(!hWnd->enabled) {
//    		if(!(hWnd->style & WS_CHILD))
        if(hWnd->active) {
      // c'entra con active? https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablewindow
          setActive(hWnd,FALSE);
          }
				}
      break;
    case WM_STYLECHANGED:
      break;
    case WM_STYLECHANGING:
      break;
      
    case WM_NOTIFY:
      break;
    case WM_INITDIALOG:
      break;
    case WM_INITMENU:
      {HMENU m=(MENU*)wParam;
      }
      return 0;
      break;
      // premendo ALT-SPAZIO sembra arrivare prima SYSCHAR, SYSCOMMAND, poi ENTERMENULOOP poi INITMENU (barra) poi MENUSELECT
      // premendo ALT arriva prima SYSCOMMAND, poi ENTERMENULOOP poi INITMENU (barra) poi MENUSELECT
      // quando si chiude, MENUSELECT -1 e EXITMENULOOP 
    case WM_INITMENUPOPUP:
      {HMENU m=(MENU*)wParam;
      }
      return 0;
      break;
    case WM_UNINITMENUPOPUP:
      {HMENU m=(MENU*)wParam;
      }
      break;
    case MN_GETHMENU:
      return (DWORD)hWnd->menu;
      break;
    case WM_MENUSELECT:
      {
      if(hWnd->active) {    // direi ovvio cmq!
        if(HIWORD(wParam) == 0xffff && !lParam) {   // chiudo
          SendMessage(hWnd,WM_UNINITMENUPOPUP,(WPARAM)activeMenu,activeMenu==&systemMenu ? MF_SYSMENU : 0);
          SendMessage(hWnd,WM_EXITMENULOOP,0,0);
          activeMenu=NULL; activeMenuWnd=NULL; activeMenuCntX=activeMenuCntY=0;
          SetRectEmpty(&activeMenuRect);
          if(hWnd==desktopWindow)
            InvalidateRect(NULL,NULL,TRUE);   // serve diciamo per popup/context menu
          else {
            InvalidateRect(hWnd,/*rc*/ NULL,TRUE);   // per ora così :) o NCPAINT
//          if(HIWORD(lParam) == MF_SYSMENU)
            DrawMenuBar(hWnd);  // 
            }
          }
        else {    // apro
          HMENU m=(MENU*)lParam;
          // salvare quello vecchio per pulire solo quello? che sarebbe activeMenu vs. m
          BYTE oldLocked=hWnd->locked;    // è abb. inutile perché tanto alla fine DEVE essere locked, ma cmq per coerenza!
          hWnd->locked=0;
          
          if(m != activeMenu) {   // nuovo menu...
            if(hWnd==desktopWindow)
              InvalidateRect(NULL,NULL,TRUE);   // serve diciamo per popup/context menu
            else {
              if(activeMenu==&systemMenu)
                DrawMenuBar(hWnd);    // o NCPAINT, se era aperto il system (anche popup?
              InvalidateRect(hWnd,/*rc*/ NULL,TRUE);   // per ora così :) o NCPAINT
              }

            if(m==&systemMenu) {    // questo non ha i flag di popup e quindi lo gestisco a parte!
              SendMessage(hWnd,WM_INITMENUPOPUP,(WPARAM)m,MAKELONG(0,1));
              S_SIZE cs;
              if(hWnd->style & WS_SYSMENU) {    // mah lo lascio cmq (da tastiera p.es.?)
// fare, forse, circa         if(SendMessage(hWnd,WM_MENUCHAR,MAKELONG(LOBYTE(wParam),MF_SYSMENU),(LPARAM)m) == MNC_SELECT)
                drawMenuPopup(hWnd,m,(BORDER_SIZE-1),TITLE_HEIGHT+(BORDER_SIZE-1),1,activeMenuCntY+1);
                getMenuSize(m,&cs,1);
                activeMenuRect.left=hWnd->nonClientArea.left;
                activeMenuRect.top=hWnd->nonClientArea.top+TITLE_HEIGHT;
                activeMenuRect.right=activeMenuRect.left+cs.cx;
                activeMenuRect.bottom=activeMenuRect.top+cs.cy;
                activeMenu=m; activeMenuParent=(HMENU)&systemMenu; activeMenuWnd=hWnd; /*activeMenuCntX=*/ activeMenuCntY=0;
                }
              else {
                SendMessage(hWnd,WM_MENUCHAR,MAKELONG(LOBYTE(wParam),MF_SYSMENU),(LPARAM)activeMenu);
                }
              }
            // qua si dovrebbe ridisegnare la menubar evidenziando il popup in apertura
            else {
              if(HIWORD(wParam) & MF_POPUP) {   // i flag del menu in questione
                S_SIZE cs;
              // fare? if(!(HIWORD(wParam) & (MF_GRAYED | MF_DISABLED)) { 
                SendMessage(hWnd,WM_INITMENUPOPUP,(WPARAM)m,MAKELONG(getMenuIndex(m,hWnd->menu),0));
                drawMenuPopup(hWnd,m,getMenuPosition(hWnd->menu,m).x+(BORDER_SIZE-1),TITLE_HEIGHT+MENU_HEIGHT+BORDER_SIZE,1,activeMenuCntY+1);
                getMenuSize(m,&cs,1);
                activeMenuRect.left=hWnd->nonClientArea.left+getMenuPosition(hWnd->menu,m).x+(BORDER_SIZE-1);
                activeMenuRect.top=hWnd->nonClientArea.top+TITLE_HEIGHT+MENU_HEIGHT+1;
                activeMenuRect.right=activeMenuRect.left+cs.cx;
                activeMenuRect.bottom=activeMenuRect.top+cs.cy;
                activeMenu=m; activeMenuParent=hWnd->menu; activeMenuWnd=hWnd; /*activeMenuCntX=*/activeMenuCntY=0;
                }
              else {
                //highlight singola voce ecc drawmenupopupitem(
    /*          else {
                myMenu=(HMENU)lParam;
                // trovare item.. disegnarlo
                // drawMenuPopupItem
                if(myMenu) {
    //              myMenu=myMenu->menuItems[i].menu;
                  }
                }*/
                }
              }
            }
          else {
            if(m==&systemMenu) {    // questo non ha i flag di popup e quindi lo gestisco a parte!
              if(hWnd->style & WS_SYSMENU) {    // mah lo lascio cmq (da tastiera p.es.?)
                drawMenuPopup(hWnd,m,(BORDER_SIZE-1),TITLE_HEIGHT+(BORDER_SIZE-1),1,activeMenuCntY+1);
                }
              }
            else {

              drawMenuPopup(hWnd,m,getMenuPosition(hWnd->menu,m).x+(BORDER_SIZE-1),TITLE_HEIGHT+MENU_HEIGHT+BORDER_SIZE,1,activeMenuCntY+1);

                //highlight singola voce ecc drawmenupopupitem(
    /*          else {
                myMenu=(HMENU)lParam;
                // trovare item.. disegnarlo
                // drawMenuPopupItem
                if(myMenu) {
    //              myMenu=myMenu->menuItems[i].menu;
                  }
                }*/

              }
            }
          hWnd->locked=oldLocked;
          }
        }
      }
      break;
    case WM_MENUCHAR:
#if 0
      fare! v. sotto
                for(i=0; i<MAX_MENUITEMS; i++) {
                  if(!(hWnd->menu->menuItems[i].flags & MF_BITMAP)) {
                    char ch;
                    ch=findMenuLetter(hWnd->menu->menuItems[i].text);
                    if(ch == LOBYTE(wParam)) {
                      if(hWnd->menu->menuItems[i].flags & MF_POPUP) {
                        SendMessage(hWnd,WM_SYSCOMMAND,SC_KEYMENU,MAKELONG(LOBYTE(wParam),0/*mnemonic?*/));
                        }
                      else {//bah c'era qualche coglione che metteva un menu di una sola riga!
                        if(hWnd->menu->menuItems[i].flags & MNS_NOTIFYBYPOS) 
                          SendMessage(hWnd,WM_MENUCOMMAND,i,(LPARAM)&hWnd->menu->menuItems[i]);
                        else
                          SendMessage(hWnd,WM_COMMAND,MAKELONG(hWnd->menu->menuItems[i].command,0),0);
                        }
                      break;
                      }
                    }
                  }
                if(i==MAX_MENUITEMS)    // se non trovato
                  return MAKELONG(0,MNC_IGNORE);
#endif
      MessageBeep(MB_ICONASTERISK);
      return MAKELONG(0,MNC_IGNORE);
      break;
    case WM_NEXTMENU:   // MDI soprattutto
      break;
    case WM_ENTERMENULOOP:
      // wparam=true entrato da trackpopupmenu opp false
      hWnd->locked=1;
			hWnd->internalState=MSGF_MENU;    // o WM_ENTERIDLE
      break;
    case WM_EXITMENULOOP:
      // wparam=true se shortcut opp false
      if(hWnd->internalState==MSGF_MENU || hWnd->internalState==MSGF_DIALOGBOX) 
        hWnd->internalState=0; 
      hWnd->locked=0;
      break;
    case WM_ENTERIDLE:
      hWnd->internalState=wParam;
      break;
    case WM_KICKIDLE:   // in effetti pare solo AFX
      break;
    case WM_CANCELMODE:  
      hWnd->internalState=0; // dovrebbe essere così ma OCCHIO CMQ!
      break;
    case WM_GETTITLEBARINFOEX:    // v. anche GetTitleBarInfo
      {TITLEBARINFOEX *tbi=(TITLEBARINFOEX*)lParam;
        // if(tbi->cbSize=
      tbi->rcTitleBar.top=hWnd->nonClientArea.top;
      tbi->rcTitleBar.left=hWnd->nonClientArea.left;
      tbi->rcTitleBar.right=hWnd->nonClientArea.right;
      if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION))
        tbi->rcTitleBar.bottom=tbi->rcTitleBar.top+=TITLE_HEIGHT;
      else
        tbi->rcTitleBar.bottom=tbi->rcTitleBar.top;
      tbi->rgstate[0]=hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION) ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
      tbi->rgstate[2]=hWnd->style & WS_MINIMIZEBOX ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
      tbi->rgstate[3]=hWnd->style & WS_MAXIMIZEBOX  ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
      tbi->rgstate[4]=hWnd->style & 0 /*WS_HELP*/ ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
      tbi->rgstate[5]=hWnd->style & WS_SYSMENU ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
      }
      break;
    case WM_APPCOMMAND:   // browser ecc https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-appcommand
      /*cmd  = GET_APPCOMMAND_LPARAM(lParam);
      uDevice = GET_DEVICE_LPARAM(lParam);
      dwKeys = GET_KEYSTATE_LPARAM(lParam);*/
      break;
    case WM_MENUCOMMAND:    // lParam=HANDLE menu
      break;
    case WM_COMMAND:  //HI-wparam: 0/1 	LO-wparam: menu/accelerator if lparam:0
      break;
    case WM_SYSCOMMAND:
      switch(wParam & 0xfff0) {
        int i;
        POINT pt;
        HMENU myMenu;
        case SC_CLOSE:
          {
          // doppio click su systemmenu... quindi close?? Selects the default item; the user double-clicked the window menu.
//          SendMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,MAKELPARAM(pt.x,pt.y));
          WNDCLASS *wc;
          GetClassInfo(0,hWnd->class,&wc);
          if(!(wc->style & CS_NOCLOSE))
            SendMessage(hWnd,WM_CLOSE,0,0);
          }
          break;
        case SC_CONTEXTHELP:
          break;
        case SC_DEFAULT:
          {
          // doppio click su systemmenu... quindi close?? Selects the default item; the user double-clicked the window menu.
//          SendMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,MAKELPARAM(pt.x,pt.y));
          WNDCLASS *wc;
          GetClassInfo(0,hWnd->class,&wc);
          if(!(wc->style & CS_NOCLOSE))
            SendMessage(hWnd,WM_CLOSE,0,0);
          }
          break;
        case SC_HOTKEY:
          break;
        case SCF_ISSECURE:
          break;
        case SC_KEYMENU:
          if(inDialogLoop()) {
            MessageBeep(MB_ICONASTERISK);
            return 0;
            }
          switch(LOBYTE(LOWORD(lParam))) {
            case VK_SPACE:
              if(hWnd->style & WS_SYSMENU) {
                myMenu=(HMENU)&systemMenu;
                SendMessage(hWnd,WM_ENTERMENULOOP,0,0);
                activeMenuCntX=0;
                SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,
                  myMenu->menuItems[0].flags), (LPARAM)myMenu);
                }
              break;
            default:    // 
              myMenu=hWnd->menu;
              for(i=0; i<MAX_MENUITEMS; i++) {
                if(!(myMenu->menuItems[i].flags & MF_BITMAP)) {
                  char ch;
                  ch=findMenuLetter(myMenu->menuItems[i].text);
                  if(ch == LOBYTE(LOWORD(lParam))) {  // ovvero FARE lettere sottolineata! in culo ISTC bechis di merda 1990 :D
                    if(myMenu->menuItems[i].flags & MF_POPUP) {
                      activeMenuCntX=i;
                      if(hWnd->style & WS_SYSMENU)
                        activeMenuCntX++;
                      SendMessage(hWnd,WM_ENTERMENULOOP,0,0);
                      SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,
                        myMenu->menuItems[i].flags), (LPARAM)myMenu->menuItems[i].menu);
                      break;
                      }
                    }
                  }
                }

              //restituire 1 se non trovato?
              break;
            }
          break;
        case SC_MAXIMIZE:
          ShowWindow(hWnd,SW_MAXIMIZE /*SW_SHOWMAXIMIZED*/);
          break;
        case SC_MINIMIZE:
          ShowWindow(hWnd,SW_MINIMIZE /*SW_SHOWMINIMIZED*/);
          break;
        case SC_RESTORE:
          ShowWindow(hWnd,SW_SHOWNORMAL);
          break;
        case SC_MONITORPOWER:
					// in effetti non è chiaro se questo causa il power-off o se arriva a seguito di... v. powerMode in main
					switch(lParam) {
						case -1:		// -1 (the display is powering on)
							break;
						case 1:			// 1 (the display is going to low power)
							break;
						case 2:			// 2 (the display is being shut off)
							break;
						}
					// restituire 0 se non si vuole che parta!
          break;
        case SC_MOVE:
          // questo semplicemente attiva il "move mode" con tastiera!
          hWnd->internalState=MSGF_MOVE; // hWnd->locked=1;
          SendMessage(hWnd,WM_ENTERSIZEMOVE,0,0);
          break;
        case SC_MOUSEMENU:
          if(inDialogLoop()) {
            MessageBeep(MB_ICONASTERISK);
            return 0;
            }
          if(lParam==0xffffffff) {  // truschino mio
            myMenu=(HMENU)&systemMenu;
            SendMessage(hWnd,WM_ENTERMENULOOP,0,0);
            SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,
              myMenu->menuItems[0].flags | MF_MOUSESELECT), (LPARAM)myMenu);
            }
          else {
            pt=MAKEPOINT(LOWORD(lParam),HIWORD(lParam));
            i=getMenuFromPoint(hWnd,pt,&myMenu);
            if(myMenu) {
              activeMenuCntX=i;
              if(hWnd->style & WS_SYSMENU)
                activeMenuCntX++;
              SendMessage(hWnd,WM_ENTERMENULOOP,0,0);
              SendMessage(hWnd,WM_MENUSELECT,MAKELONG(i,
                myMenu->menuItems[0].flags | MF_MOUSESELECT), (LPARAM)myMenu->menuItems[0].menu);
              }
            }
          break;
        case SC_NEXTWINDOW:
          /* togliere da loop principale
          if(keypress.modifier & 0b00000100) {    // finire con prima o succ. pressione di ALT!
            static BYTE whichWindow=0;     // non è il massimo ma ok...
            HWND w2=rootWindows,w3=NULL;
            i=0;
            if(w2) {
              while(w2->next) {
                if(i==whichWindow) {
                  w3=w2;
                  whichWindow++;
                  break;
                  }
                w2=w2->next;
                i++;
                }
              if(!w3) {
                w3=rootWindows;
                whichWindow=0;
                }
              if(w3)
  // provare!              SwitchToThisWindow(w3,1);
                ShowWindow(w3,SW_SHOWNORMAL);
              }
            }*/
          break;
        case SC_PREVWINDOW:
          break;
        case SC_SCREENSAVE:
					// restituire 0 se non si vuole che parta!
          break;
        case SC_SIZE:
          // questo semplicemente attiva il "size mode" con tastiera!
          hWnd->internalState=MSGF_SIZE; //hWnd->locked=1;
          SendMessage(hWnd,WM_ENTERSIZEMOVE,0,0);
          break;
        case SC_TASKLIST:
          if(taskbarWindow) {
            SendMessage(taskbarWindow,WM_COMMAND,MAKEWORD(SC_TASKLIST,0),0);
            }
          break;
        case SC_HSCROLL:
          {POINT pt={LOWORD(lParam),HIWORD(lParam)};
					int x,i;

					x=(hWnd->scrollSizeX*(pt.x-hWnd->nonClientArea.right))/(hWnd->nonClientArea.right-hWnd->nonClientArea.left);
					if(pt.x>2/*arrow_size*/) {
						i=SB_LINEUP;
						}
					else if(x>hWnd->scrollPosX) {
						i=SB_PAGEDOWN;
						}
					else if(x>hWnd->scrollPosX /*+thumb*/) {
						i=SB_PAGEDOWN;
						}
					else if(pt.x>>hWnd->nonClientArea.bottom-2/*arrow_size*/) {
						i=SB_LINEDOWN;
						}
					else {
						i=SB_PAGEUP;
						}
          SetScrollPos(hWnd,SB_HORZ,x,TRUE);//finire!
          SendMessage(hWnd,WM_HSCROLL,MAKELONG(i,0 /*thumb*/),(LPARAM)NULL);    // finire
          }
          break;
        case SC_VSCROLL:
          {POINT pt={LOWORD(lParam),HIWORD(lParam)};
					int y,i;

					y=(hWnd->scrollSizeY*(pt.y-hWnd->nonClientArea.top))/(hWnd->nonClientArea.bottom-hWnd->nonClientArea.top);
					if(pt.y>2/*arrow_size*/) {
						i=SB_LINEUP;
						}
					else if(y>hWnd->scrollPosY) {
						i=SB_PAGEDOWN;
						}
					else if(y>hWnd->scrollPosY /*+thumb*/) {
						i=SB_PAGEDOWN;
						}
					else if(pt.y>>hWnd->nonClientArea.bottom-2/*arrow_size*/) {
						i=SB_LINEDOWN;
						}
					else {
						i=SB_PAGEUP;
						}
          SetScrollPos(hWnd,SB_VERT,y,TRUE);//finire!
          SendMessage(hWnd,WM_VSCROLL,MAKELONG(i,0 /*thumb*/),(LPARAM)NULL);    // finire
          }
          break;
//        case SC_SEPARATOR:		// eh? :D
//          break;
        }
      return 1;
      break;
/*    case WM_QUERYCENTERWINDOW:    // afx ecc
    case WM_SETMESSAGESTRING:
    case WM_QUERYAFXWNDPROC:
    case WM_FLOATSTATUS:
    case WM_SIZEPARENT:
    case WM_SIZECHILD:
    case WM_INITIALUPDATE:
    case WM_RECALCPARENT:
      break;*/

    case WM_ENDSESSION:
      return 0;
      break;
    case WM_QUERYENDSESSION:
      return TRUE;
      break;
    case WM_QUIT:
      break;
    case WM_NULL:
      break;
    }
  return 0;
  }



BOOL DrawFrameControl(HDC hDC,RECT *rc,BYTE type,uint16_t state) {
  
  switch(type) {
    case DFC_BUTTON:
      switch(state & 0xff) {
        case DFCS_BUTTON3STATE:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              SetTextColor(hDC,DFCS_PUSHED ? WHITE : GRAY192);
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_BUTTONCHECK:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              drawRectangleWindow(hDC,rc->left,rc->top,rc->right,rc->bottom);
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_BUTTONPUSH:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
//              SetTextColor(hDC,DFCS_PUSHED ? WHITE : GRAY192);
              drawRectangleWindow(hDC,rc->left,rc->top,rc->right,rc->bottom);
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_BUTTONRADIO:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              break;
            case DFCS_INACTIVE:
              drawEllipseWindow(hDC,rc->left,rc->top,rc->right,rc->bottom);
              break;
            case DFCS_CHECKED:
              fillEllipseWindow(hDC,rc->left,rc->top,rc->right,rc->bottom);
              break;
            case DFCS_FLAT:
              drawEllipseWindow(hDC,rc->left,rc->top,rc->right,rc->bottom);
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_BUTTONRADIOIMAGE:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              SetTextColor(hDC,DFCS_PUSHED ? WHITE : GRAY192);
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_BUTTONRADIOMASK:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              SetTextColor(hDC,DFCS_PUSHED ? WHITE : GRAY192);
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        }
      break;
    case DFC_CAPTION:
      switch(state & 0xff) {
        case DFCS_CAPTIONCLOSE:
          drawLineWindow(hDC,rc->left+2,rc->top+2,rc->right-1,rc->top+6);
          drawLineWindow(hDC,rc->left+2,rc->top+6,rc->right-1,rc->top+2);
          break;
        case DFCS_CAPTIONHELP:
          //drawChar('?')
          break;
        case DFCS_CAPTIONMAX:
          drawVertArrowWindow(hDC,rc->left+2,rc->top+3,rc->right-rc->left-3,(rc->bottom-rc->top)/3,TRUE);
          break;
        case DFCS_CAPTIONMIN:
          drawVertArrowWindow(hDC,rc->left+2,rc->top+3,rc->right-rc->left-3,(rc->bottom-rc->top)/3,FALSE);
          break;
        case DFCS_CAPTIONRESTORE:
          drawRectangleWindow(hDC,rc->left+2,rc->top+2,rc->right-1,rc->bottom-2);
          break;
        }
      break;
    case DFC_MENU:
      switch(state & 0xff) {
        case DFCS_MENUARROW:
          break;
        case DFCS_MENUARROWRIGHT:
          break;
        case DFCS_MENUBULLET:
          break;
        case DFCS_MENUCHECK:
          drawLineWindow(hDC,rc->left+2,rc->top+MENU_HEIGHT/2,rc->left+4,rc->top+MENU_HEIGHT-2);
          drawLineWindow(hDC,rc->left+5,rc->top+MENU_HEIGHT-2,rc->left+7,rc->top+1);
          break;
        }
      break;
    case DFC_SCROLL:
      switch(state & 0xff) {
        case DFCS_SCROLLCOMBOBOX:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_SCROLLDOWN:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              drawVertArrowWindow(hDC,rc->left+1,rc->top+1,
                (rc->right-rc->left)-2,(rc->bottom-rc->top)-2,FALSE);
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_SCROLLLEFT:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              drawHorizArrowWindow(hDC,rc->left+1,rc->top+1,
                (rc->right-rc->left)-2,(rc->bottom-rc->top)-2,FALSE);
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_SCROLLRIGHT:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              drawHorizArrowWindow(hDC,rc->left+1,rc->top+1,
                (rc->right-rc->left)-2,(rc->bottom-rc->top)-2,TRUE);
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_SCROLLSIZEGRIP:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_SCROLLSIZEGRIPRIGHT:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        case DFCS_SCROLLUP:
          switch(state & 0xff00) {
            case DFCS_PUSHED:
              break;
            case DFCS_INACTIVE:
              break;
            case DFCS_CHECKED:
              break;
            case DFCS_FLAT:
              drawVertArrowWindow(hDC,rc->left+1,rc->top+1,
                (rc->right-rc->left)-2,(rc->bottom-rc->top)-2,TRUE);
              break;
            case DFCS_HOT:
              break;
            case DFCS_MONO:
              break;
            case DFCS_TRANSPARENT:
              break;
            case DFCS_ADJUSTRECT:
              break;
            }
          break;
        }
      {
//      DrawText(hDC,hWnd->caption,-1,&rc,DT_LEFT);
      }
      break;

    default:
      break;
    }

  }

BOOL DrawCaption(HWND hWnd,HDC hDC,const RECT *lpRect,BYTE flags) {
  GFX_COLOR c;
  UGRAPH_COORD_T myx,myy;
  RECT rc;
  
  rc=*lpRect;
  c= flags & DC_ACTIVE ? windowForeColor : windowInactiveForeColor;
  SetDCPenColor(hDC,c);
  
  if(flags & DC_SMALLCAP) {
    SelectObject(hDC,OBJ_FONT,GetStockObject(SYSTEM_FIXED_FONT));
    }
  
  if(flags & DC_ICON) {
    //  if(flags & DC_INBUTTON) usare
//                  drawIcon8(hDC,x,y,hWnd->icon);
    fillRectangleWindowColor(hDC,rc.left+2,rc.top+2,rc.left+TITLE_ICON_WIDTH,rc.top+TITLE_ICON_WIDTH,
      c);
    rc.left=rc.left+TITLE_ICON_WIDTH+2;
    drawVertLineWindow(hDC,rc.left,rc.top,rc.top+TITLE_HEIGHT-1);
    }
  if(flags & DC_TEXT) {
    RECT rc3={rc.left+1,rc.top+1,rc.right-1,rc.top+1+TITLE_HEIGHT-1};
    if(flags & DC_BUTTONS)
      rc3.right-=TITLE_ICON_WIDTH;
    if(hWnd->style & WS_MAXIMIZEBOX)
      rc3.right-=TITLE_ICON_WIDTH;
    if(hWnd->style & WS_MINIMIZEBOX)
      rc3.right-=TITLE_ICON_WIDTH;
    DrawEdge(hDC,&rc3,0,BF_MIDDLE | BF_ADJUST);
    SetTextColor(hDC,c);
    DrawText(hDC,hWnd->caption,-1,&rc3,DT_LEFT | DT_HIDEPREFIX | DT_SINGLELINE);
    }
  if(flags & DC_BUTTONS) {
    {
    RECT rc3={rc.right-TITLE_ICON_WIDTH,rc.top+1,rc.right-1,rc.top+TITLE_HEIGHT-1};
    DrawFrameControl(hDC,&rc3,DFC_CAPTION,DFCS_CAPTIONCLOSE);
    }
    drawVertLineWindow(hDC,rc.right-TITLE_ICON_WIDTH,rc.top,rc.top+TITLE_HEIGHT-1);
    rc.right -= TITLE_ICON_WIDTH;
    if(hWnd->style & WS_MAXIMIZEBOX) {
      RECT rc3={rc.right-TITLE_ICON_WIDTH,rc.top+1,rc.right-1,rc.top+TITLE_HEIGHT-1};
      if(hWnd->maximized)
        DrawFrameControl(hDC,&rc3,DFC_CAPTION,DFCS_CAPTIONRESTORE);
      else
        DrawFrameControl(hDC,&rc3,DFC_CAPTION,DFCS_CAPTIONMAX);
      drawVertLineWindow(hDC,rc.right-TITLE_ICON_WIDTH,rc.top,rc.top+TITLE_HEIGHT-1);
      rc.right -= TITLE_ICON_WIDTH;
      }
    if(hWnd->style & WS_MINIMIZEBOX) {
      RECT rc3={rc.right-TITLE_ICON_WIDTH,rc.top+1,rc.right-1,rc.top+TITLE_HEIGHT-1};
      DrawFrameControl(hDC,&rc3,DFC_CAPTION,DFCS_CAPTIONMIN);
      drawVertLineWindow(hDC,rc.right-TITLE_ICON_WIDTH,rc.top,rc.top+TITLE_HEIGHT-1);
      rc.right-= TITLE_ICON_WIDTH;
      }
    }
// rimettere a posto??  SelectObject(hDC,OBJ_FONT,GetStockObject(SYSTEM_FONT));
  }

void drawHorizArrowWindow(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE size,BYTE width,BYTE dir) {
  
  if(dir) {   //right
    drawLineWindow(hDC,x,y,x+width,y+size/2);
    drawLineWindow(hDC,x+width,y+size/2,x,y+size);
    }
  else {    // left
    drawLineWindow(hDC,x+width,y,x,y+size/2);
    drawLineWindow(hDC,x,y+size/2,x+width,y+size);
    }
  if(size>6)    // estetica...
    drawHorizLineWindow(hDC,x,y+size/2,x+width);
  }

void drawVertArrowWindow(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE size,BYTE height,BYTE dir) {
  
  if(dir) {   //up
    drawLineWindow(hDC,x,y+height,x+size/2,y);
    drawLineWindow(hDC,x+size/2,y,x+size,y+height);
    }
  else {    // down
    drawLineWindow(hDC,x,y,x+size/2,y+height);
    drawLineWindow(hDC,x+size/2,y+height,x+size,y);
    }
  if(size>6)    // estetica...
    drawVertLineWindow(hDC,x+size/2,y,y+height);
  }

BOOL DrawEdge(HDC hDC,RECT *qrc,BYTE edge,uint16_t grfFlags) {
  
  if(edge & BDR_RAISEDINNER) {   //FINIRE!
    }
  if(edge & BDR_SUNKENINNER) {   //
    }
  if(edge & BDR_RAISEDOUTER) {   //FINIRE!
    }
  if(edge & BDR_SUNKENOUTER) {   //
    }
  if(grfFlags & BF_FLAT) {
    }
  if(grfFlags & BF_ADJUST) {
    qrc->left++;
    qrc->top++;
    qrc->right--;
    qrc->bottom--;
    }
  if(grfFlags & BF_LEFT) {
    }
  if(grfFlags & BF_TOP) {
    }
  if(grfFlags & BF_RIGHT) {
    }
  if(grfFlags & BF_BOTTOM) {
    }
  if(grfFlags & BF_MIDDLE)
    fillRectangleWindow(hDC,qrc->left,qrc->top,qrc->right,qrc->bottom);
  else
    drawRectangleWindow(hDC,qrc->left,qrc->top,qrc->right,qrc->bottom);
  
  }

BOOL DrawFocusRect(HDC hDC,const RECT *lprc) {
  
  // bah dice SetColors(WHITE,BLACK,R2_XORPEN);
  drawRectangleWindowColor(hDC,lprc->left,lprc->top,lprc->right,lprc->bottom,CYAN);
  // bah dice SetColors(WHITE,BLACK,R2_COPYPEN);
  }


int DialogBox(HINSTANCE hInstance,const void *lpTemplate,HWND hWndParent,DIALOGPROC *lpDialogFunc) {
  HWND hWnd,focusWnd=NULL;
  HWND myWnd;
  DLGTEMPLATE *t=(DLGTEMPLATE*)lpTemplate;
  int i;
  POINT pt={t->x,t->y};
  
  if(t->style & WS_DLGFRAME)
    t->style &= ~(WS_CAPTION | WS_SYSMENU);   // così dice..
  if(t->style & DS_CENTER) {
    RECT rc;
    GetClientRect(hWndParent ? hWndParent : desktopWindow,&rc);
    pt.x=rc.right>t->cx ? (rc.right-t->cx)/2 : 0;
    pt.y=rc.bottom>t->cy ? (rc.bottom-t->cy)/2 : 0;
    }
  if(t->style & DS_CENTERMOUSE) {
    POINT pt2;
    GetCursorPos(&pt);
    pt.x=pt2.x-(t->cx)/2;
    pt.y=pt2.y-(t->cy)/2;
    }
	hWnd=internalCreateWindow(MAKECLASS(WC_DIALOG),t->caption,t->style  /*FORZARE?  | WS_POPUP | WS_DLGFRAME | WS_CHILD | WS_CLIPCHILDREN*/,
    pt.x,pt.y,pt.x+t->cx,pt.y+t->cy,
    hWndParent ? hWndParent : desktopWindow,(HMENU)0,NULL,lpDialogFunc
    );
  for(i=0; i<t->cdit; i++) {
    DLGITEMTEMPLATE *t2=t->item[i];
    myWnd=CreateWindow(t2->class,t2->caption,t2->style | WS_CHILD ,
      t2->x,t2->y,t2->cx,t2->cy,
      hWnd,(HMENU)(int)t2->id,NULL
      );
		if(t->style & DS_SETFONT)
			SendMessage(myWnd,WM_SETFONT,(WPARAM)&t->font,0);
		if(t2->style & BS_DEFPUSHBUTTON)
      SetWindowByte(hWnd,DWL_INTERNAL+1,(BYTE)(DWORD)myWnd->menu);
    if(!focusWnd && !(myWnd->style & WS_DISABLED))
      focusWnd=myWnd;     // SCEGLIERE QUALE! il più basso come ID??
    }
  if(!focusWnd)
    focusWnd=myWnd;
  SetWindowLong(hWnd,DWL_DLGPROC,(DWORD)lpDialogFunc); 
  
  SendMessage(hWnd,WM_INITDIALOG,(WPARAM)focusWnd,0);
  SendMessage(hWnd,WM_SETFONT,(WPARAM)&t->font,TRUE);
//  ShowWindow(hWnd,SW_SHOW);   // anche se WS_VISIBLE?
  if(hWndParent)
    SendMessage(hWndParent,WM_ENTERIDLE,MSGF_DIALOGBOX,(LPARAM)hWnd);   //
  
//  setActive(hWnd,TRUE);   //SW_SHOW non attiva le child..
  hWnd->active=1;
  
  // FARE MODAL LOOP!
  return GetWindowLong(hWnd,DWL_MSGRESULT);
  }
int DialogBoxParam(HINSTANCE hInstance,const void *lpTemplate,HWND hWndParent,DIALOGPROC *lpDialogFunc,LPARAM dwInitParam) {
  HWND hWnd,focusWnd=NULL;
  HWND myWnd;
  DLGTEMPLATE *t=(DLGTEMPLATE*)lpTemplate;
  int i;
  POINT pt={t->x,t->y};
  
  if(t->style & WS_DLGFRAME)
    t->style &= ~(WS_CAPTION | WS_SYSMENU);   // così dice..
  if(t->style & DS_CENTER) {
    RECT rc;
    GetClientRect(hWndParent ? hWndParent : desktopWindow,&rc);
    pt.x=rc.right>t->cx ? (rc.right-t->cx)/2 : 0;
    pt.y=rc.bottom>t->cy ? (rc.bottom-t->cy)/2 : 0;
    }
  if(t->style & DS_CENTERMOUSE) {
    POINT pt2;
    GetCursorPos(&pt);
    pt.x=pt2.x-(t->cx)/2;
    pt.y=pt2.y-(t->cy)/2;
    }
  
	hWnd=internalCreateWindow(MAKECLASS(WC_DIALOG),t->caption,t->style  /*FORZARE?  | WS_POPUP | WS_DLGFRAME | WS_CHILD | WS_CLIPCHILDREN*/,
    pt.x,pt.y,pt.x+t->cx,pt.y+t->cy,
    hWndParent ? hWndParent : desktopWindow,(HMENU)0,NULL,lpDialogFunc
    );
  for(i=0; i<t->cdit; i++) {
    DLGITEMTEMPLATE *t2=t->item[i];
    myWnd=CreateWindow(t2->class,t2->caption,t2->style | WS_CHILD ,
      t2->x,t2->y,t2->cx,t2->cy,
      hWnd,(HMENU)(int)t2->id,NULL
      );
		if(t->style & DS_SETFONT)
			SendMessage(myWnd,WM_SETFONT,(WPARAM)&t->font,0);
		if(t2->style & BS_DEFPUSHBUTTON)
      SetWindowByte(hWnd,DWL_INTERNAL+1,(BYTE)(DWORD)myWnd->menu);
    if(!focusWnd && !(myWnd->style & WS_DISABLED))
      focusWnd=myWnd;     // SCEGLIERE QUALE! il più basso come ID??
    }
  if(!focusWnd)
    focusWnd=myWnd;
  SetWindowLong(hWnd,DWL_DLGPROC,(DWORD)lpDialogFunc);
  
  SendMessage(hWnd,WM_INITDIALOG,(WPARAM)focusWnd,dwInitParam);
  SendMessage(hWnd,WM_SETFONT,(WPARAM)&t->font,TRUE);
//  ShowWindow(hWnd,SW_SHOW);   // anche se WS_VISIBLE?
  if(hWndParent)
    SendMessage(hWndParent,WM_ENTERIDLE,MSGF_DIALOGBOX,(LPARAM)hWnd);   //
  
  // FARE MODAL LOOP!
  return GetWindowLong(hWnd,DWL_MSGRESULT);
  }

HWND CreateDialog(HINSTANCE hInstance,const char *lpName,HWND hWndParent,DIALOGPROC *lpDialogFunc) {
  DLGTEMPLATE *t;
  HWND hWnd,focusWnd=NULL;
  HWND myWnd;
  int i;
  t=(DLGTEMPLATE*)lpName;   // qua SOLO con struct! no risorse..
  POINT pt={t->x,t->y};
  
  if(t->style & WS_DLGFRAME)
    t->style &= ~(WS_CAPTION | WS_SYSMENU);   // così dice..
  if(t->style & DS_CENTER) {
    RECT rc;
    GetClientRect(hWndParent ? hWndParent : desktopWindow,&rc);
    pt.x=rc.right>t->cx ? (rc.right-t->cx)/2 : 0;
    pt.y=rc.bottom>t->cy ? (rc.bottom-t->cy)/2 : 0;
    }
  if(t->style & DS_CENTERMOUSE) {
    POINT pt2;
    GetCursorPos(&pt);
    pt.x=pt2.x-(t->cx)/2;
    pt.y=pt2.y-(t->cy)/2;
    }
  
	hWnd=internalCreateWindow(MAKECLASS(WC_DIALOG),t->caption,t->style  /*FORZARE?  | WS_POPUP | WS_DLGFRAME | WS_CHILD | WS_CLIPCHILDREN*/,
    pt.x,pt.y,pt.x+t->cx,pt.y+t->cy,
    hWndParent ? hWndParent : desktopWindow,(HMENU)0,NULL,lpDialogFunc
    );
  for(i=0; i<t->cdit; i++) {
    DLGITEMTEMPLATE *t2=t->item[i];
    myWnd=CreateWindow(t2->class,t2->caption,t2->style | WS_CHILD,
      t2->x,t2->y,t2->cx,t2->cy,
      hWnd,(HMENU)(int)t2->id,NULL
      );
		if(t->style & DS_SETFONT)
			SendMessage(myWnd,WM_SETFONT,(WPARAM)&t->font,0);
		if(t2->style & BS_DEFPUSHBUTTON)
      SetWindowByte(hWnd,DWL_INTERNAL+1,(BYTE)(DWORD)myWnd->menu);
    if(!focusWnd && !(myWnd->style & WS_DISABLED))
      focusWnd=myWnd;     // SCEGLIERE QUALE! il più basso come ID??
    }
  if(!focusWnd)
    focusWnd=myWnd;
  SetWindowLong(hWnd,DWL_DLGPROC,(DWORD)lpDialogFunc);

  SendMessage(hWnd,WM_INITDIALOG,(WPARAM)focusWnd,0);
  SendMessage(hWnd,WM_SETFONT,(WPARAM)&t->font,TRUE);
//  ShowWindow(hWnd,SW_SHOW); no! fuori o cmq non serve
  return hWnd;
  }

HWND CreateDialogParam(HINSTANCE hInstance,const char *lpName,HWND hWndParent,DIALOGPROC *lpDialogFunc, LPARAM dwInitParam) {
  DLGTEMPLATE *t;
  HWND hWnd,focusWnd=NULL;
  HWND myWnd;
  int i;
  t=(DLGTEMPLATE*)lpName;   // qua SOLO con struct! no risorse..
  POINT pt={t->x,t->y};
  
  if(t->style & WS_DLGFRAME)
    t->style &= ~(WS_CAPTION | WS_SYSMENU);   // così dice..
  if(t->style & DS_CENTER) {
    RECT rc;
    GetClientRect(hWndParent ? hWndParent : desktopWindow,&rc);
    pt.x=rc.right>t->cx ? (rc.right-t->cx)/2 : 0;
    pt.y=rc.bottom>t->cy ? (rc.bottom-t->cy)/2 : 0;
    }
  if(t->style & DS_CENTERMOUSE) {
    POINT pt2;
    GetCursorPos(&pt);
    pt.x=pt2.x-(t->cx)/2;
    pt.y=pt2.y-(t->cy)/2;
    }
	hWnd=CreateWindow(MAKECLASS(WC_DIALOG),t->caption,t->style /*FORZARE?  | WS_POPUP | WS_DLGFRAME | WS_CHILD | WS_CLIPCHILDREN*/,
    pt.x,pt.y,pt.x+t->cx,pt.y+t->cy,
    hWndParent ? hWndParent : desktopWindow,(HMENU)0,NULL
    );
  for(i=0; i<t->cdit; i++) {
    DLGITEMTEMPLATE *t2=t->item[i];
    myWnd=CreateWindow(t2->class,t2->caption,t2->style | WS_CHILD,
      t2->x,t2->y,t2->cx,t2->cy,
      hWnd,(HMENU)(int)t2->id,NULL
      );
		if(t->style & DS_SETFONT)
			SendMessage(myWnd,WM_SETFONT,(WPARAM)&t->font,0);
		if(t2->style & BS_DEFPUSHBUTTON)
      SetWindowByte(hWnd,DWL_INTERNAL+1,(BYTE)(DWORD)myWnd->menu);
    if(!focusWnd && !(myWnd->style & WS_DISABLED))
      focusWnd=myWnd;     // SCEGLIERE QUALE! il più basso come ID??
    }
  if(!focusWnd)
    focusWnd=myWnd;     // SCEGLIERE QUALE! il più basso come ID??
  SetWindowLong(hWnd,DWL_DLGPROC,(DWORD)lpDialogFunc);

  SendMessage(hWnd,WM_INITDIALOG,(WPARAM)focusWnd,dwInitParam);
  SendMessage(hWnd,WM_SETFONT,(WPARAM)&t->font,TRUE);
//  ShowWindow(hWnd,SW_SHOW); no! fuori o cmq non serve
  return hWnd;
  }

BOOL EndDialog(HWND hDlg,int nResult) {
  
  SetWindowLong(hDlg,DWL_MSGRESULT,nResult);
  if(hDlg->parent && !(hDlg->style & DS_NOIDLEMSG))
    SendMessage(hDlg->parent,WM_EXITMENULOOP,0,0);    // dovrebbe andare bene ugualmente!
  

//  err_printf("endDlg: %x %X: %u",hDlg,hDlg->parent,nResult);
  
/*EndDialog does not destroy the dialog box immediately. Instead, it sets a flag and allows the dialog box procedure to return control to the system. The system checks the flag before attempting to retrieve the next message from the application queue. If the flag is set, the system ends the message loop, destroys the dialog box, and uses the value in nResult as the return value from the function that created the dialog box.*/
  
  SendMessage(hDlg,WM_CLOSE,0,0);
  return TRUE;
  }

HWND GetDlgItem(HWND hDlg,uint16_t nIDDlgItem) {
  HWND myWnd=hDlg->children;
  
  while(myWnd) {
    if((uint16_t)(DWORD)myWnd->menu == nIDDlgItem)
      return myWnd;
    myWnd=myWnd->next;
    }
  return NULL;
  }

uint16_t GetDlgCtrlID(HWND hWnd) {
  
  return (uint16_t)(DWORD)hWnd->menu;
  }

UINT SetDlgItemInt(HWND hDlg,uint16_t nIDDlgItem,UINT uValue,BOOL bSigned) {
  char buf[20];
  
  sprintf(buf,bSigned ? "%d" : "%u",uValue);
  return SetWindowText(GetDlgItem(hDlg,nIDDlgItem),buf);
  }

UINT GetDlgItemInt(HWND hDlg,uint16_t nIDDlgItem,BOOL *lpTranslated,BOOL bSigned) {
  int32_t n;
  
  n=atoi(GetDlgItem(hDlg,nIDDlgItem)->caption);
  if(lpTranslated)
    *lpTranslated=TRUE;
  if(!bSigned && n<0)
    n=-n;
    
  return n;
  }

UINT SetDlgItemText(HWND hDlg,uint16_t nIDDlgItem,const char *lpString) {
  
  return SetWindowText(GetDlgItem(hDlg,nIDDlgItem),lpString);
  }

UINT GetDlgItemText(HWND hDlg,uint16_t nIDDlgItem,char *lpString,uint16_t cchMax) {
  
  return GetWindowText(GetDlgItem(hDlg,nIDDlgItem),lpString,cchMax);
  }

BOOL CheckDlgButton(HWND hDlg,uint16_t nIDButton,BYTE uCheck) {
  SetWindowByte(GetDlgItem(hDlg,nIDButton),GWL_USERDATA,uCheck);    // FINIRE!
  }
BOOL CheckRadioButton(HWND hDlg,uint16_t nIDFirstButton,uint16_t nIDLastButton,uint16_t nIDCheckButton) {
  int i;
  
  for(i=nIDFirstButton; i<=nIDLastButton; i++)
    SetWindowByte(GetDlgItem(hDlg,nIDCheckButton),GWL_USERDATA,1);    // FINIRE!
  
  SetWindowByte(GetDlgItem(hDlg,nIDCheckButton),GWL_USERDATA,1);    // FINIRE!
  return 1;
  }
UINT IsDlgButtonChecked(HWND hDlg,uint16_t nIDButton) {
  return GetWindowByte(GetDlgItem(hDlg,nIDButton),GWL_USERDATA+0);    // FINIRE!
  }
LRESULT SendDlgItemMessage(HWND hDlg,uint16_t nIDDlgItem,UINT Msg,WPARAM wParam,LPARAM lParam) {
  return SendMessage(GetDlgItem(hDlg,nIDDlgItem),Msg,wParam,lParam);
  }
static HWND subNextDlgItem(HWND hDlg,HWND hCtl) {   // uso la Lista e non l'ID...
  hCtl=hCtl->next;
  if(!hCtl)
    hCtl=hDlg->children;
  return hCtl;
  }
static HWND subPrevDlgItem(HWND hDlg,HWND hCtl) {
  BYTE i=0;
  HWND myWnd=hCtl,myWnd2;
  do {
    myWnd2=myWnd;
    myWnd=myWnd->next;
    if(!myWnd) {
      if(!i) {
        myWnd=hDlg->children;
        i=1;
        }
      else
        break;
      }
    } while(myWnd != hCtl);
  return myWnd2;
  }
static HWND subNextDlgItemID(HWND hDlg,HWND hCtl) {   // uso l'ID...
  DWORD i=(DWORD)hCtl->menu;
  HWND myWnd=hCtl->parent->children;
  while(myWnd) {
    if((DWORD)myWnd->menu>i)
      return myWnd;
    myWnd=myWnd->next;
    }
  return hCtl;
  }
static HWND subPrevDlgItemID(HWND hDlg,HWND hCtl) {
  DWORD i=(DWORD)hCtl->menu;
  HWND myWnd=hCtl->parent->children;
  while(myWnd) {
    if((DWORD)myWnd->menu<i)
      return myWnd;
    myWnd=myWnd->next;
    }
  return hCtl;
  }
HWND GetNextDlgTabItem(HWND hDlg,HWND hCtl,BOOL bPrevious) {
  HWND myWnd;
  
  if(!hCtl)   // qua il doc dice di no, ma siccome (giustamente direi) dicse sì per Group... v. sotto
    hCtl=hDlg->children;
  myWnd=hCtl;
  
  do {
    if(bPrevious)
      myWnd=subPrevDlgItem(hDlg,myWnd);
    else
      myWnd=subNextDlgItem(hDlg,myWnd);
    if(!myWnd)
      break;
    if(myWnd->visible && myWnd->enabled && myWnd->style & WS_TABSTOP)
      break;
    } while(myWnd != hCtl);
  return myWnd;
  }
HWND GetNextDlgGroupItem(HWND hDlg,HWND hCtl,BOOL bPrevious) {
  HWND myWnd;
  
  if(!hCtl)
    hCtl=hDlg->children;
  myWnd=hCtl;
  
  do {
    if(bPrevious)
      myWnd=subPrevDlgItem(hDlg,myWnd);
    else
      myWnd=subNextDlgItem(hDlg,myWnd);
    if(!myWnd)
      break;
    if(myWnd->visible && myWnd->enabled && myWnd->style & WS_GROUP)
      break;
    } while(myWnd != hCtl);
  return myWnd;
  }

static HWND moveDlgItemFocus(HWND hDialog,HWND getFocusWnd) {
  HWND lostFocusWnd=NULL;
  
  if(getFocusWnd) {
    lostFocusWnd=GetDlgItem(hDialog,GetWindowByte(hDialog,DWL_INTERNAL));		// chi ha il focus precedentemente..
    if(lostFocusWnd)
      SendMessage(lostFocusWnd,WM_KILLFOCUS,(WPARAM)getFocusWnd,0);
    SendMessage(getFocusWnd,WM_SETFOCUS,(WPARAM)lostFocusWnd,0);
    SetWindowByte(hDialog,DWL_INTERNAL,(BYTE)(DWORD)getFocusWnd->menu);   // 
    }
  return lostFocusWnd;
  }

LRESULT DefDlgProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_ERASEBKGND:
      {
      HDC hDC=(HDC)wParam;
      int c=hDC->brush.color;

      if((c=SendMessage(hWnd,WM_CTLCOLORDLG,    // a me stesso! dice così... boh
        wParam,(LPARAM)hWnd)) == 0xffffffff) {
        c=hDC->brush.color;
        }
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
      }
      return 1;
      break;
    case WM_CTLCOLORDLG:
      return 0xffffffff;   // questa per impostare il colore di fondo della dialog
      break;
    case WM_DRAWITEM:   // v. anche defwindowproc, ma toro viugsto esplicitarli qua
      {
      DRAWITEMSTRUCT *dis=(DRAWITEMSTRUCT*)lParam;
      //wparam CtlID; If the message was sent by a menu, this parameter is zero.
      drawRectangleWindowColor(dis->hDC,dis->rcItem.left,dis->rcItem.top,
        dis->rcItem.right,dis->rcItem.bottom,SADDLEBROWN); // tanto per!
      }
      break;
    case WM_MEASUREITEM:
      {
      MEASUREITEMSTRUCT *mis=(MEASUREITEMSTRUCT*)lParam;
      //wparam CtlID;  If the message was sent by a menu, this parameter is zero. If the value is nonzero or the value is zero and the value of the CtlType member of the MEASUREITEMSTRUCT pointed to by lParam is not ODT_MENU, the message was sent by a combo box or by a list box. If the value is nonzero, and the value of the itemID member of the MEASUREITEMSTRUCT pointed to by lParam is (UINT) 1, the message was sent by a combo edit field.
      }
      break;
    case WM_COMPAREITEM:
      {
      COMPAREITEMSTRUCT *cis=(COMPAREITEMSTRUCT*)lParam;
      //wparam CtlID; 
      }
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
//      SetWindowByte(hWnd,DWL_,cs->style & 0x0f);   // salvo il tipo, anche se in effetti sarebbe anche in style..
      int i;
      }
      return 0;
      break;
    case WM_INITDIALOG:
      //wParam=A handle to the control to receive the default keyboard focus. The system assigns the default keyboard focus only if the dialog box procedure returns TRUE.
      //lParam=Additional initialization data. This data is passed to the system as the lParam parameter 
      if(wParam) {
        SetFocus((HWND)wParam); 
        SetWindowByte(hWnd,DWL_INTERNAL,(BYTE)(DWORD)((HWND)wParam)->menu);   // 
        }
      else {
        if(hWnd->children) {
          SetFocus(hWnd->children);
          SetWindowByte(hWnd,DWL_INTERNAL,(BYTE)(DWORD)hWnd->children->menu);   // 
          }
        }
      // p.es. CheckDlgButton(hwndDlg, ID_ABSREL, fRelative); 
      return 1;
      break;
    case WM_NEXTDLGCTL:   // WS_TABSTOP ecc
      {HWND myWnd,myWnd2;
			int i;

      if(LOWORD(lParam)) {
        myWnd=(HWND)wParam;
        if(myWnd->visible && myWnd->enabled /*&& myWnd->style & WS_TABSTOP*/) {   // bah io direi così...
          moveDlgItemFocus(hWnd,myWnd);
// anche BM_SETSTATE,pare per ridisegnare (o se lo smazza lui? v.
          }
        }
      else {
        myWnd2=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));		// chi ha il focus precedentemente..
				if(!myWnd2) {		//.. se per qualche motivo non era scritta, la cerco
	        myWnd2=hWnd->children;
					while(myWnd2) {
		        if(myWnd2->visible && myWnd2->enabled && /*myWnd->style & WS_TABSTOP && */ myWnd2->focus)
							break;
	          myWnd2=myWnd2->next;
						}
					}
				// (se non l'ho trovata, cmq dopo la ignoro)
        myWnd=GetNextDlgTabItem(hWnd,myWnd2,wParam);
        moveDlgItemFocus(hWnd,myWnd);
        }
//      printf("nextDlgCtl: %u\r\n",GetWindowByte(hWnd,DWL_INTERNAL));
      }
      break;
    case WM_ACTIVATE: // save or restore control focus..
      if(hWnd->enabled)
      {
      if(LOWORD(wParam) != WA_INACTIVE) {
//no!        HWND myWnd=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));		// chi ha il focus precedentemente..
//        moveDlgItemFocus(hWnd,myWnd);
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
    case WM_MOUSEACTIVATE: // save or restore control focus..
      if(hWnd->enabled)
      {
//no!      HWND myWnd=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));		// chi ha il focus precedentemente..
//      moveDlgItemFocus(hWnd,myWnd);
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
    case WM_SHOWWINDOW:  // saves
      break;
//    case WM_CLOSE:
      // se serve ... SetWindowLong(hwndDlg, DWL_MSGRESULT, lResult) ma v. EndDialog
//      PostMessage(hWnd,WM_COMMAND,MAKELONG(IDCANCEL,BN_CLICKED),(LPARAM)NULL);
//      DestroyWindow(hWnd);
//      break;
    case DM_GETDEFID:
      {BYTE i;
      if(i=GetWindowByte(hWnd,DWL_INTERNAL+1))
        return MAKELONG(i,DC_HASDEFID);
      else
        return 0;
      }
      break;
    case DM_SETDEFID:
      {HWND myWnd=GetDlgItem(hWnd,wParam);
      SetWindowByte(hWnd,DWL_INTERNAL+1,wParam);
      if(myWnd)
        SendMessage(myWnd,BM_SETSTYLE,BS_DEFPUSHBUTTON,TRUE);
      }
      break;
    case DM_REPOSITION:
			if(!(hWnd->style & WS_CHILD)) {
				RECT rc;
				int n;
				GetWindowRect(hWnd,&rc);
				if(rc.right>Screen.cx) {
					n=Screen.cx-rc.right;
					rc.left+=n; rc.right+=n;
					}
				if(rc.bottom>Screen.cy) {
					n=Screen.cy-rc.bottom;
					rc.top+=n; rc.bottom+=n;
					}
				// fit within desktop...
				MoveWindow(hWnd,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,1);
				}
      break;
      
    case WM_COMMAND:  //HI-wparam: Control-defined notification code 	LO-wparam: Control identifier 	lparam:Handle to the control window
      switch(HIWORD(wParam)) { 
        HWND myWnd;
//        case STN_CLICKED: è lo stesso!
        case BN_CLICKED:
          myWnd=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));		// chi ha il focus precedentemente..
//          if(myWnd)
//            SendMessage(myWnd,WM_KILLFOCUS,(WPARAM)(HWND)lParam,0);
// DOVRBBE PENSARCI PARENTNOTIFY!          SendMessage((HWND)lParam,WM_SETFOCUS,(WPARAM)myWnd,0);
          SetWindowByte(hWnd,DWL_INTERNAL,(BYTE)(DWORD)(((HWND)lParam)->menu));   // 
          if(LOWORD(wParam)==GetWindowByte(hWnd,DWL_INTERNAL+1)) { // se è il tasto DEFPUSHBUTTON
            switch(LOWORD(wParam)) { 
              case IDOK:            // Notify the owner window to carry out the task. 
              case IDYES:            // bah sì
                EndDialog(hWnd,1);
                return TRUE; 
                break;
              case IDNO:            // bah sì
              case IDCANCEL: 
                EndDialog(hWnd,0);
                return TRUE; 
                break;
              } 
            } 
//          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        case BN_SETFOCUS:
          break;
        case BN_KILLFOCUS:
          break;
//        case BN_PAINT:      //3.x only, v. ownerdraw
//          break;
          
        case STN_ENABLE:
          break;
        case STN_DISABLE:
          break;
        case STN_DBLCLK:
          break;
        } 
      break;
    case WM_SYSCOMMAND:   
      // saves focus
//      SetWindowByte(hWnd,DWL_INTERNAL,0);   // finire
      switch(wParam) { 
        case SC_CLOSE:
          {
          WNDCLASS *wc;
          GetClassInfo(0,hWnd->class,&wc);
          if(!(wc->style & CS_NOCLOSE))
            EndDialog(hWnd,0);
          }
          return TRUE; 
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;

    case WM_PARENTNOTIFY: /* By default, child windows in a dialog box have the WS_EX_NOPARENTNOTIFY style, unless the CreateWindowEx function is called to create the child window without this style.*/
      /*ma io lo vorrei usare per impostare focus & attivare da child in dialog...*/
      switch(LOWORD(wParam)) {
        case WM_LBUTTONDOWN:          // mi gestisco il focus dei control
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
          {POINT pt={LOWORD(lParam),HIWORD(lParam)};   // che froci, ci faceva schifo passare la window qua!
          HWND myWnd=(HWND)WindowFromPoint(pt);
          moveDlgItemFocus(hWnd,myWnd);
          // in questo caso dovrei fare return MA_ACTIVATE ma non so come passarlo alla DefWindowProc... o duplicare tutto?
          }
          break;
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_NOTIFY:
      switch(((NMHDR*)lParam)->code) {
        case LBN_SELCHANGE:
//          if(((LPNMHDR)lParam)->idFrom == IDC_CUSTOMLISTBOX1) 
                  // Respond to message.
          return TRUE;
//              }
          break; 
        case LBN_DBLCLK:
          return TRUE;
        case NM_LDOWN:
          return TRUE;
        case NM_DBLCLK:
          return TRUE;
        case NM_KEYDOWN:
          return TRUE;
        case WM_VSCROLL:    // finire
          return TRUE;
        }
      break;
    case WM_CHAR:
      if(hWnd->enabled) {   // pare che NORMALMENTE la pretranslatemessage ecc NON fa arrivare i CHAR a dialog... v. anche sotto WM_GETDLGCODE
        //IsDialogMessage
        HWND myWnd2;
        if(myWnd2=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL))) {
          if(SendMessage(myWnd2,WM_GETDLGCODE,wParam,(DWORD)NULL /*msg..*/) & 
            (DLGC_WANTALLKEYS | DLGC_WANTCHARS))
            SendMessage(myWnd2,message,wParam,lParam);
          }
        else
          return DefWindowProc(hWnd,message,wParam,lParam);
        }
      else {
        // hmmm no if(hWnd->parent && !hWnd->parent->internalState)
        //  return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      return 0;
      break;
    case WM_KEYDOWN:
      if(hWnd->enabled) {   
        //IsDialogMessage
        switch(wParam) {
          case VK_RETURN:
          { BYTE n=GetWindowByte(hWnd,DWL_INTERNAL+1);		// il default button
            HWND myWnd=GetDlgItem(hWnd,n);
            if(myWnd /*&& myWnd->style & BS_DEFPUSHBUTTON*/) {   // solo se c'è... ed è DEFPUSHBUTTON?
              switch((DWORD)myWnd->menu) { 
                case IDOK:            // Notify the owner window to carry out the task. 
                  EndDialog(hWnd,1);
                  return TRUE; 
                  break;
                case IDCANCEL: 
                  EndDialog(hWnd,0);
                  return TRUE; 
                  break;
                } 
              }
  // hmm no serve per cose particolari..          if(hWnd->parent)
  //            SendMessage(hWnd->parent,WM_NOTIFY,0, 0 /*NMHDR struct*/);
          }
            break;
          case VK_TAB:
            SendMessage(hWnd,WM_NEXTDLGCTL,GetAsyncKeyState(VK_SHIFT) & 0x8000 ? 1 : 0,MAKELONG(0,0));
       // se c'è una child EDIT (con multiline) andrebbe inoltrato...?
//            err_puts("DLGtab ");
            
            
            break;
  /* no :) ..        case VK_SPACE:
          {HWND myWnd=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));
        printf("premo SPAZIO: %u\r\n",myWnd);
          if(myWnd)
            SendMessage(hWnd,WM_COMMAND,MAKELONG((WORD)myWnd->tag,BN_CLICKED),myWnd);
          }
            break;*/
          default:
            {HWND myWnd2;
            if(myWnd2=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL))) {
              if(SendMessage(myWnd2,WM_GETDLGCODE,wParam,(DWORD)NULL /*msg..*/) & 
                DLGC_WANTALLKEYS)      
                SendMessage(myWnd2,message,wParam,lParam);
              else if((SendMessage(myWnd2,WM_GETDLGCODE,wParam,(DWORD)NULL /*msg..*/) & DLGC_WANTARROWS) && 
                (wParam>=VK_PRIOR && wParam<=VK_DOWN))
                SendMessage(myWnd2,message,wParam,lParam);
              }
            else
              return DefWindowProc(hWnd,message,wParam,lParam);
              }
            break;
          }
        }
      else {
        // hmmm no if(hWnd->parent && !hWnd->parent->internalState)
        //  return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      break;
    case WM_KEYUP:
      if(hWnd->enabled) {  
        //IsDialogMessage
        {HWND myWnd2;
        if(myWnd2=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL))) {
          if(SendMessage(myWnd2,WM_GETDLGCODE,wParam,(DWORD)NULL /*msg..*/) & DLGC_WANTALLKEYS)      
            SendMessage(myWnd2,message,wParam,lParam);
          else if((SendMessage(myWnd2,WM_GETDLGCODE,wParam,(DWORD)NULL /*msg..*/) & DLGC_WANTARROWS) && 
            (wParam>=VK_PRIOR && wParam<=VK_DOWN))
            SendMessage(myWnd2,message,wParam,lParam);
          }
        else
          return DefWindowProc(hWnd,message,wParam,lParam);
          }
        }
      else {
        // hmmm no if(hWnd->parent && !hWnd->parent->internalState)
        //  return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      return 0;
      break;
    case WM_CHARTOITEM:
      //Sent by a list box with the LBS_WANTKEYBOARDINPUT style to its owner in response to a WM_CHAR message.
      break;
    case WM_VKEYTOITEM:
      //Sent by a list box with the LBS_WANTKEYBOARDINPUT style to its owner in response to a WM_KEYDOWN message.
      break;
      
//    case WM_GETDLGCODE: // E' PER i controls/children
//wParam=The virtual key, pressed by the user, that prompted Windows to issue this notification. 
//DLGC_WANTALLKEYS      
//DLGC_WANTCHARS
//      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcDlgMessageBox(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_KEYDOWN:
      if(hWnd->enabled) {   //
        //IsDialogMessage
        switch(wParam) {
          case VK_RETURN:
            {HWND myWnd=GetDlgItem(hWnd,IDOK);
            if(myWnd /*&& myWnd->style & BS_DEFPUSHBUTTON*/) {   // solo se c'è... ed è DEFPUSHBUTTON?
              EndDialog(hWnd,1);
              return TRUE; 
              }
//              SendMessage(hWnd,WM_COMMAND,MAKELONG(IDOK,0),0);
  // hmm no serve per cose particolari..          if(hWnd->parent)
  //            SendMessage(hWnd->parent,WM_NOTIFY,0, 0 /*NMHDR struct*/);
            else {
              myWnd=GetDlgItem(hWnd,IDYES);
              if(myWnd /*&& myWnd->style & BS_DEFPUSHBUTTON*/) {   // solo se c'è... ed è DEFPUSHBUTTON?
                EndDialog(hWnd,1);      // boh ok
                return TRUE; 
              }
//                SendMessage(hWnd,WM_COMMAND,MAKELONG(IDYES,0),0);
              }
            }
            break;
          case VK_ESCAPE:
            {HWND myWnd=GetDlgItem(hWnd,IDCANCEL);
            if(myWnd /*&& myWnd->style & BS_DEFPUSHBUTTON*/) {   // solo se c'è... ed è DEFPUSHBUTTON?
              EndDialog(hWnd,0);
              return TRUE; 
              }
//              SendMessage(hWnd,WM_COMMAND,MAKELONG(IDCANCEL,0),0);
  // hmm no serve per cose particolari..          if(hWnd->parent)
  //            SendMessage(hWnd->parent,WM_NOTIFY,0, 0 /*NMHDR struct*/);
            }
            break;
          default:
            return DefDlgProc(hWnd,message,wParam,lParam);
            break;
          }
        }
      else {
        // hmmm no if(hWnd->parent && !hWnd->parent->internalState)
        //  return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      return 0;
      break;
    case WM_CHAR:
      // bah questi me li mangio qua cmq! in questo caso non servono a nessun figlio
      break;
      
    case WM_COMMAND:
//no qua!      if(HIWORD(wParam) == 0) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) { 
          case IDOK:
          case IDCANCEL:
          case IDABORT:
          case IDRETRY:
          case IDIGNORE:
          case IDYES:
          case IDNO:
            EndDialog(hWnd,LOWORD(wParam));
            return TRUE; 
            break;
          default:
            break;
          }
        return DefDlgProc(hWnd,message,wParam,lParam);
//        }
      break;
          
    default:
      return DefDlgProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


  
/*static */void calculateClientArea(HWND hWnd,RECT *rc) {
  
  *rc=hWnd->nonClientArea;
  if(!hWnd->minimized) {
    if(hWnd->style & WS_BORDER) {
      rc->top+=BORDER_SIZE;
      rc->bottom-=BORDER_SIZE;
      rc->left+=BORDER_SIZE;
      rc->right-=BORDER_SIZE;
      }
    if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION)) {
      rc->top+=TITLE_HEIGHT;
      }
    if(!(hWnd->style & WS_CHILD) && hWnd->menu) {
      rc->top+=MENU_HEIGHT+1;
      }
    if(hWnd->style & WS_HSCROLL) {
      rc->bottom -= SCROLL_SIZE + (hWnd->style & WS_BORDER ? 0 : 1);//hWnd->style & WS_BORDER ? (SCROLL_SIZE+BORDER_SIZE) : SCROLL_SIZE; // se c'è bordo, lo scroll usa il bordo, se no no!
      }
    if(hWnd->style & WS_VSCROLL) {
      rc->right -= SCROLL_SIZE + (hWnd->style & WS_BORDER ? 0 : 1);//hWnd->style & WS_BORDER ? (SCROLL_SIZE+BORDER_SIZE) : SCROLL_SIZE;
      }
    }
  else {  // bordino fisso (v. disegna)
    rc->top++;
    rc->bottom--;
    rc->left++;
    rc->right--;
    }
  
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  if(wc->style & CS_BYTEALIGNCLIENT) {
    rc->left = (rc->left + 7) & 0xfff8;   // boh credo :)
    rc->right &= 0xfff8;
    }
  
  }

static void calculateNonClientArea(HWND hWnd,RECT *rc) {

  if(!hWnd->minimized) {
    if(hWnd->style & (WS_HSCROLL | WS_VSCROLL)) {
  //    hWnd->scrollSizeX= rc->right-rc->left - (hWnd->style & WS_VSCROLL ? SCROLL_SIZE : 0);
      // forse pure -border...
  //    hWnd->scrollSizeX /= 3;   // test..
  //    hWnd->scrollSizeY=rc->bottom-rc->top  - (hWnd->style & WS_HSCROLL ? SCROLL_SIZE : 0)
  //      - (hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION) ? TITLE_HEIGHT : 0);
  //    hWnd->scrollSizeY /= 3;   // test..
      }
    else {
//boh perché^      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      }
    }
  else {
    }
  }

BOOL GetWindowInfo(HWND hWnd,WINDOWINFO *pwi) {
  
  pwi->wCreatorVersion=MAKEWORD(BREAKTHROUGH_VERSION_L,BREAKTHROUGH_VERSION_H);
  pwi->atomWindowType=0;
  pwi->rcWindow=hWnd->nonClientArea;
  pwi->rcClient=hWnd->clientArea;
  pwi->dwStyle=hWnd->style;
//  pwi->dwExStyle=hWnd->styleEx;
  pwi->dwWindowStatus=MAKELONG(MAKEWORD(hWnd->internalState,hWnd->internalStateTemp),hWnd->status);
  pwi->cxWindowBorders=BORDER_SIZE;
  pwi->cyWindowBorders=BORDER_SIZE;
  
  return TRUE;
  }

// ----------------------------------------------------------------------------
int RegisterClass(const WNDCLASS *lpWndClass) {
  WNDCLASS *myClass;
  
  if(GetClassInfo(0,lpWndClass->class,&myClass))    // controllo che non ci sia già...
    return 0;
  if(!rootClasses)
    rootClasses=(WNDCLASS *)lpWndClass;
  else {
    myClass=rootClasses;
    while(myClass && myClass->next) {
      myClass=myClass->next;
      }
    myClass->next=(WNDCLASS *)lpWndClass;
    }
  ((WNDCLASS *)lpWndClass)->next=NULL;
  return 1;
  }

BOOL UnregisterClass(CLASS Class, HINSTANCE hInstance) {
  WNDCLASS *myClass,*myClass2,*theClass;
  
  if(GetClassInfo(0,Class,&theClass)) {    // DOVREBBE SALTARE quelle predefinite!
    myClass=rootClasses;
    while(myClass) {
      myClass2=myClass;
      if(myClass==theClass) {
        if(myClass==rootClasses)
          rootClasses=myClass->next;
        else
          myClass2->next=myClass->next;
        return 1;
        }
      myClass=myClass->next;
      }
    }
  return 0;
  }

static HWND findWindowHelper(HWND myWnd,const CLASS *Class, const char *lpWindowName) {
  
  while(myWnd) {
    if(Class) {
      if(myWnd->class.class==Class->class) {
        return myWnd;
        }
      else 
        continue;
      }
    if(lpWindowName) {
      if(!stricmp(myWnd->caption,lpWindowName)) {
        return myWnd;
        }
      else 
        continue;
      }
    myWnd=myWnd->next;
    }
  
  return NULL;
  }
HWND FindWindow(const CLASS *Class, const char *lpWindowName) {
  HWND myWnd;
  
  if(!(myWnd=findWindowHelper(rootWindows,Class,lpWindowName)))
    if(!(myWnd=findWindowHelper(taskbarWindow,Class,lpWindowName)))
      myWnd=findWindowHelper(desktopWindow,Class,lpWindowName);
    
  return myWnd;
  }

HWND FindWindowEx(HWND hWndParent,HWND hWndChildAfter,const CLASS *Class, const char *lpWindowName) {
  HWND myWnd;
  
  if(hWndParent)
    myWnd=hWndParent;
  else
    myWnd=rootWindows /* FINIRE GetDesktopWindow()*/;
  if(hWndChildAfter) {
    myWnd=myWnd->children;
//  if(!myWnd)
//    return NULL;
    while(myWnd) {
      if(myWnd==hWndChildAfter) {
        myWnd=myWnd->next;
        break;
        }
      myWnd=myWnd->next;
      }
    }
  if(!(myWnd=findWindowHelper(myWnd,Class,lpWindowName)))
    if(!(myWnd=findWindowHelper(taskbarWindow,Class,lpWindowName)))
      myWnd=findWindowHelper(desktopWindow,Class,lpWindowName);
  
  return myWnd;
  }

int GetClassName(HWND hWnd,CLASS *Class) {
  
  *Class=hWnd->class;
  return sizeof(Class);
  }

inline DWORD __attribute__((always_inline)) GetClassLong(HWND hWnd,int nIndex) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  return *((DWORD *)GET_CLASS_OFFSET(wc,nIndex));
  }

inline WORD __attribute__((always_inline)) GetClassWORD(HWND hWnd,int nIndex) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  return MAKEWORD(*GET_CLASS_OFFSET(wc,nIndex),*GET_CLASS_OFFSET(wc,nIndex+1));
  }

inline DWORD __attribute__((always_inline)) GetWindowLong(HWND hWnd,int nIndex) {
  return *((DWORD *)GET_WINDOW_OFFSET(hWnd,nIndex));
  }

inline void __attribute__((always_inline)) SetClassLong(HWND hWnd,int nIndex,DWORD value) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  *((DWORD *)GET_CLASS_OFFSET(wc,nIndex))=value;
  }

inline void __attribute__((always_inline)) SetWindowLong(HWND hWnd,int nIndex,DWORD value) {
  *((DWORD *)GET_WINDOW_OFFSET(hWnd,nIndex))=value;
  }

inline BYTE __attribute__((always_inline)) GetClassByte(HWND hWnd,int nIndex) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  return *((BYTE *)GET_CLASS_OFFSET(wc,nIndex));
  }

inline BYTE __attribute__((always_inline)) GetWindowByte(HWND hWnd,int nIndex) {
  return *((BYTE *)GET_WINDOW_OFFSET(hWnd,nIndex));
  }
inline WORD __attribute__((always_inline)) GetWindowWord(HWND hWnd,int nIndex) {
  return MAKEWORD(*((BYTE *)GET_WINDOW_OFFSET(hWnd,nIndex)),*((BYTE *)GET_WINDOW_OFFSET(hWnd,nIndex+1)));
  }

inline void __attribute__((always_inline)) SetClassByte(HWND hWnd,int nIndex,BYTE value) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  *((BYTE *)GET_CLASS_OFFSET(wc,nIndex))=value;
  }

inline void __attribute__((always_inline)) SetWindowByte(HWND hWnd,int nIndex,BYTE value) {
  *((BYTE *)GET_WINDOW_OFFSET(hWnd,nIndex))=value;
  }
inline void __attribute__((always_inline)) SetWindowWord(HWND hWnd,int nIndex,WORD value) {
  *((BYTE *)GET_WINDOW_OFFSET(hWnd,nIndex))=LOBYTE(value);
  *((BYTE *)GET_WINDOW_OFFSET(hWnd,nIndex+1))=HIBYTE(value);
  }


BOOL nonClientPaint(HWND hWnd,const RECT *rc,BYTE mode) {
  DC myDC;
  HDC hDC;
  UGRAPH_COORD_T myx,myy;
  union {
    struct {
      unsigned int drawCaption:1;
      unsigned int drawMenu:1;
      unsigned int drawBorder:1;
      unsigned int unused3:1;
      unsigned int drawHScroll:1;
      unsigned int drawVScroll:1;
      unsigned int drawClient:1;
      unsigned int drawBackground:1;
      };
    BYTE b;
    } mask;    //
  RECT rc2={0,0,hWnd->nonClientArea.right-hWnd->nonClientArea.left,
    hWnd->nonClientArea.bottom-hWnd->nonClientArea.top};

  
//#ifndef USING_SIMULATOR
  if(!hWnd->visible)
    return 0;

  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);

  if(!rc)
    mask.b=0xff;
  else {
    mask.b=0; //fare if style qua? tutti? inutile cmq controllo sotto
    if(rc->top>=hWnd->nonClientArea.top && rc->bottom<=hWnd->nonClientArea.top+TITLE_HEIGHT)
      mask.drawCaption = 1;
    if(rc->top>=hWnd->nonClientArea.top+TITLE_HEIGHT && rc->bottom<=hWnd->nonClientArea.top+TITLE_HEIGHT+MENU_HEIGHT)
      mask.drawMenu = 1;
    if(rc->top>=hWnd->nonClientArea.bottom-SCROLL_SIZE && rc->bottom<=hWnd->nonClientArea.bottom) // IntersectRect
      mask.drawHScroll = 1;
    myy=hWnd->nonClientArea.top;
    if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION))
      myy+=TITLE_HEIGHT;
    if(rc->top>=myy && rc->bottom<=hWnd->nonClientArea.bottom) {  // IntersectRect
      if(rc->left>=hWnd->nonClientArea.right-SCROLL_SIZE && rc->right<=hWnd->nonClientArea.right)
        mask.drawVScroll = 1;
      mask.drawClient=1;
      }
    }
  
  hDC=GetWindowDC(hWnd,&myDC);
  
  if(hWnd->minimized) {
    ReleaseDC(hWnd,hDC);
    hDC=GetWindowDC(hWnd /*GetDesktopWindow()*/,&myDC);//sbagliato?? DC hWnd? credo
    if(hWnd->style & WS_BORDER)
      DrawEdge(hDC,&rc2,0,BF_TOPLEFT | BF_BOTTOMRIGHT);
    drawIcon8(hDC,4,4,
      hWnd->icon ? hWnd->icon : standardIcon); // se icon è NULL poi la finestra può disegnarci, riceve WM_PAINT
    ReleaseDC(GetDesktopWindow(),hDC);
    return;
    }
  
  if(hWnd->maximized) {
    // FINIRE! o forse non serve altro, una volta cambiate le dimensioni..
    }
  
  hDC->foreColor=hDC->pen.color=mode ? windowForeColor : windowInactiveForeColor;
  // e hWnd->enabled ??
  
  if(mask.drawBackground) {
    fillRectangleWindowColor(hDC,rc2.left,rc2.top,rc2.right,rc2.bottom,windowBackColor);
// usare      DrawEdge(hDC,&rc2,0,BF_MIDDLE);
    if(wc->style & (CS_OWNDC | CS_SAVEBITS)) {
      //usare per fare DrawRectangle diretto! v. paint ecc.
      }
    }
  if(mask.drawBorder) {
    if(hWnd->style & WS_BORDER) {		// questo sempre
  //    drawRectangleWindow(hDC,rc2.left,rc2.top,rc2.right,rc2.bottom);
      DrawEdge(hDC,&rc2,0,BF_TOPLEFT | BF_BOTTOMRIGHT);
      if(hWnd->style & WS_THICKFRAME) {
        drawRectangleWindow(hDC,rc2.left+1,rc2.top+1,rc2.right-1,rc2.bottom-1);
// usare      DrawEdge(hDC,&rc2,0,BF_TOPLEFT | BF_BOTTOMRIGHT);
        }
      }
    }
  if(wc->style & CS_DROPSHADOW && !(hWnd->style & WS_CHILD)) {    // direi non child ren
    // tarocco che cmq va :) ovviamente poi DOPO può succedere che l'ombra venga sovrascritta... sistemare se si vuole
    hWnd->nonClientArea.right+=SHADOW_SIZE; hWnd->nonClientArea.bottom+=SHADOW_SIZE;
    hWnd->paintArea.right+=SHADOW_SIZE; hWnd->paintArea.bottom+=SHADOW_SIZE;
    hDC=GetWindowDC(hWnd,&myDC);
    hDC->pen.size=SHADOW_SIZE;
    drawVertLineWindowColor(hDC,rc2.right+BORDER_SIZE,rc2.top,rc2.bottom,GRAY032);
    drawHorizLineWindowColor(hDC,rc2.left,rc2.bottom+BORDER_SIZE,rc2.right,GRAY032);
//    DrawEdge(hDC,&rc2,0,BF_RIGHT | BF_BOTTOM);   // usare!
    hWnd->nonClientArea.right-=SHADOW_SIZE; hWnd->nonClientArea.bottom-=SHADOW_SIZE;
    hWnd->paintArea.right-=SHADOW_SIZE; hWnd->paintArea.bottom-=SHADOW_SIZE;
    hDC=GetWindowDC(hWnd,&myDC);
    }
  
  if(hWnd->style & WS_THICKFRAME) {
    rc2.top++;
    rc2.bottom--;
    rc2.left++;
    rc2.right--;
    }
  
  if(mask.drawCaption) {
    BYTE flags=0;

    if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION))
      flags |= DC_BUTTONS;
    if(hWnd->style & WS_CAPTION)
      flags |= DC_TEXT;
    if((hWnd->style & WS_SYSMENU) && !(hWnd->style & WS_CHILD))
      flags |= DC_ICON;   // sarà poi anche :)
    if(mode)
      flags |= DC_ACTIVE;

    DrawCaption(hWnd,hDC,&rc2,flags);
    }
  
  if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION)) {
    if(mask.drawCaption || mask.drawMenu) {
      drawHorizLineWindow(hDC,rc2.left,rc2.top+TITLE_HEIGHT,rc2.right-1);
      }
    rc2.top=rc2.top+TITLE_HEIGHT;
    }
  if(hWnd->menu && !(hWnd->style & WS_CHILD)) {
    if(mask.drawMenu) {
      drawHorizLineWindow(hDC,rc2.left,rc2.top+MENU_HEIGHT+1,rc2.right-1);
      SendMessage(hWnd,WM_INITMENU,(WPARAM)hWnd->menu,MAKELONG(0,1));
      drawMenu(hWnd,hWnd->menu,BORDER_SIZE,TITLE_HEIGHT+(BORDER_SIZE-1),mode,0);
      }
    rc2.top=rc2.top+MENU_HEIGHT+BORDER_SIZE;
    }
  if(mask.drawHScroll) {
    // e usare DrawFrameControl con DFCS_SCROLLLEFT ecc per frecce
    if(hWnd->style & WS_HSCROLL) {
      UGRAPH_COORD_T x,x2;
      drawHorizLineWindow(hDC,rc2.left,rc2.bottom-SCROLL_SIZE,rc2.right-1);
      x2=hWnd->clientArea.right-hWnd->clientArea.left-2;
      if(hWnd->style & WS_VSCROLL) 
        x2-=SCROLL_SIZE;
      if(hWnd->scrollSizeX) {
        x2=x2/hWnd->scrollSizeX;
        x=hWnd->scrollPosX ? x/hWnd->scrollPosX : 0;
        fillRectangleWindowColor(hDC,rc2.left+2+x,rc2.bottom-SCROLL_SIZE+2,
          rc2.left+2+x+x2-1,rc2.bottom-2,hDC->pen.color);
//      drawHorizArrowWindow(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE size,BYTE width,BYTE dir);
      //DrawFrameControl(hDC,&rc2,DFC_SCROLL | DFCS_SCROLLLEFT,DFCS_FLAT);
      //DrawFrameControl(hDC,&rc2,DFC_SCROLL | DFCS_SCROLLRIGHT,DFCS_FLAT);
        }
      }
    else {
//      fillRectangleWindowColor(hDC,newleft+2,newbottom-SCROLL_SIZE, // non è perfetto... il colore.. forse basterebbe lasciare alla client area...
 //             newright-3,newbottom-3,hDC->brush.color);
      }
    }
  if(mask.drawVScroll) {
    if(hWnd->style & WS_VSCROLL) {
      UGRAPH_COORD_T y,y2;
      drawVertLineWindow(hDC,rc2.right-SCROLL_SIZE,rc2.top,rc2.bottom-1);
      y2=hWnd->clientArea.bottom-hWnd->clientArea.top-2;
      if(hWnd->style & WS_HSCROLL) 
        y2-=SCROLL_SIZE;
      if(hWnd->scrollSizeY) {
        y2=y2/hWnd->scrollSizeY; 
        y=hWnd->scrollPosX ? y/hWnd->scrollPosX : 0;
        fillRectangleWindowColor(hDC,rc2.right-SCROLL_SIZE+2,rc2.top+2+y,
          rc2.right-2,rc2.top+2+y+y2,hDC->pen.color);
//      drawVertArrowWindow(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE size,BYTE height,BYTE dir);
      //DrawFrameControl(hDC,&rc2,DFC_SCROLL | DFCS_SCROLLUP,DFCS_FLAT);
      //DrawFrameControl(hDC,&rc2,DFC_SCROLL | DFCS_SCROLLDOWN,DFCS_FLAT);
        }
     }
    else {
//      fillRectangleWindowColor(hDC,newright-SCROLL_SIZE,newtop+2,
 //             newright-3,newbottom-3,hDC->brush.color);
      }
    }
  if(hWnd->style & WS_SIZEBOX) {
    if(mask.b) {    // bah diciamo :)
      drawLineWindow(hDC,rc2.right-1,rc2.bottom-TITLE_ICON_WIDTH,rc2.right-TITLE_ICON_WIDTH,rc2.bottom-1);
      //DrawFrameControl(hDC,&rc2,DFC_SCROLL | DFCS_SCROLLSIZEGRIP,DFCS_FLAT);
      }
    }

  ReleaseDC(hWnd,hDC);
//#endif
  
  calculateClientArea(hWnd,&hWnd->clientArea);    // mah io penso sia giusto sempre qua..

  return 1;
  }

static BOOL clientPaintHelper(HWND myWnd,HWND pWnd,const RECT *rc) {
  BYTE i=0;
  
  while(myWnd) {
    if(myWnd->visible) {
      RECT rc3;
            
      WS_CLIPSIBLINGS>>25; //gestire

      if(IntersectRect(&rc3,&myWnd->nonClientArea,rc)) {
        SendMessage(myWnd,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);
        InvalidateRect(myWnd,NULL     /*&rc3*/,TRUE);    // 
        clientPaintHelper(myWnd->children,myWnd,&rc3);
        }
      i=1;
      }
    myWnd=myWnd->next;
    }
  return i;
  }
BOOL clientPaint(HWND hWnd,const RECT *rc) {
  HWND myWnd;
  
  if(!rc) {
    rc=&hWnd->paintArea;
    }
  
  if(!hWnd->visible)
    return;
  if(hWnd->parent /* WS_CHILD*/ && hWnd->parent->minimized)
    return;
  
  if(!hWnd->minimized)
    clientPaintHelper(hWnd->children,hWnd,rc);
  
	SetRectEmpty(&hWnd->paintArea);   // facendo così poi tutte le line/ecc non vanno, giustamente...
  // non saprei come fare, per ora così ossia sempre tutto abilitato, v. BeginPaint
//    hWnd->paintArea=hWnd->hDC.area;

  return 1;
  }

BOOL InvalidateRect(HWND hWnd,const RECT *rc,BOOL bErase) {

  if(!hWnd) {   // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-invalidaterect
    PaintDesktop(NULL,0);
    
    {HWND myWnd=rootWindows;
    while(myWnd) {
      SendMessage(myWnd,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);		// ncpaint fa/implica pure paint qua...
      InvalidateRect(myWnd,NULL,TRUE);
      
//#warning usare qua, ma dentro nc_paint
			// magari legato a CS_OWNDC o CS_SAVEBITS ?
//      DrawWindow(myWnd->nonClientArea.left,myWnd->nonClientArea.top,
//              myWnd->nonClientArea.right,myWnd->nonClientArea.bottom,
//              windowInactiveForeColor,windowBackColor); // se la finestra è sotto le altre, come qua!
      
      myWnd=myWnd->next;
      }
    SendMessage(taskbarWindow,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);		// ncpaint fa/implica pure paint qua...
    InvalidateRect(taskbarWindow,NULL,TRUE);
    }

    DrawCursor(mousePosition.x,mousePosition.y,mouseCursor,0);
    // in teoria prendere quello di WindowFromPoint(mousePosition);

    }
  else {
    if(!rc) {
      rc=&hWnd->clientArea;
//      GetClientRect(hWnd,&hWnd->hDC.paintArea);
      }
    UnionRect(&hWnd->paintArea,&hWnd->paintArea,rc);    // accumulo le aree paint...
    if(!hWnd->minimized || !hWnd->icon)   // solo se non c'è icona!
      SendMessage(hWnd,WM_PAINT,0,0);   // normalmente non sarebbe qua ma fatto dal sistema quando la coda è vuota...
    // v. anche updatewindow
    }
  
  return 1;
  }

BOOL ValidateRect(HWND hWnd,const RECT *lpRect) {
  
  if(!hWnd) {
    InvalidateRect(NULL,NULL,TRUE);
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);
    InvalidateRect(hWnd,NULL,TRUE);
    }
  else {
    if(lpRect) {
      UnionRect(&hWnd->paintArea,&hWnd->paintArea,lpRect);
      }
    else {
      SetRectEmpty(&hWnd->paintArea);
      }
    }
  }

BOOL ValidateRgn(HWND hWnd,HRGN hRgn) {
  // sistemare...
  if(hRgn) {
    UnionRect(&hWnd->paintArea,&hWnd->paintArea,hRgn);
    }
  else {
    SetRectEmpty(&hWnd->paintArea);
    }
  }

BOOL GetUpdateRect(HWND hWnd,RECT *lpRect,BOOL bErase) {

	if(lpRect)
		*lpRect=hWnd->paintArea;
//	else bah sì mi pare
//		return !IsRectEmpty(&hWnd->paintArea);

	if(bErase && !IsRectEmpty(&hWnd->paintArea)) {
    PAINTSTRUCT ps;
    HDC hDC=BeginPaint(hWnd,&ps);
//		SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&hWnd->hDC,0);
    EndPaint(hWnd,&ps);
    }
  return !IsRectEmpty(&hWnd->paintArea);
	}

BOOL GetUpdateRgn(HWND hWnd,HRGN hRgn,BOOL bErase) {
  
  return GetUpdateRect(hWnd,hRgn,bErase);     // per ora così!
	}


WORD SetTimer(HWND hWnd,WORD nIDEvent,DWORD uElapse,TIMERPROC *lpTimerFunc) {
  BYTE i,j;
  
  if(hWnd) {
    for(j=0; j<MAX_TIMERS; j++) {
      if(hWnd==timerData[j].hWnd && timerData[j].uEvent == nIDEvent) {
        i=j;
        goto replacetimer;
        break;
        }
      }
    }
  else {
    for(j=0; j<MAX_TIMERS; j++) {
      if(timerData[j].uEvent == nIDEvent) {
        i=j;
        goto replacetimer;
        break;
        }
      }
    }
//bah... If the hWnd parameter is NULL, and the nIDEvent does not match an existing timer then it is ignored and a new timer ID is generated.
  for(i=0; i<MAX_TIMERS; i++) {

    if(!timerData[i].uEvent) {
      timerData[i].hWnd=hWnd;
      timerData[i].uEvent=nIDEvent;
      
replacetimer:      
      uElapse=max(uElapse,USER_TIMER_MINIMUM);
      uElapse=min(uElapse,USER_TIMER_MAXIMUM);
      timerData[i].time_cnt=timerData[i].timeout=uElapse;  
      timerData[i].tproc=lpTimerFunc;
      return i+1;
      }
    }
  return 0;
  }

BOOL KillTimer(HWND hWnd,WORD uIDEvent) {
  BYTE i;
  
  for(i=0; i<MAX_TIMERS; i++) {
    if(hWnd) {
      if(hWnd==timerData[i].hWnd && timerData[i].uEvent == uIDEvent)
        goto delete;
      }
    else {
      if(timerData[i].uEvent == uIDEvent) {
delete:
        timerData[i].uEvent=0;
        timerData[i].hWnd=0;
        timerData[i].timeout=0;
        timerData[i].tproc=NULL;
        timerData[i].time_cnt=0;
        return TRUE;
        }
      }
    }
  return FALSE;
  }

BOOL EnableWindow(HWND hWnd,BOOL bEnable) {
  
  hWnd->style &= ~WS_DISABLED;
  if(!bEnable)
    hWnd->style |= WS_DISABLED;
  else
    SendMessage(hWnd,WM_CANCELMODE,0,0);
  hWnd->enabled=bEnable;
  SendMessage(hWnd,WM_ENABLE,bEnable,0);
  }

BOOL IsWindowVisible(HWND hWnd) {
  
  return hWnd->visible;
  }

BOOL IsWindowEnabled(HWND hWnd) {
  
  return hWnd->enabled;
  }

  
BOOL RectVisible(HDC hDC,const RECT *lprect) {
  RECT rc;
  OffsetRect2(&rc,&hDC->area,-hDC->area.left,-hDC->area.top);

  return lprect->left>=rc.left && lprect->right<rc.right &&   // o hWnd->cliparea?? 
          lprect->top>=rc.top && lprect->bottom<rc.bottom;
  }

BOOL PtVisible(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  RECT rc;
  OffsetRect2(&rc,&hDC->area,-hDC->area.left,-hDC->area.top);
  
  return x>=rc.left && x<rc.right &&   // o hWnd->cliparea?? 
          y>=rc.top && y<rc.bottom;
  }

static void destroyWindowHelper(HWND hWnd) {
  HWND myWnd2;
  while(hWnd) {
    myWnd2=hWnd->next;
    DestroyWindow(hWnd);
    hWnd=myWnd2;
    }
  }
BOOL DestroyWindow(HWND hWnd) {

  SendMessage(hWnd,WM_ACTIVATE,MAKELONG(WA_INACTIVE,hWnd->minimized),     // METTERE QUA??
    (LPARAM)GetActiveWindow() /*andrebbe gestita ma ok*/);
  SendMessage(hWnd,WM_DESTROY,0,0);
  
  if(hWnd->style & WS_CHILD) {
    //if(!WS_EX_NOPARENTNOTIFY) mettere
    SendMessage(hWnd->parent,WM_PARENTNOTIFY,MAKELONG(WM_DESTROY,(WORD)(DWORD)hWnd->menu),(LPARAM)hWnd); // tranne WS_EX_NOPARENTNOTIFY ..
		// child di child no?... ricorsivo?
    if(hWnd->parent->style & WS_CHILD) {
//      DestroyWindow(hWnd);
      }
    
    destroyWindowHelper(hWnd->children);
    
	  SendMessage(hWnd,WM_NCDESTROY,0,0);		// DOPO aver tolto le children!
//    printf("remove child %X, %X\r\n",hWnd,hWnd->parent);
    
    removeWindow(hWnd,&hWnd->parent->children);
    list_bubble_sort(&hWnd->parent->children);
//    InvalidateRect(hWnd->parent,NULL,TRUE);     // lo fa parentnotify...
    }
  else {
    destroyWindowHelper(hWnd->children);
	  SendMessage(hWnd,WM_NCDESTROY,0,0);		// DOPO aver tolto le children!
    removeWindow(hWnd,&rootWindows);
    list_bubble_sort(&rootWindows);
    
    HWND myWnd2=NULL;
    HWND myWnd=rootWindows;
    while(myWnd) {    // ora attivo la più in alto rimasta
      myWnd2=myWnd;
//			printf("destr HWND: %x\r\n",myWnd);
      myWnd=myWnd->next;
      }
    if(myWnd2)    // direi che ci sarà sempre ;) ma...
      setActive(myWnd2,TRUE);
    InvalidateRect(NULL,NULL,TRUE);
    }
  free(hWnd);
  
  //usare interesectrect per ridisegnare desktop dopo destroy ecc
  }

BOOL CloseWindow(HWND hWnd) {
  
  ShowWindow(hWnd,SW_MINIMIZE);
  /*
  hWnd->minimized=1;
  SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
  SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&hWnd->hDC,0);
  SendMessage(hWnd,WM_PAINT,0,0);
  InvalidateRect( */
  }

BOOL RedrawWindow(HWND hWnd,const RECT *lprcUpdate,HRGN hrgnUpdate,WORD flags) {
  
  if(!hWnd)
    hWnd=desktopWindow;
  
//    PaintDesktop(NULL,0);
  
  if(flags & RDW_ERASE) {
//    SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&hWnd->hDC,0);
    hWnd->paintArea=hWnd->clientArea;
    }
  if(flags & RDW_FRAME) {
    hWnd->paintArea=hWnd->nonClientArea;    // bah per ora ok
    }
  if(flags & RDW_INVALIDATE) {
    if(lprcUpdate)      // finire...
      hWnd->paintArea=*lprcUpdate;
    else if(lprcUpdate)
      hWnd->paintArea=*hrgnUpdate;
    else
      hWnd->paintArea=hWnd->clientArea;
    }
  if(flags & RDW_ERASENOW) {
    if(flags & RDW_ALLCHILDREN) {
      }
    if(flags & RDW_NOCHILDREN) {
      }
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
    InvalidateRect(hWnd,NULL,TRUE);
//    PostMessage(hWnd,WM_PAINT,0,0);
    }
  if(flags & RDW_UPDATENOW) {
    if(flags & RDW_ALLCHILDREN) {
      }
    if(flags & RDW_NOCHILDREN) {
      }
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
    InvalidateRect(hWnd,NULL,TRUE);
//    SendMessage(hWnd,WM_PAINT,0,0);
    }
  }

#define BLOCK_LINES_2_SCROLL 8
BOOL ScrollWindow(HWND hWnd,int16_t XAmount,int16_t YAmount,const RECT *lpRect,const RECT *lpClipRect) {
  GFX_COLOR savedpixels[640*2*BLOCK_LINES_2_SCROLL];    // ovviamente cambiare con schermo più grande...
  int i,j,x,y,x1,y1,h,w,yResidual;
  GFX_COLOR *p3;
  RECT rc;
  
  if(!lpRect)
    GetClientRect(hWnd,&rc);
  else
    rc=*lpRect;
  OffsetRect(&rc,hWnd->clientArea.left,hWnd->clientArea.top);
  
  ShowCursor(FALSE); HideCaret(hWnd);   // però forse sarebbe meglio usare createsprite direttamente..
  
  x1=rc.left; y1=rc.top; w=rc.right-rc.left; h=rc.bottom-rc.top;
  
  Scroll(rc.left,rc.top,rc.right,rc.bottom,XAmount,YAmount);
  
  
  // finire! ovviamente con asse Z, ev. usare BitBlt
#if 0  
  do {
    yResidual=min(YAmount,BLOCK_LINES_2_SCROLL);
    // PER ORA SOLO IN SU! 30/11/21 quind sarebbe YAmount negativo :)
    p3=savedpixels;
    /* GESTIRE SCROLL!
    START_WRITE();
    setAddrWindowForRead(x1,y1,w,h);
    for(y=0; y<yResidual; y++) {
      for(i=0; i<w; i++) {
        *p3++=readdata16();
        }
      }
    END_WRITE();
     * */

    p3=((GFX_COLOR *)savedCursorArea)+yResidual*w;
    // in teoria qua si potrebbe pure usare la Write in blocco, tanto siamo SOPRA tutto il resto!
    /* GESTIRE SCROLL!
    START_WRITE();
    setAddrWindow(x1,y1,w,h);
    for(y=0; y<yResidual; y++) {
      for(i=0; i<w; i++) {
        writedata16(*p3++);
        }
      }
    END_WRITE();
     * */
    
    YAmount-=y;
    
    } while(YAmount>0);
#endif
  ShowCursor(TRUE);   
  drawCaret(hWnd,caretPosition.x,caretPosition.y,NULL,FALSE);    // e si dovrebbe pure fermare il timer di chi lo visualizza...
  
  //The area uncovered by ScrollWindow is not repainted, but it is combined into the window's update region. 
  if(XAmount>0) {
    rc.left=0;
    rc.right=XAmount;
    }
  else if(XAmount<0) {
    rc.left=rc.right-XAmount;
    }
  if(YAmount>0) {
    rc.top=0;
    rc.bottom=XAmount;
    }
  else if(YAmount>0) {
    rc.top=rc.bottom-YAmount;
    }
  UnionRect(&hWnd->paintArea,&hWnd->paintArea,&rc);
  return TRUE;
  }

int ScrollWindowEx(HWND hWnd,int16_t dx,int16_t dy,const RECT *prcScroll,const RECT *prcClip,
  HRGN hrgnUpdate,RECT *prcUpdate,WORD flags) {
  GFX_COLOR savedpixels[320*2*BLOCK_LINES_2_SCROLL];    // ovviamente cambiare con schermo più grande...
  int i,j,x,y,x1,y1,h,w,yResidual;
  GFX_COLOR *p3;
  RECT rc;
  
  if(!prcScroll)
    GetClientRect(hWnd,&rc);
  else
    rc=*prcScroll;
  OffsetRect(&rc,hWnd->clientArea.left,hWnd->clientArea.top);
  
  ShowCursor(FALSE); HideCaret(hWnd);
  
  x1=rc.left; y1=rc.top; w=rc.right-rc.left; h=rc.bottom-rc.top;
  
  Scroll(rc.left,rc.top,rc.right,rc.bottom,dx,dy);
  
  // FINIRE! v. sopra
  do {
    yResidual=min(dy,BLOCK_LINES_2_SCROLL);
    // PER ORA SOLO IN SU! 30/11/21
    p3=savedpixels;
    
    /* GESTIRE SCROLL!
    START_WRITE();
    setAddrWindowForRead(x1,y1,w,h);
    for(y=0; y<yResidual; y++) {
      for(i=0; i<w; i++) {
        *p3++=readdata16();
        }
      }
    END_WRITE();
     * */

//    p3=((GFX_COLOR *)savedCursorArea)+yResidual*w;
    // in teoria qua si potrebbe pure usare la Write in blocco, tanto siamo SOPRA tutto il resto!
    /* GESTIRE SCROLL!
    START_WRITE();
    setAddrWindow(x1,y1,w,h);
    for(y=0; y<yResidual; y++) {
      for(i=0; i<w; i++) {
        writedata16(*p3++);
        }
      }
    END_WRITE();
     * */
    
    dy-=y;
    
    } while(dy>0);
    
  rc.top=rc.bottom-dy;     // FINIRE!

  if(prcUpdate) {
    *prcUpdate=rc;
    }
          
  if(flags & SW_ERASE) {
    InvalidateRect(hWnd,NULL /*hrgnUpdate*/,flags & SW_ERASE);
    }
          
  ShowCursor(TRUE);   
  drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,FALSE); 
  
  return TRUE;
  }

BOOL UpdateWindow(HWND hWnd) {
  
  if(!IsRectEmpty(&hWnd->paintArea))
    return SendMessage(hWnd,WM_PAINT,0,0);
  else 
    return FALSE;
  }
  
BOOL SetWindowPos(HWND hWnd,HWND hWndInsertAfter,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,
  UGRAPH_COORD_T cx,UGRAPH_COORD_T cy, DWORD uFlags) {
  RECT oldNCarea,oldArea;
  int temp;
  WINDOWPOS wpos;
  NCCALCSIZE_PARAMS nccp;
	MINMAXINFO mmi;
  
  if(1 || uFlags) {    // bah, controllare
    wpos.hwnd=hWnd;
    wpos.hwndInsertAfter=hWndInsertAfter;
    wpos.x=X;
    wpos.y=Y;
    wpos.cx=cx;
    wpos.cy=cy;
    wpos.flags=uFlags;
    if(!(uFlags & SWP_NOSENDCHANGING))
      SendMessage(hWnd,WM_WINDOWPOSCHANGING,0,(LPARAM)&wpos);
		SendMessage(hWnd,WM_GETMINMAXINFO,0,(LPARAM)&mmi);
		}
  if(!(uFlags & SWP_NOACTIVATE)) {
    if(hWnd->enabled && !(hWnd->style & WS_CHILD))
  		setActive(hWnd,TRUE);
		}
  if(uFlags & SWP_HIDEWINDOW)
    hWnd->visible=0;
  if(uFlags & SWP_SHOWWINDOW)   // bah :) cmq vince questo
    hWnd->visible=1;
//  if(!(uFlags & SWP_NOREPOSITION)) { NO! è come SWP_NOOWNERZORDER e serve se tocco child!
  
  oldNCarea=hWnd->nonClientArea;
  if(!(uFlags & SWP_NOMOVE)) {
    RECT rc;
    rc=oldNCarea;
    if(wpos.x>=mmi.ptMaxPosition.x && wpos.y>=mmi.ptMaxPosition.y) {    // [safety... in effetti non dovrebbe essere così ma se no salta tutto!
      rc.left=wpos.x;
      rc.top=wpos.y;
      }
    rc.right=rc.left+(oldNCarea.right-oldNCarea.left);
    rc.bottom=rc.top+(oldNCarea.bottom-oldNCarea.top);
    hWnd->nonClientArea=rc;
    calculateClientArea(hWnd,&hWnd->clientArea);
    }
  oldNCarea=hWnd->nonClientArea;
  if(!(uFlags & SWP_NOSIZE)) {
    RECT rc;
    rc=oldNCarea;
    if((wpos.cx>=GetSystemMetrics(SM_CXMIN) && wpos.cx<mmi.ptMaxSize.x) && (wpos.cy>=GetSystemMetrics(SM_CYMIN) && wpos.cy<mmi.ptMaxSize.y)) {    // 
      rc.right= rc.left+wpos.cx;
      rc.bottom= rc.top+wpos.cy;
      }
    nccp.lppos=&wpos;
    nccp.rgrc[0]=rc;
    nccp.rgrc[1]=oldNCarea;
    nccp.rgrc[2]=hWnd->clientArea;
    if(((oldNCarea.right != rc.right) || (oldNCarea.bottom != rc.bottom)))
      SendMessage(hWnd,WM_NCCALCSIZE,1,(LPARAM)&nccp);
    hWnd->nonClientArea=rc;
    calculateClientArea(hWnd,&hWnd->clientArea);
    }
  else
    if(uFlags & SWP_FRAMECHANGED)   // boh credo
      SendMessage(hWnd,WM_NCCALCSIZE,1,(LPARAM)&nccp);
  if(!(uFlags & SWP_NOZORDER /* SWP_NOOWNERZORDER che è? */)) {
    switch((int)hWndInsertAfter) {
      case (int)HWND_BOTTOM:
        hWnd->topmost=0;
        temp=rootWindows->zOrder;
        rootWindows->zOrder=hWnd->zOrder;
        hWnd->zOrder=temp;
        break;
      case (int)HWND_NOTOPMOST:
        hWnd->topmost=0;
				hWnd->style &= ~WS_EX_TOPMOST;
        break;
      case (int)HWND_TOP:
        hWnd->topmost=0;
hwnd_top:
        {
        HWND myWnd=rootWindows;
        while(myWnd)
          myWnd=myWnd->next;
        temp=myWnd->zOrder;
        myWnd->zOrder=hWnd->zOrder;
        }
        hWnd->zOrder=temp;
        break;
      case (int)HWND_TOPMOST:
        hWnd->topmost=1;
				hWnd->style |= WS_EX_TOPMOST;
        goto hwnd_top;
        break;
      default:
        insertWindow(hWnd,hWndInsertAfter,&rootWindows);
        break;
      }
    list_bubble_sort(&rootWindows);
    }
  {
//  wpos.hwndInsertAfter=NULL /*boh, da sopra direi, o dopo il sort magari? */;
  wpos.x=hWnd->nonClientArea.left;
  wpos.y=hWnd->nonClientArea.top;
  wpos.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
  wpos.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
// dove perch quando? :)  wpos.flags=hWnd->style; 
  
// io lo farei anche se il doc non lo dice... ma questo causa il problema che le figlie si ridisegnano se c'è questo msg nel padre...
  if(!(uFlags & SWP_NOSENDCHANGING))
    SendMessage(hWnd,WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);    // boh sempre
  }
    
  if(uFlags & SWP_DRAWFRAME) {    // bah usare? per move/size...
    SetColors(WHITE,BLACK,R2_XORPEN);
    DrawRectangle(hWnd->nonClientArea.left,hWnd->nonClientArea.top,
      hWnd->nonClientArea.right,hWnd->nonClientArea.bottom,WHITE);
    SetColors(WHITE,BLACK,R2_COPYPEN);
    }
  
  if(!(uFlags & SWP_NOREDRAW)) {
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
    InvalidateRect(hWnd,NULL,TRUE);
    }
  return 1;
  }

BOOL SetWindowPlacement(HWND hWnd,const WINDOWPLACEMENT *lpwndpl) {
  
//  lpwndpl->length;    // me ne frego :D
  hWnd->nonClientArea=lpwndpl->rcNormalPosition;
  if(hWnd->nonClientArea.left>=Screen.cx)
    hWnd->nonClientArea.left=Screen.cx-2;
  if(hWnd->nonClientArea.top>=Screen.cy)
    hWnd->nonClientArea.top=Screen.cy-2;
  hWnd->savedArea=hWnd->nonClientArea;
  calculateClientArea(hWnd,&hWnd->clientArea);
  lpwndpl->flags;   // bah finire
  }

BOOL GetWindowPlacement(HWND hWnd,WINDOWPLACEMENT *lpwndpl) {
  
//  lpwndpl->length;    // me ne frego :D
  if(hWnd->maximized)
    lpwndpl->rcNormalPosition=hWnd->savedArea;
  else
    lpwndpl->rcNormalPosition=hWnd->nonClientArea;
  lpwndpl->ptMaxPosition=MAKEPOINT(0,0);
  HWND myWnd=GetForegroundWindow();
  lpwndpl->ptMinPosition=MAKEPOINT(4+((myWnd ? myWnd->zOrder : 1) -hWnd->zOrder)*10 ,Screen.cy-16);    // v. desktopclass
  if(hWnd->maximized)
    lpwndpl->showCmd=SW_SHOWMAXIMIZED;
  else if(hWnd->minimized)
    lpwndpl->showCmd=SW_SHOWMINIMIZED;
  else
    lpwndpl->showCmd=SW_SHOWNORMAL;
  lpwndpl->flags=0;   // bah finire
  }

BOOL MoveWindow(HWND hWnd,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,BOOL bRepaint) {
  
  return SetWindowPos(hWnd,NULL,X,Y,nWidth,nHeight,
    /*SWP_NOSENDCHANGING | */ SWP_NOZORDER | SWP_NOACTIVATE
    | (bRepaint ? 0 : SWP_NOREDRAW) );
//MoveWindow sends the WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGED, WM_MOVE, WM_SIZE, and WM_NCCALCSIZE messages to the window.
// ..ma dalle prove NON arriva changing e nemmeno move/size..
  }

BOOL OpenIcon(HWND hWnd) {
  
  return ShowWindow(hWnd,SW_SHOWNORMAL);
  }

WORD TileWindows(HWND hWndParent,BYTE wHow,const RECT *lpRect,UINT cKids,const HWND *lpKids) {
  HWND myWnd;
  //fare...
  if(hWndParent)
    myWnd=hWndParent;
  else
    myWnd= rootWindows /* FINIRE! GetDesktopWindow()*/;
  if(lpKids) {
    myWnd=myWnd->children;
    while(myWnd)
      myWnd=myWnd->next;
    }
  
  myWnd=rootWindows;
  while(myWnd) {
    if(myWnd->maximized) {
      myWnd->maximized=0;
      InvalidateRect(myWnd,NULL,TRUE);
      }
  //Calling TileWindows causes all maximized windows to be restored to their previous size.
    myWnd=myWnd->next;
    }
  }

WORD CascadeWindows(HWND hWndParent,BYTE wHow,const RECT *lpRect,UINT cKids,const HWND *lpKids) {
  HWND myWnd;
  //fare...
  if(hWndParent)
    myWnd=hWndParent;
  else
    myWnd= rootWindows /* FINIRE! GetDesktopWindow()*/;
  if(lpKids) {
    myWnd=myWnd->children;
    while(myWnd)
      myWnd=myWnd->next;
    }
  
  myWnd=rootWindows;
  while(myWnd) {
    if(myWnd->maximized) {
      myWnd->maximized=0;
      InvalidateRect(myWnd,NULL,TRUE);
      }
  //Calling TileWindows causes all maximized windows to be restored to their previous size.
    myWnd=myWnd->next;
    }
  }

WORD MinimizeWindows(HWND hWndParent,const RECT *lpRect) {    // mia aggiunta :)
  RECT rc;
  HWND myWnd=hWndParent,myWndOld;
  
  if(!myWnd)
    myWnd=desktopWindow;
  if(lpRect)
    rc=*lpRect;
  else {
    GetClientRect(myWnd,&rc);
    }
  
  myWndOld=myWnd;
  if(myWnd == desktopWindow) {
    myWnd->locked=1;
    myWnd=rootWindows;
    }
  while(myWnd) {
    if(!myWnd->minimized) {
      ShowWindow(myWnd,SW_SHOWMINNOACTIVE);		// a causa del rimescolamento degli Zorder che deriva... due si sovrappongono sempre...
      }
    myWnd=myWnd->next;
    }
  if(myWndOld == desktopWindow) {
    myWndOld->locked=0;
    }
  return 1;
  }

BOOL PhysicalToLogicalPoint(HWND hWnd,POINT *lpPoint) {
  
  lpPoint;  //fare..
  }

BOOL LPtoDP(HWND hWnd,POINT *lpPoint,int c) {
  
  while(c--) {
    lpPoint;  //fare..
    }
  }

BOOL ShowWindow(HWND hWnd,BYTE nCmdShow) {
  BYTE alreadyUpdated=FALSE;

  if(hWnd->locked) {    // direi così, qualsiasi evento "SHOW"
    if(activeMenu) {
      SendMessage(hWnd,WM_UNINITMENUPOPUP,(WPARAM)activeMenu,activeMenu==&systemMenu ? MF_SYSMENU : 0);
      SendMessage(hWnd,WM_EXITMENULOOP,0,0);
      }
    }

  switch(nCmdShow) {
    case SW_HIDE:
      hWnd->visible=0; 
			if(!(hWnd->style & WS_CHILD)) {
        hWnd->topmost=0;
				hWnd->style &= ~WS_EX_TOPMOST;
				alreadyUpdated=setActive(hWnd,FALSE) ? TRUE : FALSE;
				}
SendMessage(hWnd,WM_SHOWWINDOW,FALSE,0);
      break;
    case SW_SHOWNORMAL:
//    case SW_NORMAL:
//boh?      hWnd->topmost=0;
//			hWnd->style &= ~WS_EX_TOPMOST;
      if(hWnd->maximized || hWnd->minimized)
        hWnd->nonClientArea=hWnd->savedArea;
      if(hWnd->maximized) {
        WINDOWPOS wpos;
        hWnd->maximized=0;
				SendMessage(HWND_BROADCAST,WM_SHOWWINDOW,TRUE,SW_OTHERUNZOOM);		// mandare a TUTTI!
        wpos.hwnd=hWnd;
        wpos.hwndInsertAfter=NULL /*boh*/;
        wpos.x=hWnd->nonClientArea.left;
        wpos.y=hWnd->nonClientArea.top;
        wpos.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
        wpos.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
        wpos.flags=SWP_NOREPOSITION | SWP_NOZORDER | SWP_SHOWWINDOW;
        SendMessage(hWnd,WM_WINDOWPOSCHANGING,0,(LPARAM)&wpos);
				SendMessage(HWND_BROADCAST,WM_SHOWWINDOW,SIZE_MAXSHOW,
          MAKELPARAM(hWnd->nonClientArea.right-hWnd->nonClientArea.left,hWnd->nonClientArea.bottom-hWnd->nonClientArea.top));
  //usare interesectrect per ridisegnare desktop dopo destroy ecc
        hWnd->nonClientArea.left=wpos.x;
        hWnd->nonClientArea.top=wpos.y;
        hWnd->nonClientArea.right=hWnd->nonClientArea.left+wpos.cx;
        hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+wpos.cy;
        wpos.hwndInsertAfter=NULL /*boh*/;
        wpos.flags=SWP_NOREPOSITION | SWP_NOZORDER | SWP_SHOWWINDOW;
        SendMessage(hWnd,WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);
        InvalidateRect(NULL,NULL,TRUE);
// ?? sarebbe meglio aggiornare TUTTE le altre tranne questa,        alreadyUpdated=TRUE;
        }
      if(hWnd->minimized) {
        if(!SendMessage(hWnd,WM_QUERYOPEN,0,0))
          return 0;
        hWnd->minimized=0;
//serve cmq per pulire la zona icona... mettere...        InvalidateRect(NULL,NULL,TRUE);
        HWND myWnd=hWnd->children;
        while(myWnd) {
  				SendMessage(myWnd,WM_SHOWWINDOW,TRUE,SW_PARENTOPENING);   // dovrebbe essere ricorsivo dunque
          myWnd=myWnd->next;
          }
        }
      calculateNonClientArea(hWnd,&hWnd->nonClientArea);
      calculateClientArea(hWnd,&hWnd->clientArea);
			SendMessage(hWnd,WM_SIZE,SIZE_RESTORED,
        MAKELPARAM(hWnd->clientArea.right-hWnd->clientArea.left,hWnd->clientArea.bottom-hWnd->clientArea.top));
      // prosegue
    case SW_SHOW:
      hWnd->visible=1;
      if(hWnd->enabled && !(hWnd->style & WS_CHILD))
        alreadyUpdated=setActive(hWnd,TRUE) ? TRUE : FALSE;
SendMessage(hWnd,WM_SHOWWINDOW,TRUE,0);

      break;
    case SW_SHOWMINNOACTIVE:
      hWnd->visible=1;
      if(hWnd->enabled && !(hWnd->style & WS_CHILD))
				alreadyUpdated=setActive(hWnd,FALSE) ? TRUE : FALSE;
//      hWnd->minimized=1;
//      hWnd->maximized=0;
//      alreadyUpdated=TRUE;
      goto do_minimize;
      break;
    case SW_SHOWMINIMIZED:
      hWnd->visible=1;
      if(hWnd->enabled && !(hWnd->style & WS_CHILD))
				alreadyUpdated=setActive(hWnd,TRUE) ? TRUE : FALSE;
    case SW_MINIMIZE:
do_minimize:      
      if(hWnd->style & WS_CHILD)    // v. MDI ...
        break;
      hWnd->topmost=0;
			hWnd->style &= ~WS_EX_TOPMOST;
      if(hWnd->maximized) {
// bah assorbito sotto...				SendMessage(HWND_BROADCAST,WM_NCPAINT,(WPARAM)NULL,0);
				SendMessage(HWND_BROADCAST,WM_SHOWWINDOW,TRUE,SW_OTHERUNZOOM);		// mandare a TUTTI!
  //usare interesectrect per ridisegnare desktop dopo destroy ecc
//assorbito sotto        InvalidateRect(NULL,NULL,TRUE);
        }
      if(!hWnd->minimized) {
        int i;
        WINDOWPOS wpos;
        HWND myWnd=hWnd->children;
        while(myWnd) {
  				SendMessage(myWnd,WM_SHOWWINDOW,FALSE,SW_PARENTCLOSING);   // dovrebbe essere ricorsivo dunque
          myWnd=myWnd->next;
          }
    		myWnd=rootWindows;
        i=0;
    		while(myWnd && myWnd != hWnd) {
        	if(myWnd->minimized)
            i++;
          myWnd=myWnd->next;
          }
        if(!hWnd->maximized) {
          hWnd->savedArea=hWnd->nonClientArea;
   				SendMessage(hWnd,WM_SIZE,SIZE_MINIMIZED,
            MAKELPARAM(hWnd->clientArea.right-hWnd->clientArea.left,hWnd->clientArea.bottom-hWnd->clientArea.top));
          }
        hWnd->minimized=1;
        hWnd->maximized=0;
        hWnd->nonClientArea.top=Screen.cy-16-18;    // v. anche desktopclass
        hWnd->nonClientArea.left=8+((i)*(16+8));
        hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+16;
        hWnd->nonClientArea.right=hWnd->nonClientArea.left+16;
        wpos.hwnd=hWnd;
        wpos.hwndInsertAfter=NULL /*boh*/;
        wpos.x=hWnd->nonClientArea.left;
        wpos.y=hWnd->nonClientArea.top;
        wpos.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
        wpos.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
        wpos.flags=SWP_NOREPOSITION | SWP_NOZORDER | SWP_SHOWWINDOW;
        SendMessage(hWnd,WM_WINDOWPOSCHANGING,0,(LPARAM)&wpos);
        hWnd->nonClientArea.left=wpos.x;
        hWnd->nonClientArea.top=wpos.y;
        hWnd->nonClientArea.right=hWnd->nonClientArea.left+wpos.cx;
        hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+wpos.cy;
        calculateNonClientArea(hWnd,&hWnd->nonClientArea);
        calculateClientArea(hWnd,&hWnd->clientArea);
        wpos.hwndInsertAfter=NULL /*boh*/;
        wpos.flags=SWP_NOREPOSITION | SWP_NOZORDER | SWP_SHOWWINDOW;
        SendMessage(hWnd,WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);
        calculateNonClientArea(hWnd,&hWnd->nonClientArea);
        calculateClientArea(hWnd,&hWnd->clientArea);
//			SendMessage(HWND_BROADCAST,WM_NCPAINT,(WPARAM)NULL,0);
        InvalidateRect(NULL,NULL,TRUE);
        alreadyUpdated=TRUE;
        }
      hWnd->maximized=0;
      break;
    case SW_SHOWMAXIMIZED:
//    case SW_MAXIMIZE:
      if(hWnd->style & WS_CHILD)  // v. MDI ...
        break;
      if(!hWnd->maximized) {
        WINDOWPOS wpos;
        if(!hWnd->minimized)
          hWnd->savedArea=hWnd->nonClientArea;
        hWnd->nonClientArea.top=0;
        hWnd->nonClientArea.left=0;
        hWnd->nonClientArea.bottom=Screen.cy-1;
        hWnd->nonClientArea.right=Screen.cx-1;
        wpos.hwnd=hWnd;
        wpos.hwndInsertAfter=NULL /*boh*/;
        wpos.x=hWnd->nonClientArea.left;
        wpos.y=hWnd->nonClientArea.top;
        wpos.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
        wpos.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
        wpos.flags=SWP_NOREPOSITION | SWP_NOZORDER | SWP_SHOWWINDOW;
        SendMessage(hWnd,WM_WINDOWPOSCHANGING,0,(LPARAM)&wpos);
				SendMessage(HWND_BROADCAST,WM_SHOWWINDOW,FALSE,SW_OTHERZOOM);		// mandare a TUTTI!
        hWnd->nonClientArea.left=wpos.x;
        hWnd->nonClientArea.top=wpos.y;
        hWnd->nonClientArea.right=hWnd->nonClientArea.left+wpos.cx;
        hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+wpos.cy;
        calculateNonClientArea(hWnd,&hWnd->nonClientArea);
        calculateClientArea(hWnd,&hWnd->clientArea);
        wpos.hwndInsertAfter=NULL /*boh*/;
        wpos.flags=SWP_NOREPOSITION | SWP_NOZORDER | SWP_SHOWWINDOW;
        SendMessage(hWnd,WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);
				SendMessage(HWND_BROADCAST,WM_SHOWWINDOW,SIZE_MAXHIDE,
          MAKELPARAM(hWnd->nonClientArea.right-hWnd->nonClientArea.left,hWnd->nonClientArea.bottom-hWnd->nonClientArea.top));
        HWND myWnd=hWnd->children;
        while(myWnd) {
          SendMessage(myWnd,WM_SHOWWINDOW,TRUE,SW_PARENTOPENING);
          myWnd=myWnd->next;
          }
 				SendMessage(hWnd,WM_SIZE,SIZE_MAXIMIZED,
          MAKELPARAM(hWnd->clientArea.right-hWnd->clientArea.left,hWnd->clientArea.bottom-hWnd->clientArea.top));
//mettere    SendMessage(hWnd,WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);
        }
      hWnd->maximized=1;
      hWnd->minimized=0;
      hWnd->visible=1;
      if(hWnd->enabled && !(hWnd->style & WS_CHILD))
				alreadyUpdated=setActive(hWnd,TRUE) ? TRUE : FALSE;
      break;
    case SW_SHOWNOACTIVATE:
    case SW_SHOWNA:
      hWnd->visible=1;
      hWnd->maximized=hWnd->minimized=0;
      break;
    case SW_RESTORE:
      if(hWnd->maximized || hWnd->minimized) {
        hWnd->nonClientArea=hWnd->savedArea;
        calculateNonClientArea(hWnd,&hWnd->nonClientArea);
        calculateClientArea(hWnd,&hWnd->clientArea);
//				SendMessage(HWND_BROADCAST,WM_NCPAINT,(WPARAM)NULL,0);
 				SendMessage(hWnd,WM_SIZE,SIZE_RESTORED,
          MAKELPARAM(hWnd->clientArea.right-hWnd->clientArea.left,hWnd->clientArea.bottom-hWnd->clientArea.top));
        InvalidateRect(NULL,NULL,TRUE);
        alreadyUpdated=TRUE;
        }
      hWnd->maximized=hWnd->minimized=0;
      break;
    default:
      break;
    }
  
  if(!alreadyUpdated) {
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
    return InvalidateRect(hWnd,NULL,TRUE);
    }
  else
    return TRUE;
  }

BOOL ShowOwnedPopups(HWND hWnd,BOOL fShow) {
  // non si capisce bene...
  }
  
BOOL BringWindowToTop(HWND hWnd) {
  HWND myWnd;
  BYTE temp;
  
  if(hWnd->style & WS_CHILD) {
    myWnd=getLastChildWindow(hWnd->parent);   // credo solo MDI..
    if(myWnd && myWnd != hWnd) {
      temp=hWnd->zOrder;
      hWnd->zOrder=myWnd->zOrder;
      myWnd->zOrder=temp;
      list_bubble_sort(&hWnd->parent->children);
      return 1;
      }
    }
  else {
    myWnd=GetForegroundWindow();
    if(myWnd && myWnd != hWnd) {
      temp=hWnd->zOrder;
      hWnd->zOrder=myWnd->zOrder;
      myWnd->zOrder=temp;
      list_bubble_sort(&rootWindows);
      return 1;
      }
    }
  return 0;   //NON deve accadere
  }

HWND GetTopWindow(HWND hWnd) {
  HWND myWnd,myWnd2;
//  BYTE minZorder=255;
  
  if(hWnd) {    // child windows...
    myWnd=hWnd->children;
    while(myWnd) {
      myWnd2=myWnd;
      myWnd=myWnd->next;
      }
    return myWnd2;
    }
  else {
    myWnd=rootWindows;
    while(myWnd) {
      myWnd2=myWnd;
      myWnd=myWnd->next;
      }
    return myWnd2;
    }
  }

BOOL SetForegroundWindow(HWND hWnd) {   // aumentare anche priorità thread...
  HWND myWnd;
  BYTE temp;

	if(hWnd->style & WS_CHILD)
    return FALSE;
  // basterebbe setActive? o no??

	// e CHE FARE con topmost?? https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setforegroundwindow
	// https://stackoverflow.com/questions/3940346/foreground-vs-active-window

  myWnd=rootWindows;
  while(myWnd && myWnd->next)    // vado al fondo
    myWnd=myWnd->next;
  temp=myWnd->zOrder;
  myWnd->zOrder=hWnd->zOrder;
  hWnd->zOrder=temp;
  list_bubble_sort(&rootWindows);
  }

BOOL LockSetForegroundWindow(BYTE uLockCode) {
  //??
  }

void SwitchToThisWindow(HWND hWnd,BOOL fUnknown) {
  SetForegroundWindow(hWnd);
  }

HWND SetFocus(HWND hWnd) {
  HWND myWnd=GetFocus();
  
  if(myWnd)
    SendMessage(myWnd,WM_KILLFOCUS,(WPARAM)hWnd,0);
	SendMessage(hWnd,WM_SETFOCUS,(WPARAM)myWnd,0);
	return myWnd;
	}

static HWND getFocusHelper(HWND myWnd) {
  
  while(myWnd) {
    if(myWnd->focus)
      return myWnd;
    else {
      HWND myWnd2=getFocusHelper(myWnd->children);
      if(myWnd2)
        return myWnd2;
      }
    myWnd=myWnd->next;
    }
  return NULL;
  }
HWND GetFocus(void) {
  
  return getFocusHelper(rootWindows);    // dovrebbe SOLO essere quella Foreground e attiva...
	}

HWND SetActiveWindow(HWND hWnd) {
  HWND oWnd;

// idem come sopra...  
  if(!(hWnd->style & WS_CHILD)) {
    oWnd=setActive(hWnd,TRUE);
    return oWnd;
    }
  else
    return NULL;
  }

HWND GetActiveWindow() {

  return GetForegroundWindow();   // non è esattamente così ma ok
  }

HWND GetForegroundWindow(void) {
  HWND myWnd;
  
  myWnd=rootWindows;
  while(myWnd && myWnd->next)    // vado al fondo
    myWnd=myWnd->next;
  return myWnd;
  }

HWND SetParent(HWND hWndChild,HWND hWndNewParent) {
  HWND hWnd=hWndChild->parent;
  
  if(hWndChild->style & WS_CHILD && hWndNewParent != hWndChild /* frocio :D */)
    hWndChild->parent=hWndNewParent;
	// dice anche che dev'essere l'utente a togliere WS_CHILD/mettere POPUP se il nuovo parent è null e viceversa
  return hWnd;
  }

HWND GetParent(HWND hWnd) {
  
  return hWnd->parent;
  }

BOOL GetClientRect(HWND hWnd,RECT *lpRect) {
  
  lpRect->left=lpRect->top=0;
  lpRect->right=hWnd->clientArea.right-hWnd->clientArea.left;
  lpRect->bottom=hWnd->clientArea.bottom-hWnd->clientArea.top;
  }

BOOL GetWindowRect(HWND hWnd,RECT *lpRect) {
  
  *lpRect=hWnd->nonClientArea;
  }

BOOL SetIcon(HWND hWnd,const GFX_COLOR *icon,BYTE type) {

  return SendMessage(hWnd,WM_SETICON,type,(LPARAM)icon);
  }

BOOL SetFont(HWND hWnd,FONT font) {

  SendMessage(hWnd,WM_SETFONT,(WPARAM)&font,TRUE);
  }

BOOL IsWindow(HWND hWnd) {
  HWND myWnd;
  
  myWnd=rootWindows;
  while(myWnd) {
    if(myWnd == hWnd)
      return TRUE;
    myWnd=myWnd->next;
    }
  return FALSE;
  }

BOOL IsZoomed(HWND hWnd) { 
  return hWnd->maximized;
  }

BOOL IsIconic(HWND hWnd) {
  return hWnd->minimized;
  }

BOOL AdjustWindowRect(RECT *lpRect,DWORD dwStyle,BOOL bMenu) {
  
  if(dwStyle & (WS_HSCROLL | WS_VSCROLL)) {   // v. calculateNonclientArea
  //    hWnd->scrollSizeX= rc->right-rc->left - (hWnd->style & WS_VSCROLL ? SCROLL_SIZE : 0);
      // forse pure -border...
  //    hWnd->scrollSizeX /= 3;   // test..
  //    hWnd->scrollSizeY=rc->bottom-rc->top  - (hWnd->style & WS_HSCROLL ? SCROLL_SIZE : 0)
  //      - (hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION) ? TITLE_HEIGHT : 0);
  //    hWnd->scrollSizeY /= 3;   // test..
    }
  else {
//    hWnd->scrollSizeX=hWnd->scrollSizeY=0;
    }
  
  if(dwStyle & WS_BORDER) {
    lpRect->bottom+=((dwStyle & WS_THICKFRAME) ? 2 : 1)*2;    // v. BORDER_SIZE...
    lpRect->right+=((dwStyle & WS_THICKFRAME) ? 2 : 1)*2;
    }
  if(dwStyle & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION)) {
    lpRect->bottom += TITLE_HEIGHT;
    }
  if(!(dwStyle & WS_CHILD) && bMenu) {
    lpRect->bottom += MENU_HEIGHT+1;
    }
  if(dwStyle & WS_HSCROLL) {
    lpRect->bottom += dwStyle & WS_BORDER ? SCROLL_SIZE-((dwStyle & WS_THICKFRAME) ? 2 : 1) : SCROLL_SIZE; // se c'è bordo, lo scroll usa il bordo, se no no!
    }
  if(dwStyle & WS_VSCROLL) {
    lpRect->right += dwStyle & WS_BORDER ? SCROLL_SIZE-((dwStyle & WS_THICKFRAME) ? 2 : 1) : SCROLL_SIZE;
    }
  
/*  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  if(wc->style & CS_BYTEALIGNCLIENT) {
    rc->left = (rc->left + 7) & 0xfff8;   // boh credo :)
    rc->right &= 0xfff8;
    }*/
  return 1;
  }
BOOL AdjustWindowRectEx(RECT *lpRect,DWORD dwStyle,BOOL bMenu,DWORD dwExStyle) {
  
  AdjustWindowRect(lpRect,dwStyle,bMenu);
  if(dwExStyle) {
    }
  return 1;
  }
  
UINT ArrangeIconicWindows(HWND hWnd) {
	HWND myWnd;
  int i=0;

	if(hWnd && hWnd!=desktopWindow) {		// MDI... o cmq children?
		}
	else {
		myWnd=rootWindows;
		while(myWnd) {
			if(myWnd->minimized) {
        hWnd->nonClientArea.top=Screen.cy-16-18;    // v. anche desktopclass
        hWnd->nonClientArea.left=8+((i)*(16+8));
        hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+16;
        hWnd->nonClientArea.right=hWnd->nonClientArea.left+16;
        calculateNonClientArea(hWnd,&hWnd->nonClientArea);
        calculateClientArea(hWnd,&hWnd->clientArea);
//			SendMessage(HWND_BROADCAST,WM_NCPAINT,(WPARAM)NULL,0);
        InvalidateRect(NULL,NULL,TRUE);
        i++;
				}
			myWnd=myWnd->next;
			}
		}
  }
    
static HWND setActive(HWND hWnd,BOOL state) {
  HWND myWnd,myWnd2=NULL;
  BYTE temp;
// CREDO CHE sulle child non dovrebbe essere fatta... o forse solo per MDI
  // cmq. in giro l'ho filtrata (v.)
  // 17/3/23: questa funziona ok, ma è incoerente con la creazione per cui solo l'ultima child viene attivata,
  //  e poi in certi casi click su una disativa l'altra (correttamente) DECIDERE!!
  if(state) {     // ACTIVE
    if(hWnd->active /* WS_EX_NOACTIVATE*/) {
      myWnd=GetFocus();
      if(myWnd && myWnd!=hWnd)
        SendMessage(myWnd,WM_KILLFOCUS,(DWORD)hWnd,0);  //
      SendMessage(hWnd,WM_SETFOCUS,(DWORD)myWnd,0);  // mi attivo 
      return NULL;
      }
    if(hWnd->parent && hWnd->style & WS_CHILD) {
      myWnd=hWnd->parent->children;
      while(myWnd) {
        if(myWnd != hWnd && myWnd->active) {  // tendenzialmente dovrebbe essere sempre l'ultima, quella attiva...
          SendMessage(myWnd,WM_ACTIVATE,MAKEWORD(WA_INACTIVE,myWnd->minimized),(LPARAM)hWnd);
          SendMessage(myWnd,WM_NCACTIVATE,FALSE,myWnd->active ? 0 : -1);
          myWnd->active=0;
          SendMessage(myWnd,WM_KILLFOCUS,(WPARAM)hWnd,0);  // qua? e discendenti..
          activateChildren(myWnd,0);
          InvalidateRect(myWnd,NULL,TRUE);
          if(hWnd->zOrder < myWnd->zOrder) {
            temp=hWnd->zOrder;			// mah non è una grande idea lo scambio, sarebbe meglio decrementare tutte le altre
            hWnd->zOrder=myWnd->zOrder;
            myWnd->zOrder=temp;
            list_bubble_sort(&hWnd->parent->children);
            }
          break;
          }
        myWnd2=myWnd;
        myWnd=myWnd->next;
        }
      
      if(myWnd2)
        SendMessage(myWnd2,WM_SETFOCUS,(DWORD)NULL,0);  // attivo l'ultima...
                

/*perché?      myWnd=hWnd->parent;
      while(myWnd && myWnd->next)
        myWnd=myWnd->next;*/
      SendMessage(hWnd,WM_NCACTIVATE,TRUE,hWnd->active ? -1 : 0);
  	  hWnd->active=1;
      activateChildren(hWnd,1);
      SendMessage(hWnd,WM_ACTIVATE,MAKEWORD(WA_ACTIVE,hWnd->minimized),(LPARAM)myWnd);
      InvalidateRect(hWnd,NULL,TRUE);
      }
    else {
      myWnd=rootWindows; // GetActiveWindow()
      while(myWnd) {
        if(myWnd != hWnd /*&& !(myWnd->style & WS_CHILD) && !myWnd->parent*/ && myWnd->active) {  // tendenzialmente dovrebbe essere sempre l'ultima, quella attiva...
          if(myWnd->topmost) {	// bah direi così
            return NULL;			// segnalo nulla di fatto
            }
          SendMessage(myWnd,WM_ACTIVATE,MAKEWORD(WA_INACTIVE,myWnd->minimized),(LPARAM)hWnd);
          SendMessage(myWnd,WM_NCACTIVATE,FALSE,myWnd->active ? 0 : -1);
          myWnd->active=0;
          SendMessage(myWnd,WM_KILLFOCUS,(WPARAM)hWnd,0);  // qua? e discendenti..
          activateChildren(myWnd,0);
          InvalidateRect(myWnd,NULL,TRUE);
          if(hWnd->zOrder < myWnd->zOrder) {
            temp=hWnd->zOrder;			// mah non è una grande idea lo scambio, sarebbe meglio decrementare tutte le altre
            hWnd->zOrder=myWnd->zOrder;
            myWnd->zOrder=temp;
            list_bubble_sort(&rootWindows);
            }
          SendMessage(myWnd,WM_ACTIVATEAPP,myWnd->active,(DWORD)NULL /*thread*/);
          break;
          }
        myWnd=myWnd->next;
        }
      SendMessage(hWnd,WM_NCACTIVATE,TRUE,hWnd->active ? -1 : 0);
/* WS_EX_NOACTIVATE*/  	  hWnd->active=1;
      SendMessage(hWnd,WM_ACTIVATE,MAKEWORD(WA_ACTIVE,hWnd->minimized),(LPARAM)myWnd);
      activateChildren(hWnd,1);
      InvalidateRect(hWnd,NULL,TRUE);
      SendMessage(hWnd,WM_ACTIVATEAPP,hWnd->active,(DWORD)NULL /*thread*/);
      }
      
//    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
//  SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&hWnd->hDC,0);
//    SendMessage(hWnd,WM_PAINT,0,0);
    // c'è già nel/nei chiamante, forse togliere..
    return hWnd;
    }
  else {      // INACTIVE
    if(!hWnd->active)
      return NULL;
      // in teoria quella attiva in precedenza è l'ultima ossia quella in cima a Z-order...PENULTIMA!
    if(hWnd->parent && hWnd->style & WS_CHILD) {
      myWnd=hWnd->parent->children;
      while(myWnd) {
        if(myWnd != hWnd && !myWnd->active) {
          SendMessage(myWnd,WM_NCACTIVATE,TRUE,myWnd->active ? -1 : 0);
 				  myWnd->active=1;
          SendMessage(myWnd,WM_ACTIVATE,MAKEWORD(WA_ACTIVE,myWnd->minimized),(LPARAM)hWnd);
          activateChildren(myWnd,1);
          InvalidateRect(myWnd,NULL,TRUE);
          temp=hWnd->zOrder;		// e idem v.sopra, non scambiare
          hWnd->zOrder=myWnd->zOrder;
          myWnd->zOrder=temp;
          list_bubble_sort(&hWnd->parent->children);
          break;
          }
        myWnd=myWnd->next;
        }
      SendMessage(hWnd,WM_NCACTIVATE,FALSE,hWnd->active ? 0 : -1);
  	  hWnd->active=0;
      SendMessage(hWnd,WM_ACTIVATE,MAKEWORD(WA_INACTIVE,hWnd->minimized),(LPARAM)myWnd);
      InvalidateRect(hWnd,NULL,TRUE);
      }
    else {
      if(hWnd->topmost) {	// bah direi così
        return NULL;			// segnalo nulla di fatto
        }
      //GetForegroundWindow
      myWnd=rootWindows;
      while(myWnd) {
        if(myWnd != hWnd && !myWnd->active) {
          SendMessage(myWnd,WM_NCACTIVATE,TRUE,myWnd->active ? -1 : 0);
/* WS_EX_NOACTIVATE*/  	   				  myWnd->active=1;
          SendMessage(myWnd,WM_ACTIVATE,MAKEWORD(WA_ACTIVE,myWnd->minimized),(LPARAM)hWnd);
          SendMessage(myWnd,WM_SETFOCUS,(DWORD)NULL,0);  // qua? e discendenti..
          activateChildren(myWnd,1);
          InvalidateRect(myWnd,NULL,TRUE);
          temp=hWnd->zOrder;		// e idem v.sopra, non scambiare
          hWnd->zOrder=myWnd->zOrder;
          myWnd->zOrder=temp;
          list_bubble_sort(&rootWindows);
          SendMessage(myWnd,WM_ACTIVATEAPP,myWnd->active,(DWORD)NULL /*thread*/);
          break;
          }
        myWnd=myWnd->next;
        }
      SendMessage(hWnd,WM_NCACTIVATE,FALSE,hWnd->active ? 0 : -1);
  	  hWnd->active=0;
      SendMessage(hWnd,WM_ACTIVATE,MAKEWORD(WA_INACTIVE,hWnd->minimized),(LPARAM)myWnd);
      InvalidateRect(hWnd,NULL,TRUE);
      SendMessage(hWnd,WM_ACTIVATEAPP,hWnd->active,(DWORD)NULL /*thread*/);
      }
    
    SendMessage(hWnd,WM_KILLFOCUS,(DWORD)NULL,0);  // qua? e discendenti..
    activateChildren(hWnd,0);
    return hWnd;
    }
  
  return NULL;    //NON deve accadere!
	}

static void activateChildren(HWND hWnd,int8_t mode) {
  HWND myWnd=hWnd->children;
  
  while(myWnd) {
    switch(mode) {
      case -1:
        break;
      case 0:
        myWnd->active=mode;
        break;
      case 1:
        myWnd->active=mode;
        SendMessage(myWnd,WM_CHILDACTIVATE,0,0);     // solo MDI cmq
        break;
      }

    activateChildren(myWnd,mode);
    myWnd=myWnd->next;
    }
  }

int SetScrollPos(HWND hWnd,BYTE nBar,uint16_t nPos,BOOL bRedraw) {
  RECT rc;
  
  switch(nBar) {
    case SB_CTL:
      if(hWnd->style & SBS_HORZ) {
        hWnd->scrollPosX=nPos;
        }
      else if(hWnd->style & SBS_VERT) {
        hWnd->scrollPosY=nPos;
        }
      if(bRedraw)
        InvalidateRect(hWnd,NULL,TRUE);
      break;
    case SB_HORZ:
      hWnd->scrollPosX=nPos;
      if(bRedraw) {
        rc.left=hWnd->nonClientArea.left;
        rc.bottom=hWnd->clientArea.bottom;
        rc.right=hWnd->nonClientArea.right;
        rc.top=hWnd->clientArea.bottom-SCROLL_SIZE;
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
        }
      break;
    case SB_VERT:
      hWnd->scrollPosY=nPos;
      if(bRedraw) {
        rc.top=hWnd->clientArea.top;
        rc.bottom=hWnd->clientArea.bottom;
        rc.right=hWnd->nonClientArea.right;
        rc.left=hWnd->nonClientArea.right-SCROLL_SIZE;
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
        }
      break;
    }
  return 1;
  }

int SetScrollInfo(HWND hWnd,BYTE nBar,SCROLLINFO *lpsi,BOOL bRedraw) {
  RECT rc;
  
  switch(nBar) {
    case SB_CTL:
      if(hWnd->style & SBS_HORZ) {
        if(lpsi->fMask & SIF_POS)
          hWnd->scrollPosX=lpsi->nPos;
        if(lpsi->fMask & SIF_PAGE)
          hWnd->scrollSizeX=lpsi->nPage;
        if(lpsi->fMask & SIF_RANGE) {
          SetWindowWord(hWnd,GWL_USERDATA,lpsi->nMin);
          SetWindowWord(hWnd,GWL_USERDATA+2,lpsi->nMax);
          }
        if(lpsi->fMask & SIF_DISABLENOSCROLL)
          ;
        }
      else if(hWnd->style & SBS_VERT) {
        if(lpsi->fMask & SIF_POS)
          hWnd->scrollPosY=lpsi->nPos;
        if(lpsi->fMask & SIF_PAGE)
          ;
        if(lpsi->fMask & SIF_RANGE)
          hWnd->scrollSizeY=lpsi->nMax;
        if(lpsi->fMask & SIF_DISABLENOSCROLL)
          ;
        }
      if(bRedraw)
        InvalidateRect(hWnd,NULL,TRUE);
      break;
    case SB_HORZ:
      if(lpsi->fMask & SIF_POS)
        hWnd->scrollPosX=lpsi->nPos;
      if(lpsi->fMask & SIF_PAGE)
        ;
      if(lpsi->fMask & SIF_RANGE)
        hWnd->scrollSizeX=lpsi->nMax;
      if(lpsi->fMask & SIF_DISABLENOSCROLL)
        ;
      if(bRedraw) {
        rc.left=hWnd->nonClientArea.left;
        rc.bottom=hWnd->clientArea.bottom;
        rc.right=hWnd->nonClientArea.right;
        rc.top=hWnd->clientArea.bottom-SCROLL_SIZE;
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
        }
      break;
    case SB_VERT:
      if(lpsi->fMask & SIF_POS)
        hWnd->scrollPosY=lpsi->nPos;
      if(lpsi->fMask & SIF_PAGE)
        ;
      if(lpsi->fMask & SIF_RANGE)
        hWnd->scrollSizeY=lpsi->nMax;
      if(lpsi->fMask & SIF_DISABLENOSCROLL)
        ;
      if(bRedraw) {
        rc.top=hWnd->clientArea.top;
        rc.bottom=hWnd->clientArea.bottom;
        rc.right=hWnd->nonClientArea.right;
        rc.left=hWnd->nonClientArea.right-SCROLL_SIZE;
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
        }
      break;
    }
  return 1;
  }

BOOL SetScrollRange(HWND hWnd,BYTE nBar,uint16_t nMinPos,uint16_t nMaxPos,BOOL bRedraw) {
  RECT rc;
  int oldsize;
  
  switch(nBar) {
    case SB_CTL:
      SetWindowWord(hWnd,GWL_USERDATA,nMinPos);
      SetWindowWord(hWnd,GWL_USERDATA+2,nMaxPos);
      if(bRedraw)
        InvalidateRect(hWnd,NULL,TRUE);
      break;
    case SB_HORZ:
      oldsize=hWnd->scrollSizeX;
      hWnd->scrollSizeX=nMaxPos;
      hWnd->scrollPosX=min(hWnd->scrollPosX,hWnd->scrollSizeX);   // mah, a me pare logico anche se non specificato
      if(bRedraw && oldsize!=hWnd->scrollSizeX) {   // evito rientro/ridisegno (specie quando viene disabilitato)
        rc.left=hWnd->nonClientArea.left;
        rc.bottom=hWnd->clientArea.bottom;
        rc.right=hWnd->nonClientArea.right;
        rc.top=hWnd->clientArea.bottom-SCROLL_SIZE;
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
        }
      break;
    case SB_VERT:
      oldsize=hWnd->scrollSizeY;
      hWnd->scrollSizeY=nMaxPos;
      hWnd->scrollPosY=min(hWnd->scrollPosY,hWnd->scrollSizeY);   // mah, a me pare logico anche se non specificato
      if(bRedraw && oldsize!=hWnd->scrollSizeY) {
        rc.top=hWnd->clientArea.top;
        rc.bottom=hWnd->clientArea.bottom;
        rc.right=hWnd->nonClientArea.right;
        rc.left=hWnd->nonClientArea.right-SCROLL_SIZE;
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
        }
      break;
    }
  return 1;
  }

BOOL ShowScrollBar(HWND hWnd,BYTE wBar,BOOL bShow) {
  RECT rc;
  
  switch(wBar) {
    case SB_CTL:
      ShowWindow(hWnd,bShow);
      break;
    case SB_HORZ:
      if(bShow)
        hWnd->style |= WS_HSCROLL;
      else
        hWnd->style &= ~WS_HSCROLL;
      rc.left=hWnd->nonClientArea.left;
      rc.bottom=hWnd->clientArea.bottom;
      rc.right=hWnd->nonClientArea.right;
      rc.top=hWnd->clientArea.bottom-SCROLL_SIZE;
      SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
      break;
    case SB_VERT:
      if(bShow)
        hWnd->style |= WS_VSCROLL;
      else
        hWnd->style &= ~WS_VSCROLL;
      rc.top=hWnd->clientArea.top;
      rc.bottom=hWnd->clientArea.bottom;
      rc.right=hWnd->nonClientArea.right;
      rc.left=hWnd->nonClientArea.right-SCROLL_SIZE;
      SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
      break;
    }
  return 1;
  }

BOOL EnableScrollBar(HWND hWnd,UINT wSBflags,UINT wArrows) {
  RECT rc;
  
  switch(wSBflags) {
    case SB_CTL:
      switch(wArrows) {
        case ESB_ENABLE_BOTH:
          break;
        }
      break;
    case SB_HORZ:
      switch(wArrows) {
        case ESB_ENABLE_BOTH:
          break;
        }
      break;
    case SB_VERT:
      switch(wArrows) {
        case ESB_ENABLE_BOTH:
          break;
        }
      break;
    case SB_BOTH:
      switch(wArrows) {
        case ESB_ENABLE_BOTH:
          break;
        }
      break;
    }
  return 1;
  }

BOOL SetWindowText(HWND hWnd,const char *title) {

  return SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)title);
  }

int GetWindowText(HWND hWnd,char *lpString,int nMaxCount) {
  
  return SendMessage(hWnd,WM_GETTEXT,nMaxCount,(LPARAM)lpString);
  }

int InternalGetWindowText(HWND hWnd,char *lpString,int nMaxCount) {
  
  int i=min(sizeof(hWnd->caption),nMaxCount);
  strncpy(lpString,hWnd->caption,i-1);
  lpString[i]=0;
  }

BOOL GetTitleBarInfo(HWND hWnd,TITLEBARINFO *pti) {
  
  // if(pti->cbSize=
  pti->rcTitleBar.top=hWnd->nonClientArea.top;
  pti->rcTitleBar.left=hWnd->nonClientArea.left;
  pti->rcTitleBar.right=hWnd->nonClientArea.right;
  if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION))
    pti->rcTitleBar.bottom=pti->rcTitleBar.top+=TITLE_HEIGHT;
  else
    pti->rcTitleBar.bottom=pti->rcTitleBar.top;

  pti->rgstate[0]=hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION) ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  pti->rgstate[2]=hWnd->style & WS_MINIMIZEBOX ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  pti->rgstate[3]=hWnd->style & WS_MAXIMIZEBOX  ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  pti->rgstate[4]=hWnd->style & 0 /*WS_HELP*/ ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  pti->rgstate[5]=hWnd->style & WS_SYSMENU ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  }


int PaintDesktop(HDC hDC, signed char effects) {   // non è esattamente così in Windows, ma ok
    
  if(effects>0) {
    FillRectangle(0,0,Screen.cx,Screen.cy,DARKGRAY);
    CreateSprite(0,0,0,0,0,0,0,1   | 8,SPRITE_SETATTR);    
    }
//#warning  usare DrawWindow togliere fillrectangle unire
  
//  FillRectangle(0,0,Screen.cx,Screen.cy,desktopColor);
//  SendMessage(desktopWindow,WM_NCPAINT,(WPARAM)NULL,0);
  if(effects>=0) {    // inutile se si sta spegnendo :)
    SendMessage(desktopWindow,WM_PAINT,(WPARAM)NULL,0);
    }
//  InvalidateRect(desktopWindow,NULL,TRUE);
  if(effects>=0) {    // inutile se si sta spegnendo :)
    SendMessage(taskbarWindow,WM_NCPAINT,(WPARAM)NULL,0);
    SendMessage(taskbarWindow,WM_PAINT,(WPARAM)NULL,0);
  //  InvalidateRect(taskbarWindow,NULL,TRUE);
    }
      
  if(effects<0) {
    CreateSprite(0,0,0,0,0,0,0,0   | 8,SPRITE_SETATTR);    
    FillRectangle(0,0,Screen.cx,Screen.cy,DARKGRAY);
    FillRectangle(0,0,Screen.cx,Screen.cy,BLACK);
    }
    
  return 1;
  }

BOOL MessageBeep(BYTE uType) {
  BYTE isWave=(eXtra & 16 && GetID(AUDIO_CARD) == 'A');

	switch(uType & 0xf0) {
		case MB_ICONEXCLAMATION:
      if(isWave) {
        SetAudioWave(0,2,1000,0,8,getVolumeAudio(),0,0);
        __delay_ms(400);
        }
      else {
  			StdBeep(400);
        }
			break;
		case MB_ICONHAND:
      if(isWave) {
        SetAudioWave(0,2,660,0,8,getVolumeAudio(),0,0);
        __delay_ms(800);
        }
      else {
  			ErrorBeep(400);
        }
			break;
		case MB_ICONQUESTION:
      if(isWave) {
        SetAudioWave(0,2,1200,0,8,getVolumeAudio(),0,0);
        __delay_ms(400);
        }
      else {
  			StdBeep(400);
        }
			break;
		case MB_ICONASTERISK:
      if(isWave) {
        SetAudioWave(0,1,1000,0,8,getVolumeAudio(),0,0);
        __delay_ms(200);
        }
      else {
  			StdBeep(200);
        }
			break;
		default:
      if(isWave) {
        SetAudioWave(0,2,880,0,8,getVolumeAudio(),0,0);
        __delay_ms(500);
        }
      else {
  			StdBeep(500);
        }
			break;
		}
  if(isWave)
    SetAudioWave(0,0,0,0,8,getVolumeAudio(),0,0);

	}	

int MessageBox(HWND hWnd,const char *lpText,const char *lpCaption,WORD uType) {
	RECT rc;
  BYTE n=0;
  FONT myFont=GetStockObject(SYSTEM_FONT).font;
  const char *p;
  SIZE ds,sz;
  HDC hDC;
  DC myDC;

  hDC=GetDC(hWnd,&myDC);
  GetTextExtentPoint(hDC,lpText,strlen(lpText),&sz);
  ReleaseDC(hWnd,hDC);
  ds.cx=strlen(lpCaption) +3;
  ds.cx *= getFontWidth(&myFont);
  sz.cx+=getFontWidth(&myFont)*2;
  ds.cx=max(ds.cx,sz.cx);
	if((uType & 0xf) == MB_YESNOCANCEL) 
    ds.cx=max(ds.cx,30);
	if(hWnd) {
		RECT rc2;
		GetClientRect(hWnd,&rc2);
  	ds.cx=min(ds.cx,rc2.right-5);
    ds.cx=max(ds.cx,100);
    ds.cy=max(rc2.bottom/3,60);
    if(p)
      ds.cy+=getFontHeight(&myFont);
		rc.top=(rc2.bottom-ds.cy)/2;
		rc.left=(rc2.right-ds.cx)/2;
		}
	else {
  	ds.cx=max(ds.cx,Screen.cx/3);
    if(Screen.cy>200)
      ds.cy=max(ds.cy,Screen.cy/4);
    else
      ds.cy=max(ds.cy,Screen.cy/3);
    if(p)
      ds.cy+=getFontHeight(&myFont);
		rc.top=(Screen.cy-ds.cy)/2;
		rc.left=(Screen.cx-ds.cx)/2;
		}
	HWND myDlg=internalCreateWindow(MAKECLASS(WC_DIALOG),lpCaption,WS_BORDER | WS_SYSMENU | WS_VISIBLE | WS_DLGFRAME | 
    WS_CAPTION | (hWnd ? WS_CHILD : WS_POPUP /*boh direi per ora*/)  | WS_CLIPCHILDREN,
//	HWND myDlg=CreateWindow(MAKECLASS(WC_DIALOG),lpCaption,WS_ACTIVE | WS_EX_TOPMOST | WS_BORDER | WS_SYSMENU | WS_VISIBLE | WS_DLGFRAME | 
//    WS_CAPTION | (hWnd ? WS_CHILD : 0 /*boh direi per ora*/),
    rc.left,rc.top,ds.cx,ds.cy,
    hWnd,(HMENU)NULL,(void*)256,(WINDOWPROC*)DefWindowProcDlgMessageBox
    );
  SendMessage(myDlg,WM_SETFONT,(WPARAM)&myFont,0);

	HWND myWnd2=NULL,myWnd3=NULL,myWnd4=NULL, focusWnd;
  POINT bpos; SIZE bsiz;
  bpos.y=ds.cy-28 /*considera CAPTION*/; bsiz.cy=12;
	switch(uType & 0xf) {
		case MB_OK:
			myWnd2=CreateWindow(MAKECLASS(WC_BUTTON),"OK",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_DEFPUSHBUTTON | BS_CENTER,
				ds.cx/2-16,bpos.y,30,bsiz.cy,
				myDlg,(HMENU)IDOK,NULL
				);
      focusWnd=myWnd2;
      n=IDOK;
			break;
		case MB_OKCANCEL:
			myWnd2=CreateWindow(MAKECLASS(WC_BUTTON),"OK",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
				ds.cx/3-20,bpos.y,40,bsiz.cy,
				myDlg,(HMENU)IDOK,NULL
				);
			myWnd3=CreateWindow(MAKECLASS(WC_BUTTON),"Annulla",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
				(ds.cx*2)/3-20,bpos.y,48,bsiz.cy,
				myDlg,(HMENU)IDCANCEL,NULL
				);
			switch(uType & 0xf00) {
				case MB_DEFBUTTON1:
          focusWnd=myWnd2; n=IDOK;
          myWnd2->style |= BS_DEFPUSHBUTTON;
					break;
				case MB_DEFBUTTON2:
          focusWnd=myWnd3; n=IDCANCEL;
          myWnd3->style |= BS_DEFPUSHBUTTON;
					break;
				}
			break;
		case MB_YESNO:
			myWnd2=CreateWindow(MAKECLASS(WC_BUTTON),"Si",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
				ds.cx/3-16,bpos.y,30,bsiz.cy,
				myDlg,(HMENU)IDYES,NULL
				);
			myWnd3=CreateWindow(MAKECLASS(WC_BUTTON),"No",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
				(ds.cx*2)/3-16,bpos.y,30,bsiz.cy,
				myDlg,(HMENU)IDNO,NULL
				);
			switch(uType & 0xf00) {
				case MB_DEFBUTTON1:
          focusWnd=myWnd2; n=IDYES;
          myWnd2->style |= BS_DEFPUSHBUTTON;
					break;
				case MB_DEFBUTTON2:
          focusWnd=myWnd3; n=IDNO;
          myWnd3->style |= BS_DEFPUSHBUTTON;
					break;
				}
			break;
		case MB_YESNOCANCEL:
			myWnd2=CreateWindow(MAKECLASS(WC_BUTTON),"Si",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
				ds.cx/4-16,bpos.y,30,bsiz.cy,
				myDlg,(HMENU)IDYES,NULL
				);
			myWnd3=CreateWindow(MAKECLASS(WC_BUTTON),"No",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
				(ds.cx*2)/4-16,bpos.y,30,bsiz.cy,
				myDlg,(HMENU)IDNO,NULL
				);
			myWnd4=CreateWindow(MAKECLASS(WC_BUTTON),"Annulla",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
				(ds.cx*3)/4-16,bpos.y,48,bsiz.cy,
				myDlg,(HMENU)IDCANCEL,NULL
				);
			switch(uType & 0xf00) {
				case MB_DEFBUTTON1:
          focusWnd=myWnd2; n=IDYES;
          myWnd2->style |= BS_DEFPUSHBUTTON;
					break;
				case MB_DEFBUTTON2:
          focusWnd=myWnd3; n=IDNO;
          myWnd3->style |= BS_DEFPUSHBUTTON;
					break;
				case MB_DEFBUTTON3:
          focusWnd=myWnd4; n=IDCANCEL;
          myWnd4->style |= BS_DEFPUSHBUTTON;
					break;
				}
			break;
//	e qua?	if(t->style & DS_SETFONT) 
//			SendMessage(myWnd,WM_SETFONT,font,0);
      // faccio così!
    if(myWnd2)
      SendMessage(myWnd2,WM_SETFONT,(WPARAM)&myFont,0);
    if(myWnd3)
      SendMessage(myWnd3,WM_SETFONT,(WPARAM)&myFont,0);
    if(myWnd4)
      SendMessage(myWnd4,WM_SETFONT,(WPARAM)&myFont,0);
		}
  SetWindowByte(myDlg,DWL_INTERNAL+1,n);
	switch(uType & 0xf0) {
		case MB_ICONEXCLAMATION:
//			DrawIcon8();
			break;
		}

  rc.left=(myDlg->clientArea.right-myDlg->clientArea.left-sz.cx)/2;
  if((int)rc.left<0)
    rc.left=0;
  rc.top=(myDlg->clientArea.bottom-myDlg->clientArea.top)/2-sz.cy-4;
  if((int)rc.top<0)
    rc.top=0;
	myWnd2=CreateWindow(MAKECLASS(WC_STATIC),lpText,WS_VISIBLE | WS_CHILD | 
          SS_CENTER,
		rc.left,rc.top,
    sz.cx,sz.cy,myDlg,(HMENU)255,NULL
		);
  SendMessage(myDlg,WM_INITDIALOG,(WPARAM)focusWnd,0);
//  SendMessage(myDlg,WM_SETFONT,(WPARAM)NULL /*sarebbe font da template*/,TRUE);
  SendMessage(myWnd2,WM_SETFONT,(WPARAM)&myFont,0);
  ShowWindow(myDlg,SW_SHOW);
  if(!hWnd)
    hWnd=desktopWindow;   // verificare se va bene...
  if(hWnd) {
    SendMessage(hWnd,WM_ENTERIDLE,MSGF_DIALOGBOX,(LPARAM)myDlg);   //FINIRE, gestire
    }
          
// dovrebbe restare qua e MODAL LOOP v. altri
  return GetWindowLong(myDlg,DWL_MSGRESULT);
	return 1;

	}

BOOL AnyPopup() {
  HWND myWnd=rootWindows;
  
  while(myWnd) {
    if(myWnd->style & (WS_POPUP | WS_OVERLAPPED) && myWnd->parent)
      return TRUE;
    myWnd=myWnd->next;
    }
  return FALSE;
	}


int drawIcon8(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const GFX_COLOR icon[]) {
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i,j;
  const GFX_COLOR *p2=icon;
  
  w=8; h=8;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      drawPixelWindowColor(hDC,x1+i,y1+j,*p2++);
      }
    }
  return j;
  }

int DrawIcon(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const GFX_COLOR icon[]) {
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i, j;
  const GFX_COLOR *p2=icon;
  
  w=16; h=16;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      drawPixelWindowColor(hDC,x1+i,y1+j,*p2++);
      }
    }
  return j;
  }

static int drawBitmap(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const GFX_COLOR bitmap[],UGRAPH_COORD_T xs, UGRAPH_COORD_T ys) {
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i, j;
  const GFX_COLOR *p2=bitmap;
  
  w=xs; h=ys;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      drawPixelWindowColor(hDC,x1+i,y1+j,*p2++);
      }
    }
  return j;
  }

int drawCaret(HWND hWnd,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const CURSOR caret,BYTE bShow) {
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i, j;
  BYTE stretch;
  const GFX_COLOR *p2=caret;
  GFX_COLOR color;
  DC myDC;
  HDC hDC=GetDC(hWnd,&myDC);
  
  
  if(inScreenSaver)     
    #warning finire/SISTEMARE gestendo bShow e il timer del caret e chi lo possiede!
    return 0;
  
  if(activeMenu /*activeMenuWnd*/)
    return 0;
  
// non va mai...  no xché vuole coord sistemate! però in effetti non dovrebbe servire,
  //if(!isPointVisible(hDC,x1+1,y1+1))    // diciam così
//    return 0;
  

  w=4; h=8;
  stretch=getFontHeight(&hWnd->font) / 8;   //occhio se font 5x3..
//  if(stretch<1)
//    stretch=1;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      if(bShow) {
        color=*p2++;
        if(color != CUR_TRNSP) {
          color = color == CUR_WHT ? WHITE : BLACK;
          if(stretch>1)
            fillRectangleWindowColor(hDC,x1+i*stretch,y1+j*stretch,
              x1+i*stretch+stretch-1,y1+j*stretch+stretch-1,color);
          else
            drawPixelWindowColor(hDC,x1+i,y1+j,color);
          }
        }
      else {
        if(color != CUR_TRNSP)
          color = GRAY064;  // si potrebbe fare rasterOp INV...
        else
          color = GRAY128;  //
        
//        if(caret)   // se fare inversione potrà servire...
//          color=*p2++;
        
          // e/o cmq va salvato il colore precedente, direi
          if(stretch>1)
            fillRectangleWindowColor(hDC,x1+i*stretch,y1+j*stretch,
              x1+i*stretch+stretch-1,y1+j*stretch+stretch-1,color);
          else
            drawPixelWindowColor(hDC,x1+i,y1+j,color);    //finire..
          
        }
      }
    }
  ReleaseDC(hWnd,hDC);
  return j;
  }

static void DrawCursor(UGRAPH_COORD_T x,UGRAPH_COORD_T y,const CURSOR cursor,BYTE size) {
  UGRAPH_COORD_T w,h;
// v. anche DrawIconEx, windows fa così..  
  
  if(size) {
    w=16; h=16;
    }
  else {
    w=8; h=8;
    }
  
//  if(cursor != mouseCursor)
//    DefineSprite(0,(GFX_COLOR*)(mouseCursor=cursor));			//
  MovePointer(x,y);

  }

int ShowCursor(BOOL m) {
  
  if(m)
    cursorCnt++;
  else
    cursorCnt--;

  CreateSprite(0,0,0,0,0,0,0,(cursorCnt>=0 ? 1 : 0)   | 8,SPRITE_SETATTR);    
  }

BOOL DrawIconEx(HDC hDC,UGRAPH_COORD_T xLeft,UGRAPH_COORD_T yTop,ICON hIcon,UGRAPH_COORD_T cxWidth,UGRAPH_COORD_T cyWidth,
  UINT istepIfAniCur,BRUSH hbrFlickerFreeDraw,UINT diFlags) {
  
  if(!cxWidth && diFlags & DI_DEFAULTSIZE)
    cxWidth=SM_CXICON;
  if(!cyWidth && diFlags & DI_DEFAULTSIZE)
    cyWidth=SM_CYICON;
  if(cxWidth==8)
    drawIcon8(hDC,xLeft,yTop,hIcon);
  else if(cxWidth==16)
    DrawIcon(hDC,xLeft,yTop,hIcon);
  else
    ;
  
  if(diFlags & DI_MASK)
    ;
  }

BOOL DrawState(HDC hDC,BRUSH hbrFore,DRAWSTATEPROC qfnCallBack,LPARAM lData,
  WPARAM wData,UGRAPH_COORD_T x,UGRAPH_COORD_T y,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy,UINT uFlags) {
  RECT rc={x,y,x+cx,y+cy};
  SIZE sz;
  int i;
  
  switch(uFlags & 0xfff0) {
    }
  switch(uFlags & 0xf) {
    case DST_TEXT:
    case DST_PREFIXTEXT:
      i=strlen((char*)lData);
      if(!cx || !cy) {
        GetTextExtentPoint(hDC,(char*)lData,i,&sz);
        if(!cx)
          rc.right=rc.left+sz.cx;
        if(!cy)
          rc.top=rc.top+sz.cy;
        }
      if((uFlags & 0xf) == DST_PREFIXTEXT)
        DrawText(hDC,(char*)lData,i,&rc,DT_TOP | (uFlags & DSS_RIGHT ? DT_RIGHT : DT_LEFT) |
          (uFlags & DSS_HIDEPREFIX ? DT_HIDEPREFIX : 0) |
          (uFlags & DSS_PREFIXONLY ? DT_PREFIXONLY : 0));
      else
        DrawText(hDC,(char*)lData,i,&rc,DT_TOP | (uFlags & DSS_RIGHT ? DT_RIGHT : DT_LEFT));
      break;
    case DST_ICON:
      if(uFlags & DSS_RIGHT)
        ;
      if(!cx)     // calcolare, finire...
        cx=8;
      if(!cy)
        cy=8;
      if(cx==8 && cy==8)
        drawIcon8(hDC,x,y,(GFX_COLOR*)lData);
      else
        DrawIcon(hDC,x,y,(GFX_COLOR*)lData);
      break;
    case DST_BITMAP:
      if(uFlags & DSS_RIGHT)
        ;
      if(!cx)     // calcolare, finire...
        cx=16;
      if(!cy)
        cy=16;
      drawBitmap(hDC,x,y,(GFX_COLOR*)lData,cx,cy);
      break;
    case DST_COMPLEX:
      if(uFlags & DSS_RIGHT)
        ;
      (*qfnCallBack)(hDC,lData,wData,cx,cy);
      break;
    }
  }

inline uint8_t getFontHeight(HFONT font) {
  if(!font->font)
    return font->base16*font->multiplier16;
  else
    return font->base64 /*yAdvance*/*font->multiplier4;
  }
inline uint8_t getFontWidth(HFONT font) {
  if(!font->font)
    return (font->base16*font->multiplier16*6)/8;
  else
    return (font->base64*font->multiplier4*6)/8;  // xAdvance...
  }
BOOL TextOut(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,const char *s,uint16_t c) {
  char ch;
  
  if(hDC->textAlignment == TA_UPDATECP) {
    x=hDC->textCursor.x;
    y=hDC->textCursor.y;
    }
  while(c--) {
    ch=*s++;
    if(!hDC->font.font) { // 'Classic' built-in font
      switch(ch) {
        case '\n':
          y += getFontHeight(&hDC->font);
          x = 0;
          break;
        case '\r':
          // skip em
          break; 
        default:
          drawCharWindow(hDC,x,y,ch);
          x += getFontWidth(&hDC->font);
          break; 
        }
      } 
    else { // Custom font
      const GFXfont *gfxFont=hDC->font.font;
      
      switch(ch) {
        case '\n':
          x=0;
          y += (GRAPH_COORD_T)hDC->font.multiplier4 * gfxFont->yAdvance;
          break;
        case '\r':
          break;
        default:
          {
          UINT8 first = gfxFont->first;
          if(ch >= first && ch <= gfxFont->last) {
            UINT8   c2 = ch - gfxFont->first;
            const GFXglyph *glyph = &gfxFont->glyph[c2];
            UINT8   w=glyph->width, h=glyph->height;
            if(w>0 && h>0) { // Is there an associated bitmap?
              GRAPH_COORD_T xo = glyph->xOffset; // sic
              drawCharWindow(hDC,x,y,ch);
              }
            x += glyph->xAdvance * (GRAPH_COORD_T)hDC->font.multiplier4;
            }
          }
          break;
        }

      }
    }
  if(hDC->textAlignment == TA_UPDATECP) {
    hDC->textCursor.x=x;
    hDC->textCursor.y=y;
    }
  return 1;
  }

BOOL ExtTextOut(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,UINT options,const RECT *lpRect,const char *s,uint16_t n,const INT *lpDx) {
  char c;
  UINT cnt=0;
  
  if(hDC->textAlignment == TA_UPDATECP) {
    x=hDC->textCursor.x;
    y=hDC->textCursor.y;
    }
  while(cnt<n) {
    c=*s++;
    
    if(!hDC->font.font) { // 'Classic' built-in font

      switch(c) {
        case '\n':
          y += getFontHeight(&hDC->font);
          x = 0;
          break;
        case '\r':
          // skip em
          break; 
        default:
          if(options & ETO_NUMERICSLOCAL)
            ;
          if(options & ETO_SMALL_CHARS)   // questa roba sembra per windows + recenti..
            ;
          drawCharWindow(hDC,x,y,c);
          x += getFontWidth(&hDC->font);
          if(options & ETO_CLIPPED /*&& ! ETO_NO_RECT*/) {
            if(x>=lpRect->right)
              break;
            }
          if(lpDx)
            x+=lpDx[cnt];
          break; 
        }

      }
    else { // Custom font
      const GFXfont *gfxFont=hDC->font.font;

      switch(c) {
        case '\n':
          x = 0;
          y += (GRAPH_COORD_T)hDC->font.multiplier4 * gfxFont->yAdvance;
          break;
        case '\r':
          break;
        default:
          {
          UINT8 first = gfxFont->first;
          if(c >= first && c <= gfxFont->last) {
            UINT8   c2 = c - gfxFont->first;
            const GFXglyph *glyph = &gfxFont->glyph[c2];
            UINT8 w=glyph->width, h=glyph->height;
            if(w>0 && h>0) { // Is there an associated bitmap?
              GRAPH_COORD_T xo = glyph->xOffset; // sic
              if(options & ETO_NUMERICSLOCAL)
                ;
              if(options & ETO_SMALL_CHARS)   // questa roba sembra per windows + recenti..
                ;
              drawCharWindow(hDC,x,y,c);
              }
            x += glyph->xAdvance * (GRAPH_COORD_T)hDC->font.multiplier4;
            if(options & ETO_CLIPPED /*&& ! ETO_NO_RECT*/) {
              if(x>=lpRect->right)
                break;
              }
            }
          }
          break;
        }
    	}

    cnt++;
    }
  
  if(hDC->textAlignment == TA_UPDATECP) {
    hDC->textCursor.x=x;
    hDC->textCursor.y=y;
    }
  
  return 1;
  }
BOOL ExtTextOutWrap(HDC hDC,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UINT uoptions,const RECT *lprc,const char *lpString,uint16_t cbCount,const INT *lpDx) {
  // uguale all'altra, ma gestire wrap! cmq dice che è sconsigliata...
  return ExtTextOut(hDC,X,Y,uoptions,lprc,lpString,cbCount,lpDx);
  }
BOOL PolyTextOut(HDC hDC,const POLYTEXT *ppt,WORD nstrings) {
  
  while(--nstrings) {
    ExtTextOut(hDC,ppt->x,ppt->y,ppt->uiFlags,&ppt->rcl,ppt->lpstr,ppt->n,ppt->pdx);
    ppt++;
    }
  return TRUE;
  }
LONG TabbedTextOut(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,const char *lpString,uint16_t chCount,
  BYTE nTabPositions,const BYTE *lpnTabStopPositions,BYTE nTabOrigin) {
  char buf[2];
  SIZE sz,sz2;
  
  while(chCount--) {
    if(*lpString == '\t') {
      if(nTabPositions--)
        x=nTabOrigin+*lpnTabStopPositions++;    // sicuro?
      }
    else {
      buf[0]=*lpString++; buf[1]=0;
      TextOut(hDC,x,y,buf,1);     // 
      GetTextExtentPoint(hDC,buf,1,&sz2);
      x+=sz2.cx;
      }
    }
  return MAKELONG(sz.cx+x,sz2.cy);      // circa
  }

int DrawText(HDC hDC,const char *lpchText,int cchText,RECT *lprc,UINT format) {
  char c;
  UINT cnt;
  int x,y,newx;
  SIZE sz;
  BYTE next_underlined=0,old_underline=0;
//DOVREMMO CLIPPARE a dx a livello di pixel e non di carattere...
//The text alignment mode for the device context must include the TA_LEFT, TA_TOP, and TA_NOUPDATECP flags.  
  
  if(cchText==-1)
    cchText=strlen(lpchText);
  x=lprc->left;
  y=lprc->top;
  GetTextExtentPoint(hDC,lpchText,cchText,&sz);
  if(format & DT_CALCRECT) {    // boh, direi
    lprc->right=lprc->left+sz.cx;
    lprc->bottom=lprc->top+sz.cy;
    }
  if(format & (DT_VCENTER | DT_BOTTOM)) {   // gestire multiline..
    if(format & DT_BOTTOM)
      y += (((int)(lprc->bottom+1-lprc->top) - sz.cy));
    else
      y += (((int)(lprc->bottom+1-lprc->top) - sz.cy) / 2);
    }
  if(y<(int)lprc->top) 
    y=lprc->top;
  if(format & DT_CENTER) {
    if(format & DT_INTERNAL) {
      }
    x += (((int)(lprc->right-lprc->left) - (int)sz.cx) / 2);
    }
  if(format & DT_RIGHT) {
    if(format & DT_INTERNAL) {
      }
    x = ((int)(lprc->right-lprc->left) - (int)sz.cx);
    }
  if(x<(int)lprc->left) 
    x=lprc->left;
  newx=x;
  
  while(cnt<cchText) {
    c=*lpchText++;
    
    if(!hDC->font.font) { // 'Classic' font

      switch(c) {
        case '\n':
          if(!(format & DT_SINGLELINE)) {
            y += getFontHeight(&hDC->font);
            x = newx;
            }
          break;
        case '\t':
          if(format & DT_EXPANDTABS) {
            x += getFontWidth(&hDC->font);
            while(!((x/getFontWidth(&hDC->font)) & 7))
              x+=getFontWidth(&hDC->font);
            }
          break; 
        case '\r':
          // skip em
          break; 
        case '&':
          if(format & DT_NOPREFIX) {
            goto normal_char;
            }
          else if(format & DT_HIDEPREFIX) {
            goto skippa;
            }
          else if(format & DT_PREFIXONLY) {
            c='_';
            goto normal_char;
            }
          else {
            next_underlined=1;
            }
          break; 
        default:
          if((format & DT_PREFIXONLY) && (c=='&'))
            goto skippa;
normal_char:
          if(next_underlined) {
            old_underline=hDC->font.underline;
            hDC->font.underline=1;
            }
          if(!(format & DT_SINGLELINE) && (x + getFontWidth(&hDC->font) > (lprc->right))) {
            y += getFontHeight(&hDC->font);
            x = newx;
            }
          drawCharWindow(hDC,x,y,c);
          x += getFontWidth(&hDC->font);
          if(next_underlined) {
            hDC->font.underline=old_underline;
            next_underlined=0;
            }
          if(!(format & DT_NOCLIP)) {
            if(x>lprc->right)
              goto fine;
            }
          break; 
        }
      } 
    else { // Custom font
      const GFXfont *gfxFont=hDC->font.font;

      switch(c) {
        case '\n':
          if(!(format & DT_SINGLELINE)) {
            x = newx;
            y += (GRAPH_COORD_T)getFontHeight(&hDC->font);
            }
          break;
        case '\t':
          if(format & DT_EXPANDTABS) {
            x += getFontWidth(&hDC->font);  // perfezionare, con font variabile...
            while(!((x/getFontWidth(&hDC->font)) & 7))
              x+=getFontWidth(&hDC->font);
            }
          break; 
        case '\r':
          break;
        case '&':
          if(format & DT_NOPREFIX) {
            goto normal_char2;
            }
          else if(format & DT_HIDEPREFIX) {
            goto skippa;
            }
          else if(format & DT_PREFIXONLY) {
            c='_';
            goto normal_char2;
            }
          else {
            next_underlined=1;
            }
          break; 
        default:
          if((c=='&') && (format & DT_PREFIXONLY))
            goto skippa;
normal_char2:
          if(next_underlined) {
            old_underline=hDC->font.underline;
            hDC->font.underline=1;
            }
          {
          UINT8 first = gfxFont->first;
          if(c >= first && c <= gfxFont->last) {
            UINT8   c2 = c - gfxFont->first;
            const GFXglyph *glyph = &gfxFont->glyph[c2];
            UINT8 w=glyph->width, h=glyph->height;
            if(w>0 && h>0) { // Is there an associated bitmap?
              UGRAPH_COORD_T xo = glyph->xOffset; // sic
              if(!(format & DT_SINGLELINE) && ((x + getFontWidth(&hDC->font) * (xo+w)) > lprc->right)) {
                // Drawing character would go off right edge; wrap to new line
                x = newx;
                y += (GRAPH_COORD_T)getFontHeight(&hDC->font);
                }
              drawCharWindow(hDC,x,y,c);
              }
            x += glyph->xAdvance * (UGRAPH_COORD_T)hDC->font.multiplier4;
            if(next_underlined) {
              hDC->font.underline=old_underline;
              next_underlined=0;
              }
            if(!(format & DT_NOCLIP)) {
              if(x>lprc->right)
                goto fine;
              }
            }
          }
          break;
        }
    	}

    if(format & DT_TABSTOP)
      ;
    if(format & DT_EXPANDTABS)
      ;
    
    if(format & DT_WORD_ELLIPSIS)
      ;
    
skippa:    
    cnt++;
    }
  
  if(format & DT_PATH_ELLIPSIS)
    ;
  if(format & DT_END_ELLIPSIS)
    ;
  
fine:
  if(format & (DT_VCENTER | DT_BOTTOM))    // v. doc
    return (DWORD)lprc->bottom-(UGRAPH_COORD_T)getFontHeight(&hDC->font);
  else
    return (UGRAPH_COORD_T)getFontHeight(&hDC->font);
  }

void setWindowTextCursor(HDC hDC,BYTE col,BYTE row) {
  
  SetXY(0,col*getFontWidth(&hDC->font),row*getFontHeight(&hDC->font));
//        drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,TRUE);
//  #warning cazzata
  }

BOOL MoveToEx(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,POINT *lppt) {
  
  if(lppt) {
    lppt->x=hDC->cursor.x;
    lppt->y=hDC->cursor.y;
    }
  hDC->cursor.x=x;
  hDC->cursor.y=y;
  return 1;
  }

BOOL LineTo(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  
  drawLineWindow(hDC,hDC->cursor.x, hDC->cursor.y,x,y);
  hDC->cursor.x=x;
  hDC->cursor.y=y;
  return 1;
  }

GFX_COLOR SetPixel(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR c) {

  drawPixelWindowColor(hDC,x,y,c);
  return c; 
  }
  
BOOL SetPixelV(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR c) {

  return drawPixelWindowColor(hDC,x,y,c);   // "approssimazione del colore" :)
  }
  
GFX_COLOR GetPixel(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  POINT pt={x+hDC->area.left,y+hDC->area.top};
  
//  OffsetPoint(pt,hDC->area.left,hDC->area.top);
  if(hDC->hWnd->parent /*hWnd->style & WS_CHILD*/) {  // se sono figlia, calcolo le mie coordinate reali (come drawPixel)
    HWND myWnd=hDC->hWnd->parent;
    while(myWnd) {
      pt.x+=myWnd->clientArea.left;
      pt.y+=myWnd->clientArea.top;
  //    OffsetPoint(pt,int16_t dx,int16_t dy);
      myWnd=myWnd->parent;
      }
    }
  //  ClientToScreen(hDC->hWnd,&pt);
// ORA sulla scheda principale va (a parte il basic, fuori windows) ma su un'altra non andava... provare 21/1

  SetXY(0,pt.x,pt.y);
  __delay_us(2);    //
//serve quando si fanno letture consecutive... ??
  
  return GetPixelColor() ; //(GFX_COLOR)CLR_INVALID;   // 
  // If the pixel is outside of the current clipping region, the return value is CLR_INVALID 
  }
  
BOOL Rectangle(HDC hDC,UGRAPH_COORD_T left,UGRAPH_COORD_T top,UGRAPH_COORD_T right,UGRAPH_COORD_T bottom) {
  
  right--;
  bottom--;    // esclude right e bottom https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-rectangle

  if(hDC->brush.style != BS_NULL)
    fillRectangleWindow(hDC,left,top,right,bottom);
  if(hDC->pen.style == PS_NULL) {
    right--;bottom--;
  //If a PS_NULL pen is used, the dimensions of the rectangle are 1 pixel less in height and 1 pixel less in width.
    }
  else
    drawRectangleWindow(hDC,left,top,right,bottom);
  }

BOOL FillRect(HDC hDC,const RECT *lprc,BRUSH hbr) {
  
  fillRectangleWindowColor(hDC,lprc->left,lprc->top,lprc->right-1,lprc->bottom-1,hbr.color);    // esclude right e bottom https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-fillrect
  return TRUE;
  }

BOOL FillRgn(HDC hDC,HRGN lprc,BRUSH hbr) {
  
  fillRectangleWindowColor(hDC,lprc->left,lprc->top,lprc->right,lprc->bottom,hbr.color);    // per ora così!
  return TRUE;
  }

BOOL FrameRect(HDC hDC,const RECT *lprc,BRUSH hbr) {
// in effetti no, sempre 1!  int i=hbr.size;   https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-framerect

//  do {
    drawRectangleWindowColor(hDC,lprc->left,lprc->top,lprc->right,lprc->bottom,hbr.color);
    // sarebbe giusto centrare lo spessore...
//    } while(--i);

  return TRUE;
  }

BOOL InvertRect(HDC hDC,const RECT *lprc) {
  int x,y;
  
  for(y=lprc->top; y<lprc->bottom; y++) {
    for(x=lprc->left; x<lprc->right; x++) {
      SetPixel(hDC,x,y,~GetPixel(hDC,x,y));
      }
    }
  return 1;
  }

BOOL Ellipse(HDC hDC,UGRAPH_COORD_T left,UGRAPH_COORD_T top,UGRAPH_COORD_T right,UGRAPH_COORD_T bottom) {

  if(hDC->brush.style != BS_NULL)   // è una mia estensione :)
    fillEllipseWindow(hDC,left,top,right,bottom);
  if(hDC->pen.style != PS_NULL /*&& hDC->pen.color != hDC->brush.color bah no :)*/)
    drawEllipseWindow(hDC,left,top,right,bottom);
  }

BOOL Arc(HDC hDC,UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2,
  UGRAPH_COORD_T x3,UGRAPH_COORD_T y3,UGRAPH_COORD_T x4,UGRAPH_COORD_T y4) {
  
//  DrawEllipseWindow(hDC, left, top, right, bottom);
  }

BOOL Polygon(HDC hDC,const POINT *apt,int cpt) {
  POINT oldPt=hDC->cursor;
  
  MoveTo(hDC,apt->x,apt->y);
  while(--cpt) {
    apt++;
    LineTo(hDC,apt->x,apt->y);
    }
  hDC->cursor=oldPt;
  return TRUE;
  }

BOOL Polyline(HDC hDC,const POINT *apt,int cpt) {
  POINT oldPt=hDC->cursor;
  
  MoveTo(hDC,apt->x,apt->y);
  while(--cpt) {
    apt++;
    LineTo(hDC,apt->x,apt->y);
    }
  hDC->cursor=oldPt;
  return TRUE;
  }

BOOL PolylineTo(HDC hDC,const POINT *apt,DWORD cpt) {
  
  MoveTo(hDC,apt->x,apt->y);
  while(--cpt) {
    apt++;
    LineTo(hDC,apt->x,apt->y);
    }
  return TRUE;
  }

int SetPolyFillMode(HDC hDC,int mode) {
  // bah per ora nulla
  }
void FillSolidRect(HDC hDC,const RECT *lpRect, GFX_COLOR clr) {
  fillRectangleWindowColor(hDC,lpRect->left,lpRect->top,lpRect->right,lpRect->bottom,clr);
  }
void FillSolidRectCoords(HDC hDC,int x, int y, int cx, int cy, GFX_COLOR clr) {
  fillRectangleWindowColor(hDC,x,y,x+cx,y+cy,clr);
  }
void Draw3dRect(HDC hDC,const RECT *lpRect,
    GFX_COLOR clrTopLeft,GFX_COLOR clrBottomRight) {
  }
void Draw3dRectCoords(HDC hDC,int x,int y,int cx,int cy,
    GFX_COLOR clrTopLeft,GFX_COLOR clrBottomRight) {
  drawHorizLineWindowColor(hDC,x,y,x+cx,clrTopLeft);
  drawHorizLineWindowColor(hDC,x,y+cy,x+cx,clrBottomRight);
  drawVertLineWindowColor(hDC,x,y,y+cy,clrTopLeft);
  drawVertLineWindowColor(hDC,x+cx,y,y+cy,clrBottomRight);    // per chiudere, v. altri
  }
void DrawDragRect(HDC hDC,const RECT *lpRect,SIZE size,const RECT *lpRectLast,SIZE sizeLast,
    BRUSH pBrush /*=NULL*/,BRUSH pBrushLast /*=NULL*/) {
  
  while(sizeLast.cx--)    // bah non so bene cosa si intenda...
    drawRectangleWindowColor(hDC,lpRectLast->left+sizeLast.cx,lpRectLast->top+sizeLast.cy,
            lpRectLast->right-sizeLast.cx,lpRectLast->bottom-sizeLast.cy,pBrushLast.color);
  while(size.cx--)
    drawRectangleWindowColor(hDC,lpRect->left+size.cx,lpRect->top+size.cy,
            lpRect->right+size.cx,lpRect->bottom+size.cy,pBrush.color);
  }

HRGN CreateRectRgn(int x1,int y1,int x2,int y2) {
  
  theRgn.left=x1;
  theRgn.right=x2;
  theRgn.top=y1;
  theRgn.bottom=y2;
  return &theRgn;
  }

HRGN CreateRectRgnIndirect(const RECT *rect) {
  
  theRgn = *rect;
  return &theRgn;
  }

int GetWindowRgn(HWND hWnd,HRGN hRgn) {
  
  theRgn=hWnd->paintArea;   // sarebbero 2 cose diverse ma per ora faccio così
  *hRgn=theRgn;
  return IsRectEmpty(hRgn) ? SIMPLEREGION : NULLREGION;
  }
int SetWindowRgn(HWND hWnd,HRGN hRgn,BOOL bRedraw) {

  hWnd->paintArea=theRgn;   // sarebbero 2 cose diverse ma per ora faccio così
  WINDOWPOS wpos;
  wpos.hwnd=hWnd;
  wpos.hwndInsertAfter=NULL;
  wpos.x=hWnd->nonClientArea.left;
  wpos.y=hWnd->nonClientArea.top;
  wpos.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
  wpos.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
  wpos.flags=SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW;
  SendMessage(hWnd,WM_WINDOWPOSCHANGING,0,(LPARAM)&wpos);   // bah così dice
  if(bRedraw)
    InvalidateRect(hWnd,hRgn,TRUE);
  }
int GetWindowRgnBox(HWND hWnd,RECT *lprc) {

  *lprc=hWnd->paintArea;   // sarebbe diverso ma per ora faccio così e v. sopra
  return IsRectEmpty(lprc) ? SIMPLEREGION : NULLREGION;
  }


BOOL FillPath(HDC hDC) {
  }

BOOL FloodFill(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR color) {
  
  ExtFloodFill(hDC,x,y,color,FLOODFILLBORDER);
  }

#define MAX_WIDTH 1024
#define MAX_HEIGHT 768
#define MAX_FILL_STACK 512
BOOL ExtFloodFill(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR color,UINT type) {
  BYTE up,down;
  GFX_COLOR bg;
  POINTS pt;
  
	if(type==FLOODFILLBORDER) {
    bg = color;// if bg is None else bg
    POINTS stack[MAX_FILL_STACK];
    int stack_ptr=0;
    
    pt.x=x; pt.y=y;
    stack[stack_ptr++]=pt;
    while(stack_ptr>0) {
      pt = stack[--stack_ptr];
      if(GetPixel(hDC,pt.x,pt.y) != bg)
        continue;
      // find left boundary
      while(pt.x > 0 && GetPixel(hDC,pt.x-1,pt.y) != bg)
        pt.x--;        
      // scan to right
      up=TRUE; down=TRUE;
      while(pt.x < MAX_WIDTH && GetPixel(hDC,pt.x,pt.y) != bg) {
        drawPixelWindowColor(hDC,pt.x,pt.y,hDC->brush.color);
    // detect color change above
        if((pt.y+1) < MAX_HEIGHT) {
          if(up && GetPixel(hDC,pt.x,pt.y+1) != bg) {
            POINTS pt2;
            pt2.x=pt.x; pt2.y=pt.y+1;
            stack[stack_ptr++]=pt2;
            }
          up = GetPixel(hDC,pt.x,pt.y+1) == bg;
          }
    // detect color change below
        if(pt.y>0) {
          if(down && GetPixel(hDC,pt.x,pt.y-1) != bg) {
            POINTS pt2;
            pt2.x=pt.x; pt2.y=pt.y-1;
            stack[stack_ptr++]=pt2;
            }
          down = GetPixel(hDC,pt.x, pt.y-1) == bg;       
          }
        pt.x++;
        }
      if(stack_ptr>=MAX_FILL_STACK)
        return 0;
      }

  	return 1;
    }
	else if(type==FLOODFILLSURFACE) {
    pt.x=x; pt.y=y;
    bg = hDC->foreColor ;// if bg is None else bg
    POINTS stack[MAX_FILL_STACK];
    //sembra andare bene, 3/10/22: 500 dovrebbe bastare.. ogni cambio direzione conta 1...
    int stack_ptr=0;
    
    stack[stack_ptr++]=pt;
    while(stack_ptr>0) {
      pt = stack[--stack_ptr];
      if(GetPixel(hDC,pt.x,pt.y) != bg)
        continue;
      // find left boundary
      while(pt.x > 0 && GetPixel(hDC,pt.x-1,pt.y) == bg)
        pt.x--;        
      // scan to right
      up=TRUE; down=TRUE;
      while(pt.x < MAX_WIDTH && GetPixel(hDC,pt.x,pt.y) == bg) {
        drawPixelWindowColor(hDC,pt.x,pt.y,hDC->brush.color);
    // detect color change above
        if((pt.y+1) < MAX_HEIGHT) {
          if(up && GetPixel(hDC,pt.x,pt.y+1) == bg) {
            POINTS pt2;
            pt2.x=pt.x; pt2.y=pt.y+1;
            stack[stack_ptr++]=pt2;
            }
          up = GetPixel(hDC,pt.x,pt.y+1) != bg;
          }
    // detect color change below
        if(pt.y>0) {
          if(down && GetPixel(hDC,pt.x,pt.y-1) == bg) {
            POINTS pt2;
            pt2.x=pt.x; pt2.y=pt.y-1;
            stack[stack_ptr++]=pt2;
            }
          down = GetPixel(hDC,pt.x, pt.y-1) != bg;       
          }
        pt.x++;
        }
      if(stack_ptr>=MAX_FILL_STACK)
        return 0;
      }
    }

  return 1;
  }

BOOL BitBlt(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy,HDC hdcSrc,UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,DWORD rop) {
  // non è chiaro se anche questa può fare mirror...
  int i;
  
	if(hdcSrc->isMemoryDC) {
//    void *p=hdcSrc->mem;
    switch(_bpp) {
      case 8:
        {
        BYTE *p2=hdcSrc->mem;
        while(cy--) {
          while(cx--) {
            SetPixel(hDC,x++,y++,*p2++);
            }
          }
        i=1;
        }
        break;
      case 16:
        {
        WORD *p2=hdcSrc->mem;
        while(cy--) {
          while(cx--) {
            SetPixel(hDC,x++,y++,*p2++);
            }
          }
        }
        i=1;
        break;
      default:
        i=0;
        break;
      }
		}
	else {
    while(cy--) {
      while(cx--) {
        SetPixel(hDC,x++,y++,GetPixel(hdcSrc,x1,y1));
        }
      }
    i=1;
		}
  //DrawBitmap ecc
  return i;
  }
BOOL StretchBlt(HDC hdcDest,UGRAPH_COORD_T xDest,UGRAPH_COORD_T yDest,GRAPH_COORD_T wDest,GRAPH_COORD_T hDest,
  HDC hdcSrc,UGRAPH_COORD_T xSrc,UGRAPH_COORD_T ySrc,GRAPH_COORD_T wSrc,GRAPH_COORD_T hSrc,DWORD rop) {
  int xr,yr,xr2,yr2;
  GFX_COLOR c;
  int i;
  enum TERNARY_OP oldRop=hdcDest->rop;
  
  xr=1; yr=1;   //finire!
  xr2=1; yr2=1;   //finire!
  
  SetColors(WHITE,BLACK,hdcDest->rop);

	if(hdcSrc->isMemoryDC) {
//    void *p=hdcSrc->mem;
    switch(_bpp) {
      case 8:
        {
        BYTE *p2=hdcSrc->mem;
        while(hDest) {
          while(wDest) {
            drawRectangleWindowColor(hdcDest,xDest,yDest,xDest+xr2,yDest+yr2,*p2++);
            xSrc+=1;
            wSrc-=1;
            xDest+=xr2;
            wDest-=xr2;
            }
          ySrc+=1;
          hSrc-=1;
          yDest+=yr2;
          hDest-=yr2;
          }
        }
        i=1;
        break;
      case 16:
        {
        WORD *p2=hdcSrc->mem;
        while(hDest) {
          while(wDest) {
            drawRectangleWindowColor(hdcDest,xDest,yDest,xDest+xr2,yDest+yr2,*p2++);
            xSrc+=1;
            wSrc-=1;
            xDest+=xr2;
            wDest-=xr2;
            }
          ySrc+=1;
          hSrc-=1;
          yDest+=yr2;
          hDest-=yr2;
          }
        }
        i=1;
        break;
      default:
        i=0;
        break;
      }
		}
	else {
    while(hDest) {
      while(wDest) {
        c=GetPixel(hdcSrc,xSrc,ySrc);
        drawRectangleWindowColor(hdcDest,xDest,yDest,xDest+xr2,yDest+yr2,c);
        xSrc+=1;
        wSrc-=1;
        xDest+=xr2;
        wDest-=xr2;
        }
      ySrc+=1;
      hSrc-=1;
      yDest+=yr2;
      hDest-=yr2;
      }
    i=1;
		}
  //DrawBitmap ecc
  SetColors(WHITE,BLACK,oldRop);
  return i;
  }

int StretchDIBits(HDC hDC,UGRAPH_COORD_T xDest,UGRAPH_COORD_T yDest,GRAPH_COORD_T DestWidth,GRAPH_COORD_T DestHeight,
  UGRAPH_COORD_T xSrc,UGRAPH_COORD_T ySrc,GRAPH_COORD_T SrcWidth,GRAPH_COORD_T SrcHeight,const void *lpBits,
  const GFX_COLOR *lpbmi,BYTE iUsage,enum TERNARY_OP rop) {
  int xr,yr,xr2,yr2;
  int i;
  GFX_COLOR c;
  
  xr=1; yr=1;   //finire!
  xr2=1; yr2=1;   //finire!
  
  switch(_bpp) {
    case 8:
      {
      while(DestHeight) {
        while(DestWidth) {
          c=*lpbmi++;
          drawRectangleWindowColor(hDC,xDest,yDest,xDest+xr2,yDest+yr2,Color565To332(c));
          xSrc+=1;
          SrcWidth-=1;
          xDest+=xr2;
          DestWidth-=xr2;
          }
        ySrc+=1;
        SrcHeight-=1;
        yDest+=yr2;
        DestHeight-=yr2;
        }
      i=1;
      }
      break;
    case 16:
      {
      while(DestHeight) {
        while(DestWidth) {
          c=*lpbmi++;
          drawRectangleWindowColor(hDC,xDest,yDest,xDest+xr2,yDest+yr2,c);
          xSrc+=1;
          SrcWidth-=1;
          xDest+=xr2;
          DestWidth-=xr2;
          }
        ySrc+=1;
        SrcHeight-=1;
        yDest+=yr2;
        DestHeight-=yr2;
        }
      i=1;
      }
      break;
    default:
      i=0;
      break;
    }
  return i;
  }

GFX_COLOR *CopyImage(GFX_COLOR *h,BYTE type,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy,UINT flags) {
  int xr,yr,xr2,yr2;
  GFX_COLOR c,*pDest;
  
  xr=1; yr=1;   //finire!
  xr2=1; yr2=1;   //finire!
  
  while(cy--) {
    while(cx--) {
      c=*h;
      *pDest=c;
      
      h += xr2;
      
      }
    }
  }

BYTE SetStretchBltMode(HDC hDC,BYTE mode) {
  uint8_t i=hDC->rop;
  hDC->rop = mode;  // in effetti sarebbero un'altra cosa, ma per ora ok!
  return i;
  }
BOOL GetBitmapDimensionEx(BITMAP *hbit,S_SIZE *lpsize) {
  lpsize->cx=hbit->bmWidth;    // vabbe' :)
  lpsize->cy=hbit->bmHeight;    // vabbe' :)
  }
long GetBitmapBits(BITMAP  *hbit,long cb,void *lpvBits) {
  return 0;
  }

enum TERNARY_OP GetROP2(HDC hDC) {
  return hDC->rop;
  }
enum TERNARY_OP SetROP2(HDC hDC,enum TERNARY_OP rop2) {
  uint8_t i=hDC->rop;
  hDC->rop = rop2;
  return i;
  }

PEN CreatePen(BYTE iStyle,BYTE cWidth,GFX_COLOR color) {
  PEN myPen;
  
  myPen.style=iStyle;
  myPen.size=cWidth;
  myPen.color=color;
  return myPen;
	}
  
PEN ExtCreatePen(BYTE iStyle,BYTE cWidth,const BRUSH *brush,BYTE cStyle,const BYTE *pStyle) {
  PEN myPen;
  
  myPen.style=iStyle;
//  myPen.size=cWidth;
//  myPen.color=color;
  //FINIRE!
  return myPen;
	}
  
BRUSH CreateBrushIndirect(const LOGBRUSH *plBrush) {
  BRUSH myBrush;
  
  myBrush.style=plBrush->lbStyle;
  myBrush.size=1 /*plBrush->size boh... fare*/;
  myBrush.color=plBrush->lbColor;
  return myBrush;
	}

BRUSH CreateSolidBrush(GFX_COLOR color) {
  BRUSH myBrush;
  
  myBrush.style=BS_SOLID;
  myBrush.size=1;
  myBrush.color=color;
  return myBrush;
	}

BRUSH CreateHatchBrush(BYTE iHatch,GFX_COLOR color) {
  BRUSH myBrush;
  
  myBrush.style=iHatch;
  myBrush.size=1;
  myBrush.color=color;
  return myBrush;
	}

BRUSH CreatePatternBrush(BITMAP *hbm) {
  BRUSH myBrush;
  
  myBrush.style=BS_PATTERN;   //FARE!
  myBrush.size=1;
  myBrush.color=0;
  return myBrush;
	}

BRUSH GetSysColorBrush(int nIndex) {
  
  return CreateSolidBrush(GetSysColor(nIndex));
  }


GDIOBJ GetStockObject(int i) {
  GDIOBJ g;
  
  switch(i) {
    case WHITE_BRUSH:
      g.brush=CreateSolidBrush(WHITE);
      return g;
      break;
    case LTGRAY_BRUSH:
      g.brush=CreateSolidBrush(LIGHTGRAY);
      return g;
      break;
    case GRAY_BRUSH:
      g.brush=CreateSolidBrush(GRAY192);
      return g;
      break;
    case DKGRAY_BRUSH:
      g.brush=CreateSolidBrush(DARKGRAY);
      return g;
      break;
    case BLACK_BRUSH:
      g.brush=CreateSolidBrush(BLACK);
      return g;
      break;
    case NULL_BRUSH:
//    case HOLLOW_BRUSH:
      g.brush=CreateSolidBrush(0);
      g.brush.style=0;
      return g;
      break;
    case WHITE_PEN:
      g.pen=CreatePen(PS_SOLID,1,WHITE);
      return g;
      break;
    case BLACK_PEN:
      g.pen=CreatePen(PS_SOLID,1,BLACK);
      return g;
      break;
    case NULL_PEN:
      g.pen=CreatePen(PS_SOLID,0,0);
      return g;
      break;
    case DC_BRUSH:
      g.brush=CreateSolidBrush(BLACK);    // CONTRARIO DI WINDOWS :)
      return g;
      break;
    case DC_PEN:
      g.pen=CreatePen(PS_SOLID,1,WHITE);    // contrario di windows !
      return g;
      break;
    case SYSTEM_FIXED_FONT:   // qui uso il mini 3x5 :)
      g.font=CreateFont(6,4,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case SYSTEM_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case OEM_FIXED_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case ANSI_FIXED_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case ANSI_VAR_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case DEVICE_DEFAULT_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case DEFAULT_GUI_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case DEFAULT_PALETTE:
      break;
    }
  }

CURSOR LoadCursor(HINSTANCE hInstance,const char *lpCursorName) {
  
  switch((DWORD)lpCursorName) {
    case IDC_APPSTARTING:
      return hourglassCursorSm;
      break;
    case IDC_ARROW:
      return standardCursorSm;
      break;
    case IDC_CROSS:
      return crossCursorSm;
      break;
    case IDC_HAND:
      return handCursorSm;
      break;
    case IDC_HELP:
      break;
    case IDC_IBEAM:
      return halfSquareCursorSm;
      break;
    case IDC_NO:
      break;
    case IDC_SIZEALL:
      return crossCursorSm;
      break;
    case IDC_SIZENESW:
      return crossCursorSm;   // finire
      break;
    case IDC_SIZENS:
      return crossCursorSm;   // finire
      break;
    case IDC_SIZENWSE:
      return crossCursorSm;   // finire
      break;
    case IDC_SIZEWE:
      return crossCursorSm;   // finire
      break;
    case IDC_UPARROW:
      break;
    case IDC_WAIT:
      return hourglassCursorSm;
      break;
    }
  }
ICON LoadIcon(HINSTANCE hInstance,const char *lpIconName) {
  
  switch((DWORD)lpIconName) {
    case IDI_APPLICATION:
      return standardIcon;
      break;
//    case IDI_INFORMATION:
    case IDI_ASTERISK:
      break;
//    case IDI_WARNING:
    case IDI_EXCLAMATION:
      break;
//    case IDI_ERROR:
    case IDI_HAND:
      return handCursorSm;
      break;
    case IDI_QUESTION:
      break;
    case IDI_WINLOGO:
      break;
    case IDI_SHIELD:
			return shieldIcon;
      break;
    }
  }
HANDLE LoadImage(HINSTANCE hInst,const char *name,BYTE type,UGRAPH_COORD_T cx,UGRAPH_COORD_T cy,BYTE fuLoad) {
  }
uint16_t LoadString(HINSTANCE hInstance,UINT uID,char *lpBuffer,uint16_t cchBufferMax) {
  }
HMENU LoadMenu(HINSTANCE hInstance,const char *lpMenuName) {
  }


FONT CreateFont(BYTE cHeight,BYTE cWidth,BYTE cEscapement,BYTE cOrientation,WORD cWeight,
  BYTE bItalic,BYTE bUnderline,BYTE bStrikeOut,enum FONTCHARSET iCharSet,enum FONTPRECISION iOutPrecision,
  enum FONTCLIPPRECISION iClipPrecision, WORD iQuality, enum FONTPITCHANDFAMILY iPitchAndFamily,const char *pszFaceName) {
  FONT myFont;
#define FONT24_TIMES 28
#define FONT18_TIMES 21
#define FONT12_TIMES 14
#define FONT9_TIMES 10
#define FONT24_ARIAL 28
#define FONT18_ARIAL 21
#define FONT12_ARIAL 14
#define FONT9_ARIAL 10
  switch(iPitchAndFamily & 0xf0) {
    case FF_ROMAN:
      if(iQuality == DEFAULT_QUALITY || iQuality == DRAFT_QUALITY) {
        myFont.font= (void*) (cHeight>FONT18_TIMES ? FreeSerif24pt7b :    // v. nota font
          (cHeight>FONT12_TIMES ? FreeSerif18pt7b : (cHeight>FONT9_TIMES ? FreeSerif12pt7b : FreeSerif9pt7b)));
        }
      else {    // PROOF, tanto per..
        myFont.font= (void*) (cHeight>(FONT18_TIMES*1.1) ? FreeSerif24pt7b :    // v. nota font
          (cHeight>(FONT12_TIMES*1.1) ? FreeSerif18pt7b : (cHeight>(FONT9_TIMES*1.1) ? FreeSerif12pt7b : FreeSerif9pt7b)));
        }
      break;
    case FF_SWISS:
      if(iQuality == DEFAULT_QUALITY || iQuality == DRAFT_QUALITY) {
        myFont.font= (void*) (cHeight>FONT18_ARIAL ? FreeSans24pt7b : 
          (cHeight>FONT12_ARIAL ? FreeSans18pt7b : (cHeight>FONT9_ARIAL ? FreeSans12pt7b : FreeSans9pt7b)));
        }
      else {    // PROOF, tanto per..
        myFont.font= (void*) (cHeight>(FONT18_ARIAL*1.1) ? FreeSans24pt7b : 
          (cHeight>(FONT12_ARIAL*1.1) ? FreeSans18pt7b : (cHeight>(FONT9_ARIAL*1.1) ? FreeSans12pt7b : FreeSans9pt7b)));
        }
      break;
    case FF_DONTCARE:
    default:
      if(!pszFaceName) {
        myFont.font=NULL;
        myFont.base16=cHeight; myFont.multiplier16=1;
        }
      else if(!stricmp(pszFaceName,"system") || !stricmp(pszFaceName,"modern")) {
        myFont.font=NULL;
        myFont.base16=cHeight; myFont.multiplier16=1;
        }
      else if(!stricmp(pszFaceName,"times")) {
        if(iQuality == DEFAULT_QUALITY || iQuality == DRAFT_QUALITY) {
          myFont.font= (void*) (cHeight>FONT18_TIMES ? FreeSerif24pt7b :    // v. nota font
            (cHeight>FONT12_TIMES ? FreeSerif18pt7b : (cHeight>FONT9_TIMES ? FreeSerif12pt7b : FreeSerif9pt7b)));
          }
        else {    // PROOF, tanto per..
          myFont.font= (void*) (cHeight>(FONT18_TIMES*1.1) ? FreeSerif24pt7b :    // v. nota font
            (cHeight>(FONT12_TIMES*1.1) ? FreeSerif18pt7b : (cHeight>(FONT9_TIMES*1.1) ? FreeSerif12pt7b : FreeSerif9pt7b)));
          }
        }
      else if(!stricmp(pszFaceName,"arial")) {
        if(iQuality == DEFAULT_QUALITY || iQuality == DRAFT_QUALITY) {
          myFont.font= (void*) (cHeight>FONT18_ARIAL ? FreeSans24pt7b : 
            (cHeight>FONT12_ARIAL ? FreeSans18pt7b : (cHeight>FONT9_ARIAL ? FreeSans12pt7b : FreeSans9pt7b)));
          }
        else {    // PROOF, tanto per..
          myFont.font= (void*) (cHeight>(FONT18_ARIAL*1.1) ? FreeSans24pt7b : 
            (cHeight>(FONT12_ARIAL*1.1) ? FreeSans18pt7b : (cHeight>(FONT9_ARIAL*1.1) ? FreeSans12pt7b : FreeSans9pt7b)));
          }
        }
      break;
    }
  
  if(myFont.font) {
    DWORD i=(iQuality == DEFAULT_QUALITY || iQuality == DRAFT_QUALITY) ?
      myFont.font->glyph['G'-myFont.font->first].height : myFont.font->glyph['@'-myFont.font->first].height;   // carattere medio completo, poi gioco un po' con la qualità;
    BYTE n=cHeight;     // una via di mezzo
    myFont.multiplier4=1;
    while((n*2)/3>i && myFont.multiplier4<3) {
      myFont.multiplier4++;
      n = cHeight / myFont.multiplier4;
      }
    myFont.base64=n;
    }
  else {
    myFont.base16=cHeight;
    BYTE n=cHeight;     // una via di mezzo
    myFont.multiplier16=1;
    while((n*2)/3>8 && myFont.multiplier4<15) {
      myFont.multiplier16++;
      n = cHeight / myFont.multiplier16;
      }
    myFont.base16=n;
    }

  myFont.inclination=0; 
  myFont.weight=cWeight/10;   // faccio così, risparmio :)
  myFont.attributes=0;
  myFont.italic=bItalic;
  myFont.bold= myFont.weight > FW_NORMAL/10;
  myFont.strikethrough=bStrikeOut;
  myFont.underline=bUnderline;
  return myFont;
	}

static BOOL getFontInfo(const GFXfont *font,LOGFONT *lf,TEXTMETRIC *lptm) {
  
  if(font==NULL) {    // andrebbe gestito anche il piccolo...
    strcpy(lf->lfFaceName,"System");
    lf->lfCharSet=DEFAULT_CHARSET;   // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-logfonta
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=8;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=FIXED_PITCH | FF_DONTCARE;
    lf->lfQuality=DRAFT_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=6;
    lptm->tmAscent=(lf->lfHeight*3)/4;   // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-textmetrica
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=13;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=0;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=255;
    lptm->tmMaxCharWidth=lf->lfWidth;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=0 /*!TMPF_FIXED_PITCH*/ | FF_DONTCARE;
    return TRUE;
    }
  // finire ascent, descent, maxcharwidth
  else if(font==FreeSans9pt7b) {
    strcpy(lf->lfFaceName,"Arial");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=9;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_SWISS;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=7;
    lptm->tmAscent=(lf->lfHeight*3)/4;
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=(lf->lfWidth*3)/2;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_VARIABLE_PITCH | FF_SWISS;
    return TRUE;
    }
  else if(font==FreeSans12pt7b) {
    strcpy(lf->lfFaceName,"Arial");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=12;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_SWISS;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=9;
    lptm->tmAscent=(lf->lfHeight*3)/4;
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=(lf->lfWidth*3)/2;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_VARIABLE_PITCH | FF_SWISS;
    return TRUE;
    }
  else if(font==FreeSans18pt7b) {
    strcpy(lf->lfFaceName,"Arial");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=18;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_SWISS;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=13;
    lptm->tmAscent=(lf->lfHeight*3)/4;
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=(lf->lfWidth*3)/2;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_VARIABLE_PITCH | FF_SWISS;
    return TRUE;
    }
  else if(font==FreeSans24pt7b) {
    strcpy(lf->lfFaceName,"Arial");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=24;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_SWISS;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=18;
    lptm->tmAscent=(lf->lfHeight*3)/4;
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=(lf->lfWidth*3)/2;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_VARIABLE_PITCH | FF_SWISS;
    return TRUE;
    }
  else if(font==FreeSerif9pt7b) {
    strcpy(lf->lfFaceName,"Times");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=9;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=7;
    lptm->tmAscent=(lf->lfHeight*3)/4;
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=(lf->lfWidth*3)/2;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_VARIABLE_PITCH | FF_ROMAN;
    return TRUE;
    }
  else if(font==FreeSerif12pt7b) {
    strcpy(lf->lfFaceName,"Times");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=12;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=9;
    lptm->tmAscent=(lf->lfHeight*3)/4;
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=(lf->lfWidth*3)/2;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_VARIABLE_PITCH | FF_ROMAN;
    return TRUE;
    }
  else if(font==FreeSerif18pt7b) {
    strcpy(lf->lfFaceName,"Times");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=18;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=13;
    lptm->tmAscent=(lf->lfHeight*3)/4;
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=(lf->lfWidth*3)/2;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_VARIABLE_PITCH | FF_ROMAN;
    return TRUE;
    }
  else if(font==FreeSerif24pt7b) {
    strcpy(lf->lfFaceName,"Times");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=24;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=18;
    lptm->tmAscent=(lf->lfHeight*3)/4;
    lptm->tmAveCharWidth=lf->lfWidth;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=(lf->lfHeight*1)/4;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=lf->lfHeight;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=(lf->lfWidth*3)/2;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_VARIABLE_PITCH | FF_ROMAN;
    return TRUE;
    }
  return FALSE;
  }
  
BOOL GetTextMetrics(HDC hDC,TEXTMETRIC *lptm) {
  LOGFONT lf;
  
  return getFontInfo(hDC->font.font,&lf,lptm);
	}
  
int EnumFonts(HDC hDC,const char *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam) {
  LOGFONT lf;
  TEXTMETRIC tm;
  BYTE i;

  if(lpLogfont) {
    if(!stricmp(lpLogfont,"system") || !stricmp(lpLogfont,"modern")) {
      getFontInfo(NULL,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"arial")) {
      for(i=0; i<sizeof(fontsHelv)/sizeof(GFXfont**); i++) {
        getFontInfo(fontsHelv[i],&lf,&tm);
        lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
        }
      }
    else if(!stricmp(lpLogfont,"times")) {
      for(i=0; i<sizeof(fontsTimes)/sizeof(GFXfont**); i++) {
        getFontInfo(fontsTimes[i],&lf,&tm);
        lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
        }
      }
    }
  else {
    getFontInfo(NULL,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    for(i=0; i<sizeof(fontsHelv)/sizeof(GFXfont**); i++) {
      getFontInfo(fontsHelv[i],&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }

    for(i=0; i<sizeof(fontsTimes)/sizeof(GFXfont**); i++) {
      getFontInfo(fontsTimes[i],&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
  	}
	}

int EnumFontFamilies(HDC hDC,const char *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam) {
  LOGFONT lf;
  TEXTMETRIC tm;
  BYTE i;

  if(lpLogfont) {
    if(!stricmp(lpLogfont,"system") || !stricmp(lpLogfont,"modern")) {
      getFontInfo(NULL,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"arial")) {
      for(i=0; i<sizeof(fontsHelv)/sizeof(GFXfont**); i++) {
        getFontInfo(fontsHelv[i],&lf,&tm);
        lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
        }
      }
    else if(!stricmp(lpLogfont,"times")) {
      for(i=0; i<sizeof(fontsTimes)/sizeof(GFXfont**); i++) {
        getFontInfo(fontsTimes[i],&lf,&tm);
        lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
        }
      }
    }
  else {
    getFontInfo(NULL,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    for(i=0; i<sizeof(fontsHelv)/sizeof(GFXfont**); i++) {
      getFontInfo(fontsHelv[i],&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }

    for(i=0; i<sizeof(fontsTimes)/sizeof(GFXfont**); i++) {
      getFontInfo(fontsTimes[i],&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
  	}
	}

int EnumFontFamiliesEx(HDC hDC,const char /*LOGFONT*/ *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam,DWORD dwFlags) {
  LOGFONT lf;
  TEXTMETRIC tm;
  BYTE i;
// in teoria questa usa le strutture simili ma "EX" ... 
  if(lpLogfont) {
    if(!stricmp(lpLogfont,"system") || !stricmp(lpLogfont,"modern")) {
      getFontInfo(NULL,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"arial")) {
      for(i=0; i<sizeof(fontsHelv)/sizeof(GFXfont**); i++) {
        getFontInfo(fontsHelv[i],&lf,&tm);
        lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
        }
      }
    else if(!stricmp(lpLogfont,"times")) {
      for(i=0; i<sizeof(fontsTimes)/sizeof(GFXfont**); i++) {
        getFontInfo(fontsTimes[i],&lf,&tm);
        lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
        }
      }
    }
  else {
    getFontInfo(NULL,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    for(i=0; i<sizeof(fontsHelv)/sizeof(GFXfont**); i++) {
      getFontInfo(fontsHelv[i],&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }

    for(i=0; i<sizeof(fontsTimes)/sizeof(GFXfont**); i++) {
      getFontInfo(fontsTimes[i],&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    }
	}

int GetTextFace(HDC hDC,WORD c,char *lpName) {
  LOGFONT lf;
  TEXTMETRIC tm;
  int i;
  
  i=getFontInfo(hDC->font.font,&lf,&tm);
  strncpy(lpName,lf.lfFaceName,c);
  return i;
  }

int SetBkMode(HDC hDC,int mode) {
  // fare...
  }

GDIOBJ SelectObject(HDC hDC,BYTE type,GDIOBJ g) {
  GDIOBJ oldSel;
  
	switch(type) {
		case OBJ_PEN:
      oldSel=(GDIOBJ)hDC->pen;
			hDC->pen=g.pen;
			break;
		case OBJ_BRUSH:
      oldSel=(GDIOBJ)hDC->brush;
			hDC->brush=g.brush;
			break;
		case OBJ_FONT:
      oldSel=(GDIOBJ)hDC->font;
			hDC->font=g.font;
			break;
		case OBJ_ICON:
//			hDC->icon=g.icon;
			break;
		case OBJ_BITMAP:
//			hDC->bitmap=g.bitmap;
			break;
		case OBJ_REGION:
//			hDC-> =g.region;
			break;
		}

	return oldSel;
	}

BOOL DeleteObject(BYTE type,GDIOBJ g) {
  // qua nulla :)
	switch(type) {
		case OBJ_PEN:
			return TRUE;
			break;
		case OBJ_BRUSH:
			return TRUE;
			break;
		case OBJ_FONT:
			return TRUE;
			break;
		case OBJ_ICON:
    	return TRUE;
			break;
		case OBJ_BITMAP:
    	return TRUE;
			break;
		case OBJ_REGION:
    	return TRUE;
			break;
		}
	return FALSE;
	}
  
GFX_COLOR SetTextColor(HDC hDC,GFX_COLOR color) {
  GFX_COLOR c=hDC->foreColor;
  
  hDC->foreColor=color;
  return c;
  }

GFX_COLOR GetTextColor(HDC hDC) {
  
  return hDC->foreColor;
  }

GFX_COLOR SetBkColor(HDC hDC,GFX_COLOR color) {
  GFX_COLOR c=hDC->backColor;
  
  hDC->backColor=color;
  return c;
  }

GFX_COLOR GetBkColor(HDC hDC) {
  
  return hDC->backColor;
  }

int GetBkMode(HDC hDC) {
  
  return hDC->brush.style==BS_HOLLOW ? 0 /*TRANSPARENT*/ : 1 /*OPAQUE*/;    // bah circa.. e NON si trovano le #efine!
  }

int SetTextCharacterExtra(HDC hDC,int extra) {
  }

BYTE SetTextAlign(HDC hDC,BYTE align) {
  BYTE n=hDC->textAlignment;
  
  hDC->textAlignment=align;
  return n;
  }

BYTE GetTextAlign(HDC hDC) {
  
  return hDC->textAlignment;
  }

BOOL GetCharWidth(HDC hDC,uint16_t iFirst,uint16_t iLast,uint16_t *lpBuffer) {
  BYTE i;
  
  for(i=0; iFirst<=iLast; iFirst++,i++) {
    SIZE sz;
    char buffer[4];
    buffer[0]=iFirst;   // sarebbero UNICODE, immagino..
    GetTextExtentPoint(hDC,buffer,1,&sz);
    lpBuffer[i] = sz.cx;
    }
  // anche se il doc dice che questa non si usa con truetype...
  return TRUE;
  }

BOOL GetCharABCWidths(HDC hDC,uint16_t wFirst,uint16_t wLast,uint16_t *lpBuffer /*ABC *lpABC*/) { // qua così!
  BYTE i;
  
  for(i=0; wFirst<=wLast; wFirst++,i++) {
    SIZE sz;
    char buffer[4];
    buffer[0]=wFirst;   // sarebbero UNICODE, immagino..
    GetTextExtentPoint(hDC,buffer,1,&sz);
    lpBuffer[i] = sz.cx;
    
// usare  glyph->xAdvance * (GRAPH_COORD_T)hDC->font.size;
    // con ABC, poi...

    }
  // anche se il doc dice che questa SI usa con truetype...
  return TRUE;
  }

BOOL GetTextExtentPoint(HDC hDC,const char *lpString,WORD c,SIZE *psize) {
  BYTE foundCR=0;
  int x=0;
  
  psize->cx=0;
  psize->cy=0;
  
  while(c--) {
    if(*lpString == '\n') {
      psize->cx=max(x,psize->cx);
      x=0;
      foundCR++;
      }
    else {
      if(!hDC->font.font) {
        if(*lpString >= ' ' && *lpString <= '\x7f') {
          x+=(GRAPH_COORD_T)getFontWidth(&hDC->font);
          psize->cy=max(psize->cy,(GRAPH_COORD_T)getFontHeight(&hDC->font));   // :)
          }
        }
      else {
        const GFXfont *gfxFont=hDC->font.font;
        UINT8 first = gfxFont->first;
        if(*lpString >= first && *lpString <= gfxFont->last) {
          UINT8 c2 = *lpString - gfxFont->first;
          const GFXglyph *glyph = &gfxFont->glyph[c2];
          x += (GRAPH_COORD_T)glyph->width /*xAdvance*/ * (GRAPH_COORD_T)hDC->font.multiplier4;
          psize->cy=max(psize->cy,(GRAPH_COORD_T)hDC->font.multiplier4 * (GRAPH_COORD_T)glyph->height /*gfxFont->yAdvance*/); 
          }
        }
      }
    lpString++;
    }
  
  psize->cx=max(x,psize->cx);
  psize->cy *= (foundCR+1);   // alla buona..
  
  return TRUE;
  }
  
BOOL GetTextExtentPoint32(HDC hDC,const char *lpString,WORD n,SIZE *psizl) {
  TEXTMETRIC tm;
  // l'avevo pensata così ma boh... intanto uso GetTextExtentPoint
  GetTextMetrics(hDC,&tm);
  
  psizl->cx= psizl->cy= 0;
  while(n--) {
    psizl->cx += tm.tmAveCharWidth /* *lpString*/;
    psizl->cy = max(psizl->cy,tm.tmHeight);
    lpString++;
    }
// v. anche void getTextBounds(char *str, UGRAPH_COORD_T x, UGRAPH_COORD_T y, UGRAPH_COORD_T *x1, UGRAPH_COORD_T *y1, UGRAPH_COORD_T *w, UGRAPH_COORD_T *h) {

  return TRUE;
  }

BOOL GetTextExtentExPoint(HDC hDC,const char *lpszString,int cchString,
  int nMaxExtent,int *lpnFit,int *lpnDx,SIZE *lpSize) {
  }

int GetTextCharacterExtra(HDC hDC) {
  }

void drawCharWindow(HDC hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y, unsigned char c) {
	INT8 i,j;
	const BYTE *fontPtr;

  if(!hDC->font.font) { // 'Classic' built-in font
  	INT8 i2,j2;

//boh??    if(!_cp437 && (c >= 176)) 
//			c++; // Handle 'classic' charset behavior
    if(hDC->font.base16<8) {
      fontPtr=font3x5+((uint16_t)c)*4;
      }
    else {
      fontPtr=font5x7+((uint16_t)c)*6;
      }
    i2=getFontWidth(&hDC->font); 
    j2=getFontHeight(&hDC->font);
    for(i=0; i<i2; i++) {
      UINT8 line;
      UGRAPH_COORD_T xpos,ypos;
      BYTE doItalic;

      line = *(fontPtr+i);
      xpos=x+(i*hDC->font.multiplier16);
      for(j=0; j<j2; j++, line >>= 1) {
        ypos=y+(j*hDC->font.multiplier16);
        if(hDC->font.italic && j<=(j2/2))
          doItalic=1;
        else
          doItalic=0;
        if(hDC->font.underline && j==(j2-1)) {
          if(hDC->font.multiplier16 == 1) 
            drawPixelWindowColor(hDC,xpos+doItalic,ypos,hDC->foreColor);
          else
            fillRectangleWindowColor(hDC,xpos+doItalic,ypos,xpos+doItalic+hDC->font.multiplier16-1,
                    ypos+hDC->font.multiplier16-1,hDC->foreColor);
          }
        else if(hDC->font.strikethrough && j==(j2/2)) {
          if(hDC->font.multiplier16 == 1) 
            drawPixelWindowColor(hDC,xpos+doItalic,ypos,hDC->foreColor);
          else
            fillRectangleWindowColor(hDC,xpos+doItalic,ypos,xpos+doItalic+hDC->font.multiplier16-1,
                    ypos+hDC->font.multiplier16-1,hDC->foreColor);
          }
        else if(line & 0x1) {
          if(hDC->font.multiplier16 == 1) {
            drawPixelWindowColor(hDC,xpos+doItalic,ypos,hDC->foreColor);
            if(hDC->font.bold)
              drawPixelWindowColor(hDC,xpos+doItalic+1,ypos,hDC->foreColor);
            }
          else {
            fillRectangleWindowColor(hDC,xpos+doItalic,ypos,xpos+doItalic+hDC->font.multiplier16-1,
                    ypos+hDC->font.multiplier16-1,hDC->foreColor);
            if(hDC->font.bold)
              fillRectangleWindowColor(hDC,xpos+doItalic+1,ypos,xpos+doItalic+1+hDC->font.multiplier16-1,
                      ypos+hDC->font.multiplier16-1,hDC->foreColor);
            }
          } 
        else if(/*hDC->backColor != hDC->foreColor && */    (!hDC->font.bold ) ) {
// GESTIRE SetBkMode
          if(hDC->font.multiplier16 == 1) 
            drawPixelWindowColor(hDC,xpos+doItalic,ypos,hDC->backColor);
          else          
            fillRectangleWindowColor(hDC,xpos+doItalic,ypos,xpos+doItalic+hDC->font.multiplier16-1,
                    ypos+hDC->font.multiplier16-1,hDC->backColor);
          }
        }
      }

    } 
  else {
    const GFXglyph *glyph;
    const UINT8  *bitmap;
    UINT16 bo;
    UINT8  w,h, xa;
    INT8   xo, yo;
    UINT8  xx, yy, bits, bit;
    GRAPH_COORD_T  xo16, yo16;
    BYTE doItalic;

    const GFXfont *gfxFont=(GFXfont*)hDC->font.font;

	  if(c<gfxFont->first || c>gfxFont->last)
	    return;

    c -= gfxFont->first;
    glyph  = &gfxFont->glyph[c];
    bitmap = gfxFont->bitmap;

    bo = glyph->bitmapOffset;
    w  = glyph->width; h = glyph->height; xa = glyph->xAdvance;
    xo = glyph->xOffset; yo = gfxFont->yAdvance/2+glyph->yOffset /*glyph->yOffset*/ /*-glyph->yOffset-h*/;
    bit = 0;
    
    if(hDC->font.multiplier4 > 1) {
      xo16 = xo;
      yo16 = yo;
      }

    // Todo: Add character clipping here

    // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
    // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
    // has typically been used with the 'classic' font to overwrite old
    // screen contents with new data.  This ONLY works because the
    // characters are a uniform size; it's not a sensible thing to do with
    // proportionally-spaced fonts with glyphs of varying sizes (and that
    // may overlap).  To replace previously-drawn text when using a custom
    // font, use the getTextBounds() function to determine the smallest
    // rectangle encompassing a string, erase the area with fillRect(),
    // then draw new text.  This WILL unfortunately 'blink' the text, but
    // is unavoidable.  Drawing 'background' pixels will NOT fix this,
    // only creates a new set of problems.  Have an idea to work around
    // this (a canvas object type for MCUs that can afford the RAM and
    // displays supporting setAddrWindow() and pushColors()), but haven't
    // implemented this yet.

    for(yy=0; yy<h; yy++) {
      if(hDC->font.italic && yy<((h/2)))
        doItalic=1;
      else
        doItalic=0;
      for(xx=0; xx<w; xx++) {
        
        if(!(bit++ & 7)) {
          bits = bitmap[bo++];
          }
        
        if(hDC->font.underline && yy==(h-1)) {
          if(hDC->font.multiplier4 == 1) 
            drawPixelWindowColor(hDC,x+xo+xx+doItalic,y+yo+yy,hDC->foreColor);
          else
            fillRectangleWindowColor(hDC,x+(xo16+xx)*hDC->font.multiplier4, y+(yo16+yy)*hDC->font.multiplier4,
              x+(xo16+xx)*hDC->font.multiplier4+hDC->font.multiplier4-1, y+(yo16+yy)*hDC->font.multiplier4+hDC->font.multiplier4-1,hDC->foreColor);
          }
        else if(hDC->font.strikethrough && yy==((h/2))) {
          if(hDC->font.multiplier4 == 1) 
            drawPixelWindowColor(hDC,x+xo+xx+doItalic,y+yo+yy,hDC->foreColor);
          else
            fillRectangleWindowColor(hDC,x+(xo16+xx)*hDC->font.multiplier4+doItalic, y+(yo16+yy)*hDC->font.multiplier4, 
              x+(xo16+xx)*hDC->font.multiplier4+hDC->font.multiplier4+doItalic-1, y+(yo16+yy)*hDC->font.multiplier4+hDC->font.multiplier4-1,hDC->foreColor);
          }
        else {
          if(bits & 0x80) {
            if(hDC->font.multiplier4 == 1) {
              drawPixelWindowColor(hDC,x+xo+xx+doItalic,y+yo+yy,hDC->foreColor);
              if(hDC->font.bold)
                drawPixelWindowColor(hDC,x+xo+xx+doItalic+1,y+yo+yy,hDC->foreColor);
              } 
            else {
              fillRectangleWindowColor(hDC,x+(xo16+xx)*hDC->font.multiplier4+doItalic, y+(yo16+yy)*hDC->font.multiplier4, 
                x+(xo16+xx)*hDC->font.multiplier4+hDC->font.multiplier4+doItalic-1, y+(yo16+yy)*hDC->font.multiplier4+hDC->font.multiplier4-1,hDC->foreColor);
              if(hDC->font.bold)
                fillRectangleWindowColor(hDC,x+(xo16+xx)*hDC->font.multiplier4+doItalic+1, y+(yo16+yy)*hDC->font.multiplier4, 
                  x+(xo16+xx)*hDC->font.multiplier4+hDC->font.multiplier4+doItalic+1-1, y+(yo16+yy)*hDC->font.multiplier4+hDC->font.multiplier4-1,hDC->foreColor);
              }
            }
          if(/*hDC->backColor != hDC->foreColor && */    (!hDC->font.bold ) ) {
  // GESTIRE SetBkMode
            //qua no direi v. nota originale sopra
            }
          }
        
        bits <<= 1;
        }
      }

    } // End classic vs custom font
  
	}

inline HWND __attribute__((always_inline)) WindowFromDC(HDC hDC) {
  
  return hDC->hWnd;
  }

BOOL drawPixelWindow(HDC hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y) {
  HWND hWnd=WindowFromDC(hDC);
  
//  if(hDC->flags &  USARE per desktop ecc!)
//    return TRUE;
  
  x+=hDC->area.left;
  y+=hDC->area.top;
//    OffsetPoint(POINT *lppt,int16_t dx,int16_t dy);
  
	if(/*wndPtInRect*/ /*((x < hWnd->paintArea.left) || (y < hWnd->paintArea.top)) ||*/ // verifico lo mia paint area..
          // QUESTO SERVE CMQ poi con aree parziali!
          
    ((x > hDC->area.right) || (y > hDC->area.bottom)) || (((int)x < hDC->area.left) || ((int)y < hDC->area.top)) )
  	return FALSE;
  

  
  if(hWnd->parent /*hWnd->style & WS_CHILD*/) {  // se sono figlia, calcolo le mie coordinate reali
    HWND myWnd=hWnd->parent;
    while(myWnd) {
      x+=myWnd->clientArea.left;
      y+=myWnd->clientArea.top;
  //    OffsetPoint(RECT *lprc,int16_t dx,int16_t dy);
      myWnd=myWnd->parent;
      }
    }
  
  if(isPointVisible(hDC,x,y)) {
   	DrawPixel(x,y,hDC->pen.color);
    return TRUE;
    }
  else 
    return FALSE;
  }
// le lascio entrambe per velocità, dato che TUTTO passa di qua!
BOOL drawPixelWindowColor(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR c) {
  HWND hWnd=WindowFromDC(hDC);
  
//  if(hDC->flags &  USARE per desktop ecc!)
//    return TRUE;
  
//è uguale a sotto, if((((int)x < 0) || ((int)y < 0)) )
//    return FALSE;
  x+=hDC->area.left;
  y+=hDC->area.top;
//    OffsetPoint(POINT *lppt,int16_t dx,int16_t dy);
  
	if(/*wndPtInRect*/ /*((x < hWnd->paintArea.left) || (y < hWnd->paintArea.top)) || */ // verifico lo mia paint area..
          // QUESTO SERVE CMQ poi con aree parziali!
          
    ((x > hDC->area.right) || (y > hDC->area.bottom)) || (((int)x < hDC->area.left) || ((int)y < hDC->area.top)) )
  	return FALSE;
  
  if(hWnd->parent /*hWnd->style & WS_CHILD*/) {  // se sono figlia, calcolo le mie coordinate reali
    HWND myWnd=hWnd->parent;
    while(myWnd) {
      x+=myWnd->clientArea.left;
      y+=myWnd->clientArea.top;
  //    OffsetPoint(RECT *lprc,int16_t dx,int16_t dy);
      myWnd=myWnd->parent;
      }
    }
  
  if(isPointVisible(hDC,x,y)) {
   	DrawPixel(x,y,c);
    return TRUE;
    }
  else 
    return FALSE;
  }

void drawHorizLineWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y,UGRAPH_COORD_T x2) {

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y &= 0xffff;
  x2 &= 0xffff;
//  if(x1<x2)
  while(x1<=x2)
    drawPixelWindow(hDC,x1++,y);
  }

void drawHorizLineWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y,UGRAPH_COORD_T x2,GFX_COLOR c) {

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y &= 0xffff;
  x2 &= 0xffff;
//  if(x1<x2)
  while(x1<=x2)
    drawPixelWindowColor(hDC,x1++,y,c);
  }

void drawVertLineWindow(HDC hDC, UGRAPH_COORD_T x, UGRAPH_COORD_T y1,UGRAPH_COORD_T y2) {
  
  x &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  y2 &= 0xffff;
//  if(y1<y2)
  while(y1<=y2)
    drawPixelWindow(hDC,x,y1++);
  }

void drawVertLineWindowColor(HDC hDC, UGRAPH_COORD_T x, UGRAPH_COORD_T y1,UGRAPH_COORD_T y2,GFX_COLOR c) {
  
  x &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  y2 &= 0xffff;
//  if(y1<y2)
  while(y1<=y2)
    drawPixelWindowColor(hDC,x,y1++,c);
  }

void drawLineWindow(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {
 	BOOL steep;
	GRAPH_COORD_T dx,dy;
	GRAPH_COORD_T err;
	GRAPH_COORD_T ystep;
	GRAPH_COORD_T xbegin;

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  x2 &= 0xffff;
  y2 &= 0xffff;
  
  steep = abs(y2-y1) > abs(x2-x1);
  if(steep) {
    _swap(&x1, &y1);
    _swap(&x2, &y2);
    }
  if(x1>x2) {
    _swap(&x1, &x2);
    _swap(&y1, &y2);
    }

  dx = x2-x1;
  dy = abs(y2-y1);
  err = dx/2;
  ystep = y1<y2 ? 1 : -1;

  xbegin = x1;
  if(steep) {
    for(; x1<=x2; x1++) {
      err -= dy;
      if(err < 0) {
        INT16 len = x1-xbegin;
        if(len) {
          if(hDC->pen.size==1)
            drawVertLineWindow(hDC,y1,xbegin,xbegin+len);
          else {
            do {
              fillRectangleWindowColor(hDC,y1,xbegin,y1+hDC->pen.size-1,xbegin+hDC->pen.size-1,hDC->pen.color);
              xbegin++;
              } while(len--);
            }
          }
        else {
          if(hDC->pen.size==1)
            drawPixelWindow(hDC,y1,x1);
          else
            fillRectangleWindowColor(hDC,y1,x1,y1+hDC->pen.size-1,x1+hDC->pen.size-1,hDC->pen.color);
          }
        xbegin = x1+1;
        y1 += ystep;
        err += dx;
        }
      }
    if(x1 > xbegin) {
      if(hDC->pen.size==1)
        drawVertLineWindow(hDC,y1,xbegin,x1+1);
      else {
        while(xbegin<=x1) {
          fillRectangleWindowColor(hDC,y1,xbegin,y1+hDC->pen.size-1,xbegin+hDC->pen.size-1,hDC->pen.color);
          xbegin++;
          }
        }
      }
    } 
  else {
    for(; x1<=x2; x1++) {
      err -= dy;
      if(err < 0) {
        INT16 len = x1-xbegin;
        if(len) {
          if(hDC->pen.size==1)
            drawHorizLineWindow(hDC,xbegin,y1,xbegin+len);
          else {
            do {
              fillRectangleWindowColor(hDC,xbegin,y1,xbegin+hDC->pen.size-1,y1+hDC->pen.size-1,hDC->pen.color);
              xbegin++;
              } while(len--);
            }
          }
        else {
          if(hDC->pen.size==1)
            drawPixelWindow(hDC,x1,y1);
          else
            fillRectangleWindowColor(hDC,x1,y1,x1+hDC->pen.size-1,y1+hDC->pen.size-1,hDC->pen.color);
          }
        xbegin = x1+1;
        y1 += ystep;
        err += dx;
        }
      }
    if(x1 > xbegin) {
      if(hDC->pen.size==1)
        drawHorizLineWindow(hDC,xbegin,y1,x1);
      else {
        while(xbegin<=x1) {
          fillRectangleWindowColor(hDC,xbegin,y1,xbegin+hDC->pen.size-1,y1+hDC->pen.size-1,hDC->pen.color);
          xbegin++;
          }
        }
      }
    }
  }

void drawLineWindowColor(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c) {
 	BOOL steep;
	GRAPH_COORD_T dx,dy;
	GRAPH_COORD_T err;
	GRAPH_COORD_T ystep;
	GRAPH_COORD_T xbegin;

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  x2 &= 0xffff;
  y2 &= 0xffff;
  
  steep = abs(y2-y1) > abs(x2-x1);
  if(steep) {
    _swap(&x1, &y1);
    _swap(&x2, &y2);
    }
  if(x1>x2) {
    _swap(&x1, &x2);
    _swap(&y1, &y2);
    }

  dx = x2-x1;
  dy = abs(y2-y1);
  err = dx/2;
  ystep = y1<y2 ? 1 : -1;

  xbegin = x1;
  if(steep) {
    for(; x1<=x2; x1++) {
      err -= dy;
      if(err < 0) {
        INT16 len = x1-xbegin;
        if(len) {
          if(hDC->pen.size==1)
            drawVertLineWindowColor(hDC,y1,xbegin,xbegin+len,c);
          else {
            while(len--) {
              fillRectangleWindowColor(hDC,y1,xbegin,y1+hDC->pen.size-1,xbegin+hDC->pen.size-1,c);
              xbegin++;
              }
            }
          }
        else {
          if(hDC->pen.size==1)
            drawPixelWindowColor(hDC,y1,x1,c);
          else
            fillRectangleWindowColor(hDC,y1,x1,y1+hDC->pen.size-1,x1+hDC->pen.size-1,c);
          }
        xbegin = x1+1;
        y1 += ystep;
        err += dx;
        }
      }
    if(x1 > xbegin+1) {
      if(hDC->pen.size==1)
        drawVertLineWindowColor(hDC,y1,xbegin,x1+1,c);
      else {
        while(xbegin<=x1) {
          fillRectangleWindowColor(hDC,y1,xbegin,y1+hDC->pen.size-1,xbegin+hDC->pen.size-1,c);
          xbegin++;
          }
        }
      }
    } 
  else {
    for(; x1<=x2; x1++) {
      err -= dy;
      if(err < 0) {
        INT16 len = x1-xbegin;
        if(len) {
          if(hDC->pen.size==1)
            drawHorizLineWindowColor(hDC,xbegin,y1,xbegin+len,c);
          else {
            while(len--) {
              fillRectangleWindowColor(hDC,xbegin,y1,xbegin+hDC->pen.size-1,y1+hDC->pen.size-1,c);
              xbegin++;
              }
            }
          }
        else {
          if(hDC->pen.size==1)
            drawPixelWindowColor(hDC,x1,y1,c);
          else
            fillRectangleWindowColor(hDC,x1,y1,x1+hDC->pen.size-1,y1+hDC->pen.size-1,c);
          }
        xbegin = x1+1;
        y1 += ystep;
        err += dx;
        }
      }
    if(x1 > xbegin+1) {
      if(hDC->pen.size==1)
        drawHorizLineWindowColor(hDC,xbegin,y1,x1,c);
      else {
        while(xbegin<=x1) {
          fillRectangleWindowColor(hDC,xbegin,y1,xbegin+hDC->pen.size-1,y1+hDC->pen.size-1,c);
          xbegin++;
          }
        }
      }
    }

  }

void drawRectangleWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  x2 &= 0xffff;
  y2 &= 0xffff;
  drawHorizLineWindow(hDC,x1,y1,x2);
  drawHorizLineWindow(hDC,x1,y2,x2);
  drawVertLineWindow(hDC,x1,y1,y2);
  drawVertLineWindow(hDC,x2,y1,y2/*+1*/);   // per chiudere
  }

void drawRectangleWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c) {

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  x2 &= 0xffff;
  y2 &= 0xffff;
  drawHorizLineWindowColor(hDC,x1,y1,x2,c);
  drawHorizLineWindowColor(hDC,x1,y2,x2,c);
  drawVertLineWindowColor(hDC,x1,y1,y2,c);
  drawVertLineWindowColor(hDC,x2,y1,y2 /*+1*/,c);    // per chiudere
  }

void fillRectangleWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, 
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  x2 &= 0xffff;
  y2 &= 0xffff;
  while(y1<=y2)
    drawHorizLineWindowColor(hDC,x1,y1++,x2,hDC->brush.color);
  }

void fillRectangleWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, 
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c) {
  
  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  x2 &= 0xffff;
  y2 &= 0xffff;
  while(y1<=y2)
    drawHorizLineWindowColor(hDC,x1,y1++,x2,c);
  }

void drawEllipseWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  x2 &= 0xffff;
  y2 &= 0xffff;
  
  UGRAPH_COORD_T rx=(x2-x1)/2,ry=(y2-y1)/2;
  UGRAPH_COORD_T x0=x1+rx,y0=y1+ry;
  
  //https://www.geeksforgeeks.org/midpoint-ellipse-drawing-algorithm/
  GRAPH_COORD_T dx, dy, d, x, y;
  x = 0;
  y = ry;

  // Initial decision parameter of region 1
  d = (ry*ry) - (rx*rx*ry) + (rx*rx/4);
  dx = 2*ry*ry*x;
  dy = 2*rx*rx*y;

  // For region 1
  while(dx < dy) {

     // Print points based on 4-way symmetry
		if(hDC->pen.size==1) {  //
			drawPixelWindow(hDC,x0+x,y0+y);
			drawPixelWindow(hDC,x0-x,y0+y);
			drawPixelWindow(hDC,x0+x,y0-y);
			drawPixelWindow(hDC,x0-x,y0-y);
			}
		else {
			fillRectangleWindow(hDC,x0+x, y0+y, x0+x+hDC->pen.size-1, y0+y+hDC->pen.size-1);
			fillRectangleWindow(hDC,x0-x, y0+y, x0-x+hDC->pen.size-1, y0+y+hDC->pen.size-1);
			fillRectangleWindow(hDC,x0+x, y0-y, x0+x+hDC->pen.size-1, y0-y+hDC->pen.size-1);
			fillRectangleWindow(hDC,x0-x, y0-y, x0-x+hDC->pen.size-1, y0-y+hDC->pen.size-1);
			}

    // Checking and updating value of decision parameter based on algorithm
    if(d<0) {
      x++;
      dx += (2*ry*ry);
      d += dx + (ry*ry);
			}
    else {
      x++;
      y--;
      dx += (2*ry*ry);
      dy -= (2*rx*rx);
      d += dx - dy + (ry*ry);
      }
    }
 
  // Decision parameter of region 2
  d = ((ry*ry) * ((x + 0.5) * (x + 0.5))) +
       ((rx*rx) * ((y-1) * (y-1))) -
       (rx*rx*ry*ry);
 
  // Plotting points of region 2
  while(y>=0) {

		if(hDC->pen.size==1) {  //
			drawPixelWindow(hDC,x0+x, y0+y);
			drawPixelWindow(hDC,x0-x, y0+y);
			drawPixelWindow(hDC,x0+x, y0-y);
			drawPixelWindow(hDC,x0-x, y0-y);
			}
		else {
			fillRectangleWindow(hDC,x0+x, y0+y, x0+x+hDC->pen.size-1, y0+y+hDC->pen.size-1);
			fillRectangleWindow(hDC,x0-x, y0+y, x0-x+hDC->pen.size-1, y0+y+hDC->pen.size-1);
			fillRectangleWindow(hDC,x0+x, y0-y, x0+x+hDC->pen.size-1, y0-y+hDC->pen.size-1);
			fillRectangleWindow(hDC,x0-x, y0-y, x0-x+hDC->pen.size-1, y0-y+hDC->pen.size-1);
			}
			
		// Checking and updating parameter value based on algorithm
		if(d>0) {
      y--;
      dy -= (2*rx*rx);
      d += (rx*rx) - dy;
      }
    else {
      y--;
      x++;
      dx += (2*ry*ry);
      dy -= (2*rx*rx);
      d += dx - dy + (rx*rx);
      }
	  }

  }

void fillEllipseWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {

  x1 &= 0xffff;      // uso int per velocità ma poi mi basta uint16..
  y1 &= 0xffff;
  x2 &= 0xffff;
  y2 &= 0xffff;
  
  UGRAPH_COORD_T rx=(x2-x1)/2,ry=(y2-y1)/2;
  UGRAPH_COORD_T x0=x1+rx,y0=y1+ry;
  GRAPH_COORD_T x,y;
  
	// https://stackoverflow.com/questions/10322341/simple-algorithm-for-drawing-filled-ellipse-in-c-c
	int hh = ry * ry;
	int ww = rx * rx;
	int hhww = hh * ww;
	x2 = rx;
	int dx = 0;

	// do the horizontal diameter 
	drawHorizLineWindowColor(hDC,x0-rx, y0, x0+rx, hDC->brush.color);
  // questo presume che la dim verticale sia dispari...

// now do both halves at the same time, away from the diameter
	for(y=1; y <= ry; y++) {
    x1 = x2 - (dx-1);  // try slopes of dx - 1 or more
    for( ; x1 > 0; x1--)
      if(x1*x1*hh + y*y*ww <= hhww)
        break;

    dx = x2 - x1;  // current approximation of the slope
    x2 = x1;

		drawHorizLineWindowColor(hDC,x0-x2, y0+y, x0+x2, hDC->brush.color);
		drawHorizLineWindowColor(hDC,x0-x2, y0-y, x0+x2, hDC->brush.color);
		}

  }

static BOOL isPointVisibleHelper(HWND myWnd,HWND hWnd,POINT pt) {
  RECT rc;
  
  while(myWnd) {
    if(myWnd->visible) {
			rc=myWnd->nonClientArea;
      
			HWND myWnd2=hWnd;
	    while(myWnd2) {
        
        
	      OffsetRect2(&rc,&rc,myWnd2->clientArea.left,myWnd2->clientArea.top);
			  myWnd2=myWnd2->parent;
				}
// no      IntersectRect(&rc,&rc,&myWnd->clientArea);
      IntersectRect(&rc,&rc,&hWnd->clientArea);		// trimmate sulla mia client area
      if(wndPtInRect(&rc,pt)) {
        return 0;
        }
      }
    myWnd=myWnd->next;
    }
  return TRUE;
  }
static BOOL isPointVisible(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  // entro con coordinate schermo, ossia già traslate rispetto a MIO hDC e poi come figlia (di figlia ecc)
  HWND hWnd,myWnd;
  POINT pt;
  
  hWnd=WindowFromDC(hDC);
  
  if(hWnd->locked)      // 
    return FALSE;
  
  pt.x=x; pt.y=y;
  if(activeMenu && activeMenuWnd==desktopWindow) {    // per i menu popup... 
    if(wndPtInRect(&activeMenuRect,pt)) 
      return FALSE;
    }
  
  if(hWnd->active /* o zOrder*/ && !(hWnd->style & (WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS))) // ..quindi, se sono top-level, attiva o senza clipping, do ok
    // mancherebbe taskbar...
	  return TRUE;
  
  if(hWnd->style & WS_CHILD /* && hWnd->parent */) {    // ora, se sono figlia..
    myWnd=hWnd->parent;
    RECT rc;
    rc=myWnd->clientArea;
    if(myWnd->minimized || !myWnd->visible)     // ovviamente! e a cascata
      return 0;
    while(myWnd=myWnd->parent) {
      if(myWnd->minimized || !myWnd->visible)     // ovviamente! e a cascata
        return 0;
      if(myWnd==taskbarWindow) {   // gestisco taskbar, se non on top
        if(!(GetWindowByte(taskbarWindow,GWL_USERDATA+0) & 2)) {
// come fare qua? vedere se il punto è coperto da una qualsiasi altra top window?          if(wndPtInRect(&taskbarWindow->nonClientArea,pt))
//            return FALSE;
          }
        }
      OffsetRect2(&rc,&rc,myWnd->clientArea.left,myWnd->clientArea.top);
      IntersectRect(&rc,&rc,&myWnd->clientArea);
      }
    if(!wndPtInRect(&rc,pt))     // ..non devo uscire da mio padre/nonni (traslato/i come serve)!
      return 0;
    
    
    if(hDC->flags & (WS_CLIPSIBLINGS>>25)) {   // ..e se devo evitare i fratelli, li controllo tutti
      myWnd=hWnd->parent->children;
      while(myWnd) {
        if(myWnd->visible && myWnd != hWnd && myWnd->zOrder > hWnd->zOrder) {    //
				  RECT rc;
					rc=myWnd->nonClientArea;
					HWND myWnd2=hWnd;
					while(myWnd2) {
						OffsetRect2(&rc,&rc,myWnd2->parent->clientArea.left,myWnd2->parent->clientArea.top);
						myWnd2=myWnd2->parent;
						}
          if(wndPtInRect(&rc,pt)) {
            return FALSE;
            }
          }
        myWnd=myWnd->next;
        }
      }
    
    }

  if(hDC->flags & (WS_CLIPCHILDREN>>25)) {   // ovvero, se devo ritagliare le figlie..
    if(!isPointVisibleHelper(hWnd->children,hWnd,pt))
      return FALSE;
    }

  if(!(hWnd->style & WS_CHILD)) {    // e, se NON sono figlia..
    myWnd=hWnd->next;   // ora, le top level: inizio da quella subito sopra di me
    while(myWnd) {
      if(myWnd->zOrder > hWnd->zOrder) {    // in teoria questo test è inutile, dato il sort
        if(wndPtInRect(&myWnd->nonClientArea,pt)) {
          return FALSE;
          }
        }
      myWnd=myWnd->next;

      }
    if(taskbarWindow && hWnd != taskbarWindow) {   // e la taskbar, se on top
      if(GetWindowByte(taskbarWindow,GWL_USERDATA+0) & 2)
        if(wndPtInRect(&taskbarWindow->nonClientArea,pt)) 
          return FALSE;
      }
    }

  return TRUE;
  }

static HWND windowFromPointHelper(HWND myWnd,POINT pt,BYTE *myOrder) {
  HWND foundWnd=NULL;
  
  while(myWnd) {
    if(myWnd->visible && /*myWnd->enabled */ myWnd->zOrder > *myOrder && wndPtInRect(&myWnd->nonClientArea,pt)) {
			*myOrder=myWnd->zOrder;
      foundWnd=myWnd;
      HWND myWnd2;
      
			*myOrder=0;   // boh

      {
      POINT pt2;
      pt2.x=pt.x-myWnd->clientArea.left;
      pt2.y=pt.y-myWnd->clientArea.top;
      myWnd2=windowFromPointHelper(myWnd->children,pt2,myOrder);
      }
      if(myWnd2)
        foundWnd=myWnd2;
      }
    myWnd=myWnd->next;
    } 
fine:
  return foundWnd;
  }
HWND WindowFromPoint(POINT pt) { //not retrieve a handle to a hidden or disabled
	// If the point is over a static text control, the return value is a handle to the window under the static text control.
  HWND foundWnd;
  BYTE myOrder=0;
          
  if(GetWindowByte(taskbarWindow,GWL_USERDATA+0) & 2) {       // se on top
    if(foundWnd=windowFromPointHelper(taskbarWindow,pt,&myOrder))
      return foundWnd;
    }
  
  myOrder=0;
  if(!(foundWnd=windowFromPointHelper(rootWindows,pt,&myOrder))) {
    if(!(GetWindowByte(taskbarWindow,GWL_USERDATA+0) & 2)) {       // se on top
      myOrder=0;
      if(foundWnd=windowFromPointHelper(taskbarWindow,pt,&myOrder))
        return foundWnd;
      }
    myOrder=0;
    foundWnd=desktopWindow ? (wndPtInRect(&desktopWindow->nonClientArea,pt) ?
      desktopWindow : NULL) : NULL;  // diciamo che sarebbe sicuro :) ma ok
    }
  return foundWnd;
  }
HWND ChildWindowFromPoint(HWND parent,POINT pt) {
  HWND foundWnd;
  BYTE myOrder=0;
  POINT pt2;
  HWND myWnd=parent;
  
  do {
    pt2.x=pt.x-myWnd->clientArea.left;    // entrano coord relative a finesta/parent
    pt2.y=pt.y-myWnd->clientArea.top;
    myWnd=parent->parent;
    } while(myWnd);
  
  if(!(foundWnd=windowFromPointHelper(parent->children,pt2,&myOrder))) {
    if(wndPtInRect(&parent->nonClientArea,pt2))
      foundWnd=parent;
    }
  return foundWnd;
  }

HWND GetWindow(HWND hWnd,UINT uCmd) {
  HWND myWnd;
  
  switch(uCmd) {
    case GW_CHILD:
      // occhio anche a zorder cmq
      // getLastChildWindow()
      return hWnd->children;
      break;

    case GW_ENABLEDPOPUP:
      break;

    case GW_HWNDFIRST:
      // finire...
      break;

    case GW_HWNDLAST:
      break;

    case GW_HWNDNEXT:
      break;

    case GW_HWNDPREV:
      break;
    case GW_OWNER:
      return hWnd->parent;    // ma se parent è WS_CHILD allora salire di parent!
      break;
    }
  }

static HWND getLastChildWindow(HWND hWnd) {
  HWND myWnd=hWnd->children;
  
  while(myWnd && myWnd->next)
    myWnd=myWnd->next;
  return myWnd;
  }
static HWND getActiveChildWindow(HWND hWnd) {
  HWND myWnd=hWnd->children;
  
  while(myWnd) {
    if(myWnd->active) /* && myWnd->internalState==MSGF_DIALOGBOX*/  /*oe/ focus??*/
      break;
    myWnd=myWnd->next;
    }
  return myWnd;
  }

BOOL IsChild(HWND hWndParent,HWND hWnd) {
  HWND myWnd=hWndParent->children;
  
  while(myWnd) {
    if(myWnd==hWnd)
      return TRUE;
    myWnd=myWnd->next;
    }
  if(hWndParent->parent)    // bah ricorsiva direi ok così
    return IsChild(hWndParent->parent,hWnd);
  return FALSE;
  }

BOOL EnumChildWindows(HWND hWndParent,WNDENUMPROC lpEnumFunc,LPARAM lParam) {
  HWND myWnd=hWndParent->children;
  
  while(myWnd) {
    if(!lpEnumFunc(myWnd,lParam))
      break;
    EnumChildWindows(myWnd,lpEnumFunc,lParam);    // ricorsivo per le child
    myWnd=myWnd->next;
    }
  // return not used... ;)
  }

BOOL EnumWindows(WNDENUMPROC lpEnumFunc,LPARAM lParam) {
  HWND myWnd;
  BYTE n=0;

  if(!lpEnumFunc(desktopWindow,lParam))
    return 0;
  if(!lpEnumFunc(taskbarWindow,lParam))
    return 0;
  myWnd=rootWindows;
  while(myWnd) {
    n=1;
    if(!lpEnumFunc(myWnd,lParam))
      return 0;
    myWnd=myWnd->next;
    }
  return n;
  }

static BYTE countWindows(void) {
  HWND myWnd;
  BYTE n=0;

  myWnd=rootWindows;
  while(myWnd) {
    n++;
    myWnd=myWnd->next;
    }
  return n;
  }

BOOL EnumDesktopWindows(HWND hWndDesk,WNDENUMPROC lpEnumFunc,LPARAM lParam) {
  HWND myWnd;

  myWnd=hWndDesk ? hWndDesk : desktopWindow;    // così posso passare rootWindows se le voglio tutte!
  while(myWnd) {
    lpEnumFunc(desktopWindow,lParam);
    myWnd=myWnd->next;
    }
  }

HWND /*HDESK*/ GetThreadDesktop(DWORD dwThreadId) {
  
  return desktopWindow /*thread*/;   // bah sì :)
  }
HWND /*HDESK*/ OpenDesktop(const char *lpszDesktop,DWORD dwFlags,BOOL fInherit,DWORD /*ACCESS_MASK*/ dwDesiredAccess) {
  
  return desktopWindow;   // idem :)
  }

BOOL SetRect(RECT *lprc,UGRAPH_COORD_T xLeft,UGRAPH_COORD_T yTop,UGRAPH_COORD_T xRight,UGRAPH_COORD_T yBottom) {
  
  lprc->left=xLeft;
  lprc->right=xRight;
  lprc->top=yTop;
  lprc->bottom=yBottom;
  }

BOOL SetRectEmpty(RECT *lprc) {
  
  lprc->left=lprc->right=lprc->top=lprc->bottom=0;
  }

BOOL UnionRect(RECT *rcDst,const RECT *rcSrc1,const RECT *rcSrc2) {
  
  rcDst->left=min(rcSrc1->left,rcSrc2->left);
  rcDst->top=min(rcSrc1->top,rcSrc2->top);
  rcDst->right=max(rcSrc1->right,rcSrc2->right);
  rcDst->bottom=max(rcSrc1->bottom,rcSrc2->bottom);
  
  return !IsRectEmpty(rcDst);
  }

inline void __attribute__((always_inline)) OffsetRect(RECT *lprc,int16_t dx,int16_t dy) {
  
  lprc->left+=dx;
  lprc->right+=dx;
  lprc->top+=dy;
  lprc->bottom+=dy;
  }

inline void __attribute__((always_inline)) OffsetPoint(POINT *lppt,int16_t dx,int16_t dy) {
  
  lppt->x += dx;
  lppt->y += dy;
  }

inline void __attribute__((always_inline)) OffsetRect2(RECT *lprcd,RECT *lprcs,int16_t dx,int16_t dy) {
  
  lprcd->left=lprcs->left+dx;
  lprcd->right=lprcs->right+dx;
  lprcd->top=lprcs->top+dy;
  lprcd->bottom=lprcs->bottom+dy;
  }

inline BOOL __attribute__((always_inline)) IntersectRect(RECT *lprcDst,const RECT *lprcSrc1,const RECT *lprcSrc2) {
  
  if(lprcDst) {
    lprcDst->left = max(lprcSrc1->left, lprcSrc2->left);
    lprcDst->right = min(lprcSrc1->right, lprcSrc2->right);
    lprcDst->top = max(lprcSrc1->top, lprcSrc2->top);
    lprcDst->bottom= min(lprcSrc1->bottom, lprcSrc2->bottom);
    }
  return ! (lprcSrc2->left > lprcSrc1->right || lprcSrc2->right < lprcSrc1->left
        || lprcSrc2->top > lprcSrc1->bottom  || lprcSrc2->bottom < lprcSrc1->top);
  }

inline void __attribute__((always_inline)) InflateRect(RECT *lprc,int dx,int dy) {
  
  lprc->left-=dx;
  lprc->right+=dx;
  lprc->top-=dy;
  lprc->bottom+=dy;
  }

inline BOOL __attribute__((always_inline)) CopyRect(RECT *lprcDst,const RECT *lprcSrc) {
  
  *lprcDst=*lprcSrc;
  return TRUE;
  }


  
BOOL SetMenu(HWND hWnd,HMENU Menu) {
  
  hWnd->menu=Menu;
  }
HMENU GetMenu(HWND hWnd) {
  
  return hWnd->menu;
  }
BOOL GetMenuInfo(HMENU Menu,MENUINFO *mi) {
  
  mi->dwStyle=Menu->menuItems[0].flags & (MNS_NOTIFYBYPOS | MNS_CHECKORBMP); // FINIRE..
  mi->fMask=Menu->menuItems[0].flags;
  mi->hbrBack=GetStockObject(GRAY_BRUSH).brush;
  S_SIZE cs;
  getMenuSize(Menu,&cs,1);    // diciamo popup, in caso estendere..
  mi->cyMax=cs.cy;
  }
BOOL GetMenuBarInfo(HWND hWnd,LONG idObject,uint16_t idItem,MENUBARINFO *mbi) {
  
  mbi->hMenu=NULL;
  switch(idObject) {
    case OBJID_CLIENT:    // popup associated with the window... FARE/finire se si vuole
      mbi->hMenu=activeMenu;      // boh per ora!
      if(idItem==0) {
        S_SIZE cs;
        getMenuSize(mbi->hMenu,&cs,1);
        mbi->rcBar.left=activeMenuRect.left;      // funzia se il menu è aperto, diciamo ok
        mbi->rcBar.top=activeMenuRect.top;
        mbi->rcBar.right=mbi->rcBar.left+cs.cx;
        mbi->rcBar.bottom=mbi->rcBar.top+cs.cy;
        }
      else {
        S_SIZE cs;
        getMenuSize(mbi->hMenu,&cs,1);
        mbi->rcBar.left=activeMenuRect.left;      // funzia se il menu è aperto, diciamo ok
        mbi->rcBar.top=activeMenuRect.top;
        mbi->rcBar.right=mbi->rcBar.left+cs.cx;
        mbi->rcBar.bottom=mbi->rcBar.top+cs.cy;
        }
      break;
    case OBJID_MENU:    // dim. menubar
      mbi->hMenu=hWnd->menu;
      S_SIZE cs;
      getMenuSize(mbi->hMenu,&cs,0);
      mbi->rcBar.left=hWnd->nonClientArea.left;
      mbi->rcBar.top=hWnd->nonClientArea.left+TITLE_HEIGHT;
      mbi->rcBar.right=mbi->rcBar.left+cs.cx;
      mbi->rcBar.bottom=mbi->rcBar.top+cs.cy;
      break;
    case OBJID_SYSMENU:   // dim sysmenu
      if(hWnd->style & WS_SYSMENU) {
        mbi->hMenu=(HMENU)&systemMenu;
        S_SIZE cs;
        getMenuSize(mbi->hMenu,&cs,1);
        mbi->rcBar.left=hWnd->nonClientArea.left;
        mbi->rcBar.top=hWnd->nonClientArea.left+TITLE_HEIGHT;
        mbi->rcBar.right=mbi->rcBar.left+cs.cx;
        mbi->rcBar.bottom=mbi->rcBar.top+cs.cy;
        }
      break;
    }
  if(mbi->hMenu) {
    mbi->hwndMenu=hWnd;
    mbi->fBarFocused=mbi->hMenu==activeMenu ? 1 : 0;    // diciamo
    mbi->fFocused=0 /*activeMenuCntY .. */;   // fare
    return 1;
    }
  return 0;
  }
int GetMenuString(HMENU hMenu,UINT uIDItem,LPSTR lpString,uint16_t cchMax,UINT flags) {
  int n;
  
  if(flags == MF_BYCOMMAND) {
    n=0;
    while(hMenu->menuItems[n].command != uIDItem && n<MAX_MENUITEMS) {
      n++;
      }
    if(n>=MAX_MENUITEMS)
      return 0;
    }
  else 
    n=uIDItem;
  if(!(hMenu->menuItems[n].flags & (MF_BITMAP | MF_SEPARATOR | MF_POPUP))) {
    strncpy(lpString,hMenu->menuItems[n].text,cchMax);
    return strlen(lpString);
    }
  else
    return 0;
  }
int GetMenuItemCount(HMENU Menu) {
  BYTE i,nmax=0;
  MENUITEM *m;
  
  for(i=0; i<MAX_MENUITEMS; i++) { 
    m=&Menu->menuItems[i];
    if(m && (m->bitmap || m->command || m->flags)) {
      }
    else
      break;
    nmax++;
    }
  return nmax;
  }
MENUITEM *GetMenuItemFromCommand(HMENU Menu,uint16_t cmd) {   // mia estensione :)
  BYTE i;
  MENUITEM *m;
  
  m=&Menu->menuItems[i];
  for(i=0; i<MAX_MENUITEMS; i++) { 
    if(m->command==cmd)
      return m;
    }
  return NULL;
  }
BYTE MenuItemFromPoint(HWND hWnd,HMENU hMenu,POINT ptScreen) {
  // si potrebbero riunire le due private getMenuFrom qua...
  RECT rc;
  BYTE i;
  MENUITEM *m;

  i=0;
  if(!hWnd) {    // trovare la finestra, se è un popup...
    hWnd=activeMenuWnd;
    }
//ptScreeen: A structure that specifies the location to test. If hMenu specifies a menu bar, this parameter is in window coordinates. Otherwise, it is in client coordinates.
  if(hMenu==hWnd->menu) {
    rc.left=hWnd->nonClientArea.left+BORDER_SIZE;
    rc.right=rc.left;
    rc.top=hWnd->nonClientArea.top+TITLE_HEIGHT+1;
    rc.bottom=rc.top+1+MENU_HEIGHT;
    ptScreen.x+=hWnd->nonClientArea.left;
    ptScreen.y+=hWnd->nonClientArea.top;
    //OffsetPoint(
    do {
      m=&hMenu->menuItems[i];
      if(m && (m->bitmap || m->command || m->flags)) {
        if(m->flags & MF_BITMAP) {
          rc.right += 16+2;  //finire..
          }
        else if(*m->text) {
          rc.right += strlen(m->text)*6 +2;     // font size fisso...
          if(strchr(m->text,'&'))
            rc.right-=6;
          }
        else if(m->flags & MF_SEPARATOR) {
          rc.right +2;
          }

        if(wndPtInRect(&rc,ptScreen)) {
          return i;
          }
        }
      else
        break;

      rc.left = rc.right+2;
      rc.right = rc.left;

      i++;
      } while(m);
    }
  else {
    ptScreen.x+=hWnd->clientArea.left;
    ptScreen.y+=hWnd->clientArea.top;
    //OffsetPoint(
    RECT rc;
    POINTS pt;
    S_SIZE sz;

    pt=getMenuPosition(hWnd->menu,hMenu);
    rc.left=pt.x+1;   // salto i bordi
    rc.top=pt.y+1;
    getMenuSize(hMenu,&sz,1);
    rc.right=sz.cx+rc.left;
    rc.bottom=sz.cy+rc.top;
    for(i=0; i<MAX_MENUITEMS; i++) { 
      m=&hMenu->menuItems[i];

      if(m->bitmap || m->command || m->flags) {
        // andrà poi fatto ricorsivo per i vari sottopopup..
        if(m->flags & MF_BITMAP) {
          rc.bottom=rc.top+16;
          }
        else if(*m->text) {
          rc.bottom=rc.top+MENU_HEIGHT;
          }
        else if(m->flags & MF_SEPARATOR) {
          rc.bottom=rc.top+1;
          }

        if(wndPtInRect(&rc,ptScreen)) {
          return i;
          }
        }
      else
        break;

      rc.top = rc.bottom+1;
      rc.bottom = rc.top;
      }
  
    }

  return -1;
  }
uint16_t GetMenuItemID(HMENU Menu,BYTE nPos) {
  
  return Menu->menuItems[nPos].command;
  }
uint16_t GetMenuDefaultItem(HMENU Menu,BYTE fByPos,uint16_t gmdiFlags) {
  return -1; // fare...
  }
BOOL SetMenuDefaultItem(HMENU Menu,uint16_t item,BYTE fByPos) {
  return -1; // fare...
  }
UGRAPH_COORD_T GetMenuCheckMarkDimensions() { return 8; }     // v.CXMENUCHECK 
BOOL EnableMenuItem(HMENU hMenu,uint16_t uIDEnableItem,BYTE uEnable) {  
  }
DWORD CheckMenuItem(HMENU hMenu,uint16_t uIDEnableItem,uint16_t uCheck) {
  }
BOOL AppendMenuA(HMENU Menu,uint16_t uFlags,uint16_t uIDNewItem,const char *lpNewItem) {
  }
BOOL ModifyMenu(HMENU Menu,uint16_t uPosition,uint16_t uFlags,uint16_t uIDNewItem,const char *lpNewItem) {
  }
BOOL InsertMenu(HMENU Menu,uint16_t uPosition,uint16_t uFlags,uint16_t uIDNewItem,const char *lpNewItem) {
  }
BOOL InsertMenuItem(HMENU Menu,UINT item,BOOL fByPosition,MENUITEMINFO *lpmi) {
  }
HMENU CreatePopupMenu(HMENU menu) {
  menu->menuItems[0].bitmap=0;
  menu->accelerators[0].key[0]=0;
  return menu;
  }
BOOL DestroyMenu(HMENU hMenu) {
  }
BOOL DeleteMenu(HMENU hMenu,BYTE uPosition,uint16_t uFlags) {
  }
HMENU GetSystemMenu(HWND hWnd,BOOL bRevert) {
  if(!hWnd)
    return (MENU*)&systemMenu;    // mia aggiunta :)
  else if(hWnd->style & WS_SYSMENU)
    return (MENU*)&systemMenu;    
  else
    return NULL;
  }
HMENU GetActiveMenu(void) {
  return activeMenu;
  }
HMENU GetSubMenu(HMENU menu,uint16_t nPos) {
  
  if(nPos<MAX_MENUITEMS)
    return menu->menuItems[nPos].menu;
  else
    return NULL;
  }

static char findMenuLetter(const char *s) {
  char ch=0;
  const char *s1=s;

  while(*s1 && *s1!='&') {   // la lettera sottolineata! in culo ISTC bechis di merda 1990 :D
    s1++;
    }
  if(*s1)
    ch=*(s1+1);    // e NON mettere a fine parola ;)
  else
    ch=*s;
  return toupper(ch);   // evitiamo cagate :)
  }
uint16_t getMenuPopupFromPoint(HMENU menu,RECT *rc,POINT pt,HMENU*inMenu) {
  BYTE i;
  RECT rc2;
  MENUITEM *m;
  
  rc2=*rc;
  rc2.left++;   // salto i bordi
  rc2.top++;
  rc2.right--;
  for(i=0; i<MAX_MENUITEMS; i++) { 
    m=&menu->menuItems[i];
    
    if(m->bitmap || m->command || m->flags) {
      // andrà poi fatto ricorsivo per i vari sottopopup..
      if(m->flags & MF_BITMAP) {
        rc2.bottom=rc2.top+16;
        }
      else if(*m->text) {
        rc2.bottom=rc2.top+MENU_HEIGHT;
        }
      else if(m->flags & MF_SEPARATOR) {
        rc2.bottom=rc2.top+1;
        }
      
      if(wndPtInRect(&rc2,pt)) {
        if(inMenu)
          *inMenu=(MENU*)m;
        return m->command;
        }
      }
    else
      break;
    
    rc2.top = rc2.bottom+1;
    rc2.bottom = rc2.top;
    }
  
  if(inMenu)
    *inMenu=NULL;
  return 0;
  }

static BYTE getMenuSize(HMENU menu,S_SIZE *cs,BYTE flags) {
  uint16_t xs,ys;
  BYTE i,nmax=0;
  MENUITEM *m;
  
  xs=0; ys=0;
  if(flags) {   //1 x popup, 0 per menubar
    for(i=0; i<MAX_MENUITEMS; i++) { 
      m=&menu->menuItems[i];
      if(m->bitmap || m->command || m->flags) {
        if(m->flags & MF_BITMAP) {
          if(m->flags & MF_POPUP)
            xs=max(xs,6+16);    // finire...
          else
            xs=max(xs,16);    // finire...
          ys+=MENU_HEIGHT+1;
          }
        else if(*m->text) {
          int xs2=strlen(m->text)*6;         // font size fisso...
          if(strchr(m->text,'&'))
            xs2-=6;
          if(m->flags & MF_POPUP)
            xs=max(xs,xs2);
          else
            xs=max(xs,xs2);
          ys+=MENU_HEIGHT+1;
          }
        else if(m->flags & MF_SEPARATOR) {
          xs=max(xs,8);     // bah diciamo :) come emergenza
          ys+=1 /*+1*/;
          }
        }
      else
        break;
      nmax++;
      }
    ys++;
    }
  else {
    i=0;
    m=&menu->menuItems[0];
    do {
      if(m && (m->bitmap || m->command || m->flags)) {
        if(m->flags & MF_BITMAP) {
          xs+= 16 +   2;    // per vertical line
          }
        else if(*m->text) {
          xs=strlen(m->text)*6;     // font size fisso... OCCHIO AMPERSAND! v.drawtext
          if(strchr(m->text,'&'))
            xs-=6;
          xs+=2;    // per vertical line
          }
        else if(m->flags & MF_SEPARATOR) {
          xs+=2;
          }
        xs+=2;
        ys=MENU_HEIGHT;
        }

      i++;
      if(i>=MAX_MENUITEMS)
        break;
      m=&menu->menuItems[i];
      } while(m->bitmap || m->command || m->flags);
    }
  
  cs->cx=xs;
	cs->cy=ys;
  return nmax;
  }
static S_SIZE drawMenuitem(HDC hDC,MENUITEM *m,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE state) {
  S_SIZE sz={0};
  
  hDC->foreColor=hDC->pen.color= state ? windowForeColor : windowInactiveForeColor;
  if(state & 2)
    hDC->foreColor = WHITE;  // o reverse?
  else {
    if(m->flags & MF_GRAYED)   //fare
      hDC->foreColor /= 2;    //boh :D
    if(m->flags & MF_DISABLED)    //fare
      hDC->foreColor /= 2;    //boh :D
    }

  if(m && (m->bitmap || m->command || m->flags)) {
    if(m->flags & MF_BITMAP) {
      sz.cx+= 16 +   2;    // per vertical line
      }
    else if(*m->text) {
      sz.cx=strlen(m->text)*6;     // font size fisso... OCCHIO AMPERSAND! v.drawtext
      if(strchr(m->text,'&'))
        sz.cx-=6;
      sz.cx+=2;    // per vertical line
      }
    else if(m->flags & MF_SEPARATOR) {
      sz.cx=2;
      }
    fillRectangleWindowColor(hDC,x,y+1,x+sz.cx-1,y+MENU_HEIGHT-1,windowBackColor);
    drawVertLineWindow(hDC,x+sz.cx,y,y+MENU_HEIGHT+1);
    if(m->flags & MF_BITMAP) {
  //        DrawBitmap(hDC,x,ys,m->bitmap);
      }
    else if(*m->text) {
      int x1=x+1;
      RECT rc;
      
      rc.left=x1; 
			rc.top=y+2;
      rc.right=x1+sz.cx-2;
			rc.bottom=y+2+MENU_HEIGHT+1;
      DrawText(hDC,m->text,strlen(m->text),&rc,DT_LEFT | DT_SINGLELINE);
      }
    else if(m->flags & MF_SEPARATOR) {
      drawVertLineWindow(hDC,x+sz.cx+1,y,y+MENU_HEIGHT+1); // si duplica se ultima ma ok..
      // opp pare + spessa e ok :)
      }
    sz.cx+=2;
    sz.cy=MENU_HEIGHT;
    }
  return sz;
  }
static BOOL drawMenu(HWND hWnd,HMENU menu,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE mode,int8_t selectedItem) {    // mode: b0=active/inactive, b1=selected
  S_SIZE sz;
  BYTE i;
  DC myDC;
  HDC hDC;
  MENUITEM *m;
  HRGN rgn;

  rgn=CreateRectRgn(hWnd->nonClientArea.left/*+BORDER_SIZE*/,hWnd->nonClientArea.top/*+BORDER_SIZE*/,
    hWnd->nonClientArea.right-BORDER_SIZE,hWnd->nonClientArea.bottom-BORDER_SIZE); // per non sporcare i bordi!
  hDC=GetDCEx(hWnd,&myDC,rgn,DCX_WINDOW | DCX_PARENTCLIP | DCX_INTERSECTRGN /*|DCX_LOCKWINDOWUPDATE*/);

  selectedItem--; //entra 0 per nessuno
  
  i=0;
  m=&menu->menuItems[0];
  do {
    sz=drawMenuitem(hDC,m,x,y,mode | (i==selectedItem ? 2 : 0));
//bah no    if(!sz.cx || !sz.cy)
//      break;
    x+=sz.cx;
//    if(x >= rc.right) bah cmq viene trimmata in video!
//      break;
    i++;
    if(i>=MAX_MENUITEMS)
      break;
    m=&menu->menuItems[i];
    } while(m->bitmap || m->command || m->flags);
// non serve perché la nonclientPaint, però...  DrawHorizVertLineWindowColor(hDC,x,y+MENU_HEIGHT,x+xs+1,itemsColor);
  
  ReleaseDC(hWnd,hDC);
  }
static S_SIZE drawMenuitemPopup(HDC hDC,MENUITEM *m,UGRAPH_COORD_T x,UGRAPH_COORD_T y,S_SIZE cs,BYTE state) {
  uint16_t x1=x,y1=y;
  RECT rc;
  
  hDC->foreColor=hDC->pen.color=windowForeColor;    // direi ovviamente
  if(state & 1)
    hDC->foreColor = WHITE;  // o reverse?
  else {
    if(m->flags & MF_GRAYED)    //fare
      hDC->foreColor /= 2;    //boh :D      hDC->font.strikethrough = 1;    // boh :)
    if(m->flags & MF_DISABLED)    //fare
      hDC->foreColor /= 2;    //boh :D      hDC->font.strikethrough = 1;    // boh :)
    }
  hDC->pen.color=hDC->foreColor;

//    if(m->command == SC_CLOSE && hWnd->class == CS_NOCLOSE) disabilitare.. forse fare in INITMENU e ovviamente non in ROM!
  if(m->flags & MF_BITMAP) {
//        DrawBitmap(hDC,x,cs.cy,m->bitmap);
    // state, grayed ecc...
    if(m->flags & MF_POPUP)
      drawCharWindow(hDC,cs.cx+14,y+1,'>');
    y+=MENU_HEIGHT;
    }
  else if(*m->text) {
    if(m->flags & MF_CHECKED) {
      drawLineWindow(hDC,x1+2,y+MENU_HEIGHT/2,x1+4,y+MENU_HEIGHT-2);
      drawLineWindow(hDC,x1+5,y+MENU_HEIGHT-2,x1+7,y+1);
      }
    rc.left=x1+(state & 2 ? 9 : 1); 
		rc.top=y+1;
    rc.right=x1+(state & 2 ? 9 : 1)+strlen(m->text)*6; 
		rc.bottom=y+1+MENU_HEIGHT;
    DrawText(hDC,m->text,strlen(m->text),&rc,DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS);
    // GESTIRE TAB!!
    
    hDC->font.strikethrough = 0;
    if(m->flags & MF_POPUP)
      drawCharWindow(hDC,x+cs.cx-6,y+1,'>');

//    if(menu->accelerators[i].key[0])      // test/debug (troppo piccoli gli schermi per un testo vero..
//      DrawCharWindow(hDC,x1+8,y+1,menu->accelerators[i].key[0]);

    y+=MENU_HEIGHT;
    }
  else if(m->flags & MF_SEPARATOR) {
    drawHorizLineWindow(hDC,x,y,x+cs.cx+2);
//      drawLineWindowColor(hDC,x1+3,y+MENU_HEIGHT/2,x1+xs-2,y+MENU_HEIGHT/2,itemsColor);
//    y+=1;
    }
  cs.cy=y-y1; //no cs.cx=x1-x;
  return cs;
  }  
static BOOL drawMenuPopup(HWND hWnd,HMENU menu,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE mode,int8_t selectedLine) {
  uint16_t x1;
  BYTE i,nmax;
  S_SIZE cs;
  DC myDC;
  HDC hDC;
  MENUITEM *m;
  HRGN rgn;
  
  nmax=getMenuSize(menu,&cs,1);
  selectedLine--;     //entra 0 per nessuna linea selezionata
  
  rgn=CreateRectRgn(hWnd->nonClientArea.left/*+BORDER_SIZE*/,hWnd->nonClientArea.top/*+BORDER_SIZE*/,
    hWnd->nonClientArea.right-BORDER_SIZE,hWnd->nonClientArea.bottom-BORDER_SIZE); // per non sporcare i bordi!
  hDC=GetDCEx(hWnd,&myDC,rgn,DCX_WINDOW | DCX_PARENTCLIP | DCX_INTERSECTRGN /*|DCX_LOCKWINDOWUPDATE*/);
  
  if(mode)
    cs.cx+=8;    // per check
  fillRectangleWindowColor(hDC,x,y,x+cs.cx+1,y+cs.cy-2,windowBackColor);
  drawRectangleWindow(hDC,x,y,x+cs.cx+2,y+cs.cy-1);
  
	y++;
  for(i=0; i<nmax; i++) { 
    x1=x;
    m=&menu->menuItems[i];
    cs=drawMenuitemPopup(hDC,m,x,y,cs,(mode ? 2 : 0) | (i==selectedLine ? 1 : 0));
    
//bah no    if(!cs.cx || !cs.cy)
//      break;
    y+=cs.cy;
//    if(y >= rc.bottom) bah cmq viene trimmata in video!
//      break;
    if(!m->bitmap && !m->command && !m->flags)
      break;
    
    hDC->pen.color=windowForeColor;    // 
    drawHorizLineWindow(hDC,x,y++,x+cs.cx+2);
    }
  
  ReleaseDC(hWnd,hDC);
  return 1;
  }

static uint16_t getMenuFromPoint(HWND hWnd,POINT pt,HMENU *inMenu) {
  RECT rc;
  BYTE i;
  HMENU menu;
  MENUITEM *m;

  i=0;

  menu=hWnd->menu;
  rc.left=hWnd->nonClientArea.left+BORDER_SIZE;
  rc.right=rc.left;
  rc.top=hWnd->nonClientArea.top+TITLE_HEIGHT+1;
  rc.bottom=rc.top+1+MENU_HEIGHT;
  
  do {
    m=&menu->menuItems[i];
    if(m && (m->bitmap || m->command || m->flags)) {
      if(m->flags & MF_BITMAP) {
        rc.right += 16+2;  //finire..
        }
      else if(*m->text) {
        rc.right += strlen(m->text)*6 +2;     // font size fisso...
        if(strchr(m->text,'&'))
          rc.right-=6;
        }
      else if(m->flags & MF_SEPARATOR) {
        rc.right +2;
        }
      
//        printf("cerco menu %u %u %u %u \r\n",rc.left,rc.top,rc.right,rc.bottom);
        
        
      if(wndPtInRect(&rc,pt)) {
        if(inMenu)
          *inMenu=(MENU*)m;
        
//        printf("trovo menu %x, command=%u, i=%u\r\n",m,m->command,i);
                
                
        return m->command;
        }
      }
    else
      break;
    
    rc.left = rc.right+2;
    rc.right = rc.left;

    i++;
    } while(m);

  if(inMenu)
    *inMenu=NULL;
    
  return 0;
  }

POINTS getMenuPosition(HMENU menuorig,HMENU menu) {
  POINTS pt;
  BYTE i;
  MENUITEM *m;
  FONT myFont=GetStockObject(SYSTEM_FONT).font;
  
  pt.y=0; // poi per popup...
	if(menuorig->menuItems[0].flags & MF_POPUP)
		;
  
  i=0;
  pt.x=0;

  do {
    m=&menuorig->menuItems[i];
//        printf("cerco menupos %x, %x, i=%u\r\n",menu2,m,i);
    if((HMENU)m->menu == menu)
      break;
    
    if(m && (m->bitmap || m->command || m->flags)) {
      if(m->flags & MF_BITMAP) {
        pt.x += 16 +   2;    // per vertical line
        }
      else if(m->flags & MF_SEPARATOR) {    // qua non ha senso, almeno per la menu bar!
        pt.x+=2;
        }
      else if(*m->text) {
        pt.x += getFontWidth(&myFont)*strlen((char *)m->text); 
        if(strchr((char *)m->text,'&'))
          pt.x-=getFontWidth(&myFont);
        pt.x+=2;
        }
      }
    else  // NON deve accadere!
      break;
    
    pt.x+=2;   // separatore
    
    i++;
    } while(m);
    
//        printf("trovo menupos pt=%u %u, i=%u\r\n",pt.x,pt.y,i);
        
  return pt;
  }

BYTE getMenuIndex(HMENU menu,HMENU menuorig) {
  BYTE i;
  MENUITEM *m;
  
	if(menuorig->menuItems[0].flags & MF_POPUP)
		;
  
  i=0;

  do {
    m=&menuorig->menuItems[i];
    if((HMENU)m == menu)
      break;
    
    i++;
    } while(m && (m->bitmap || m->command || m->flags));
    
  return i;
  }

BOOL DrawMenuBar(HWND hWnd) {
  BYTE i;
  GFX_COLOR itemsColor=hWnd->active ? windowForeColor : windowInactiveForeColor;
  
  if(hWnd->menu && !(hWnd->style & WS_CHILD)) {
    HDC hDC;
    DC myDC;
    hDC=GetWindowDC(hWnd,&myDC);
    
    fillRectangleWindowColor(hDC,BORDER_SIZE,TITLE_HEIGHT+BORDER_SIZE,
      hDC->area.right-hDC->area.left-BORDER_SIZE,
      TITLE_HEIGHT+MENU_HEIGHT+BORDER_SIZE-1,windowBackColor);
    // o metterla dentro sopra? tutti i parziali e poi il rimanente...
    
    ReleaseDC(hWnd,hDC);
    drawMenu(hWnd,hWnd->menu,BORDER_SIZE,TITLE_HEIGHT+BORDER_SIZE-1,hWnd->active,0);
//    drawHorizLineWindowColor(&hWnd->hDC,(hWnd->style & WS_THICKFRAME ? 2 : 1),TITLE_HEIGHT+MENU_HEIGHT+1,
//            hWnd->nonClientArea.right - hWnd->nonClientArea.left - (hWnd->style & WS_THICKFRAME ? 2 : 1),itemsColor);    // bah sì, cmq
  // forse servirebbe anche il rettangolone su tutta la barra... v. sopra cmq
  // e forse non serve la linea horiz perché fa già nonclientPaint, idem sopra
    }
  }

BOOL TrackPopupMenu(HMENU menu,UINT uFlags,UGRAPH_COORD_T x,UGRAPH_COORD_T y,
  int reserved,HWND hWnd,const RECT *prcRect) {
  S_SIZE cs;
  
  getMenuSize(menu,&cs,1);
  if(y+cs.cy > Screen.cy)
    y-=cs.cy+1;
    if(uFlags & TPM_TOPALIGN)
      ;
  if(x+cs.cx > Screen.cx)
    x-=cs.cx+1;
    if(uFlags & TPM_LEFTALIGN)
      ;
  drawMenuPopup(hWnd,menu,x,y,0,0);
  activeMenu=menu; activeMenuWnd=hWnd; activeMenuCntX=activeMenuCntY=0;
  activeMenuRect.left=x;
  activeMenuRect.right=activeMenuRect.left+cs.cx;
  activeMenuRect.top=y;
  activeMenuRect.bottom=activeMenuRect.top+cs.cy;
  SendMessage(hWnd,WM_INITMENUPOPUP,(WPARAM)menu,0);
  if(inDialogLoop()) {    // qua non dovrebbe servire... o forse ok
    MessageBeep(MB_ICONASTERISK);
    return 0;
    }
  SendMessage(hWnd,WM_ENTERMENULOOP,1,0);
//  SendMessage(hWnd,WM_ENTERIDLE,MSGF_MENU,(LPARAM)hWnd);  
  if(uFlags & TPM_RETURNCMD) {
    }
  return 1;
  }
BOOL TrackPopupMenuEx(HMENU menu,UINT uFlags,UGRAPH_COORD_T x,UGRAPH_COORD_T y,HWND hWnd,TPMPARAMS *lptpm) {
  S_SIZE cs;
  
  getMenuSize(menu,&cs,1);
  if(y+cs.cy > Screen.cy)
    y-=cs.cy+1;
    if(uFlags & TPM_TOPALIGN)
      ;
  if(x+cs.cx > Screen.cx)
    x-=cs.cx+1;
    if(uFlags & TPM_LEFTALIGN)
      ;
  drawMenuPopup(hWnd,menu,x,y,0,0);
  activeMenu=menu; activeMenuWnd=hWnd; activeMenuCntX=activeMenuCntY=0;
  activeMenuRect.left=x;
  activeMenuRect.top=y;
  activeMenuRect.right=activeMenuRect.left+cs.cx;
  activeMenuRect.bottom=activeMenuRect.top+cs.cy;
  SendMessage(hWnd,WM_INITMENUPOPUP,(WPARAM)menu,0);
  if(inDialogLoop()) {    // qua non dovrebbe servire... o forse ok
    MessageBeep(MB_ICONASTERISK);
    return 0;
    }
  SendMessage(hWnd,WM_ENTERMENULOOP,1,0);
//  SendMessage(hWnd,WM_ENTERIDLE,MSGF_MENU,(LPARAM)hWnd);  
  if(uFlags & TPM_RETURNCMD) {
    }
  return 1;
  }

static int matchAccelerator(HMENU menu,char ch,BYTE modifiers) {
  int i,j;
  BYTE myModifiers;
  
  myModifiers=(modifiers & 0b00001111) | (modifiers > 4);
  

  
  for(i=0; i<MAX_MENUITEMS; i++) {
    if(!menu->menuItems[i].bitmap && !menu->menuItems[i].command && !menu->menuItems[i].flags)
      break;
    if(menu->menuItems[i].menu /*implicito && menu->menuItems[i].flags & MF_POPUP*/) {
      if(j=matchAccelerator(menu->menuItems[i].menu,ch,modifiers))    // ricerca ricorsiva..
        return j;
      }
    else {
      if(menu->accelerators[i].key[0] == ch) {    // VK_ qualsiasi, no toupper
        if(!(menu->accelerators[i].key[1] ^ myModifiers)) { //
          return menu->menuItems[i].command;
          }
        }
      }
    }
  return 0;
  }

BOOL RegisterHotKey(HWND hWnd,uint16_t id,BYTE fsModifiers,BYTE vk) {
  //hotkeyTable[]...
  }


HDC GetWindowDC(HWND hWnd,HDC hDC) {    // io faccio così, per risparmiare memoria dinamica
  
  if(!hWnd)
    hWnd=desktopWindow;
  hDC->hWnd=hWnd;
  hDC->area=hWnd->nonClientArea;
  hWnd->paintArea=hDC->area;
  OffsetRect(&hWnd->paintArea,-hWnd->paintArea.left,-hWnd->paintArea.top);
  
// GetWindowDC assigns default attributes to the window device context each time it retrieves the device context. Previous attributes are lost.
  hDC->font=hWnd->font;
  hDC->pen.size=1; hDC->pen.style=PS_SOLID;
  hDC->foreColor=hDC->pen.color=windowForeColor;
  hDC->backColor=hDC->brush.color=windowBackColor;
  hDC->cursor.x=hDC->cursor.y=0;
  hDC->textCursor.x=hDC->textCursor.y=0;
  hDC->flags = (hWnd->style & (WS_CLIPCHILDREN | WS_CLIPSIBLINGS))>>25;
	hDC->isMemoryDC=0;
  hDC->mem=NULL;
  hDC->mapping=MM_TEXT;
  hDC->textAlignment=TA_LEFT | TA_TOP | TA_NOUPDATECP;
  hDC->rop=R2_COPYPEN;
  
  return hDC;
	}

HDC GetDC(HWND hWnd,HDC hDC) {
  
  if(!hWnd)
    hWnd=desktopWindow;
  hDC->hWnd=hWnd;
  
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  if(wc->style & CS_OWNDC) {   // fare.. un solo DC allocato per tutta la vita della window
    hDC=(HDC)GET_WINDOW_OFFSET(hWnd,0);
    }
  if(wc->style & CS_CLASSDC) {    // fare.. stesso DC per tutte le classi
    hDC=(HDC)GET_CLASS_OFFSET(hWnd,0);
    }
  
  if(wc->style & CS_PARENTDC) {   // 
    hDC->area=hWnd->parent->clientArea;
    }
  else {
    hDC->area=hWnd->clientArea;
    }
  hWnd->paintArea=hDC->area;
  OffsetRect(&hWnd->paintArea,-hWnd->paintArea.left,-hWnd->paintArea.top);
  
  hDC->pen.size=1; hDC->pen.style=PS_SOLID;
  hDC->brush=wc->hbrBackground;
  switch(hWnd->class.class) {
    case WC_STATIC:
    case WC_BUTTON:
    case WC_COMBOBOX:
    case WC_EDIT:
    case WC_LISTBOX:
    case WC_SCROLLBAR:
    case WC_SLIDER:
    case WC_UPDOWN:
    case WC_TRACKBAR:
    case WC_PROGRESS:
    case WC_DIALOG:
    case WC_FILEDLG: 
    case WC_VIEWERCLASS: 
    case WC_PLAYERCLASS: 
    case WC_TASKMANCLASS:
    case WC_CONTROLPANELCLASS:
      hDC->font=hWnd->font;
      hDC->foreColor=hDC->pen.color=WHITE;    // CONTRARIO di windows, almeno per ora :) e occhio minibasic con sfondo bianco..
      break;
    case WC_STATUSBAR:
      hDC->font=hWnd->font;
      hDC->foreColor=hDC->pen.color=BLUE;    // boh, tanto per
      break;
    default:
// boh ma è sbagliato...2023      hDC->font=GetStockObject(SYSTEM_FONT).font; /*cmq lo stock? o anche custom?? MSDN dice stock, system*/;
      hDC->font=hWnd->font;
      hDC->foreColor=hDC->pen.color=WHITE;    // CONTRARIO di windows, almeno per ora :) e occhio minibasic con sfondo bianco..
      break;
    }
  hDC->backColor=hDC->brush.color /*BLACK*/;
//  hDC->font=hWnd->font;
  hDC->cursor.x=hDC->cursor.y=0;
  hDC->textCursor.x=hDC->textCursor.y=0;
  hDC->flags = (hWnd->style & (WS_CLIPCHILDREN | WS_CLIPSIBLINGS))>>25;
	hDC->isMemoryDC=0;
  hDC->mem=NULL;
  hDC->mapping=MM_TEXT;
  hDC->textAlignment=TA_LEFT | TA_TOP | TA_NOUPDATECP;
  hDC->rop=R2_COPYPEN;
  
	return hDC;
	}

HDC GetDCEx(HWND hWnd,HDC hDC,HRGN hrgnClip,DWORD flags) {
  HDC mydc;
  
  if(flags & DCX_WINDOW)
    mydc=GetWindowDC(hWnd,hDC);
  else
    mydc=GetDC(hWnd,hDC); 
  if(flags & DCX_PARENTCLIP)
    hDC->flags &= ~(WS_CLIPCHILDREN>>25);
  if(flags & DCX_CLIPSIBLINGS)
    hDC->flags |= (WS_CLIPSIBLINGS>>25);
  if(flags & DCX_CLIPCHILDREN)
    hDC->flags |= (WS_CLIPCHILDREN>>25);
  if(flags & DCX_CACHE)
    ;
  if(flags & DCX_INTERSECTRGN)
    IntersectRect(&hDC->area,&hDC->area,hrgnClip);
  if(flags & DCX_EXCLUDERGN)
    ;
  if(flags & DCX_LOCKWINDOWUPDATE)
    hWnd->locked=FALSE;   // dovrei salvare? bah direi di no, in questo caso
// già sopra	hDC->flags &= ~1;
  return mydc;
  }

BOOL ReleaseDC(HWND hWnd,HDC hDC) {
  
/*  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  if(wc->style & CS_OWNDC) {   // Class and private DCs do not have to be released.
    }
  if(wc->style & CS_PARENTDC)    // 
    ;
  if(wc->style & CS_CLASSDC) {    //
    }*/
	if(hDC->isMemoryDC) {
    free(hDC->mem);
    hDC->mem=NULL;
    hDC->isMemoryDC=0;
    }
  return TRUE;
	}
HDC CreateCompatibleDC(HDC hDCOrig,HDC hDC) {
  
  *hDC=*hDCOrig;
  hDC->mem=malloc(((hDC->area.right-hDC->area.left)*(hDC->area.bottom-hDC->area.top)*_bpp)/8);  // boh, credo
	hDC->isMemoryDC=1;
  
  return hDC; ///:)
  }
BOOL RestoreDC(HDC hdc,int8_t nSavedDC) {
  }
int8_t SaveDC(HDC hDC) {
  }

GFX_COLOR GetDCBrushColor(HDC hDC) {
  return hDC->brush.color;
  }
GFX_COLOR SetDCBrushColor(HDC hDC,GFX_COLOR color) {
  GFX_COLOR c=hDC->brush.color;
  hDC->brush.color=color;
  return c;
  }
GFX_COLOR GetDCPenColor(HDC hDC) {
  return hDC->pen.color;
  }
GFX_COLOR SetDCPenColor(HDC hDC,GFX_COLOR color) {
  GFX_COLOR c=hDC->pen.color;
  hDC->pen.color=color;
  return c;
  }
BOOL GetDCOrgEx(HDC hDC,POINT *lppt) {
  HWND hWnd=WindowFromDC(hDC);
  lppt->x=hWnd->clientArea.left;
  lppt->y=hWnd->clientArea.top;
  if(hWnd->parent && hWnd->style & WS_CHILD) {
    HWND myWnd=hWnd->parent;
    while(myWnd) {
      lppt->x+=myWnd->clientArea.left;
      lppt->y+=myWnd->clientArea.top;
  //    OffsetRect(RECT *lprc,int16_t dx,int16_t dy);
      myWnd=myWnd->parent;
      }
    }
  return 1;
  }
BOOL SetViewportExtEx(HDC hdc,int x,int y,SIZE *lpsz) {
  
  if(lpsz) {
    }
  }
BOOL SetWindowExtEx(HDC hdc,int x,int y,SIZE *lpsz) {
  
  if(lpsz) {
    }
  }
BOOL SetWindowOrgEx(HDC hdc,int x,int y,POINT *lppt) {
  
  if(lppt) {
    }
  }
BYTE SetMapMode(HDC hDC,BYTE iMode) {
  BYTE i=hDC->mapping;
  hDC->mapping=iMode;
  return i;
  }

HDC BeginPaint(HWND hWnd,PAINTSTRUCT *lpPaint) {

  GetDC(hWnd,&lpPaint->DC);
  if(!IsRectEmpty(&hWnd->paintArea)) {
          //=lpPaint->hDC.area;
    lpPaint->rcPaint=hWnd->paintArea;
    lpPaint->fErase=!SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&lpPaint->DC,0);
// "BeginPaint sends a WM_ERASEBKGND message to the window". 
    }
  else {
    SetRectEmpty(&lpPaint->rcPaint);
    lpPaint->fErase=FALSE;
    }
//If the caret is in the area to be painted, BeginPaint automatically hides the caret to prevent it from being erased.
  HideCaret(hWnd);
  return &lpPaint->DC;
	}

BOOL EndPaint(HWND hWnd,const PAINTSTRUCT *lpPaint) {
//If the caret was hidden by BeginPaint, EndPaint restores the caret to the screen  
//??? tolgo per children 4/2/23  if(!lpPaint->fErase)
//    SetRectEmpty(&hWnd->paintArea);
// tuttavia "BeginPaint sets the update region of a window to NULL. This clears the region, preventing it from generating subsequent WM_PAINT messages"...
//  lpPaint->fErase=0; NO questa no, è const
  //ShowCaret
  ReleaseDC(hWnd,&(((PAINTSTRUCT *)lpPaint)->DC));
	}
BOOL LockWindowUpdate(HWND hWndLock) {  // non è esattamente pensata per questo, dice, ma ok v. SETREDRAW
  hWndLock->locked=1;
  }
BOOL EnumDisplayDevices(const char *lpDevice,BYTE iDevNum,DISPLAY_DEVICE *lpDisplayDevice,uint16_t dwFlags) {
  BYTE t=0;

  if(!lpDevice) {
    strcpy(lpDisplayDevice->DeviceString,"video...");   // gestire
    return 1;
    }
  if(!strcmp(lpDevice,"VGA"))   // finire..
    ;
  switch(iDevNum) {   // usare anche VideoMode...
    case 0:
      strcpy(lpDisplayDevice->DeviceName,"LCD");
      lpDisplayDevice->StateFlags=DISPLAY_DEVICE_PRIMARY_DEVICE;
      break;
    case 1:
      strcpy(lpDisplayDevice->DeviceName,"VGA");
      lpDisplayDevice->StateFlags=DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_VGA_COMPATIBLE;
      break;
    case 2:
      strcpy(lpDisplayDevice->DeviceName,"Composite");
      lpDisplayDevice->StateFlags=DISPLAY_DEVICE_PRIMARY_DEVICE;
      break;
    }
  //DISPLAY_DEVICE_MIRRORING_DRIVER  se VNC :)
  
  return t;
  }
LONG ChangeDisplaySettings(DEVMODE *lpDevMode, DWORD dwFlags) {
  BYTE t=DISP_CHANGE_BADMODE;

  if(lpDevMode->dmPelsWidth==160) {     // per ora!
    SetVideoMode(MODE_LCD,160,128,16);
    t=DISP_CHANGE_SUCCESSFUL;
    }
  if(lpDevMode->dmPelsWidth==320) {     // 
    if(lpDevMode->dmColor==256) {     // 
      SetVideoMode(MODE_VGA,320,240,8);
      t=DISP_CHANGE_SUCCESSFUL;
      }
    else {
      SetVideoMode(MODE_COMPOSITE,320,240,1);
      t=DISP_CHANGE_SUCCESSFUL;
      }
    }
  if(lpDevMode->dmPelsWidth==640) {     // 
    if(lpDevMode->dmColor==256) {     // 
      SetVideoMode(MODE_VGA,640,480,8);
      t=DISP_CHANGE_SUCCESSFUL;
      }
    else {
      SetVideoMode(MODE_COMPOSITE,640,480,1);
      t=DISP_CHANGE_SUCCESSFUL;
      }
    }
  if(lpDevMode->dmPelsWidth==480) {     // 
    SetVideoMode(/*MODE_LCD*/MODE_LCD_DMA,480,320,16);
    t=DISP_CHANGE_SUCCESSFUL;
    }
  if(lpDevMode->dmPelsWidth==800) {     // 
    SetVideoMode(MODE_LCD_DMA,800,480,8);
    t=DISP_CHANGE_SUCCESSFUL;
    }
  SendMessage(HWND_BROADCAST,WM_DISPLAYCHANGE,(WPARAM)_bpp,MAKELPARAM(Screen.cx,Screen.cy));
  return t;
  }
LONG ChangeDisplaySettingsEx(LPCSTR lpszDeviceName,DEVMODE *lpDevMode,
  HWND hWnd,DWORD dwFlags,void *lParam) {
  return ChangeDisplaySettings(lpDevMode, dwFlags);   // vaja con dios per ora :D
  }

// -------------------------------------------------------------------------------
WNDCLASS baseClass= {
  MAKECLASS(WC_DEFAULTCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS /*   | CS_DROPSHADOW*/,
  windowIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProc,
  NULL,
  0,
  0,
  NULL,
  {BS_SOLID,1,DARKGRAY}
  };
struct WNDCLASS_TOOLBAR {
  WNDCLASS baseClass;
  } toolbarClass = { {
  MAKECLASS(WC_TOOLBAR),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcToolbarWC,
  NULL,
  0,
  0,
  NULL,
  {BS_SOLID,1,DARKGRAY}
  }
  };
struct WNDCLASS_STATIC {
  WNDCLASS baseClass;
  } staticClass = { {
  MAKECLASS(WC_STATIC),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcStaticWC,
  NULL,
  0,
  0,    // [testo esteso, se si usa]
  NULL,
  {BS_SOLID,1,DARKGRAY}
  }
  };
struct WNDCLASS_BUTTON {
  WNDCLASS baseClass;
  } buttonClass = { {
  MAKECLASS(WC_BUTTON),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcButtonWC,
  NULL,
  0,
  0, // in USERDATA: BYTE [non usato, era stile];  BYTE state;	BYTE hotkey; sistemare ev.
  NULL,
  {BS_SOLID,1,GRAY204}
  }
  };
struct WNDCLASS_EDIT {
  WNDCLASS baseClass;
  } editClass = { {
  MAKECLASS(WC_EDIT),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcEditWC,
  NULL,
  0,
  64,  //	(in USERDATA BYTE caretPos; BYTE b0=caretState,b1=modified,b2=canundo;b3=readonly;	BYTE selStart,selEnd;)  BYTE content[64] opp buffer privato;
  NULL,
  {BS_SOLID,1,GRAY224}
  }
  };
struct WNDCLASS_LISTBOX {
  WNDCLASS baseClass;
  } listboxClass = { {
  MAKECLASS(WC_LISTBOX),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcListboxWC,
  NULL,
  0,
  4, //   BYTE *data; 	(in USERDATA WORD selectedLine, totLine;)
  NULL,
  {BS_SOLID,1,GRAY224}
  }   
  };
struct WNDCLASS_SCROLLBAR {
  WNDCLASS baseClass;
  } scrollBarClass = { {
  MAKECLASS(WC_SCROLLBAR),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcScrollBarWC,
  NULL,
  0,
  0, //   (in USERDATA WORD minrange, maxrange; scrollposXY è la Pos e scrollSizeXY è Page)
  NULL,
  {BS_SOLID,1,GRAY224}
  }   
  };
struct WNDCLASS_STATUSBAR {
  WNDCLASS baseClass;
  } statusBarClass = { {
  MAKECLASS(WC_STATUSBAR),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcStatusBar,
  NULL,
  0,
  0, //   (in USERDATA: 1 byte=2 x icone/testo extra, 2bit tipo 2bit stato); word alta=colore
  NULL,
  {BS_SOLID,1,GRAY224}
  }   
  };
struct WNDCLASS_PROGRESSBAR {
  WNDCLASS baseClass;
  } progressBarClass = { {
  MAKECLASS(WC_PROGRESS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcProgressWC,
  NULL,
  0,
  1+1+1+1, //   (in USERDATA: colore bar/resto) BYTE ?; BYTE min, max; BYTE value;
  NULL,
  {BS_SOLID,1,GRAY224}
  }   
  };
struct WNDCLASS_SPINCONTROL {
  WNDCLASS baseClass;
  } spinControlClass = { {
  MAKECLASS(WC_UPDOWN),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcSpinWC,
  NULL,
  0,
  0, //   (in USERDATA: BYTE step; BYTE min, max; BYTE value)
  NULL,
  {BS_SOLID,1,GRAY224}
  }   
  };
struct WNDCLASS_SLIDERCONTROL {
  WNDCLASS baseClass;
  } sliderControlClass = { {
  MAKECLASS(WC_SLIDER),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  NULL,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcSliderWC,
  NULL,
  0,
  0, //   (in USERDATA: BYTE step; BYTE min, max; BYTE value)
  NULL,
  {BS_SOLID,1,GRAY224}
  }   
  };
struct WNDCLASS_DIALOG {
  WNDCLASS baseClass;
  } dialogClass = { {
  MAKECLASS(WC_DIALOG),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  windowIcon,
  standardCursorSm,
  (WINDOWPROC *)DefDlgProc,
  NULL,
  0,
  DWL_USER,
  NULL,
  {BS_SOLID,1,GRAY224}
  }
  };
struct WNDCLASS_FILEBROWSER {
  WNDCLASS WNDCLASS_DIALOG;
  } fileDialogClass= { {
  MAKECLASS(WC_FILEDLG),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcFileDlgWC,
  (MENU*)NULL,
  0,
  DWL_USER+sizeof(OPENFILENAME),   //   OPENFILENAME ofn; in USERDATA+1 tipoVis (icone, dettagli); in USERDATA+2 lo stato del disco, DISK_PROPERTIES fatto ecc
  NULL,
  {BS_SOLID,1,LIGHTGRAY}
  }
  };
struct WNDCLASS_DIR {
  WNDCLASS baseClass;
  } diskExplorerClass= { {
  MAKECLASS(WC_DIRCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcDirWC,
  (MENU*)&explorerMenu,
  0,
  sizeof(DIRLIST)+4,   //  DIRLIST; lista file DISKITEM; (in USERDATA+1 tipoVis (icone, dettagli); in USERDATA+2 lo stato del disco, DISK_PROPERTIES fatto ecc
  NULL,
  {BS_SOLID,1,LIGHTGRAY}
  }
  };
struct WNDCLASS_CMDSHELL {
  WNDCLASS baseClass;
  } cmdShellClass= { {
  MAKECLASS(WC_CMDSHELL),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcCmdShellWC,
  (MENU*)&cmdShellMenu,
  0,
  1+32,   //   (in USERDATA x,y,coloref,coloreb) statocursore,commandline
  NULL,
  {BS_SOLID,1,BLACK}
  }
  };
struct WNDCLASS_VIEWER {
  WNDCLASS baseClass;
  } viewerClass= { {
  MAKECLASS(WC_VIEWERCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS | CS_DROPSHADOW,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)viewerWndProc,
  (MENU*)NULL,
  0,
  32,   //   (in USERDATA zoom) nomefile
  NULL,
  {BS_SOLID,1,BLACK}
  }
  };
struct WNDCLASS_PLAYER {
  WNDCLASS baseClass;
  } playerClass= { {
  MAKECLASS(WC_PLAYERCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS | CS_DROPSHADOW,
  audioIcon8,
  standardCursorSm,
  (WINDOWPROC *)playerWndProc,
  (MENU*)NULL,
  0,
  32,   //   (in USERDATA pos audio) nomefile
  NULL,
  {BS_SOLID,1,ORANGE}
  }
  };
struct WNDCLASS_DESKTOP {
  WNDCLASS baseClass;
  } desktopClass = { {
  MAKECLASS(WC_DESKTOPCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcDC,
  NULL,
  0,
  sizeof(POINTS)*16+16+16+4+4+1,    //  (in USERDATA colore f/b), POINTS iconPosition[16], file wallpaper, filescreensaver, puntatore jpeg, lungh. jpeg, attributi
// in 8 byte una union con 
//DWORD jpegLen;
// BYTE *jpegPtr;
// SUPERFILE *jpegFile;   // tutti dentro la classe desktopclass... 
  NULL,
  {BS_SOLID,1,DARKBLUE}
  }
  };
struct WNDCLASS_TASKBAR {
  WNDCLASS baseClass;
  } taskbarClass = { {
  MAKECLASS(WC_TASKBARCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcTaskBar,
  NULL,
  0,
  0,    //  (in USERDATA attributi,) 
  NULL,
  {BS_SOLID,1,LIGHTGRAY}
  }
  };
struct WNDCLASS_TASKMAN {
  WNDCLASS baseClass;
  } taskmanagerClass = { {
  MAKECLASS(WC_TASKMANCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcTaskManager,
  NULL,
  0,
  0,
  NULL,
  {BS_SOLID,1,LIGHTGRAY}
  }
  };
struct WNDCLASS_CONTROLPANEL {
  WNDCLASS baseClass;
  } controlPanelClass = { {
  MAKECLASS(WC_CONTROLPANELCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcControlPanel,
  NULL,
  0,
  0,    //  (in USERDATA quale gruppo opp. 0; totale elementi per quel gruppo
  NULL,
  {BS_SOLID,1,LIGHTGRAY}
  }
  };
struct WNDCLASS_VIDEOCAPCLASS {
  WNDCLASS videoCapClass;
  } videoCapClass = { {
  MAKECLASS(WC_VIDEOCAPCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursorSm,
  (WINDOWPROC *)DefWindowProcVideoCap,
  NULL,
  0,
  4,    //  diciamo settaggi video UVC webcam
  NULL,
  {BS_SOLID,1,BLACK}
  }
  };
  
  
BOOL GetClassInfo(HINSTANCE hInstance, CLASS Class, WNDCLASS **lpWndClass) {
  
  switch(Class.class) {
    case WC_STATIC:
      *lpWndClass=(WNDCLASS *)&staticClass;
      break;
    case WC_TOOLBAR:
      *lpWndClass=(WNDCLASS *)&toolbarClass;
      break;
    case WC_BUTTON:
      *lpWndClass=(WNDCLASS *)&buttonClass;
      break;
    case WC_COMBOBOX:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_EDIT:
      *lpWndClass=(WNDCLASS *)&editClass;
      break;
    case WC_DIALOG:
      *lpWndClass=(WNDCLASS *)&dialogClass;
      break;
    case WC_LISTBOX:
      *lpWndClass=(WNDCLASS *)&listboxClass;
      break;
    case WC_SCROLLBAR:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_HOTKEY:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_PROGRESS:
      *lpWndClass=(WNDCLASS *)&progressBarClass;
      break;
    case WC_STATUSBAR:
      *lpWndClass=(WNDCLASS *)&statusBarClass;
      break;
    case WC_TRACKBAR:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_UPDOWN:
      *lpWndClass=(WNDCLASS *)&spinControlClass;
      break;
    case WC_SLIDER:
      *lpWndClass=(WNDCLASS *)&sliderControlClass;
      break;
    case WC_FILEDLG:
      *lpWndClass=(WNDCLASS *)&fileDialogClass;
      break;
    case WC_DIRCLASS:
      *lpWndClass=(WNDCLASS *)&diskExplorerClass;
      break;

    case WC_CMDSHELL:
      *lpWndClass=(WNDCLASS *)&cmdShellClass;
      break;
    case WC_DESKTOPCLASS:
      *lpWndClass=(WNDCLASS *)&desktopClass;
      break;
    case WC_TASKBARCLASS:
      *lpWndClass=(WNDCLASS *)&taskbarClass;
      break;
    case WC_CONTROLPANELCLASS:
      *lpWndClass=(WNDCLASS *)&controlPanelClass;
      break;
    case WC_TASKMANCLASS:
      *lpWndClass=(WNDCLASS *)&taskmanagerClass;
      break;
    case WC_VIEWERCLASS:
      *lpWndClass=(WNDCLASS *)&viewerClass;
      break;
    case WC_PLAYERCLASS:
      *lpWndClass=(WNDCLASS *)&playerClass;
      break;

    case WC_VIDEOCAPCLASS:
      *lpWndClass=(WNDCLASS *)&videoCapClass;
      break;
      
    case WC_DEFAULTCLASS:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
      
    default:
    {
      WNDCLASS *myClass=rootClasses;
      while(myClass) {
        if(myClass->class.class==Class.class) {
          *lpWndClass=myClass;
          return TRUE;
          break;
          }
        myClass=myClass->next;
        }
      return FALSE;
    }

      break;
    }
  return TRUE;
  }


SHORT GetKeyState(int nVirtKey) {
  
	switch(nVirtKey) {
		case VK_NUMLOCK:
      return (internalKeyboardBuffer[nVirtKey] ? 0x8000 : 0x0000)
        | (ledStatus & 0b00000001 ? 1 : 0);		// 
			break;
		case VK_CAPITAL:
      return (internalKeyboardBuffer[nVirtKey] ? 0x8000 : 0x0000)
        | (ledStatus & 0b00000010 ? 1 : 0);		// 
			break;
		case VK_SCROLL:
      return (internalKeyboardBuffer[nVirtKey] ? 0x8000 : 0x0000)
        | (ledStatus & 0b00000100 ? 1 : 0);		// 
			break;
		case VK_LBUTTON:
      return mouse.buttons & 1;   // salvare, questo è async
			break;
		case VK_RBUTTON:
      return mouse.buttons & 2;   // salvare, questo è async
			break;
		default:
      return internalKeyboardBuffer[nVirtKey] ? 0x8000 : 0x0000;		// finire 
			break;
    }
	}
SHORT GetAsyncKeyState(int vKey) {

	switch(vKey) {
		case VK_CONTROL:
			if(keypress.modifier & 0b00010001
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_LCONTROL:
			if(keypress.modifier & 0b00000001
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_RCONTROL:
			if(keypress.modifier & 0b00010000
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_MENU:
			if(keypress.modifier & 0b01000100
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_LMENU:
			if(keypress.modifier & 0b00000100
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_RMENU:
			if(keypress.modifier & 0b01000000
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_SHIFT:
			if(keypress.modifier & 0b00100010
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_LSHIFT:
			if(keypress.modifier & 0b00000010
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_RSHIFT:
			if(keypress.modifier & 0b00100000
        && !(keypress.state & 0b10000000))
				return 0x8001;		// finire
			break;
		case VK_RETURN:
			if(keypress.key == '\n')
				return 0x8001;		// finire
			break;
		case VK_NUMLOCK:
      return (internalKeyboardBuffer[vKey] ? 0x8000 : 0x0000)
        | (ledStatus & 0b00000001 ? 1 : 0);		// 
			break;
		case VK_CAPITAL:
      return (internalKeyboardBuffer[vKey] ? 0x8000 : 0x0000)
        | (ledStatus & 0b00000010 ? 1 : 0);		// 
			break;
		case VK_SCROLL:
      return (internalKeyboardBuffer[vKey] ? 0x8000 : 0x0000)
        | (ledStatus & 0b00000100 ? 1 : 0);		// 
			break;
		case VK_LBUTTON:
      return mouse.buttons & 1;   
			break;
		case VK_RBUTTON:
      return mouse.buttons & 2;
			break;
		default:
      return internalKeyboardBuffer[vKey] ? 0x8000 : 0x0000;		// finire con "stato precedente"
			break;
		}
	return 0;
  }

SHORT VkKeyScan(char ch) {
  int i;


  for(i=0; i<256; i++) {
    if(ch==MapVirtualKey(i,MAPVK_VK_TO_CHAR)) {   // bah ma sì :D
      if(keypress.modifier & 0b00010001)    // forse usare tabella stati e non async?
        i |= 0b0000001000000000;
      if(keypress.modifier & 0b00010010)
        i |= 0b0000000100000000;
      if(keypress.modifier & 0b01000100)
        i |= 0b0000010000000000;
      return i;
      }
    }
  return -1;
  }
SHORT VkKeyScanEx(char ch,DWORD /*HKL*/ dwhkl) {
  return VkKeyScan(ch);
  }
UINT MapVirtualKey(UINT uCode,BYTE uMapType) {
  UINT ch=0;
  //https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

  switch(uMapType) {
    case MAPVK_VK_TO_VSC:
      switch(uCode) {
        case VK_UP: // up, 
          ch=0x94;
          break;
        case VK_DOWN: // down
          ch=0x93;
          break;
        case VK_LEFT: // left
          ch=0x92;
          break;
        case VK_RIGHT: // right
          ch=0x91;
          break;
        case VK_PRIOR: // pgup
          ch=0x96;
          break;
        case VK_NEXT: // pgdn
          ch=0x95;
          break;
        case VK_HOME: // home
          ch=0x97;
          break;
        case VK_END: // end
          ch=0x98;
          break;
        case VK_INSERT:
          ch=0x99;
          break;
        case VK_DELETE:
          ch=0x9a;
          break;
        case VK_SNAPSHOT:
          ch=0xB8;
          break;
        case VK_CANCEL:
        case VK_PAUSE:
          ch=0x9f;
          break;
        case VK_APPS:
          ch=0x9e;
          break;
        case VK_F1:
        case VK_F2:
        case VK_F3:
        case VK_F4:
        case VK_F5:
        case VK_F6:
        case VK_F7:
        case VK_F8:
        case VK_F9:
        case VK_F10:
        case VK_F11:
        case VK_F12:
          ch=uCode-VK_F1+0xa0;
          break;
        case VK_RETURN:   // 
        //0xa:		// serve/iva... :)
          ch=0xd;
          break;
        case VK_NUMLOCK:
          ch=0xb1;
          break;
        case VK_CAPITAL:
          ch=0xb2;
          break;
        case VK_SCROLL:
          ch=0xb3;
          break;
        case VK_MEDIA_NEXT_TRACK:
          ch=0xc1;
          break;
        case VK_MEDIA_PREV_TRACK:
          ch=0xc2;
          break;
        case VK_MEDIA_STOP:
          ch=0xc3;
          break;
        case VK_MEDIA_PLAY_PAUSE:
          ch=0xc4;
          break;
        case VK_VOLUME_MUTE:
          ch=0xc5;
          break;
        case VK_OEM_1:
          ch=0xc6;			// qua abbiamo bass/loudness/treble su usb... ma in windows non li vedo...
          break;
        case VK_OEM_2:
          ch=0xc7;
          break;
        case VK_VOLUME_UP:
          ch=0xc8;
          break;
        case VK_VOLUME_DOWN:
          ch=0xc9;
          break;
        case VK_OEM_3:
          ch=0xca;
          break;
        case VK_OEM_4:
          ch=0xcb;
          break;
        case VK_OEM_5:
          ch=0xcc;
          break;
        case VK_OEM_6:
          ch=0xcd;
          break;
        case VK_LAUNCH_MEDIA_SELECT:
          ch=0xd1;
          break;
        case VK_LAUNCH_MAIL:
          ch=0xd2;
          break;
        case VK_LAUNCH_APP1:
          ch=0xd3;			// calculator...
          break;
        case VK_LAUNCH_APP2:
          ch=0xd4;			// my computer...
          break;
        case VK_BROWSER_SEARCH:
          ch=0xd5;
          break;
        case VK_BROWSER_HOME:
          ch=0xd6;
          break;
        case 0xd7:
          ch=VK_BROWSER_BACK;
          break;
        case VK_BROWSER_FORWARD:
          ch=0xd8;
          break;
        case VK_SLEEP:
          ch=0xba;
          break;
        case VK_ADD:
         ch='+';
         break;
       case VK_SUBTRACT:
         ch='-';
         break;
       case VK_MULTIPLY:
         ch='*';
         break;
       case VK_DIVIDE:
         ch='/';
         break;
       case VK_DECIMAL:
         ch='.';
         break;
       case VK_SEPARATOR:
         ch=',';
         break;
       default:
          ch=uCode;   // bah per ora ok! ossia lettere numeri ecc e simboli
          break;
        }
      break;
    case MAPVK_VSC_TO_VK:
      switch(uCode) {
        case 0x94: // up, 
          ch=VK_UP;
          break;
        case 0x93: // down
          ch=VK_DOWN;
          break;
        case 0x92: // left
          ch=VK_LEFT;
          break;
        case 0x91: // right
          ch=VK_RIGHT;
          break;
        case 0x96: // pgup
          ch=VK_PRIOR;
          break;
        case 0x95: // pgdn
          ch=VK_NEXT;
          break;
        case 0x97: // home
          ch=VK_HOME;
          break;
        case 0x98: // end
          ch=VK_END;
          break;
        case 0x99:
          ch=VK_INSERT;
          break;
        case 0x9a:
          ch=VK_DELETE;
          break;
        case 0xB8:
          ch=VK_SNAPSHOT;
          break;
        case 0x9f:
          if(keypress.modifier & 0b00010001)
            ch=VK_CANCEL;
          else
            ch=VK_PAUSE;
          break;
        case 0x9e:
          ch=VK_APPS;
          break;
        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
        case 0xa4:
        case 0xa5:
        case 0xa6:
        case 0xa7:
        case 0xa8:
        case 0xa9:
        case 0xaa:
        case 0xab:
          ch=uCode-0xa1+VK_F1;
          break;
        case 0xd:   // 
        case 0xa:		// serve... :)
          ch=VK_RETURN;   //sopra diceva che son diversi ma dalle prove non sembra if(keypress.modifier & 0b00100010)
          break;
        case 0xb1:
          ch=VK_NUMLOCK;
          break;
        case 0xb2:
          ch=VK_CAPITAL;
          break;
        case 0xb3:
          ch=VK_SCROLL;
          break;
        case 0xc1:
          ch=VK_MEDIA_NEXT_TRACK;
          break;
        case 0xc2:
          ch=VK_MEDIA_PREV_TRACK;
          break;
        case 0xc3:
          ch=VK_MEDIA_STOP;
          break;
        case 0xc4:
          ch=VK_MEDIA_PLAY_PAUSE;
          break;
        case 0xc5:
          ch=VK_VOLUME_MUTE;
          break;
        case 0xc6:
          ch=VK_OEM_1;			// qua abbiamo bass/loudness/treble su usb... ma in windows non li vedo...
          break;
        case 0xc7:
          ch=VK_OEM_2;
          break;
        case 0xc8:
          ch=VK_VOLUME_UP;
          break;
        case 0xc9:
          ch=VK_VOLUME_DOWN;
          break;
        case 0xca:
          ch=VK_OEM_3;
          break;
        case 0xcb:
          ch=VK_OEM_4;
          break;
        case 0xcc:
          ch=VK_OEM_5;
          break;
        case 0xcd:
          ch=VK_OEM_6;
          break;
        case 0xd1:
          ch=VK_LAUNCH_MEDIA_SELECT;
          break;
        case 0xd2:
          ch=VK_LAUNCH_MAIL;
          break;
        case 0xd3:
          ch=VK_LAUNCH_APP1;			// calculator...
          break;
        case 0xd4:
          ch=VK_LAUNCH_APP2;			// my computer...
          break;
        case 0xd5:
          ch=VK_BROWSER_SEARCH;
          break;
        case 0xd6:
          ch=VK_BROWSER_HOME;
          break;
        case 0xd7:
          ch=VK_BROWSER_BACK;
          break;
        case 0xd8:
          ch=VK_BROWSER_FORWARD;
          break;
        case 0xba:
          ch=VK_SLEEP;
          break;
        case 0xb9:		// tolgo tasti speciali
        case 0xbb:
          ch=0;   // faccio così
          break;
        case '+':
          ch=VK_ADD;
          break;
        case '-':
          ch=VK_SUBTRACT;
          break;
        case '*':
          ch=VK_MULTIPLY;
          break;
        case '/':
          ch=VK_DIVIDE;
          break;
        case '.':
          ch=VK_DECIMAL;
          break;
        case ',':
          ch=VK_SEPARATOR;
          break;
        default:
          if(uCode) {
            if(isalpha(uCode))
              ch=uCode-'A'+VK_A;
            else if(isdigit(uCode))
              ch=uCode-'0'+VK_0;
//              ch=uCode-'0'+VK_NUMPAD0;    // non distinguo...
            else 
              ch=uCode;   // tutti i simboli... boh
            }
          else {
            ch=0;
            if(keypress.modifier & 0b00000001)
              ch=VK_LCONTROL;
            if(keypress.modifier & 0b00000010)
              ch=VK_LSHIFT;
            if(keypress.modifier & 0b00000100)
              ch=VK_LMENU;
            if(keypress.modifier & 0b00001000)
              ch=VK_LWIN;
            if(keypress.modifier & 0b00010000)
              ch=VK_RCONTROL;
            if(keypress.modifier & 0b00100000)
              ch=VK_RSHIFT;
            if(keypress.modifier & 0b01000000)
              ch=VK_RMENU;
            if(keypress.modifier & 0b10000000)    // prende l'ultimo...
              ch=VK_RWIN;
            }
          break;
        }
      break;
    case MAPVK_VK_TO_CHAR:
      if(GetAsyncKeyState(VK_RMENU)) {    // ALT-gr
        // e che cazzo :D faccio così
        switch(uCode) {
          case ';':   // tast americana
            ch='#';
            break;
          case '\'':
            ch='@';
            break;
          case '#':
            ch='~';
            break;
          case 'e':
            ch='E';   // ma euro :D
            break;
          default:
            ch=0;
            break;
          }
        }
      else {
        switch(uCode) {
          case VK_SPACE:
            ch=' ';
            break;
          case VK_BACK:
            ch='\x8';
            break;
          case VK_RETURN: 
            ch='\r';    // forse poi mettere \r, rifare south bridge
            break;
          case VK_ESCAPE:
            ch='\x1b';
            break;
          case VK_TAB:
            ch='\x9';
            break;
          case VK_ADD:
            ch='+';
            break;
          case VK_SUBTRACT:
            ch='-';
            break;
          case VK_MULTIPLY:
            ch='*';
            break;
          case VK_DIVIDE:
            ch='/';
            break;
          case VK_DECIMAL:
            ch='.';
            break;
          case VK_SEPARATOR:
            ch=',';
            break;
          default:
            if(uCode>=VK_A && uCode<=VK_Z) {
              ch=uCode-VK_A+'A';
      // forse usare tabella stati e non async?
              if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                ch &= 0x1f;
              else if(!GetAsyncKeyState(VK_SHIFT) & 0x8000)    // 
                ch=tolower(ch);
              }
            else if(uCode>=VK_0 && uCode<=VK_9)
              ch=uCode-VK_0+'0';
            else if(uCode>=VK_NUMPAD0 && uCode<=VK_NUMPAD9)
              ch=uCode-VK_NUMPAD0+'0';
            else if((uCode>=0x3a && uCode<=0x40) || (uCode>=0x5b && uCode<=0x60) || (uCode>=0x7b && uCode<0x80))   // tutti i simboli ecc... come arrivano da south bridge
              return uCode;
            else
              ch=0;
            break;
          }
        }
      break;
    case MAPVK_VSC_TO_VK_EX:
      break;
    case MAPVK_VK_TO_VSC_EX:
      break;
    }
  return ch;
  }
BOOL SetKeyboardState(BYTE *lpKeyState) {
  memcpy(internalKeyboardBuffer,lpKeyState,256);
  return 1;
  }
BOOL GetKeyboardState(BYTE *lpKeyState) {
  memcpy(lpKeyState,internalKeyboardBuffer,256);
  return 1;
  }

  
// -------------------------------------------------------------------------------
#if 0
void SortLinkedListValue(struct _WINDOW *node) {   //https://stackoverflow.com/questions/35914574/sorting-linked-list-simplest-way
  struct _WINDOW *temp;
  int tempvar,j;  //temp variable to store node data
    
  temp = node->next;//temp node to hold node data and next link
  while(node && node->next) {
    for(j=0; j<maxWindows; j++) {   //[value 5 because I am taking only 5 nodes]
      if(node->zOrder > temp->zOrder) {   //swap node data
        tempvar = node->zOrder;
        node->zOrder= temp->zOrder;
        temp->zOrder = tempvar;
        }
      temp = temp->next;
      }
    node = node->next;
    }
  }
 
static void SortLinkedList(struct _WINDOW *head) {     // https://www.javatpoint.com/program-to-sort-the-elements-of-the-singly-linked-list
  struct _WINDOW *node = head, *i, *j;
  struct _WINDOW *temp;
  
#if 0
  //Node current will point to head  
  struct _WINDOW *current = head, *index = NULL;  
  int temp;  

  if(!head) {  
    return;  
    } 
  else {  
    while(current) {  
      //Node index will point to node next to current  
      index = current->next;  

      while(index ) {  
        //If current node's data is greater than index's node data, swap the data between them  
        if(current->zOrder > index->zOrder) {  
          temp = current->zOrder;  
          current->zOrder = index->zOrder;  
          index->zOrder = temp;  
          }  
        index = index->next;  
        }  
      current = current->next;  
      }      
    }  
#endif
  
  // https://stackoverflow.com/questions/8981823/bubble-sort-algorithm-for-a-linked-list
  i = node;
  while(i) {
    j = node->next;
    while(j) {
      if(i->zOrder > j->zOrder) {
        temp = i->next;
        i->next = j->next;
        j->next = temp;
        }
      j = j->next;
      }
    i = i->next;
    }
  }
#endif

void list_bubble_sort(struct _WINDOW **head) {    //https://stackoverflow.com/questions/21388916/bubble-sort-singly-linked-list-in-c-with-pointers
  int done = 0;         // True if no swaps were made in a pass

  // Don't try to sort empty or single-node lists
  if(!*head || !((*head)->next))
    return;

  while(!done) {
    struct _WINDOW **pv = head;         // "source" of the pointer to the current node in the list struct
    struct _WINDOW *nd = *head;            // local iterator pointer
    struct _WINDOW *nx = (*head)->next;  // local next pointer

    done = 1;

    while(nx) {
      if(nd->zOrder > nx->zOrder) {   // ordino da più basso a più alto (v. anche ZCheckWindow e HWND_TOP ecc)
        nd->next = nx->next;
        nx->next = nd;
        *pv = nx;
        done = 0;
        }
      pv = &nd->next;
      nd = nx;
      nx = nx->next;
      }
    }
  }

void handleWinTimers(void) {
  int i;

#ifdef TEST_THREADS
  while(1) {
  timerData[0].time_cnt--;  // porcodio di debugger se non si ferma
//ThreadSleep(5);  
    Yield();      
    SetEvent(GetCurrentThread(),2);
  }
//  GetCurrentThread()->env[ENV_POSITION_RETURNCODE]=33;
  ExitThread(33);
  return;
#endif
  
  for(i=0; i<MAX_TIMERS; i++) {
    if(timerData[i].uEvent) {
      if(timerData[i].time_cnt) {
        timerData[i].time_cnt--;
        if(!timerData[i].time_cnt) {
//      LED1^=1;
//      printf("timer %x,%u,%u\r\n",timerData[i].hWnd,timerData[i].uEvent,timerData[i].timeout);
          if(timerData[i].hWnd)
            SendMessage(timerData[i].hWnd,WM_TIMER,timerData[i].uEvent,(LPARAM)timerData[i].tproc);
#warning sarebbe Post
          else if(timerData[i].tproc)
            timerData[i].tproc(timerData[i].hWnd,0,0,0);
          else
            goto skip;
          timerData[i].time_cnt=timerData[i].timeout;
skip: ;
          }
        }
      }
    }
  }

BYTE handle_filechanges(char drive,BYTE mode) {
  HWND hwnd=rootWindows;
  BYTE i=0;
  
  while(hwnd) {
    if(hwnd->class.class==WC_DIRCLASS) {
      SendMessage(hwnd,WM_FILECHANGE,drive,MAKELONG(mode,0)); // qua tutti...
      i=1;
      }
    hwnd=hwnd->next;
    }
  return i;
  }

BYTE handle_console_output(char ch,BYTE m) {
  HWND hwnd=rootWindows;
  BYTE i=0;
  
  while(hwnd) {
    if(hwnd->class.class==WC_CMDSHELL) {
      SendMessage(hwnd,WM_PRINTCHAR,ch,m ? MAKELONG(LIGHTRED,0) : -1);
      i=1;
      break;      // direi SOLO UNA! e magari scegliere quale, in caso...
      }
    hwnd=hwnd->next;
    }
  return i;
  }

/*static*/ uint8_t initADC(void) {
// ovviamente verificare con INADC del minibasic...
  
  ADCCON1=0;    // AICPMPEN=0, siamo sopra 2.5V
  CFGCONbits.IOANCPEN=0;    // idem
  ADCCON2=0;
  ADCCON3=0;
  
  //Configure Analog Ports
  ADCCON3bits.VREFSEL = 0; //Set Vref to VREF+/-

  ADCCMPEN1=0x00000000;
  ADCCMPEN2=0x00000000;
  ADCCMPEN3=0x00000000;
  ADCCMPEN4=0x00000000;
  ADCCMPEN5=0x00000000;
  ADCCMPEN6=0x00000000;
  ADCFLTR1=0x00000000;
  ADCFLTR2=0x00000000;
  ADCFLTR3=0x00000000;
  ADCFLTR4=0x00000000;
  ADCFLTR5=0x00000000;
  ADCFLTR6=0x00000000;
  
  ADCFSTAT=0;
  
  ADCTRGMODE=0;
  ADCTRGSNS=0;

  ADCTRG1=0;
  ADCTRG2=0;
  ADCTRG3=0;
  ADCTRGSNSbits.LVL3 = 0; // Edge trigger
  ADCTRGSNSbits.LVL4 = 0; 
  ADCTRG1bits.TRGSRC3 = 0b00001; // Set AN3 to trigger from software
  ADCTRG2bits.TRGSRC4 = 0b00001; // Set AN4 to trigger from software
  
  // I PRIMI 12 POSSONO OVVERO DEVONO USARE gli ADC dedicati! e anche se si usano
  // poi gli SCAN, per quelli >12, bisogna usarli entrambi (e quindi TRGSRC passa a STRIG ossia "common")

  ADCIMCON1bits.DIFF3 = 0; // single ended, unsigned
  ADCIMCON1bits.SIGN3 = 0; // 
  ADCIMCON1bits.DIFF4 = 0;
  ADCIMCON1bits.SIGN4 = 0;
   
  // Initialize warm up time register
  ADCANCON = 0;
  ADCANCONbits.WKUPCLKCNT = 5; // Wakeup exponent = 32 * TADx

  ADCEIEN1 = 0;
    
  ADCCON2bits.ADCDIV = 64; // per SHARED: 2 TQ * (ADCDIV<6:0>) = 64 * TQ = TAD
  ADCCON2bits.SAMC = 5;
    
  ADCCON3bits.ADCSEL = 0;   //0=periph clock 3; 1=SYSCLK
  ADCCON3bits.CONCLKDIV = 4; // 25MHz, sotto è poi diviso 2 per il canale, = max 50MHz come da doc

  ADC3TIMEbits.SELRES=0b10;        // 10 bits
  ADC3TIMEbits.ADCDIV=4;       // 
  ADC3TIMEbits.SAMC=5;        // 
  ADC4TIMEbits.SELRES=0b10;        // 10 bits
  ADC4TIMEbits.ADCDIV=4;       // 
  ADC4TIMEbits.SAMC=5;        //   
  
  ADCCSS1 = 0; // Clear all bits
  ADCCSS2 = 0;

  ADC0CFG=DEVADC0;
  ADC1CFG=DEVADC1;
  ADC2CFG=DEVADC2;
  ADC3CFG=DEVADC3;
  ADC4CFG=DEVADC4;
  ADC7CFG=DEVADC7;

  ADCCON1bits.ON = 1;   //Enable AD
  ClrWdt();
  
  // Wait for voltage reference to be stable 
#ifndef USING_SIMULATOR
  while(!ADCCON2bits.BGVRRDY); // Wait until the reference voltage is ready
  //while(ADCCON2bits.REFFLT); // Wait if there is a fault with the reference voltage
#endif

  // Enable clock to the module.
  
  ADCANCONbits.ANEN3 = 1;
  ADCCON3bits.DIGEN3 = 1;
  ADCANCONbits.ANEN4 = 1;
  ADCCON3bits.DIGEN4 = 1;
#ifndef USING_SIMULATOR
  while(!ADCANCONbits.WKRDY3); // Wait until ADC is ready
  while(!ADCANCONbits.WKRDY4); // Wait until ADC is ready
#endif
  
  // ADCGIRQEN1bits.AGIEN4=1;     // IRQ (anche ev. per DMA))

  return 1;
	}

static uint16_t readADC(uint8_t channel) {

//	__delay_us(50);
  ANSELBbits.ANSB3 = 0;
  ANSELBbits.ANSB4 = 0;
  switch(channel) {
    case 0:   // B3
      ANSELBbits.ANSB3 = 1;
//      ADCCON3bits.ADINSEL = 2;
      break;
    case 1:   // B4
      ANSELBbits.ANSB4 = 1;
//      ADCCON3bits.ADINSEL = 4;
      break;
    }
	__delay_us(30);

  ADCCON3bits.GSWTRG = 1; // Start software trigger

  switch(channel) {
    case 0:   // B3
      while(ADCDSTAT1bits.ARDY3 == 0) // Wait until the measurement run
        ClrWdt();
      ANSELBbits.ANSB3 = 0;
      ANSELBbits.ANSB4 = 0;
      ADCDATA4;
      // PARE che quando mandi il trigger, lui converte TUTTI i canali abilitati,
      // per cui se non pulisco "l'altro" mi becco un RDY e una lettura precedente...
// forse COSI' funzinerebbe, PROVARE      while(ADCCON2bits.EOSRDY == 0) // Wait until the measurement run
//        ClrWdt();
      return ADCDATA3 >> 2;
      break;
    case 1:   // B4
      while(ADCDSTAT1bits.ARDY4 == 0) // Wait until the measurement run
        ClrWdt();
      ANSELBbits.ANSB3 = 0;
      ANSELBbits.ANSB4 = 0;
      ADCDATA3;
      return ADCDATA4 >> 2;
      break;
    }

	}

// il PORCO e il DIO non è così, madonna di merda!!
//#define YP B3 //A2  
//#define XM B2 //A3 
//#define YM E6 //8   
//#define XP E7 //9   
// https://forum.arduino.cc/t/getting-bmps-to-work-on-3-1-2-touch-screen/691080
#define YP B4 //A1
#define XM B3 //A2 
#define XP F5 //6  
#define YM E5 //7   
#define RPLATE 300
#define NUMSAMPLES 2
#define SAMPLE_NOISE 6
BYTE manageTouchScreen(UGRAPH_COORD_T *x,UGRAPH_COORD_T *y,uint8_t *z) {
  int samples[NUMSAMPLES];
  uint8_t i, valid;

  valid = 1;

  TRISBbits.TRISB4=1;    //(_yp, INPUT);
  TRISEbits.TRISE5=1;    //(_ym, INPUT);
  TRISFbits.TRISF5=0;    //(_xp, OUTPUT);
  TRISBbits.TRISB3=0;    //(_xm, OUTPUT);

  LATFbits.LATF5=1;      //(_xp, HIGH);
  LATBbits.LATB3=0;      //(_xm, LOW);

  __delay_us(30); // Fast ARM chips need to allow voltages to settle

  for(i=0; i<NUMSAMPLES; i++) {
    samples[i] = readADC(1 /*_yp*/);
    }

  TRISBbits.TRISB4=0;    //(_yp, OUTPUT);
  TRISEbits.TRISE5=0;    //(_ym, OUTPUT);
  TRISFbits.TRISF5=1;    //(_xp, INPUT);
  TRISBbits.TRISB3=1;    //(_xm, INPUT);

  LATEbits.LATE5=1;      //(_ym, LOW);
  LATBbits.LATB4=0;      //(_yp, HIGH);
  
  
#if NUMSAMPLES > 2
   insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
   // Allow small amount of measurement noise, because capacitive
   // coupling to a TFT display's signals can induce some noise.
  if(((samples[0] - samples[1]) < -SAMPLE_NOISE) || ((samples[0] - samples[1]) > SAMPLE_NOISE) || 
          samples[0]==0 || samples[0]==1023) {
    valid = 0;
    } 
  else {
    samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples
    }
#endif

  *y = 1023-(1023-samples[NUMSAMPLES/2]);
// qui li inverto... x con y e poi la direzione di y (credo dipenda dalla Rotazione)


  __delay_us(30); // Fast ARM chips need to allow voltages to settle

  for(i=0; i<NUMSAMPLES; i++) {
    samples[i] = readADC(0 /*_xm*/);
    }

   // Set X+ to ground
   // Set Y- to VCC
   // Hi-Z X- and Y+
  TRISBbits.TRISB4=1;    //(_yp, INPUT);
  TRISEbits.TRISE5=0;    //(_ym, OUTPUT);
  TRISFbits.TRISF5=0;    //(_xp, OUTPUT);
  TRISBbits.TRISB3=1;    //(_xm, INPUT);

  LATEbits.LATE5=1;      //(_ym, HIGH); 
  LATFbits.LATF5=0;      //(_xp, LOW);
  
  
#if NUMSAMPLES > 2
   insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
   // Allow small amount of measurement noise, because capacitive
   // coupling to a TFT display's signals can induce some noise.
  if(((samples[0] - samples[1]) < -SAMPLE_NOISE) || ((samples[0] - samples[1]) > SAMPLE_NOISE) || 
          samples[0]==0 || samples[0]==1023) {
    valid = 0;
    } 
  else {
    samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples
    }
#endif

  *x = (1023-samples[NUMSAMPLES/2]);
  
//  *y = readADC(0);
  

  int z1 = readADC(0 /*_xm*/); 
  int z2 = readADC(1 /*_yp*/);

  if(RPLATE != 0) {
    // now read the x 
    float rtouch;
    rtouch = z2;
    rtouch /= z1;
    rtouch -= 1;
    rtouch *= *x;
    rtouch *= RPLATE;
    rtouch /= 1024;
     
    *z = rtouch;
    // abbiamo da 200 a 150 (< se + pressione) con stilo, 300..200 con cotton fioc :) 250..50 con dito
    } 
  else {
    *z = (1023-(z2-z1));
    }

  if(!valid) {
    *z = 0;
    }

  TRISBbits.TRISB3=0;
  TRISBbits.TRISB4=0;
  TRISEbits.TRISE5=0;
  TRISFbits.TRISF5=0;
  ANSELBbits.ANSB3 = 0;
  ANSELBbits.ANSB4 = 0;
//  ANSELEbits.ANSE5 = 0;
//  ANSELFbits.ANSF5 = 0;
  
  return valid;
  }


BOOL TranslateMessage(const MSG *lpMsg) {   // io la uso in maniera un po' diversa
  WPARAM j;
  LPARAM f;
  
  // flag vari, .. https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-char
  f = ((DWORD)lpMsg->wParam) << 16;   //lo scancode
  if(lpMsg->lParam & 0b00000100)    // e ALT-gr??
    f |= 0x20000000;    // 
  if(lpMsg->lParam & 0b01010000)    // qua! r-ctrl, alt-gr
    f |= 0x01000000;    // 
  
  j=MapVirtualKey(lpMsg->wParam,MAPVK_VSC_TO_VK);
  // 0 non sarebbe valido ma non importa!
  if(!(lpMsg->lParam & 0x80000000)) {
    internalKeyboardBuffer[j]++;
    f |= internalKeyboardBuffer[j];
    }
  else {
    internalKeyboardBuffer[j]=0;
    f |= 0x80000000;    // 
    }
  if(internalKeyboardBuffer[j])
    f |= 0x40000000;    // 
  ((MSG*)lpMsg)->wParam=j;
  ((MSG*)lpMsg)->lParam=f;
  return 1;
  }
int TranslateAccelerator(HWND hWnd,HACCEL hAccTable,MSG *lpMsg) {
  // usare hAccTable....
  if(!hWnd->internalState) {   // specialmente se acceleratori includono ESC e simili, e cmq ok
    if(!(lpMsg->lParam & 0x80000000)) {
      if(hWnd->menu && !(hWnd->style & WS_CHILD)) {
        int i;
        if(i=matchAccelerator(hWnd->menu,LOBYTE(lpMsg->wParam),LOBYTE(lpMsg->lParam))) {
          if(hWnd->menu->menuItems[i].flags & MNS_NOTIFYBYPOS) 
            SendMessage(hWnd,WM_MENUCOMMAND,i,(LPARAM)&hWnd->menu); // verificare...
          else
            SendMessage(hWnd,WM_COMMAND,MAKELONG(i,1 /*credo.. accel*/),MAKELONG(0,0));
          return 1;
          }
        }
      }
    }
  return 0;
  }
LRESULT DispatchMessage(const MSG *lpMsg) {
  int i;
  RECT rc;
  
  if(lpMsg->wParam) {
    HWND myWnd=lpMsg->hWnd;
    while(myWnd && !myWnd->enabled)
      myWnd=myWnd->parent;
    if(myWnd) {
      switch(myWnd->internalState) {
/*The TranslateMessage function generates a WM_CHAR message when the user presses any of the following keys:
    Any character key
    BACKSPACE
    ENTER (carriage return)
    ESC
    SHIFT+ENTER (linefeed)
    TAB*/
        case MSGF_MENU:   // se sono in menu, mando i msg alla DefWindowProc che si smazza frecce ecc!
          if(!(lpMsg->lParam & 0x80000000))
            DefWindowProc(myWnd,WM_KEYDOWN,lpMsg->wParam,lpMsg->lParam);
          else
            DefWindowProc(myWnd,WM_KEYUP,lpMsg->wParam,lpMsg->lParam);
          i=SendMessage(myWnd,WM_ENTERIDLE,myWnd->internalState,   (LPARAM)lpMsg->hWnd);   //SE DIALOG MANDARE HWND DIALOG...
          break;
        case MSGF_DIALOGBOX:           // (bah, è così)
          if(!(lpMsg->lParam & 0x80000000)) {
            SendMessage(myWnd,WM_KEYDOWN,lpMsg->wParam,lpMsg->lParam);
// pare che NORMALMENTE la pretranslatemessage ecc NON fa arrivare i CHAR a dialog... faccio così cmq
            i=MapVirtualKey(lpMsg->wParam,MAPVK_VK_TO_CHAR);
            if(i)
              SendMessage(myWnd,WM_CHAR,i,lpMsg->lParam);    // 
// ??                i=SendMessage(myWnd,WM_DEADCHAR,lpMsg->wParam,lpMsg->lParam);    // 
            }
          else
            SendMessage(myWnd,WM_KEYUP,lpMsg->wParam,lpMsg->lParam);
          i=SendMessage(myWnd,WM_ENTERIDLE,myWnd->internalState,   (LPARAM)lpMsg->hWnd);   //SE DIALOG MANDARE HWND DIALOG...
          break;
        case MSGF_MOVE:
          if(!lpMsg->hWnd->parent) {
            if(!(lpMsg->lParam & 0x80000000)) {
              rc=lpMsg->hWnd->nonClientArea;
              switch(lpMsg->wParam) {
                case VK_UP:
                  if(rc.top>=10) {
                    rc.top-=10; rc.bottom-=10;
                    }
                  goto move_window;
                  break;
                case VK_RIGHT:
                  if(rc.left<Screen.cx-10) {   // mah sì
                    rc.left+=10; rc.right+=10;
                    }
                  goto move_window;
                  break;
                case VK_DOWN:
                  if(rc.top<Screen.cy-10) {   // mah sì
                    rc.top+=10; rc.bottom+=10;
                    }
                  goto move_window;
                  break;
                case VK_LEFT:
                  if(rc.left>=10) {
                    rc.left-=10; rc.right-=10;
                    }
move_window:
						      SendMessage(lpMsg->hWnd,WM_MOVING,0,(LPARAM)&rc);
                  SetWindowPos(lpMsg->hWnd,NULL,rc.left,rc.top,0,0,SWP_NOSIZE | SWP_NOREDRAW | /*SWP_DRAWFRAME*/
                    SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOACTIVATE);
                  SetColors(WHITE,BLACK,R2_XORPEN);
                  DrawRectangle(lpMsg->hWnd->nonClientArea.left,lpMsg->hWnd->nonClientArea.top,
                    lpMsg->hWnd->nonClientArea.right,lpMsg->hWnd->nonClientArea.bottom,LIGHTGREEN);
                  SetColors(WHITE,BLACK,R2_COPYPEN);
/*moving 
poschanging
getminmaxinfo
poschanged
move*/
                  break;
                case VK_RETURN:
                case VK_ESCAPE:   // se ESC dovrebbe ripristinare condizioni precedenti al move...
                  SendMessage(lpMsg->hWnd,WM_EXITSIZEMOVE,0,0);
			            InvalidateRect(NULL,NULL,TRUE);   // serve ridisegnare desktop...
                  break;
                }
              }
            }
          break;
        case MSGF_SIZE:
          if(!lpMsg->hWnd->parent) {
            if(!(lpMsg->lParam & 0x80000000)) {
              rc=lpMsg->hWnd->nonClientArea;
              switch(lpMsg->hWnd->internalStateTemp) {
                case 0:
                  switch(lpMsg->wParam) {
                    case VK_LEFT:
                    case VK_UP:
                      lpMsg->hWnd->internalStateTemp=1;
                      break;
                    case VK_RIGHT:
                    case VK_DOWN:
                      lpMsg->hWnd->internalStateTemp=2;
                      break;
                    case VK_RETURN:
// mi piacerebbe	usare setwindopos solo con RETURN, ma non è così...
                    case VK_ESCAPE:   // se ESC dovrebbe ripristinare condizioni precedenti al size...
                      SendMessage(lpMsg->hWnd,WM_EXITSIZEMOVE,0,0);
                      break;
                    }
                  break;
                case 1:   // su, sinistra
                  switch(lpMsg->wParam) {
                    case VK_UP:
                      if(rc.top>=10)    //CY_MENU
                        rc.top-=10;
                      i=WMSZ_TOP;
                      goto update_size2;
                      break;
                    case VK_RIGHT:
                      if(rc.left<rc.right-20)    // mah sì v.getsystemmetrics CX_SIZE ecc
                        rc.left+=10;
                      i=WMSZ_RIGHT;
                      goto update_size2;
                      break;
                    case VK_DOWN:
                      if(rc.top<rc.bottom-20)    // mah sì
                        rc.top+=10;
                      i=WMSZ_BOTTOM;
                      goto update_size2;
                      break;
                    case VK_LEFT:
                      if(rc.left>=10)
                        rc.left-=10;
                      i=WMSZ_LEFT;
update_size2:
                      SendMessage(lpMsg->hWnd,WM_SIZING,i,(LPARAM)&rc);
                      SetWindowPos(lpMsg->hWnd,NULL,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,SWP_NOREDRAW | /*SWP_DRAWFRAME*/
                        SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOACTIVATE);
                      SetColors(WHITE,BLACK,R2_XORPEN);
                      DrawRectangle(lpMsg->hWnd->nonClientArea.left,lpMsg->hWnd->nonClientArea.top,
                        lpMsg->hWnd->nonClientArea.right,lpMsg->hWnd->nonClientArea.bottom,LIGHTGREEN);
                      SetColors(WHITE,BLACK,R2_COPYPEN);

/*sizing 
poschanging
getminmaxinfo
nccalcsize
ncpaint
erasebkgnd
poschanged
size*/

                      break;
                    case VK_RETURN:
// mi piacerebbe	usare setwindopos solo con RETURN, ma non è così...
                    case VK_ESCAPE:   // se ESC dovrebbe ripristinare condizioni precedenti al size...
                      SendMessage(lpMsg->hWnd,WM_EXITSIZEMOVE,0,0);
                      InvalidateRect(NULL,NULL,TRUE);   // serve ridisegnare desktop...
                      break;
                    }
                  break;
                case 2:   // giu, destra
                  switch(lpMsg->wParam) {
                    case VK_UP:
                      if(rc.bottom>rc.top+30)    //CY_MENU CY_SIZE
                        rc.bottom-=10;
                      i=WMSZ_TOP;
                      goto update_size;
                      break;
                    case VK_RIGHT:
                      if(rc.right<=Screen.cx-10)    // mah sì
                        rc.right+=10;
                      i=WMSZ_RIGHT;
                      goto update_size;
                      break;
                    case VK_DOWN:
                      if(rc.bottom<=Screen.cy-10)    // mah sì
                        rc.bottom+=10;
                      i=WMSZ_BOTTOM;
                      goto update_size;
                      break;
                    case VK_LEFT:
                      if(rc.right>rc.left+20) // CX_SIZE
                        rc.right-=10;
update_size:
                      SendMessage(lpMsg->hWnd,WM_SIZING,i,(LPARAM)&rc);
                      SetWindowPos(lpMsg->hWnd,NULL,0,0,rc.right-rc.left,rc.bottom-rc.top,SWP_NOMOVE | SWP_NOREDRAW | /*SWP_DRAWFRAME*/
                        SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOACTIVATE);
                      SetColors(WHITE,BLACK,R2_XORPEN);
                      DrawRectangle(lpMsg->hWnd->nonClientArea.left,lpMsg->hWnd->nonClientArea.top,
                        lpMsg->hWnd->nonClientArea.right,lpMsg->hWnd->nonClientArea.bottom,LIGHTGREEN);
                      SetColors(WHITE,BLACK,R2_COPYPEN);
                      break;
                    case VK_RETURN:
// mi piacerebbe	usare setwindopos solo con RETURN, ma non è così...
                    case VK_ESCAPE:   // se ESC dovrebbe ripristinare condizioni precedenti al size...
                      SendMessage(lpMsg->hWnd,WM_EXITSIZEMOVE,0,0);
                      InvalidateRect(NULL,NULL,TRUE);   // serve ridisegnare desktop...
                      break;
                    }
                  break;
                }
              }
            }
          break;
        default:
        // se tasto "speciale" browser volume ecc, mandare WM_APPCOMMAND bah ma non esiste?! forse solo MFC https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-appcommand
          if(!(lpMsg->lParam & 0x80000000)) {
            if(lpMsg->lParam & 0x20000000 /* || GetAsyncKeyState(VK_CONTROL) & 0x8000*/) {
              i=SendMessage /*DefwindowProc*/(myWnd,WM_SYSKEYDOWN,lpMsg->wParam,lpMsg->lParam);    // per ora :)
              i=MapVirtualKey(lpMsg->wParam,MAPVK_VK_TO_CHAR);
              if(i)
                SendMessage(myWnd,WM_SYSCHAR,i,lpMsg->lParam);    // 
// ??                i=SendMessage(myWnd,WM_SYSDEADCHAR,lpMsg->wParam,lpMsg->lParam);    // 
              }
            else {
              i=SendMessage(myWnd,WM_KEYDOWN,lpMsg->wParam,lpMsg->lParam);    //
              i=MapVirtualKey(lpMsg->wParam,MAPVK_VK_TO_CHAR);
              if(i)
                SendMessage(myWnd,WM_CHAR,i,lpMsg->lParam);    // 
// ??                i=SendMessage(myWnd,WM_DEADCHAR,lpMsg->wParam,lpMsg->lParam);    // 
              }
/*The TranslateMessage function generates a WM_CHAR message when the user presses any of the following keys:
    Any character key
    BACKSPACE
    ENTER (carriage return)
    ESC
    SHIFT+ENTER (linefeed)
    TAB*/
            }
          else {
            if(lpMsg->lParam & 0x20000000 /*|| GetAsyncKeyState(VK_CONTROL) & 0x8000*/) 
              i=SendMessage /*DefwindowProc*/(myWnd,WM_SYSKEYUP,lpMsg->wParam,lpMsg->lParam);
            else
              i=SendMessage(myWnd,WM_KEYUP,lpMsg->wParam,lpMsg->lParam);   // 
            }
          break;
        }
      }
    }
  return i;
  }
BOOL IsDialogMessage(HWND hDlg,MSG *lpMsg) {
  
// NO! questa va al padre...  if(hDlg->internalState == MSGF_DIALOGBOX) {
    switch(lpMsg->wParam) {
      case VK_RETURN:
      { BYTE n=GetWindowByte(hDlg,DWL_INTERNAL+1);		// il default button
        HWND myWnd=GetDlgItem(hDlg,n);
        if(myWnd /*&& myWnd->style & BS_DEFPUSHBUTTON*/) {   // solo se c'è... ed è DEFPUSHBUTTON?
          switch((DWORD)myWnd->menu) { 
            case IDOK:            // Notify the owner window to carry out the task. 
              EndDialog(hDlg,1);
              return TRUE; 
              break;
            case IDCANCEL: 
              EndDialog(hDlg,0);
              return TRUE; 
              break;
            } 
          }
// hmm no serve per cose particolari..          if(hWnd->parent)
//            SendMessage(hWnd->parent,WM_NOTIFY,0, 0 /*NMHDR struct*/);
      }
        goto forward_char;
        break;
      case VK_TAB:
        SendMessage(hDlg,WM_NEXTDLGCTL,GetAsyncKeyState(VK_SHIFT) & 0x8000,MAKELONG(0,0));
   // se c'è una child EDIT (con multiline) andrebbe inoltrato...?
//            err_puts("DLGtab ");

        return TRUE;
        break;
/* no :) ..        case VK_SPACE:
      {HWND myWnd=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));
    printf("premo SPAZIO: %u\r\n",myWnd);
      if(myWnd)
        SendMessage(hWnd,WM_COMMAND,MAKELONG((WORD)myWnd->tag,BN_CLICKED),myWnd);
      }
        break;*/
      default:
      {HWND myWnd2;
forward_char:
        if(myWnd2=GetDlgItem(hDlg,GetWindowByte(hDlg,DWL_INTERNAL))) {
          if(SendMessage(myWnd2,WM_GETDLGCODE,lpMsg->wParam,(DWORD)NULL /*msg..*/) & 
            DLGC_WANTALLKEYS | DLGC_WANTCHARS)      
          SendMessage(myWnd2,WM_CHAR,lpMsg->wParam,lpMsg->lParam);
          return TRUE;
          }
        }
        break;
      }
//    }
  return FALSE;
  }
BOOL IsHungAppWindow(HWND hwnd) {
  }
BOOL GetInputState() {
  }
BOOL InSendMessage() {
  }
DWORD InSendMessageEx() {
  }
BOOL GetMessage(MSG *lpMsg,HWND hWnd,uint16_t wMsgFilterMin,uint16_t wMsgFilterMax) {
  
  Yield();
  
  if(lpMsg->message==WM_QUIT)
    return FALSE;
  return TRUE;
  }
BOOL PeekMessage(MSG *lpMsg,HWND hWnd,uint16_t wMsgFilterMin,uint16_t wMsgFilterMax) {
  
  Yield();
  
  }

//extern HWND m_Wnd,m_Wnd2,m_Wnd3,m_Wnd4,m_Wnd5,m_Wnd6;
int8_t manageWindows(BYTE mouseClick) {
  int i;
  HWND hWnd;
  static DWORD oldclick=0;
  static BYTE oldmousebuttons=0;
  static WORD divider;
  BYTE dclick;
  DWORD nchit;
  MSG msg;
  
#ifdef TEST_THREADS
  for(i=1; i<3; i++) {
  nchit=(DWORD)&__PRETTY_FUNCTION__;   // porcodio di debugger se non si ferma
  nchit=(DWORD)&manageWindows;
  
    Yield();      

  }
//  GetCurrentThread()->env[ENV_POSITION_RETURNCODE]=34;
  ExitThread(34);
  return;
#endif
 
  if(mouseClick) {
    mouse.new=2;    // differenzio..
    goto is_touch;
    }
  
  POINT pt;

  if(!(eXtra & 1) && mouse.new) {

    setPowerMode(1); idleTime=0;
    
    if(mouse.x || mouse.y) {
      if(Screen.cx<640) {   // troppo veloce su schermo piccolo - servirebbe settaggio :)
        mouse.x/=2; mouse.y/=2;
        }
      mousePosition.x+=mouse.x;
      mousePosition.y+=mouse.y;
      if(!IsRectEmpty(&clipCursorRect)) {
        if((int16_t)mousePosition.x<clipCursorRect.left)
          mousePosition.x=clipCursorRect.left;
        if(mousePosition.x>=clipCursorRect.right-1)
          mousePosition.x=clipCursorRect.right-1;
        if((int16_t)mousePosition.y<clipCursorRect.top)
          mousePosition.y=clipCursorRect.top;
        if(mousePosition.y>=clipCursorRect.bottom-1)
          mousePosition.y=clipCursorRect.bottom-1;
        }
      else if(captureWindow) {
        if((int16_t)mousePosition.x<captureWindow->nonClientArea.left)
          mousePosition.x=captureWindow->nonClientArea.left;
        if(mousePosition.x>=captureWindow->nonClientArea.right-1)
          mousePosition.x=captureWindow->nonClientArea.right-1;
        if((int16_t)mousePosition.y<captureWindow->nonClientArea.top)
          mousePosition.y=captureWindow->nonClientArea.top;
        if(mousePosition.y>=captureWindow->nonClientArea.bottom-1)
          mousePosition.y=captureWindow->nonClientArea.bottom-1;
        }
      else {
        if((int16_t)mousePosition.x<0)
          mousePosition.x=0;
        if(mousePosition.x>=Screen.cx-1   /*8 posso sforare? o non vado info in fondo..*/  )//o size??
          mousePosition.x=Screen.cx-1;
        if((int16_t)mousePosition.y<0)
          mousePosition.y=0;
        if(mousePosition.y>=Screen.cy-1  /*8*/  )//o size?? MA NON va fino in fondo! si può sforare??
          mousePosition.y=Screen.cy-1;
        }
    
is_touch:    
drawcursor:
      pt.x=mousePosition.x; pt.y=mousePosition.y;
      hWnd=WindowFromPoint(pt);
      if(hWnd) {
        if(!SendMessage(hWnd,WM_SETCURSOR,(WPARAM)hWnd /*occhio se Captured*/,
          MAKELONG(0 /*ci vorrebbe HITTEST... ma io lo faccio sotto*/,
          hWnd->internalState>=MSGF_MOVE ? 0 : WM_MOUSEMOVE)))
          DefWindowProc(hWnd,WM_SETCURSOR,(WPARAM)hWnd /*occhio se Captured*/,
            MAKELONG(0 /*ci vorrebbe HITTEST... ma io lo faccio sotto*/,
            hWnd->internalState>=MSGF_MOVE ? 0 : WM_MOUSEMOVE));
        }
      else
        SetCursor(standardCursorSm);
      DrawCursor(mousePosition.x,mousePosition.y,NULL,0);
  // si potrebbe saltare questa parte se click mousebuttons..
      if(hWnd) {
        int wparam,lparam;
        wparam=keypress.modifier & 0b00010001 ? MK_CONTROL : 0;
        wparam |= keypress.modifier & 0b00100010 ? MK_SHIFT : 0;
        wparam |= mouse.buttons & 0b00000001 ? MK_LBUTTON : 0;
        wparam |= mouse.buttons & 0b00000010 ? MK_RBUTTON : 0;
        wparam |= mouse.buttons & 0b00000100 ? MK_MBUTTON : 0;
        lparam=MAKELONG(mousePosition.x,mousePosition.y);
        if(hWnd->internalState>=MSGF_MOVE) {
        // se siamo in MOVE o SIZE, il mouse è CATTURATO subito dopo aver usato una freccia! e si comporta come le frecce
        // fino a un click
          }
        i=SendMessage(hWnd,WM_NCHITTEST,0,lparam);
        RECT rc=hWnd->nonClientArea;
        // gestire WM_MOUSEHOVER qua
        switch(i) {
          case HTCLIENT:
            ScreenToClient(hWnd,&pt);
            SendMessage(hWnd,WM_MOUSEMOVE,wparam,MAKELPARAM(pt.x,pt.y));
            if(trackMouseEvent.hwndTrack) {
              TrackMouseEvent(&trackMouseEvent);
  //  						SendMessage(hWnd,WM_NCMOUSELEAVE,wparam,lparam);
  //  						SendMessage(hWnd,WM_NCMOUSEHOVER,wparam,lparam);
  //  						SendMessage(hWnd,WM_MOUSELEAVE,wparam,lparam);
              }
            if(hWnd->internalState)    // bah, è così
              SendMessage(hWnd,WM_ENTERIDLE,hWnd->internalState,  (LPARAM)hWnd);   // SE DIALOG MANDARE HWND DIALOG
            break;
          case HTTOPLEFT:
          case HTTOPRIGHT:
          case HTTOP:
          case HTLEFT:
          case HTRIGHT:
          case HTBOTTOMLEFT:
          case HTBOTTOM:
          case HTBOTTOMRIGHT:
//                case HTGROWBOX:
          case HTSIZE:
            if(longClickCnt && !(hWnd->style & WS_CHILD)) {
              if(hWnd->internalState < MSGF_MOVE) {
                hWnd->internalState=MSGF_SIZE;  //hWnd->locked=1;
                SendMessage(hWnd,WM_ENTERSIZEMOVE,0,0);
// boh no                      SendMessage(hWnd,WM_SYSCOMMAND,SC_SIZE,MAKELPARAM(pt.x,pt.y));
                switch(i) {
                  case HTTOPLEFT:
                  case HTTOPRIGHT:
                  case HTTOP:
                  case HTLEFT:
                    hWnd->internalStateTemp=1;
                    break;
                  case HTRIGHT:
                  case HTBOTTOMLEFT:
                  case HTBOTTOM:
                  case HTBOTTOMRIGHT: 
//                case HTGROWBOX:
                  case HTSIZE:
                    hWnd->internalStateTemp=2;
                    break;
                  }
                }
              switch(i) {
                case HTTOPLEFT:
                  rc.left+=mouse.x;
                  rc.top+=mouse.y;
                  i=WMSZ_TOPLEFT;
                  break;
                case HTTOPRIGHT:
                  rc.right+=mouse.x;
                  rc.top+=mouse.y;
                  i=WMSZ_TOPRIGHT;
                  break;
                case HTTOP:
                  rc.top+=mouse.y; 
                  i=WMSZ_TOP;
                  break;
                case HTLEFT:
                  rc.left+=mouse.x;
                  i=WMSZ_LEFT;
                  break;
                case HTRIGHT:
                  rc.right+=mouse.x;
                  i=WMSZ_RIGHT;
                  break;
                case HTBOTTOMLEFT:
                  rc.left+=mouse.x;
                  rc.bottom+=mouse.y;
                  i=WMSZ_BOTTOMLEFT;
                  break;
                case HTBOTTOM:
                  rc.bottom+=mouse.y;
                  i=WMSZ_BOTTOM;
                  break;
                case HTBOTTOMRIGHT:
//                  rc.right+=mouse.x;
//                  rc.bottom+=mouse.y;
//                  break;
//                case HTGROWBOX:
                case HTSIZE:
                  rc.right+=mouse.x;
                  rc.bottom+=mouse.y;
                  i=WMSZ_BOTTOMRIGHT;
                  break;
                }
              if((int)rc.left<=0)     // per ora almeno
                rc.left=0;
              if(rc.right>Screen.cx)
                rc.right=Screen.cx;
              if((int)rc.top<=0)
                rc.top=0;
              if(rc.bottom>Screen.cy)
                rc.bottom=Screen.cy;
              SendMessage(hWnd,WM_SIZING,0/*WMSZ_BOTTOM ecc gestire*/,(LPARAM)&rc);
              SetWindowPos(hWnd,NULL,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
                SWP_NOSENDCHANGING | SWP_NOREDRAW | SWP_NOZORDER | SWP_NOACTIVATE);
              SetColors(WHITE,BLACK,R2_XORPEN);
              DrawRectangle(hWnd->nonClientArea.left,hWnd->nonClientArea.top,
                hWnd->nonClientArea.right,hWnd->nonClientArea.bottom,WHITE);
              SetColors(WHITE,BLACK,R2_COPYPEN);
              // fare rettangolo XOR...
              }
            else
              goto ncmousemove;
            break;
          case HTCAPTION:
            if(longClickCnt && !(hWnd->style & WS_CHILD)) {
              if(hWnd->internalState < MSGF_MOVE) {
                hWnd->internalState=MSGF_MOVE;  //hWnd->locked=1;
                SendMessage(hWnd,WM_ENTERSIZEMOVE,0,0);
// boh no                      SendMessage(hWnd,WM_SYSCOMMAND,SC_MOVE,MAKELPARAM(pt.x,pt.y));
                }
              rc.left+=mouse.x;
              if((int)rc.left<0)   // per ora...
                rc.left=0;
              if(rc.left>=Screen.cx)
                rc.left=Screen.cx;
              rc.top+=mouse.y;
              if((int)rc.top<0)   //
                rc.top=0;
              if(rc.top>=Screen.cy)
                rc.top=Screen.cy;
              // si potrebbe confluire su sopra...
  			      SendMessage(hWnd,WM_MOVING,0,(LPARAM)&rc); 
              SetWindowPos(hWnd,NULL,rc.left,rc.top,0,0,SWP_NOSIZE | SWP_NOREDRAW |
                SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOACTIVATE);
              SetColors(WHITE,BLACK,R2_XORPEN);
              DrawRectangle(hWnd->nonClientArea.left,hWnd->nonClientArea.top,
                hWnd->nonClientArea.right,hWnd->nonClientArea.bottom,WHITE);
              SetColors(WHITE,BLACK,R2_COPYPEN);
              // fare rettangolo XOR...
              }
            else
              goto ncmousemove;
            break;
          case HTERROR:
						MessageBeep(MB_ICONASTERISK);
            break;
          default:
            if(i != HTNOWHERE) {
ncmousemove:              
              if(hWnd->internalState==MSGF_MENU) {    // mah, mi pare così
                ScreenToClient(hWnd,&pt);
                SendMessage(hWnd,WM_MOUSEMOVE,wparam,MAKELPARAM(pt.x,pt.y));
                }
              else {
                SendMessage(hWnd,WM_NCMOUSEMOVE,i,lparam);
                }
              if(trackMouseEvent.hwndTrack) {
                TrackMouseEvent(&trackMouseEvent);
  //  						SendMessage(hWnd,WM_NCMOUSELEAVE,wparam,lparam);
  //  						SendMessage(hWnd,WM_NCMOUSEHOVER,wparam,lparam);
  //  						SendMessage(hWnd,WM_MOUSELEAVE,wparam,lparam);
                }
  //          if(hWnd->internalState==MSGF_MENU)    // qua direi di NO
  //            SendMessage(hWnd,WM_ENTERIDLE,MSGF_MENU,   (LPARAM)hWnd);   //
              }
            break;
          }
        }
      }
    
    mouse.new=0;
    
		if(inScreenSaver) {
      exitScreenSaver();
      oldmousebuttons=0;
      goto fine;
      }

    if((mouse.buttons ^ oldmousebuttons) & 7)
      goto is_click;
    else
      goto fine;
    }

  if(keypress.new) {
    
    if(!(keypress.state & 0b10000000)) {    // solo pressione qua!
      setPowerMode(1); 
      idleTime=0;
      if(inScreenSaver) {
        exitScreenSaver();
        goto fine;
        }
      }
      
    hWnd=GetForegroundWindow();   //non c'è taskbar qua... non è gravissimo se intercetto tipo SC_TASKLIST in DefWindowProc di tutti!
    if(hWnd) {
      HACCEL hAccelTable;
      msg.hWnd=hWnd;
//      msg.lPrivate=0;
      msg.time=TickGetMilliseconds();
      msg.wParam=keypress.key;
      if(keypress.key>='a' && keypress.key<='z')
        msg.wParam=keypress.key & ~0x20;   // sarebbe meglio che da south bridge uscissero maiuscole... FARE ma attenzione a DOS
      msg.lParam=keypress.modifier;
      if(keypress.state & 0b10000000) 
        msg.lParam |= 0x80000000;
      if(hWnd->menu)
        hAccelTable=hWnd->menu->accelerators;
      else
        hAccelTable=NULL;
      msg.message=0;
//      if(IsDialogMessage(hWnd,&msg)) {
//        }
//      else          // https://devblogs.microsoft.com/oldnewthing/20160818-00/?p=94115
      if(!TranslateAccelerator(msg.hWnd,hAccelTable,&msg)) {  //if wParam != 0 o no...? ossia solo modifier
        // siccome la Translate avviene dopo, Gli accelerator non-ascii non vanno o meglio ci va lo scancode... sistemare! v. surf
        TranslateMessage(&msg);   // è ok che la traduzione dei tasti arrivi dopo... anche se non era esattamente così in windows
        msg.message=WM_CHAR;    // non usato cmq, v.
        DispatchMessage(&msg);
        }
      }

    if(!(keypress.state & 0b10000000)) {    // solo pressione qua!
      //METTERE (anche) QUESTO in DispatchMessage !
      switch(keypress.key) {
        case 0xa9:      // F9 ingrandisci / riduci a icona .. bah http://aggedor.freeshell.org/win31key.html
          if(hWnd) {
            if(keypress.modifier & 0b00000100) {
              if(hWnd->minimized)
                ShowWindow(hWnd,SW_SHOWNORMAL); //SendMessage(hWnd,WM_SYSCOMMAND,SC_RESTORE,0);
              else
                ShowWindow(hWnd,SW_SHOWMINIMIZED); //SendMessage(hWnd,WM_SYSCOMMAND,SC_MINIMIZE,0);
              }
            else {
              if(hWnd->maximized)
                ShowWindow(hWnd,SW_SHOWNORMAL); //SendMessage(hWnd,WM_SYSCOMMAND,SC_RESTORE,0);
              else
                ShowWindow(hWnd,SW_SHOWMAXIMIZED);  //SendMessage(hWnd,WM_SYSCOMMAND,SC_MAXIMIZE,0);
              }
            }
          break;
        case 0x9:   //TAB
          if(keypress.modifier & 0b00000100) {    // finire con prima o succ. pressione di ALT!
            SendMessage(hWnd,WM_SYSCOMMAND,keypress.modifier & 0b00100010 ? SC_PREVWINDOW : SC_NEXTWINDOW,0);
            static BYTE whichWindow=0;     // non è il massimo ma ok...
            HWND w2=rootWindows,w3=NULL;
            i=0;
            if(w2) {
              while(w2->next) {
                if(i==whichWindow) {
                  w3=w2;
                  whichWindow++;
                  break;
                  }
                w2=w2->next;
                i++;
                }
              if(!w3) {
                w3=rootWindows;
                whichWindow=0;
                }
              if(w3)
  // provare!              SwitchToThisWindow(w3,1);
                ShowWindow(w3,SW_SHOWNORMAL);   // e questa manda wm_windowposchanging anche qua (Asse Z)
              }
            }
          if(keypress.modifier & 0b00000001) {    // MDI; finire con prima o succ. pressione di CTRL!
            }
          break;
        case 0xa4:     // CTRL ALT F4, chiude baracca!
          if((keypress.modifier & 0b00000101) == 0b00000101) {
            quitSignal=1;
            }
          break;
        case 0x9e:    // menu key
          SendMessage(hWnd,WM_CONTEXTMENU,0,MAKELONG(mousePosition.x,mousePosition.y));
          break;
        case 0x1b:     // CTRL ESC task manager ecc
          if(keypress.modifier & 0b00001000) {
            }
          if(keypress.modifier & 0b00000001) {
            if(taskbarWindow) {
              if(keypress.modifier & 0b00000010)    // SHIFT
                ShellExecute(NULL,"taskman",NULL,NULL,NULL,SW_SHOWNORMAL);
              else
                SendMessage(taskbarWindow,WM_SYSCOMMAND,SC_TASKLIST,0);   // 
              }
            }
          break;
        case 0xc8:    // volume & mute
          if((volumeAudio & 0x7f) < 100)
            volumeAudio++;
          break;
        case 0xc9:
          if((volumeAudio & 0x7f) > 0)
            volumeAudio--;
          break;
        case 0xc5:
          volumeAudio ^= 0x80;
          break;
        case 'm': 
          if(keypress.modifier & 0b10001000)    // WIN-M mostra desktop
            SendMessage(desktopWindow,WM_COMMAND,MAKELONG(18,0),0); 
          //else dovrebbe anche rimassimizzarle...
          break;
          
          
          //hotkeyTable
  //          SendMessage(hWnd,WM_HOTKEY,hotkeyTable.command /*IDHOT_SNAPWINDOW IDHOT_SNAPDESKTOP*/,
    //          MAKELONG(keypress.modifier/* OCCHIO SON DIVERSI! sistemare*/,keypress.key); 

        case 0x94: // up, 
          if(eXtra & 1) {
            mouse.buttons=0;
            if(mousePosition.y>0)
              mousePosition.y-=2;
            goto drawcursor;
            }
          break;
        case 0x93: // down
          if(eXtra & 1) {
            mouse.buttons=0;
            if(mousePosition.y<Screen.cy-1 /*8 sforo per andare in fondo*/  )//o size??
              mousePosition.y+=2;
            goto drawcursor;
            }
          break;
        case 0x92: // left
          if(eXtra & 1) {
            mouse.buttons=0;
            if(mousePosition.x>0)
              mousePosition.x-=2;
            goto drawcursor;
          }
          break;
        case 0x91: // right
          if(eXtra & 1) {
            mouse.buttons=0;
            if(mousePosition.x<Screen.cx-1 /*8 sforo per andare in fondo*/    )//o size??
              mousePosition.x+=2;
            goto drawcursor;
            }
          break;
  


        case 0x0a:		// simulo lbuttondown con RETURN
          if(eXtra & 1) {
            DWORD tipoClick;

            mouse.buttons=keypress.modifier & 0b01000000 ? 2 : 1;    // ALT-gr

is_click: 
      //e METTERE (pure) QUESTO in DispatchMessage !
            dclick=0;

            if(mouse.buttons & 1) {
              if(oldmousebuttons & 1) {
                longClickCnt++;
                }
              else {
                tipoClick=WM_LBUTTONDOWN;
                dclick=(timeGetTime()-oldclick) < doubleClickTime;
                oldclick=timeGetTime();
                if(!longClickCnt)
                  longClickCnt++;
                }
              }
            else {
              if(oldmousebuttons & 1) {
                tipoClick=WM_LBUTTONUP;
                longClickCnt=0;
                }
              else {
                }
              }
            if(mouse.buttons & 2) {
              if(oldmousebuttons & 2) {
  //poi              if(!longClickCnt)
  //                longClickCnt++;
                }
              else {
                tipoClick=WM_RBUTTONDOWN;
                }
              }
            else {
              if(oldmousebuttons & 2) {
                tipoClick=WM_RBUTTONUP;
  // poi..              longClickCnt=0;
  //              oldclick=timeGetTime();
                }
              else {
                }
              }

            pt.x=mousePosition.x; pt.y=mousePosition.y;
            hWnd=WindowFromPoint(pt);
            if(activeMenuWnd && IsChild(activeMenuWnd,hWnd))    //se menu aperto, non prendo i figli!
              hWnd=activeMenuWnd;
            if(hWnd) {
              nchit=SendMessage(hWnd,WM_NCHITTEST,0,MAKELPARAM(mousePosition.x,mousePosition.y));
    //        printf("..NCHIT: %d\r\n",nchit);
              if(nchit == HTCLIENT) {
                int wparam,lparam;
                wparam=keypress.modifier & 0b00010001 ? MK_CONTROL : 0;
                wparam |= keypress.modifier & 0b00100010 ? MK_SHIFT : 0;
                wparam |= mouse.buttons & 0b00000001 ? MK_LBUTTON : 0;
                wparam |= mouse.buttons & 0b00000010 ? MK_RBUTTON : 0;
                wparam |= mouse.buttons & 0b00000100 ? MK_MBUTTON : 0;
                int x=mousePosition.x,y=mousePosition.y;
                HWND myWnd=hWnd;
                if(myWnd != desktopWindow && myWnd != taskbarWindow) {
                  while(myWnd && (!myWnd->enabled && (myWnd->class.class==WC_STATIC && !(myWnd->style & SS_NOTIFY))))   // NON mando a disabled (ma includo SS_NOTIFY...
                    myWnd=myWnd->parent;
                  }
                if(myWnd) {   // non dovrebbe accadere cmq
                  HWND myWnd2=myWnd;
                  do { //ScreenToClient( ??
                    x-=myWnd2->clientArea.left;
                    y-=myWnd2->clientArea.top;
                  //    OffsetPoint(RECT *lprc,int16_t dx,int16_t dy);
                    myWnd2=myWnd2->parent;
                    } while(myWnd2);
                  lparam=MAKELONG(x,y);
                  switch(myWnd->internalState) {   // anche qua??
                    case MSGF_MENU:   // se sono in menu, mando i msg alla DefWindowProc che si smazza frecce ecc!
                      DefWindowProc(hWnd,tipoClick,wparam,lparam);
                      //#warning  verificare... mettere in windowproc??
                      if(activeMenu) {
                        if(activeMenuWnd==hWnd) {
                          if(i=getMenuPopupFromPoint(activeMenu,&activeMenuRect,pt,NULL)) {
                            if(activeMenu==&systemMenu) {
                              SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);   // unire? forse PULISCE activeMENU OCCHIO
                              SendMessage(hWnd,WM_SYSCOMMAND,MAKELONG(i,0),MAKELONG(0,0));
                              }
                            else {
                              SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);   // unire? forse PULISCE activeMENU OCCHIO
                              if(activeMenu->menuItems[i].flags & MNS_NOTIFYBYPOS) 
                                SendMessage(hWnd,WM_MENUCOMMAND,i,(LPARAM)activeMenu);
                              else
                                SendMessage(hWnd,WM_COMMAND,MAKELONG(i,0),0);
                              }
                            }
                          else
                            SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);   // in teoria se non era mio menu, ci pensa ACTIVATE a chiuderlo all'altra...
                          }
                        else if(activeMenuWnd==hWnd->parent) {    // e andrebbe fatto a cascata...
                          SendMessage(hWnd->parent,WM_MENUSELECT,MAKELONG(0,0xffff),0);
                          }
                        }
                      break;
                    case MSGF_DIALOGBOX:          
                      break;
                    default:
                      SendMessage(myWnd,tipoClick,wparam,lparam);
                      if(tipoClick==WM_RBUTTONUP)
                        SendMessage(myWnd,WM_CONTEXTMENU,wparam,lparam);
                      break;
                    }
                  }
                }
              else if(nchit != HTNOWHERE) {
                if(nchit != HTMENU) {
                  if(activeMenu && tipoClick == WM_LBUTTONDOWN) {
    //#warning  verificare... mettere in windowproc??
                    if(dclick) {      // HTSYSMENU ma dopo un click arriva qua
//non va mai... v. anche sotto, credo dipenda dal tempo che impiega a disegnare menu e quindi il timer dclick salta
                      SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0); // NON va cmq,,,se no si mangia il dclick...
                      SendMessage(hWnd,WM_SYSCOMMAND,SC_DEFAULT,MAKELPARAM(pt.x,pt.y));
                      }
                    else
                      SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0); 
                    }
                  }
                else {
                  UGRAPH_COORD_T x,y;
//                  HWND myWnd=hWnd;
                  x=mousePosition.x;
                  y=mousePosition.y;

                  // ANCHE QUESTO solo se ENABLED?? boh
                  SendMessage(hWnd,tipoClick == WM_LBUTTONDOWN ? WM_NCLBUTTONDOWN : WM_NCRBUTTONDOWN,
                    nchit,MAKELPARAM(x,y));
                  }
                }

              switch(nchit) {
                case HTTOPLEFT:
                  {
                  goto entersize;
                  }
                  break;
                case HTTOPRIGHT:
                  {
                  goto entersize;
                  }
                  break;
                case HTTOP:
                  {
                  goto entersize;
                  }
                  break;
                case HTLEFT:
                  {
                  goto entersize;
                  }
                  break;
                case HTRIGHT:
                  {
                  goto entersize;
                  }
                  break;
                case HTBOTTOMLEFT:
                  {
                  goto entersize;
                  }
                  break;
                case HTBOTTOM:
                  {
                  goto entersize;
                  }
                  break;
                case HTBOTTOMRIGHT:
                  {
                  goto entersize;
                  }
                  break;
                case HTBORDER:  //In the border of a window that does not have a sizing border.
                  break;
                case HTHELP:
                  break;
//                case HTGROWBOX:
                case HTSIZE:
                  {
entersize:
                  if(tipoClick == WM_LBUTTONDOWN)
                    SendMessage(hWnd,WM_SYSCOMMAND,SC_SIZE,MAKELPARAM(pt.x,pt.y));
                  }
                  break;
                case HTSYSMENU:
                  //nclmousebuttondown (ht_sysmenu) e poi syscommand e quindi entermenuloop
    //              SendMessage(hWnd,WM_SYSCOMMAND,SC_KEYMENU,MAKELONG(' ',0/*mnemonic?*/));
                  if(tipoClick == WM_LBUTTONDBLCLK) {
//non va mai... v. anche sopra, credo dipenda dal tempo che impiega a disegnare menu e quindi il timer dclick salta
                    SendMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,MAKELPARAM(pt.x,pt.y));
// forse giusto così       SendMessage(hWnd,WM_SYSCOMMAND,SC_DEFAULT,MAKELPARAM(pt.x,pt.y));
                    }
                  else if(tipoClick == WM_LBUTTONDOWN) {
//                    if(dclick)
//                      SendMessage(hWnd,WM_SYSCOMMAND,SC_DEFAULT,MAKELPARAM(pt.x,pt.y));   // V.SOPRA (non va.. strano.. 27/2/23 forse dipende dal fatto si apre un menu
//                      SendMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,MAKELPARAM(pt.x,pt.y));
//                    else
                      SendMessage(hWnd,WM_SYSCOMMAND,SC_MOUSEMENU,0xffffffff); // truschino per indicare il systemmenu!
                    }
                  break;
                case HTCAPTION:
                  if(dclick) {
                    if(hWnd->maximized)
                      SendMessage(hWnd,WM_SYSCOMMAND,SC_RESTORE,MAKELPARAM(pt.x,pt.y));
                    else
                      SendMessage(hWnd,WM_SYSCOMMAND,SC_MAXIMIZE,MAKELPARAM(pt.x,pt.y));
                    }
                  else if(tipoClick == WM_RBUTTONDOWN) {
                    // in effetti c'è anche su scrollbar (menu scrollbar) e pare su closebox e minimizebox!
                    // e in effetti è DOPO CONTEXTMENU... spostare?
      /*                      if(TrackPopupMenu((HMENU)&systemMenu,TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON,
                      mousePosition.x-hWnd->nonClientArea.left+4,mousePosition.y-hWnd->nonClientArea.top + TITLE_HEIGHT, //lo ficco entro la client così si pulisce bene :D 
                      0,hWnd,NULL)) {
                      }*/SendMessage(hWnd,WM_RBUTTONDOWN,0,MAKELPARAM(pt.x,pt.y));
                    }
                  else if(tipoClick == WM_RBUTTONUP) {
                    SendMessage(hWnd,WM_CONTEXTMENU,0,MAKELPARAM(pt.x,pt.y));
                    }
                  break;
                case HTCLOSE:
                  if(tipoClick == WM_LBUTTONDOWN)
                    SendMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,MAKELPARAM(pt.x,pt.y));
                  break;
                case HTMAXBUTTON:
                  if(tipoClick == WM_LBUTTONDOWN) {
                    if(hWnd->maximized)
                      SendMessage(hWnd,WM_SYSCOMMAND,SC_RESTORE,MAKELPARAM(pt.x,pt.y));
                    else
                      SendMessage(hWnd,WM_SYSCOMMAND,SC_MAXIMIZE,MAKELPARAM(pt.x,pt.y));
                    }
                  break;
                case HTMINBUTTON:
                  if(tipoClick == WM_LBUTTONDOWN)
                    SendMessage(hWnd,WM_SYSCOMMAND,SC_MINIMIZE,MAKELPARAM(pt.x,pt.y));
                  break;
                case HTMENU:
                  if(tipoClick == WM_LBUTTONDOWN) {    
                    if(hWnd->menu && !(hWnd->style & WS_CHILD)) {   // [direi ovvio dato nchit...]
                      HMENU myMenu;
                      POINT pt;
                      pt.x=mousePosition.x; pt.y=mousePosition.y;
                      i=getMenuFromPoint(hWnd,pt,&myMenu);
                      if(myMenu) {
                        if(myMenu->menuItems[0].flags & MF_POPUP)
                          SendMessage(hWnd,WM_SYSCOMMAND,SC_MOUSEMENU,MAKELPARAM(pt.x,pt.y)); // v. altri... 
                        else {
                          if(i) {
                            if(myMenu->menuItems[0].flags & MNS_NOTIFYBYPOS) 
                              SendMessage(hWnd,WM_MENUCOMMAND,i,(LPARAM)&myMenu->menuItems[0]);
                            else
                              SendMessage(hWnd,WM_COMMAND,MAKELONG(i,0),0);
                            }
                          }
                        }
                      else {    // a sto punto forse questo non ha senso...
                        if(i) {
                          if(myMenu->menuItems[0].flags & MNS_NOTIFYBYPOS) 
                            SendMessage(hWnd,WM_MENUCOMMAND,i,(LPARAM)&myMenu->menuItems[0]);
                          else
                            SendMessage(hWnd,WM_COMMAND,MAKELONG(i,0),0);
                          }
                        }
                      }
                    }
                  break;
                case HTHSCROLL:
                  if(tipoClick == WM_LBUTTONDOWN)
                    SendMessage(hWnd,WM_SYSCOMMAND,SC_HSCROLL,MAKELPARAM(pt.x,pt.y)); // v. altri... 
                  break;
                case HTCLIENT:
                  if(tipoClick == WM_LBUTTONDOWN && dclick) {
                    if(hWnd->minimized)
                      ShowWindow(hWnd,SW_SHOWNORMAL);
                    }
                  break;
                case HTVSCROLL:
                  if(tipoClick == WM_LBUTTONDOWN)
                    SendMessage(hWnd,WM_SYSCOMMAND,SC_VSCROLL,MAKELPARAM(pt.x,pt.y)); // v. altri... 
                  break;
                case HTNOWHERE:
                  break;
                }

              if(tipoClick == WM_LBUTTONDOWN || tipoClick == WM_RBUTTONDOWN) {  // mah direi entrambi, sì
//                if(!hWnd->active && hWnd != desktopWindow /*faccio così per non fare casino*/) {
                  HWND myWnd=hWnd->parent,myWnd2=hWnd->parent;
                  while(myWnd) {
//#warning il parentnotify si potrebbe mandare da qua..  coord schermo già ok TOGLIERE da buttondown ecc
                    myWnd2=myWnd;
                    SendMessage(myWnd,WM_PARENTNOTIFY,tipoClick,MAKELPARAM(pt.x,pt.y));
                    myWnd=myWnd->parent;
                    }
      //            if(hWnd->parent && !hWnd->parent->internalState)    // non è troppo chiaro...
//                  if(!hWnd->active) {
                    BYTE i=hWnd->locked;  hWnd->locked=0; // in caso si fosse aperto un menu un attimo prima!
                    if(SendMessage(hWnd,WM_MOUSEACTIVATE,(WPARAM)myWnd2,MAKELONG(nchit,tipoClick)) == MA_ACTIVATE /*MA_ACTIVATEANDEAT*/) {
                      HWND oWnd=setActive(hWnd,TRUE);
                      WINDOWPOS wpos;
                      wpos.hwnd=hWnd;
                      wpos.hwndInsertAfter=oWnd /*boh*/;
                      wpos.x=hWnd->nonClientArea.left;
                      wpos.y=hWnd->nonClientArea.top;
                      wpos.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
                      wpos.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
                      wpos.flags=SWP_NOREPOSITION | SWP_SHOWWINDOW;
                      SendMessage(hWnd,WM_WINDOWPOSCHANGING,0,(LPARAM)&wpos);
                      // finire, verificare... anche in ShowWindow

//#warning The DefWindowProc function passes the message to a child window's parent window before any processing occurs. The parent window determines whether to activate the child window. If it activates the child window, the parent window should return MA_NOACTIVATE or MA_NOACTIVATEANDEAT to prevent the system from processing the message further.
                      }
          // PROVARE! SwitchToThisWindow(hWnd,FALSE);
                    hWnd->locked=i;
//                    }
  //                }
                }
              if(tipoClick==WM_LBUTTONUP && hWnd->internalState>=MSGF_MOVE) {
                // v. anche nota sopra, se il mouse è catturato da MOVE o SIZE
                SetColors(WHITE,BLACK,R2_XORPEN);
                DrawRectangle(hWnd->nonClientArea.left,hWnd->nonClientArea.top,
                  hWnd->nonClientArea.right,hWnd->nonClientArea.bottom,WHITE);
                SetColors(WHITE,BLACK,R2_COPYPEN);
                SendMessage(hWnd,WM_EXITSIZEMOVE,0,0);
  	            InvalidateRect(NULL,NULL,TRUE);   // serve ridisegnare desktop...
//per cui no queste                SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
//                InvalidateRect(hWnd,NULL,TRUE);
                //ShowWindow(hWnd,SW_SHOW);
                SetCursor(hWnd->cursor ? hWnd->cursor : standardCursor);
                }

              if(dclick     && tipoClick == WM_LBUTTONDOWN  /* PER ORA!*/ && nchit==HTCLIENT) {
                WNDCLASS *wc;
                GetClassInfo(0,hWnd->class,&wc);
                if(wc->style & CS_DBLCLKS) {
                  UGRAPH_COORD_T x,y;
                  HWND myWnd=hWnd;
                  if(myWnd != desktopWindow && myWnd != taskbarWindow) {
                    while(myWnd && (!myWnd->enabled && (myWnd->class.class==WC_STATIC && !(myWnd->style & SS_NOTIFY))))   // NON mando a disabled (ma includo SS_NOTIFY...
                      myWnd=myWnd->parent;
                    }
                  if(myWnd) {   // non dovrebbe accadere cmq
                    int wparam,lparam;
                    wparam=keypress.modifier & 0b00010001 ? MK_CONTROL : 0;
                    wparam |= keypress.modifier & 0b00100010 ? MK_SHIFT : 0;
                    wparam |= mouse.buttons & 0b00000001 ? MK_LBUTTON : 0;
                    wparam |= mouse.buttons & 0b00000010 ? MK_RBUTTON : 0;
                    wparam |= mouse.buttons & 0b00000100 ? MK_MBUTTON : 0;
                    HWND myWnd2=myWnd;
                    x=mousePosition.x;
                    y=mousePosition.y; //ScreenToClient
                    do {
                      x-=myWnd2->clientArea.left;
                      y-=myWnd2->clientArea.top;
                    //    OffsetPoint(RECT *lprc,int16_t dx,int16_t dy);
                      myWnd2=myWnd2->parent;
                      } while(myWnd2);
                    SendMessage(myWnd,WM_LBUTTONDBLCLK,wparam,(LPARAM)MAKELPARAM(x,y));
                    }
                  }
                }
              // 
              // o forse deve gestirsi le cose la window??

    /*          if(i == HTCLIENT)
                SendMessage(hWnd,WM_LBUTTONUP,0,0);
              else if(i != HTNOWHERE)
                SendMessage(hWnd,WM_NCLBUTTONUP,0,0);*/

              }

            oldmousebuttons=mouse.buttons;
            }
          break;

        case 0xb9: // Power ecc
        case 0xba:
        case 0xbb:
          setPowerMode(keypress.key==0xb9 || keypress.key==0xbb);    // per ora così
          break;

        case 0xb8:    // VK_SNAPSHOT PrtSc
          {SUPERFILE f;
          UGRAPH_COORD_T x,y;
          GFX_COLOR c;
          struct BITMAPFILEHEADER bfi;
          struct BITMAPINFOHEADER bi;
          DC myDC;
          HDC hDC;
          HWND myWnd=NULL;
          f.drive=currDrive;
          // ovvero andare in Clipboard!
          if(SuperFileOpen(&f,"snapshot.bmp",'w')) {    // o JPG o meglio PNG
            bfi.bfReserved1=bfi.bfReserved2=0;
            bfi.bfOffBits=sizeof(struct BITMAPFILEHEADER)+sizeof(struct BITMAPINFOHEADER);
            bfi.bfType=MAKEWORD('B','M');
            bi.biSize=sizeof(struct BITMAPINFOHEADER);
            bi.biBitCount=24 /*16*/;
            bi.biPlanes=1;
            bi.biClrImportant=bi.biClrUsed=0;
            bi.biXPelsPerMeter=bi.biYPelsPerMeter=0;
            if(GetAsyncKeyState(VK_SHIFT /*VK_MENU non arriva qua...*/) & 0x8000) {     // 
              myWnd=GetForegroundWindow();
              x=0;
              y=myWnd->nonClientArea.bottom-myWnd->nonClientArea.top;
              bi.biWidth=myWnd->nonClientArea.right-myWnd->nonClientArea.left+1;
              bi.biHeight=myWnd->nonClientArea.bottom-myWnd->nonClientArea.top+1; 
              }
            else {
              x=0;
              y=Screen.cy-1;
              bi.biWidth=Screen.cx;
              bi.biHeight=Screen.cy;
              }
            hDC=GetWindowDC(myWnd,&myDC);
            bi.biWidth=(bi.biWidth+3) & 0xfffc;   // PAD DWORD; ovviamente se la finestra/area è minore, pesco pixel a dx, non è un problema
            bi.biCompression=0;
            bi.biSizeImage=bi.biWidth*bi.biHeight*24 /*sizeof(GFX_COLOR)*//8;
            bfi.bfSize=bfi.bfOffBits+bi.biSizeImage;
            SetCursor(hourglassCursorSm);
            SuperFileWrite(&f,(char*)&bfi,sizeof(struct BITMAPFILEHEADER));
            SuperFileWrite(&f,(char*)&bi,sizeof(struct BITMAPINFOHEADER));
            while(bi.biHeight--) {
              bfi.bfReserved2=bi.biWidth;
              x=0;
              while(bfi.bfReserved2--) {
                c=GetPixel(hDC,x,y);
                COLORREF c2=ColorRGB(c);
                SuperFileWrite(&f,(char*)&c2,3);
                ClrWdt();
                x++;
                }
              y--;    // bitmap top-down!
              // fare handle_events qua? bah...
              }
            SuperFileClose(&f);
            SetCursor(standardCursorSm);
            }
          ReleaseDC(myWnd,hDC);
          }
          break;
        }
			}
    
    if(keypress.state & 0b10000000) {    // e questi al rilascio
      switch(keypress.key) {
        case 0x0:     // L-WIN start .. e ce ne sono altri? no!
          // BISOGNEREBBE FARLO SOLO SE PRIMA NON C'è STATO M O ALTRO
          if(keypress.modifier & 0b10001000)    // WIN
            SendMessage(taskbarWindow,WM_SYSCOMMAND,SC_TASKLIST,0);   // 
          break;
        }
			}

fine:
    keypress.new=0; // no, o mi perdo ev. modifier...KBClear();
    }
  
//  if(longClickCnt)
//    longClickCnt++;


	if(!inScreenSaver) {
		idleTime++;
		if((eXtra & 128) && (idleTime/1000)>screenSaverTime && 
			(mousePosition.y!=0 || mousePosition.x!=Screen.cx-1)) {		// vecchia ideuzza :)
      HWND myWnd=rootWindows;
      i=1;
      while(myWnd) {
        if(!SendMessage(myWnd,WM_SYSCOMMAND,SC_SCREENSAVE,0)) {
          i=0;
          break;
          }
        myWnd=myWnd->next;
        }
      if(i)
        enterScreenSaver();
      // e dopo un po' fare PowerDown?! :)
			}
		}
	else {
		divider++;
    if(divider>=1000) {
      divider=0;
      SendMessage(desktopWindow,WM_PAINT,0,0);
      // e dopo un po' fare PowerDown?! :)
      }
		}

  if(quitSignal) {
    HWND myWnd=rootWindows;
    while(myWnd) {
      if(!SendMessage(myWnd,WM_QUERYENDSESSION,0,0))
        return 1;
      myWnd=myWnd->next;
      }
    SendMessage(HWND_BROADCAST,WM_ENDSESSION,0,0);   // 
    return WM_QUIT;   // boh :D e anche mandarlo a tutte SendMessage(HWND_BROADCAST,WM_QUIT,0,0);
    }

  return 1;
  }

void printWindows(HWND root) {
  HWND myWnd,myWnd2;
  static BYTE level,i;

  level++;
  myWnd=root;
  while(myWnd) {
    for(i=0; i<(level-1)*2; i++)
      putchar(' ');
    printf("Wnd: %X:[%u,%u,%u,%u],%X;%X,%u; %u %u %u %u\n",myWnd,
      myWnd->nonClientArea.left,myWnd->nonClientArea.top,myWnd->nonClientArea.right,myWnd->nonClientArea.bottom,
//      myWnd->clientArea.left,myWnd->clientArea.top,myWnd->clientArea.right,myWnd->clientArea.bottom,
//      myWnd->paintArea.left,myWnd->paintArea.top,myWnd->paintArea.right,myWnd->paintArea.bottom,
      myWnd->style,GetWindowLong(myWnd,0),myWnd->zOrder,
//            myWnd->maximized,myWnd->minimized,myWnd->topmost,myWnd->focus);
      myWnd->active,myWnd->enabled,myWnd->focus,myWnd->internalState);
    printWindows(myWnd->children);
    myWnd=myWnd->next;
    }
  level--;
  }

// ----------------------------------------------------------------------------
//https://github.com/Kraego/STM32-MiniOS/blob/main/Usercode/Concurrency/scheduler.c
// pic32-libs/libpic32/arch/mips/setjmp.S (col cazzo che c'è...)
static THREAD *gRunningThread;
static BYTE addThread(THREAD *thread) {
  THREAD *myThread;
  BYTE n=0;
  
  if(!rootThreads)
    rootThreads=thread;
  else {
    myThread=rootThreads;
    n++;
    while(myThread && myThread->next) {
      myThread=myThread->next;
      n++;
      }
    myThread->next=thread;
    }
// USARE MAX_THREADS!
  return n;
  }
static BOOL removeThread(THREAD *thread) {
  THREAD *myThread,*myThread2;

  if(thread==rootThreads) {
    rootThreads=thread->next;
	  return TRUE;
		}

  myThread=rootThreads;
  while(myThread != thread) {
    myThread2=myThread;
    myThread=myThread->next;
		}
	if(myThread2) {
	  myThread2->next=myThread->next;
		myThread=myThread2->next;
	  return TRUE;
		}

  return FALSE;
  }
THREAD *BeginThread(void *function,BYTE state,DWORD parm) {
  THREAD *thread;
  
	ATOMIC_START();
  thread=malloc(sizeof(THREAD));
  thread->next=NULL;
  thread->context=function;
  thread->priority=THREAD_PTY_MIDDLE;
  thread->state=THREAD_NEW;
  thread->sleepCount=0;
  thread->signals=0;
  thread->pendingIO=0;
  thread->ID=addThread(thread);
  if(thread->ID>=MAX_THREADS) {
    removeThread(thread);
    return NULL;
    }
	ATOMIC_END();
  gRunningThread=thread;
  ((void (*)(DWORD))(thread->context))(parm);    // per primo preset
  
// NON SI PUò FARE LONGJMP A UNA FUNZIONE TERMINATA... per cui va usato ExitThread() e il return non viene mai eseguito
//  https://web.eecs.utk.edu/~mbeck/classes/cs560/360/notes/Setjmp/lecture.html
  // v. sotto: potrebbero esserci dei leak di memoria...
  switch(state) {   // FORZARE automaticamente se è il primo/root? avrebbe senso
    case THREAD_RUNNING:
      setActiveThread(thread);
      longjmp(thread->env,1);
      break;
    case THREAD_SLEEPING:
      break;
    case THREAD_BLOCKED:
      break;
    default:
      gRunningThread=NULL;
      break;
    }
  return thread;
  }
BOOL EndThread(THREAD *thread) {
  
	ATOMIC_START();
  removeThread(thread);
  free(thread);
	ATOMIC_END();
  return TRUE;
  }

void ExitThread(DWORD n) {
  THREAD *thread=GetCurrentThread();
  
	ATOMIC_START();
	//DEBUG_PRINTF("Exiting Thread with id: %d", thread);
    
//  GetCurrentThread()->env=rootThreads->env;
//  gRunningThread->env[11*2]=rootThreads->env[11*2];
//  gRunningThread->env[22*2]=rootThreads->env[22*2];
  
  thread->env[ENV_POSITION_RETURNCODE]=n;   // NATURALMENTE diventa difficile accedere al thread dopo che è stato deallocato :D ! vedere...
  setActiveThread(getNextThread());
  EndThread(thread);
  longjmp(gRunningThread->env, -1);
  // ora va , ma potremmo avere dei leak di memoria..
	ATOMIC_END();
  }

BOOL TerminateThread(THREAD *thread,DWORD ret) {
  
	ATOMIC_START();
	//DEBUG_PRINTF("Finishing Thread with id: %d", gRunningThread);
	thread->state = THREAD_DONE;
  thread->env[ENV_POSITION_RETURNCODE]=ret;
	ATOMIC_END();
  Yield();
  }

void SuspendThread(THREAD *thread) {    // leggermente diversa da Windows...
  
	thread->state = THREAD_BLOCKED;
// mah...  Yield(); direi che serve solo se agisco sul thread in corso
  }

void ResumeThread(THREAD *thread) {    // leggermente diversa da Windows...
  
	thread->state = THREAD_READY;
  }

BOOL GetExitCodeThread(THREAD *t,DWORD *lpExitCode) {
  
  *lpExitCode=t->env[ENV_POSITION_RETURNCODE];      // pescato a caso, un posto dove mettere il valore! v. nelle routine
  }

THREAD *GetThreadFromContext(void *c) {
  THREAD *myThread=rootThreads;
  
  while(myThread) {
		if(myThread->context == c)
      return myThread;
    myThread=myThread->next;
    }
  return NULL;
  }

THREAD_PRIORITY GetThreadPriority(THREAD *t) {
  return t->priority;
  }
BOOL SetThreadPriority(THREAD *t,THREAD_PRIORITY p) {
  if(t) {
    t->priority=p;
    return TRUE;
    }
  else
    return FALSE;
  }
BYTE GetThreadID(THREAD *t) {
  THREAD *myThread=rootThreads;
  BYTE n=0;
  
  while(myThread) {
		if(myThread == t)
      return n;
    myThread=myThread->next;
    }
  return THREAD_ID_INVALID;
  }
BYTE GetCurrentThreadID() {
  return gRunningThread ? GetThreadID(gRunningThread) : THREAD_ID_INVALID;
  }
THREAD *GetThreadFromID(BYTE n) {
  THREAD *myThread=rootThreads;
  
  if(n==THREAD_ID_INVALID)
    return NULL;
  while(n-- && myThread) {
    myThread=myThread->next;
    }
  return myThread;
  }
BOOL GetThreadIOPendingFlag(THREAD *t,BOOL *lpIOIsPending) {
  *lpIOIsPending=t->pendingIO;
  return FALSE;
  }
  
void updateThreads(void) {
  THREAD *myThread=rootThreads;
  
  __delay_ms(TIMESLICE_MS); 
  
  while(myThread) {
		switch(myThread->state) {
      case THREAD_SLEEPING:
        {uint16_t sleep = myThread->sleepCount;
        myThread->sleepCount = sleep >= TIMESLICE_MS ? (sleep-TIMESLICE_MS) : 0;
        if(myThread->sleepCount == 0)
          myThread->state = THREAD_READY;
        }
        break;
    	case THREAD_DONE:
        {THREAD *t=myThread->next;
        EndThread(myThread);    // mah mi sembra giusto
// serve qua? come fare?			  longjmp(gRunningThread->env, -1);
        myThread=t;
        continue;
        }
        break;
      }
    myThread=myThread->next;
    }
  }

THREAD *getActiveThread(void) {
  return gRunningThread;
  }
THREAD *setActiveThread(THREAD *thread) {
  gRunningThread=thread;
  gRunningThread->state = THREAD_RUNNING;
  }
THREAD *getNextThreadPty(THREAD *oldThread,THREAD_PRIORITY myPty) {
  THREAD *myThread;

  myThread=oldThread;
  while(myThread) {
    if((myThread->state==THREAD_READY || myThread->state==THREAD_RUNNING) && myThread->priority==myPty) {
      break;
      }
    myThread=myThread->next;
    }

  return myThread;
  }
THREAD *getNextThread(void) {
  THREAD *myThread;
  static THREAD_PRIORITY myPty=THREAD_PTY_HIGH;
  static THREAD *oldThread;
  THREAD *oldThread2;
  static BYTE ptyCnt=0;
  BYTE foundThread=0;

rifo:
  myThread=oldThread;
	if(myThread)
    myThread=myThread->next;
  else
    myThread=rootThreads;
    
  oldThread2=getNextThreadPty(myThread,myPty);
  
  if(!oldThread2 || oldThread2==oldThread) {
    ptyCnt++;
    if(ptyCnt>6) {    // 0..3 servo gli HIGH, 4..5 i MID e 6 i LOW
      
      myThread=rootThreads;   // se non trovo niente, restituisco il primo a questo livello cmq (o root)
      while(myThread) {
        if(myThread->priority==myPty)
          break;
        myThread=myThread->next;
        }

      ptyCnt=0;
      myPty=THREAD_PTY_HIGH;
      oldThread=NULL;
//      goto rifo;    // questo mi entra in loop all'inizio quando non c'è nessun thread READY...
      return myThread ? myThread : rootThreads;   // root si becca uno slot in più... sistemare (o ok)
      }
    else if(ptyCnt>5) {
      myPty=THREAD_PTY_LOW;
      }
    else if(ptyCnt>3) {
      myPty=THREAD_PTY_MIDDLE;
      }
    oldThread=NULL;
    goto rifo;
    }
  
  oldThread=oldThread2;
  return oldThread;
  }

THREAD *getNextThreadSeq(void) {    // questa cerca semplicemente il thread READY successivo
  THREAD *myThread;

  myThread=rootThreads;
  while(myThread) {
    if(myThread->state == THREAD_RUNNING) {   // diciamo inutile, tutto questo...
      if(myThread == gRunningThread)
        break;
      }
    myThread=myThread->next;
    }

	if(myThread) {
    BYTE done1=FALSE;
    myThread=myThread->next;
    
    do {
      if(myThread == gRunningThread)    // se ho fatto il giro...
        done1=TRUE;
      if(myThread && myThread->state == THREAD_READY) {
        break;
        }
      if(myThread)
        myThread=myThread->next;
      else
        myThread=rootThreads;
      } while(!done1);
    }
  else {
    myThread=rootThreads /*NULL*/;    // NON deve accadere, diciamo! il manager deve sempre essere attivo (magari gestire in Kill/Suspend
    }
  
  return myThread;
  }

void ThreadSleep(uint16_t duration_ms) {  // OVVIAMENTE deve agire solo sul thread in corso!
  
  if(gRunningThread->state>THREAD_NEW && gRunningThread->state<THREAD_DONE) {
    ATOMIC_START();
    gRunningThread->sleepCount = duration_ms;
    gRunningThread->state = THREAD_SLEEPING;
  	ATOMIC_END();
    Yield();
    }
  }

BOOL SwitchToThread(THREAD *t) {
  THREAD *t2=gRunningThread;
// in un VERO multitasking, non si può scegliere a chi passare il controllo...  
// ..per cui sorvoliamo :)  gRunningThread->state=THREAD_READY;
//  getNextThread()->state=THREAD_RUNNING;
  Yield();
  return t2 != gRunningThread;
  }

BOOL SetEvent(THREAD *t,BYTE hEvent) {
  t->signals |= (1 << hEvent);
  return TRUE;
  }
BOOL ResetEvent(THREAD *t,BYTE hEvent) {
  t->signals &= ~(1 << hEvent);
  return TRUE;
  }
DWORD WaitForSingleObject(THREAD *t,BYTE hEvent,DWORD dwMilliseconds) {
  while(dwMilliseconds--) {   // INFINITE è ok :)
    if(t->signals & (1 << hEvent))
      return WAIT_OBJECT_0;
    Yield();
    }
  return WAIT_TIMEOUT;
  }
DWORD WaitForSingleObjectEx(THREAD *t,BYTE hEvent,DWORD dwMilliseconds,BOOL bAutoReset) {
  while(dwMilliseconds--) {   // INFINITE è ok :)
    if(t->signals & (1 << hEvent)) {
      if(bAutoReset)
        t->signals &= ~(1 << hEvent);
      return WAIT_OBJECT_0;
      }
    Yield();
    }
  return WAIT_TIMEOUT;
  }
DWORD WaitForMultipleObjects(THREAD *t,BYTE eventMask,BOOL bWaitAll,DWORD dwMilliseconds) {
  while(dwMilliseconds--) {   // INFINITE è ok :)
    if(bWaitAll) {
      if((t->signals & eventMask) == eventMask)
        return WAIT_OBJECT_0;
        break;
      }
    else {
      if(t->signals & eventMask)
        return WAIT_OBJECT_0 | eventMask;     // boh ok
      }
    Yield();
    }
  return WAIT_TIMEOUT;
  }



HHOOK SetWindowsHookEx(int idHook,HOOKPROC lpfn,HINSTANCE hmod,DWORD dwThreadId) {
  }
BOOL UnhookWindowsHookEx(HHOOK hhk) {
  }
LRESULT CallNextHookEx(HHOOK hhk,int nCode,WPARAM wParam,LPARAM lParam) {
  }

BOOL FileTimeToSystemTime(const FILETIME *lpFileTime,SYSTEMTIME *lpSystemTime) {

  lpSystemTime->wHour=(lpFileTime->dwLowDateTime >> 11) & 31;
  lpSystemTime->wMinute=(lpFileTime->dwLowDateTime >> 5) & 63;
  lpSystemTime->wSecond=(lpFileTime->dwLowDateTime) & 63;
  lpSystemTime->wDay=(lpFileTime->dwHighDateTime) & 31;
  lpSystemTime->wDayOfWeek=((lpFileTime->dwHighDateTime) & 31)  % 7 /*FINIRE!! serve wday*/;
  lpSystemTime->wYear=(lpFileTime->dwHighDateTime >> 5) & 15;
  lpSystemTime->wMonth=(lpFileTime->dwHighDateTime >> 9) + 1980;
  lpSystemTime->wMilliseconds=0;
  return TRUE;
  }
BOOL SetFileTime(SUPERFILE *hFile,const FILETIME *lpCreationTime,
  const FILETIME *lpLastAccessTime,const FILETIME *lpLastWriteTime) {
  if(lpLastAccessTime)
    SuperFileSetTime(hFile,NULL,*(DWORD*)lpLastAccessTime);   // diciamo... sistemare
  if(lpCreationTime)
    SuperFileSetTime(hFile,NULL,*(DWORD*)lpCreationTime); 
  if(lpLastWriteTime)
    SuperFileSetTime(hFile,NULL,*(DWORD*)lpLastWriteTime);
  }
BOOL GetFileTime(SUPERFILE *hFile,FILETIME *lpCreationTime,
  FILETIME *lpLastAccessTime,FILETIME *lpLastWriteTime) {
  FILETIME n;
  
  *(DWORD *)&n=SuperFileGetTime(hFile,NULL);   // diciamo... sistemare
  if(MAKEWORD(n.dwHighDateTime,n.dwLowDateTime) != 0xffffffff) {
    if(lpCreationTime)
      *lpCreationTime=n;
    if(lpLastAccessTime)    // qua non ci sono!
      *lpLastAccessTime=n;
    if(lpLastWriteTime)
      *lpLastWriteTime=n;
    }
  else
    return FALSE;
  }

DWORD __attribute__((weak)) timeGetTime(void) {
  
  ClrWdt();
  return TickGetMilliseconds();
//  return millis;    // con USB va a 1mS... 
  }


HACCEL LoadAccelerators(HINSTANCE hInstance,LPCSTR lpTableName) {
  // fare..
  }

int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;

// mettere!	MyRegisterClass(hInstance);

// mettere!	if(!InitInstance(hInstance, nCmdShow))
//		return FALSE;

  msg.hWnd=(HWND)hInstance;    // PER ORA COSI'!
  
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

	while(GetMessage(&msg, NULL, 0, 0)) {
		if(!TranslateAccelerator(msg.hWnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
      }
    }

	return msg.wParam;
  }


void _swap(UGRAPH_COORD_T *a, UGRAPH_COORD_T *b) {
	UGRAPH_COORD_T t = *a; 
	*a = *b; 
	*b = t; 
	}

inline BOOL __attribute__((always_inline)) PtInRect(const RECT *lprc,POINT pt) {	
  
  if(pt.x>=lprc->left && pt.x<lprc->right &&			// giusta così https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-ptinrect
          pt.y>=lprc->top && pt.y<lprc->bottom)
    return TRUE;
  else
    return FALSE;
  }

inline BOOL __attribute__((always_inline)) wndPtInRect(const RECT *lprc,POINT pt) {	
  
  if(pt.x>=lprc->left && pt.x<=lprc->right &&			// siccome le finestre vanno da 0 a 639 e simili, voglio INCLUDERE i bordi tutti!
          pt.y>=lprc->top && pt.y<=lprc->bottom)
    return TRUE;
  else
    return FALSE;
  }

inline POINT __attribute__((always_inline)) MAKEPOINT(UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  POINT pt={x,y};
  
  return pt;
  }

inline BOOL __attribute__((always_inline)) IsRectEmpty(const RECT *lprc) {
  
  if(lprc->bottom<=lprc->top || lprc->right<=lprc->left)
    return 1;
  else
    return 0;
  }

void setTextSize(BYTE n) {
  
  textsize=n;
  }


static int getParmN(const char *buf,const char *parm,int *val) {
	char *p;

	if((p=strchr(buf,'='))) {
		*p=0;
		if(!stricmp(buf,parm)) {
			if(val)
				*val=atoi(p+1);
			*p='=';
			return 1;
			}
		else			
			*p='=';
		}
	return 0;
	}

static int getParmS(const char *buf,const char *parm,char *val,WORD size) {
	char *p;

	if((p=strchr(buf,'='))) {
		*p=0;
		if(!stricmp(buf,parm)) {
			if(val) {
				strncpy(val,p+1,size);
//        val[size]=0; mettere!
        }
			*p='=';
			return 1;
			}
		else			
			*p='=';
		}
	return 0;
	}

static BYTE getProfileSection(SUPERFILE *f,const char *section) {
  char buffer[32],*p;
  
  while(SuperFileGets(f,buffer,31) >= 0) {
    if(buffer[0]==';') 
      continue;
    if(buffer[0]=='[') {
      if(p=strchr(buffer+1,']')) {
        *p=0;
        if(!stricmp(buffer+1,section))
          return 1;
        }
      }
    }
  return 0;
  }

// occhio che sono diverse da quelle di Windows... https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprofilestringa
BYTE GetPrivateProfileString(const char *section,const char *key,const char *defval,char *val,WORD size,const char *file) {
  SUPERFILE f;
  char buffer[64];
  BYTE n=0;
  VARIABLESTRING *vars;
  
  *val=0;
  vars=findEnvVariable("TEMP");   // trovo il drive di Windows..
  if(vars)
    f.drive=*vars->sval;   
  else
    f.drive=currDrive;
  if(SuperFileOpen(&f,file,'r')) {
    if(!section) {      // restituire tutte...
      }
    else {
      if(getProfileSection(&f,section)) {
        if(!key) {      // restituire tutte...
          }
        else {
    //      SuperFileSeek(&f,0,SEEK_SET);
          while(SuperFileGets(&f,buffer,63) >= 0) {
            if(buffer[0]==';') 
              continue;
            if(getParmS(buffer,key,val,size)) {
              n=1;
              goto fine;
              }
            }
          }
        }
      }
fine:  
    SuperFileClose(&f);
    }
  
  if(!n)
    if(defval)
      strcpy(val,defval);

  return n;
  }

int GetPrivateProfileInt(const char *section,const char *key,int defval,const char *file) {
  SUPERFILE f;
  char buffer[32];
  int n;
  VARIABLESTRING *vars;
  
  vars=findEnvVariable("TEMP");   // trovo il drive di Windows..
  if(vars)
    f.drive=*vars->sval;   
  else
    f.drive=currDrive;
  
  if(SuperFileOpen(&f,file,'r')) {
    if(getProfileSection(&f,section)) {
//      SuperFileSeek(&f,0,SEEK_SET);
      while(SuperFileGets(&f,buffer,31) >= 0) {
        if(buffer[0]==';') 
          continue;
        if(getParmN(buffer,key,&n)) {
          defval=n; // :)
          goto fine;
          }
        }
      }
fine:    
    SuperFileClose(&f);
    }

  return defval;
  }

BYTE WritePrivateProfileString(const char *section,const char *key,const char *val,const char *file) {
  }

BYTE WritePrivateProfileInt(const char *section,const char *key,int val,const char *file) {
  }

BOOL GetFirmwareType(FIRMWARE_TYPE *FirmwareType) {
  *FirmwareType=FirmwareTypeBios;
  return 1;
  }
BOOL SetFirmwareEnvironmentVariable(const char *lpName,const char *lpGuid,void *pValue,DWORD nSize) {
  }
BOOL VerifyVersionInfo(OSVERSIONINFOEX *lpVersionInformation,DWORD dwTypeMask,uint64_t dwlConditionMask) {
  }
BOOL GetUserName(LPSTR lpBuffer,BYTE *pcbBuffer) {
        strncpy(lpBuffer,"dario",*pcbBuffer-1);
  *pcbBuffer=strlen(lpBuffer)+1;
  }
BOOL GetComputerName(LPSTR lpBuffer,BYTE *nSize) {
  strncpy(lpBuffer,BiosArea.NetBIOSName,*nSize-1);   //  diciamo
  *nSize=strlen(lpBuffer)+1;
  }
IP_ADDR GetIPAddress(int8_t n) {
  IP_ADDR ip;
  switch(n) {
    case 0:
      ip.Val=BiosArea.MyIPAddr.Val;
      break;
    case 1:
#ifdef USA_WIFI
      ip.Val=myIp.ip /*BiosArea.MyIPAddr2*/;
#endif
      break;
    default:
// fondamentalmente non c'è un modo decente, per entrambi, per sapere se sono attive..
#if defined(USA_WIFI)
      // gpio
      ip.Val=myIp.ip /*BiosArea.MyIPAddr2.Val*/;
#endif
#if defined(USA_ETHERNET)
      // MACIsLinked(
      ip.Val=BiosArea.MyIPAddr.Val;
#endif
      break;
    }
  return ip;
  }


char *CharLower(char *lpsz) {
  if(HIWORD(lpsz))
    return strlwr(lpsz);
  else
    return (char*)MAKELONG(tolower(*lpsz),0);
  }
char *CharUpper(char *lpsz) {
  if(HIWORD(lpsz))
    return strupr(lpsz);
  else
    return (char*)MAKELONG(toupper(*lpsz),0);
  }
uint16_t CharLowerBuff(char *lpsz,uint16_t cchLength) {
  int n=0;
  while(cchLength--) {
    *lpsz++ |= 0x20;
    n++;
    }
  return n;
  }
uint16_t CharUpperBuff(char *lpsz,uint16_t cchLength) {
  int n=0;
  while(cchLength--) {
    *lpsz++ &= ~0x20;
    n++;
    }
  return n;
  }
char *CharNext(const char *lpsz) {
  return (char*)lpsz+1;    // w.. multibyte UNICODE ecc
  }
char *CharPrev(const char *lpszStart,const char *lpszCurrent) {
  if(lpszCurrent == lpszStart)
    return (char*)lpszStart;
  else
    return (char*)lpszCurrent-1;    // w.. multibyte UNICODE ecc
  }
BOOL CharToOem(const char *pSrc,char *pDst) {
  strcpy(pDst,pSrc);    // w.. multibyte UNICODE ecc
  }


UINT GetWindowModuleFileName(HWND hwnd,char *pszFileName,uint16_t cchFileNameMax) {
  }

BYTE GetTempPath(BYTE nBufferLength,char *lpBuffer) {
  VARIABLESTRING *vars;
  
  vars=findEnvVariable("TEMP");
  if(vars)
    strncpy(lpBuffer,vars->sval,nBufferLength);
  else
    *lpBuffer=0;
  lpBuffer[nBufferLength-1]=0;
  return strlen(lpBuffer);
  }
UINT16 GetTempFileName(char *lpPathName,const char *lpPrefixString,
  UINT16 uUnique,char *lpTempFileName) {
  char buffer[32];
  UINT16 u=uUnique; 
  
  if(lpPathName) {      // mia variante!
    strncpy(buffer,lpPathName,31);
    buffer[31]=0;
    }
  else
    GetTempPath(31,buffer);
  if(!uUnique)
    uUnique=rand();
    uUnique %= 9999;
  if(buffer && *buffer)
    sprintf(lpTempFileName,"%s\\%3s_%04u.tmp",buffer,lpPrefixString,uUnique);
  else
    sprintf(lpTempFileName,"%3s_%04u.tmp",lpPrefixString,uUnique);
  if(!u) //If uUnique is zero, GetTempFileName creates an empty file and closes it. If uUnique is not zero, you must create the file yourself
    ;
  return uUnique;
  }
DWORD GetLogicalDrives(void) {
  DWORD n=0;
  
  if(SDcardOK) {
    n |= 1 << 0;
    }
  if(FDOK) {
    n |= 1 << 1;
    }
  if(HDOK) {
    n |= 1 << 2;
    }
//  n |= 1 << 3;
#if defined(USA_USB_HOST_MSD)
  if(USBmemOK) {
    n |= 1 << 4;
    }
#endif
#ifdef USA_RAM_DISK 
  if(RAMdiscArea) {
    n |= 1 << 17;
    }
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
  if(1   ) {
    n |= 1 << 25;
    }
#endif
  return n;
  }
BOOL GetVolumeInformation(const char *lpRootPathName,char *lpVolumeNameBuffer,
  DWORD nVolumeNameSize, DWORD *lpVolumeSerialNumber,
  DWORD *lpMaximumComponentLength,DWORD *lpFileSystemFlags,
  char *lpFileSystemNameBuffer,BYTE nFileSystemNameSize) {
  
  
  switch(*lpRootPathName) {
    case 'A':
      if(SDcardOK) {
        }
      break;
    case 'B':
      if(FDOK) {
        }
      break;
    case 'C':
//#warning messo MOUNT in C: ecc
      if(HDOK) {
        }
      break;
    case 'D':
      break;
#if defined(USA_USB_HOST_MSD)
    case 'E':
      if(USBmemOK) {
        }
      break;
#endif
#ifdef USA_RAM_DISK 
    case DEVICE_RAMDISK:
      if(RAMdiscArea) {
        }
      break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
    case DEVICE_NETWORK:
      if(1   ) {
        }
      break;
#endif
    }
  
  }


BOOL ModifyStyle(HWND hWnd,DWORD dwRemove, DWORD dwAdd, UINT nFlags /*=0*/) { // http://www.icodeguru.com/vc&mfc/mfcreference/html/_mfc_cwnd.3a3a.modifystyle.htm

  SetWindowLong(hWnd,GWL_STYLE,(GetWindowLong(hWnd,GWL_STYLE) & ~dwRemove) | dwAdd);
  if(nFlags)
    SetWindowPos(hWnd,NULL,0,0,0,0,nFlags | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  return 1;
  }


BOOL OpenClipboard(HWND hWndNewOwner) {
  clipboard.owner=hWndNewOwner;
  }
BOOL CloseClipboard() {
//  clipboard.owner=NULL;
  }
BOOL EmptyClipboard() {
  clipboard.data=NULL;
  clipboard.format=0;
  }
HANDLE SetClipboardData(UINT uFormat,HANDLE hMem) {
  clipboard.data=hMem;
  clipboard.format=uFormat;
  SendMessage(HWND_BROADCAST,WM_CLIPBOARDUPDATE,0,0); 
  }
HANDLE GetClipboardData(UINT uFormat) {
  if(clipboard.format==uFormat)
    return clipboard.data;
  }
HWND GetClipboardOwner() {
  return clipboard.owner;
  }
int CountClipboardFormats() {
  if(clipboard.data)
    return 1;
  else
    return 0;
  }
UINT EnumClipboardFormats(UINT format) {
  if(clipboard.data && clipboard.format==format)
    return 1;
  else
    return 0;
  }
int GetClipboardFormatName(UINT format,LPSTR lpszFormatName,int cchMaxCount) {
  }

BYTE enterScreenSaver(void) {
	HWND myWnd=rootWindows;
  
  CreateSprite(0,0,0,0,0,0,0,0   | 8,SPRITE_SETATTR);    
	while(myWnd) {
		myWnd->locked=1;
		if(myWnd->internalState==MSGF_MENU)
			myWnd->internalState=0;
		// mandare exitmenuloop, exitidle ?
		myWnd=myWnd->next;
		}
  desktopWindow->locked=0;   // mah per sicurezza!
  activeMenu=activeMenuParent=NULL;   // e idem
  activeMenuCntX=activeMenuCntY=0;
  activeMenuWnd=NULL;
  SetRectEmpty(&activeMenuRect);
	inScreenSaver=1;
  if(GetWindowByte(taskbarWindow,GWL_USERDATA+0) & 1)
    KillTimer(taskbarWindow,1);
  // e anche task manager... e in generale?
  if(myWnd=GetFocus())
    drawCaret(myWnd,caretPosition.x,caretPosition.y,NULL,FALSE);    // e si dovrebbe pure fermare il timer di chi lo visualizza...
	}
BYTE exitScreenSaver(void) {
	HWND myWnd=rootWindows;
  
  // PASSWORD protected? :D
  
	inScreenSaver=0;
	while(myWnd) {
		myWnd->locked=0;
		myWnd=myWnd->next;
		}
  InvalidateRect(NULL,NULL,TRUE);
  CreateSprite(0,0,0,0,0,0,0,1   | 8,SPRITE_SETATTR);    
  if(GetWindowByte(taskbarWindow,GWL_USERDATA+0) & 1) {
    SetTimer(taskbarWindow,1,60000,NULL);
//    SendMessage(taskbarWindow,WM_TIMER,0,0);      // per forzare update orologio!
    }
	}

HINSTANCE ShellExecute(HWND hWnd,const char *lpOperation,const char *lpFile,
  const char *lpParameters,const char *lpDirectory,BYTE nShowCmd) {
  
  if(!stricmp(lpOperation,"clock")) {
    if(!m_WndClock) {
      WNDCLASS *clockWC;
      clockWC=malloc(sizeof(WNDCLASS));
      clockWC->class=MAKECLASS(MAKEFOURCC('C','L','O','K'));
      clockWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS  | CS_DROPSHADOW;
      clockWC->icon= NULL /*standardIcon*/;
      clockWC->cursor=hourglassCursorSm;
      clockWC->cbClsExtra=0;
      clockWC->cbWndExtra=0;
  extern LRESULT clockWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
      clockWC->lpfnWndProc=(WINDOWPROC *)clockWndProc;
      clockWC->hbrBackground=CreateSolidBrush(RED);
      clockWC->menu=NULL  /* ev menu per internet time ecc*/;
      if(!RegisterClass(clockWC))
        free(clockWC);
      // metterla in basso a dx!
      m_WndClock=CreateWindow(MAKECLASS(MAKEFOURCC('C','L','O','K')),"Orologio",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE |
        WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
        CW_USEDEFAULT,CW_USEDEFAULT,99,109,
        hWnd,NULL,(void*)lpParameters
        );
      }
    return m_WndClock ? 1 : 0;
    }
  else if(!stricmp(lpOperation,"calendar")) {
    if(!m_WndCalendar) {
      WNDCLASS *calendarWC;
      calendarWC=malloc(sizeof(WNDCLASS));
      calendarWC->class=MAKECLASS(MAKEFOURCC('C','L','D','R'));
      calendarWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS  | CS_DROPSHADOW;
      calendarWC->icon= NULL /*standardIcon*/;
      calendarWC->cursor=hourglassCursorSm;
      calendarWC->cbClsExtra=0;
      calendarWC->cbWndExtra=0;
  extern LRESULT calendarWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
      calendarWC->lpfnWndProc=(WINDOWPROC *)calendarWndProc;
      calendarWC->hbrBackground=CreateSolidBrush(WHITE);
      calendarWC->menu=NULL  /* ev menu per internet time ecc*/;
      if(!RegisterClass(calendarWC))
        free(calendarWC);
      // metterla in basso a dx!
      m_WndCalendar=CreateWindow(MAKECLASS(MAKEFOURCC('C','L','D','R')),"Calendario",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE | 
        WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
        CW_USEDEFAULT,CW_USEDEFAULT,99,109,
        hWnd,NULL,(void*)lpParameters
        );
      }
    return m_WndCalendar ? 1 : 0;
    }
  else if(!stricmp(lpOperation,"calc")) {
  extern LRESULT calcWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
    m_WndCalc=CreateWindow(MAKECLASS(WC_DEFAULTCLASS),"Calcolatrice",WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX |
      WS_SYSMENU | WS_VISIBLE | WS_CLIPCHILDREN,
      CW_USEDEFAULT,CW_USEDEFAULT,122,144,
      NULL,(MENU *)NULL,(void*)1
      );
    m_WndCalc->windowProc=(WINDOWPROC*)calcWndProc;    // 
    SendMessage(m_WndCalc,WM_CREATE,0,0);   // per wndproc; cfr internalCreateWindow
    }
  else if(!stricmp(lpOperation,"basic")) {
    WNDCLASS *miniBasicWC;
    miniBasicWC=malloc(sizeof(WNDCLASS));
    miniBasicWC->class=MAKECLASS(MAKEFOURCC('M','B','A','S'));
    miniBasicWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
  extern const MENU minibasicMenu;
  extern const GFX_COLOR minibasicIcon[];
  LRESULT minibasicWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
  int minibasic(MINIBASIC *hInstance,const char *scriptName,HWND hWnd);
  int basic(MINIBASIC *mInstance,const char *script,BYTE where,HWND hWnd);

    miniBasicWC->icon=minibasicIcon;
    miniBasicWC->cursor=standardCursorSm;
    miniBasicWC->cbClsExtra=0;
    miniBasicWC->cbWndExtra=4+sizeof(MINIBASIC);
    miniBasicWC->lpfnWndProc=(WINDOWPROC *)minibasicWndProc;
    miniBasicWC->hbrBackground=CreateSolidBrush(BLACK);
    miniBasicWC->menu=(MENU*)&minibasicMenu;
    if(!RegisterClass(miniBasicWC))
      free(miniBasicWC);
    
    m_WndBasic=CreateWindow(MAKECLASS(MAKEFOURCC('M','B','A','S')),"MiniBASIC",WS_BORDER | WS_CAPTION | WS_VISIBLE |
      WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
      CW_USEDEFAULT,CW_USEDEFAULT,199,159,
      NULL,(MENU *)NULL,(void*)lpFile
      );
    
    MINIBASIC *minstance=(MINIBASIC*)GET_WINDOW_OFFSET(m_WndBasic,4);
    DWORD script=GetWindowLong(m_WndBasic,0);
extern const char demoscript[],demoscript2[],demoscript3[],demoscript4[];
    if(script) {
      if(HIWORD(script)) {
        char *prg=(char*)script;
        SetWindowText(m_WndBasic,prg);
        basic(minstance,prg,1,m_WndBasic);
        }
      else {
        SetWindowText(m_WndBasic,script==4 ? "MiniBasic - first prg" : 
              (script==3 ? "MiniBasic - trigircle" : (script==2 ? "MiniBasic - demoscrpt2" : "MiniBasic - demoscript")));

        basic(minstance,script==4 ? demoscript4 : 
          (script==3 ? demoscript3 : (script==2 ? demoscript2 : demoscript)),0,hWnd);
        }
      }
    else
      minibasic(minstance,NULL,m_WndBasic);
      
    return 1;
    
    }
  else if(!stricmp(lpOperation,"rexx")) {
    }
  else if(!stricmp(lpOperation,"forth")) {
    }
  else if(!stricmp(lpOperation,"explore")) {
    if(lpFile) {
      return ShellExecute(hWnd,"open",lpFile,NULL,NULL,SW_SHOWNORMAL);
      }
    else {
      m_WndFileManager[0]=CreateWindow(MAKECLASS(WC_DIRCLASS),"Disco",WS_BORDER | WS_CAPTION | WS_VISIBLE |
        WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
        CW_USEDEFAULT,CW_USEDEFAULT,239,159,
        NULL,(MENU *)&explorerMenu,(void*)lpDirectory
        );    // ovviamente potremmo averne più d'una di queste!
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(m_WndFileManager[0],0);
      dfn->disk=0; *dfn->path=0;
      if(lpParameters) {
        dfn->disk=*lpParameters;
        }
      if(dfn->disk)   // se dischi, parto con icone grandi altrimenti piccole!
        SetWindowByte(m_WndFileManager[0],GWL_USERDATA+1,0);
      else
        SetWindowByte(m_WndFileManager[0],GWL_USERDATA+1,1);
//fare         SendMessage(m_Wnd,WM_CHOOSE_FOLDERDISK,(DWORD)disk,(DWORD)path);
      }
    return 1;
    }
  else if(!stricmp(lpOperation,"open")) {
    char *p=NULL;
    int i;
    if(!lpFile) {
      lpFile=strchr(lpOperation,' ');
      if(lpFile)
        lpFile++;
      p=strchr(lpOperation,'/');
      if(p)
        p++;
      }
    i=getFileInfo(lpFile,NULL,0);
    switch(i) {
      case TIPOFILE_BASIC:
        return ShellExecute(hWnd,"basic",lpFile,p,lpDirectory,nShowCmd);
        break;
      case TIPOFILE_REXX:
        return ShellExecute(hWnd,"rexx",lpFile,p,lpDirectory,nShowCmd);
        break;
      case TIPOFILE_FORTH:
        return ShellExecute(hWnd,"forth",lpFile,p,lpDirectory,nShowCmd);
        break;
      case TIPOFILE_LINK:
        {
        char buffer[32]={0};
        SUPERFILE f;
        if(lpDirectory)
          getDrive(lpDirectory,&f,NULL);
        else
          f.drive=currDrive;
        if(SuperFileOpen(&f,lpFile,'r')) {
          SuperFileGets(&f,buffer,31);
          SuperFileClose(&f);
          }
        return ShellExecute(hWnd,buffer,NULL,NULL,lpDirectory,nShowCmd);
        }
        break;
      case TIPOFILE_IMMAGINE:
        return ShellExecute(hWnd,"view",lpFile,p,lpDirectory,nShowCmd);
        break;
      case TIPOFILE_TESTO:
      case TIPOFILE_INI:
        return ShellExecute(hWnd,"notepad",lpFile,p,lpDirectory,nShowCmd);
        break;
      case TIPOFILE_AUDIO:
        {int n=0;
          
        if(!m_WndPlayer) {
          m_WndPlayer=CreateWindow(MAKECLASS(WC_PLAYERCLASS),"Player",WS_BORDER | WS_CAPTION | 
            WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE,
            CW_USEDEFAULT,CW_USEDEFAULT,159,99 /*CYMENU*/,
            NULL,(MENU *)NULL,(void*)lpFile
            ); 
          }
        else {
          SendMessage(m_WndPlayer,WM_PLAYER_OPEN,0,(LPARAM)lpFile);
          ShowWindow(m_WndPlayer,nShowCmd);
          UpdateWindow(m_WndPlayer);
          }
          
        uint32_t offset=0;
        int readBytes;
        uint8_t buffer[1152*4];   // VERIFICARE!
        SUPERFILE f;
        if(lpDirectory)
          getDrive(lpDirectory,&f,NULL);
        else
          f.drive=currDrive;
        if(SuperFileOpen(&f,lpFile,'r')) {
  //        uint16_t sampling_rate = mp3_get_sampling_rate();
  //        BYTE channels = mp3_get_channel_mode() == mp3_Mono ? 1 : 2;
          
          if(stristr(lpFile,".MP3")) {
            SetAudioMode(SAMPLES);
            WAVE_SAMPLES_FORMAT wsf;
            wsf.channels=1;
            wsf.bytesPerSample=1;
            wsf.samplesPerSec=44100;
            wsf.bufferSize=1024;
            wsf.buffers=1;
            wsf.active=1; wsf.loop=1;
            wsf.notify=1;
            SetAudioSamplesFormat(&wsf);
          readBytes=SuperFileRead(&f,((uint8_t*)buffer),1152);
          if(readBytes < 1152)
            goto mp3_fine;
mp3_read:
          KBClear();
          offset=0;
          mp3_mp3(&buffer[offset]);
          while(readBytes > (offset + mp3_get_header_size() + mp3_get_frame_size())) {
mp3_resync:              
            mp3_init_header_params(&buffer[offset]);
            if(mp3_is_valid()) {
              mp3_init_frame_params(&buffer[offset]);
              offset += mp3_get_frame_size();
              }
            else {
              offset++;
              mp3_mp3(&buffer[offset]);
              continue;
              }

            audio_played=FALSE;
            if(mp3_is_valid()) { //all'ultimo si pianta... forse mp3_get_channels=0? NO boh
              SetAudioSamples(0, 0, (BYTE*)mp3_get_samples_as_int(),2* 576*2*mp3_get_channels());
              while(!audio_played)
                handle_events();
              }
// ci mette 200mS circa invec di 20-25 come dovrebbe (4500bytes @44100*2*2/sec
            printf("MP3 frame %u(%u):%u  ofs %u\n",++n,readBytes,timeGetTime(),offset);
            }


  // decimare, lampeggiare, fare finestra avanzamento        
      KBClear();
      handle_events();

      handle_more_events();

          if(keypress.key == 0x1b)
            goto mp3_fine;

          goto mp3_read;

  mp3_fine:        
        ;
        }
      else if(stristr(lpFile,".WAV")) {
        struct WAV_HEADER wh;
        struct WAV_HEADER_FACT whf;
        struct WAV_HEADER_DATA whd;
          //        leggere header RIFF ecc!!
        readBytes=SuperFileRead(&f,&wh,sizeof(struct WAV_HEADER));
        readBytes=SuperFileRead(&f,&whf,sizeof(struct WAV_HEADER_FACT));
        readBytes=SuperFileRead(&f,&whd,sizeof(struct WAV_HEADER_DATA));

        SetAudioMode(SAMPLES);
        WAVE_SAMPLES_FORMAT wsf;
        wsf.channels=1;
        wsf.bytesPerSample=1;
        wsf.samplesPerSec=22050;
        wsf.bufferSize=512;
        wsf.buffers=1;
        wsf.active=1; wsf.loop=1;
        wsf.notify=1;
        SetAudioSamplesFormat(&wsf);
        for(;;) {
          readBytes=SuperFileRead(&f,buffer,512);    // usare buffer diverso!
          if(readBytes <=0)
            break;
      SetAudioSamples(0, 0, (BYTE*)buffer, 512);

   audio_played=FALSE;
        printf("WAV frame %u\n",readBytes);
   while(!audio_played)
    handle_events();
        
  // decimare, lampeggiare, fare finestra avanzamento        
      KBClear();
      handle_events();

      handle_more_events();

              if(keypress.key == 0x1b)
                break;
              }
    
            }
          SuperFileClose(&f);
          }
        }
        break;
      case TIPOFILE_INTERNET:
        return ShellExecute(hWnd,"surf",lpFile,p,NULL,nShowCmd);
        break;
      case TIPOFILE_PROGRAMMA:
        execCmd(lpFile,NULL,p);   // funziona, ma scrive su schermo :) aprire una finestra dosshell...?
        break;
      default:
        MessageBox(hWnd,lpFile,"tipo file sconosciuto",MB_OK);
        break;
      }
    }
  else if(!stricmp(lpOperation,"view")) {
    char nomefile[64]={0};
    if(lpDirectory) {
      strncpy(nomefile,lpDirectory,31);
      nomefile[31]=0;
      }
    if(lpFile) {
      strcat(nomefile,lpFile);
      }
    if(!m_WndViewer) {
      m_WndViewer=CreateWindow(MAKECLASS(WC_VIEWERCLASS),"Viewer",WS_BORDER | WS_CAPTION | 
        WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT,CW_USEDEFAULT,159,169 /*CYMENU*/,
        NULL,(MENU *)NULL,(void*)nomefile
        ); 
      }
    else {
      SendMessage(m_WndViewer,WM_VIEWER_OPEN,0,(DWORD)nomefile);
      ShowWindow(m_WndViewer,nShowCmd);
      UpdateWindow(m_WndViewer);
      }
    }
  else if(!stricmp(lpOperation,"print")) {
    }
  else if(!stricmp(lpOperation,"taskman")) {
    m_WndTaskManager=CreateWindow(MAKECLASS(WC_TASKMANCLASS),"Task Manager",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_VISIBLE | 
      WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VSCROLL   | WS_EX_TOPMOST /*opzionale :)*/,
      100,60,199,159,   // CENTRARE!
      NULL,(MENU *)NULL,(void*)0
      );
    return m_WndTaskManager ? 1 : 0;
    }
  else if(!stricmp(lpOperation,"control")) {
    if(!m_WndControlPanel)
      m_WndControlPanel=CreateWindow(MAKECLASS(WC_CONTROLPANELCLASS),"Pannello di controllo",WS_BORDER | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE |
        WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
        CW_USEDEFAULT,CW_USEDEFAULT,219,159,
        NULL,(HMENU)&menuControlPanel,(void*)lpFile
        );
    return m_WndControlPanel ? 1 : 0;
    }
  else if(!stricmp(lpOperation,"dosshell")) {
    m_WndDOS = CreateWindow(MAKECLASS(WC_CMDSHELL),"DOS shell",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE |
      WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
      240,30,219,149,
      NULL,(MENU *)NULL,(void*)lpFile
      );
    return 1;
    }
  else if(!stricmp(lpOperation,"surf")) {
    if(!m_WndSurf) {
      WNDCLASS *surfWC;
      surfWC=malloc(sizeof(WNDCLASS));
      surfWC->class=MAKECLASS(MAKEFOURCC('S','U','R','F'));
      surfWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
  extern const MENU surfMenu;
  extern const GFX_COLOR surfIcon[],surfWaitIcon[];
  extern const char *surfApp;
  LRESULT surfWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
      surfWC->icon=surfIcon;
      surfWC->cursor=standardCursorSm;
      surfWC->cbClsExtra=0;
      surfWC->cbWndExtra=sizeof(struct HTMLinfo) + HTML_SIZE;   // dati pagina web, pagina web (in USERDATA attrib, SOCKET, socket-tipo,stato
      surfWC->lpfnWndProc=(WINDOWPROC *)surfWndProc;
      surfWC->hbrBackground=CreateSolidBrush(GRAY096);
      surfWC->menu=(MENU*)&surfMenu;
      if(!RegisterClass(surfWC))
        free(surfWC);
      m_WndSurf=CreateWindow(MAKECLASS(MAKEFOURCC('S','U','R','F')),surfApp,WS_BORDER | WS_CAPTION | WS_VISIBLE |
        WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT,CW_USEDEFAULT,299,239,
        NULL,(MENU *)NULL,(void*)lpFile
        );
      
      SetWindowByte(GetDlgItem(m_WndSurf,204),GWL_USERDATA,0b00000101);   // prova! status led OCCHIO se togli status!
      }
    else {
      if(lpFile)
        SendMessage(m_WndSurf,WM_SURF_NAVIGATE,0,(LPARAM)lpFile);
      }
    return 1;
    }
  else if(!stricmp(lpOperation,"notepad")) {
    if(!m_WndNotepad) {
      WNDCLASS *notepadWC;
      notepadWC=malloc(sizeof(WNDCLASS));
      notepadWC->class=MAKECLASS(MAKEFOURCC('N','O','T','E'));
      notepadWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
  extern const MENU notepadMenu;
  extern const GFX_COLOR notepadIcon[];
  extern const char *notepadApp;
  LRESULT notepadWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
      notepadWC->icon=notepadIcon;
      notepadWC->cursor=standardCursorSm;
      notepadWC->cbClsExtra=0;
      notepadWC->cbWndExtra=4+32;   // handle memoria testo per Edit; nomefile
      notepadWC->lpfnWndProc=(WINDOWPROC *)notepadWndProc;
      notepadWC->hbrBackground=CreateSolidBrush(GRAY096);
      notepadWC->menu=(MENU*)&notepadMenu;
      if(!RegisterClass(notepadWC))
        free(notepadWC);
      m_WndNotepad=CreateWindow(MAKECLASS(MAKEFOURCC('N','O','T','E')),notepadApp,WS_BORDER | WS_CAPTION | WS_VISIBLE |
        WS_OVERLAPPEDWINDOW | WS_SYSMENU,
        CW_USEDEFAULT,CW_USEDEFAULT,219,199,
        NULL,(MENU *)NULL,(void*)lpFile
        );
      }
    else {
      if(lpFile)
        SendMessage(m_WndNotepad,WM_NOTEPAD_OPEN,0,(LPARAM)lpFile);
      }
    
    return 1;
    }
  else  // tutto il resto, in particolare il contenuto dei file .LNK
    return ShellExecute(hWnd,lpFile,NULL,NULL,lpDirectory,nShowCmd);
  return 0;
  }

BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX *picce) {
  return 1; //:)
  }

  
int MulDiv(int nNumber,int nNumerator,int nDenominator) {
  
  if(nDenominator)
    return ((uint64_t)nNumber*nNumerator)/nDenominator;
  else
    return -1;
  }
uint64_t UnsignedMultiply128(uint64_t Multiplier,uint64_t Multiplicand,uint64_t *HighProduct) {
  }
uint64_t Multiply128(int64_t Multiplier,int64_t Multiplicand,int64_t *HighProduct) {
  }
uint64_t PopulationCount64(uint64_t operand) {
  BYTE i=64,n=0;
  while(i--) {
    if(operand & 1)
      n++;
    operand >>= 1;
    }
  return n;
  }
uint64_t ShiftLeft128(uint64_t LowPart,uint64_t HighPart,BYTE Shift) {
  }
uint64_t ShiftRight128(uint64_t LowPart,uint64_t HighPart,BYTE Shift) {
  }
uint64_t UnsignedMultiplyHigh(uint64_t Multiplier,uint64_t Multiplicand) {
  }

BOOL StrTrim(LPSTR psz, LPCSTR pszTrimChars) {    // per ora solo spazi 
  char *str,*str2,*str3;
  
  while(*psz && isspace(*psz))
		strcpy(psz,psz+1);
  while(*psz)
		psz++;
	psz--;
  while(isspace(*psz))
		psz--;
  *(psz+1) = 0;
  return 1;
  }


void DrawWindow(UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2,GFX_COLOR f,GFX_COLOR b) {
  BYTE parms[18];
  
  parms[0]=1;   // cmd
  parms[1]=LOBYTE(x1);
  parms[2]=HIBYTE(x1);
  parms[3]=LOBYTE(y1);
  parms[4]=HIBYTE(y1);
  parms[5]=LOBYTE(x2);
  parms[6]=HIBYTE(x2);
  parms[7]=LOBYTE(y2);
  parms[8]=HIBYTE(y2);
  parms[9]=LOBYTE(f);
  parms[10]=HIBYTE(f);
  parms[11]=LOBYTE(b);
  parms[12]=HIBYTE(b);
  
  CNNEFbits.CNNEF1=0;
  WritePMPs(VIDEO_CARD,BIOS_VIDEO_WINDOWS,parms,17);
  while(!m_VACK)
    ClrWdt();
  __delay_ns(VIDEO_WAIT);
  CNNEFbits.CNNEF1=1;
  }

void MovePointer(UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  BYTE parms[18];
  
  parms[0]=10;   // cmd
  parms[1]=LOBYTE(x);
  parms[2]=HIBYTE(x);
  parms[3]=LOBYTE(y);
  parms[4]=HIBYTE(y);
  
  CNNEFbits.CNNEF1=0;
  WritePMPs(VIDEO_CARD,BIOS_VIDEO_WINDOWS,parms,17);
  while(!m_VACK)
    ClrWdt();
  __delay_ns(VIDEO_WAIT);
  CNNEFbits.CNNEF1=1;
  }

