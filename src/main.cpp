#include <windows.h>
#include <shlobj.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <Richedit.h>

#include <combaseapi.h>

#include <objbase.h>
#include <INITGUID.H>

#include <memory>
#include <locale>
#include <map>
#include <algorithm>
#include <forward_list>
#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}

extern "C" {
#include "breakpath.h"
#include "stringchains.h"
#include "images.h"
#include "dupstr.h"
#include "arrayarithmetic.h"
#include "bytearithmetic.h"
#include "portables.h"
#include "tfiles.h"

//! TEMP:
#include "errorf.h"

//! TODO: remove from all files
#include "prgdir.h"

}
//! TEMP
#define errorf_old(...) g_errorf(__VA_ARGS__)

#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

#include "userinterface.hpp"
#include "uiutils.hpp"
#include "portables.hpp"
#include "indextools.hpp"
#include "prgdir.hpp"

std::filesystem::path g_fsPrgDir;
//! TODO: remove from all files
char *g_prgDir;


#define IDM_FILE_MIMAN 1
#define IDM_FILE_DMAN 2

#define MAX_NROWS 3 // upping this to 1000 later
#define MAX_NTHUMBS 10
#define BASE_WMEM_SIZE 4

enum {WM_U_SL = WM_USER+3, WM_U_RET_SL, WM_U_TL, WM_U_RET_TL, WM_U_PL, WM_U_RET_PL, WM_U_DMAN, WM_U_RET_DMAN, WM_U_TMAN, WM_U_RET_TMAN, WM_U_VI, WM_U_RET_VI, WM_U_RET_SBAR, WM_U_TOP, WM_U_INIT, WM_U_TED, WM_U_RET_TED, WM_U_LISTMAN, WM_U_RET_LISTMAN };

enum { LISTMAN_REFRESH = 0, LISTMAN_CH_PAGE_REF, LISTMAN_CH_PAGE_NOREF, LISTMAN_SET_DEF_WIDTHS, LISTMAN_LEN_REF, LISTMAN_RET_SEL_POS };

enum { PL_REFRESH };

enum { SL_CHANGE_PAGE };

/*
#define WM_U_SL WM_APP-1
#define WM_U_TL WM_APP-3
#define WM_U_PL WM_APP-4
#define WM_U_DMAN WM_APP-2
#define WM_U_TMAN WM_APP-5
#define WM_U_VI WM_APP-6
#define WM_U_SBAR WM_APP-7
#define WM_U_TOP WM_APP-8
#define WM_U_INIT WM_APP-9
*/

#define DL_NUM_UE_COL 0x00d1d1d1 // BGR
#define DL_NUM_E_COL 0x00dbdbdb
#define DL_SL_COL 0x00d6ad6b
#define DL_SA_COL 0x00e8c178
#define DL_DIR_UE_COL 0x00e3e3e3
#define DL_DIR_E_COL 0x00eaeaea
#define DL_BORDER_COL 0x00BEBEBE

#define DMAN_TOP_MRG 38
#define DMAN_BOT_MRG 21
#define SBAR_H 30
#define STRLIST_TOP_MRG 17
#define ROW_HEIGHT 16
#define THMBLIST_TOP_MRG 5
#define THMBLIST_LEFT_MRG 5

#define CHAR_WIDTH 7	// predictive
#define PAGE_PAD 12		// both sides combined
#define PAGES_SPACE 10
#define PAGES_SIDEBUF 12
#define DEF_THUMBW 100
#define DEF_THUMBH 100
#define DEF_THUMBFW 110
#define DEF_THUMBFH 110
#define DEF_THUMBGAPX 2
#define DEF_THUMBGAPY 1

#define IMGEXTS ".bmp.gif.jpeg.jpg.png"

//#define HWND_BUF_SIZE sizeof(HWND)
#define HWND_BUF_SIZE 256

HINSTANCE ghInstance;

DWORD color;
HFONT hFont, hFont2, hFontUnderlined;
HPEN bgPen1, bgPen2, bgPen3, bgPen4, bgPen5, selPen, selPen2, hPen1;
HBRUSH bgBrush1, bgBrush2, bgBrush3, bgBrush4, bgBrush5, selBrush;
HBITMAP hListSliceBM, hThumbListBM;
WNDPROC g_OldEditProc;
struct wMemEntry **wMemArray;
uint64_t wMemArrLen, nwMemWnds;

HCURSOR hDefCrs, hCrsSideWE, hCrsHand;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void DelRer(HWND, uint8_t, STRLISTV const*);
int CleanEditText(HWND);

void dialogf(HWND, char*, ...);

char keepremovedandadded(oneslnk *origchn, char *buf, oneslnk **addaliaschn, oneslnk **remaliaschn, uint8_t presort);

int CheckInitMutex(HANDLE &hMutex);
int MainInit(struct MainInitStruct &ms);
int MainDeInit(struct MainInitStruct &ms);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
	ghInstance = hInstance;

	int i = 0;
	struct MainInitStruct ms = {};
	if ((i = MainInit(ms)) != 0) {
		if (i == 2) { // mutex already reserved
			return 0;
		} else {
			errorf_old("Preinit: MainInit failed: %d", i);
			return 1;
		}
	}

	HWND hMsgHandler = MsgHandler::createWindowInstance(WinInstancer(0, L"FileTagIndex", WS_OVERLAPPEDWINDOW, 100, 100, 100, 100, HWND_MESSAGE, NULL, hInstance, NULL));

	// try to prevent program opening and taking mutex with no real window
	if (!hMsgHandler || !WindowClass::getWindowPtr(hMsgHandler)) {
		errorf("Failed to create MsgHandler (main)");
		return 1;
	}

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if ((i = MainDeInit(ms)) != 0) {
		errorf_old("MainDeInit failed: %d", i);
		return 1;
	}

	return (int) msg.wParam;
}

int MainInit(struct MainInitStruct &ms) {
	if (ms != MainInitStruct()) {
		errorf("Preinit: MainInitStruct was not empty");
		return 1;
	}

	if (PrgDirInit() != true) {
		errorf("Preinit: PrgDirInit failed");
		return 1;
	}
	//! TODO: remove the c_string PrgDir from every file
	//! TODO:  remove the use of dupstr and the ugly cast
	g_prgDir = dupstr((char *) g_fsPrgDir.string().c_str(), 32767*4, 0);
	if (g_prgDir == nullptr) {
		errorf("Preinit: g_PrgDir init failed");
		return 1;
	}

	if (CheckInitMutex(ms.hMutex) != 0) { // requires PrgDir to be inited
		errorf("Preinit: CheckInitMutex failed");
	}

	if (ms.hMutex == NULL) {
		return 2;
	}
errorf("maininit 2");
	{
		HMODULE hModule = LoadLibraryW(L"Msftedit.dll");
		if (hModule == NULL) {
			errorf("failed to load Msftedit.dll");
			return 3;
		}
	}

errorf("maininit 3");
	{
		bool b1 = EditSuperClass::helper.registerWindowClass();
		if (!b1) {
			errorf("ESC helper registerWindowClass failed");
			return 3;
		}
	}

errorf("maininit 4");

	hDefCrs = LoadCursor(NULL, IDC_ARROW);
	hCrsSideWE = LoadCursor(NULL, IDC_SIZEWE);
	hCrsHand = LoadCursor(NULL, IDC_HAND);

	hFont = CreateFontW(14, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, 0, 0, L"Consolas");
	hFontUnderlined = CreateFontW(14, 0, 0, 0, FW_MEDIUM, 0, 1, 0, 0, 0, 0, 0, 0, L"Consolas");
	hFont2 = CreateFontW(16, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, 0, 0, L"Helvetica");
	color = GetSysColor(COLOR_BTNFACE);
	if (!(hPen1 = CreatePen(PS_SOLID, 0, DL_BORDER_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(bgPen1 = CreatePen(PS_SOLID, 0, color))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(bgPen2 = CreatePen(PS_SOLID, 0, DL_NUM_UE_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(bgPen3 = CreatePen(PS_SOLID, 0, DL_NUM_E_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(bgPen4 = CreatePen(PS_SOLID, 0, DL_DIR_UE_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(bgPen5 = CreatePen(PS_SOLID, 0, DL_DIR_E_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(selPen = CreatePen(PS_SOLID, 0, DL_SL_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(selPen2 = CreatePen(PS_SOLID, 0, DL_SA_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(bgBrush1 = CreateSolidBrush(color))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(bgBrush2 = CreateSolidBrush(DL_NUM_UE_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(bgBrush3 = CreateSolidBrush(DL_NUM_E_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(bgBrush4 = CreateSolidBrush(DL_DIR_UE_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(bgBrush5 = CreateSolidBrush(DL_DIR_E_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(selBrush = CreateSolidBrush(DL_SA_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}

	return 0;

}

int CheckInitMutex(HANDLE &hMutex) {
	wchar_t *mutex_name = 0;
	{
		char buf1[MAX_PATH*4];
		char buf2[MAX_PATH*4];
		sprintf(buf2, "%s", g_prgDir);
		int i = 0;
		while (buf2[i] != '\0') {
			if (buf2[i] == '\\') buf2[i] = '/';
			i++;
		}
		sprintf(buf1, "Global\\%s", buf2);
		mutex_name = wide_from_mb(buf2);
	}
	hMutex = CreateMutexW(NULL, TRUE, mutex_name);

	if (mutex_name)  {
		free(mutex_name);
	}

	if (hMutex == NULL) {
		errorf_old("Preinit: CreateMutex error: %d\n", GetLastError());
		return 1;
	}

	DWORD dwWaitResult = WaitForSingleObject(hMutex, 0);

	char disableMutexCheck = 0;

	if (!disableMutexCheck && dwWaitResult != WAIT_OBJECT_0 && dwWaitResult != WAIT_ABANDONED) { // if mutex is already taken
		CloseHandle(hMutex);
		hMutex = NULL;

		//! TODO:
		wchar_t *file_map_name = 0;

		char buf[MAX_PATH*4];
		char buf2[MAX_PATH*4];
		sprintf(buf2, "%s", g_prgDir);
		int i = 0;
		while (buf2[i] != '\0') {
			if (buf2[i] == '\\') buf2[i] = '/';
			i++;
		}
		sprintf(buf, "%s/HWND", buf2);
		file_map_name = wide_from_mb(buf);

		HANDLE hMapFile = OpenFileMappingW(
			 FILE_MAP_ALL_ACCESS, // read/write access
			 FALSE, // do not inherit the name
			 file_map_name);

		free(file_map_name);

		if (hMapFile == NULL) {
			errorf_old("Preinit: Could not open file mapping object (%d).\n", GetLastError());
			return 1;
		}

		LPCTSTR pBuf = (LPTSTR) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, HWND_BUF_SIZE);

		if (pBuf == NULL) {
			errorf_old("Preinit: Could not map view of file (%d).\n", GetLastError());
			CloseHandle(hMapFile);

			return 1;
		}

		HWND thwnd;

		CopyMemory((PVOID) &thwnd, (VOID*) pBuf, sizeof(HWND));

		UnmapViewOfFile(pBuf);

		CloseHandle(hMapFile);

		SendMessage(thwnd, WM_U_TOP, (WPARAM) 0, (LPARAM) 0);
	}

	return 0;
}

int MainDeInit(struct MainInitStruct &ms) {
	if (ms.hMutex != NULL) {
		CloseHandle(ms.hMutex);
	}

	SetCursor(hDefCrs);
	if (!(DestroyCursor(hCrsSideWE))) {
		errorf("DestroyCursor failed");
	}
	if (!(DestroyCursor(hCrsHand))) {
		errorf("DestroyCursor failed");
	}
	if (hListSliceBM) {
		if (!(DeleteObject(hListSliceBM))) {
			errorf("DeleteObject failed");
		} hListSliceBM = 0;
	}
	if (hThumbListBM) {
		if (!(DeleteObject(hThumbListBM))) {
			errorf("DeleteObject failed");
		} hThumbListBM = 0;
	}
	if (!(DeleteObject(hFont))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgPen1))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgPen2))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgPen3))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgPen4))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgPen5))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(selPen))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(selPen2))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgBrush1))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgBrush2))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgBrush3))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgBrush4))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(bgBrush5))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(selBrush))) {
		errorf("DeleteObject failed");
	}


	// NOTE: this means g_PrgDir can't be used in deconstructors of static objects
	//! TODO: remove g_prgDir from all files
	if (g_prgDir == NULL) {
		free(g_prgDir);
		g_prgDir = NULL;
	}

	return 0;
}



#define BIT_VI_FIT_MODE_ENABLED (1ULL << 0)
#define BIT_VI_FIT_DISP_IS_ORIG (1ULL << 1)
#define BIT_VI_FIT_RESIZE_FITS_X (1ULL << 2)
#define BIT_VI_FIT_RESIZE_FITS_Y (1ULL << 3)



//{ WindowClass
//public:
//static
std::shared_ptr<WindowClass> WindowClass::getWindowPtr(HWND hwnd) {
	auto findIt = winMemMap.find(hwnd);
	if (findIt == winMemMap.end()) {
		//errorf("couldn't find window pointer");
		if (hwnd == 0) {
			errorf("tried to get pointer for hwnd: 0");
		}
		return std::shared_ptr<WindowClass>();
	}
	return findIt->second;
}

//static
std::shared_ptr<WindowClass> WindowClass::createWindowMemory(HWND hwnd, WinCreateArgs &winArgs) {
	if (hwnd == 0) {
		errorf("tried to register hwnd: 0");
	} else {
		auto resPair = winMemMap.emplace(hwnd, std::shared_ptr<WindowClass>(winArgs.constructor()));
		if (resPair.first == winMemMap.end()) {
			errorf("failed to emplace windowMemory");
		} else if (resPair.second == false) {
			errorf("windowMemory already registered");
		} else {
			auto elemPair = *resPair.first;

			if (!elemPair.second) {
				errorf("registered nullptr to windowMemory");
			}

			return elemPair.second;
		}
	}
	return std::shared_ptr<WindowClass>();
}

//static
boolean WindowClass::releaseWindowMemory(HWND hwnd) {
	if (hwnd == 0) {
		errorf("tried to release hwnd: 0");
	} else {
		int c = winMemMap.erase(hwnd);

		if (!c) {
			errorf("failed to release hwnd");
		} else {
			return true;
		}
	}
	return false;
}

//static
LRESULT CALLBACK WindowClass::generalWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	std::shared_ptr<WindowClass> winPtr = WindowClass::getWindowPtr(hwnd);
	if (!winPtr) {
		// the subclassed RichEdit does initialization in WM_NCCREATE before WM_CREATE
		if (msg == WM_NCCREATE) {
			WinCreateArgs *winArgs = *(WinCreateArgs **) lParam;
			if (winArgs == NULL) {
				errorf("creating window class without lpParam");
				return 0;
			}

			winPtr = WindowClass::createWindowMemory(hwnd, *winArgs);
			// modifying (*lParam) here doesn't seem to carry over to WM_CREATE
			*((void **)lParam) = 0;
		} else if (msg == WM_CREATE) {
			errorf("winPtr was null for WM_CREATE");
			return -1;
		} else {
			return DefWindowProcW(hwnd, msg, wParam, lParam);
		}
	}

	if (msg == WM_CREATE) {
		WinCreateArgs *winArgs = *(WinCreateArgs **) lParam;
		if (winArgs == NULL) {
			errorf("creating window class without lpParam");
			return -1;
		}

		*((void **)lParam) = winArgs->lpParam;
	}

	LRESULT res = winPtr->winProc(hwnd, msg, wParam, lParam);
	if (msg == WM_NCDESTROY) {
		WindowClass::releaseWindowMemory(hwnd);
	}
	return res;
}

std::map<HWND, std::shared_ptr<WindowClass>> WindowClass::winMemMap;

//} WindowClass

//{ Non-WindowClass

// WinCreateArgs
WinCreateArgs::WinCreateArgs(WindowClass *(*constructor_)(void)) : constructor{*constructor_}, lpParam{0} {}

/*
WindowClass *WinCreateArgs::create(HWND hwnd) {
	return StoreWindowInstance(hwnd, this->constructor);
}
*/


// WinInstancer
WinInstancer::WinInstancer(DWORD dwExStyle_, LPCWSTR lpWindowName_, DWORD dwStyle_, int X_, int Y_, int nWidth_, int nHeight_, HWND hWndParent_, HMENU hMenu_, HINSTANCE hInstance_, LPVOID lpParam_) : dwExStyle{dwExStyle_}, lpWindowName{lpWindowName_}, dwStyle{dwStyle_}, X{X_}, Y{Y_}, nWidth{nWidth_}, nHeight{nHeight_}, hWndParent{hWndParent_}, hMenu{hMenu_}, hInstance{hInstance_}, lpParam{lpParam_} {}

HWND WinInstancer::create(std::shared_ptr<WinCreateArgs> winArgs, const std::wstring &winClassName) {
	winArgs->lpParam = lpParam;

	return CreateWindowExW(dwExStyle, winClassName.c_str(), lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, winArgs.get());
}


// WindowHelper
WindowHelper::WindowHelper(const std::wstring winClassName, void (*modifyWinStruct)(WNDCLASSW &wc)) : winClassName_{winClassName} {
	WNDCLASSW wc = {0};

	wc.style = 0;
	wc.hbrBackground = 0;
	wc.hCursor = NULL;

	modifyWinStruct(wc);

	wc.lpszClassName = this->winClassName_.c_str();
	wc.lpfnWndProc = WindowClass::generalWinProc;
	wc.hInstance = ghInstance;

	int c = RegisterClassW(&wc);
	if (!c) {
//! TODO: add collision detection and crash on collision
		errorf("RegisterClassW failed");
	}
}

WindowHelper::WindowHelper(const std::wstring winClassName_) : winClassName_{winClassName_} {}

bool WindowHelper::registerWindowClass() {}

// DeferredRegWindowHelper
DeferredRegWindowHelper::DeferredRegWindowHelper(const std::wstring winClassName_, void (*modifyWinStruct_)(WNDCLASSW &wc)) : modifyWinStruct{modifyWinStruct_}, isRegistered_(false), WindowHelper(winClassName_) {}

bool DeferredRegWindowHelper::registerWindowClass() {
	if (!this->isRegistered_) {

		WNDCLASSW wc = {0};

		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = NULL;

		modifyWinStruct(wc);

		wc.lpfnWndProc = WindowClass::generalWinProc;
		wc.lpszClassName = this->winClassName_.c_str();
		wc.hInstance = ghInstance;

		int c = RegisterClassW(&wc);
		if (!c) {
			errorf("RegisterClassW failed");
			int error1 = GetLastError();
			g_errorfStream << error1 << std::flush;
			return false;
		} else {
			isRegistered_ = true;
			return true;
		}
	}
	errorf("RWC already registered");
	g_errorfStream << "tried to register: " << u16_to_u8(std::wstring(winClassName_)) << std::flush;
	return true;
}

bool DeferredRegWindowHelper::isRegistered(void) const {
	return isRegistered_;
}

//} Non-WindowClass

//{ MsgHandler
//subclass of WindowClass
//public:
// static
const WindowHelper MsgHandler::helper = WindowHelper(std::wstring(L"MsgHandlerClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND MsgHandler::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new MsgHandler(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK MsgHandler::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int i;
	HWND thwnd;

	switch(msg) {

		case WM_CREATE: {
			thwnd = TabWindow::createWindowInstance(WinInstancer(0, L"FileTagIndex", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 500, NULL, NULL, ghInstance, NULL));

			if (!thwnd)	{
				errorf("failed to create TabWindow");
				return -1;
			}
			ShowWindow(thwnd, SW_MAXIMIZE);

			{
				//wchar_t *file_map_name = 0;

				std::string s = std::string(g_prgDir);

				std::replace(s.begin(), s.end(), '\\', '/');

				s += "/HWND";

				// errorf("file_map_name is:" + s);

				/*
				char buf[MAX_PATH*4];
				char buf2[MAX_PATH*4];
				sprintf(buf2, "%s", g_prgDir);
				int i = 0;
				while (buf2[i] != '\0') {
					if (buf2[i] == '\\') buf2[i] = '/';
					i++;
				}

				sprintf(buf, "%s/HWND", buf2);
				file_map_name = wide_from_mb(buf);
				*/
				//! TODO:
				const wchar_t *file_map_name = u8_to_u16(s).c_str();

				hMapFile = std::shared_ptr<void>(
					CreateFileMappingW(
						INVALID_HANDLE_VALUE, // use paging file
						NULL,  // default security
						PAGE_READWRITE, // read/write access
						0,  // maximum object size (high-order DWORD)
						256, // maximum object size (low-order DWORD)
						file_map_name  // name of mapping object
					), CloseHandle
				);

				//// should be embedded in std::wstring
				//free(file_map_name);

				if (hMapFile == NULL) {
					errorf_old("Could not create file mapping object (%d).\n", GetLastError());
					return -1;
				}
				//LPTSTR pBuf = (LPTSTR) MapViewOfFile(hMapFile.get(), FILE_MAP_ALL_ACCESS, 0, 0, HWND_BUF_SIZE);
				std::shared_ptr<void> pBuf = std::shared_ptr<void>(MapViewOfFile(hMapFile.get(), FILE_MAP_ALL_ACCESS, 0, 0, HWND_BUF_SIZE), UnmapViewOfFile);

				if (pBuf == NULL) {
					errorf_old("Could not map view of file (%d).\n", GetLastError());
					//CloseHandle(hMapFile);
					return -1;
				}

				CopyMemory((PVOID)(pBuf.get()), (VOID*) &hwnd, sizeof(HWND));

				//UnmapViewOfFile(pBuf);
			}

			break;
		}
		case WM_U_TOP: {
			errorf("received WM_U_TOP");
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);

			return 0;
		}
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}
//} MsgHandler

//{ TabWindow
//public:
// static
const WindowHelper TabWindow::helper = WindowHelper(std::wstring(L"TabWindowClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hbrBackground = 0;
		wc.hCursor = LoadCursor(0, IDC_ARROW);
	}
);

// static
HWND TabWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new TabWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK TabWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static HWND wHandle[2];
	static HWND wHandle2[1];
	static unsigned int ndirman = 2;
	static unsigned int nthmbman = 1;
	static int lastoption, lastdnum;
	static HWND dispWindow;
	HMENU hMenubar;
	HMENU hMenu;
	int i;
	RECT rect;
	char *buf;

	HPEN hOldPen;
	HBRUSH hOldBrush;
	HDC hdc;
	PAINTSTRUCT ps;

	switch(msg) {

		case WM_CREATE: {

			hMenubar = CreateMenu();
			hMenu = CreateMenu();

			//AppendMenuW(hMenu, MF_STRING, IDM_FILE_DMAN, L"&Manage Directories");
			AppendMenuW(hMenu, MF_STRING, IDM_FILE_MIMAN, L"&Manage Main Indices");
			AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR) hMenu, L"&File");

			SetMenu(hwnd, hMenubar);

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			HWND thwnd = 0;
			dispWindow = thwnd = TabClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, rect.right, rect.bottom, hwnd, (HMENU) 3, NULL, NULL));

			if (!thwnd) {
				errorf("failed to create TabClass");
				return -1;
			}

			break;
		}
		case WM_PAINT: //{
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			hdc = BeginPaint(hwnd, &ps);
			hOldBrush = (HBRUSH) SelectObject(hdc, bgBrush1);
			hOldPen = (HPEN) SelectObject(hdc, bgPen1);
			Rectangle(hdc, 0, 0, rect.right, rect.bottom);
			SelectObject(hdc, hOldBrush);
			SelectObject(hdc, hOldPen);
			EndPaint(hwnd, &ps);
			break;
		//}
		case WM_SIZE: //{
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			MoveWindow(dispWindow, 0, 0, rect.right, rect.bottom, 0);
		//}
		case WM_SETFOCUS: //{
			SetFocus(dispWindow);

			break;
		//}
		case WM_COMMAND: //{

			if ((wParam & (256U*256-1)*256*256) == 0 && lParam == 0) {
				switch(LOWORD(wParam)) {
				case IDM_FILE_MIMAN:
					if (!wHandle[0]) {
						lastoption = 0;
						//DmanClass::createWindowInstance(WinInstancer(0, L"Manage Directories", WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_SIZEBOX | WS_POPUP, 200, 200, 300, 300, hwnd, 0, ghInstance, 0));
						MainIndexManClass::createWindowInstance(WinInstancer(0, L"Manage Main Indices", WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_SIZEBOX | WS_POPUP, 200, 200, 300, 300, hwnd, 0, ghInstance, 0));
					} else {
						if (!BringWindowToTop(wHandle[0])) {
							errorf("BringWindowToTop for wHandle[0] failed");
						}
					}
					break;
				}
				break;
			}

			break;
		//}
		case WM_ERASEBKGND: //{
			return 1;
		//}
		case WM_DESTROY: //{

			PostQuitMessage(0);

			return 0;
		//}
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//} TabWindow

//{ TabClass
//public:
// static
const WindowHelper TabClass::helper = WindowHelper(std::wstring(L"TabClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hbrBackground = 0;
		wc.hCursor = LoadCursor(0, IDC_ARROW);
	}
);

// static
HWND TabClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new TabClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK TabClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static HWND wHandle[2];
	static HWND wHandle2[1];
	static unsigned int ndirman = 2;
	static unsigned int nthmbman = 1;
	static int lastoption, lastdnum;
	HMENU hMenubar;
	HMENU hMenu;
	int i;
	RECT rect;
	char *buf;
	HWND thwnd;

	HPEN hOldPen;
	HBRUSH hOldBrush;
	HDC hdc;
	PAINTSTRUCT ps;

	switch(msg) {

		case WM_CREATE: {
			HWND thwnd;
			thwnd = CreateWindowW(L"Button", L"Open Dir", WS_VISIBLE | WS_CHILD , 5, 5, 80, 25, hwnd, (HMENU) 1, NULL, NULL);
			SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont2, 1);
			if (!thwnd) return -1;

			{
				int y1 = 20;

				//thwnd = CreateWindowExW(0, MSFTEDIT_CLASS, L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 5, 35, 380, y1, hwnd, (HMENU) 2, NULL, NULL);
errorf("creating editsuperclass outer");
				thwnd = EditSuperClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 5, 35, 380, y1, hwnd, (HMENU) 2, NULL, NULL));
				if (!thwnd) {
					errorf("couldn't create editwindow");
				}
				thwnd = EditSuperClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 5, 35 + y1 + 5, 380, 300, hwnd, (HMENU) 3, NULL, NULL));
				if (!thwnd) {
					errorf("couldn't create editwindow 2");
				}
			}

			break;
		}
		case WM_PAINT: //{
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			hdc = BeginPaint(hwnd, &ps);
			hOldBrush = (HBRUSH) SelectObject(hdc, bgBrush1);
			hOldPen = (HPEN) SelectObject(hdc, bgPen1);
			Rectangle(hdc, 0, 0, rect.right, rect.bottom);
			SelectObject(hdc, hOldBrush);
			SelectObject(hdc, hOldPen);
			EndPaint(hwnd, &ps);
			break;
		//}
		case WM_SIZE: //{
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			MoveWindow(this->dispWindow, 0, 0, rect.right, rect.bottom, 0);
		//}
		case WM_SETFOCUS: //{
			if (this->dispWindow) {
				SetFocus(this->dispWindow);
			}

			break;
		//}
		case WM_COMMAND: //{

			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) == 1) {

					//! TODO: set to MIMANARGS
					DIRMANARGS args = {};
					args.option = 2;
					args.parent = hwnd;

					thwnd = MainIndexManClass::createWindowInstance(WinInstancer(0, L"Select Index", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 100, 100, 500, 500, hwnd, NULL, ghInstance, (LPVOID) &args));
				}
			}

			break;
		//}
		case WM_KEYDOWN: //{
			switch(wParam) {
			case VK_BACK:
				DestroyWindow(this->dispWindow);

				ShowWindow(GetDlgItem(hwnd, 1), 1);
				ShowWindow(GetDlgItem(hwnd, 2), 1);
				ShowWindow(GetDlgItem(hwnd, 3), 1);
				this->dispWindow = 0;

				break;
			}
			break;
		//}
		case WM_U_RET_DMAN: //{
		//!
		/*

			switch(wParam) {
			case 0: // dman termination, lParam is hwnd
				for (i = 0; i < ndirman; i++) {
					if ((HWND) lParam == wHandle[i]) {
						wHandle[i] = 0;
						break;
					}
				} if (i == ndirman) {
					errorf("didn't zero wHandle");
				}
				break;
			case 3: // selected directory
//				errorf_old("returned: %d", (int) lParam);
				if ((int) lParam == 0)
					break;
				lastdnum = (int) lParam;
				lastoption = 0;
				buf = dread(lastdnum);
				if (buf != NULL && existsdir(buf)) {
					THMBMANARGS args = {};
					args.parent = hwnd;
					args.option = 0;
					args.dnum = lastdnum;

//					thwnd = ThumbManClass::createWindowInstance(WinInstancer(0, L"Thumbnail View", WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_SIZEBOX | WS_POPUP, 200, 200, 300, 300, hwnd, 0, ghInstance, &args));

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					thwnd = ThumbManClass::createWindowInstance(WinInstancer(0, L"Thumbnail View", WS_VISIBLE | WS_CHILD, 0, 0, rect.right, rect.bottom, hwnd, 0, ghInstance, &args));
					ShowWindow(GetDlgItem(hwnd, 1), 0);
					ShowWindow(GetDlgItem(hwnd, 2), 0);
					ShowWindow(GetDlgItem(hwnd, 3), 0);
					this->dispWindow = thwnd;
				} else {
					if (buf != NULL) {
						dialogf(hwnd, "Couldn't find directory: %s.", buf);
						free(buf);
					} else {
						dialogf(hwnd, "Directory is null.");
					}
				}

				break;
			}

			break;
		*/
		//}
		case WM_U_RET_TMAN: //{

			switch(wParam) {
			case 0:
				for (i = 0; i < nthmbman; i++) {
					if ((HWND) lParam == wHandle2[i]) {
						wHandle2[i] = 0;
						break;
					}
				} if (i == nthmbman) {
					errorf("didn't close wHandle2");
				}
				break;
			}
			break;
		//}
		case WM_ERASEBKGND: //{
			return 1;
		//}
		case WM_NCDESTROY: //{
			//FreeWindowMem(hwnd);

			PostQuitMessage(0);

			return 0;
		//}
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}



//} TabClass


//{ ListManClass


uint64_t ListManClass::getSingleSelID(void) const {
	ErrorObject retError;
	auto retStr = this->slv->getSingleSelRowCol(0, &retError);

	if (retError) {
g_errorfStream << "has nothing = " << retError.hasNothing() << std::flush;
		errorf("getSingleSelRowCol failed");
		g_errorfStream << retError << std::flush;
		return 0;
	}

	uint64_t retUint = stringToUint(retStr, &retError);

	if (retError) {
		errorf("stringToUint failed");
		return 0;
	}

	return retUint;
}


LRESULT CALLBACK ListManClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch(msg) {
		case WM_CREATE: {

			break;
		}
		case WM_U_LISTMAN: {

			switch(wParam) {

				case LISTMAN_REFRESH:	{	// refresh rows
					//this->slv->nRows = MAX_NROWS;
					PostMessage(hwnd, WM_USER+1, 0, 0);

					InvalidateRect(hwnd, NULL, 1);

					break;
				}
				case LISTMAN_CH_PAGE_REF:	{	// change page and refresh
					if (lParam != 0) {
						SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_CH_PAGE_NOREF, lParam);
					}
					PostMessage(hwnd, WM_USER+1, 0, 0);
					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_SET_DEF_WIDTHS, 0);
					SendDlgItemMessageW(hwnd, 3, WM_U_SL, SL_CHANGE_PAGE, 0);

					break;
				}
				case LISTMAN_CH_PAGE_NOREF:	{ // change page without refreshing (when the window will be refreshed soon anyway)

					this->plv->curPage = lParam;

					SendDlgItemMessageW(hwnd, 4, WM_U_PL, PL_REFRESH, 0);

					break;
				}
				case LISTMAN_SET_DEF_WIDTHS: {
					uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;
					int32_t i = log10(fNum+MAX_NROWS-1)+1;
					if ( !this->slv->setColumnWidth(0, CHAR_WIDTH*(i)+6) ) {
						errorf("setColumnWidth failed (0)");
						return -1;
					}
					if ( !this->slv->setColumnWidth(1, 0) ) {
						errorf("setColumnWidth failed (1)");
						return -1;
					}

					InvalidateRect(hwnd, NULL, 1);

					break;

				}

				case LISTMAN_LEN_REF: { 	// redetermine last page and refresh pages

					uint64_t lastlastpage = this->plv->lastPage;
					int32_t j = 0, x = 0;

					uint64_t ll = this->getNumElems();
g_errorfStream << "got getNumElems: " << ll << std::flush;
					this->plv->lastPage = ll/MAX_NROWS + !!(ll % MAX_NROWS);
					if (this->plv->lastPage == 0) {
						this->plv->lastPage = 1;
					}

					if (this->plv->curPage > this->plv->lastPage) {
						this->plv->curPage = this->plv->lastPage;
						j = 1;
					}

					if (this->plv->lastPage != lastlastpage) {
g_errorfStream << "set lastpage to " << this->plv->lastPage << std::flush;
						if (j == 1) {
							SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_CH_PAGE_NOREF, this->plv->curPage);
						}
						this->plv->hovered = 0;
						SetCursor(hDefCrs);
						SendDlgItemMessageW(hwnd, 4, WM_USER, 0, 0);

						SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);

					} else {
						SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
					}

					break;
				}
				//! TODO: move to protected function
				case LISTMAN_RET_SEL_POS: { 	// return selected row num
					// error if more than one selected
					if (this->option == 2) {
						uint64_t ull = this->getSingleSelID();

						EnableWindow(this->parent, 1);
						PostMessage(this->parent, WM_U_RET_DMAN, 3, (LPARAM) ull);
						DestroyWindow(hwnd);
					}

					break;
				}
			}
			break;
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);

}

//} ListManClass

//{ MainIndexManClass
//public:
// static
const WindowHelper MainIndexManClass::helper = WindowHelper(std::wstring(L"MainIndexManClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND MainIndexManClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new MainIndexManClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

int MainIndexManClass::menuCreate(HWND hwnd) {
	HMENU hMenu;

	if (this->option != 2) {
		hMenu = CreatePopupMenu();
		AppendMenuW(hMenu, MF_STRING, 1, L"&Remove");	// if menu handles are changed, the message from WM_COMMAND needs to be changed to correspond to the right case
		AppendMenuW(hMenu, MF_STRING, 2, L"&Rename");
		AppendMenuW(hMenu, MF_STRING, 3, L"&Manage Directories");
		AppendMenuW(hMenu, MF_STRING, 4, L"&Manage Subdirectories");
		AppendMenuW(hMenu, MF_STRING, 5, L"&Manage Files");
		AppendMenuW(hMenu, MF_STRING, 6, L"&Manage Tags");

		TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, this->slv->point.x, this->slv->point.y, 0, hwnd, 0);
		DestroyMenu(hMenu);
	}

	return 0;
};

int MainIndexManClass::menuUse(HWND hwnd, int32_t menu_id) {
	errorf_old("menu_id is: %d", menu_id);

	uint64_t fNum;

	if (this->option != 2) {
		switch (menu_id) {
		case 1:
		case 2:
			if (this->option != 2) {
				fNum = (this->plv->curPage-1)*MAX_NROWS+1;
				//DelRer(hwnd, menu_id, &this->slv);
				SendMessage(hwnd, WM_USER, 0, 0);
				SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
			}
			break;
		case 3:
			{

				DIRMANARGS args = {0};
				args.option = 0;
				args.parent = hwnd;
				args.inum = this->getSingleSelID();
				HWND thwnd = 0;
				// errorf_old("inum: %llu", args->inum);
				if (args.inum != 0) {
					thwnd = DmanClass::createWindowInstance(WinInstancer(0, L"Manage Directories", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 100, 100, 500, 500, hwnd, NULL, ghInstance, (LPVOID) &args));
					if (!thwnd) {
						errorf("failed to create DmanClass from MainIndexManClass");
					}
				}
			}
			break;
		case 4:
			{

				SDIRMANARGS *args = (SDIRMANARGS *) calloc(1, sizeof(SDIRMANARGS));
				args->option = 0;
				args->parent = hwnd;
				args->inum = this->getSingleSelID();
				HWND thwnd = 0;
				// errorf_old("inum: %llu", args->inum);
				if (args->inum != 0) {
					thwnd = SDmanClass::createWindowInstance(WinInstancer(0, L"Manage Subdirectories", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 100, 100, 500, 500, hwnd, NULL, ghInstance, (LPVOID) args));
				}
				free(args);
			}
			break;
		}
	} else {

	}
	return 0;
};

uint64_t MainIndexManClass::getNumElems() {
	return getlastminum();
}

LRESULT CALLBACK MainIndexManClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	/*
	char *p;
	wchar_t *wp;
	int i, j, x;
	uint64_t ll, lastlastpage, fNum;
	RECT rect;
	HWND thwnd;
	oneslnk *link, *link2;
	*/

	switch(msg) {
		case WM_CREATE: {

			this->plv = NULL;
			this->slv = NULL;

			MIMANARGS *args = *((MIMANARGS **)lParam);

			if (args) {
				this->option = args->option;
				this->parent = args->parent;
			} else {
				this->option = 0;
				this->parent = 0;
			}

			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			HWND thwnd = NULL;
			uint64_t ll = 0;
			int32_t i = 0;

			if (this->option != 2) {
				int32_t x = rect.right / 2 - 100;
				if (x < 5)
					x = 5;
				CreateWindowW(L"Button", L"Add...", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, NULL, NULL);
				SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont2, 1);
				//CreateWindowW(L"Button", L"Relative path", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, x+90, 2, 110, 35, hwnd, (HMENU) 2, NULL, NULL);
				//SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) hFont2, 1);
				//CheckDlgButton(hwnd, 2, BST_UNCHECKED);
			} else {
				int32_t x = (rect.right-80) / 2;
				if (x < 5)
					x = 5;
				EnableWindow(this->parent, 0);
				CreateWindowW(L"Button", L"Select", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, NULL, NULL);
				SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont2, 1);
			}

			thwnd = PageListClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 4, NULL, NULL));
			if (!thwnd) {
				errorf("failed to create PageListClass");
				return -1;
			}

			this->plv = std::dynamic_pointer_cast<PageListClass>(WindowClass::getWindowPtr(thwnd));
			if (!this->plv) {
				errorf("failed to receive PageListClass pointer");
				return -1;
			}

			thwnd = StrListClass::createWindowInstance(WinInstancer(0,  0, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 3, NULL, NULL));
			if (!thwnd) {
				errorf("failed to create StrListClass");
				return -1;
			}

			this->slv = std::dynamic_pointer_cast<StrListClass>(WindowClass::getWindowPtr(thwnd));
			if (!this->slv) {
				errorf("failed to receive StrListClass pointer");
				return -1;
			}

			//! TODO: move this to the createWindowInstance arguments
			if (this->option == 2) {
				this->slv->option = 1;
			}

			this->plv->curPage = 1;
			ll = this->getNumElems();
g_errorfStream << "getNumElems: " << ll << std::flush;
			this->plv->lastPage = ll/MAX_NROWS + !!(ll % MAX_NROWS);
			if (this->plv->lastPage == 0) {
				this->plv->lastPage = 1;
			}

			this->plv->startPaint();

			i = MAX_NROWS/8 + !!(MAX_NROWS%8);
			if (!i) {
				errorf("no space row selections");
			}

			this->slv->setNColumns(2);
			uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;
			i = log10(fNum+MAX_NROWS-1)+1;
			if ( !this->slv->setColumnWidth(0, CHAR_WIDTH*(i)+6) ) {
				errorf("setColumnWidth failed (0)");
				return -1;
			}
			if ( !this->slv->setColumnWidth(1, 0) ) {
				errorf("setColumnWidth failed (1)");
				return -1;
			}

			if ( !this->slv->setHeaders(std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>( { "#", "Name" }))) ) {
				errorf("failed to set slv headers");
				return -1;
			}

			SetFocus(GetDlgItem(hwnd, 3));

			SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
errorf("miman 6");

			break;
		}
		case WM_COMMAND: //{

			if (this->option != 2) {
				if (HIWORD(wParam) == 0 && lParam == 0) { // context menu
					//SendMessage(hwnd, WM_U_RET_SL, 3, LOWORD(wParam);
					this->menuUse(hwnd, LOWORD(wParam));
					break;
				} else if (HIWORD(wParam) == BN_CLICKED) {
					if (LOWORD(wParam) == 1) {	// add listing

						HWND *arg = (HWND*) malloc(sizeof(HWND));
						TextEditDialogClass::createWindowInstance(WinInstancer(0, L"Enter Main Index Name", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 150, 150, 500, 500, hwnd, 0, NULL, arg));
						//SendMessage(thwnd, WM_USER, 0, (LPARAM) this->dnum);
						free(arg);
					}
				}
			} else {
				if (HIWORD(wParam) == BN_CLICKED) {

					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_RET_SEL_POS, 0);

				}
			}

			break;
		//}
		case WM_SIZE: {
			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			if (this->option != 2) {
				int32_t x = rect.right / 2 - 100;
				if (x < 5)
					x = 5;

				{
					HWND thwnd = GetDlgItem(hwnd, 1);
					MoveWindow(thwnd, x, 5, 80, 25, 0);
				}
				// {
				//	HWND thwnd = GetDlgItem(hwnd, 2);
				//	MoveWindow(thwnd, x+90, 2, 110, 35, 0);
				// }
			} else {
				int32_t x = (rect.right-80) / 2;
				if (x < 5)
					x = 5;
				HWND thwnd = GetDlgItem(hwnd, 1);
				MoveWindow(thwnd, x, 5, 80, 25, 0);
			}

			{
				HWND thwnd = GetDlgItem(hwnd, 3);
				MoveWindow(thwnd, 0, DMAN_TOP_MRG, rect.right, rect.bottom-DMAN_TOP_MRG-(!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, 0);
			} {
				HWND thwnd = GetDlgItem(hwnd, 4);
				MoveWindow(thwnd, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, 0);
			}

			break;
		}
		case WM_PAINT: {
errorf("miman paint 1");

			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			HPEN hOldPen;
			HBRUSH hOldBrush;
			HDC hdc;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hwnd, &ps);

			hOldPen = (HPEN) SelectObject(hdc, bgPen1);
			hOldBrush = (HBRUSH) SelectObject(hdc, bgBrush1);

			Rectangle(hdc, 0, 0, rect.right, DMAN_TOP_MRG);
			SelectObject(hdc, hPen1);
			MoveToEx(hdc, 0, 0, NULL);
			LineTo(hdc, rect.right, 0);
			MoveToEx(hdc, 0, DMAN_TOP_MRG-1, NULL);
			LineTo(hdc, rect.right, DMAN_TOP_MRG-1);

			if (this->plv->lastPage > 1) {
				MoveToEx(hdc, 0, rect.bottom-DMAN_BOT_MRG, NULL);
				LineTo(hdc, rect.right, rect.bottom-DMAN_BOT_MRG);
			}
			SelectObject(hdc, hOldPen);
			SelectObject(hdc, hOldBrush);
			EndPaint(hwnd, &ps);

			break;
		}
		case WM_MOUSEMOVE: {

			this->plv->hovered = 0;
			SetCursor(hDefCrs);

			break;
		}
		case WM_SETFOCUS: {
			SetFocus(GetDlgItem(hwnd, 3));

			break;
		}
		case WM_USER+1: { 		// refresh num and dir strings
errorf("miman u+1 -- 1");

			this->slv->clearRows();
errorf("miman u+1 -- 2");

			uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;

			std::shared_ptr<std::forward_list<std::string>> list_ptr = imiread(fNum, MAX_NROWS);
errorf("miman u+1 -- 3");

			if (list_ptr != nullptr && !list_ptr->empty()) {
errorf("miman u+1 -- 4");
				auto workPtr = std::make_shared<std::vector<std::vector<std::string>>>();
				if (workPtr == nullptr) {
					errorf("failed to allocate shared_ptr");
					return 1;
				}
				auto &workVector = *workPtr;

				uint64_t uint = fNum;

				for (auto &entry : *list_ptr) {
					workVector.emplace_back( std::vector<std::string>( { std::to_string(uint), entry } ) );

					uint++;
				}

				int64_t gotRows = uint - fNum;

				if (gotRows <= 0) {
					errorf("list_ptr was somehow empty");
					return 1;
				} else if (gotRows > MAX_NROWS) {
					errorf("returned too many entries");
					return 1;
				} else {
					this->slv->assignRows(workPtr);

					//! TODO: figure this out
					// columnWidth probably shouldn't be when refreshing and instead only when changing page
					//this->slv->setColumnWidth();

					//this->slv->bpos[1] = x*CHAR_WIDTH+5;
				}
			} else {
errorf("miman u+1 -- 5");
				if (this->plv->curPage > 1) {
					this->plv->curPage--;
					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
					PostMessage(hwnd, WM_USER+0, 0, 0);
					return 0;
				}
			}

errorf("miman u+1 -- 6");
			SendDlgItemMessageW(hwnd, 3, WM_USER, 1, 0);

			if (!(InvalidateRect(hwnd, 0, 1))) {
				errorf("InvalidateRect failed");
			}

			break;
		}
		case WM_U_LISTMAN: {
			break;
		}
		case WM_U_RET_SL: {
errorf("miman WM_U_RET_SL -- 1");

			switch (wParam) {
			case 0:
				return (LRESULT) &this->slv;
			case 1:
			case 2:
			case 3:
				if (this->option != 2) {
					uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;
					errorf("MainIndexManProc -- WM_U_RET_SL -- DelRer commented out");
					//DelRer(hwnd, lParam, &this->slv);
					SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
				}
				break;
			case 4:
				this->menuCreate(hwnd);
				break;
			case 5:	// make selection

				if (this->option == 2) {
					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_RET_SEL_POS, 0);
				}
				break;
			}
			break;
		}
		case WM_U_RET_PL: {

			switch (wParam) {
			case 0:
				//return (LRESULT) &this->plv;
				errorf("deprecated: 3452089hre");
			case 1:
				//! TODO:
				SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_CH_PAGE_REF, 0);
				break;
			}
			break;
		}
		case WM_U_RET_TED: {

				char *buf = (char *) wParam;

				if (buf != NULL) {

					errorf_old("WM_U_TED -- Received: \"%s\"", buf);
					uint64_t uint = mireg(buf);
					errorf("after mireg");
					if (uint == 0) {
						errorf("mireg failed");
					} else {
						SendMessage(hwnd, WM_USER, 0, 0);
					}
				}

			break;
		}
		case WM_ERASEBKGND: {
			return 1;
		}
		case WM_CLOSE: {

			if (this->parent) {
				EnableWindow(this->parent, 1);
			}
			break;
		}
		case WM_DESTROY: {

			break;
		}
		case WM_NCDESTROY: {

			HWND thwnd = this->parent;

			if (thwnd) {
				EnableWindow(thwnd, 1);
				SendMessage(thwnd, WM_U_RET_DMAN, 0, (LPARAM) hwnd);
			}

			return 0;
		}
	}

	return this->ListManClass::winProc(hwnd, msg, wParam, lParam);

	//return DefWindowProcW(hwnd, msg, wParam, lParam);
}

//} MainIndexManClass

//{ DmanClass
//public:
// static
const WindowHelper DmanClass::helper = WindowHelper(std::wstring(L"DmanClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND DmanClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new DmanClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

int DmanClass::menuCreate(HWND hwnd) {
	HMENU hMenu;

	if (this->option != 2) {
		hMenu = CreatePopupMenu();
		AppendMenuW(hMenu, MF_STRING, 1, L"&Remove");	// if menu handles are changed, the message from WM_COMMAND needs to be changed to correspond to the right case
		AppendMenuW(hMenu, MF_STRING, 2, L"&Reroute");
		AppendMenuW(hMenu, MF_STRING, 3, L"&Reroute (RP)");

		TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, this->slv->point.x, this->slv->point.y, 0, hwnd, 0);
		DestroyMenu(hMenu);
	}

	return 0;
};

int DmanClass::menuUse(HWND hwnd, int32_t menu_id) {
	errorf_old("menu_id is: %d", menu_id);

	uint64_t fNum;

	switch (menu_id) {
	case 1:
	case 2:
	case 3:
		if (this->option != 2) {
			fNum = (this->plv->curPage-1)*MAX_NROWS+1;
			//DelRer(hwnd, menu_id, &this->slv);
			SendMessage(hwnd, WM_USER, 0, 0);
			SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
		}
		break;
	}

	return 0;
};

uint64_t DmanClass::getNumElems() {
	return getlastdnum(this->inum);
}

LRESULT CALLBACK DmanClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	/*
	char *p;
	wchar_t *wp;
	int32_t i, j, x;
	uint64_t ll, lastlastpage, fNum;
	RECT rect;
	HWND thwnd;
	oneslnk *link, *link2;
	*/

	switch(msg) {
		case WM_CREATE: {

			this->plv = NULL;
			this->slv = NULL;

			DIRMANARGS *args = *(DIRMANARGS **) lParam;

			if (args) {
				if (args->inum == 0) {
					errorf("args->inum is 0");
					return -1;
				} else {
					this->inum = args->inum;
				}

				this->option = args->option;
				this->parent = args->parent;
			} else {
				errorf("args is NULL");
				return -1;
			}

			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			HWND thwnd = NULL;
			uint64_t ll = 0;
			int32_t i = 0;

			if (this->option != 2) {
				int32_t x = rect.right / 2 - 100;
				if (x < 5)
					x = 5;
				CreateWindowW(L"Button", L"Add...", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, NULL, NULL);
				SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont2, 1);
				CreateWindowW(L"Button", L"Relative path", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, x+90, 2, 110, 35, hwnd, (HMENU) 2, NULL, NULL);
				SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) hFont2, 1);
				CheckDlgButton(hwnd, 2, BST_UNCHECKED);
			} else {
				int32_t x = (rect.right-80) / 2;
				if (x < 5)
					x = 5;
				EnableWindow(this->parent, 0);
				CreateWindowW(L"Button", L"Select", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, NULL, NULL);
				SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont2, 1);
			}

			thwnd = PageListClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 4, NULL, NULL));
			if (!thwnd) {
				errorf("failed to create PageListClass");
				return -1;
			}

			this->plv = std::dynamic_pointer_cast<PageListClass>(WindowClass::getWindowPtr(thwnd));
			if (!this->plv) {
				errorf("failed to receive PageListClass pointer");
				return -1;
			}

			thwnd = StrListClass::createWindowInstance(WinInstancer(0,  0, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 3, NULL, NULL));
			if (!thwnd) {
				errorf("failed to create StrListClass");
				return -1;
			}

			this->slv = std::dynamic_pointer_cast<StrListClass>(WindowClass::getWindowPtr(thwnd));
			if (!this->slv) {
				errorf("failed to receive StrListClass pointer");
				return -1;
			}

			//! TODO: move this to the createWindowInstance arguments
			if (this->option == 2) {
				this->slv->option = 1;
			}

			this->plv->curPage = 1;
			ll = this->getNumElems();
			this->plv->lastPage = ll/MAX_NROWS + !!(ll % MAX_NROWS);
			if (this->plv->lastPage == 0) {
				this->plv->lastPage = 1;
			}

			this->plv->startPaint();

			i = MAX_NROWS/8 + !!(MAX_NROWS%8);
			if (!i) {
				errorf("no space row selections");
			}

			this->slv->setNColumns(2);
			uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;
			i = log10(fNum+MAX_NROWS-1)+1;
			if ( !this->slv->setColumnWidth(0, CHAR_WIDTH*(i)+6) ) {
				errorf("setColumnWidth failed (0)");
				return -1;
			}
			if ( !this->slv->setColumnWidth(1, 0) ) {
				errorf("setColumnWidth failed (1)");
				return -1;
			}

			if ( !this->slv->setHeaders(std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>( {"#", "Path"}))) ) {
				errorf("failed to set slv headers");
				return -1;
			}
			//this->slv->header = malloc(sizeof(char *)*this->slv->nColumns);
			//this->slv->header[0] = dupstr("#", 5, 0);
			//this->slv->header[1] = dupstr("Path", 5, 0);

			SetFocus(GetDlgItem(hwnd, 3));

			SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);

			break;
		}
		case WM_COMMAND: {

			if (this->option != 2) {
				bool checkedRelative = !!(IsDlgButtonChecked(hwnd, 2));

				if (HIWORD(wParam) == 0 && lParam == 0) { // context menu
					this->menuUse(hwnd, LOWORD(wParam));
					break;
				} else if (HIWORD(wParam) == BN_CLICKED) {
					if (LOWORD(wParam) == 1) {	// add listing
						std::fs::path retPath = SeekDir(hwnd);

						if (!retPath.empty()) {

							if (checkedRelative) {
								//! TODO: add error return argument and print in window
								retPath = makePathRelativeToProgDir(retPath);

								if (!retPath.empty()) {
									if (!std::fs::is_directory(g_fsPrgDir / retPath)) {
										errorf("retPath was not directory (1)");
										g_errorfStream << "retPath was: " << retPath << std::flush;

										retPath.clear();
									}
								}
							} else {
								if (!std::fs::is_directory(retPath)) {
									errorf("retPath was not directory (2)");
									g_errorfStream << "retPath was: " << retPath << std::flush;

									retPath.clear();
								}
							}

							if (!retPath.empty()) {
errorf("calling dreg");
								dreg(this->inum, retPath);

								SendMessage(hwnd, WM_USER, 0, 0);
							}
						}
					} else if (LOWORD(wParam) == 2) {	// relative-dir button
						if (checkedRelative) {
							CheckDlgButton(hwnd, 2, BST_UNCHECKED);
						} else {
							CheckDlgButton(hwnd, 2, BST_CHECKED);
						}
					}
				}
			} else {
				if (HIWORD(wParam) == BN_CLICKED) {

					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_RET_SEL_POS, 0);

				}
			}

			break;
		}
		case WM_SIZE: {
			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			if (this->option != 2) {
				int32_t x = rect.right / 2 - 100;
				if (x < 5)
					x = 5;

				{
					HWND thwnd = GetDlgItem(hwnd, 1);
					MoveWindow(thwnd, x, 5, 80, 25, 0);
				} {
					HWND thwnd = GetDlgItem(hwnd, 2);
					MoveWindow(thwnd, x+90, 2, 110, 35, 0);
				}
			} else {
				int32_t x = (rect.right-80) / 2;
				if (x < 5)
					x = 5;
				HWND thwnd = GetDlgItem(hwnd, 1);
				MoveWindow(thwnd, x, 5, 80, 25, 0);
			}

			{
				HWND thwnd = GetDlgItem(hwnd, 3);
				MoveWindow(thwnd, 0, DMAN_TOP_MRG, rect.right, rect.bottom-DMAN_TOP_MRG-(!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, 0);
			} {
				HWND thwnd = GetDlgItem(hwnd, 4);
				MoveWindow(thwnd, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, 0);
			}

			break;
		}
		case WM_PAINT: {

			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			HPEN hOldPen;
			HBRUSH hOldBrush;
			HDC hdc;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hwnd, &ps);

			hOldPen = (HPEN) SelectObject(hdc, bgPen1);
			hOldBrush = (HBRUSH) SelectObject(hdc, bgBrush1);

			Rectangle(hdc, 0, 0, rect.right, DMAN_TOP_MRG);
			SelectObject(hdc, hPen1);
			MoveToEx(hdc, 0, 0, NULL);
			LineTo(hdc, rect.right, 0);
			MoveToEx(hdc, 0, DMAN_TOP_MRG-1, NULL);
			LineTo(hdc, rect.right, DMAN_TOP_MRG-1);

			if (this->plv->lastPage > 1) {
				MoveToEx(hdc, 0, rect.bottom-DMAN_BOT_MRG, NULL);
				LineTo(hdc, rect.right, rect.bottom-DMAN_BOT_MRG);
			}
			SelectObject(hdc, hOldPen);
			SelectObject(hdc, hOldBrush);
			EndPaint(hwnd, &ps);

			break;
		}
		case WM_MOUSEMOVE: {

			this->plv->hovered = 0;
			SetCursor(hDefCrs);

			break;
		}
		case WM_SETFOCUS: {
			SetFocus(GetDlgItem(hwnd, 3));

			break;
		}
		case WM_USER+1: {	// refresh num and dir strings

			this->slv->clearRows();

			uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;

			std::shared_ptr<std::forward_list<std::fs::path>> list_ptr = idread(this->inum, (uint64_t) fNum, MAX_NROWS);

			if (list_ptr != nullptr && !list_ptr->empty()) {
				auto workPtr = std::make_shared<std::vector<std::vector<std::string>>>();
				if (!workPtr) {
					errorf("failed to allocate shared_ptr");
					return 1;
				}
				auto &workVector = *workPtr;

				uint64_t uint = fNum;

				for (auto &entry : *list_ptr) {
					workVector.emplace_back( std::vector<std::string>( { std::to_string(uint), entry.generic_string() } ) );

					uint++;
				}

				int64_t gotRows = uint - fNum;

				if (gotRows <= 0) {
					errorf("list_ptr was somehow empty");
					return 1;
				} else if (gotRows > MAX_NROWS) {
					errorf("returned too many entries");
					return 1;
				} else {
					this->slv->assignRows(workPtr);

					//! TODO: figure this out
					// columnWidth probably shouldn't be when refreshing and instead only when changing page
					//this->slv->setColumnWidth();

					//this->slv->bpos[1] = x*CHAR_WIDTH+5;
				}
			} else {
				if (this->plv->curPage > 1) {
					this->plv->curPage--;
					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
					PostMessage(hwnd, WM_USER+0, 0, 0);
					return 0;
				}
			}

			SendDlgItemMessageW(hwnd, 3, WM_USER, 1, 0);

			if (!(InvalidateRect(hwnd, 0, 1))) {
				errorf("InvalidateRect failed");
			}

			break;
		}
		case WM_U_LISTMAN: {
			break;
		}
		case WM_U_RET_SL: {

			switch (wParam) {
			case 0:
				return (LRESULT) &this->slv;
			case 1:
			case 2:
			case 3:
				if (this->option != 2) {
					uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;
					//DelRer(hwnd, lParam, &this->slv);
					SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_LEN_REF, 0);
				}
				break;
			case 4:
				this->menuCreate(hwnd);
				break;
			case 5:	// make selection

				if (this->option == 2) {
					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_RET_SEL_POS, 0);
				}
				break;
			}
			break;
		}
		case WM_U_RET_PL: {

			switch (wParam) {
			case 0:
				//return (LRESULT) &this->plv;
				errorf("deprecated: 3452089hre");
			case 1:
				//! TODO:
				SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_CH_PAGE_REF, 0);
				break;
			}
			break;
		}
		case WM_U_RET_TED: {

			break;
		}
		case WM_ERASEBKGND: {
			return 1;
		}
		case WM_CLOSE: {

			if (this->parent) {
				EnableWindow(this->parent, 1);
			}
			break;
		}
		case WM_DESTROY: {

			break;
		}
		case WM_NCDESTROY: {

			HWND thwnd = this->parent;

			if (thwnd) {
				EnableWindow(thwnd, 1);
				SendMessage(thwnd, WM_U_RET_DMAN, 0, (LPARAM) hwnd);
			}
		}
	}

	return this->ListManClass::winProc(hwnd, msg, wParam, lParam);

	//return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//} DmanClass

//{ SDmanClass
//public:
// static
const WindowHelper SDmanClass::helper = WindowHelper(std::wstring(L"SDmanClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND SDmanClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new SDmanClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

int SDmanClass::menuCreate(HWND hwnd) {
	HMENU hMenu;

	if (this->option != 2) {
		hMenu = CreatePopupMenu();
		AppendMenuW(hMenu, MF_STRING, 1, L"&Remove");	// if menu handles are changed, the message from WM_COMMAND needs to be changed to correspond to the right case
		AppendMenuW(hMenu, MF_STRING, 2, L"&Reroute");
		AppendMenuW(hMenu, MF_STRING, 3, L"&Reroute (RP)");

		TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, this->slv->point.x, this->slv->point.y, 0, hwnd, 0);
		DestroyMenu(hMenu);
	}

	return 0;
};

int SDmanClass::menuUse(HWND hwnd, int32_t menu_id) {
	errorf_old("menu_id is: %d", menu_id);

	uint64_t fNum;

	switch (menu_id) {
	case 1:
	case 2:
	case 3:
		if (this->option != 2) {
			fNum = (this->plv->curPage-1)*MAX_NROWS+1;
			//DelRer(hwnd, menu_id, &this->slv);
			SendMessage(hwnd, WM_USER, 0, 0);
			SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
		}
		break;
	}

	return 0;
};

uint64_t SDmanClass::getNumElems() {
	return getlastsdnum(this->inum);
}

LRESULT CALLBACK SDmanClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	/*
	char *p;
	wchar_t *wp;
	int i, j, x;
	uint64_t ll, lastlastpage, fNum;
	HWND thwnd;
	oneslnk *link, *link2;
	*/

	switch(msg) {
		case WM_CREATE: {

			this->plv = NULL;
			this->slv = NULL;

			SDIRMANARGS *args = *(SDIRMANARGS **) lParam;

			if (args->inum == 0) {
				errorf("inum is 0");
				DestroyWindow(hwnd);
				return 0;
			}

			if (args) {
				this->option = args->option;
				this->parent = args->parent;
				this->inum = args->inum;
			} else {
				errorf("args is NULL");
				//DestroyWindow(hwnd);
				//FreeWindowMem(hwnd);
				return -1;

				// this->option = 0;
				// this->parent = 0;
			}

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			HWND thwnd = NULL;
			uint64_t ll = 0;
			int32_t i = 0;

			if (this->option != 2) {
				int32_t x = rect.right / 2 - 100;
				if (x < 5)
					x = 5;
				CreateWindowW(L"Button", L"Add...", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, NULL, NULL);
				SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont2, 1);
				//CreateWindowW(L"Button", L"Relative path", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, x+90, 2, 110, 35, hwnd, (HMENU) 2, NULL, NULL);
				//SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) hFont2, 1);
				//CheckDlgButton(hwnd, 2, BST_UNCHECKED);
			} else {
				int32_t x = (rect.right-80) / 2;
				if (x < 5)
					x = 5;
				EnableWindow(this->parent, 0);
				CreateWindowW(L"Button", L"Select", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, NULL, NULL);
				SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont2, 1);
			}

			thwnd = PageListClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 4, NULL, NULL));
			if (!thwnd) {
				errorf("failed to create PageListClass");
				return -1;
			}

			this->plv = std::dynamic_pointer_cast<PageListClass>(WindowClass::getWindowPtr(thwnd));
			if (!this->plv) {
				errorf("failed to receive PageListClass pointer");
				return -1;
			}

			thwnd = StrListClass::createWindowInstance(WinInstancer(0,  0, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 3, NULL, NULL));
			if (!thwnd) {
				errorf("failed to create StrListClass");
				return -1;
			}

			this->slv = std::dynamic_pointer_cast<StrListClass>(WindowClass::getWindowPtr(thwnd));
			if (!this->slv) {
				errorf("failed to receive StrListClass pointer");
				return -1;
			}

			//! TODO: move this to the createWindowInstance arguments
			if (this->option == 2) {
				this->slv->option = 1;
			}

			this->plv->curPage = 1;
			ll = this->getNumElems();
			this->plv->lastPage = ll/MAX_NROWS + !!(ll % MAX_NROWS);
			if (this->plv->lastPage == 0) {
				this->plv->lastPage = 1;
			}

			this->plv->startPaint();

			i = MAX_NROWS/8 + !!(MAX_NROWS%8);
			if (!i) {
				errorf("no space row selections");
			}

			//! TODO:
			this->slv->setNColumns(2);
			uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;
			i = log10(fNum+MAX_NROWS-1)+1;
			if ( !this->slv->setColumnWidth(0, CHAR_WIDTH*(i)+6) ) {
				errorf("setColumnWidth failed (0)");
				return -1;
			}
			if ( !this->slv->setColumnWidth(1, 0) ) {
				errorf("setColumnWidth failed (1)");
				return -1;
			}

			if ( !this->slv->setHeaders(std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>( {"#", "Path"} ))) ) {
				errorf("failed to set slv headers");
				return -1;
			}

			SetFocus(GetDlgItem(hwnd, 3));

			SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);

			break;
		}
		case WM_COMMAND: {

			if (this->option != 2) {

				if (HIWORD(wParam) == 0 && lParam == 0) { // context menu
					//SendMessage(hwnd, WM_U_RET_SL, 3, LOWORD(wParam);
					this->menuUse(hwnd, LOWORD(wParam));
					break;
				//! TODO: move next to button creation
				} else if (HIWORD(wParam) == BN_CLICKED) {
					if (LOWORD(wParam) == 1) {	// add listing

						DIRMANARGS args = {};
						args.option = 2;
						args.inum = this->inum;
						args.parent = hwnd;

						HWND thwnd = DmanClass::createWindowInstance(WinInstancer(0, L"Select Superdirectory", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 100, 100, 500, 500, hwnd, NULL, ghInstance, (LPVOID) &args));
					}
				}
			} else {
				if (HIWORD(wParam) == BN_CLICKED) {

					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_RET_SEL_POS, 0);

				}
			}

			break;
		}
		case WM_SIZE: {
			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			if (this->option != 2) {
				int32_t x = rect.right / 2 - 100;
				if (x < 5)
					x = 5;

				{
					HWND thwnd = GetDlgItem(hwnd, 1);
					MoveWindow(thwnd, x, 5, 80, 25, 0);
				} {
					// HWND thwnd = GetDlgItem(hwnd, 2);
					// MoveWindow(thwnd, x+90, 2, 110, 35, 0);
				}
			} else {
				int32_t x = (rect.right-80) / 2;
				if (x < 5)
					x = 5;
				HWND thwnd = GetDlgItem(hwnd, 1);
				MoveWindow(thwnd, x, 5, 80, 25, 0);
			}

			{
				HWND thwnd = GetDlgItem(hwnd, 3);
				MoveWindow(thwnd, 0, DMAN_TOP_MRG, rect.right, rect.bottom-DMAN_TOP_MRG-(!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, 0);
			} {
				HWND thwnd = GetDlgItem(hwnd, 4);
				MoveWindow(thwnd, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, 0);
			}


			break;
		}
		case WM_PAINT: {

			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			HPEN hOldPen;
			HBRUSH hOldBrush;
			HDC hdc;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hwnd, &ps);

			hOldPen = (HPEN) SelectObject(hdc, bgPen1);
			hOldBrush = (HBRUSH) SelectObject(hdc, bgBrush1);

			Rectangle(hdc, 0, 0, rect.right, DMAN_TOP_MRG);
			SelectObject(hdc, hPen1);
			MoveToEx(hdc, 0, 0, NULL);
			LineTo(hdc, rect.right, 0);
			MoveToEx(hdc, 0, DMAN_TOP_MRG-1, NULL);
			LineTo(hdc, rect.right, DMAN_TOP_MRG-1);

			if (this->plv->lastPage > 1) {
				MoveToEx(hdc, 0, rect.bottom-DMAN_BOT_MRG, NULL);
				LineTo(hdc, rect.right, rect.bottom-DMAN_BOT_MRG);
			}
			SelectObject(hdc, hOldPen);
			SelectObject(hdc, hOldBrush);
			EndPaint(hwnd, &ps);

			break;
		}
		case WM_MOUSEMOVE: {

			this->plv->hovered = 0;
			SetCursor(hDefCrs);

			break;
		}
		case WM_SETFOCUS: {
			SetFocus(GetDlgItem(hwnd, 3));

			break;
		}
		case WM_USER+1: {		// refresh num and dir strings

			this->slv->clearRows();

			uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;

			std::shared_ptr<std::forward_list<std::fs::path>> list_ptr = idread(this->inum, (uint64_t) fNum, MAX_NROWS);

			if (list_ptr != nullptr && !list_ptr->empty()) {
				auto workPtr = std::make_shared<std::vector<std::vector<std::string>>>();
				if (workPtr == nullptr) {
					errorf("failed to allocate shared_ptr");
					return 1;
				}
				auto &workVector = *workPtr;

				uint64_t uint = fNum;

				for (auto &entry : *list_ptr) {
					workVector.emplace_back( std::vector<std::string>( { std::to_string(uint), entry.generic_string() } ) );

					uint++;
				}

				int64_t gotRows = uint - fNum;

				if (gotRows <= 0) {
					errorf("list_ptr was somehow empty");
					return 1;
				} else if (gotRows > MAX_NROWS) {
					errorf("returned too many entries");
					return 1;
				} else {
					this->slv->assignRows(workPtr);

					//! TODO: figure this out
					// columnWidth probably shouldn't be when refreshing and instead only when changing page
					//this->slv->setColumnWidth();

					//this->slv->bpos[1] = x*CHAR_WIDTH+5;
				}
			} else {
				if (this->plv->curPage > 1) {
					this->plv->curPage--;
					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_REFRESH, 0);
					PostMessage(hwnd, WM_USER+0, 0, 0);
					return 0;
				}
			}

			SendDlgItemMessageW(hwnd, 3, WM_USER, 1, 0);

			if (!(InvalidateRect(hwnd, 0, 1))) {
				errorf("InvalidateRect failed");
			}

			break;
		}
		case WM_U_LISTMAN: {
			break;
		}
		case WM_U_RET_SL: //{

			switch (wParam) {
			case 0:
				return (LRESULT) &this->slv;
			case 1:
			case 2:
			case 3:
				if (this->option != 2) {
					uint64_t fNum = (this->plv->curPage-1)*MAX_NROWS+1;
					//DelRer(hwnd, lParam, &this->slv);
					SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_LEN_REF, 0);
				}
				break;
			case 4:
				this->menuCreate(hwnd);
				break;
			case 5:	// make selection

				if (this->option == 2) {
					PostMessage(hwnd, WM_U_LISTMAN, LISTMAN_RET_SEL_POS, 0);
				}
				break;
			}
			break;
		//}
		case WM_U_RET_PL: {

			switch (wParam) {
			case 0:
				//return (LRESULT) &this->plv;
				errorf("deprecated: 3452089hre (2)");
			case 1:
				//! TODO:
				SendMessage(hwnd, WM_U_LISTMAN, LISTMAN_CH_PAGE_REF, 0);
				break;
			}
			break;
		}
		case WM_U_RET_TED: {

			break;
		}
		case WM_ERASEBKGND: {
			return 1;
		}
		case WM_CLOSE: {

			if (this->parent) {
				EnableWindow(this->parent, 1);
			}
			break;
		}
		case WM_DESTROY: {

			break;
		}
		case WM_NCDESTROY: {

			HWND thwnd = this->parent;

			if (thwnd) {
				EnableWindow(thwnd, 1);
				SendMessage(thwnd, WM_U_RET_DMAN, 0, (LPARAM) hwnd);
			}
		}
	}

	return this->ListManClass::winProc(hwnd, msg, wParam, lParam);

	//return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//} SDmanClass

//{ ThumbManClass
//public:
// static
const WindowHelper ThumbManClass::helper = WindowHelper(std::wstring(L"ThumbManClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND ThumbManClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new ThumbManClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK ThumbManClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	char *p;
	int i, j, x;
	uint64_t ll, lastlastpage, fnum;
	RECT rect;
	HWND thwnd;
	oneslnk *link, *flink, *inlink;
	HMENU hMenu;
	ImgF *tempimg;
	double zoom;

	HPEN hOldPen;
	HDC hdc;
	PAINTSTRUCT ps;

	/*
	switch(msg) {
		case WM_CREATE: //{

				{
					HWND thwnd = GetDlgItem(hwnd, 1);
					MoveWindow(thwnd, x, 5, 80, 25, 0);
				} {
					HWND thwnd = GetDlgItem(hwnd, 2);
					MoveWindow(thwnd, x+90, 2, 110, 35, 0);
				}

				{
					HWND thwnd = GetDlgItem(hwnd, 1);
					MoveWindow(thwnd, x, 5, 80, 25, 0);
				} {
					HWND thwnd = GetDlgItem(hwnd, 2);
					MoveWindow(thwnd, x+90, 2, 110, 35, 0);
				}

			{
				THMBMANARGS *args = *((void **)lParam);
				if (args) {
					this->option = args->option;
					this->parent = args->parent;
					this->dnum = args->dnum;
				} else {
					return -1;
				}
			}

			this->dname = dread(this->dnum);
			if (this->dname == 0) {
				errorf("dname came null");
				DestroyWindow(hwnd);
				return 0;
			}
			this->tfstr = 0;

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			this->plv->curPage = 1;
			this->plv->lastPage = 1;

			i = MAX_NTHUMBS/8 + !!(MAX_NTHUMBS%8);
			if (!i) {
				errorf("no space thumb selections");
			}
			this->tlv.ThumbSel = malloc(i);
			while (--i >= 0) {
				this->tlv.ThumbSel[i] = 0;
			}
			this->tlv.nChns = 2;
			this->tlv.nThumbs = 0;
			this->tlv.lastsel = 0;
			this->tlv.strchn = malloc(sizeof(oneslnk*)*this->tlv.nChns);
			this->tlv.strchn[0] = 0;
			this->tlv.strchn[1] = 0;
			this->tlv.thumb = 0;
			this->tlv.width = DEF_THUMBW;
			this->tlv.height = DEF_THUMBH;
			this->ivv.FullImage = 0;
			this->ivv.DispImage = 0;
			this->ivv.imgpath = 0;
			this->ivv.option = 0;

			if (getfilemodified(this->dname) >= getdlastchecked(this->dnum) || (p = firead(this->dnum, 1)) == NULL) {
				dirfreg(this->dname, 1<<(5-1));
			} else {
				if (p)
					free(p);
			}
			this->tfstr = ffireadtagext(this->dnum, 0, IMGEXTS);	//! probably want to hide the window before loading all the images
			if (this->tfstr) {
				this->nitems = getframount(this->tfstr);
				this->plv->lastPage = this->nitems/MAX_NTHUMBS + !!(this->nitems % MAX_NTHUMBS);
			} else {
				this->plv->lastPage = 1;
			}

			ThumbListClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, SBAR_H, rect.right, rect.bottom-(!!(this->plv->lastPage > 1))*DMAN_BOT_MRG - SBAR_H, hwnd, (HMENU) 3, NULL, NULL));
			PageListClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, hwnd, (HMENU) 4, NULL, NULL));
			SearchBarClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, rect.right, SBAR_H, hwnd, (HMENU) 6, NULL, NULL));
			SetFocus(GetDlgItem(hwnd, 3));

			SendMessage(hwnd, WM_USER+1, 0, 0);
			break;
		//}
		case WM_SIZE: //{
			if (!(this->option & 1)) {
				thwnd = GetDlgItem(hwnd, 3);
				MoveWindow(thwnd, 0, SBAR_H, rect.right, rect.bottom-(!!(this->plv->lastPage > 1))*DMAN_BOT_MRG - SBAR_H, 0);
				thwnd = GetDlgItem(hwnd, 4);
				MoveWindow(thwnd, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, 0);
				thwnd = GetDlgItem(hwnd, 6);
				MoveWindow(thwnd, 0, 0, rect.right, SBAR_H, 0);
			} else {
				thwnd = GetDlgItem(hwnd, 5);
				MoveWindow(thwnd, 0, 0, rect.right, rect.bottom, 0);
			}
			break;
		//}
		case WM_PAINT: //{
			hdc = BeginPaint(hwnd, &ps);

			hOldPen = (HPEN) SelectObject(hdc, hPen1);

			if (this->plv->lastPage > 1 && !(this->option & 1)) {
				MoveToEx(hdc, 0, rect.bottom-DMAN_BOT_MRG, NULL);
				LineTo(hdc, rect.right, rect.bottom-DMAN_BOT_MRG);
			}
			SelectObject(hdc, hOldPen);
			EndPaint(hwnd, &ps);
			break;
		//}
		case WM_MOUSEMOVE: //{
			this->plv->hovered = 0;
			SetCursor(hDefCrs);

			break;
		//}
		case WM_SETFOCUS: //{
errorf("setting focus1");
			if (varp) {
				if (!(this->option & 1)) {
					SetFocus(GetDlgItem(hwnd, 3));
				} else {
					SetFocus(GetDlgItem(hwnd, 5));
				}
			}

			break;
		//}
		case WM_USER: //{	refresh pages

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			lastlastpage = this->plv->lastPage;
			j = 0, x = 0;

			if (this->tfstr) {
				this->nitems = getframount(this->tfstr);
				this->plv->lastPage = this->nitems/MAX_NTHUMBS + !!(this->nitems % MAX_NTHUMBS);
			} else
				this->plv->lastPage = 1;

			if (this->plv->curPage > this->plv->lastPage)
				this->plv->curPage = this->plv->lastPage, j = 1;

			if (this->plv->lastPage != lastlastpage) {
				SendDlgItemMessageW(hwnd, 4, WM_USER, 0, 0);
				if (j == 1) {
					SendMessage(hwnd, WM_USER+2, 1, this->plv->curPage);
				} else {
					if (!(InvalidateRect(hwnd, 0, 1))) {
						errorf("InvalidateRect failed");
					}
				}
				this->plv->hovered = 0;
				SetCursor(hDefCrs);
			}

			break;
		//}
		case WM_USER+1:		//{		refresh fnum and file name strings and thumbnails

			if (this->tlv.strchn[0] != NULL) {
				killoneschn(this->tlv.strchn[0], 1);
				this->tlv.strchn[0] = NULL;
			}
			if (this->tlv.strchn[1] != NULL) {
				killoneschn(this->tlv.strchn[1], 0);
				this->tlv.strchn[1] = NULL;
			}
			if (this->tlv.thumb != 0) {
				for (i = 0; i > this->tlv.nThumbs; i++) {
					if (this->tlv.thumb[i] != 0) {
						DestroyImgF(this->tlv.thumb[i]);
					}
				} free(this->tlv.thumb), this->tlv.thumb = NULL, this->tlv.nThumbs = 0;
			}

			fnum = (this->plv->curPage-1)*MAX_NTHUMBS+1;

			if (this->tfstr) {
				if (1) {
					this->tlv.strchn[0] = ifrread(this->tfstr, (uint64_t) fnum, MAX_NTHUMBS); //! potentially modify it to have fnums and fnames in the same file

					for (link = this->tlv.strchn[0], i = 0; link != 0; link = link->next, i++);
					if (fnum + i - 1 > this->nitems || (this->plv->curPage == this->plv->lastPage && fnum + i - 1 != this->nitems)) {
						errorf("thumbman item amount mismatch");
					}

					if (this->tlv.strchn[0]) {
						this->tlv.strchn[1] = cfiread(this->dnum, this->tlv.strchn[0]);
					}
				}

				if (this->tlv.strchn[0]) {
					this->tlv.nThumbs = i;
					this->tlv.thumb = malloc(sizeof(ImgF*)*i);

					char buffer[MAX_PATH*4];

					for (i = 0, (link = this->tlv.strchn[1]); i < this->tlv.nThumbs && link != 0; i++, link = link->next) {
						if (link->str != 0) {
							sprintf(buffer, "%s\\%s", this->dname, link->str);

							this->tlv.thumb[i] = ReadImage(buffer);

							if (this->tlv.thumb[i] != 0) {
								tempimg = this->tlv.thumb[i];
								this->tlv.thumb[i] = FitImage(this->tlv.thumb[i], DEF_THUMBW, DEF_THUMBH, 1, 0);
								DestroyImgF(tempimg);
							}
						} else {
							this->tlv.thumb[i] = 0;
						}
					}
					while (i++ < this->tlv.nThumbs) {
						this->tlv.thumb[i] = 0;
					}
				}
			}

			SendDlgItemMessageW(hwnd, 3, WM_USER, 1, 0);	// update scroll in thmblist
			if (!(InvalidateRect(hwnd, 0, 1))) {
				errorf("InvalidateRect failed");
			}
			break;
		//}
		case WM_USER+2: //{

			switch(wParam) {

			case 0:		// refresh
//				this->tlv.nThumbs = MAX_NTHUMBS;

				break;

			case 1:		// change page

				for (i = this->tlv.nThumbs/8+!!(this->tlv.nThumbs%8)-1; i >= 0; i--) {
					if (i < 0)
						break;
					this->tlv.ThumbSel[i] = 0;
				}
				this->tlv.nThumbs = MAX_NTHUMBS;
				fnum = (this->plv->curPage-1)*MAX_NTHUMBS+1;

				break;
			}

			PostMessage(hwnd, WM_USER+1, 0, 0);

			break;
		//}
		case WM_COMMAND: //{

			if (HIWORD(wParam) == 0) { // menu
				switch (LOWORD(wParam)) {
				case 1:
					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					for (j = 0; !(this->tlv.ThumbSel[j/8] & (1 << j%8)) && j < this->tlv.nThumbs; j++);
					if (j == this->tlv.nThumbs) {
						errorf("trying to view image with no selections");
						break;
					}
					DestroyWindow(GetDlgItem(hwnd, 3)), DestroyWindow(GetDlgItem(hwnd, 4)), DestroyWindow(GetDlgItem(hwnd, 6));

					if (this->tlv.thumb) {
						for (i = 0; i > this->tlv.nThumbs; i++) {
							if (this->tlv.thumb[i] != 0) {
								DestroyImgF(this->tlv.thumb[i]);
							}
						} free(this->tlv.thumb), this->tlv.thumb = 0;
					}

					for (i = 0, (link = this->tlv.strchn[1]) && link != 0; link && i < j; i++, link = link->next);
					if (!link || !link->str) {
						errorf_old("no link or string in this->tlv.strchn[1], i: %llu", i);
						DestroyWindow(hwnd);
					}
					this->ivv.imgpath = malloc(strlen(this->dname)+1+strlen(link->str)+1);
					sprintf(this->ivv.imgpath, "%s\\%s", this->dname, link->str);

					for (i = 0, (link = this->tlv.strchn[0]) && link != 0; link && i < j; i++, link = link->next);
					if (!link) {
						errorf_old("no link in this->tlv.strchn[0], j: %llu", j);
						DestroyWindow(hwnd);
					}

					this->ivv.fnum = link->ull;

					if (this->tlv.strchn[0] != 0)
						killoneschn(this->tlv.strchn[0], 1), this->tlv.strchn[0] = 0;
					if (this->tlv.strchn[1] != 0)
						killoneschn(this->tlv.strchn[1], 0), this->tlv.strchn[1] = 0;

//					if (this->tlv.ThumbSel)
//						free(this->tlv.ThumbSel), this->tlv.ThumbSel = 0;
					for (j = 0; j < this->tlv.nThumbs/8+!!(this->tlv.nThumbs%8); this->tlv.ThumbSel[j] = 0, j++);

					this->option |= 1;
					ViewImageClass::createWindowInstance(WinInstancer(0, L"View Image", WS_VISIBLE | WS_CHILD, 0, 0, rect.right, rect.bottom, hwnd, (HMENU) 5, NULL, NULL));
					SetFocus(GetDlgItem(hwnd, 5));
					break;
				case 2:
					link = flink = malloc(sizeof(oneslnk));
					for (j = 0, inlink = this->tlv.strchn[0]; j < this->tlv.nThumbs && inlink; j++, inlink = inlink->next) {
						if (this->tlv.ThumbSel[(j)/8] & (1 << j%8)) {
							link = link->next = malloc(sizeof(oneslnk));
							link->ull = inlink->ull;
						}
					}
					link->next = 0;
					if (j != this->tlv.nThumbs) {
						errorf("didn't go through all thumbs");
						killoneschn(link, 0);
					}
					link = flink->next;
					free(flink);
					if (link == 0) {
						break;
					}

					thwnd = FileTagEditClass::createWindowInstance(WinInstancer(0, L"Edit Tags", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 150, 150, 500, 500, hwnd, 0, NULL, NULL));
					SendMessage(thwnd, WM_USER, 0, (LPARAM) this->dnum);

					if (SendMessage(thwnd, WM_USER, 1, (LPARAM) link) != 1) {
						errorf("didn't get filetagedit confirmation");
						killoneschn(link, 1);
					}
					break;
				}
				break;
			}

			break;
		//}
		case WM_U_RET_TL: //{
			switch (lParam) {
			case 0:
				return (LRESULT) &this->tlv;
			case 1:
			case 2:
			case 3:
				break;
			case 4:
				if (this->option != 1) {
					hMenu = CreatePopupMenu();
					AppendMenuW(hMenu, MF_STRING, 1, L"&View Image");	// if menu handles are changed, the message from WM_COMMAND needs to be changed to correspond to the right case
					AppendMenuW(hMenu, MF_STRING, 2, L"&Edit Tags");
//					AppendMenuW(hMenu, MF_STRING, 3, L"&Reroute (RP)");

					TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, this->tlv.point.x, this->tlv.point.y, 0, hwnd, 0);
					DestroyMenu(hMenu);
				}
				break;
			}
			break;
		//}
		case WM_U_RET_PL: //{
			switch (lParam) {
			case 0:
				return (LRESULT) &this->plv;
			case 1:
				SendMessage(hwnd, WM_USER+2, 1, 0);
				break;
			}
			break;
		//}
		case WM_U_VI: //{
			switch (lParam) {
			case 0:
				return (LRESULT) &this->ivv;
			case 1:
				if (GetClientRect(hwnd, &rect) == 0) {	// all along it didn't draw because this was missing
					errorf("GetClientRect failed");
				}
				DestroyWindow(GetDlgItem(hwnd, (int) wParam));
				if (this->ivv.imgpath) {
					free(this->ivv.imgpath);
					this->ivv.imgpath = 0;
				}
				if (this->ivv.FullImage) {
					DestroyImgF(this->ivv.FullImage);
					this->ivv.FullImage = 0;
				}
				if (this->ivv.DispImage && !(this->ivv.fit & 2)) {
					DestroyImgF(this->ivv.DispImage);
					this->ivv.DispImage = 0;
				}

				this->option &= ~1;


				CreateWindowW(L"PageListClass", 0, WS_VISIBLE | WS_CHILD, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->plv->lastPage > 1))*DMAN_BOT_MRG, hwnd, (HMENU) 4, NULL, NULL);
				CreateWindowW(L"ThumbListClass", 0, WS_VISIBLE | WS_CHILD, 0, SBAR_H, rect.right, rect.bottom-(!!(this->plv->lastPage > 1))*DMAN_BOT_MRG - SBAR_H, hwnd, (HMENU) 3, NULL, NULL);
				CreateWindowW(L"SearchBarClass", 0, WS_VISIBLE | WS_CHILD, 0, 0, rect.right, SBAR_H, hwnd, (HMENU) 6, NULL, NULL);

				SendMessage(hwnd, WM_USER+1, 0, 0);
				SetFocus(hwnd);

				break;
			}
			break;
		//}
		case WM_U_SBAR: //{

			if (lParam == 0) {
				p = (char *) wParam;

				if (this->tfstr)
					releasetfile(this->tfstr, 2);

				this->tfstr = ffireadtagext(this->dnum, p, IMGEXTS);

				if (this->tfstr) {
					this->nitems = getframount(this->tfstr);
					this->plv->lastPage = this->nitems/MAX_NTHUMBS + !!(this->nitems % MAX_NTHUMBS);
				} else {
					this->plv->lastPage = 1;
errorf("ffireadtagext no result");
				}

				this->plv->curPage = 1;
				SendDlgItemMessageW(hwnd, 4, WM_USER, 0, 0);

				SendMessage(hwnd, WM_USER+1, 0, 0);
			}

			break;
		//}
		case WM_KEYDOWN: //{
			switch(wParam) {
			case VK_BACK:
				PostMessage(this->parent, WM_KEYDOWN, wParam, lParam);

				break;
			}
			break;
		//}
		case WM_NCDESTROY: //{
			if (varp) {
				if (this->dname)
					free(this->dname);
				if (this->tfstr)
					releasetfile(this->tfstr, 2);

				if (this->tlv.ThumbSel) {
					free(this->tlv.ThumbSel);
				}
				if (this->tlv.strchn) {
					this->tlv.strchn[0]?killoneschn(this->tlv.strchn[0], 1):0, this->tlv.strchn[1]?killoneschn(this->tlv.strchn[1], 0):0, free(this->tlv.strchn);
				}
				if (this->tlv.thumb != 0) {
					for (i = 0; i > this->tlv.nThumbs; i++) {
						if (this->tlv.thumb[i] != 0) {
							DestroyImgF(this->tlv.thumb[i]);
						}
					} free(this->tlv.thumb), this->tlv.thumb = 0;
				}

				if (this->ivv.imgpath) {
					free(this->ivv.imgpath);
					this->ivv.imgpath = 0;
				}
				if (this->ivv.FullImage) {
					DestroyImgF(this->ivv.FullImage);
					this->ivv.FullImage = 0;
				}
				if (this->ivv.DispImage && !(this->ivv.fit & 2)) {
					DestroyImgF(this->ivv.DispImage);
					this->ivv.DispImage = 0;
				}
			}

			thwnd = this->parent;
			//FreeWindowMem(hwnd);
			if (thwnd) {
				EnableWindow(thwnd, 1);
				SendMessage(thwnd, WM_U_RET_TMAN, 0, (LPARAM) hwnd);
			}

			return 0;
		//}
	}

	*/
	return DefWindowProcW(hwnd, msg, wParam, lParam);

}


//} ThumbManClass

//{ PageListClass
//public:
// static
const WindowHelper PageListClass::helper = WindowHelper(std::wstring(L"PageListClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND PageListClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new PageListClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

bool PageListClass::isPainting() {
	return doPaint;
}

void PageListClass::startPaint() {
	doPaint = true;
}

void PageListClass::pausePaint() {
	doPaint = false;
}

void PageListClass::getPages(int32_t edge) {
	// the selected page's button is centered unless buttons for all pages fit or the buttons left of a centered button would reach the left edge (that is no left arrow buttons are required) or reach the right edge to the right (no right arrows)

	int32_t endremainder, midremainder, taillen, headlen;
	int32_t selstart;
	uint16_t decimals, decimals2;
	uint64_t pos, pos2 = 0, lastnum, lastnum2;
	uint8_t status = 0;

	this->pageListPtr = nullptr;
	std::shared_ptr<std::vector<struct ListPage>> result;

	if (lastPage == 1 || (curPage > lastPage) || (curPage == 0)) {
		return;
	}

	headlen = ((1+1)*CHAR_WIDTH+2*(PAGES_SPACE+PAGE_PAD)); // space for the left arrow and '1' button
	decimals = (uint16_t) log10(lastPage)+1;
	taillen = ((1+decimals)*CHAR_WIDTH+2*(PAGES_SPACE+PAGE_PAD)); // space for the right arrow and last button
	// headlen = taillen = ((3)*CHAR_WIDTH+2*(PAGES_SPACE+PAGE_PAD)) // if using double arrows instead of first and last page nums

	midremainder = (edge + ((uint16_t)(log10(curPage)+1)*CHAR_WIDTH + PAGE_PAD)) / 2 - PAGES_SIDEBUF + PAGES_SPACE; // space to fit curpage button in middle
	endremainder = edge - PAGES_SIDEBUF*2 + PAGES_SPACE;	// the first button doesn't have space at the front
	if (endremainder - taillen < midremainder) {
		midremainder = endremainder - taillen; // space to fit curpage button if tail is included
	}
	if (endremainder < 0) {
		endremainder = 0;
	} if (midremainder < 0) {
		midremainder = 0;
	}

	pos = 1;
	do {
		decimals = (uint16_t) log10(pos)+1;
		lastnum = pow(10, decimals)-1;

		if (!(status & 4)) {
			if (lastnum >= curPage) {
				if (pos - 1 + midremainder / (PAGES_SPACE+decimals*CHAR_WIDTH+PAGE_PAD) >= curPage) {
					midremainder = endremainder - taillen;
					pos2 = pos, lastnum2 = lastnum;
					do { // determine last number and remaining space when with tail
						decimals2 = (uint16_t) log10(pos2)+1;
						lastnum2 = pow(10, decimals2)-1;
						if ((lastnum2-pos2+1)*(decimals2*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE) >= midremainder) {	// if midremainder is larger it can be decremented
							pos2 = pos2 - 1 + (midremainder) / (decimals2*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);	// the last page that fits -- pos-1 to account for the space the first page take
							break;
						} else {
							midremainder -= (lastnum2-pos2+1)*(decimals2*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
							pos2 = lastnum2+1;
						}
					} while (decimals2 < 100);
					status |= 2;
				} else if (curPage == 1) {
					pos2 = 1;
				}
				status |= 4;
			}
		}
		if (lastnum >= lastPage) {
			if (pos + endremainder / (PAGES_SPACE+decimals*CHAR_WIDTH+PAGE_PAD) > lastPage) {	// at least one page has to fit (e.g. pos == lastPage)
				endremainder -= (lastPage-pos+1)*(PAGES_SPACE+decimals*CHAR_WIDTH+PAGE_PAD);
				status |= 1;
			} else if (lastPage == 1) {
				endremainder = 0;
				status |= 1;
			} break;
		}
		if ((lastnum-pos+1)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE) >= endremainder) {
			break;
		} else {
			endremainder -= (lastnum-pos+1)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
			if (!(status & 2)) {
				midremainder -= (lastnum-pos+1)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
			}
		}
		pos = lastnum+1;
	} while (decimals < 100);
	if (status & 1) {	// if buttons for all pages fit (no arrows)
		endremainder = endremainder/2 + PAGES_SIDEBUF;	// starting coordinate
		result = std::shared_ptr<std::vector<struct ListPage>>(new std::vector<struct ListPage>(lastPage));
		if (result == nullptr) {
			errorf("failed to allocate shared_ptr");
			return;
		}
		auto &vec = *result;
		for (pos = 1; pos <= lastPage; pos++) {
			vec[pos-1].type = 0;
			vec[pos-1].str = utoc(pos);
			vec[pos-1].left = endremainder;
			vec[pos-1].right = endremainder+(((uint16_t) log10(pos)+1)*CHAR_WIDTH+PAGE_PAD);
			endremainder += (((uint16_t) log10(pos)+1)*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
		}
		vec[curPage-1].type |= 1;
		vec[lastPage-1].type |= 2;

		this->pageListPtr = result;

	} else if (status & 2 || curPage == 1) { // if the selected page's button fits before middle when starting from 1 (no left arrows)
		endremainder = PAGES_SIDEBUF;
		if ((pos2 == 0) || !(status & 2)) { // cur page is 1, but doesn't fit
			pos2 = 1;
		}
		result = std::shared_ptr<std::vector<struct ListPage>>(new std::vector<struct ListPage>( (pos2+2) ));

		auto &vec = *result;

		for (pos = 1; pos <= pos2; pos++) {
			vec[pos-1].type = 0;
			vec[pos-1].str = utoc(pos);
			vec[pos-1].left = endremainder;
			vec[pos-1].right = endremainder+(((uint16_t) log10(pos)+1)*CHAR_WIDTH+PAGE_PAD);
			endremainder += ((uint16_t) log10(pos)+1)*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE;
		}
		vec[curPage-1].type |= 1;
		vec[pos-1].type = 0;
		vec[pos-1].str = (char *) malloc(2), vec[pos-1].str[0] = '>', vec[pos-1].str[1] = '\0';
		vec[pos-1].left = endremainder;
		vec[pos-1].right = endremainder+CHAR_WIDTH+PAGE_PAD;
		endremainder += CHAR_WIDTH+PAGE_PAD+PAGES_SPACE;
		vec[pos].type = 2;
		vec[pos].str = utoc(lastPage);
//		vec[pos].str = malloc(3), vec[pos].str[0] = '>', vec[pos].str[1] = '>', vec[pos].str[2] = '\0';
		vec[pos].left = endremainder;
//		vec[pos].right = endremainder+2*CHAR_WIDTH+PAGE_PAD;
		vec[pos].right = endremainder+(((uint16_t) log10(lastPage)+1)*CHAR_WIDTH+PAGE_PAD);

		this->pageListPtr = result;

	} else {
		status = 0;
		selstart = (edge - ((uint16_t)(log10(curPage)+1)*CHAR_WIDTH + PAGE_PAD)) / 2;		// space before curpage button; not casting to uint16_t caused it to go off-center
		if (selstart < PAGES_SIDEBUF + headlen + PAGES_SPACE) {		// ensure end part doesn't overextend
		//! also needs room for the space before (I think)(test later)(or is PAGES_SPACE extra if included in head)
			selstart = PAGES_SIDEBUF + headlen + PAGES_SPACE;
		}
		pos = curPage;
		endremainder = edge - selstart - PAGES_SIDEBUF + PAGES_SPACE;
		if (endremainder < 0) {
			endremainder = 0;
		}
		do {
			decimals = (uint16_t) log10(pos)+1;
			lastnum = pow(10, decimals)-1;

			if (!(status & 2)) {
				if ((lastnum-pos+1)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE) >= endremainder-taillen) {
					midremainder = endremainder-taillen;
					pos2 = pos, lastnum2 = lastnum;
					do { // determine last num before tail
						decimals2 = (uint16_t) log10(pos2)+1;
						lastnum2 = pow(10, decimals2)-1;
						if ((lastnum2-pos2+1)*(decimals2*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE) >= midremainder) {
							pos2 = pos2 - 1 + (midremainder) / (decimals2*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
							break;
						} else {
							midremainder -= (lastnum2-pos2+1)*(decimals2*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
							pos2 = lastnum2+1;
						}
					} while (decimals2 < 100);

					if (pos2 < curPage) {	// too small to fit even the selected page
						pos2 = curPage;
					}
					status |= 2;
				}
			}
			if (lastnum >= lastPage) {
				if (pos + endremainder / (decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE) > lastPage) {
					endremainder -= (lastPage-pos+1)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
					status |= 1;
				} break;
			}
			if ((lastnum-pos+1)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE) >= endremainder) {
				break;
			} else {
				endremainder -= (lastnum-pos+1)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
			}
			pos = lastnum+1;
		} while (decimals < 100);		// if it can fit 0, if it's the last page
		if (status & 1) {	// fit up to the right edge
			endremainder = selstart + endremainder - PAGES_SIDEBUF - headlen;		// an extra PAGES_SPACE is already accounted for in selstart since it's up to the actual start after the space
			if (endremainder < 0)
				endremainder = 0;
			pos2 = lastPage;
		} else {
			if (!(status & 2)) {
				status |= 2;
				pos2 = curPage;
			}
			endremainder = selstart - PAGES_SIDEBUF - headlen;
			if (endremainder < 0)
				endremainder = 0;
		}
		pos = curPage;
		do {
			decimals = (uint16_t) log10(pos-1)+1; 	// the actual pos doesn't need to fit
			lastnum = pow(10, decimals-1);

			if ((pos-lastnum)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE) >= endremainder) {
				lastnum = pos;
				pos = pos - endremainder / (decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
				endremainder -= (lastnum-pos)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
				break;
			} else {
				endremainder -= (pos-lastnum)*(decimals*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE);
				pos = lastnum;
			}
		} while (decimals > 1);

		result = std::shared_ptr<std::vector<struct ListPage>>(new std::vector<struct ListPage>( (2+(pos2-pos+1)+2*(!(status & 1))) ));

		auto &vec = *result;

		endremainder += PAGES_SIDEBUF;

		vec[0].type = 0;
		vec[0].str = utoc(1);
	//		vec[0].str = malloc(3), vec[0].str[0] = '<', vec[0].str[1] = '<', vec[0].str[2] = '\0';
		vec[0].left = endremainder;
	//		vec[0].right = endremainder+2*CHAR_WIDTH+PAGE_PAD;
		vec[0].right = endremainder+1*CHAR_WIDTH+PAGE_PAD;
		endremainder += 2*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE;
		vec[1].type = 0;
		vec[1].str = (char *) malloc(2), vec[1].str[0] = '<', vec[1].str[1] = '\0';
		vec[1].left = endremainder;
		vec[1].right = endremainder+CHAR_WIDTH+PAGE_PAD;
		endremainder += CHAR_WIDTH+PAGE_PAD+PAGES_SPACE;
		int i = pos;
		for (lastnum = 2; pos <= pos2; pos++, lastnum++) {
			vec[lastnum].type = 0;
			vec[lastnum].str = utoc(pos);
			vec[lastnum].left = endremainder;
			vec[lastnum].right = endremainder+(((uint16_t) log10(pos)+1)*CHAR_WIDTH+PAGE_PAD);
			endremainder += ((uint16_t) log10(pos)+1)*CHAR_WIDTH+PAGE_PAD+PAGES_SPACE;
		}
		if (!(status & 1)) {
			vec[lastnum].type = 0;
			vec[lastnum].str = (char *) malloc(2), vec[lastnum].str[0] = '>', vec[lastnum].str[1] = '\0';
			vec[lastnum].left = endremainder;
			vec[lastnum].right = endremainder+CHAR_WIDTH+PAGE_PAD;
			endremainder += CHAR_WIDTH+PAGE_PAD+PAGES_SPACE;
			lastnum++;
			vec[lastnum].type = 0;
			vec[lastnum].str = utoc(lastPage);
//			vec[lastnum].str = malloc(3), vec[lastnum].str[0] = '>', vec[lastnum].str[1] = '>', vec[lastnum].str[2] = '\0';
			vec[lastnum].left = endremainder;
//			vec[lastnum].right = endremainder+2*CHAR_WIDTH+PAGE_PAD;
			vec[lastnum].right = endremainder+(((uint16_t) log10(lastPage)+1)*CHAR_WIDTH+PAGE_PAD);
			lastnum++;
		}
		lastnum--;
		vec[lastnum].type |= 2;
		vec[curPage-i+2].type |= 1;

		this->pageListPtr = result;
	}
}

LRESULT CALLBACK PageListClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	int i, j, x;
	uint64_t ll, lastlastpage, fnum;
	RECT rect;
	HWND thwnd;
	oneslnk *link;
	HMENU hMenu;
	switch(msg) {
		case WM_CREATE: {

			//! TODO: maybe initialize in class instead

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			this->hovered = 0;
			this->pageListPtr = nullptr;
			this->curPage = 0;
			this->lastPage = 0;

			if (this->isPainting()) {
				this->getPages(rect.right);
			}

			break;
		}
		case WM_SIZE:
		case WM_USER:
		{
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (this->isPainting()) {
				this->getPages(rect.right);
			}
			break;
		}
		case WM_PAINT: {

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			HDC hdc;
			PAINTSTRUCT ps;

			HPEN hOldPen;
			HBRUSH tempBrush, hOldBrush;
			HFONT hOldFont;

			hdc = BeginPaint(hwnd, &ps);

			hOldPen = (HPEN) SelectObject(hdc, bgPen1);
			hOldFont = (HFONT) SelectObject(hdc, hFont);
			hOldBrush = (HBRUSH) SelectObject(hdc, bgBrush1);
			Rectangle(hdc, 0, 0, rect.right, rect.bottom);
			SelectObject(hdc, hPen1);
			SetBkMode(hdc, TRANSPARENT);
			tempBrush = CreateSolidBrush(DL_BORDER_COL);

/*			MoveToEx(hdc, PAGES_SIDEBUF, rect.bottom-DMAN_BOT_MRG, NULL);
			LineTo(hdc, PAGES_SIDEBUF, rect.bottom);
			MoveToEx(hdc, rect.right/2, rect.bottom-DMAN_BOT_MRG, NULL);
			LineTo(hdc, rect.right/2, rect.bottom);
			MoveToEx(hdc, rect.right-PAGES_SIDEBUF-1, rect.bottom-DMAN_BOT_MRG, NULL);
			LineTo(hdc, rect.right-PAGES_SIDEBUF-1, rect.bottom);
*/

			if (this->pageListPtr != nullptr) {
				auto &pageList = *pageListPtr;
				for (i = 0; ; i++) {
					if (i == this->hovered-1) {
						SelectObject(hdc, tempBrush);
						if (pageList[i].type & 1) {
							SelectObject(hdc, hFontUnderlined);
						}
					}
					if (!(pageList[i].type & 1)) {
						Rectangle(hdc, pageList[i].left, rect.bottom-18, pageList[i].right, rect.bottom-2);
					}
					TextOutA(hdc, pageList[i].left+PAGE_PAD/2, rect.bottom-17, pageList[i].str, strlen(pageList[i].str));
					if (i == this->hovered-1) {
						SelectObject(hdc, bgBrush1);
						SelectObject(hdc, hFont);
					}
					if (pageList[i].type & 2)
						break;
					if (i > this->lastPage+4) {
						errorf("too many PageList loops");
						break;
					}
				}
			}

			SelectObject(hdc, hOldPen);
			SelectObject(hdc, hOldFont);
			SelectObject(hdc, hOldBrush);
			if (!(DeleteObject(tempBrush))) {
				errorf("DeleteObject failed");
			}
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_MOUSEMOVE: {
			{

				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}
				int hvrdbefore = this->hovered;
				this->hovered = 0;

				if (this->pageListPtr) {
					auto &pageList = *pageListPtr;
					if (((i = lParam/256/256) >= rect.bottom-18) && (i < rect.bottom-2)) {
						int temp2 = lParam % (256*256);
						for (i = 0; ; i++) {
//							if (!(pageList[i].type & 1)) {
								if ((temp2 >= pageList[i].left) && (temp2 <= pageList[i].right)) {
									this->hovered = i+1;
									break;
								}
//							}
							if (pageList[i].type & 2) {
								break;
							}
						}
					}
				}

				if (this->hovered != hvrdbefore) {
					if (!(InvalidateRect(hwnd, 0, 0))) {
						errorf("InvalidateRect failed");
					}
				}

				if (this->hovered) {
					SetCursor(hCrsHand);
				} else {
					SetCursor(hDefCrs);
				}
			}
			break;
		}
		case WM_LBUTTONDOWN: {

			if (!this->pageListPtr) {
				break;
			}
			auto &pageList = *pageListPtr;

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (((i = lParam/256/256) >= rect.bottom-18) && (i < rect.bottom-2)) {
				int temp2 = lParam % (256*256);

				for (i = 0; ; i++) {
					if (!(pageList[i].type & 1)) {
						if ((temp2 >= pageList[i].left)	&& (temp2 <= pageList[i].right)) {
							if (pageList[i].str[0] == '<') {
								if (pageList[i].str[1] == '<')
									this->curPage = 1;
								else
									this->curPage--;
							} else if (pageList[i].str[0] == '>') {
								if (pageList[i].str[1] == '>')
									this->curPage = this->lastPage;
								else
									this->curPage++;
							} else {
								this->curPage = ctou(pageList[i].str);
							}

							for (int j = 0; ; j++) {
								if (pageList[j].str != nullptr) {
									free(pageList[j].str);
									pageList[j].str = nullptr;
								}
								if (pageList[j].type & 2)
									break;
							}
							this->getPages(rect.right);
							this->hovered = 0;
							SetCursor(hDefCrs);
							if (!(InvalidateRect(hwnd, 0, 1))) {
								errorf("InvalidateRect failed");
							}
							SendMessage(GetParent(hwnd), WM_U_RET_PL, 1, 0);

							break;
						}
					}
					if (pageList[i].type & 2)
						break;
				}
			}

			break;
		}
		case WM_U_PL: {
			switch (wParam) {
				case PL_REFRESH: {
					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					this->getPages(rect.right);

					break;
				}
			}
		}
		case WM_ERASEBKGND: {
			return 1;
		}
		case WM_NCDESTROY: {
			return 0;
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//} PageListClass

//{ StrListClass
//public:
// static
const WindowHelper StrListClass::helper = WindowHelper(std::wstring(L"StrListClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND StrListClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new StrListClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

int64_t StrListClass::getSingleSelPos(void) const {
	if (this->getNRows() <= 0) {
		errorf("getSingleSelPos getNRows <= 0");
		return -1;
	}
	int64_t gotNRows = this->getNRows();

	if (this->rowSelsPtr == nullptr) {
		errorf("getSingleSelPos rowSelsPtr was null");
		return -1;
	}
	auto selsPtrHolder = this->rowSelsPtr;

	auto &rowSels = *selsPtrHolder;

	int64_t pos, i = -1;

	for (pos = 0; pos < gotNRows; pos++) {
		if (rowSels[pos]) {
			if (i == -1) {
				i = pos;
			} else {
				errorf("getSingleSelPos had multiple selections");
				return -1;
			}
		}
	}

	if (pos != gotNRows) {
		errorf("didn't go through all rows");
		return -1;
	}

	return i;
}

std::shared_ptr<std::vector<std::string>> StrListClass::getSingleSelRowCopy(void) const {
	int64_t pos = this->getSingleSelPos();

	if (pos < 0) {
		return {};
	}

	std::shared_ptr<std::vector<std::string>> rowCopyPtr( new std::vector<std::string>((*this->rowsPtr)[pos]) );

	return rowCopyPtr;
}

std::string StrListClass::getSingleSelRowCol(int32_t argCol, ErrorObject *retError) const {
errorf("gssrc pos 0");
	if (argCol >= this->getNColumns() || argCol < 0) {
errorf("gssrc err1");
		setErrorPtr(retError, 2);
		return {};
	}

	int64_t pos = this->getSingleSelPos();
g_errorfStream << "gssrc got singleselpos: " << pos << std::flush;

	if (pos < 0) {
errorf("gssrc err2 ");
		setErrorPtr(retError, 1);
		return {};
	}

errorf("gssrc pos 5");
	return (*this->rowsPtr)[pos][argCol];
}


std::shared_ptr<std::forward_list<int64_t>> StrListClass::getSelPositions(void) const {
	if (this->getNRows() <= 0) {
		errorf("getSelPositions getNRows <= 0");
		return {};
	}
	int64_t gotNRows = this->getNRows();

	if (this->rowSelsPtr == nullptr) {
		errorf("getSelPositions rowSelsPtr was null");
		return {};
	}
	auto selsPtrHolder = this->rowSelsPtr;

	auto &rowSels = *selsPtrHolder;

	if (this->rowsPtr == nullptr) {
		errorf("getSelPositions rowsPtr was null");
		return {};
	}
	auto rowsPtrHolder = this->rowsPtr;

	auto &rows = *this->rowsPtr;

	std::shared_ptr<std::forward_list<int64_t>> selListPtr( new std::forward_list<int64_t>() );

	if (selListPtr == nullptr) {
		errorf("shared_ptr alloc fail");
		return {};
	}

	std::forward_list<int64_t> &selList = *selListPtr;

	auto it = selList.before_begin();

	int64_t pos, i;

	for (pos = 0, i = 0; pos < gotNRows; pos++) {
		if (rowSels[pos]) {
			int64_t toEmplace = pos;
			it = selList.emplace_after(it, toEmplace);
			i++;
		}
	}

	if (pos != gotNRows) {
		errorf("didn't go through all rows");
		return {};
	}

	return selListPtr;
}

bool StrListClass::setNColumns(int32_t argI) {
	if (argI < 0) {
		errorf("tried to set nColumns to less than 0");
		return false;
	}

	this->nColumns = argI;

	sectionHeadersPtr = nullptr;
	rowsPtr = nullptr;
	rowSelsPtr = nullptr;

	columnWidthsPtr = std::shared_ptr<std::vector<int16_t>>( new std::vector<int16_t>(argI) );

	return true;

}

bool StrListClass::setHeaders(std::shared_ptr<std::vector<std::string>> argHeaderPtr) {
	if (!argHeaderPtr) {
		errorf("setHeaders received nullptr");
		return false;
	}

	auto &headerVec = *argHeaderPtr;

	if (headerVec.size() != this->getNColumns()) {
		errorf("setHeaders vector size doesn't match nColumns");
		return false;
	} else {
		this->sectionHeadersPtr = argHeaderPtr;
		return true;
	}
}

bool StrListClass::setColumnWidth(int32_t colArg, int32_t width) {
	auto columnWidthsHoldPtr = this->columnWidthsPtr;
	if (!columnWidthsHoldPtr) {
		errorf("columnWidthsPtr was null");
		return false;
	}
	auto &colWidths = *columnWidthsHoldPtr;

	int32_t gotNColumns = this->getNColumns();

	if (colArg >= gotNColumns) {
		errorf("colArg >= gotNColumns");
		return false;
	} else if (colArg < 0) {
		errorf("colArg < 0");
		return false;
	}

	colWidths[colArg] = width;
	if (width < this->minColWidth) {
		colWidths[colArg] = this->minColWidth;
	} else if (width > this->maxColWidth) {
		colWidths[colArg] = this->maxColWidth;
	}

	return true;

}

int32_t StrListClass::getNColumns(void) const {
	return this->nColumns;
}

int32_t StrListClass::getTotalColumnsWidth(void) const {
	auto holdPtr = this->columnWidthsPtr;
	if (holdPtr == nullptr) {
		return 0;
	}

	auto &columnWidths = *holdPtr;

	int32_t res = 0;
	for (const auto &width : columnWidths) {
		res += width;
	}

	return res;
}

bool StrListClass::clearRows(void) {
	this->rowsPtr = nullptr;

	// clear selections as well
	bool b_clearRes = this->clearSelections();

	return true;
}

bool StrListClass::assignRows(std::shared_ptr<std::vector<std::vector<std::string>>> inputVectorPtr) {
	if (!inputVectorPtr) {
		this->rowsPtr = nullptr;
		this->rowSelsPtr = nullptr;
		this->nRows = 0;

		return true;
	} else {
		auto &inputVector = *inputVectorPtr;

		int32_t gotNColumns = this->getNColumns();

		for (auto &row : inputVector) {
			if (row.size() != gotNColumns) {
				errorf("input row had mismatching columns");
				this->assignRows(nullptr);
				return false;
			}
		}

		this->rowsPtr = inputVectorPtr;
		this->nRows = inputVector.size();
		this->rowSelsPtr.reset( new boost::dynamic_bitset<>(this->nRows) );

		return true;
	}
}

int64_t StrListClass::getNRows(void) const {
	return this->nRows;
}

bool StrListClass::clearSelections(void) {
	auto selsHoldPtr = this->rowSelsPtr;

	if (selsHoldPtr) {
		auto &rowSels = *selsHoldPtr;
		rowSels.reset();
		return true;
	}
	errorf("StrListClass::clearSelections no rowSelsPtr");
	return false;
}

LRESULT CALLBACK StrListClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

/*
	RECT rect;
	HWND thwnd;

	int64_t i;
	int j, k, l, pos;
	uint8_t *buf, exit = 0;
	wchar_t *wbuf;
	SCROLLINFO sinfo = {0};
	//STRLISTV *lv = 0;


	oneslnk **link;

	int len, diff;
	int64_t lastrow;
	char uneven, sel;
*/

	switch(msg) {
		case WM_CREATE: {
errorf("strlistclass create 1");
			this->yspos = this->xspos = 0;
			this->hvrd = 0;
			this->drag = 0;
			this->timed = 0;
			this->lastSel = 0;

			this->assignRows(nullptr);

			this->setNColumns(0);

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			//! TODO: add a method to do this instead
			SCROLLINFO sinfo = {0};
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_ALL;
			sinfo.nMin = 0;
			sinfo.nMax = ROW_HEIGHT*this->nRows-1;		// since it starts from 0 ROW_HEIGHT*nRows would be the first pixel after the area
			sinfo.nPage = rect.bottom-STRLIST_TOP_MRG;
			sinfo.nPos = this->yspos;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!(ShowScrollBar(hwnd, SB_VERT, 1))) {
				errorf("ShowScrollBar failed.");
			}

			int j = this->getTotalColumnsWidth();
			sinfo.nMax = j-1;	// the first column has its first pixel off-screen
			sinfo.nPage = rect.right;
			sinfo.nPos = this->xspos;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

errorf("strlistclass create 9");

			break;
		}
		case WM_PAINT: {

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			HDC hdc, hdc2;
			PAINTSTRUCT ps;
			HFONT hOldFont;
			HPEN hOldPen;
			HBRUSH hOldBrush;
			HBITMAP hOldBM;

			hdc = BeginPaint(hwnd, &ps);

			hdc2 = CreateCompatibleDC(hdc);
			hOldPen = (HPEN) GetCurrentObject(hdc2, OBJ_PEN);
			hOldBrush = (HBRUSH) GetCurrentObject(hdc2, OBJ_BRUSH);
			hOldFont = (HFONT) SelectObject(hdc2, hFont);
			SetBkMode(hdc2, TRANSPARENT);

			int64_t firstRow = this->yspos/ROW_HEIGHT;
			// upOffset is the number of pixels outside the window in the first row; ROW_HEIGHT-upOffset is the number of pixels in the first row
			int32_t upOffset = this->yspos%ROW_HEIGHT;

			if (!hListSliceBM) {
				int64_t j = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				if (j == 0) {
					j = 1920;
				}
				hListSliceBM = CreateCompatibleBitmap(hdc, j, ROW_HEIGHT);
			}
			hOldBM = (HBITMAP) SelectObject(hdc2, hListSliceBM);

			int64_t lastRow = firstRow + (!!upOffset) + (rect.bottom-STRLIST_TOP_MRG-(ROW_HEIGHT-upOffset)%ROW_HEIGHT)/ROW_HEIGHT + (((rect.bottom-STRLIST_TOP_MRG-(ROW_HEIGHT - upOffset) % ROW_HEIGHT)) > 0) - 1;
			// (rect.bottom-STRLIST_TOP_MRG-(ROW_HEIGHT-upOffset)%ROW_HEIGHT)/ROW_HEIGHT
			// = (rect.bottom-STRLIST_TOP_MRG-(ROW_HEIGHT-upOffset))/ROW_HEIGHT + !upOffset
			// = (rect.bottom-STRLIST_TOP_MRG + upOffset))/ROW_HEIGHT + !upOffset - 1
			// !!upOffset + !upOffset - 1 = 0
			//! TODO: uncomment and test
			// int64_t lastRow = firstRow - 1 + (rect.bottom-STRLIST_TOP_MRG + upOffset))/ROW_HEIGHT + !!((rect.bottom-STRLIST_TOP_MRG + upOffset) % ROW_HEIGHT);

			auto rowSelsHoldPtr = this->rowSelsPtr;
			auto rowsHoldPtr = this->rowsPtr;
			auto columnWidthsHoldPtr = this->columnWidthsPtr;
			auto headersHoldPtr = this->sectionHeadersPtr;



			int32_t gotNColumns = this->getNColumns();
			int64_t gotNRows = this->getNRows();

			//! TODO: deinitialize paint objects on return

			// if no columns, draw a single column with no rows
			if (!columnWidthsHoldPtr || gotNColumns == 0) {
errorf("no columnWidthsHoldPtr");
				gotNColumns = 1;
				gotNRows = 0;
				columnWidthsHoldPtr = std::shared_ptr<std::vector<int16_t>>( new std::vector<int16_t>({1}) );
				headersHoldPtr = std::shared_ptr<std::vector<std::string>>( new std::vector<std::string>{""});
			}

			//! TODO: allow rowSels not to be defined
			if (!rowSelsHoldPtr || !rowsHoldPtr) {
				gotNRows = 0;
				rowsHoldPtr = std::make_shared<std::vector<std::vector<std::string>>>();
			}

			auto &columnWidths = *columnWidthsHoldPtr;
			auto &rowSels = *rowSelsHoldPtr;

			auto &rows = *rowsHoldPtr;

			bool uneven = (firstRow+1) % 2;
			for (int64_t row = firstRow; row <= lastRow; row++, uneven = !uneven) {
				bool sel = false;
				for (int32_t col = 0; col < gotNColumns; col++) {
					// k is set to totalColumnWidths before
					//int32_t k = 0;
					int32_t w1 = 0;
					for (int32_t tempCol = 0; tempCol < col; w1 += columnWidths[tempCol], tempCol++);
					w1 -= this->xspos;
					// column has left edge is after view area
					if (w1 >= rect.right) {
						break;
					}

					int32_t w2 = 0;
					// column has right edge before view area
					if ((w2 = w1 + columnWidths[col]) < 0) {
						continue;
					}
					// is last column
					if (col+1 >= gotNColumns) {
						w2 = rect.right;
					}
					// row is selected
					if (row < gotNRows && rowSels[row]) {
						SelectObject(hdc2, selPen);
						SelectObject(hdc2, selBrush);
						sel = true;
					// row in uneven position
					} else if (uneven) {
						if (!(col % 2)) {
							SelectObject(hdc2, bgPen2);
							SelectObject(hdc2, bgBrush2);
						} else {
							SelectObject(hdc2, bgPen4);
							SelectObject(hdc2, bgBrush4);
						}
					// row in even position
					} else {
						if (!(col % 2)) {
							SelectObject(hdc2, bgPen3);
							SelectObject(hdc2, bgBrush3);
						} else {
							SelectObject(hdc2, bgPen5);
							SelectObject(hdc2, bgBrush5);
						}
					}
					Rectangle(hdc2, w1, 0, w2 + (sel && !(col+1 == gotNColumns)), ROW_HEIGHT);
					if (sel && col > 0) {
						SelectObject(hdc2, selPen2);
						MoveToEx(hdc2, w1, 1, NULL);
						LineTo(hdc2, w1, ROW_HEIGHT-1);
					}

					if (row < gotNRows) {
						std::wstring wstr = u8_to_u16(rows[row][col]);

						TextOutW(hdc2, w1+3, (ROW_HEIGHT)-(ROW_HEIGHT/2)-7, wstr.c_str(), wstr.length());

						/*

						if (this->strs[j][i] == 0) {
							if ((MultiByteToWideChar(65001, 0, "<missing>", -1, wbuf, MAX_PATH)) == 0) {
								errorf("MultiByteToWideChar Failed");
							}
						} else if ((MultiByteToWideChar(65001, 0, this->strs[j][i], -1, wbuf, MAX_PATH)) == 0) {
							errorf("MultiByteToWideChar Failed");
						}
						for (len = 0; wbuf[len] != '\0' && len < MAX_PATH; len++);
						if (len >= MAX_PATH) {
							errorf("strlist string too long");
						} else {
							TextOutW(hdc2, w1 + 3, (ROW_HEIGHT)-(ROW_HEIGHT/2)-7, wbuf, len);
						}
						*/
					}
				}
				if (row == 0) {
					BitBlt(hdc, 0, STRLIST_TOP_MRG, rect.right, ROW_HEIGHT- upOffset, hdc2, 0, upOffset, SRCCOPY);
				} else {
					BitBlt(hdc, 0, STRLIST_TOP_MRG+(row)*ROW_HEIGHT - upOffset, rect.right, ROW_HEIGHT, hdc2, 0, 0, SRCCOPY);
				}
			}
			SelectObject(hdc2, hOldPen);
			SelectObject(hdc2, hOldBrush);
			SelectObject(hdc2, hOldFont);
			DeleteDC(hdc2);

			hOldBrush = (HBRUSH) SelectObject(hdc, bgBrush1);
			hOldPen = (HPEN) SelectObject(hdc, bgPen1);
			hOldFont = (HFONT) SelectObject(hdc, hFont);

			Rectangle(hdc, 0, 0, rect.right, STRLIST_TOP_MRG);

			SelectObject(hdc, hPen1);
			MoveToEx(hdc, 0, STRLIST_TOP_MRG-1, NULL);
			LineTo(hdc, rect.right, STRLIST_TOP_MRG-1);

			SetBkMode(hdc, TRANSPARENT);

			if (!headersHoldPtr) {
				errorf("headersHoldPtr was null");
				break;
			}

			// spec: if nColumns > 0, sectionHeadersPtr must hold a vector with nColumns elements
			auto &headers = *headersHoldPtr;

			// draw column headers
			//! TODO: use bitblt so there shouldn't be any flickering issues or overflow from the area
			for (int32_t col = 0; col < gotNColumns; col++) {
				int32_t w1 = 0;
				for (int32_t tempCol = 0; tempCol < col; w1 += columnWidths[tempCol], tempCol++);
				w1 -= this->xspos;
				// column has left edge is after view area
				if (w1 >= rect.right) { //! TODO: test near 0
					break;
				}

				int32_t w2 = 0;
				// column has right edge before view area
				if ((w2 = w1 + columnWidths[col]) < 0) {
					continue;
				}
				// is last column
				if (col+1 >= gotNColumns) {
					w2 = rect.right;
				}

				Rectangle(hdc, w1, 0, w2, ROW_HEIGHT);

				std::wstring wstr = u8_to_u16(headers[col]);

				TextOutW(hdc, w1 + 3, (ROW_HEIGHT)-(ROW_HEIGHT/2)-7, wstr.c_str(), wstr.length());

				/*

				if (this->header) {
					if (this->header[col] == 0) {
						if ((MultiByteToWideChar(65001, 0, "<missing>", -1, wbuf, MAX_PATH)) == 0) {
							errorf("MultiByteToWideChar Failed");
						}
					} else if ((MultiByteToWideChar(65001, 0, this->header[col], -1, wbuf, MAX_PATH)) == 0) {
						errorf("MultiByteToWideChar Failed");
					}
					for (len = 0; wbuf[len] != '\0' && len < MAX_PATH; len++);
					if (len >= MAX_PATH) {
						errorf("strlist header string too long");
					} else {
						TextOutW(hdc, w1 + 3, (STRLIST_TOP_MRG-14)/2, wbuf, len);
					}
				}
				*/
				MoveToEx(hdc, w1, 0, NULL);
				LineTo(hdc, w1, STRLIST_TOP_MRG-1);
			}

			SelectObject(hdc, hOldPen);
			SelectObject(hdc, hOldBrush);
			SelectObject(hdc, hOldFont);
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_USER: {
			SCROLLINFO sinfo = {0};
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			auto widthsHoldPtr = this->columnWidthsPtr;
			if (!widthsHoldPtr) {
				return 1;
			}

			auto &widths = *widthsHoldPtr;

			int32_t gotNColumns = this->getNColumns();

			switch (wParam) {
			case 1: {
				sinfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
				sinfo.nMin = 0;
				if (this->nRows == 0)
					sinfo.nMax = 0;
				else
					sinfo.nMax = ROW_HEIGHT*this->nRows-1;
				sinfo.nPos = this->yspos = 0;
				sinfo.nPage = rect.bottom-STRLIST_TOP_MRG;

				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				int32_t w2 = 0;
				for (int32_t i1 = 0; i1 < this->nColumns; w2 += widths[i1], i1++);
				sinfo.nMax = w2 - 1;
				sinfo.nPage = rect.right;
				sinfo.nPos = this->xspos = 0;
				SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);


				break;
			}
			case 2: {
				sinfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
				if (this->nRows == 0) {
					sinfo.nMax = 0;
				} else {
					sinfo.nMax = ROW_HEIGHT*this->nRows-1;
				}
				sinfo.nPos = this->yspos = 0;
				sinfo.nPage = rect.bottom-STRLIST_TOP_MRG;	// don't know why setting page is necessary

				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				int32_t w2 = 0;

				for (int32_t col1 = 0; col1 < gotNColumns; w2 += widths[col1], col1++);
				sinfo.nMax = w2 - 1;
				sinfo.nPage = rect.right;
				sinfo.nPos = this->xspos = 0;
				SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

				if ((this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG) || (rect.bottom-STRLIST_TOP_MRG <= 0)) {
					this->yspos = 0;
					ShowScrollBar(hwnd, SB_VERT, 0);
				} else {
					ShowScrollBar(hwnd, SB_VERT, 1);
					if (this->yspos > this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG))
						this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
				}
				if ((w2 <= rect.right) || (rect.right <= 0)) {
					this->xspos = 0;
					ShowScrollBar(hwnd, SB_HORZ, 0);
				} else {
					ShowScrollBar(hwnd, SB_VERT, 1);
					if (this->xspos > w2 - rect.right)
						this->xspos = w2 - rect.right;
				}
				break;
			}
			}
		}
		case WM_U_SL: {
			switch (wParam) {
				case SL_CHANGE_PAGE: {
					this->yspos = this->xspos = 0;
					this->hvrd = 0;
					if (this->drag) {
						if (!ReleaseCapture()) {
							errorf("ReleaseCapture failed");
						}
					}
					this->drag = 0;
					this->timed = 0;
					this->lastSel = 0;

					RECT rect;
					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					//! TODO: add a method to do this instead
					SCROLLINFO sinfo = {0};
					sinfo.cbSize = sizeof(SCROLLINFO);
					sinfo.fMask = SIF_ALL;
					sinfo.nMin = 0;
					sinfo.nMax = ROW_HEIGHT*this->getNRows()-1;		// since it starts from 0 ROW_HEIGHT*nRows would be the first pixel after the area
					sinfo.nPage = rect.bottom-STRLIST_TOP_MRG;
					sinfo.nPos = this->yspos;
					SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

					if (!(ShowScrollBar(hwnd, SB_VERT, 1))) {
						errorf("ShowScrollBar failed.");
					}

					int j = this->getTotalColumnsWidth();
					sinfo.nMax = j-1;	// the first column has its first pixel off-screen
					sinfo.nPage = rect.right;
					sinfo.nPos = this->xspos;
					SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

					InvalidateRect(hwnd, NULL, 1);

					break;
				}
			}
		}
		case WM_SIZE: {

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			auto widthsHoldPtr = this->columnWidthsPtr;
			if (!widthsHoldPtr) {
				return 1;
			}

			auto &widths = *widthsHoldPtr;

			int32_t gotNColumns = this->getNColumns();

			if (this->nRows*ROW_HEIGHT <= (rect.bottom-STRLIST_TOP_MRG) || (rect.bottom-STRLIST_TOP_MRG <= 0)) {
				this->yspos = 0;
				ShowScrollBar(hwnd, SB_VERT, 0);
			} else {
				ShowScrollBar(hwnd, SB_VERT, 1);
				if (this->yspos > this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG))
					this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
			}
			int32_t w2 = 0;
			for (int32_t col1 = 0; col1 < gotNColumns; w2 += widths[col1], col1++);
			if (w2 <= rect.right || rect.right <= 0) {
				this->xspos = 0;
				ShowScrollBar(hwnd, SB_HORZ, 0);
			} else {
				ShowScrollBar(hwnd, SB_HORZ, 1);
				if (this->xspos > w2 - rect.right)
					this->xspos = w2 - rect.right;
			}

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_PAGE | SIF_POS;
			sinfo.nPage = rect.bottom-STRLIST_TOP_MRG;
			sinfo.nPos = this->yspos;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			sinfo.nPage = rect.right;
			sinfo.nPos = this->xspos;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

			if (!this->timed) {		// if the previous timer is a '1' timer, the top text windows will not change
				SetTimer(hwnd, 0, 16, 0);
				this->timed = 1;
			}

			break;
		}
		case WM_VSCROLL: {
			bool b_abort = false;
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			switch(LOWORD(wParam)) {

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					this->yspos = HIWORD(wParam);

					break;

				case SB_LINEUP:

					if (this->yspos <= ROW_HEIGHT) {
						if (this->yspos == 0) {
							b_abort = true;
						} else {
							this->yspos = 0;
						}
					} else
						this->yspos -= ROW_HEIGHT;

					break;

				case SB_LINEDOWN:

					if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG || this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {	// nRows*ROW_HEIGHT is the length of the rows and rect.bottom-STRLIST_TOP_MRG is the length of the view area
						b_abort = true;																													// their difference is the amount of pixels outside of view, each increase in yspos reveals one from the bottom
					} else if (this->yspos >= (this->nRows-1)*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {	// if nRows is changed to an unsigned value all line downs and page downs will break (could be fixed with a cast or moving the removed row or page to the left side of the comparison)
						this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
					} else {
						this->yspos += ROW_HEIGHT;
					}
					break;

				case SB_PAGEUP:

					if (this->yspos <= rect.bottom-STRLIST_TOP_MRG) {
						if (this->yspos == 0) {
							b_abort = true;
						} else {
							this->yspos = 0;
						}
					} else this->yspos -= rect.bottom-STRLIST_TOP_MRG;

					break;

				case SB_PAGEDOWN:

					if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG || this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
						b_abort = true;
					} else if (this->yspos >= (this->nRows)*ROW_HEIGHT-2*(rect.bottom-STRLIST_TOP_MRG)) {
						this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
					} else
						this->yspos += rect.bottom-STRLIST_TOP_MRG;

					break;

				case SB_TOP:

					if (this->yspos == 0) {
						b_abort = true;
					} else
						this->yspos = 0;

					break;

				case SB_BOTTOM:
					if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG || this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
						b_abort = true;
					} else
						this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);

					break;

				default:
					b_abort = true;
					break;
			} if (b_abort)
				break;

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->yspos;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->timed) {
				SetTimer(hwnd, 1, 34, 0);
				this->timed = 1;
			}

			break;
		}
		case WM_HSCROLL: {
			bool b_abort = false;
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			switch(LOWORD(wParam)) {

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					this->xspos = HIWORD(wParam);

					break;

				case SB_LINEUP:

					if (this->xspos <= ROW_HEIGHT) {
						if (this->xspos == 0) {
							b_abort = true;
						} else {
							this->xspos = 0;
						}
					} else
						this->xspos -= ROW_HEIGHT;

					break;

				case SB_LINEDOWN: {
					int32_t w1 = this->getTotalColumnsWidth();

					if (w1 <= rect.right || this->xspos == w1 - rect.right) {
						b_abort = true;
					} else if (this->xspos >= w1 - rect.right-ROW_HEIGHT) {
						this->xspos = w1 - rect.right;		// if nRows is changed to an unsigned value all line downs and page downs will break
					} else {
						this->xspos += ROW_HEIGHT;
					}
					break;
				}
				case SB_PAGEUP:

					if (this->xspos <= rect.right) {
						if (this->xspos == 0) {
							b_abort = true;
						} else {
							this->xspos = 0;
						}
					} else this->xspos -= rect.right;

					break;

				case SB_PAGEDOWN: {
					int32_t w1 = this->getTotalColumnsWidth();

					if (w1 <= rect.right || this->xspos == w1 - rect.right) {
						b_abort = true;
					} else if (this->xspos >= w1 - 2*rect.right) {
						this->xspos = w1 - rect.right;
					} else {
						this->xspos += rect.right;
					}

					break;
				}
				case SB_TOP:

					if (this->xspos == 0) {
						b_abort = true;
					} else
						this->xspos = 0;

					break;

				case SB_BOTTOM: {
					int32_t w1 = this->getTotalColumnsWidth();

					if (w1 <= rect.right || this->xspos == w1 - rect.right) {
						b_abort = true;
					} else {
						this->xspos = w1 - rect.right;
					}
					break;
				}
				default: {
					b_abort = true;
					break;
				}
			} if (b_abort) {
				break;
			}

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->xspos;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

			if (!this->timed) {
				SetTimer(hwnd, 1, 34, 0);
				this->timed = 1;
			}

			break;
		}
		case WM_MOUSEWHEEL: {
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG)
				break;

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_POS;

			if ((HIWORD(wParam))/ROW_HEIGHT/2*ROW_HEIGHT == 0)
				break;

			if (this->yspos <= (int16_t)(HIWORD(wParam))/ROW_HEIGHT/2*ROW_HEIGHT) {
				if (this->yspos == 0)
					break;
				this->yspos = 0;
			} else if (this->yspos >= (this->nRows*ROW_HEIGHT)-(rect.bottom-STRLIST_TOP_MRG)+((int16_t)(HIWORD(wParam))/ROW_HEIGHT/2*ROW_HEIGHT)) {	// wParam is negative if scrolling down
				if (this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG))
					break;
				this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
			} else this->yspos -= (int16_t)(HIWORD(wParam))/ROW_HEIGHT/2*ROW_HEIGHT;
			sinfo.nPos = this->yspos;

			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->timed)  {
				SetTimer(hwnd, 1, 34, 0);
				this->timed = 1;
			}

			break;
		}
		case WM_MOUSEMOVE: {

			auto widthsHoldPtr = this->columnWidthsPtr;
			if (!widthsHoldPtr) {
				break;
			}
			auto &widths = *widthsHoldPtr;

			if (!this->drag) {
				int32_t w2 = 0;
				int32_t col1 = 0;

				for (col1 = 0, w2 = -this->xspos; col1 < this->nColumns-1; col1++) {
					w2 += widths[col1];
					if ((lParam & (256*256-1)) >= w2 - 2 && (lParam & (256*256-1)) <= w2 + 2) {
						this->hvrd = col1 + 1;
						SetCursor(hCrsSideWE);
						break;
					}
				} if (col1 == this->nColumns-1) {
					this->hvrd = 0;
					SetCursor(hDefCrs);
				}
				break;
			}
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			int32_t col2 = this->drag-1;
			widths[col2] = (((int16_t) (lParam)) - this->drgpos);
			if (widths[col2] < 5)
				widths[col2] = 5;
			if (widths[col2] > rect.right-5+this->xspos) {
				widths[col2] = rect.right-5+this->xspos;
			}

			int32_t w1 = this->getTotalColumnsWidth();
			if (w1 <= rect.right || rect.right <= 0) {
				this->xspos = 0;
				ShowScrollBar(hwnd, SB_HORZ, 0);
			} else {
				ShowScrollBar(hwnd, SB_HORZ, 1);
				if (this->xspos > w1 - rect.right)
					this->xspos = w1 - rect.right;
			}

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			sinfo.nMin = 0;
			sinfo.nMax = w1 - 1;
			sinfo.nPos = this->xspos;
			sinfo.nPage = rect.right;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

			if (!this->timed) {
				SetTimer(hwnd, 0, 34, 0);
				this->timed = 1;
			}

			break;
		}
		case WM_LBUTTONDOWN: {
			RECT rect2 = {};

			rect2.left = lParam & (256*256-1);
			rect2.top = lParam/256/256 & (256*256-1);

			if (!this->hvrd || this->drag) {
				if (rect2.top >= STRLIST_TOP_MRG && !this->drag) {
					// the row the cursor is hovering over
					int32_t hoverRow = this->yspos/ROW_HEIGHT+(rect2.top-STRLIST_TOP_MRG + this->yspos%ROW_HEIGHT)/ROW_HEIGHT;	// first position is 0

					if (!(wParam & MK_CONTROL) || (this->option & 1)) {
						this->clearSelections();
					}

					if (hoverRow < this->nRows) {
						auto selsHoldPtr = rowSelsPtr;
						if (!selsHoldPtr) {
							break;
						}

						auto &rowSels = *selsHoldPtr;

						if (!(wParam & MK_SHIFT) || (this->option & 1)) {
							rowSels[hoverRow] = !rowSels[hoverRow];
							this->lastSel = hoverRow;
						//! TODO: with shift, select rows in range if hoverRow is selected and deselect otherwise
						} else {
							int64_t sel1 = this->lastSel;
							bool b_lastSelState = rowSels[sel1];
							if (sel1 <= hoverRow) {
								sel1++;
								for (; sel1 <= hoverRow; sel1++) {
									rowSels[sel1] = b_lastSelState;
								}
							} else {
								sel1--;
								for (; sel1 >= hoverRow; sel1--) {
									rowSels[sel1] = b_lastSelState;
								}
							}
						}
					}
					if (!(InvalidateRect(hwnd, 0, 1))) {
						errorf("InvalidateRect failed");
					}
				}
			} else {
				auto widthsHoldPtr = this->columnWidthsPtr;
				if (!widthsHoldPtr) {
					break;
				}
				auto &colWidths = *widthsHoldPtr;

				this->drag = this->hvrd;
				this->drgpos = rect2.left - colWidths[this->hvrd-1];
				SetCapture(hwnd);
			}

			break;
		}
		case WM_LBUTTONUP: {

			if (!this->drag)
				break;
			if (!ReleaseCapture())
				errorf("ReleaseCapture failed");
			this->drag = 0;
			SendMessage(hwnd, WM_MOUSEMOVE, wParam, lParam);

			break;
		}
		case WM_RBUTTONDOWN: {
			RECT rect2 = {};

			this->point.x = rect2.left = lParam & (256*256-1);
			this->point.y = rect2.top = lParam/256/256 & (256*256-1);
			if (rect2.top < STRLIST_TOP_MRG)
				break;
			MapWindowPoints(hwnd, 0, &this->point, 1);

			if (!(wParam & MK_CONTROL)) {
				int64_t hoverRow = this->yspos/ROW_HEIGHT+(rect2.top-STRLIST_TOP_MRG + this->yspos%ROW_HEIGHT)/ROW_HEIGHT;
				if (hoverRow >= this->nRows) {
					break;
				}
				// if row is not selected, clear selections
				auto selsHoldPtr = this->rowSelsPtr;
				if (!selsHoldPtr) {
					break;
				}
				auto &rowSels = *selsHoldPtr;

				if (!rowSels[hoverRow]) {
					this->clearSelections();

					rowSels[hoverRow] = !rowSels[hoverRow];
					this->lastSel = hoverRow;
				}
				if (!(InvalidateRect(hwnd, 0, 1))) {
					errorf("InvalidateRect failed");
				}
			}
			SendMessage(GetParent(hwnd), WM_U_RET_SL, 4, (LPARAM) GetMenu(hwnd));

			break;
		}
		case WM_TIMER: {
			bool b_abort = false;

			switch(wParam) {

			case 0:
			case 1:

				KillTimer(hwnd, 0);
				this->timed = 0;

				if (!(InvalidateRect(hwnd, 0, 0))) {
					errorf("InvalidateRect failed");
				}
				break;

			case 2:

				KillTimer(hwnd, 2);
				SetTimer(hwnd, 2, 67, 0);

				RECT rect;

				switch(this->LastKey) {
				case 1:
					if (this->yspos == 0) {
						b_abort = true;
					} else if (this->yspos <= ROW_HEIGHT) {
						this->yspos = 0;
					} else
						this->yspos -= ROW_HEIGHT;

					break;

				case 2:
					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG || this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
						b_abort = true;
					}
					else if (this->yspos >= (this->nRows-1)*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
						this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
					} else
						this->yspos += ROW_HEIGHT;

					break;

				case 3:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->yspos == 0) {
						b_abort = true;
					} else if (this->yspos <= rect.bottom-STRLIST_TOP_MRG) {
						this->yspos = 0;
					} else
						this->yspos -= rect.bottom-STRLIST_TOP_MRG;

					break;

				case 4:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG || this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
						b_abort = true;
					} else if (this->yspos >= (this->nRows)*ROW_HEIGHT-2*(rect.bottom-STRLIST_TOP_MRG)) {
						this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
					} else
						this->yspos += rect.bottom-STRLIST_TOP_MRG;

					break;

				case 5:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->xspos <= ROW_HEIGHT) {
						if (this->xspos == 0) {
							b_abort = true;
						} else {
							this->xspos = 0;
						}
					} else
						this->xspos -= ROW_HEIGHT;

					break;

				case 6: {

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					int32_t w1 = this->getTotalColumnsWidth();

					if (w1 <= rect.right || this->xspos == w1 - rect.right) {
						b_abort = true;
					} else if (this->xspos >= w1 - rect.right-ROW_HEIGHT) {
						this->xspos = w1 - rect.right;
					} else {
						this->xspos += ROW_HEIGHT;
					}

					break;
				}
				default: {
					KillTimer(hwnd, 2);
					b_abort = true;
					break;
				}
				}

				if (b_abort)
					break;

				SCROLLINFO sinfo = {0};

				sinfo.fMask = SIF_POS;
				sinfo.nPos = this->yspos;
				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);
				sinfo.nPos = this->xspos;
				SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

				if (!this->timed) {
					SetTimer(hwnd, 1, 34, 0);
					this->timed = 1;
				}

				break;
			}
			break;
		}
		case WM_KEYDOWN: {
			bool b_abort = false;
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if ((lParam & 0x40000000) != 0)
				break;

			KillTimer(hwnd, 2);

			switch(wParam) {

			case VK_HOME:
				if (this->yspos == 0) {
					b_abort = true;
				} else {
					this->yspos = 0;
				}
				break;

			case VK_END:

				if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG || this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
					b_abort = true;
				} else {
					this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
				}

				break;

			case VK_UP:

				if (this->yspos == 0) {
					b_abort = true;
				} else if (this->yspos <= ROW_HEIGHT) {
					this->yspos = 0;
				} else {
					this->yspos -= ROW_HEIGHT;
					this->LastKey = 1;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_DOWN:

				if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG || this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
					b_abort = true;
				} else if (this->yspos >= (this->nRows-1)*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
					this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
				} else {
					this->yspos += ROW_HEIGHT;
					this->LastKey = 2;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_LEFT:

				if (this->xspos <= ROW_HEIGHT) {
					if (this->xspos == 0) {
						b_abort = true;
					} else {
						this->xspos = 0;
					}
				} else {
					this->xspos -= ROW_HEIGHT;
					this->LastKey = 5;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_RIGHT: {

				int32_t w1 = this->getTotalColumnsWidth();

				if (w1 <= rect.right || this->xspos == w1 - rect.right) {
					b_abort = true;
				} else if (this->xspos >= w1 - rect.right-ROW_HEIGHT) {
					this->xspos = w1 - rect.right;
				} else {
					this->xspos += ROW_HEIGHT;
					this->LastKey = 6;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;
			}

			case VK_PRIOR:

				if (this->yspos <= rect.bottom-STRLIST_TOP_MRG) {
					if (this->yspos == 0) {
						b_abort = true;
					} else {
						this->yspos = 0;
					}
				} else {
					this->yspos -= rect.bottom-STRLIST_TOP_MRG;
					this->LastKey = 3;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_NEXT:

				if (this->nRows*ROW_HEIGHT <= rect.bottom-STRLIST_TOP_MRG || this->yspos == this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG)) {
					b_abort = true;
				} else if (this->yspos >= (this->nRows)*ROW_HEIGHT-2*(rect.bottom-STRLIST_TOP_MRG)) {
					this->yspos = this->nRows*ROW_HEIGHT-(rect.bottom-STRLIST_TOP_MRG);
				} else {
					this->yspos += rect.bottom-STRLIST_TOP_MRG;
					this->LastKey = 4;
					SetTimer(hwnd, 2, 500, 0);
				}
				break;

			case VK_DELETE:

				b_abort = true;
				SendMessage(GetParent(hwnd), WM_U_RET_SL, 1, (LPARAM) GetMenu(hwnd));

				break;

			case VK_F2:

				b_abort = true;
				SendMessage(GetParent(hwnd), WM_U_RET_SL, 2, (LPARAM) GetMenu(hwnd));

				break;

			} if (b_abort)
				break;

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->yspos;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);
			sinfo.nPos = this->xspos;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

			if (!this->timed) {
				SetTimer(hwnd, 1, 34, 0);
				this->timed = 1;
			}
			break;
		}
		case WM_SYSKEYDOWN: {

			if (lParam & (unsigned long)(1UL << 29)) {
				switch(wParam) {
					case VK_F2:
						SendMessage(GetParent(hwnd), WM_U_RET_SL, 3, (LPARAM) GetMenu(hwnd));

						break;
				}
			}

			break;
		}
		case WM_KEYUP: {

			switch(wParam) {
			case VK_UP:
				if (this->LastKey == 1) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_DOWN:
				if (this->LastKey == 2) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_PRIOR:
				if (this->LastKey == 3) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_NEXT:
				if (this->LastKey == 4) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_LEFT:
				if (this->LastKey == 5) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_RIGHT:
				if (this->LastKey == 6) {
					KillTimer(hwnd, 2);
				}

				break;
			}

			break;
		}
		case WM_KILLFOCUS: {
			KillTimer(hwnd, 2);

			break;
		}
		case WM_ERASEBKGND: {
			return 1;
		}
		case WM_NCDESTROY: {
errorf("destroyed strlist");
			return 0;
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ ThumbListClass
//public:
// static
const WindowHelper ThumbListClass::helper = WindowHelper(std::wstring(L"ThumbListClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND ThumbListClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new ThumbListClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK ThumbListClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int64_t i;
	int j, k, l, pos;
	uint8_t *buf;
	wchar_t *wbuf;
	SCROLLINFO sinfo = {0};
	//THMBLISTV *lv = 0;
	BITMAPINFO bminfo = {0};

	HDC hdc, hdc2, hdc3;
	PAINTSTRUCT ps;
	HFONT hOldFont;
	HPEN hOldPen;
	HBRUSH hOldBrush;
	HBITMAP hOldBM, hBitmap;

	oneslnk **link;

	int len, diff, rowlen, lastrow, firstrow, sparep;
	char sel;

	switch(msg) {
		case WM_CREATE: {

			this->yspos = 0;
			this->hvrd = 0;
			this->timed = 0;

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			rowlen = (rect.right-THMBLIST_LEFT_MRG+DEF_THUMBGAPX) / (DEF_THUMBFW+DEF_THUMBGAPX);
			if (rowlen < 1)
				rowlen = 1;
			if (rowlen >= this->nThumbs)
				lastrow = 1;
			else
				lastrow = this->nThumbs/rowlen + !!(this->nThumbs%rowlen);

			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_ALL;
			sinfo.nMin = 0;
			sinfo.nMax = THMBLIST_TOP_MRG+lastrow*(DEF_THUMBFH+DEF_THUMBGAPY)-DEF_THUMBGAPY-1;
			sinfo.nPage = rect.bottom;
			sinfo.nPos = this->yspos;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!(ShowScrollBar(hwnd, SB_VERT, 1))) {
				errorf("ShowScrollBar failed.");
			}

			break;
		}
		case WM_PAINT: //{

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			rowlen = (rect.right-THMBLIST_LEFT_MRG+DEF_THUMBGAPX) / (DEF_THUMBFW+DEF_THUMBGAPX);
			if (rowlen < 1)
				rowlen = 1;
			else if (rowlen > this->nThumbs)
				rowlen = this->nThumbs;
			if (rowlen > 1) {
				sparep = (((rect.right-THMBLIST_LEFT_MRG+DEF_THUMBGAPX) - rowlen*(DEF_THUMBFW+DEF_THUMBGAPX)) / (rowlen));
			} else
				sparep = 0;

			if (sparep > (DEF_THUMBFW+DEF_THUMBGAPX-1)/2) { // capped at the maximum the gap could get with multiple rows
				sparep = (DEF_THUMBFW+DEF_THUMBGAPX-1)/2;
			}

			firstrow = (this->yspos-THMBLIST_TOP_MRG)/(DEF_THUMBFH+DEF_THUMBGAPY);
			if (this->yspos < THMBLIST_TOP_MRG)
				firstrow = -1;

			diff = (this->yspos-THMBLIST_TOP_MRG)-firstrow*(DEF_THUMBFH+DEF_THUMBGAPY);		// diff is the amount of pixels outside the area

			bminfo.bmiHeader.biSize = sizeof(bminfo.bmiHeader);
			bminfo.bmiHeader.biPlanes = 1;
			bminfo.bmiHeader.biBitCount = 32;
			bminfo.bmiHeader.biCompression = BI_RGB;

			hdc = BeginPaint(hwnd, &ps);

			hdc3 = CreateCompatibleDC(hdc);
			hOldPen = (HPEN) GetCurrentObject(hdc3, OBJ_PEN);
			hOldBrush = (HBRUSH) GetCurrentObject(hdc3, OBJ_BRUSH);
//			hOldFont = (HFONT) SelectObject(hdc2, hFont);

			if (!hThumbListBM) {
				i = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				if (i == 0) {
					i = 1920;
				} j = GetSystemMetrics(SM_CYVIRTUALSCREEN);
				if (j == 0) {
					j = 1920;
				}
				hThumbListBM = CreateCompatibleBitmap(hdc, i, j);
			}
			hOldBM = (HBITMAP) SelectObject(hdc3, hThumbListBM);

			SelectObject(hdc3, bgBrush1);
			SelectObject(hdc3, bgPen1);
			Rectangle(hdc3, 0, 0, rect.right, rect.bottom);
			SelectObject(hdc3, bgPen2);
			SelectObject(hdc3, bgBrush2);

			for (i = 0, lastrow = (!!diff)+(rect.bottom-(DEF_THUMBFH+DEF_THUMBGAPY-diff)%(DEF_THUMBFH+DEF_THUMBGAPY))/(DEF_THUMBFH+DEF_THUMBGAPY)+(!!((rect.bottom-(DEF_THUMBFH+DEF_THUMBGAPY-diff))%(DEF_THUMBFH+DEF_THUMBGAPY))), sel = 0; i < lastrow; i++, pos++, sel = 0) {
				if (firstrow+i < 0)
					continue;
				for (j = 0; j < rowlen; j++) {
					if ((k = (firstrow+i)*rowlen+j) >= this->nThumbs)
						break;
					SelectObject(hdc3, bgPen2);
					SelectObject(hdc3, bgBrush2);
					if (this->ThumbSel[k/8] & (1 << k%8)) {
						SelectObject(hdc3, selPen);
						SelectObject(hdc3, selBrush);
					} else if (k+1 == this->hvrd) {
						SelectObject(hdc3, selPen2);
						SelectObject(hdc3, selBrush);
					}
					Rectangle(hdc3, THMBLIST_LEFT_MRG+j*(DEF_THUMBFW+DEF_THUMBGAPX+sparep), i*(DEF_THUMBFH+DEF_THUMBGAPY)-diff, THMBLIST_LEFT_MRG+j*(DEF_THUMBFW+DEF_THUMBGAPX+sparep)+DEF_THUMBFW, i*(DEF_THUMBFH+DEF_THUMBGAPY)-diff+DEF_THUMBFH);

					if (this->thumb[k]) {
						bminfo.bmiHeader.biWidth = this->thumb[k]->x;
						bminfo.bmiHeader.biHeight = -this->thumb[k]->y;

						if ((SetDIBitsToDevice(hdc3, THMBLIST_LEFT_MRG+j*(DEF_THUMBFW+DEF_THUMBGAPX+sparep)+(DEF_THUMBFW-this->thumb[k]->x)/2, i*(DEF_THUMBFH+DEF_THUMBGAPY)-diff+(DEF_THUMBFH - this->thumb[k]->y)/2, this->thumb[k]->x, this->thumb[k]->y,
						0, 0, 0, this->thumb[k]->y, this->thumb[k]->img[0], &bminfo, DIB_RGB_COLORS)) == 0) {
							errorf_old("SetDIBitsToDevice failed, h: %d, w: %d, source: %llu", this->thumb[k]->y, this->thumb[k]->x, this->thumb[k]->img[0]);
						}
					}
				}
			}
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdc3, 0, 0, SRCCOPY);

			DeleteDC(hdc3);

			SelectObject(hdc3, hOldPen);
			SelectObject(hdc3, hOldBrush);
			SelectObject(hdc3, hOldBM);
//			SelectObject(hdc2, hOldFont);
			EndPaint(hwnd, &ps);

			break;
		//}
		case WM_USER: //{	// update scroll info
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			rowlen = (rect.right-THMBLIST_LEFT_MRG+DEF_THUMBGAPX) / (DEF_THUMBFW+DEF_THUMBGAPX);
			if (rowlen < 1)
				rowlen = 1;
			if (rowlen > this->nThumbs)
				lastrow = 1;
			else
				lastrow = this->nThumbs/rowlen + !!(this->nThumbs%rowlen);

			switch (wParam) {
			case 1:
				sinfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.nMin = 0;
				sinfo.nPos = this->yspos = 0;
				sinfo.nMax = THMBLIST_TOP_MRG+lastrow*(DEF_THUMBFH+DEF_THUMBGAPY)-DEF_THUMBGAPY-1;
				sinfo.nPage = rect.bottom;

				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				break;

			case 2:
				sinfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
				sinfo.cbSize = sizeof(SCROLLINFO);
				if (this->nThumbs == 0)
					sinfo.nMax = 0;
				else
					sinfo.nMax = THMBLIST_TOP_MRG+lastrow*(DEF_THUMBFH+DEF_THUMBGAPY)-DEF_THUMBGAPY-1;
				sinfo.nPos = this->yspos = 0;
				sinfo.nPage = rect.bottom;	// don't know why setting page is necessary
				if ((sinfo.nMax+1 <= rect.bottom) || (rect.bottom <= 0)) {
					ShowScrollBar(hwnd, SB_VERT, 0);
				} else {
					ShowScrollBar(hwnd, SB_VERT, 1);
				}

				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}

				break;
			}
			break;
		//}
		case WM_SIZE: //{ ROW_HEIGHT

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			rowlen = (rect.right-THMBLIST_LEFT_MRG+DEF_THUMBGAPX) / (DEF_THUMBFW+DEF_THUMBGAPX);
			if (rowlen < 1)
				rowlen = 1;
			if (rowlen > this->nThumbs)
				lastrow = 1;
			else
				lastrow = this->nThumbs/rowlen + !!(this->nThumbs%rowlen);

			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_ALL;
			sinfo.nMin = 0;
			sinfo.nMax = THMBLIST_TOP_MRG+lastrow*(DEF_THUMBFH+DEF_THUMBGAPY)-DEF_THUMBGAPY-1;
			sinfo.nPage = rect.bottom;
			if (sinfo.nMax <= rect.bottom || rect.bottom <= 0) {
				this->yspos = 0;
				ShowScrollBar(hwnd, SB_VERT, 0);
			} else {
				ShowScrollBar(hwnd, SB_VERT, 1);
				if (this->yspos > sinfo.nMax)
					this->yspos = sinfo.nMax;
			}
			sinfo.nPos = this->yspos;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->timed) {		// if the previous timer is a '1' timer, the top text windows will not change
				SetTimer(hwnd, 0, 16, 0);
				this->timed = 1;
			}

			break;
		//}
		case WM_VSCROLL: {
			bool b_abort = false;

			switch(LOWORD(wParam)) {

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					this->yspos = HIWORD(wParam);

					break;

				case SB_LINEUP:

					if (this->yspos <= ROW_HEIGHT) {
						if (this->yspos == 0) {
							b_abort = true;
						} else {
							this->yspos = 0;
						}
					} else
						this->yspos -= ROW_HEIGHT;

					break;

				case SB_LINEDOWN:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);
					if (sinfo.nMax <= rect.bottom || this->yspos == sinfo.nMax+1-rect.bottom) {
						b_abort = true;
					} else if (this->yspos >= sinfo.nMax+1-ROW_HEIGHT-rect.bottom) {	// if nThumbs is changed to an unsigned value all line downs and page downs will break (could be fixed with a cast or moving the removed row or page to the left side of the comparison)
						this->yspos = sinfo.nMax+1-rect.bottom;
					} else {
						this->yspos += ROW_HEIGHT;
					}
					break;

				case SB_PAGEUP:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->yspos <= rect.bottom) {
						if (this->yspos == 0) {
							b_abort = true;
						} else {
							this->yspos = 0;
						}
					} else this->yspos -= rect.bottom;

					break;

				case SB_PAGEDOWN:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);

					if (sinfo.nMax <= rect.bottom || this->yspos == sinfo.nMax+1-rect.bottom) {
						b_abort = true;
					} else if (this->yspos >= sinfo.nMax+1-2*rect.bottom) {
						this->yspos = sinfo.nMax+1-rect.bottom;
					} else
						this->yspos += rect.bottom;

					break;

				case SB_TOP:

					if (this->yspos == 0) {
						b_abort = true;
					} else
						this->yspos = 0;

					break;

				case SB_BOTTOM:

					if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);
					if (sinfo.nMax <= rect.bottom || this->yspos == sinfo.nMax+1-rect.bottom) {
						b_abort = true;
					} else
						this->yspos = sinfo.nMax+1-(rect.bottom);

					break;

				default:
					b_abort = true;
					break;
			} if (b_abort)
				break;

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->yspos;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->timed) {
				SetTimer(hwnd, 1, 34, 0);
				this->timed = 1;
			}

			break;
		}
		case WM_MOUSEWHEEL: //{

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			sinfo.fMask = SIF_RANGE;
			GetScrollInfo(hwnd, SB_VERT, &sinfo);

			if (sinfo.nMax+1 <= rect.bottom)
				break;

			sinfo.fMask = SIF_POS;

			if ((HIWORD(wParam))/ROW_HEIGHT/2*ROW_HEIGHT == 0)
				break;

			if (this->yspos <= (int16_t)(HIWORD(wParam))/ROW_HEIGHT/2*ROW_HEIGHT) {
				if (this->yspos == 0)
					break;
				this->yspos = 0;
			} else if (this->yspos >= (sinfo.nMax+1)-rect.bottom+((int16_t)(HIWORD(wParam))/ROW_HEIGHT/2*ROW_HEIGHT)) {	// wParam is negative if scrolling down
				if (this->yspos == sinfo.nMax+1-rect.bottom)
					break;
				this->yspos = sinfo.nMax+1-rect.bottom;
			} else this->yspos -= (int16_t)(HIWORD(wParam))/ROW_HEIGHT/2*ROW_HEIGHT;
			sinfo.nPos = this->yspos;

			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->timed)  {
				SetTimer(hwnd, 1, 34, 0);
				this->timed = 1;
			}

			break;
		//}
		case WM_MOUSEMOVE: //{
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			l = this->hvrd;
			rowlen = (rect.right-THMBLIST_LEFT_MRG+DEF_THUMBGAPX) / (DEF_THUMBFW+DEF_THUMBGAPX);
			if (rowlen < 1)
				rowlen = 1;
			else if (rowlen > this->nThumbs)
				rowlen = this->nThumbs;
			if (rowlen > 1) {
				sparep = (((rect.right-THMBLIST_LEFT_MRG+DEF_THUMBGAPX) - rowlen*(DEF_THUMBFW+DEF_THUMBGAPX)) / (rowlen));
			} else
				sparep = 0;

			if (sparep > (DEF_THUMBFW+DEF_THUMBGAPX-1)/2) { // capped at the maximum the gap could get with multiple rows
				sparep = (DEF_THUMBFW+DEF_THUMBGAPX-1)/2;
			}

			if ((j = (int16_t)(lParam & (256*256-1))) < THMBLIST_LEFT_MRG || (k = (int16_t)((lParam/256/256) & (256*256-1))+this->yspos-THMBLIST_TOP_MRG) < 0 || (j - THMBLIST_LEFT_MRG) % (DEF_THUMBFW+DEF_THUMBGAPX+sparep) >= DEF_THUMBFW || (k+this->yspos) % (DEF_THUMBFH+DEF_THUMBGAPY) >= DEF_THUMBFH) {
				this->hvrd = 0;
				SetCursor(hDefCrs);
				if (l > 0) {
					if (!(InvalidateRect(hwnd, 0, 0))) {
						errorf("InvalidateRect failed");
					}
				} break;
			}

			this->hvrd = (k/(DEF_THUMBFH+DEF_THUMBGAPY))*rowlen+(j - THMBLIST_LEFT_MRG)/(DEF_THUMBFW+DEF_THUMBGAPX+sparep)+1;

			if (this->hvrd > this->nThumbs) {
				this->hvrd = 0;
				SetCursor(hDefCrs);
				if (l > 0) {
					if (!(InvalidateRect(hwnd, 0, 0))) {
						errorf("InvalidateRect failed");
					}
				} break;
			}
			SetCursor(hDefCrs);
			if (l != this->hvrd) {
				if (!(InvalidateRect(hwnd, 0, 0))) {
					errorf("InvalidateRect failed");
				}
			}

			break;
		//}
		case WM_LBUTTONDOWN: //{

			if (!(wParam & MK_CONTROL)) {
				for (i = this->nThumbs/8+!!(this->nThumbs%8)-1; i >= 0; this->ThumbSel[i] = 0, i--);
			}

			if (this->hvrd > 0 && this->hvrd <= this->nThumbs) {
				pos = this->hvrd-1;
				if (!(wParam & MK_SHIFT)) {
					this->ThumbSel[pos/8] ^= (1 << pos%8);
					this->lastSel = pos;
				} else {
					if ((i = this->lastSel) <= pos)
						for (; i <= pos; i++)
							this->ThumbSel[i/8] |= (1 << i%8);
					else
						for (; i >= pos; i--)
							this->ThumbSel[i/8] |= (1 << i%8);
				}
			}
			if (!(InvalidateRect(hwnd, 0, 1))) {
				errorf("InvalidateRect failed");
			}
			SetFocus(hwnd);

			break;
		//}
		case WM_LBUTTONUP: //{



			break;
		//}
		case WM_RBUTTONDOWN: //{


			this->point.x = rect.left = lParam & (256*256-1);
			this->point.y = rect.top = lParam/256/256 & (256*256-1);
			if (rect.top < 0)
				break;
			MapWindowPoints(hwnd, 0, &this->point, 1);

			if (!(wParam & MK_CONTROL)) {
				if (this->hvrd > this->nThumbs || this->hvrd < 1)
					break;
				pos = this->hvrd-1;
				if (!(this->ThumbSel[pos/8] & (1 << pos%8))) {
					for (i = this->nThumbs/8+!!(this->nThumbs%8)-1; i >= 0; this->ThumbSel[i] = 0, i--);
					this->ThumbSel[pos/8] ^= (1 << pos%8);
					this->lastSel = pos;
				}
				if (!(InvalidateRect(hwnd, 0, 1))) {
					errorf("InvalidateRect failed");
				}
			}
			SendMessage(GetParent(hwnd), WM_U_RET_TL, (WPARAM) GetMenu(hwnd), 4);

			break;
		//}
		case WM_TIMER: {
			bool b_abort = false;

			switch(wParam) {

			case 0:
			case 1:

				KillTimer(hwnd, 0);
				this->timed = 0;

				if (!(InvalidateRect(hwnd, 0, 0))) {
					errorf("InvalidateRect failed");
				}
				break;

			case 2:

				KillTimer(hwnd, 2);
				SetTimer(hwnd, 2, 67, 0);

				switch(this->LastKey) {
				case 1:
					if (this->yspos == 0) {
						b_abort = true;
					} else if (this->yspos <= ROW_HEIGHT) {
						this->yspos = 0;
					} else
						this->yspos -= ROW_HEIGHT;

					break;

				case 2:
					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);
					if (sinfo.nMax <= rect.bottom || this->yspos == sinfo.nMax+1-rect.bottom) {
						b_abort = true;
					}
					else if (this->yspos >= sinfo.nMax+1-ROW_HEIGHT-rect.bottom) {
						this->yspos = sinfo.nMax+1-rect.bottom;
					} else
						this->yspos += ROW_HEIGHT;

					break;

				case 3:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->yspos == 0) {
						b_abort = true;
					} else if (this->yspos <= rect.bottom) {
						this->yspos = 0;
					} else
						this->yspos -= rect.bottom;

					break;

				case 4:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);

					if (sinfo.nMax <= rect.bottom || this->yspos == sinfo.nMax+1-rect.bottom) {
						b_abort = true;
					} else if (this->yspos >= sinfo.nMax+1-2*rect.bottom) {
						this->yspos = sinfo.nMax+1-rect.bottom;
					} else
						this->yspos += rect.bottom;

					break;

				case 5:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					break;

				case 6:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					break;

				default:
					KillTimer(hwnd, 2);
					b_abort = true;
					break;
				} if (b_abort)
					break;

				sinfo.fMask = SIF_POS;
				sinfo.nPos = this->yspos;
				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				if (!this->timed) {
					SetTimer(hwnd, 1, 34, 0);
					this->timed = 1;
				}

				break;
			}
			break;
		}
		case WM_KEYDOWN: {
			bool b_abort = false;

			if ((lParam & 0x40000000) != 0)
				break;

			KillTimer(hwnd, 2);

			switch(wParam) {

			case VK_HOME:
				if (this->yspos == 0) {
					b_abort = true;
				} else {
					this->yspos = 0;
				}
				break;

			case VK_END:
				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}
				sinfo.fMask = SIF_RANGE;
				GetScrollInfo(hwnd, SB_VERT, &sinfo);
				if (sinfo.nMax <= rect.bottom || this->yspos == sinfo.nMax+1-rect.bottom) {
					b_abort = true;
				} else
					this->yspos = sinfo.nMax+1-(rect.bottom);

				break;

			case VK_UP:

				if (this->yspos == 0) {
					b_abort = true;
				} else if (this->yspos <= ROW_HEIGHT) {
					this->yspos = 0;
				} else {
					this->yspos -= ROW_HEIGHT;
					this->LastKey = 1;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_DOWN:

				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}
				sinfo.fMask = SIF_RANGE;
				GetScrollInfo(hwnd, SB_VERT, &sinfo);

				if (sinfo.nMax <= rect.bottom || this->yspos == sinfo.nMax+1-rect.bottom) {
					b_abort = true;
				} else if (this->yspos >= sinfo.nMax+1-ROW_HEIGHT-rect.bottom) {
					this->yspos = sinfo.nMax+1-rect.bottom;
				} else {
					this->yspos += ROW_HEIGHT;
					this->LastKey = 2;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_LEFT:

				if (1) {
				} else {
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_RIGHT:

				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}

				if (1) {
					b_abort = true;
				} else if (0) {
				} else {
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_PRIOR:

				if (GetClientRect(hwnd, &rect) == 0) {		// forgot this part
					errorf("GetClientRect failed");
				}

				if (this->yspos <= rect.bottom) {
					if (this->yspos == 0) {
						b_abort = true;
					} else {
						this->yspos = 0;
					}
				} else {
					this->yspos -= rect.bottom;
					this->LastKey = 3;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_NEXT:

				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}
				sinfo.fMask = SIF_RANGE;
				GetScrollInfo(hwnd, SB_VERT, &sinfo);

				if (sinfo.nMax <= rect.bottom || this->yspos == sinfo.nMax+1-rect.bottom) {
					b_abort = true;
				} else if (this->yspos >= sinfo.nMax+1-2*rect.bottom) {
					this->yspos = sinfo.nMax+1-rect.bottom;
				} else {
					this->yspos += rect.bottom;
					this->LastKey = 4;
					SetTimer(hwnd, 2, 500, 0);
				}
				break;

			case VK_DELETE:

				b_abort = true;
				SendMessage(GetParent(hwnd), WM_U_RET_TL, (WPARAM) GetMenu(hwnd), 1);

				break;

			case VK_F2:

				b_abort = true;
				SendMessage(GetParent(hwnd), WM_U_RET_TL, (WPARAM) GetMenu(hwnd), 2);

				break;
			case VK_BACK:
				PostMessage(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);

				break;
			} if (b_abort)
				break;

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->yspos;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->timed) {
				SetTimer(hwnd, 1, 34, 0);
				this->timed = 1;
			}
			break;
		}
		case WM_SYSKEYDOWN: //{

			if (lParam & (1UL << 29)) {
				switch(wParam) {
					case VK_F2:
						SendMessage(GetParent(hwnd), WM_U_RET_TL, (WPARAM) GetMenu(hwnd), 3);

						break;
				}
			}

			break;
		//}
		case WM_KEYUP: //{

			switch(wParam) {
			case VK_UP:
				if (this->LastKey == 1) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_DOWN:
				if (this->LastKey == 2) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_PRIOR:
				if (this->LastKey == 3) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_NEXT:
				if (this->LastKey == 4) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_LEFT:
				if (this->LastKey == 5) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_RIGHT:
				if (this->LastKey == 6) {
					KillTimer(hwnd, 2);
				}

				break;
			}

			break;
		//}
		case WM_KILLFOCUS: //{
			KillTimer(hwnd, 2);

			break;
		//}
		case WM_ERASEBKGND: //{
			return 1;
		//}
		case WM_DESTROY: //{
			return 0;
		//}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ ViewImageClass
//public:
// static
const WindowHelper ViewImageClass::helper = WindowHelper(std::wstring(L"ViewImageClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = NULL;
	}
);

// static
HWND ViewImageClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new ViewImageClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK ViewImageClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int64_t i;
	int j, k, l, m;
	IMGVIEWV *ev = 0;
	BITMAPINFO bminfo = {0};

	HDC hdc, hdc2;
	PAINTSTRUCT ps;
	HFONT hOldFont;
	HPEN hOldPen;
	HBRUSH hOldBrush;
	HBITMAP hOldBM, hBitmap;

	int len;
	char sel, *buf;

	HDC thdc, hdcMem;
	HBITMAP oldBitmap;

	static HBITMAP hThumbListBM;

	double ratio, midpos;

	switch(msg) {
		case WM_CREATE: //{
			if (!(ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0))) { // get mem from parent
				DestroyWindow(GetParent(hwnd));
				errorf("No external variables returned");
				return 0;
			}

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (!ev->imgpath) {
				errorf("no imgpath");
				DestroyWindow(GetParent(hwnd));
				return 0;
			}
			if (!(ev->FullImage = ReadImage(ev->imgpath))) {	//! will leak if an image already existed
				errorf_old("couldn't load image, %s");
				DestroyWindow(GetParent(hwnd));
				return 0;
			}
			ev->fit = BIT_VI_FIT_MODE_ENABLED;
			ev->DispImage = NULL;
			ev->zoomp = 0;

			if (!BIT_VI_FIT_MODE_ENABLED) {
				ev->midX = (long double)ev->FullImage->x / 2;
				ev->midY = (long double)ev->FullImage->y / 2;
			}

			ev->drag = 0;
			ev->timed = 0;

			ev->dispframe = 0;

			if (ev->FullImage->n > 1) {
				SetTimer(hwnd, 2, ev->FullImage->dur[ev->dispframe]*10, 0);
			}

			//! later maybe add the option to stretch even if it fits
			//! the following is also served by WM_USER
			if (!(ev->fit & BIT_VI_FIT_DISP_IS_ORIG)) {
				if (ev->DispImage) {
					DestroyImgF(ev->DispImage);
				}
			} else {
				ev->fit &= ~BIT_VI_FIT_DISP_IS_ORIG;
			}

			if (ev->FullImage->x > rect.right || ev->FullImage->y > rect.bottom) {
				ev->DispImage = FitImage(ev->FullImage, rect.right, rect.bottom, 0, 0);
			} else {
				ev->fit |= BIT_VI_FIT_DISP_IS_ORIG;
				ev->DispImage = ev->FullImage;
			}
			ev->xpos = ((int64_t)ev->DispImage->x - rect.right)/2;
			ev->ypos = ((int64_t)ev->DispImage->y - rect.bottom)/2;
			if (ev->DispImage == NULL) {
				errorf("no DispImage");
				DestroyWindow(GetParent(hwnd));
				return 0;
			}

//errorf_old("1: ev->midX: %Lf, ev->midY: %Lf, x: %lld, y: %lld", ev->midX, ev->midY, ev->FullImage->x, ev->FullImage->y);

			break;
		//}
		case WM_PAINT: //{
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == NULL) {
				errorf("ev is null");
				break;
			}

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			bminfo.bmiHeader.biSize = sizeof(bminfo.bmiHeader);
			bminfo.bmiHeader.biPlanes = 1;
			bminfo.bmiHeader.biBitCount = 32;
			bminfo.bmiHeader.biCompression = BI_RGB;

			hdc = BeginPaint(hwnd, &ps);

			hdc2 = CreateCompatibleDC(hdc);
			hOldPen = (HPEN) GetCurrentObject(hdc2, OBJ_PEN);
			hOldBrush = (HBRUSH) GetCurrentObject(hdc2, OBJ_BRUSH);
			hOldFont = (HFONT) SelectObject(hdc2, hFont);

			SetBkMode(hdc2, TRANSPARENT);

			if (!hThumbListBM) {
				i = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				if (i == 0) {
					i = 1920;
				} j = GetSystemMetrics(SM_CYVIRTUALSCREEN);
				if (j == 0) {
					j = 1920;
				}
				hThumbListBM = CreateCompatibleBitmap(hdc, i, j);
			}
			hOldBM = (HBITMAP) SelectObject(hdc2, hThumbListBM);

			SelectObject(hdc2, bgBrush1);
			SelectObject(hdc2, bgPen1);
			Rectangle(hdc2, 0, 0, rect.right, rect.bottom);

// errorf_old("ev->xpos: %d, ev->ypos: %d", ev->xpos, ev->ypos);
			if (ev->DispImage) {
				bminfo.bmiHeader.biWidth = ev->DispImage->x;
				bminfo.bmiHeader.biHeight = -ev->DispImage->y;
				if (ev->xpos < 0) {
					i = -ev->xpos;
				} else {
					i = 0;
				} if (ev->ypos < 0) {
					j = -ev->ypos;
				} else {
					j = 0;
				}
				if ((SetDIBitsToDevice(hdc2, i, j, ev->DispImage->x, ev->DispImage->y, 0, 0, 0, ev->DispImage->y, ev->DispImage->img[ev->dispframe], &bminfo, DIB_RGB_COLORS)) == 0) {
					errorf_old("SetDIBitsToDevice failed, h: %d, w: %d, source: %llu", ev->DispImage->y, ev->DispImage->x, ev->DispImage->img[ev->dispframe]);
				}
			}
			if (1 || ev->zoomp) { //! remove the "1 || " later
				SelectObject(hdc2, hPen1);
				Rectangle(hdc2, 5, 5, 5+6+5*CHAR_WIDTH, 5+ROW_HEIGHT);
				buf = utoc(ev->zoomp);
				TextOutA(hdc2, 5+3, 5+(ROW_HEIGHT)-(ROW_HEIGHT/2)-7, buf, strlen(buf));
				if (buf) {
					free(buf);
				}
			}

			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdc2, 0, 0, SRCCOPY);

			SelectObject(hdc2, hOldPen);
			SelectObject(hdc2, hOldBrush);
			SelectObject(hdc2, hOldBM);
			SelectObject(hdc2, hOldFont);

			DeleteDC(hdc2);

			EndPaint(hwnd, &ps);

			break;
		//}
		case WM_SIZE: //{
/*			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == NULL) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
*/
			PostMessage(hwnd, WM_USER, 0, 0);	// so no black border for expanded area
			break;
		//}
		case WM_USER: //{	// refresh image to window size
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == NULL) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (ev->fit & BIT_VI_FIT_MODE_ENABLED) {

				if (!(ev->fit & BIT_VI_FIT_DISP_IS_ORIG)) {
					if (ev->DispImage) {
						DestroyImgF(ev->DispImage);
					}
				} else {
					ev->fit &= ~BIT_VI_FIT_DISP_IS_ORIG;
				}

				if (ev->FullImage->x > rect.right || ev->FullImage->y > rect.bottom) {
					ev->DispImage = FitImage(ev->FullImage, rect.right, rect.bottom, 0, 0);
				} else {
					ev->fit |= BIT_VI_FIT_DISP_IS_ORIG;
					ev->DispImage = ev->FullImage;
				}
				ev->xpos = ((int64_t)ev->DispImage->x - rect.right)/2;
				ev->ypos = ((int64_t)ev->DispImage->y - rect.bottom)/2;
				if (ev->DispImage == NULL) {
					errorf("no DispImage");
					DestroyWindow(GetParent(hwnd));
					return 0;
				}

			} else {
				if (ev->fit & (BIT_VI_FIT_RESIZE_FITS_X | BIT_VI_FIT_RESIZE_FITS_Y) && ev->xdisp < rect.right && ev->ydisp < rect.bottom) {	// if zoomed image fits
					ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
					ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
				} else {
					if (ev->xdisp <= rect.right) {
						l = ev->xdisp;		// width of the retrieved image is the same as the whole image zoomed
						ev->xpos = 0;
						ev->midX = (long double)ev->FullImage->x / 2;
					} else {
						l = rect.right;		// width is the width of window

						if (!(ev->fit & BIT_VI_FIT_RESIZE_FITS_X)) {
							i = ((int64_t)(rect.right - rect.right % 2) - (ev->DispImage->x - ev->DispImage->x % 2))/2; // difference between dispimage size and new dispimage size both rounded down to nearest multiple of 2 -- that way image position is adjusted by pixel every other pixel
							ev->xpos -= i;	// image is revealed and hidden from both directions equally -- xpos moves the image in the opposite direction
						} else {
							midpos = (double)ev->xdisp/2;
							ev->xpos = midpos - (double)rect.right/2;	// if xpos were fractional midpos would always be in the middle -- in this case the actual middle point is at most 0,5 pixels off (always more to the right)
						}
						ev->xpos = ev->midX - (double)rect.right/2 + 0.5;
						i = 0;
						if (ev->xpos+rect.right > ev->xdisp) {
							ev->xpos = ev->xdisp-rect.right;
							i = 1;
						}
						if (ev->xpos < 0) {
							ev->xpos = 0;
							i = 1;
						}
						if (i) {
							ev->midX = (long double)(ev->xpos + (rect.right)/2.0)/(ev->zoomp/100.0);
						}
					} if (ev->ydisp <= rect.bottom) {
						m = ev->ydisp;		// width of the retrieved image is the same as the whole image
						ev->ypos = 0;
						ev->midY = (long double)ev->FullImage->y / 2;
					} else {
						m = rect.bottom;		// height is the height of window

						if (!(ev->fit & BIT_VI_FIT_RESIZE_FITS_Y)) {
							i = ((int64_t)(rect.bottom - rect.bottom % 2) - (ev->DispImage->y - ev->DispImage->y % 2))/2;
							ev->ypos -= i;
						} else {
							midpos = (double)ev->ydisp/2;
							ev->ypos = midpos - (double)rect.bottom/2;
						}
						ev->ypos = ev->midY - (double)rect.bottom/2 + 0.5;
						i = 0;
						if (ev->ypos+rect.bottom > ev->ydisp) {
							ev->ypos = ev->ydisp-rect.bottom;
							i = 1;
						}
						if (ev->ypos < 0) {
							ev->ypos = 0;
							i = 1;
						}
						if (i) {
							ev->midY = (long double)(ev->ypos + (rect.bottom)/2.0)/(ev->zoomp/100.0);
						}
					}
					errorf("calling resizeimagechunk 1");
					ev->DispImage = ResizeImageChunk(ev->FullImage, (double) ev->zoomp / 100, l, m, ev->xpos, ev->ypos);

					ev->fit &= ~(BIT_VI_FIT_RESIZE_FITS_X | BIT_VI_FIT_RESIZE_FITS_Y);
					if (ev->DispImage) {
						if (ev->xdisp <= rect.right) {
							ev->fit |= BIT_VI_FIT_RESIZE_FITS_X;
							ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
						}
						if (ev->ydisp <= rect.bottom) {
							ev->fit |= BIT_VI_FIT_RESIZE_FITS_Y;
							ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
						}
					}
				}
			}
			InvalidateRect(hwnd, 0, 0);

			break;
		//}
		case WM_MOUSEWHEEL: //{		//! have zooming refresh dragging
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == NULL) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if ((i = (int16_t)(HIWORD(wParam))/WHEEL_DELTA) == 0) {
				break;
			}
			if (!(ev->fit & BIT_VI_FIT_MODE_ENABLED) && ev->DispImage != NULL) {	// to calculate middle position -- not needed if whole image in screen
				if (ev->xdisp > rect.right)
					j = ev->xdisp;	// the virtual size of the whole zoomed image
				else
					j = 0;
				if (ev->ydisp > rect.bottom)
					k = ev->ydisp;
				else
					k = 0;
			} else {
				j = k = 0;
			}

			if (ev->fit & BIT_VI_FIT_DISP_IS_ORIG) {	// clean up and initializing zoomp if previously fit to screen
				ev->fit &= ~BIT_VI_FIT_DISP_IS_ORIG;
				ev->fit &= ~BIT_VI_FIT_MODE_ENABLED;
				ev->zoomp = 100;
			} else {
				if (ev->DispImage) {
					DestroyImgF(ev->DispImage);
					ev->DispImage = NULL;
				}
				if (ev->fit & BIT_VI_FIT_MODE_ENABLED) {
					double zoom1, zoom2;
					zoom1 = (double) rect.right / ev->FullImage->x;
					zoom2 = (double) rect.bottom / ev->FullImage->y;
					if (zoom1 > zoom2) {
						zoom1 = zoom2;
					}
					ev->zoomp = zoom1*100;
					ev->fit &= ~BIT_VI_FIT_MODE_ENABLED;
				}
			}

			while (i != 0) {
				if (i > 0) {
					if (ev->zoomp < 300) {
						ev->zoomp = ((ev->zoomp+10)/10)*10;
					} else if (ev->zoomp < 500) {
						ev->zoomp = ((ev->zoomp+20)/20)*20;
					} else if (ev->zoomp < 2000) {
						ev->zoomp = ((ev->zoomp+50)/50)*50;
					} else {
						ev->zoomp = 2000;
						break;
					}
					i--;
				} else {
					if (ev->zoomp <= 300) {
						ev->zoomp = ((ev->zoomp-1)/10)*10;
						if (ev->zoomp <= 0) {
							ev->zoomp = 5;
							break;
						}
					} else if (ev->zoomp <= 500) {
						ev->zoomp = ((ev->zoomp-1)/20)*20;
					} else {
						if (ev->zoomp > 2000) {
							ev->zoomp = 2001;
						}
						ev->zoomp = ((ev->zoomp-1)/50)*50;
					}
					i++;
				}
			}

			if (0 && ev->zoomp == 100) {	//! potential optimization when not resized at all
			} else {
				ev->xdisp = ev->FullImage->x*((double)ev->zoomp/100);	// ev->xdisp/(zoomp/100) is guaranteed to be at most ev->FullImage->x because of truncation
				ev->ydisp = ev->FullImage->y*((double)ev->zoomp/100);

				if (ev->FullImage->x > ev->FullImage->y && j) {		// to calculate previous middle position in new size
					ratio = (double) ev->xdisp / j;		// where j is the previous ev->xdisp or 0 if image was stretched to screen or fit to screen in x-axis
				} else if (k) {
					ratio = (double) ev->ydisp / k;
				} else {
					ratio = 0;
				}
				// ratio is the relation between old and new ydisp calculated by the larger x or y; or it is 0 if the image previously fit into the frame

				if (ev->xdisp <= rect.right) {
					l = ev->xdisp;		// width of the retrieved image is the same as the whole image
					ev->xpos = 0;		// the position in the image changed by dragging with mouse
				} else {	// calculate new position after zoom
					l = rect.right;		// width is the width of window

					if (ratio) {
						midpos = (ev->xpos+(double)rect.right/2)*ratio;		// no extra 0,5 needed -- if pos was 0 and window width was 1, midpos would correctly be 0,5 already
					} else {
						midpos = (double)ev->xdisp/2;
					}
					ev->xpos = (ev->midX * ev->zoomp)/100 - (double)rect.right/2 + 0.5;
					i = 0;
					if (ev->xpos+rect.right > ev->xdisp) {
						ev->xpos = ev->xdisp-rect.right;
						i = 1;
					}
					if (ev->xpos < 0) {
						ev->xpos = 0;
						i = 1;
					}
					if (i) {
						ev->midX = (long double)(ev->xpos + (rect.right)/2.0)/(ev->zoomp/100.0);
					}
				}

				if (ev->ydisp <= rect.bottom) {
					m = ev->ydisp;
					ev->ypos = 0;
				} else {
					m = rect.bottom;

					if (ratio) {
						midpos = (ev->ypos+(double)rect.bottom/2)*ratio;		// no extra 0,5 needed -- if pos was 0 and window width was 1, midpos would correctly be 0,5 already
					} else {
						midpos = (double)ev->ydisp/2;
					}
					ev->ypos = (ev->midY * ev->zoomp)/100 - (double)rect.bottom/2 + 0.5;
					i = 0;
					if (ev->ypos+rect.bottom > ev->ydisp) {
						ev->ypos = ev->ydisp-rect.bottom;
						i = 1;
					}
					if (ev->ypos < 0) {
						ev->ypos = 0;
						i = 1;
					}
					if (i) {
						ev->midY = (long double)(ev->ypos + (rect.bottom)/2.0)/(ev->zoomp/100.0);
					}
				}
				// l and m are the x and y of the retrieved image chunk

				errorf("calling resizeimagechunk 2");
				ev->DispImage = ResizeImageChunk(ev->FullImage, (double) ev->zoomp / 100, l, m, ev->xpos, ev->ypos);
			}
			ev->fit &= ~(BIT_VI_FIT_RESIZE_FITS_X | BIT_VI_FIT_RESIZE_FITS_Y);

			if (ev->DispImage) {
				if (ev->xdisp <= rect.right) {
					ev->fit |= BIT_VI_FIT_RESIZE_FITS_X;
					ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
				}
				if (ev->ydisp <= rect.bottom) {
					ev->fit |= BIT_VI_FIT_RESIZE_FITS_Y;
					ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
				}
			}
			InvalidateRect(hwnd, 0, 0);

//errorf_old("ev->midX: %Lf, ev->midY: %Lf, x: %lld, y: %lld", ev->midX, ev->midY, ev->FullImage->x, ev->FullImage->y);

			break;
		//}
		case WM_LBUTTONDOWN: //{
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == NULL) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			if (ev->drag)
				break;

			if ((ev->fit & BIT_VI_FIT_RESIZE_FITS_X && ev->fit & BIT_VI_FIT_RESIZE_FITS_Y) || ev->fit & BIT_VI_FIT_MODE_ENABLED) {
				break;
			}
			ev->drag = 1;
			ev->xdrgpos = lParam & (256*256-1);
			ev->ydrgpos = lParam/256/256 & (256*256-1);
			ev->xdrgstart = ev->xpos;
			ev->ydrgstart = ev->ypos;

			SetCapture(hwnd);

			break;
		//}
		case WM_LBUTTONUP: //{
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == NULL) {
				errorf("ev is null");
				break;
			}

			if (!ev->drag)
				break;
			if (!ReleaseCapture())
				errorf("ReleaseCapture failed");
			ev->drag = 0;
			SendMessage(hwnd, WM_MOUSEMOVE, wParam, lParam);

			break;
		//}
		case WM_MOUSEMOVE: //{
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == NULL) {
				errorf("ev is null");
				break;
			}

			SetCursor(hDefCrs);
			if (!ev->drag) {
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			if (!(ev->fit & BIT_VI_FIT_RESIZE_FITS_X)) {
				l = rect.right;
				ev->xpos = ev->xdrgstart + (ev->xdrgpos - ((int16_t) (lParam & (256*256-1))));
				if (ev->xpos+rect.right > ev->xdisp)
					ev->xpos = ev->xdisp-rect.right;
				if (ev->xpos < 0)
					ev->xpos = 0;
				j = ev->xpos;
				ev->midX = (long double)(ev->xpos + (rect.right)/2.0)/(ev->zoomp/100.0);
			} else {
				l = ev->xdisp;
				j = 0;
			}
			if (!(ev->fit & BIT_VI_FIT_RESIZE_FITS_Y)) {
				m = rect.bottom;
				ev->ypos = ev->ydrgstart + (ev->ydrgpos - ((int16_t) ((lParam/256/256) & (256*256-1))));
				if (ev->ypos+rect.bottom > ev->ydisp)
					ev->ypos = ev->ydisp-rect.bottom;
				if (ev->ypos < 0)
					ev->ypos = 0;
				k = ev->ypos;
				ev->midY = (long double)(ev->ypos + (rect.bottom)/2.0)/(ev->zoomp/100.0);
			} else {
				m = ev->ydisp;
				k = 0;
			}
			if (!ev->timed) {
				if (ev->DispImage) {
					DestroyImgF(ev->DispImage);
				}
				errorf("calling resizeimagechunk 3");
				ev->DispImage = ResizeImageChunk(ev->FullImage, (double) ev->zoomp / 100, l, m, j, k);
/*
				ev->fit &= ~(BIT_VI_FIT_RESIZE_FITS_X | BIT_VI_FIT_RESIZE_FITS_Y);
				if (ev->DispImage) {	//! maybe remove this portion since the status shouldn't change under WM_MOUSEMOVE
					if (ev->xdisp <= rect.right) {
						ev->fit |= BIT_VI_FIT_RESIZE_FITS_X;
						ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
					}
					if (ev->ydisp <= rect.bottom) {
						ev->fit |= BIT_VI_FIT_RESIZE_FITS_Y;
						ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
					}
				}
*/				InvalidateRect(hwnd, 0, 0);

				ev->timed = 1;
				SetTimer(hwnd, 1, 10, 0);
			} else {
/*				if (ev->DispImage) {	//! maybe remove this portion since the status shouldn't change under WM_MOUSEMOVE
					if (ev->xdisp <= rect.right) {
						ev->fit |= BIT_VI_FIT_RESIZE_FITS_X;
						ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
					}
					if (ev->ydisp <= rect.bottom) {
						ev->fit |= BIT_VI_FIT_RESIZE_FITS_Y;
						ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
					}
				}
*/			}

			break;
		//}
		case WM_TIMER: //{	//! zooming and changing stretch mode in the future should kill the timer
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == NULL) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (wParam == 1) {

				if (!(ev->fit & BIT_VI_FIT_RESIZE_FITS_X)) {
					l = rect.right;
					j = ev->xpos;
				} else {
					l = ev->xdisp;
					j = 0;
				}
				if (!(ev->fit & BIT_VI_FIT_RESIZE_FITS_Y)) {
					m = rect.bottom;
					k = ev->ypos;
				} else {
					m = ev->ydisp;
					k = 0;
				}
				if (ev->DispImage) {
					DestroyImgF(ev->DispImage);
				}
				if (ev->xpos < 0 && !(ev->fit & BIT_VI_FIT_RESIZE_FITS_X)) {
					errorf_old("ev->xpos is less than 0 under WM_TIMER: %d", ev->xpos);
//					ev->xpos = 0;
				}
				if (ev->ypos < 0 && !(ev->fit & BIT_VI_FIT_RESIZE_FITS_Y)) {
					errorf_old("ev->ypos is less than 0 under WM_TIMER: %d", ev->ypos);
//					ev->ypos = 0;
				}
				errorf("calling resizeimagechunk BIT_VI_FIT_RESIZE_FITS_X");
				ev->DispImage = ResizeImageChunk(ev->FullImage, (double) ev->zoomp / 100, l, m, j, k);
/*				ev->fit &= ~(BIT_VI_FIT_RESIZE_FITS_X | BIT_VI_FIT_RESIZE_FITS_Y);
				if (ev->DispImage) {	//! this should probably be removed since the fit mode nor ev->xdisp shouldn't change when dragging
					if (ev->xdisp <= rect.right) {
						ev->fit |= BIT_VI_FIT_RESIZE_FITS_X;
						ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
					}
					if (ev->ydisp <= rect.bottom) {
						ev->fit |= BIT_VI_FIT_RESIZE_FITS_Y;
						ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
					}
				}
*/				InvalidateRect(hwnd, 0, 0);
				KillTimer(hwnd, 1);
				ev->timed = 0;
			} else if (wParam == 2) {
				if (ev->dispframe >= ev->FullImage->n - 1) {
					ev->dispframe = 0;
				} else {
					ev->dispframe++;
				}

				KillTimer(hwnd, 2);
				SetTimer(hwnd, 2, ev->FullImage->dur[ev->dispframe]*10, 0);
				InvalidateRect(hwnd, 0, 0);
			}
			break;
		//}
		case WM_KEYDOWN: //{
			switch(wParam) {
			case VK_BACK:
				PostMessage(GetParent(hwnd), WM_U_VI, (WPARAM) GetMenu(hwnd), 1);

				break;
			}
			break;
		//}
		case WM_ERASEBKGND: //{
			return 1;
			break;
		//}
		case WM_DESTROY: //{
			return 0;
		//}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ FileTagEditClass
//public:
// static
const WindowHelper FileTagEditClass::helper = WindowHelper(std::wstring(L"FileTagEditClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		wc.hCursor = NULL;
	}
);

// static
HWND FileTagEditClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new FileTagEditClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK FileTagEditClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int64_t i, len;
	FTEV *ev = 0;
	char *buf, space_flag;
	wchar_t *wbuf;
	oneslnk *addaliaschn, *remaliaschn;
	oneslnk *link1, *link2, *link3, *link4, *link5;

	HDC hdc;
	PAINTSTRUCT ps;
	HFONT hOldFont;
	HPEN hOldPen;
	HBRUSH hOldBrush;


//errorf("x");
/*
	switch(msg) {
		case WM_CREATE: //{
errorf("fte create1");
			ev = AllocWindowMem(hwnd, sizeof(FTEV));
			ev->dnum = 0, ev->fnumchn = ev->tagnumchn = ev->aliaschn = 0;
			ev->rmnaliaschn = ev->regaliaschn = ev->remtagnumchn = ev->addtagnumchn = 0;

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			EditSuperClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 5, 5, 350, 200, hwnd, (HMENU) 1, NULL, NULL));

			SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont, 1);
//			SendDlgItemMessageW(hwnd, 1, EM_SETTEXTMODE, TM_PLAINTEXT, 0);

			EnableWindow(GetDlgItem(hwnd, 1), 0); // disable textedit until it is filled by tags

			CreateWindowW(L"Button", L"Apply", WS_VISIBLE | WS_CHILD, 5, 225, 80, 25, hwnd, (HMENU) 2, NULL, NULL);
			SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) hFont2, 1);
errorf("fte create9");

			break;
		//}
		case WM_PAINT: //{

			break;
		//}
		case WM_SIZE: //{

			break;
		//}
		case WM_MOUSEMOVE: //{
			SetCursor(hDefCrs);

			break;
		//}
		case WM_USER: //{	// input dnum or fnums
errorf("fte user 1");
			ev = GetWindowMem(hwnd);
errorf("fte user 2");
			if ((uint64_t) wParam == 0) {
				ev->dnum = (uint64_t) lParam;
			} else if ((uint64_t) wParam == 1) {
				ev->fnumchn = (oneslnk*) lParam;
errorf("fte user 2.2");

				if (ev->fnumchn) {
errorf("fte user 2.3");
					sortoneschnull(ev->fnumchn, 0);
errorf("fte user 2.4");
					EnableWindow(GetDlgItem(hwnd, 1), 1);
errorf("fte user 2.5");
					PostMessage(hwnd, WM_USER+2, 0, 0);
errorf("fte user 2.6");
					return 1;
				}
			}
errorf("fte user 3");
			break;
		//}
		case WM_USER+1: //{	// apply changes
			ev = GetWindowMem(hwnd);

errorf("applying changes");
			if (ev->rmnaliaschn) {
				if (!(CreateAliasClass::createWindowInstance().create(0, L"", WS_VISIBLE | WS_POPUP, 5, 5, 200, 200, hwnd, 0, NULL, NULL))) {
					errorf("failed to create registeralias window");

					killoneschn(ev->rmnaliaschn, 0), ev->rmnaliaschn = 0;
					if (ev->regaliaschn) {
						killoneschn(ev->regaliaschn, 0), ev->regaliaschn = 0;
					}
					if (ev->remtagnumchn) {
						killoneschn(ev->remtagnumchn, 1), ev->remtagnumchn = 0;
					}
					if (ev->addtagnumchn) {
						killoneschn(ev->addtagnumchn, 1), ev->addtagnumchn = 0;
					}

				}
			} else {
errorf_old("addtagnumchn %d, regaliaschn %d, remtagnumchn %d", ev->addtagnumchn, ev->regaliaschn, ev->remtagnumchn);
link1 = ev->regaliaschn;
while (link1) {
	errorf_old("regalias: %s", ev->regaliaschn->str);
	link1 = link1->next;
}
				twowaytcregaddrem(ev->dnum, ev->fnumchn, ev->addtagnumchn, ev->regaliaschn, ev->remtagnumchn, 1+2+4);

				if (ev->regaliaschn) {
					killoneschn(ev->regaliaschn, 0), ev->regaliaschn = 0;
				}
				if (ev->remtagnumchn) {
					killoneschn(ev->remtagnumchn, 1), ev->remtagnumchn = 0;
				}
				if (ev->addtagnumchn) {
					killoneschn(ev->addtagnumchn, 1), ev->addtagnumchn = 0;
				}

				PostMessage(hwnd, WM_USER+2, 0, 0);
			}

			break;
		//}
		case WM_USER+2: //{		load tags
			ev = GetWindowMem(hwnd);

			if (ev->dnum && ev->fnumchn) {
				if (ev->tagnumchn) {
					killoneschn(ev->tagnumchn, 1);
					ev->tagnumchn = 0;
				}
				if (ev->aliaschn) {
					killoneschn(ev->aliaschn, 0);
					ev->aliaschn = 0;
				}

				if (cfireadtag(ev->dnum, ev->fnumchn, 0, &link1, 1)) {
					errorf("cfireadtag failed");
				}

				if (link1) {
errorf("made it to combining tags");
					link2 = link1->next;
					while (link2) {		// remove tagnums that don't appear in all files
						link3 = link1->vp;
						link4 = link2->vp;

						while (link3 && link4) {
							if ((i = (link3->ull - link4->ull)) == 0) {
								if (link3 == link1->vp) {
									link1->vp = ((oneslnk*)link1->vp)->next;
									free(link3);
									link3 = link1->vp;
								} else {
									link5->next = link3->next;
									free(link3);
									link3 = link5->next;
								}

								link4 = link4->next;
							} else if (i > 0) {
								link4 = link4->next;
							} else {
								link5 = link3;
								link3 = link3->next;
							}
						}
						link2 = link2->next;
						if (!link1->vp)
							break;
					}
					ev->tagnumchn = link1->vp;
					link1->vp = 0;
					killoneschnchn(link1, 1);
				} else {
					ev->tagnumchn = 0;
				}

				if (ev->tagnumchn) {
errorf("made it to retrieving aliases");

					ctread(ev->dnum, ev->tagnumchn, &ev->aliaschn, 0, 1);
errorf("made it after ctread");

					if (!ev->aliaschn) {
						errorf("ctread failed");
					} else {
						sortoneschn(ev->aliaschn, (int(*)(void*,void*)) casestrcmp, 0);
						if (!ev->aliaschn->str) {
							errorf("empty alias in aliaschn");
						}

						for (i = 0, link1 = ev->aliaschn; link1; link1 = link1->next) {
							if (link1->str) {
								for (len = 0, space_flag = 0; link1->str[len] != '\0'; (link1->str[len] == ' ')?(space_flag = 1):(link1->str[len] == '\\' || link1->str[len] == '"')?(i++):0, i++, len++);
								i++;
								if (space_flag)
									i += 2;
							}
						}

						buf = malloc(i);
						for (i = 0, link1 = ev->aliaschn; link1; link1 = link1->next) {
							if (link1->str) {
								for (len = 0, space_flag = 0; link1->str[len] != '\0' && !space_flag; (link1->str[len] == ' ')?(space_flag = 1):0, len++);
								if (i != 0) {
									buf[i++] = ' ';
								}
								if (space_flag) {
									buf[i++] = '"';
								}
								for (len = 0; link1->str[len] != '\0'; (link1->str[len] == '\\' || link1->str[len] == '"')?(buf[i++] = '\\'):0, buf[i++] = link1->str[len], len++);
								if (space_flag) {
									buf[i++] = '"';
								}
							}
						} buf[i++] = '\0';

						wbuf = malloc(i*2);
						if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf, i)) == 0) {
							errorf("MultiByteToWideChar Failed");
						}
						free(buf);
						SetWindowTextW(GetDlgItem(hwnd, 1), wbuf);
						free(wbuf);
					}
				}
			}

errorf("fte9");

			break;
		//}
		case WM_ERASEBKGND: //{
//			return 1;
			break;
		//}
		case WM_COMMAND: //{
errorf("fte comm");
			ev = GetWindowMem(hwnd);

			if (HIWORD(wParam) == EN_UPDATE && !ev->clean_flag) {
				thwnd = GetDlgItem(hwnd, 1);
				ev->clean_flag = 1;
				CleanEditText(thwnd);
				ev->clean_flag = 0;
			} else if (HIWORD(wParam) == BN_CLICKED) { //! handle getting pressed when already pressed
				if (LOWORD(wParam) == 2 && !(ev->rmnaliaschn || ev->regaliaschn || ev->remtagnumchn || ev->addtagnumchn)) {
					thwnd = GetDlgItem(hwnd, 1);
					len = GetWindowTextLengthW(thwnd);
					if (!len) {
						buf = 0;
					} else {
						len++;
						if (!(wbuf = malloc(len*2))) {
							errorf("malloc failed");
							return 1;
						}
						GetWindowTextW(thwnd, wbuf, len);

						buf = malloc(len*4);

						if ((WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, len*4, NULL, NULL)) == 0) {
							errorf("WideCharToMultiByte Failed");
						}
						free(wbuf);
					}

					keepremovedandadded(ev->aliaschn, buf, &addaliaschn, &remaliaschn, 0);	// also considering directly turning the parsed aliases into tag nums then keeping removed and added from them
errorf("made it past keepremovedandadded");
					if (buf) {
						free(buf);
					}
// link1 = addaliaschn;
// while (link1) {
	// errorf_old("added aliases: %s", link1->str);
	// link1 = link1->next;
// }
// link1 = remaliaschn;
// while (link1) {
	// errorf_old("removed aliases: %s", link1->str);
	// link1 = link1->next;
// }

					if (remaliaschn) {
errorf("aliases to be removed exist");
link1 = remaliaschn;
while (link1) {
	errorf_old("removing tag with alias: %s", link1->str);
	link1 = link1->next;
}
						ev->remtagnumchn = tnumfromalias(ev->dnum, remaliaschn, &ev->rmnaliaschn);	//! add option to limit to primary aliases
						killoneschn(remaliaschn, 0);
						if (ev->rmnaliaschn) {
							errorf("removing tag from alias that doesn't reference to itself");
							ev->remtagnumchn? killoneschn(ev->remtagnumchn, 1):0, killoneschn(ev->rmnaliaschn, 0), addaliaschn? killoneschn(addaliaschn, 0):0;
							ev->rmnaliaschn = 0;
							break;
						}
						ev->remtagnumchn ? sortoneschnull(ev->remtagnumchn, 0):0;
link1 = ev->remtagnumchn;
while (link1) {
	errorf_old("removing tag num: %llu", link1->ull);
	link1 = link1->next;
}
					} else {
						ev->remtagnumchn = 0;
					}
					if (addaliaschn) {
						ev->addtagnumchn = tnumfromalias(ev->dnum, addaliaschn, &ev->rmnaliaschn);
						killoneschn(addaliaschn, 0);
						if (ev->addtagnumchn) {
							sortoneschnull(ev->addtagnumchn, 0);
							for (link1 = ev->addtagnumchn; link1->next; link1 = link1->next) {
								if ((link1->next->ull - link1->ull) == 0) {
									link2 = link1->next, link1 = link1->next->next;
									free(link2);
								}
							}
						}
					} else {
errorf("no aliases to add");
						ev->addtagnumchn = 0, ev->rmnaliaschn = 0;
					}

errorf_old("ev->rmnaliaschn: %d, ev->addtagnumchn: %d, ev->remtagnumchn: %d", ev->rmnaliaschn, ev->addtagnumchn, ev->remtagnumchn);
link1 = ev->rmnaliaschn;
while (link1) {
	errorf_old("remaining aliases: %s", link1->str);
	link1 = link1->next;
}
link1 = ev->addtagnumchn;
while (link1) {
	errorf_old("adding tag num: %llu", link1->ull);
	link1 = link1->next;
}

					link1 = ev->addtagnumchn, link2 = ev->remtagnumchn;

					while (link1 && link2) {	// if a tag gets removed but another alias containing the tag gets added
						if ((i = (link1->ull - link2->ull)) == 0) {
							if (link1 == ev->addtagnumchn) {
								ev->addtagnumchn = ev->addtagnumchn->next;
								free(link1);
								link1 = ev->addtagnumchn;
							} else {
								link3->next = link1->next;
								free(link1);
								link1 = link3->next;
							}

							if (link2 == ev->remtagnumchn) {
								ev->remtagnumchn = ev->remtagnumchn->next;
								free(link2);
								link2 = ev->remtagnumchn;
							} else {
								link4->next = link2->next;
								free(link2);
								link2 = link4->next;
							}
						} else if (i > 0) {
							link4 = link2;
							link2 = link2->next;
						} else {
							link3 = link1;
							link1 = link1->next;
						}
					}

					link1 = ev->addtagnumchn, link2 = ev->tagnumchn;

					while (link1 && link2) {	// if an alias containing an existing tag gets added
						if ((i = (link1->ull - link2->ull)) == 0) {
							if (link1 == ev->addtagnumchn) {
								ev->addtagnumchn = ev->addtagnumchn->next;
								free(link1);
								link1 = ev->addtagnumchn;
							} else {
								link3->next = link1->next;
								free(link1);
								link1 = link3->next;
							}

							link2 = link2->next;
						} else if (i > 0) {
							link2 = link2->next;
						} else {
							link3 = link1;
							link1 = link1->next;
						}
					}

					link1 = ev->remtagnumchn, link2 = ev->tagnumchn;

					while (link1 && link2) {	// if for some reason the tag name doesn't alias back to itself
						if ((i = (link1->ull - link2->ull)) == 0) {
							link3 = link1;
							link1 = link1->next;
							link2 = link2->next;
						} else if (i > 0) {
							link2 = link2->next;
						} else {
							if (link1 == ev->remtagnumchn) {
								ev->remtagnumchn = ev->remtagnumchn->next;
								free(link1);
								link1 = ev->remtagnumchn;
							} else {
								link3->next = link1->next;
								free(link1);
								link1 = link3->next;
							}
						}
					}
					while (link1) {
						if (link1 == ev->remtagnumchn) {
							ev->remtagnumchn = ev->remtagnumchn->next;
							free(link1);
							link1 = ev->remtagnumchn;
						} else {
							link3->next = link1->next;
							free(link1);
							link1 = link3->next;
						}
					}

					SendMessage(hwnd, WM_USER+1, 0, 0);

					break;


				}
else if (LOWORD(wParam) == 2) {
	errorf_old("ev->rmnaliaschn: %d, ev->regaliaschn: %d, ev->remtagnumchn: %d, ev->addtagnumchn: %d", ev->rmnaliaschn, ev->regaliaschn, ev->remtagnumchn, ev->addtagnumchn);

}
			}
errorf("fte comm 9");
			break;
		//}
		case WM_DESTROY: //{
errorf("fte destroy1");
			ev = GetWindowMem(hwnd);
errorf("fte destroy2");
			if (ev->fnumchn) {
				killoneschn(ev->fnumchn, 1);
			}
errorf("fte destroy2.1");
errorf_old("%llu", ev->tagnumchn);
link1 = ev->tagnumchn;
while (link1) {
	errorf_old("num %llu", link1->ull);
	errorf_old("%llu", link1->next);
	link1 = link1->next;
}
			if (ev->tagnumchn) {
				killoneschn(ev->tagnumchn, 1);
			}
errorf("fte destroy2.2");
			if (ev->aliaschn) {
				killoneschn(ev->aliaschn, 0);
			}
errorf("fte destroy2.5");
			if (ev->rmnaliaschn) {
				killoneschn(ev->rmnaliaschn, 0);
			}
			if (ev->regaliaschn) {
				killoneschn(ev->regaliaschn, 0);
			}
			if (ev->remtagnumchn) {
				killoneschn(ev->remtagnumchn, 1);
			}
			if (ev->addtagnumchn) {
				killoneschn(ev->addtagnumchn, 1);
			}
errorf("fte destroy3");

			//FreeWindowMem(hwnd);
errorf("fte destroy4");

errorf_old("parent: %llu", GetParent(hwnd));
			SetFocus(GetParent(hwnd));

			return 0;
		//}
	}
*/
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ CreateAliasClass
//public:
// static
const WindowHelper CreateAliasClass::helper = WindowHelper(std::wstring(L"CreateAliasClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		wc.hCursor = NULL;
	}
);

// static
HWND CreateAliasClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new CreateAliasClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK CreateAliasClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int64_t i;

	HDC hdc;
	PAINTSTRUCT ps;
	HFONT hOldFont;
	HPEN hOldPen;
	HBRUSH hOldBrush;

	switch(msg) {
		case WM_CREATE: //{
			ShowWindow(hwnd, 0);
			if (!(thwnd = GetParent(hwnd))) {
				errorf("couldn't get createalias parent");
				return -1;
			}
			//! TODO: make the casting safer
			if (!(this->parent = std::dynamic_pointer_cast<FileTagEditClass>(WindowClass::getWindowPtr(thwnd)))) {
				errorf("couldn't get parent window memory for createalias");
				return -1;
			}

			PostMessage(hwnd, WM_USER, 0, 0);

			break;
		//}
		case WM_PAINT: //{

			break;
		//}
		case WM_USER: //{
			this->parent->regaliaschn = this->parent->rmnaliaschn;
			this->parent->rmnaliaschn = 0;
			PostMessage(thwnd, WM_USER+1, 0, 0);
			DestroyWindow(hwnd);

			break;
		//}
		case WM_SIZE: //{

			break;
		//}
		case WM_ERASEBKGND: //{
//			return 1;
			break;
		//}
		case WM_COMMAND: //{
		//}
		case WM_DESTROY: //{
			return 0;
		//}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ EditSuperClass
//public:
// static
DeferredRegWindowHelper EditSuperClass::helper(DeferredRegWindowHelper(std::wstring(L"EditSuperClass"),
	[](WNDCLASSW &wc) -> void {
		WNDCLASSW t_wc = {0};
errorf("ESC helper 1");
		int c = GetClassInfoW(NULL, MSFTEDIT_CLASS, &t_wc);
		if (!c) {
			g_errorfStream << "GetClassInfoW failed" << "\n";
		}
		if (t_wc.lpfnWndProc == NULL) {
			errorf("t_wc has no proc");
		}

		wc.style = t_wc.style;
		wc.cbClsExtra = t_wc.cbClsExtra;
		wc.cbWndExtra = t_wc.cbWndExtra;
		wc.hIcon = t_wc.hIcon;
		wc.hCursor = t_wc.hCursor;
		wc.hbrBackground = t_wc.hbrBackground;
		wc.lpszMenuName = t_wc.lpszMenuName;

		g_OldEditProc = t_wc.lpfnWndProc;
errorf("ESC helper 2");

	}
));

// static
HWND EditSuperClass::createWindowInstance(WinInstancer wInstancer) {
	// has to be registered before window instance is created
	if (!EditSuperClass::helper.isRegistered()) {
		errorf("createWindowInstance: ESC not registered");
		return NULL;
	}
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new EditSuperClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK EditSuperClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	//return CallWindowProcW(g_OldEditProc, hwnd, msg, wParam, lParam);

	RECT rect;
	HWND thwnd;

	switch(msg) {
		case WM_CREATE: //{
errorf("creating editsuperclass inner");
			//ShowWindow(hwnd, 0);

			//PostMessage(hwnd, WM_USER, 0, 0);

			return CallWindowProcW(g_OldEditProc, hwnd, msg, wParam, lParam);

			break;
		//}
		case WM_KEYDOWN: //{
			switch(wParam) {
			case VK_TAB:
				break;
			case VK_RETURN:
				break;
			default:
				return CallWindowProcW(g_OldEditProc, hwnd, msg, wParam, lParam);
			}
			break;
		//}
		case WM_USER: {
			ShowWindow(hwnd, 1);
		}
		case WM_CHAR: //{
			switch(wParam) {
			case '\t':
//				break;
			case '\n':
//				break;
			case '\r':
//				break;
			default:
				return CallWindowProcW(g_OldEditProc, hwnd, msg, wParam, lParam);
			}
			break;
		//}
		default:
			return CallWindowProcW(g_OldEditProc, hwnd, msg, wParam, lParam);
	}
	return CallWindowProcW(g_OldEditProc, hwnd, msg, wParam, lParam);
}


//}

//{ SearchBarClass
//public:
// static
const WindowHelper SearchBarClass::helper = WindowHelper(std::wstring(L"SearchBarClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		wc.hCursor = NULL;
	}
);

// static
HWND SearchBarClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new SearchBarClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK SearchBarClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int button_x = 25;
	int textb_h = 20;

	switch(msg) {
		case WM_CREATE: //{
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			EditSuperClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 5, 5, rect.right - 5 - button_x - 5 - 5, textb_h, hwnd, (HMENU) 1, NULL, NULL));
			CreateWindowW(L"Button", L"", WS_VISIBLE | WS_CHILD, rect.right - 5 - button_x, 5, button_x, button_x, hwnd, (HMENU) 2, NULL, NULL);

		//}
		case WM_SIZE: //{
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			thwnd = GetDlgItem(hwnd, 1);
			MoveWindow(thwnd, 5, 5, rect.right - 5 - button_x - 5 - 5, textb_h, 0);
			thwnd = GetDlgItem(hwnd, 2);
			MoveWindow(thwnd, rect.right - 5 - button_x, 5, button_x, button_x, 0);
		//}
		case WM_COMMAND: //{
			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) == 2) {
					thwnd = GetDlgItem(hwnd, 1);
					CleanEditText(thwnd);

					wchar_t *wbuf;
					int64_t len;
					char *buf = NULL;

					len = GetWindowTextLengthW(thwnd);
					if (!len) {
					} else {
						len++;
						if (!(wbuf = (wchar_t *) malloc(len*2))) {
							errorf("malloc failed");
							return 1;
						}
						GetWindowTextW(thwnd, wbuf, len);

						buf = mb_from_wide(wbuf);
						free(wbuf);
					}

					SendMessage(GetParent(hwnd), WM_U_RET_SBAR, (WPARAM) buf, (LPARAM) 0);

					free(buf);
				}
			}
		//}
		case WM_KEYDOWN: //{
		//}
		case WM_CHAR: //{
		//}
		default:
			return CallWindowProcW(g_OldEditProc, hwnd, msg, wParam, lParam);
	}
}


//}

//{ TextEditDialogClass
//public:
// static
const WindowHelper TextEditDialogClass::helper = WindowHelper(std::wstring(L"TextEditDialogClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		wc.hCursor = NULL;
	}
);

// static
HWND TextEditDialogClass::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new TextEditDialogClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK TextEditDialogClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int64_t i, len;

	HDC hdc;
	PAINTSTRUCT ps;
	HFONT hOldFont;
	HPEN hOldPen;
	HBRUSH hOldBrush;

	switch(msg) {
		case WM_CREATE: //{

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			EditSuperClass::createWindowInstance(WinInstancer(0, L"Default name", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 5, 5, 350, 200, hwnd, (HMENU) 1, NULL, NULL));

			SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) hFont, 1);
			SendDlgItemMessageW(hwnd, 1, EM_SETTEXTMODE, TM_PLAINTEXT, 0);

			CreateWindowW(L"Button", L"Apply", WS_VISIBLE | WS_CHILD, 5, 225, 80, 25, hwnd, (HMENU) 2, NULL, NULL);
			SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) hFont2, 1);

			//PostMessage(hwnd, WM_USER+1, 0, 0);

			break;
		//}
		case WM_PAINT: //{

			break;
		//}
		case WM_SIZE: //{

			break;
		//}
		case WM_MOUSEMOVE: //{
			SetCursor(hDefCrs);

			break;
		//}
		case WM_USER+1: //{
			//ev = GetWindowMem(hwnd);

			//FreeWindowMem(hwnd);

			//SetFocus(GetParent(hwnd));

			{
				char *buf = "Default name";

				SendMessage(GetParent(hwnd), WM_U_RET_TED, (WPARAM) buf, (LPARAM) 0);
				DestroyWindow(hwnd);

			}

			//free(buf);

			break;
		//}
		case WM_COMMAND: //{
			if (HIWORD(wParam) == EN_UPDATE && !this->clean_flag) {
				thwnd = GetDlgItem(hwnd, 1);
				this->clean_flag = 1;
				CleanEditText(thwnd);
				this->clean_flag = 0;
			} else if (HIWORD(wParam) == BN_CLICKED) { //! handle getting pressed when already pressed
				if (LOWORD(wParam) == 2) {

					thwnd = GetDlgItem(hwnd, 1);
					this->clean_flag = 1;
					CleanEditText(thwnd);
					this->clean_flag = 0;

					char *buf = 0;

					len = GetWindowTextLengthW(thwnd);
					if (!len) {
						buf = 0;
					} else {
						len++;
						wchar_t *wbuf;
						if (!(wbuf = (wchar_t *) malloc(len*2))) {
							errorf("malloc failed");
							return 1;
						}
						GetWindowTextW(thwnd, wbuf, len);

						buf = (char *) malloc(len*4);

						if ((WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, len*4, NULL, NULL)) == 0) {
							errorf("WideCharToMultiByte Failed");
						}
						free(wbuf);
					}

					SendMessage(GetParent(hwnd), WM_U_RET_TED, (WPARAM) buf, (LPARAM) 0);

					if (buf) {
						free(buf);
					}
					DestroyWindow(hwnd);

					break;


				}
			}
			break;
		//}
		case WM_NCDESTROY: //{
			//ev = GetWindowMem(hwnd);

			//FreeWindowMem(hwnd);

			SetFocus(GetParent(hwnd));

			return 0;
		//}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

//}

void DelRer(HWND hwnd, uint8_t option, STRLISTV const *slv) {	// changing would require replacing option and slv with nRows, StrListSel, strchn[0] and strchn[1]
	wchar_t *wbuf;
	oneslnk *link, *flink, *inlink;
	int i, j, pos;
	char *buf, confirmed = 0;
	RECT rect;

	//char (*rerf)(uint64_t dnum, char *dpath) = drer;
	//char (*rmvf)(uint64_t dnum) = drmv;
	//int (*crmvf)(oneslnk *dnumchn) = cdrmv;
	char (*rerf)(uint64_t dnum, char *dpath) = NULL;
	char (*rmvf)(uint64_t dnum) = NULL;
	int (*crmvf)(oneslnk *dnumchn) = NULL;

	link = flink = (oneslnk *) malloc(sizeof(oneslnk));
	for (pos = 0, i = 0; pos < slv->nRows; pos++) {
		if (slv->StrListSel[(pos)/8] & (1 << pos%8)) {
			link = link->next = (oneslnk *) malloc(sizeof(oneslnk));
			link->ull = ctou(slv->strs[0][pos]);
			i++;
		}
	}
	link->next = 0;
	if (pos != slv->nRows) {
		errorf("didn't go through all rows");
		killoneschn(link, 0);
	}
	link = flink, flink = flink->next;
	free(link);
	if (i == 0)
		return;

	switch(option) {

		case 1:

			buf = (char *) malloc(46+20+MAX_PATH*4);
			wbuf = (wchar_t *) malloc(2*(46+20+MAX_PATH));
			if (i == 1) {
				char *temp;
				char *SelDir = utoc(flink->ull);
				for (pos = 0; pos < slv->nRows; pos++) {
					if (slv->StrListSel[pos/8] & (1 << pos%8)) {
						break;
					}
				}
				if (pos < slv->nRows && slv->strs[1][pos]) {
					sprintf(buf, "Are you sure you want to remove Directory %s: \"%s\"?", SelDir, slv->strs[1][pos]);
				} else {
					sprintf(buf, "Are you sure you want to remove Directory %s?", SelDir);
				}
				if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf, 45+20+MAX_PATH*4)) == 0) {
					errorf("MultiByteToWideChar Failed");
				}
				if (MessageBoxW(hwnd, wbuf, L"Confirm", MB_YESNO) == IDYES) {
					rmvf(flink->ull);
					confirmed = 1;
				}
				free(SelDir);
			} else {
				sprintf(buf, "Are you sure you want to remove %d directories?", i);
				if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf, 46+7)) == 0) {
					errorf("MultiByteToWideChar Failed");
				}
				if (MessageBoxW(hwnd, wbuf, L"Confirm", MB_YESNO) == IDYES) {
					crmvf(flink);
					confirmed = 1;
				}
			}
			free(buf), free(wbuf);

			if (confirmed) {
				for (pos = 0; pos < slv->nRows; pos++) {
					if (slv->StrListSel[(pos)/8] & (1 << pos%8)) {
						j = pos;
					}
				}
				for (pos = slv->nRows/8+!!(slv->nRows%8)-1; pos >= 0; pos--) {
					if (pos < 0)
						break;
					slv->StrListSel[pos] = 0;
				}
				if (slv->nRows-i != 0) {
					if (j > slv->nRows-i-1) {
						j = slv->nRows-i-1;
					}
					slv->StrListSel[(j)/8] |= (1 << j%8);
				}
			}

			break;

		case 2:
		case 3:

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			buf = (char *) malloc(MAX_PATH*4);
			pos = i;

			for (link = flink; link != 0; link = link->next) {	//! change binfo.lpszTitle to include dnum and dpath of dir being rerouted
				//! TODO: add error return argument and print in window
				std::fs::path retPath = SeekDir(hwnd);
				if (!retPath.empty()) {
					bool abort = false;

					if (option == 3) {

						//! TODO: add error return argument and print in window
						retPath = makePathRelativeToProgDir(retPath);

						if (!retPath.empty()) {
							if (!std::fs::is_directory(g_fsPrgDir / retPath)) {
								errorf("retPath was not directory");
								g_errorfStream << "retPath was: " << retPath << std::flush;

								retPath.clear();
							}
						}
					}

					if (retPath.empty()) {
						abort = true;
					}

					if (!abort) {
						if (pos == 1) {
							rerf(link->ull, buf);
						} else {
							rerf(link->ull, buf);		//! add to queue here
						}
					}
				} else if (link->next != 0) {
					if (MessageBoxW(hwnd, L"Stop rerouting process?", L"Confirm", MB_YESNO) == IDYES) {
						break;
					}
				}
			}
			if (pos != 1 && MessageBoxW(hwnd, L"Apply changes?", L"Confirm", MB_YESNO) == IDYES) {
				//! pop queue here
			} else {

			}
			free(buf);

			break;
	}
	killoneschn(flink, 1);
}

int CleanEditText(HWND hwnd) {
	int32_t i, offset;
	int64_t len;
	wchar_t *buf;
	DWORD selstart, selend;

	len = GetWindowTextLengthW(hwnd);
	if (!len) {
		return 0;
	} len++;
	SendMessage(hwnd, EM_GETSEL, (WPARAM) &selstart, (LPARAM) &selend);
	if (!(buf = (wchar_t *) malloc(len*2))) {
		errorf("malloc failed");
		return 1;
	}
	GetWindowTextW(hwnd, buf, len);

	for (i = 0, offset = 0; buf[i] != '\0'; i++) {
		if (buf[i] == '\n' || buf[i] == '\t' || buf[i] == '\r') {
			if (i+offset < selstart) {
				selstart--;
			} if (i+offset < selend) {
				selend--;
			}
			offset++;
		} else {
			buf[i-offset] = buf[i];
		}
	}

	buf[i-offset] = buf[i];
	if (offset == 0) {
		free(buf);
		return 0;
	}
	SetWindowTextW(hwnd, buf);
	free(buf);
	SendMessage(hwnd, EM_SETSEL, selstart, selend);
	return 0;
}

char parsefiletagstr(char *input, oneslnk **parsedchn) {
	oneslnk *flink, *link;
	char buf[10000];
	uint64_t ull1, ull2;
	uint8_t quoted;

	if (!parsedchn) {
		errorf("no parsedchn");
		return 1;
	}
	*parsedchn = 0;
	if (!input) {
		errorf("no input string in parsefiletagstr");
		return 1;
	}
	link = flink = (oneslnk *) malloc(sizeof(oneslnk)), flink->str = 0;

	ull1 = ull2 = quoted = 0;
	while (1) {
		if (ull2 >= 10000) {
			errorf("parsed tag too long");
			link->next = 0;
			killoneschn(flink, 0);
			return 1;
		}
		if (input[ull1] == '\0') {
			if (ull2 != 0) {
				buf[ull2] = '\0';
				link = link->next = (oneslnk *) malloc(sizeof(oneslnk));
				link->str = (unsigned char *) dupstr(buf, 10000, 0);
			}
			break;
		} else if (input[ull1] == '"') {
			if (quoted) {
				if (ull2 != 0) {
					buf[ull2] = '\0';
					link = link->next = (oneslnk *) malloc(sizeof(oneslnk));
					link->str = (unsigned char *) dupstr(buf, 10000, 0);
					ull2 = 0;
				}
			}
			quoted = !quoted;
			ull1++;
		} else if (input[ull1] == ' ' && !quoted) {
			if (ull2 != 0) {
				buf[ull2] = '\0';
				link = link->next = (oneslnk *) malloc(sizeof(oneslnk));
				link->str = (unsigned char *) dupstr(buf, 10000, 0);
				ull2 = 0;
			}
			ull1++;
		} else {
			if (input[ull1] == '\\') {
				ull1++;
				if (input[ull1] == '\0') {
					continue;
				}
			}
			if (input[ull1] != '\t' && input[ull1] != '\n' && input[ull1] != '\r') {
				buf[ull2++] = input[ull1];
			} ull1++;
		}
	}

	link->next = 0;
	*parsedchn = flink->next;
	free(flink);
	return 0;
}

char keepremovedandadded(oneslnk *origchn, char *buf, oneslnk **addaliaschn, oneslnk **remaliaschn, uint8_t presort) {		// if presorted for origchn
	oneslnk *parsedchn, *link1, *link2, *link3, *link4;

	if (!addaliaschn || !remaliaschn) {
		errorf("no addaliaschn or no remaliaschn");
		return 1;
	}
	*addaliaschn = *remaliaschn = 0;
	if (!buf) {
		parsedchn = 0;
	} else {
		parsefiletagstr(buf, &parsedchn);
	}

	if (parsedchn) {
		if (sortoneschn(parsedchn, (int(*)(void*,void*)) strcmp, 0)) {
			errorf("sortoneschn failed");
			killoneschn(parsedchn, 0);
			return 1;
		}
		if (parsedchn->str == 0) {
			errorf("parsed null string");
		}
		for (link1 = parsedchn; link1->next; link1 = link1->next) {
			if (strcmp((char *) link1->next->str, (char *) link1->str) == 0) {
				link2 = link1->next, link1 = link1->next->next;
				free(link2->str), free(link2);
			}
		}
	}

////////
if (!origchn) { errorf("remadd no origchn"); } else { errorf("remadd with origchn"); } //!
////////

	if (!presort && origchn) {
		 origchn = copyoneschn(origchn, 0);
		if (sortoneschn(origchn, (int(*)(void*,void*)) strcmp, 0)) {
			errorf("sortoneschn failed");
			killoneschn(parsedchn, 0), killoneschn(origchn, 0);
			return 1;
		}
	} if (origchn && origchn->str == 0) {
		errorf("origchn null string");
	}
////////
link1 = origchn;
while (link1) {
	errorf_old("origchn: %s", link1->str);
	link1 = link1->next;
}
link1 = parsedchn;
while (link1) {
	errorf_old("parsedchn: %s", link1->str);
	link1 = link1->next;
}
///////

	link1 = parsedchn, link2 = origchn;
	if (!presort) {
		while (link1 && link2) {
			int32_t i = strcmp((char *) link1->str, (char *) link2->str);
			if (i == 0) {
				if (link1 == parsedchn) {
					parsedchn = parsedchn->next;
					if (link1->str) free(link1->str);
					free(link1);
					link1 = parsedchn;
				} else {
					link3->next = link1->next;
					if (link1->str) free(link1->str);
					free(link1);
					link1 = link3->next;
				}

				if (link2 == origchn) {
					origchn = origchn->next;
					if (link2->str) free(link2->str);
					free(link2);
					link2 = origchn;
				} else {
					link4->next = link2->next;
					if (link2->str) free(link2->str);
					link2 = link4->next;
				}
			} else if (i > 0) {
				link4 = link2;
				link2 = link2->next;
			} else {
				link3 = link1;
				link1 = link1->next;
			}
		}
		*remaliaschn = origchn;
	} else {
		*remaliaschn = link4 = (tagoneslink *) malloc(sizeof(oneslnk));

		while (link1 && link2) {
			int32_t i = strcmp((char *) link1->str, (char *) link2->str);
			if (i == 0) {
				if (link1 == parsedchn) {
					parsedchn = parsedchn->next;
					if (link1->str) free(link1->str);
					free(link1);
					link1 = parsedchn;
				} else {
					link3->next = link1->next;
					if (link1->str) free(link1->str);
					link1 = link3->next;
				}

				link2 = link2->next;
			} else if (i > 0) {
				link4 = link4->next = (tagoneslink *) malloc(sizeof(oneslnk));
				link4->str = (unsigned char *) dupstr((char *) link2->str, 10000, 0);

				link2 = link2->next;
			} else {
				link3 = link1;
				link1 = link1->next;
			}
		}
		while (link2) {
			link4 = link4->next = (tagoneslink *) malloc(sizeof(oneslnk));
			link4->str = (unsigned char *) dupstr((char *) link2->str, 10000, 0);

			link2 = link2->next;
		}
		link4->next = 0, link4 = *remaliaschn, *remaliaschn = (*remaliaschn)->next;
		free(link4);
	}

	*addaliaschn = parsedchn;

	return 0;
}

void dialogf(HWND hwnd, char *str, ...) {
	char buf[1001];
	va_list args;

	va_start(args, str);
	std::shared_ptr<wchar_t[]> wbuf = std::shared_ptr<wchar_t[]> (new wchar_t[(vsprintf(buf, str, args)+1)]);
	va_end(args);

	if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf.get(), 1001)) == 0) {
		errorf("MultiByteToWideChar Failed");
	}
	MessageBoxW(hwnd, wbuf.get(), 0, 0);
}