#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

#include "userinterface.hpp"
#include "uiutils.hpp"
#include "portables.hpp"
#include "indextools.hpp"
#include "prgdir.hpp"
#include "window_procedures.hpp"
#include "images.hpp"

extern "C" {
	#include "breakpath.h"
	#include "stringchains.h"
	#include "dupstr.h"
	#include "arrayarithmetic.h"
	#include "bytearithmetic.h"
	#include "portables.h"
	#include "tfiles.h"
}

#include <windows.h>
#include <shlobj.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <richedit.h>

#include <combaseapi.h>

#include <objbase.h>
#include <initguid.h>

#include <memory>
#include <locale>
#include <map>
#include <algorithm>
#include <forward_list>
#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}

/// private declarations

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int MainInit(MainInitStruct &ms);
int MainDeInit(MainInitStruct &ms);

void MainInitHandles(SharedWindowVariables &);
void MainDeInitHandles(SharedWindowVariables &);

/// global variables
extern std::filesystem::path g_fsPrgDir;

/// constants

//# enums

enum MenuCodes : int32_t {
	// file stands for the file menu (leftmost)
	kIDM_FileMIMan = 1,
	kIDM_FileDMan = 2
};

enum ListLimitOptions : int32_t {
	kMaxNRows = 3,  // upping this to 1000 later
	kMaxNThumbs = 10,
	kBaseWMemSize = 4
};

enum WinProcMessages : int32_t {WM_U_SL = WM_USER+3, WM_U_RET_SL, WM_U_TL, WM_U_RET_TL, WM_U_PL, WM_U_RET_PL, WM_U_DMAN, WM_U_RET_DMAN, WM_U_TMAN, WM_U_RET_TMAN, WM_U_VI, WM_U_RET_VI, WM_U_RET_SBAR, WM_U_TOP, WM_U_INIT, WM_U_TED, WM_U_RET_TED, WM_U_LISTMAN, WM_U_RET_LISTMAN };

enum ListManOptions : int32_t { kListManRefresh = 0, kListmanChPageRef, kListmanChPageNoRef, kListmanSetDefWidths, kListmanLenRef, kListmanRetSelPos };

enum PageListOptions : int32_t { kPL_Refresh = 0 };

// TODO: remove if it doesn't do anything
enum StringListOptions { kSL_ChangePage = 0 };

enum UI_Colors : uint64_t {
	DL_NUM_UE_COL = 0x00d1d1d1, // BGR
	DL_NUM_E_COL = 0x00dbdbdb,
	DL_SL_COL = 0x00d6ad6b,
	DL_SA_COL = 0x00e8c178,
	DL_DIR_UE_COL = 0x00e3e3e3,
	DL_DIR_E_COL = 0x00eaeaea,
	DL_BORDER_COL = 0x00BEBEBE,
};

enum UI_SpacingConstants : int32_t {

	kDManTopMargin = 38,
	kDManBottomMargin = 21,
	kSBarH = 30,
	kStrListTopMargin = 17,
	kRowHeight = 16,
	kThumbListTopMargin = 5,
	kThumbListLeftMargin = 5,

	kCharWidth = 7,	// predictive
	kPagePad = 12,		// both sides combined
	kPagesSpace = 10,
	kPagesSideBuf = 12,
	kDefThumbW = 100,
	kDefThumbH = 100,
	kDefThumbFW = 110,
	kDefThumbFH = 110,
	kDefThumbGapX = 2,
	kDefThumbGapY = 1,

	kSearchBarButtonX = 25,
	kSearchBarTextbH = 20,
};

enum OtherOptionIntegers : int32_t {
	kHWndBufSize = 256
};
// #define HWND_BUF_SIZE sizeof(HWND)

enum ImageViewOptions : int32_t {
	kBitVI_FitModeEnabled = (1ULL << 0),
	kBitVI_FitDispIsOrig = (1ULL << 1),
	kBitVI_FitResizeFitsX = (1ULL << 2),
	kBitVI_FitResizeFitsY = (1ULL << 3)
};

enum MainInitReturnCodes : int32_t {
	kMainInitFail = 1, 
	kMainInitMutexReserved = 2,
	kMainInitDllLoadFail = 3,
	kMainInitRegisterClassFail = 4,
};

enum PageListButtonTypes : uint8_t {
	kPlbTypeCurrent = 1, 
	kPlbTypeLast = 2,
};

//# non-enums

inline const std::string kImgExtensions = ".bmp.gif.jpeg.jpg.png";

/// Initialization functions

void MainInitHandles(SharedWindowVariables &winVars) {
	winVars.hDefCrs = LoadCursor(nullptr, IDC_ARROW);
	winVars.hCrsSideWE = LoadCursor(nullptr, IDC_SIZEWE);
	winVars.hCrsHand = LoadCursor(nullptr, IDC_HAND);

	winVars.hFont = CreateFontW(14, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, 0, 0, L"Consolas");
	winVars.hFontUnderlined = CreateFontW(14, 0, 0, 0, FW_MEDIUM, 0, 1, 0, 0, 0, 0, 0, 0, L"Consolas");
	winVars.hFont2 = CreateFontW(16, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, 0, 0, L"Helvetica");
	winVars.color = GetSysColor(COLOR_BTNFACE);
	if (!(winVars.hPen1 = CreatePen(PS_SOLID, 0, DL_BORDER_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(winVars.bgPen1 = CreatePen(PS_SOLID, 0, winVars.color))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(winVars.bgPen2 = CreatePen(PS_SOLID, 0, DL_NUM_UE_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(winVars.bgPen3 = CreatePen(PS_SOLID, 0, DL_NUM_E_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(winVars.bgPen4 = CreatePen(PS_SOLID, 0, DL_DIR_UE_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(winVars.bgPen5 = CreatePen(PS_SOLID, 0, DL_DIR_E_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(winVars.selPen = CreatePen(PS_SOLID, 0, DL_SL_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(winVars.selPen2 = CreatePen(PS_SOLID, 0, DL_SA_COL))) {
		errorf("Preinit: CreatePen failed");
	}
	if (!(winVars.bgBrush1 = CreateSolidBrush(winVars.color))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(winVars.bgBrush2 = CreateSolidBrush(DL_NUM_UE_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(winVars.bgBrush3 = CreateSolidBrush(DL_NUM_E_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(winVars.bgBrush4 = CreateSolidBrush(DL_DIR_UE_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(winVars.bgBrush5 = CreateSolidBrush(DL_DIR_E_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
	if (!(winVars.selBrush = CreateSolidBrush(DL_SA_COL))) {
		errorf("Preinit: CreateSolidBrush failed");
	}
}

void MainDeInitHandles(SharedWindowVariables &winVars) {
	SetCursor(winVars.hDefCrs);
	if (!(DestroyCursor(winVars.hCrsSideWE))) {
		errorf("DestroyCursor failed");
	}
	if (!(DestroyCursor(winVars.hCrsHand))) {
		errorf("DestroyCursor failed");
	}
	if (winVars.hListSliceBM) {
		if (!(DeleteObject(winVars.hListSliceBM))) {
			errorf("DeleteObject failed");
		} winVars.hListSliceBM = 0;
	}
	if (winVars.hThumbListBM) {
		if (!(DeleteObject(winVars.hThumbListBM))) {
			errorf("DeleteObject failed");
		} winVars.hThumbListBM = 0;
	}
	if (!(DeleteObject(winVars.hFont))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgPen1))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgPen2))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgPen3))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgPen4))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgPen5))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.selPen))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.selPen2))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgBrush1))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgBrush2))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgBrush3))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgBrush4))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.bgBrush5))) {
		errorf("DeleteObject failed");
	}
	if (!(DeleteObject(winVars.selBrush))) {
		errorf("DeleteObject failed");
	}
}

void PingExistingProcess(HWND hwnd) {
		SendMessage(hwnd, WM_U_TOP, (WPARAM) 0, (LPARAM) 0);
}

/// Window Classes

// TODO: remove and use argument in procedures instead
extern std::shared_ptr<SharedWindowVariables> g_SharedWindowVar;

int32_t RegisterWindowClasses(void) {

}

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
			if (winArgs == nullptr) {
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
		if (winArgs == nullptr) {
			errorf("creating window class without lpParam");
			return -1;
		}

		*((void **)lParam) = winArgs->lpParam_;
	}

	LRESULT res = winPtr->winProc(hwnd, msg, wParam, lParam);
	if (msg == WM_NCDESTROY) {
		WindowClass::releaseWindowMemory(hwnd);
	}
	return res;
}

// TODO: make this non-static
std::map<HWND, std::shared_ptr<WindowClass>> WindowClass::winMemMap;

//# non-static

LRESULT WindowClass::onCreate(WinProcArgs procArgs) {

	return DefWindowProcW(procArgs.hwnd, procArgs.msg, procArgs.wParam, procArgs.lParam);
}

//} WindowClass

//{ Non-WindowClass

// WinCreateArgs
WinCreateArgs::WinCreateArgs(WindowClass *(*constructor_)(void)) : constructor{*constructor_}, lpParam_{0} {}

/*
WindowClass *WinCreateArgs::create(HWND hwnd) {
	return StoreWindowInstance(hwnd, this->constructor);
}
*/


// WinInstancer
WinInstancer::WinInstancer(DWORD dwExStyle_, LPCWSTR lpWindowName_, DWORD dwStyle_, int X_, int Y_, int nWidth_, int nHeight_, HWND hWndParent_, HMENU hMenu_, HINSTANCE hInstance_, LPVOID lpParam_) : dwExStyle_{dwExStyle_}, lpWindowName_{lpWindowName_}, dwStyle_{dwStyle_}, x_{X_}, y_{Y_}, nWidth_{nWidth_}, nHeight_{nHeight_}, hWndParent_{hWndParent_}, hMenu_{hMenu_}, hInstance_{hInstance_}, lpParam_{lpParam_} {}

HWND WinInstancer::create(std::shared_ptr<WinCreateArgs> winArgs, const std::wstring &winClassName) {
	winArgs->lpParam_ = lpParam_;

	return CreateWindowExW(dwExStyle_, winClassName.c_str(), lpWindowName_, dwStyle_, x_, y_, nWidth_, nHeight_, hWndParent_, hMenu_, hInstance_, winArgs.get());
}


// WindowHelper
WindowHelper::WindowHelper(const std::wstring winClassName, void (*modifyWinStruct)(WNDCLASSW &wc)) : winClassName_{winClassName} {
	WNDCLASSW wc = {0};

	wc.style = 0;
	wc.hbrBackground = 0;
	wc.hCursor = nullptr;

	modifyWinStruct(wc);

	wc.lpszClassName = this->winClassName_.c_str();
	wc.lpfnWndProc = WindowClass::generalWinProc;
	// wc.hInstance = g_SharedWindowVar->ghInstance;

	int c = RegisterClassW(&wc);
	if (!c) {
//! TODO: add collision detection and crash on collision
		errorf("RegisterClassW failed");
	}
}

WindowHelper::WindowHelper(const std::wstring winClassName_) : winClassName_{winClassName_} {}

bool WindowHelper::registerWindowClass() {
	return false;
}

// DeferredRegWindowHelper
DeferredRegWindowHelper::DeferredRegWindowHelper(const std::wstring winClassName_, void (*modifyWinStruct)(WNDCLASSW &wc))
	: WindowHelper(winClassName_)
	, modifyWinStruct_{modifyWinStruct}
	, isRegistered_(false)
{}

bool DeferredRegWindowHelper::registerWindowClass() {
	if (!this->isRegistered_) {

		WNDCLASSW wc = {0};

		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;

		modifyWinStruct_(wc);

		wc.lpfnWndProc = WindowClass::generalWinProc;
		wc.lpszClassName = this->winClassName_.c_str();
		wc.hInstance = g_SharedWindowVar->ghInstance;

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
		wc.hCursor = nullptr;
	}
);

// static
HWND MsgHandler::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new MsgHandler(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT MsgHandler::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	std::string prgDir = procArgs.sharedWinVars->prgDir;

	HWND thwnd;

	thwnd = TabContainerWindow::createWindowInstance(WinInstancer(0, L"FileTagIndex", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 500, nullptr, nullptr, g_SharedWindowVar->ghInstance, nullptr));

	if (!thwnd)	{
		errorf("failed to create TabContainerWindow");
		return -1;
	}
	ShowWindow(thwnd, SW_MAXIMIZE);

	{
		std::string fileMapStr = std::string(prgDir);
		std::replace(fileMapStr.begin(), fileMapStr.end(), '\\', '/');
		fileMapStr += "/HWND";

		std::wstring fileMapName = u8_to_u16(fileMapStr);

		// retained as member variable
		hMapFile_ = std::shared_ptr<void>(
			CreateFileMappingW(
				INVALID_HANDLE_VALUE, // use paging file
				nullptr,  // default security
				PAGE_READWRITE, // read/write access
				0,  // maximum object size (high-order DWORD)
				256, // maximum object size (low-order DWORD)
				fileMapName.c_str() // name of mapping object
			),
			CloseHandle
		);

		if (hMapFile_ == nullptr) {
			g_errorfStream << "Could not create file mapping object (" << GetLastError() << ")." << std::flush;
			return -1;
		}
		std::shared_ptr<void> pBuf = std::shared_ptr<void>(MapViewOfFile(hMapFile_.get(), FILE_MAP_ALL_ACCESS, 0, 0, kHWndBufSize), UnmapViewOfFile);

		if (pBuf == nullptr) {
			g_errorfStream << "Could not map view of file (" << GetLastError() << ")." << std::flush;
			return -1;
		}

		// copy the handle to the window to the file mapping
		// this way other processes can message this process
		CopyMemory((PVOID)(pBuf.get()), (VOID*) &hwnd, sizeof(HWND));
	}

	return WindowClass::onCreate(procArgs);

}

LRESULT CALLBACK MsgHandler::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam, g_SharedWindowVar));

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

//{ TabContainerWindow
//public:
// static
const WindowHelper TabContainerWindow::helper = WindowHelper(std::wstring(L"TabWindowClass"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hbrBackground = 0;
		wc.hCursor = LoadCursor(0, IDC_ARROW);
	}
);

// static
HWND TabContainerWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new TabContainerWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT TabContainerWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	// LPARAM &lParam = procArgs.lParam;
	// WPARAM &wParam = procArgs.wParam;

	HMENU hMenubar;
	HMENU hMenu;

	hMenubar = CreateMenu();
	hMenu = CreateMenu();

	//AppendMenuW(hMenu, MF_STRING, IDM_FILE_DMAN, L"&Manage Directories");
	AppendMenuW(hMenu, MF_STRING, kIDM_FileMIMan, L"&Manage Main Indices");
	AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR) hMenu, L"&File");

	SetMenu(hwnd, hMenubar);

	RECT rect;
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	HWND thwnd = 0;
	dispWindow_ = thwnd = TabWindow::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, rect.right, rect.bottom, hwnd, (HMENU) 3, nullptr, nullptr));

	if (!thwnd) {
		errorf("failed to create TabWindow");
		return -1;
	}

	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK TabContainerWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_PAINT: {
			HPEN hOldPen;
			HBRUSH hOldBrush;
			HDC hdc;
			PAINTSTRUCT ps;
			
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			hdc = BeginPaint(hwnd, &ps);
			hOldBrush = (HBRUSH) SelectObject(hdc, g_SharedWindowVar->bgBrush1);
			hOldPen = (HPEN) SelectObject(hdc, g_SharedWindowVar->bgPen1);
			Rectangle(hdc, 0, 0, rect.right, rect.bottom);
			SelectObject(hdc, hOldBrush);
			SelectObject(hdc, hOldPen);
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_SIZE: {
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			MoveWindow(dispWindow_, 0, 0, rect.right, rect.bottom, 0);
		}
		case WM_SETFOCUS: {
			SetFocus(dispWindow_);

			break;
		}
		case WM_COMMAND: {

			if ((wParam & (256U*256-1)*256*256) == 0 && lParam == 0) {
				switch(LOWORD(wParam)) {
				case kIDM_FileMIMan:
					if (!wHandle_[0]) {
						lastOption_ = 0;
						//DirManWindow::createWindowInstance(WinInstancer(0, L"Manage Directories", WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_SIZEBOX | WS_POPUP, 200, 200, 300, 300, hwnd, 0, g_shared_window_vars.ghInstance, 0));
						MainIndexManWindow::createWindowInstance(WinInstancer(0, L"Manage Main Indices", WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_SIZEBOX | WS_POPUP, 200, 200, 300, 300, hwnd, 0, g_SharedWindowVar->ghInstance, 0));
					} else {
						if (!BringWindowToTop(wHandle_[0])) {
							errorf("BringWindowToTop for wHandle_[0] failed");
						}
					}
					break;
				}
				break;
			}

			break;
		}
		case WM_ERASEBKGND: {
			return 1;
		}
		case WM_DESTROY: {

			PostQuitMessage(0);

			return 0;
		}
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//} TabContainerWindow

//{ TabWindow
//public:
// static
const WindowHelper TabWindow::helper = WindowHelper(std::wstring(L"TabWindow"),
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

LRESULT TabWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;

	HWND thwnd;
	thwnd = CreateWindowW(L"Button", L"Open Dir", WS_VISIBLE | WS_CHILD , 5, 5, 80, 25, hwnd, (HMENU) 1, nullptr, nullptr);
	SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);
	if (!thwnd) return -1;

	{
		int y1 = 20;

		//thwnd = CreateWindowExW(0, MSFTEDIT_CLASS, L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 5, 35, 380, y1, hwnd, (HMENU) 2, nullptr, nullptr);
errorf("creating editsuperclass outer");
		thwnd = EditWindowSuperClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 5, 35, 380, y1, hwnd, (HMENU) 2, nullptr, nullptr));
		if (!thwnd) {
			errorf("couldn't create editwindow");
		}
		thwnd = EditWindowSuperClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 5, 35 + y1 + 5, 380, 300, hwnd, (HMENU) 3, nullptr, nullptr));
		if (!thwnd) {
			errorf("couldn't create editwindow 2");
		}
	}

	return WindowClass::onCreate(procArgs);

}

LRESULT CALLBACK TabWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}

		case WM_PAINT: {

			HPEN hOldPen;
			HBRUSH hOldBrush;
			HDC hdc;
			PAINTSTRUCT ps;
			
			RECT rect;

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			hdc = BeginPaint(hwnd, &ps);
			hOldBrush = (HBRUSH) SelectObject(hdc, g_SharedWindowVar->bgBrush1);
			hOldPen = (HPEN) SelectObject(hdc, g_SharedWindowVar->bgPen1);
			Rectangle(hdc, 0, 0, rect.right, rect.bottom);
			SelectObject(hdc, hOldBrush);
			SelectObject(hdc, hOldPen);
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_SIZE: {
			RECT rect;

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			MoveWindow(this->dispWindow_, 0, 0, rect.right, rect.bottom, 0);
		}
		case WM_SETFOCUS: {
			if (this->dispWindow_) {
				SetFocus(this->dispWindow_);
			}

			break;
		}
		case WM_COMMAND: {

			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) == 1) {

					//! TODO: set to MIMANARGS
					DIRMANARGS args = {};
					args.option = 2;
					args.parent = hwnd;

					MainIndexManWindow::createWindowInstance(WinInstancer(0, L"Select Index", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 100, 100, 500, 500, hwnd, nullptr, g_SharedWindowVar->ghInstance, (LPVOID) &args));
				}
			}

			break;
		}
		case WM_KEYDOWN: {
			switch(wParam) {
			case VK_BACK:
				DestroyWindow(this->dispWindow_);

				ShowWindow(GetDlgItem(hwnd, 1), 1);
				ShowWindow(GetDlgItem(hwnd, 2), 1);
				ShowWindow(GetDlgItem(hwnd, 3), 1);
				this->dispWindow_ = 0;

				break;
			}
			break;
		}
		case WM_U_RET_DMAN: {
		//!
		/*

			switch(wParam) {
			case 0: // dman termination, lParam is hwnd
				for (i = 0; i < ndirman_; i++) {
					if ((HWND) lParam == wHandle_[i]) {
						wHandle_[i] = 0;
						break;
					}
				} if (i == ndirman_) {
					errorf("didn't zero wHandle_");
				}
				break;
			case 3: // selected directory
//				g_errorfStream << "returned: " << ((int) lParam) << std::flush;
				if ((int) lParam == 0)
					break;
				lastDirNum_ = (int) lParam;
				lastOption_ = 0;
				buf = dRead(lastDirNum_);
				if (buf != nullptr && existsdir(buf)) {
					THMBMANARGS args = {};
					args.parent = hwnd;
					args.option = 0;
					args.dnum = lastDirNum_;

//					thwnd = ThumbManWindow::createWindowInstance(WinInstancer(0, L"Thumbnail View", WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_SIZEBOX | WS_POPUP, 200, 200, 300, 300, hwnd, 0, g_shared_window_vars.ghInstance, &args));

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					thwnd = ThumbManWindow::createWindowInstance(WinInstancer(0, L"Thumbnail View", WS_VISIBLE | WS_CHILD, 0, 0, rect.right, rect.bottom, hwnd, 0, g_shared_window_vars.ghInstance, &args));
					ShowWindow(GetDlgItem(hwnd, 1), 0);
					ShowWindow(GetDlgItem(hwnd, 2), 0);
					ShowWindow(GetDlgItem(hwnd, 3), 0);
					this->dispWindow_ = thwnd;
				} else {
					if (buf != nullptr) {
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
		}
		case WM_U_RET_TMAN: {

			switch(wParam) {
			case 0: {
					uint32_t i = 0;
					for (; i < nthmbman_; i++) {
						if ((HWND) lParam == wHandle2_[i]) {
							wHandle2_[i] = 0;
							break;
						}
					} if (i == nthmbman_) {
						errorf("didn't close wHandle2_");
					}
					break;
				}
				break;
			}
		}
		case WM_ERASEBKGND: {
			return 1;
		}
		case WM_NCDESTROY: {
			//FreeWindowMem(hwnd);

			PostQuitMessage(0);

			return 0;
		}
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}



//} TabWindow


//{ ListManWindow

int ListManWindow::menuCreate(HWND hwnd) {
	return 0;
}

int ListManWindow::menuUse(HWND hwnd, int32_t menu_id) {
	return 0;
}

uint64_t ListManWindow::getSingleSelID(void) const {
	ErrorObject retError;
	auto retStr = this->strListWin_->getSingleSelRowCol(0, &retError);

	if (retError) {
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




LRESULT CALLBACK ListManWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_U_LISTMAN: {

			switch(wParam) {

				case kListManRefresh:	{	// refresh rows
					//this->strListWin_->nRows = MAX_NROWS;
					PostMessage(hwnd, WM_USER+1, 0, 0);

					InvalidateRect(hwnd, nullptr, 1);

					break;
				}
				case kListmanChPageRef:	{	// change page and refresh
					if (lParam != 0) {
						SendMessage(hwnd, WM_U_LISTMAN, kListmanChPageNoRef, lParam);
					}
					PostMessage(hwnd, WM_USER+1, 0, 0);
					PostMessage(hwnd, WM_U_LISTMAN, kListmanSetDefWidths, 0);
					SendDlgItemMessageW(hwnd, 3, WM_U_SL, kSL_ChangePage, 0);

					break;
				}
				case kListmanChPageNoRef:	{ // change page without refreshing (when the window will be refreshed soon anyway)

					this->pageListWin_->curPage_ = lParam;

					SendDlgItemMessageW(hwnd, 4, WM_U_PL, kPL_Refresh, 0);

					break;
				}
				case kListmanSetDefWidths: {
					uint64_t fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;
					int32_t i = log10(fNum+kMaxNRows-1)+1;
					if ( !this->strListWin_->setColumnWidth(0, kCharWidth*(i)+6) ) {
						errorf("setColumnWidth failed (0)");
						return -1;
					}
					if ( !this->strListWin_->setColumnWidth(1, 0) ) {
						errorf("setColumnWidth failed (1)");
						return -1;
					}

					InvalidateRect(hwnd, nullptr, 1);

					break;

				}

				case kListmanLenRef: { 	// redetermine last page and refresh pages

					uint64_t lastlastpage = this->pageListWin_->lastPage_;
					int32_t j = 0;

					uint64_t ll = this->getNumElems();
g_errorfStream << "got getNumElems: " << ll << std::flush;
					this->pageListWin_->lastPage_ = ll/kMaxNRows + !!(ll % kMaxNRows);
					if (this->pageListWin_->lastPage_ == 0) {
						this->pageListWin_->lastPage_ = 1;
					}

					if (this->pageListWin_->curPage_ > this->pageListWin_->lastPage_) {
						this->pageListWin_->curPage_ = this->pageListWin_->lastPage_;
						j = 1;
					}

					if (this->pageListWin_->lastPage_ != lastlastpage) {
g_errorfStream << "set lastpage to " << this->pageListWin_->lastPage_ << std::flush;
						if (j == 1) {
							SendMessage(hwnd, WM_U_LISTMAN, kListmanChPageNoRef, this->pageListWin_->curPage_);
						}
						this->pageListWin_->hovered_ = 0;
						SetCursor(g_SharedWindowVar->hDefCrs);
						SendDlgItemMessageW(hwnd, 4, WM_USER, 0, 0);

						SendMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);

					} else {
						SendMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);
					}

					break;
				}
				//! TODO: move to protected function
				case kListmanRetSelPos: { 	// return selected row num
					// error if more than one selected
					if (this->winOptions_ == 2) {
						uint64_t ull = this->getSingleSelID();

						EnableWindow(this->parent_, 1);
						PostMessage(this->parent_, WM_U_RET_DMAN, 3, (LPARAM) ull);
						DestroyWindow(hwnd);
					}

					break;
				}
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

			hOldPen = (HPEN) SelectObject(hdc, g_SharedWindowVar->bgPen1);
			hOldBrush = (HBRUSH) SelectObject(hdc, g_SharedWindowVar->bgBrush1);

			Rectangle(hdc, 0, 0, rect.right, kDManTopMargin);
			SelectObject(hdc, g_SharedWindowVar->hPen1);
			MoveToEx(hdc, 0, 0, nullptr);
			LineTo(hdc, rect.right, 0);
			MoveToEx(hdc, 0, kDManTopMargin-1, nullptr);
			LineTo(hdc, rect.right, kDManTopMargin-1);

			if (this->pageListWin_->lastPage_ > 1) {
				MoveToEx(hdc, 0, rect.bottom-kDManBottomMargin, nullptr);
				LineTo(hdc, rect.right, rect.bottom-kDManBottomMargin);
			}
			SelectObject(hdc, hOldPen);
			SelectObject(hdc, hOldBrush);
			EndPaint(hwnd, &ps);

			break;
		}
		case WM_MOUSEMOVE: {

			this->pageListWin_->hovered_ = 0;
			SetCursor(g_SharedWindowVar->hDefCrs);

			break;
		}
		case WM_SETFOCUS: {
			SetFocus(GetDlgItem(hwnd, 3));

			break;
		}
		case WM_U_RET_SL: {

			switch (wParam) {
			case 0:
				return (LRESULT) &this->strListWin_;
			case 1:
			case 2:
			case 3:
				if (this->winOptions_ != 2) {
					// uint64_t fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;
					// DelRer(hwnd, lParam, &this->strListWin_);
					SendMessage(hwnd, WM_U_LISTMAN, kListmanLenRef, 0);
				}
				break;
			case 4:
				this->menuCreate(hwnd);
				break;
			case 5:	// make selection

				if (this->winOptions_ == 2) {
					PostMessage(hwnd, WM_U_LISTMAN, kListmanRetSelPos, 0);
				}
				break;
			}
			break;
		}
		case WM_U_RET_PL: {

			switch (wParam) {
			case 0:
				//return (LRESULT) &this->pageListWin_;
				errorf("deprecated: 3452089hre");
			case 1:
				//! TODO:
				SendMessage(hwnd, WM_U_LISTMAN, kListmanChPageRef, 0);
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

			if (this->parent_) {
				EnableWindow(this->parent_, 1);
			}
			break;
		}
		case WM_DESTROY: {

			break;
		}
		case WM_NCDESTROY: {

			HWND thwnd = this->parent_;

			if (thwnd) {
				EnableWindow(thwnd, 1);
				SendMessage(thwnd, WM_U_RET_DMAN, 0, (LPARAM) hwnd);
			}
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);

}

//} ListManWindow

//{ MainIndexManWindow
//public:
// static
const WindowHelper MainIndexManWindow::helper = WindowHelper(std::wstring(L"MainIndexManWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;
	}
);

// static
HWND MainIndexManWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new MainIndexManWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

int MainIndexManWindow::menuCreate(HWND hwnd) {
	HMENU hMenu;

	if (this->winOptions_ != 2) {
		hMenu = CreatePopupMenu();
		AppendMenuW(hMenu, MF_STRING, 1, L"&Remove");	// if menu handles are changed, the message from WM_COMMAND needs to be changed to correspond to the right case
		AppendMenuW(hMenu, MF_STRING, 2, L"&Rename");
		AppendMenuW(hMenu, MF_STRING, 3, L"&Manage Directories");
		AppendMenuW(hMenu, MF_STRING, 4, L"&Manage Subdirectories");
		AppendMenuW(hMenu, MF_STRING, 5, L"&Manage Files");
		AppendMenuW(hMenu, MF_STRING, 6, L"&Manage Tags");

		TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, this->strListWin_->point_.x, this->strListWin_->point_.y, 0, hwnd, 0);
		DestroyMenu(hMenu);
	}

	return 0;
};

int MainIndexManWindow::menuUse(HWND hwnd, int32_t menu_id) {
	g_errorfStream << "menu_id is: " << menu_id << std::flush;


	if (this->winOptions_ != 2) {
		switch (menu_id) {
		case 1:
		case 2: {
			if (this->winOptions_ != 2) {
				//fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;
				//DelRer(hwnd, menu_id, &this->strListWin_);
				SendMessage(hwnd, WM_USER, 0, 0);
				SendMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);
			}
			break;
		}
		case 3: {

			DIRMANARGS args = {0};
			args.option = 0;
			args.parent = hwnd;
			args.inum = this->getSingleSelID();
			HWND thwnd = 0;
			// g_errorfStream << "inum: " << args->inum << std::flush;
			if (args.inum != 0) {
				thwnd = DirManWindow::createWindowInstance(WinInstancer(0, L"Manage Directories", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 100, 100, 500, 500, hwnd, nullptr, g_SharedWindowVar->ghInstance, (LPVOID) &args));
				if (!thwnd) {
					errorf("failed to create DirManWindow from MainIndexManWindow");
				}
			}

			break;
		}
		case 4: {

			SDIRMANARGS *args = (SDIRMANARGS *) calloc(1, sizeof(SDIRMANARGS));
			args->option = 0;
			args->parent = hwnd;
			args->inum = this->getSingleSelID();
			// g_errorfStream << "inum: " << args->inum << std::flush;
			if (args->inum != 0) {
				SubDirManWindow::createWindowInstance(WinInstancer(0, L"Manage Subdirectories", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 100, 100, 500, 500, hwnd, nullptr, g_SharedWindowVar->ghInstance, (LPVOID) args));
			}
			free(args);
			break;
		}
		}
	} else {

	}
	return 0;
};

uint64_t MainIndexManWindow::getNumElems() {
	return getLastMINum();
}

LRESULT MainIndexManWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;

	this->pageListWin_ = nullptr;
	this->strListWin_ = nullptr;

	//# divergence 1
	// read args

	MIMANARGS *args = *((MIMANARGS **) procArgs.lParam);

	if (args) {
		this->winOptions_ = args->option;
		this->parent_ = args->parent;
	} else {
		this->winOptions_ = 0;
		this->parent_ = 0;
	}

	//# convergence 1

	RECT rect = {0};
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	//# divergence 2
	// create child windows

	if (this->winOptions_ != 2) {
		int32_t x = rect.right / 2 - 100;
		if (x < 5)
			x = 5;
		CreateWindowW(L"Button", L"Add...", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, nullptr, nullptr);
		SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);
		//CreateWindowW(L"Button", L"Relative path", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, x+90, 2, 110, 35, hwnd, (HMENU) 2, nullptr, nullptr);
		//SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) g_shared_window_vars.hFont2, 1);
		//CheckDlgButton(hwnd, 2, BST_UNCHECKED);
	} else {
		int32_t x = (rect.right-80) / 2;
		if (x < 5)
			x = 5;
		EnableWindow(this->parent_, 0);
		CreateWindowW(L"Button", L"Select", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, nullptr, nullptr);
		SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);
	}

	//# convergence 2

	HWND thwnd = nullptr;
	uint64_t ll = 0;
	int32_t i = 0;

	thwnd = PageListWindow::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 4, nullptr, nullptr));
	if (!thwnd) {
		errorf("failed to create PageListWindow");
		return -1;
	}

	this->pageListWin_ = std::dynamic_pointer_cast<PageListWindow>(WindowClass::getWindowPtr(thwnd));
	if (!this->pageListWin_) {
		errorf("failed to receive PageListWindow pointer");
		return -1;
	}

	thwnd = StrListWindow::createWindowInstance(WinInstancer(0,  0, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 3, nullptr, nullptr));
	if (!thwnd) {
		errorf("failed to create StrListWindow");
		return -1;
	}

	this->strListWin_ = std::dynamic_pointer_cast<StrListWindow>(WindowClass::getWindowPtr(thwnd));
	if (!this->strListWin_) {
		errorf("failed to receive StrListWindow pointer");
		return -1;
	}

	//! TODO: move this to the createWindowInstance arguments
	if (this->winOptions_ == 2) {
		this->strListWin_->winOptions_ = 1;
	}

	this->pageListWin_->curPage_ = 1;
	ll = this->getNumElems();
g_errorfStream << "getNumElems: " << ll << std::flush;
	this->pageListWin_->lastPage_ = ll/kMaxNRows + !!(ll % kMaxNRows);
	if (this->pageListWin_->lastPage_ == 0) {
		this->pageListWin_->lastPage_ = 1;
	}

	this->pageListWin_->startPaint();

	i = kMaxNRows/8 + !!(kMaxNRows%8);
	if (!i) {
		errorf("no space row selections");
	}

	//# divergence 3
	// initialize columns

	this->strListWin_->setNColumns(2);
	uint64_t fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;
	i = log10(fNum+kMaxNRows-1)+1;
	if ( !this->strListWin_->setColumnWidth(0, kCharWidth*(i)+6) ) {
		errorf("setColumnWidth failed (0)");
		return -1;
	}
	if ( !this->strListWin_->setColumnWidth(1, 0) ) {
		errorf("setColumnWidth failed (1)");
		return -1;
	}

	if ( !this->strListWin_->setHeaders(std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>( { "#", "Name" }))) ) {
		errorf("failed to set slv headers");
		return -1;
	}

	//# convergence 3

	SetFocus(GetDlgItem(hwnd, 3));

	SendMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);
errorf("miman 6");

	return ListManWindow::onCreate(procArgs);
}


LRESULT CALLBACK MainIndexManWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch(msg) {
		case WM_COMMAND: {

			if (this->winOptions_ != 2) {
				if (HIWORD(wParam) == 0 && lParam == 0) { // context menu
					//SendMessage(hwnd, WM_U_RET_SL, 3, LOWORD(wParam);
					this->menuUse(hwnd, LOWORD(wParam));
					break;
				} else if (HIWORD(wParam) == BN_CLICKED) {
					if (LOWORD(wParam) == 1) {	// add listing

						HWND *arg = (HWND*) malloc(sizeof(HWND));
						TextEditDialogWindow::createWindowInstance(WinInstancer(0, L"Enter Main Index Name", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 150, 150, 500, 500, hwnd, 0, nullptr, arg));
						//SendMessage(thwnd, WM_USER, 0, (LPARAM) this->dnum);
						free(arg);
					}
				}
			} else {
				if (HIWORD(wParam) == BN_CLICKED) {

					PostMessage(hwnd, WM_U_LISTMAN, kListmanRetSelPos, 0);

				}
			}

			break;
		}
		case WM_SIZE: {
			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			//# class specific child windows
			if (this->winOptions_ != 2) {
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
			//#
			} else {
				int32_t x = (rect.right-80) / 2;
				if (x < 5)
					x = 5;
				HWND thwnd = GetDlgItem(hwnd, 1);
				MoveWindow(thwnd, x, 5, 80, 25, 0);
			}

			{
				HWND thwnd = GetDlgItem(hwnd, 3);
				MoveWindow(thwnd, 0, kDManTopMargin, rect.right, rect.bottom-kDManTopMargin-(!!(this->pageListWin_->lastPage_ > 1))*kDManBottomMargin, 0);
			} {
				HWND thwnd = GetDlgItem(hwnd, 4);
				MoveWindow(thwnd, 0, rect.bottom-kDManBottomMargin+1, rect.right, (!!(this->pageListWin_->lastPage_ > 1))*kDManBottomMargin, 0);
			}

			break;
		}

		case WM_USER+1: { 		// refresh num and dir strings

			this->strListWin_->clearRows();

			uint64_t fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;

			//#
			// count filled rows
			std::shared_ptr<std::forward_list<std::string>> list_ptr = intervalMiRead(fNum, kMaxNRows);

			if (list_ptr != nullptr && !list_ptr->empty()) {
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

			//#

				int64_t gotRows = uint - fNum;

				if (gotRows <= 0) {
					errorf("list_ptr was somehow empty");
					return 1;
				} else if (gotRows > kMaxNRows) {
					errorf("returned too many entries");
					return 1;
				} else {
					this->strListWin_->assignRows(workPtr);

					//! TODO: figure this out
					// columnWidth probably shouldn't be when refreshing and instead only when changing page
					//this->strListWin_->setColumnWidth();

					//this->strListWin_->bpos[1] = x*CHAR_WIDTH+5;
				}
			} else {
				if (this->pageListWin_->curPage_ > 1) {
					this->pageListWin_->curPage_--;
					PostMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);
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
	}

	return this->ListManWindow::winProc(hwnd, msg, wParam, lParam);

	//return DefWindowProcW(hwnd, msg, wParam, lParam);
}

//} MainIndexManWindow

//{ DirManWindow
//public:
// static
const WindowHelper DirManWindow::helper = WindowHelper(std::wstring(L"DirManWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;
	}
);

// static
HWND DirManWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new DirManWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

int DirManWindow::menuCreate(HWND hwnd) {
	HMENU hMenu;

	if (this->winOptions_ != 2) {
		hMenu = CreatePopupMenu();
		AppendMenuW(hMenu, MF_STRING, 1, L"&Remove");	// if menu handles are changed, the message from WM_COMMAND needs to be changed to correspond to the right case
		AppendMenuW(hMenu, MF_STRING, 2, L"&Reroute");
		AppendMenuW(hMenu, MF_STRING, 3, L"&Reroute (RP)");

		TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, this->strListWin_->point_.x, this->strListWin_->point_.y, 0, hwnd, 0);
		DestroyMenu(hMenu);
	}

	return 0;
};

int DirManWindow::menuUse(HWND hwnd, int32_t menu_id) {
	g_errorfStream << "menu_id is: " << menu_id << std::flush;

	// TODO: make menu_id values to enum
	switch (menu_id) {
	case 1:
	case 2:
	case 3:
		if (this->winOptions_ != 2) {
			//fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;
			//DelRer(hwnd, menu_id, &this->strListWin_);
			SendMessage(hwnd, WM_USER, 0, 0);
			SendMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);
		}
		break;
	}

	return 0;
};

uint64_t DirManWindow::getNumElems() {
	return getLastDNum(this->inum_);
}

LRESULT DirManWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;

	this->pageListWin_ = nullptr;
	this->strListWin_ = nullptr;

errorf("DirManWindow onCreate spot 1");

	//# divergence 1

	DIRMANARGS *args = *(DIRMANARGS **) procArgs.lParam;

	if (args) {
		if (args->inum == 0) {
			errorf("args->inum is 0");
			return -1;
		} else {
			this->inum_ = args->inum;
		}

		this->winOptions_ = args->option;
		this->parent_ = args->parent;
	} else {
		errorf("args is nullptr");
		return -1;
	}

	//# convergence 1
	// read args
	
	RECT rect = {0};
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	//# divergence 2
	// create child windows

	if (this->winOptions_ != 2) {
		int32_t x = rect.right / 2 - 100;
		if (x < 5)
			x = 5;
		CreateWindowW(L"Button", L"Add...", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, nullptr, nullptr);
		SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);
		CreateWindowW(L"Button", L"Relative path", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, x+90, 2, 110, 35, hwnd, (HMENU) 2, nullptr, nullptr);
		SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);
		CheckDlgButton(hwnd, 2, BST_UNCHECKED);
	} else {
		int32_t x = (rect.right-80) / 2;
		if (x < 5)
			x = 5;
		EnableWindow(this->parent_, 0);
		CreateWindowW(L"Button", L"Select", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, nullptr, nullptr);
		SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);
	}

	//# convergence 2

	HWND thwnd = nullptr;
	uint64_t ll = 0;
	int32_t i = 0;

	thwnd = PageListWindow::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 4, nullptr, nullptr));
	if (!thwnd) {
		errorf("failed to create PageListWindow");
		return -1;
	}

	this->pageListWin_ = std::dynamic_pointer_cast<PageListWindow>(WindowClass::getWindowPtr(thwnd));
	if (!this->pageListWin_) {
		errorf("failed to receive PageListWindow pointer");
		return -1;
	}


errorf("DirManWindow before StrListWindow");
	thwnd = StrListWindow::createWindowInstance(WinInstancer(0,  0, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 3, nullptr, nullptr));
	if (!thwnd) {
		errorf("failed to create StrListWindow");
		return -1;
	}
errorf("DirManWindow after StrListWindow");

	this->strListWin_ = std::dynamic_pointer_cast<StrListWindow>(WindowClass::getWindowPtr(thwnd));
	if (!this->strListWin_) {
		errorf("failed to receive StrListWindow pointer");
		return -1;
	}

errorf("DirManWindow onCreate spot 4.1");

	//! TODO: move this to the createWindowInstance arguments
	if (this->winOptions_ == 2) {
		this->strListWin_->winOptions_ = 1;
	}
errorf("DirManWindow onCreate spot 4.2");

	this->pageListWin_->curPage_ = 1;
errorf("DirManWindow onCreate spot 4.2.1");
	ll = this->getNumElems();
errorf("DirManWindow onCreate spot 4.2.2");
	this->pageListWin_->lastPage_ = ll/kMaxNRows + !!(ll % kMaxNRows);
	if (this->pageListWin_->lastPage_ == 0) {
		this->pageListWin_->lastPage_ = 1;
	}
errorf("DirManWindow onCreate spot 4.3");

	this->pageListWin_->startPaint();

	i = kMaxNRows/8 + !!(kMaxNRows%8);
	if (!i) {
		errorf("no space row selections");
	}
errorf("DirManWindow onCreate spot 4.4");

	//# divergence 3
	// initialize columns

	this->strListWin_->setNColumns(2);
	uint64_t fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;
	i = log10(fNum+kMaxNRows-1)+1;
	if ( !this->strListWin_->setColumnWidth(0, kCharWidth*(i)+6) ) {
		errorf("setColumnWidth failed (0)");
		return -1;
	}
	if ( !this->strListWin_->setColumnWidth(1, 0) ) {
		errorf("setColumnWidth failed (1)");
		return -1;
	}

	if ( !this->strListWin_->setHeaders(std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>( {"#", "Path"}))) ) {
		errorf("failed to set slv headers");
		return -1;
	}
	//this->strListWin_->header = malloc(sizeof(char *)*this->strListWin_->nColumns);
	//this->strListWin_->header[0] = dupstr("#", 5, 0);
	//this->strListWin_->header[1] = dupstr("Path", 5, 0);

	//# convergence 3

	SetFocus(GetDlgItem(hwnd, 3));

errorf("DirManWindow onCreate spot 4.4");

	SendMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);

errorf("DirManWindow onCreate spot 9");

	return ListManWindow::onCreate(procArgs);
}

LRESULT CALLBACK DirManWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch(msg) {
		case WM_COMMAND: {

			if (this->winOptions_ != 2) {
				bool checkedRelative = !!(IsDlgButtonChecked(hwnd, 2));

				if (HIWORD(wParam) == 0 && lParam == 0) { // context menu
					this->menuUse(hwnd, LOWORD(wParam));
					break;
				} else if (HIWORD(wParam) == BN_CLICKED) {
					// TODO: this should be a method in the first place (or function)
					if (LOWORD(wParam) == 1) {	// add listing
						std::fs::path retPath = SeekDir(hwnd);

						if (!retPath.empty()) {

							if (checkedRelative) {
								//! TODO: add error return argument and print in window
								retPath = makePathRelativeToProgDir(retPath, g_fsPrgDir);

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
errorf("calling dReg");
								dReg(this->inum_, retPath);

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

					PostMessage(hwnd, WM_U_LISTMAN, kListmanRetSelPos, 0);

				}
			}

			break;
		}
		case WM_SIZE: {
			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			//# class specific child windows
			if (this->winOptions_ != 2) {
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
			//#
			} else {
				int32_t x = (rect.right-80) / 2;
				if (x < 5)
					x = 5;
				HWND thwnd = GetDlgItem(hwnd, 1);
				MoveWindow(thwnd, x, 5, 80, 25, 0);
			}

			{
				HWND thwnd = GetDlgItem(hwnd, 3);
				MoveWindow(thwnd, 0, kDManTopMargin, rect.right, rect.bottom-kDManTopMargin-(!!(this->pageListWin_->lastPage_ > 1))*kDManBottomMargin, 0);
			} {
				HWND thwnd = GetDlgItem(hwnd, 4);
				MoveWindow(thwnd, 0, rect.bottom-kDManBottomMargin+1, rect.right, (!!(this->pageListWin_->lastPage_ > 1))*kDManBottomMargin, 0);
			}

			break;
		}
		case WM_USER+1: {	// refresh num and dir strings

			this->strListWin_->clearRows();

			uint64_t fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;

			//#
			// count filled rows
			std::shared_ptr<std::forward_list<std::fs::path>> list_ptr = intervalDRead(this->inum_, (uint64_t) fNum, kMaxNRows);

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

			//#

				int64_t gotRows = uint - fNum;

				if (gotRows <= 0) {
					errorf("list_ptr was somehow empty");
					return 1;
				} else if (gotRows > kMaxNRows) {
					errorf("returned too many entries");
					return 1;
				} else {
					this->strListWin_->assignRows(workPtr);

					//! TODO: figure this out
					// columnWidth probably shouldn't be when refreshing and instead only when changing page
					//this->strListWin_->setColumnWidth();

					//this->strListWin_->bpos[1] = x*CHAR_WIDTH+5;
				}
			} else {
				if (this->pageListWin_->curPage_ > 1) {
					this->pageListWin_->curPage_--;
					PostMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);
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
	}

	return this->ListManWindow::winProc(hwnd, msg, wParam, lParam);

	//return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//} DirManWindow

//{ SubDirManWindow
//public:
// static
const WindowHelper SubDirManWindow::helper = WindowHelper(std::wstring(L"SubDirManWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;
	}
);

// static
HWND SubDirManWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new SubDirManWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

int SubDirManWindow::menuCreate(HWND hwnd) {
	HMENU hMenu;

	if (this->winOptions_ != 2) {
		hMenu = CreatePopupMenu();
		AppendMenuW(hMenu, MF_STRING, 1, L"&Remove");	// if menu handles are changed, the message from WM_COMMAND needs to be changed to correspond to the right case
		AppendMenuW(hMenu, MF_STRING, 2, L"&Reroute");
		AppendMenuW(hMenu, MF_STRING, 3, L"&Reroute (RP)");

		TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, this->strListWin_->point_.x, this->strListWin_->point_.y, 0, hwnd, 0);
		DestroyMenu(hMenu);
	}

	return 0;
};

int SubDirManWindow::menuUse(HWND hwnd, int32_t menu_id) {
	g_errorfStream << "menu_id is: " << menu_id << std::flush;

	switch (menu_id) {
	case 1:
	case 2:
	case 3:
		if (this->winOptions_ != 2) {
			//fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;
			//DelRer(hwnd, menu_id, &this->strListWin_);
			SendMessage(hwnd, WM_USER, 0, 0);
			SendMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);
		}
		break;
	}

	return 0;
};

uint64_t SubDirManWindow::getNumElems() {
	return getLastSubDirNum(this->inum_);
}

LRESULT SubDirManWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;

	this->pageListWin_ = nullptr;
	this->strListWin_ = nullptr;

	//# divergence 1
	// read args

	SDIRMANARGS *args = *(SDIRMANARGS **) procArgs.lParam;

	if (args->inum == 0) {
		errorf("inum is 0");
		DestroyWindow(hwnd);
		return 0;
	}

	if (args) {
		this->winOptions_ = args->option;
		this->parent_ = args->parent;
		this->inum_ = args->inum;
	} else {
		errorf("args is nullptr");
		//DestroyWindow(hwnd);
		//FreeWindowMem(hwnd);
		return -1;

		// this->winOptions_ = 0;
		// this->parent_ = 0;
	}

	//# convergence 1

	RECT rect;
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	//# divergence 2
	// create child windows

	if (this->winOptions_ != 2) {
		int32_t x = rect.right / 2 - 100;
		if (x < 5)
			x = 5;
		CreateWindowW(L"Button", L"Add...", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, nullptr, nullptr);
		SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);
		//CreateWindowW(L"Button", L"Relative path", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, x+90, 2, 110, 35, hwnd, (HMENU) 2, nullptr, nullptr);
		//SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) g_shared_window_vars.hFont2, 1);
		//CheckDlgButton(hwnd, 2, BST_UNCHECKED);
	} else {
		int32_t x = (rect.right-80) / 2;
		if (x < 5)
			x = 5;
		EnableWindow(this->parent_, 0);
		CreateWindowW(L"Button", L"Select", WS_VISIBLE | WS_CHILD , x, 5, 80, 25, hwnd, (HMENU) 1, nullptr, nullptr);
		SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);
	}

	//# convergence 2

	HWND thwnd = nullptr;
	uint64_t ll = 0;
	int32_t i = 0;

	thwnd = PageListWindow::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 4, nullptr, nullptr));
	if (!thwnd) {
		errorf("failed to create PageListWindow");
		return -1;
	}

	this->pageListWin_ = std::dynamic_pointer_cast<PageListWindow>(WindowClass::getWindowPtr(thwnd));
	if (!this->pageListWin_) {
		errorf("failed to receive PageListWindow pointer");
		return -1;
	}

	thwnd = StrListWindow::createWindowInstance(WinInstancer(0,  0, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU) 3, nullptr, nullptr));
	if (!thwnd) {
		errorf("failed to create StrListWindow");
		return -1;
	}

	this->strListWin_ = std::dynamic_pointer_cast<StrListWindow>(WindowClass::getWindowPtr(thwnd));
	if (!this->strListWin_) {
		errorf("failed to receive StrListWindow pointer");
		return -1;
	}

	//! TODO: move this to the createWindowInstance arguments
	if (this->winOptions_ == 2) {
		this->strListWin_->winOptions_ = 1;
	}

	this->pageListWin_->curPage_ = 1;
	ll = this->getNumElems();
	this->pageListWin_->lastPage_ = ll/kMaxNRows + !!(ll % kMaxNRows);
	if (this->pageListWin_->lastPage_ == 0) {
		this->pageListWin_->lastPage_ = 1;
	}

	this->pageListWin_->startPaint();

	i = kMaxNRows/8 + !!(kMaxNRows%8);
	if (!i) {
		errorf("no space row selections");
	}

	//# divergence 3
	// initialize columns

	//! TODO:
	this->strListWin_->setNColumns(2);
	uint64_t fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;
	i = log10(fNum+kMaxNRows-1)+1;
	if ( !this->strListWin_->setColumnWidth(0, kCharWidth*(i)+6) ) {
		errorf("setColumnWidth failed (0)");
		return -1;
	}
	if ( !this->strListWin_->setColumnWidth(1, 0) ) {
		errorf("setColumnWidth failed (1)");
		return -1;
	}

	if ( !this->strListWin_->setHeaders(std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>( {"#", "Path"} ))) ) {
		errorf("failed to set slv headers");
		return -1;
	}
	//# convergence 3

	SetFocus(GetDlgItem(hwnd, 3));

	SendMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);

	return ListManWindow::onCreate(procArgs);
}

LRESULT CALLBACK SubDirManWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	/*
	char *p;
	wchar_t *wp;
	int i, j, x;
	uint64_t ll, lastlastpage, fNum;
	HWND thwnd;
	oneslnk *link, *link2;
	*/

	switch(msg) {
		case WM_COMMAND: {

			if (this->winOptions_ != 2) {

				if (HIWORD(wParam) == 0 && lParam == 0) { // context menu
					//SendMessage(hwnd, WM_U_RET_SL, 3, LOWORD(wParam);
					this->menuUse(hwnd, LOWORD(wParam));
					break;
				//! TODO: move next to button creation
				} else if (HIWORD(wParam) == BN_CLICKED) {
					if (LOWORD(wParam) == 1) {	// add listing

						DIRMANARGS args = {};
						args.option = 2;
						args.inum = this->inum_;
						args.parent = hwnd;

						DirManWindow::createWindowInstance(WinInstancer(0, L"Select Superdirectory", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 100, 100, 500, 500, hwnd, nullptr, g_SharedWindowVar->ghInstance, (LPVOID) &args));
					}
				}
			} else {
				if (HIWORD(wParam) == BN_CLICKED) {

					PostMessage(hwnd, WM_U_LISTMAN, kListmanRetSelPos, 0);

				}
			}

			break;
		}
		case WM_SIZE: {
			RECT rect = {0};
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			if (this->winOptions_ != 2) {
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
				MoveWindow(thwnd, 0, kDManTopMargin, rect.right, rect.bottom-kDManTopMargin-(!!(this->pageListWin_->lastPage_ > 1))*kDManBottomMargin, 0);
			} {
				HWND thwnd = GetDlgItem(hwnd, 4);
				MoveWindow(thwnd, 0, rect.bottom-kDManBottomMargin+1, rect.right, (!!(this->pageListWin_->lastPage_ > 1))*kDManBottomMargin, 0);
			}


			break;
		}
		case WM_USER+1: {		// refresh num and dir strings

			this->strListWin_->clearRows();

			uint64_t fNum = (this->pageListWin_->curPage_-1)*kMaxNRows+1;

			//#
			std::shared_ptr<std::forward_list<std::fs::path>> list_ptr = intervalDRead(this->inum_, (uint64_t) fNum, kMaxNRows);

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
				} else if (gotRows > kMaxNRows) {
					errorf("returned too many entries");
					return 1;
				} else {
					this->strListWin_->assignRows(workPtr);

					//! TODO: figure this out
					// columnWidth probably shouldn't be when refreshing and instead only when changing page
					//this->strListWin_->setColumnWidth();

					//this->strListWin_->bpos[1] = x*CHAR_WIDTH+5;
				}
			} else {
				if (this->pageListWin_->curPage_ > 1) {
					this->pageListWin_->curPage_--;
					PostMessage(hwnd, WM_U_LISTMAN, kListManRefresh, 0);
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
	}

	return this->ListManWindow::winProc(hwnd, msg, wParam, lParam);

	//return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//} SubDirManWindow

//{ ThumbManWindow
//public:
// static
const WindowHelper ThumbManWindow::helper = WindowHelper(std::wstring(L"ThumbManWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW; // redraw on resize
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;
	}
);

// static
HWND ThumbManWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new ThumbManWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT ThumbManWindow::onCreate(WinProcArgs procArgs) {
/*

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
			this->winOptions_ = args->option;
			this->parent_ = args->parent;
			this->dnum = args->dnum;
		} else {
			return -1;
		}
	}

	this->dname = dRead(this->dnum);
	if (this->dname == 0) {
		errorf("dname came null");
		DestroyWindow(hwnd);
		return 0;
	}
	this->tfstr = 0;

	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	this->pageListWin_->curPage_ = 1;
	this->pageListWin_->lastPage_ = 1;

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

	if (getfilemodified(this->dname) >= getdlastchecked(this->dnum) || (p = fileRead(this->dnum, 1)) == nullptr) {
		dirfreg(this->dname, 1<<(5-1));
	} else {
		if (p)
			free(p);
	}
	this->tfstr = ffiReadTagExt(this->dnum, 0, IMGEXTS);	//! probably want to hide the window before loading all the images
	if (this->tfstr) {
		this->nitems = getframount(this->tfstr);
		this->pageListWin_->lastPage_ = this->nitems/MAX_NTHUMBS + !!(this->nitems % MAX_NTHUMBS);
	} else {
		this->pageListWin_->lastPage_ = 1;
	}

	ThumbListWindow::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, SBAR_H, rect.right, rect.bottom-(!!(this->pageListWin_->lastPage_ > 1))*DMAN_BOT_MRG - SBAR_H, hwnd, (HMENU) 3, nullptr, nullptr));
	PageListWindow::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->pageListWin_->lastPage_ > 1))*DMAN_BOT_MRG, hwnd, (HMENU) 4, nullptr, nullptr));
	SearchBarWindow::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD, 0, 0, rect.right, SBAR_H, hwnd, (HMENU) 6, nullptr, nullptr));
	SetFocus(GetDlgItem(hwnd, 3));

	SendMessage(hwnd, WM_USER+1, 0, 0);

*/


	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK ThumbManWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	/*
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

	switch(msg) {
		case WM_CREATE: {
	break;
		}
		case WM_SIZE: {
			if (!(this->winOptions_ & 1)) {
				thwnd = GetDlgItem(hwnd, 3);
				MoveWindow(thwnd, 0, SBAR_H, rect.right, rect.bottom-(!!(this->pageListWin_->lastPage_ > 1))*DMAN_BOT_MRG - SBAR_H, 0);
				thwnd = GetDlgItem(hwnd, 4);
				MoveWindow(thwnd, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->pageListWin_->lastPage_ > 1))*DMAN_BOT_MRG, 0);
				thwnd = GetDlgItem(hwnd, 6);
				MoveWindow(thwnd, 0, 0, rect.right, SBAR_H, 0);
			} else {
				thwnd = GetDlgItem(hwnd, 5);
				MoveWindow(thwnd, 0, 0, rect.right, rect.bottom, 0);
			}
			break;
		}
		case WM_PAINT: {
			hdc = BeginPaint(hwnd, &ps);

			hOldPen = (HPEN) SelectObject(hdc, g_shared_window_vars.hPen1);

			if (this->pageListWin_->lastPage_ > 1 && !(this->winOptions_ & 1)) {
				MoveToEx(hdc, 0, rect.bottom-DMAN_BOT_MRG, nullptr);
				LineTo(hdc, rect.right, rect.bottom-DMAN_BOT_MRG);
			}
			SelectObject(hdc, hOldPen);
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_MOUSEMOVE: {
			this->pageListWin_->hovered_ = 0;
			SetCursor(g_shared_window_vars.hDefCrs);

			break;
		}
		case WM_SETFOCUS: {
errorf("setting focus1");
			if (varp) {
				if (!(this->winOptions_ & 1)) {
					SetFocus(GetDlgItem(hwnd, 3));
				} else {
					SetFocus(GetDlgItem(hwnd, 5));
				}
			}

			break;
		}
		case WM_USER: {	refresh pages

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			lastlastpage = this->pageListWin_->lastPage_;
			j = 0, x = 0;

			if (this->tfstr) {
				this->nitems = getframount(this->tfstr);
				this->pageListWin_->lastPage_ = this->nitems/MAX_NTHUMBS + !!(this->nitems % MAX_NTHUMBS);
			} else
				this->pageListWin_->lastPage_ = 1;

			if (this->pageListWin_->curPage_ > this->pageListWin_->lastPage_)
				this->pageListWin_->curPage_ = this->pageListWin_->lastPage_, j = 1;

			if (this->pageListWin_->lastPage_ != lastlastpage) {
				SendDlgItemMessageW(hwnd, 4, WM_USER, 0, 0);
				if (j == 1) {
					SendMessage(hwnd, WM_USER+2, 1, this->pageListWin_->curPage_);
				} else {
					if (!(InvalidateRect(hwnd, 0, 1))) {
						errorf("InvalidateRect failed");
					}
				}
				this->pageListWin_->hovered_ = 0;
				SetCursor(g_shared_window_vars.hDefCrs);
			}

			break;
		}
		case WM_USER+1:		{		refresh fnum and file name strings and thumbnails

			if (this->tlv.strchn[0] != nullptr) {
				killoneschn(this->tlv.strchn[0], 1);
				this->tlv.strchn[0] = nullptr;
			}
			if (this->tlv.strchn[1] != nullptr) {
				killoneschn(this->tlv.strchn[1], 0);
				this->tlv.strchn[1] = nullptr;
			}
			if (this->tlv.thumb != 0) {
				for (i = 0; i > this->tlv.nThumbs; i++) {
					if (this->tlv.thumb[i] != 0) {
						DestroyImgF(this->tlv.thumb[i]);
					}
				} free(this->tlv.thumb), this->tlv.thumb = nullptr, this->tlv.nThumbs = 0;
			}

			fnum = (this->pageListWin_->curPage_-1)*MAX_NTHUMBS+1;

			if (this->tfstr) {
				if (1) {
					this->tlv.strchn[0] = ifrread(this->tfstr, (uint64_t) fnum, MAX_NTHUMBS); //! potentially modify it to have fnums and fnames in the same file

					for (link = this->tlv.strchn[0], i = 0; link != 0; link = link->next, i++);
					if (fnum + i - 1 > this->nitems || (this->pageListWin_->curPage_ == this->pageListWin_->lastPage_ && fnum + i - 1 != this->nitems)) {
						errorf("thumbman item amount mismatch");
					}

					if (this->tlv.strchn[0]) {
						this->tlv.strchn[1] = chainFileRead(this->dnum, this->tlv.strchn[0]);
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
		}
		case WM_USER+2: {

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
				fnum = (this->pageListWin_->curPage_-1)*MAX_NTHUMBS+1;

				break;
			}

			PostMessage(hwnd, WM_USER+1, 0, 0);

			break;
		}
		case WM_COMMAND: {

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
						g_errorfStream << "no link or string in this->tlv.strchn[1], i: " << i << std::flush;
						DestroyWindow(hwnd);
					}
					this->ivv.imgpath = malloc(strlen(this->dname)+1+strlen(link->str)+1);
					sprintf(this->ivv.imgpath, "%s\\%s", this->dname, link->str);

					for (i = 0, (link = this->tlv.strchn[0]) && link != 0; link && i < j; i++, link = link->next);
					if (!link) {
						g_errorfStream << "no link or string in this->tlv.strchn[0], j: " << j << std::flush;
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

					this->winOptions_ |= 1;
					ViewImageWindow::createWindowInstance(WinInstancer(0, L"View Image", WS_VISIBLE | WS_CHILD, 0, 0, rect.right, rect.bottom, hwnd, (HMENU) 5, nullptr, nullptr));
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

					thwnd = FileTagEditWindow::createWindowInstance(WinInstancer(0, L"Edit Tags", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP, 150, 150, 500, 500, hwnd, 0, nullptr, nullptr));
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
		}
		case WM_U_RET_TL: {
			switch (lParam) {
			case 0:
				return (LRESULT) &this->tlv;
			case 1:
			case 2:
			case 3:
				break;
			case 4:
				if (this->winOptions_ != 1) {
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
		}
		case WM_U_RET_PL: {
			switch (lParam) {
			case 0:
				return (LRESULT) &this->pageListWin_;
			case 1:
				SendMessage(hwnd, WM_USER+2, 1, 0);
				break;
			}
			break;
		}
		case WM_U_VI: {
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

				this->winOptions_ &= ~1;


				CreateWindowW(L"PageListWindow", 0, WS_VISIBLE | WS_CHILD, 0, rect.bottom-DMAN_BOT_MRG+1, rect.right, (!!(this->pageListWin_->lastPage_ > 1))*DMAN_BOT_MRG, hwnd, (HMENU) 4, nullptr, nullptr);
				CreateWindowW(L"ThumbListWindow", 0, WS_VISIBLE | WS_CHILD, 0, SBAR_H, rect.right, rect.bottom-(!!(this->pageListWin_->lastPage_ > 1))*DMAN_BOT_MRG - SBAR_H, hwnd, (HMENU) 3, nullptr, nullptr);
				CreateWindowW(L"SearchBarWindow", 0, WS_VISIBLE | WS_CHILD, 0, 0, rect.right, SBAR_H, hwnd, (HMENU) 6, nullptr, nullptr);

				SendMessage(hwnd, WM_USER+1, 0, 0);
				SetFocus(hwnd);

				break;
			}
			break;
		}
		case WM_U_SBAR: {

			if (lParam == 0) {
				p = (char *) wParam;

				if (this->tfstr)
					releasetfile(this->tfstr, 2);

				this->tfstr = ffiReadTagExt(this->dnum, p, IMGEXTS);

				if (this->tfstr) {
					this->nitems = getframount(this->tfstr);
					this->pageListWin_->lastPage_ = this->nitems/MAX_NTHUMBS + !!(this->nitems % MAX_NTHUMBS);
				} else {
					this->pageListWin_->lastPage_ = 1;
errorf("ffiReadTagExt no result");
				}

				this->pageListWin_->curPage_ = 1;
				SendDlgItemMessageW(hwnd, 4, WM_USER, 0, 0);

				SendMessage(hwnd, WM_USER+1, 0, 0);
			}

			break;
		}
		case WM_KEYDOWN: {
			switch(wParam) {
			case VK_BACK:
				PostMessage(this->parent_, WM_KEYDOWN, wParam, lParam);

				break;
			}
			break;
		}
		case WM_NCDESTROY: {
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

			thwnd = this->parent_;
			//FreeWindowMem(hwnd);
			if (thwnd) {
				EnableWindow(thwnd, 1);
				SendMessage(thwnd, WM_U_RET_TMAN, 0, (LPARAM) hwnd);
			}

			return 0;
		}
	}

	*/
	return DefWindowProcW(hwnd, msg, wParam, lParam);

}


//} ThumbManWindow

//{ PageListWindow
//public:
// static
const WindowHelper PageListWindow::helper = WindowHelper(std::wstring(L"PageListWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;
	}
);

// static
HWND PageListWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new PageListWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

bool PageListWindow::isPainting() {
	return doPaint_;
}

void PageListWindow::startPaint() {
	doPaint_ = true;
}

void PageListWindow::pausePaint() {
	doPaint_ = false;
}

// TODO: separate writing the buttons vec from calculating the numbers
void PageListWindow::getPages(int32_t edgeSpace) {
	// the selected page's button is centered unless buttons for all pages fit or the buttons left of a centered button would reach the left edge (that is no left arrow buttons are required) or reach the right edge to the right (no right arrows)

	// 2^64 has 19 decimals
	const int32_t kMaxDecimals = 19;

	int32_t endRemainder, midRemainder;
	int32_t tailLen, headLen;

	this->pageListPtr_ = nullptr;
	std::shared_ptr<std::vector<ListPage>> result;

	if (lastPage_ == 1 || (curPage_ > lastPage_) || (curPage_ == 0)) {
		return;
	}

	{
		headLen = ((1+1)*kCharWidth+2*(kPagesSpace+kPagePad)); // space for the left arrow and '1' button
		uint16_t decimals3 = (uint16_t) log10(lastPage_)+1;
		tailLen = (1 + decimals3) * kCharWidth+2 * (kPagesSpace+kPagePad); // space for the right arrow and last button

		midRemainder = (edgeSpace + ((uint16_t)(log10(curPage_)+1)*kCharWidth + kPagePad)) / 2 - kPagesSideBuf + kPagesSpace; // space to fit curpage button in middle
		endRemainder = edgeSpace - kPagesSideBuf*2 + kPagesSpace;	// the first button doesn't have space at the front
		if (endRemainder - tailLen < midRemainder) {
			midRemainder = endRemainder - tailLen; // space to fit curpage button if tail is included
		}
		if (endRemainder < 0) {
			endRemainder = 0;
		} if (midRemainder < 0) {
			midRemainder = 0;
		}
	}

	boolean statusFitRight = false;
	boolean statusFitLeft = false;
	uint64_t pos = 1;
	uint64_t pos2 = 0;
	// count how many buttons fit within edgeSpace without arrow buttons -- also test whether the currents page's button fits before middle (in which case no left arrow buttons are rendered)
	{
		uint64_t lastNum1, lastNum2;
		boolean statusPastCurPage = false;

		uint16_t decimals, decimals2;
	
		do {
			decimals = (uint16_t) log10(pos)+1;
			// values: 9, 99, 999, 9999, ...
			lastNum1 = pow(10, decimals)-1;

			int32_t totalCharWidth = decimals*kCharWidth;
			int32_t totalButtonWidth1 = kPagesSpace + totalCharWidth + kPagePad;

			// don't overwrite in further loops
			if (!statusPastCurPage) {
				// determine if curPage is reachable in the remaining space
				if (lastNum1 >= curPage_) {
					statusPastCurPage = true;
					uint64_t nMidPages = midRemainder / totalButtonWidth1;
					uint64_t endMidPos =  pos - 1 + nMidPages;

					if (endMidPos >= curPage_) {
						midRemainder = endRemainder - tailLen;
						pos2 = pos;
						lastNum2 = lastNum1;

						// determine last number and remaining space when with tail
						do {
							decimals2 = (uint16_t) log10(pos2)+1;
							lastNum2 = pow(10, decimals2)-1;
							int32_t totalCharWidth2 = decimals2*kCharWidth;
							int32_t totalButtonWidth2 = totalCharWidth2 + kPagePad + kPagesSpace;
							int64_t widthToPos2 = (lastNum2 - pos2 + 1) * totalButtonWidth2;
							
							if ( widthToPos2 >= midRemainder) {
								// the last page that fits -- pos-1 to account for the space the first page take
								uint64_t nPages2 = midRemainder / totalButtonWidth2;
								pos2 = pos2 - 1 + nPages2;
								break;

							// if midRemainder is larger, it can be decremented to indicate the remaining space
							} else {
								midRemainder -= widthToPos2;
								pos2 = lastNum2+1;
							}
						} while (decimals2 < kMaxDecimals);

						statusFitLeft = true;

					} else if (curPage_ == 1) {
						pos2 = 1;
					}
				}
			}
												
			if (lastNum1 >= lastPage_) {
				uint64_t nEndPages = endRemainder / totalButtonWidth1;
				uint64_t endPos = pos + nEndPages;

				// at least one page has to fit (e.g. pos == lastPage_)
				if (endPos > lastPage_) {
					// minus width to last page
					endRemainder -= (lastPage_ - pos + 1) * totalButtonWidth1;
					statusFitRight = true;
				// if there is only one page, don't create arrows to the right
				} else if (lastPage_ == 1) {
					endRemainder = 0;
					statusFitRight = true;
				} break;
			}

			int64_t widthToPos1 = (lastNum1 - pos + 1) * totalButtonWidth1;
			if (widthToPos1 >= endRemainder) {
				break;
			} else {
				endRemainder -= widthToPos1;

				if (!statusFitLeft) {
					midRemainder -= widthToPos1;
				}
			}

			pos = lastNum1+1;

		} while (decimals < kMaxDecimals);
	}
	
	// if buttons for all pages fit (no arrow buttons)
	if (statusFitRight) {
		int32_t xPos = endRemainder/2 + kPagesSideBuf;	// starting coordinate

		// TODO: place in function
		{
			result = std::shared_ptr<std::vector<ListPage>>(new std::vector<ListPage>(lastPage_));
			if (result == nullptr) {
				errorf("failed to allocate shared_ptr");
				return;
			}
			auto &vec = *result;
			for (pos = 1; pos <= lastPage_; pos++) {
				vec[pos-1].type = 0;
				vec[pos-1].str = utoc(pos);
				vec[pos-1].left = xPos;
				int32_t totalCharWidth = ( ((uint16_t) log10(pos)) + 1) * kCharWidth;
				uint16_t totalButtonWidth = totalCharWidth + kPagePad;
				vec[pos-1].right = xPos + totalButtonWidth;
				xPos += totalButtonWidth + kPagesSpace;
			}
			vec[curPage_-1].type |= kPlbTypeCurrent;
			vec[lastPage_-1].type |= kPlbTypeLast;

			this->pageListPtr_ = result;
		}

	// if the selected page's button fits before middle when starting from 1 (no left arrow buttons)
	} else if (statusFitLeft || curPage_ == 1) {
		endRemainder = kPagesSideBuf;
		if ((pos2 == 0) || !(statusFitLeft)) { // cur page is 1, but doesn't fit
			pos2 = 1;
		}

		// TODO: place in function
		{
			result = std::shared_ptr<std::vector<ListPage>>(new std::vector<ListPage>( (pos2+2) ));

			auto &vec = *result;

			for (pos = 1; pos <= pos2; pos++) {
				vec[pos-1].type = 0;
				vec[pos-1].str = utoc(pos);
				vec[pos-1].left = endRemainder;
				vec[pos-1].right = endRemainder+(((uint16_t) log10(pos)+1)*kCharWidth+kPagePad);
				endRemainder += ((uint16_t) log10(pos)+1)*kCharWidth+kPagePad+kPagesSpace;
			}
			vec[curPage_-1].type |= kPlbTypeCurrent;
			vec[pos-1].type = 0;
			vec[pos-1].str = (char *) malloc(2), vec[pos-1].str[0] = '>', vec[pos-1].str[1] = '\0';
			vec[pos-1].left = endRemainder;
			vec[pos-1].right = endRemainder+kCharWidth+kPagePad;
			endRemainder += kCharWidth+kPagePad+kPagesSpace;
			vec[pos].type = kPlbTypeLast;
			vec[pos].str = utoc(lastPage_);
			vec[pos].left = endRemainder;
			vec[pos].right = endRemainder+(((uint16_t) log10(lastPage_)+1)*kCharWidth+kPagePad);

			this->pageListPtr_ = result;
		}

	} else {
		statusFitRight = false;
		statusFitLeft = false;

		int32_t selStart = (edgeSpace - ((uint16_t)(log10(curPage_)+1)*kCharWidth + kPagePad)) / 2;		// space before curpage button; not casting to uint16_t caused it to go off-center
		if (selStart < kPagesSideBuf + headLen + kPagesSpace) {		// ensure end part doesn't overextend
		//! TODO: also needs room for the space before (I think)(test later)(or is PAGES_SPACE extra if included in head)
			selStart = kPagesSideBuf + headLen + kPagesSpace;
		}
		pos = curPage_;
		endRemainder = edgeSpace - selStart - kPagesSideBuf + kPagesSpace;
		if (endRemainder < 0) {
			endRemainder = 0;
		}

		{
			uint64_t lastNum1, lastNum2;
			uint16_t decimals, decimals2;

			// counts how many buttons fit before right arrow buttons if the current page's button is rendered at the center -- also test whether the last page number can be reached before right edge
			do {
				decimals = (uint16_t) log10(pos)+1;
				lastNum1 = pow(10, decimals)-1;

				if (!statusFitLeft) {
					if ( (int64_t)(lastNum1-pos+1)*(decimals*kCharWidth+kPagePad+kPagesSpace) >= endRemainder-tailLen) {
						midRemainder = endRemainder-tailLen;
						pos2 = pos, lastNum2 = lastNum1;
						do { // determine last num before tail
							decimals2 = (uint16_t) log10(pos2)+1;
							lastNum2 = pow(10, decimals2)-1;
							if ( (int64_t)(lastNum2-pos2+1)*(decimals2*kCharWidth+kPagePad+kPagesSpace) >= midRemainder) {
								pos2 = pos2 - 1 + (midRemainder) / (decimals2*kCharWidth+kPagePad+kPagesSpace);
								break;
							} else {
								midRemainder -= (lastNum2-pos2+1)*(decimals2*kCharWidth+kPagePad+kPagesSpace);
								pos2 = lastNum2+1;
							}
						} while (decimals2 < 100);

						if (pos2 < curPage_) {	// too small to fit even the selected page
							pos2 = curPage_;
						}
						statusFitLeft = true;
					}
				}
				if (lastNum1 >= lastPage_) {
					if (pos + endRemainder / (decimals*kCharWidth+kPagePad+kPagesSpace) > lastPage_) {
						endRemainder -= (lastPage_-pos+1)*(decimals*kCharWidth+kPagePad+kPagesSpace);
						statusFitRight = true;
					} break;
				}
				int32_t i1 = (lastNum1 - pos + 1) * (decimals*kCharWidth+kPagePad+kPagesSpace);
				if ( i1 >= endRemainder) {
					break;
				} else {
					endRemainder -= (lastNum1-pos+1)*(decimals*kCharWidth+kPagePad+kPagesSpace);
				}
				pos = lastNum1+1;
			} while (decimals < kMaxDecimals);		// if it can fit 0, if it's the last page
		}

		// fit up to the right edge
		if (statusFitRight) {
			endRemainder = selStart + endRemainder - kPagesSideBuf - headLen;		// an extra PAGES_SPACE is already accounted for in selStart since it's up to the actual start after the space
			if (endRemainder < 0)
				endRemainder = 0;
			pos2 = lastPage_;
		} else {
			if (!(statusFitLeft)) {
				statusFitLeft = true;
				pos2 = curPage_;
			}
			endRemainder = selStart - kPagesSideBuf - headLen;
			if (endRemainder < 0)
				endRemainder = 0;
		}

		// count how many buttons fit before left arrow with the current page's button rendered at the center -- the left edge is assumed unreachable before button for number '1' (since otherwise this else should not have been reached)
		pos = curPage_;

		{
			uint64_t lastNum1;
			uint16_t decimals;

			do {
				decimals = (uint16_t) log10(pos-1)+1; 	// the actual pos doesn't need to fit
				lastNum1 = pow(10, decimals-1);

				int32_t i1 = (pos-lastNum1)*(decimals*kCharWidth+kPagePad+kPagesSpace);
				if ( i1 >= endRemainder) {
					lastNum1 = pos;
					pos = pos - endRemainder / (decimals*kCharWidth+kPagePad+kPagesSpace);
					endRemainder -= (lastNum1-pos)*(decimals*kCharWidth+kPagePad+kPagesSpace);
					break;
				} else {
					endRemainder -= (pos-lastNum1)*(decimals*kCharWidth+kPagePad+kPagesSpace);
					pos = lastNum1;
				}
			} while (decimals > 1);
		}

		// TODO: place in function
		{
			int32_t i1 = 2 + (pos2-pos+1) + 2 * (!statusFitRight);
			result = std::shared_ptr<std::vector<ListPage>>(new std::vector<ListPage>(i1));

			auto &vec = *result;

			endRemainder += kPagesSideBuf;

			vec[0].type = 0;
			vec[0].str = utoc(1);
		//		vec[0].str = malloc(3), vec[0].str[0] = '<', vec[0].str[1] = '<', vec[0].str[2] = '\0';
			vec[0].left = endRemainder;
		//		vec[0].right = endRemainder+2*CHAR_WIDTH+PAGE_PAD;
			vec[0].right = endRemainder+1*kCharWidth+kPagePad;
			endRemainder += 2*kCharWidth+kPagePad+kPagesSpace;
			vec[1].type = 0;
			vec[1].str = (char *) malloc(2), vec[1].str[0] = '<', vec[1].str[1] = '\0';
			vec[1].left = endRemainder;
			vec[1].right = endRemainder+kCharWidth+kPagePad;
			endRemainder += kCharWidth+kPagePad+kPagesSpace;
			int i = pos;

			uint64_t lastNum1 = 2;
			for (; pos <= pos2; pos++, lastNum1++) {
				vec[lastNum1].type = 0;
				vec[lastNum1].str = utoc(pos);
				vec[lastNum1].left = endRemainder;
				vec[lastNum1].right = endRemainder+(((uint16_t) log10(pos)+1)*kCharWidth+kPagePad);
				endRemainder += ((uint16_t) log10(pos)+1)*kCharWidth+kPagePad+kPagesSpace;
			}

			if (!statusFitRight) {
				vec[lastNum1].type = 0;
				vec[lastNum1].str = (char *) malloc(2), vec[lastNum1].str[0] = '>', vec[lastNum1].str[1] = '\0';
				vec[lastNum1].left = endRemainder;
				vec[lastNum1].right = endRemainder+kCharWidth+kPagePad;
				endRemainder += kCharWidth+kPagePad+kPagesSpace;
				lastNum1++;
				vec[lastNum1].type = 0;
				vec[lastNum1].str = utoc(lastPage_);
	//			vec[lastNum1].str = malloc(3), vec[lastNum1].str[0] = '>', vec[lastNum1].str[1] = '>', vec[lastNum1].str[2] = '\0';
				vec[lastNum1].left = endRemainder;
	//			vec[lastNum1].right = endRemainder+2*CHAR_WIDTH+PAGE_PAD;
				vec[lastNum1].right = endRemainder+(((uint16_t) log10(lastPage_)+1)*kCharWidth+kPagePad);
				lastNum1++;
			}

			lastNum1--;
			vec[lastNum1].type |= kPlbTypeLast;
			vec[curPage_-i+2].type |= kPlbTypeCurrent;

			this->pageListPtr_ = result;
		}
	}
}

LRESULT PageListWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;


	//! TODO: maybe initialize in class instead

	RECT rect;
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	this->hovered_ = 0;
	this->pageListPtr_ = nullptr;
	this->curPage_ = 0;
	this->lastPage_ = 0;

	if (this->isPainting()) {
		this->getPages(rect.right);
	}

	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK PageListWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_SIZE:
		case WM_USER:
		{
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (this->isPainting()) {
				this->getPages(rect.right);
			}
			break;
		}
		case WM_PAINT: {

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			HDC hdc;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hwnd, &ps);

			HPEN hOldPen;
			HBRUSH tempBrush, hOldBrush;
			HFONT hOldFont;

			hOldPen = (HPEN) SelectObject(hdc, g_SharedWindowVar->bgPen1);
			hOldFont = (HFONT) SelectObject(hdc, g_SharedWindowVar->hFont);
			hOldBrush = (HBRUSH) SelectObject(hdc, g_SharedWindowVar->bgBrush1);
			Rectangle(hdc, 0, 0, rect.right, rect.bottom);
			SelectObject(hdc, g_SharedWindowVar->hPen1);
			SetBkMode(hdc, TRANSPARENT);
			tempBrush = CreateSolidBrush(DL_BORDER_COL);

/*			MoveToEx(hdc, PAGES_SIDEBUF, rect.bottom-DMAN_BOT_MRG, nullptr);
			LineTo(hdc, PAGES_SIDEBUF, rect.bottom);
			MoveToEx(hdc, rect.right/2, rect.bottom-DMAN_BOT_MRG, nullptr);
			LineTo(hdc, rect.right/2, rect.bottom);
			MoveToEx(hdc, rect.right-PAGES_SIDEBUF-1, rect.bottom-DMAN_BOT_MRG, nullptr);
			LineTo(hdc, rect.right-PAGES_SIDEBUF-1, rect.bottom);
*/

			if (this->pageListPtr_ != nullptr) {
				auto &pageList = *pageListPtr_;
				for (int32_t i = 0; ; i++) {
					if (i == this->hovered_-1) {
						SelectObject(hdc, tempBrush);
						if (pageList[i].type & 1) {
							SelectObject(hdc, g_SharedWindowVar->hFontUnderlined);
						}
					}
					if (!(pageList[i].type & 1)) {
						Rectangle(hdc, pageList[i].left, rect.bottom-18, pageList[i].right, rect.bottom-2);
					}
					TextOutA(hdc, pageList[i].left+kPagePad/2, rect.bottom-17, pageList[i].str, strlen(pageList[i].str));
					if (i == this->hovered_-1) {
						SelectObject(hdc, g_SharedWindowVar->bgBrush1);
						SelectObject(hdc, g_SharedWindowVar->hFont);
					}
					if (pageList[i].type & 2)
						break;
					if ( ((uint64_t) i ) > this->lastPage_+4) {
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
				RECT rect;
				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}
				int hvrdbefore = this->hovered_;
				this->hovered_ = 0;

				if (this->pageListPtr_) {
					auto &pageList = *pageListPtr_;
					int32_t i2 = lParam/256/256;
					if ( (i2 >= rect.bottom-18) && (i2 < rect.bottom-2)) {
						int temp2 = lParam % (256*256);
						for (int32_t i = 0; ; i++) {
//							if (!(pageList[i].type & 1)) {
								if ((temp2 >= pageList[i].left) && (temp2 <= pageList[i].right)) {
									this->hovered_ = i+1;
									break;
								}
//							}
							if (pageList[i].type & 2) {
								break;
							}
						}
					}
				}

				if (this->hovered_ != hvrdbefore) {
					if (!(InvalidateRect(hwnd, 0, 0))) {
						errorf("InvalidateRect failed");
					}
				}

				if (this->hovered_) {
					SetCursor(g_SharedWindowVar->hCrsHand);
				} else {
					SetCursor(g_SharedWindowVar->hDefCrs);
				}
			}
			break;
		}
		case WM_LBUTTONDOWN: {

			if (!this->pageListPtr_) {
				break;
			}
			auto &pageList = *pageListPtr_;

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			int32_t i2 = lParam/256/256;
			if ( (i2 >= rect.bottom-18) && (i2 < rect.bottom-2)) {
				int temp2 = lParam % (256*256);

				for (int32_t i = 0; ; i++) {
					if (!(pageList[i].type & 1)) {
						if ((temp2 >= pageList[i].left)	&& (temp2 <= pageList[i].right)) {
							if (pageList[i].str[0] == '<') {
								if (pageList[i].str[1] == '<')
									this->curPage_ = 1;
								else
									this->curPage_--;
							} else if (pageList[i].str[0] == '>') {
								if (pageList[i].str[1] == '>')
									this->curPage_ = this->lastPage_;
								else
									this->curPage_++;
							} else {
								this->curPage_ = ctou(pageList[i].str);
							}

							for (int32_t j = 0; ; j++) {
								if (pageList[j].str != nullptr) {
									free(pageList[j].str);
									pageList[j].str = nullptr;
								}
								if (pageList[j].type & 2)
									break;
							}
							this->getPages(rect.right);
							this->hovered_ = 0;
							SetCursor(g_SharedWindowVar->hDefCrs);
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
				case kPL_Refresh: {
					RECT rect;
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


//} PageListWindow

//{ StrListWindow
//public:
// static
const WindowHelper StrListWindow::helper = WindowHelper(std::wstring(L"StrListWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;
	}
);

// static
HWND StrListWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new StrListWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

int64_t StrListWindow::getSingleSelPos(void) const {
	if (this->getNRows() <= 0) {
		errorf("getSingleSelPos getNRows <= 0");
		return -1;
	}
	int64_t gotNRows = this->getNRows();

	if (this->rowSelsPtr_ == nullptr) {
		errorf("getSingleSelPos rowSelsPtr was null");
		return -1;
	}
	auto selsPtrHolder = this->rowSelsPtr_;

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

std::shared_ptr<std::vector<std::string>> StrListWindow::getSingleSelRowCopy(void) const {
	int64_t pos = this->getSingleSelPos();

	if (pos < 0) {
		return {};
	}

	std::shared_ptr<std::vector<std::string>> rowCopyPtr( new std::vector<std::string>((*this->rowsPtr_)[pos]) );

	return rowCopyPtr;
}

std::string StrListWindow::getSingleSelRowCol(uint32_t argCol, ErrorObject *retError) const {
errorf("gssrc spot 0");
	if ((int64_t) argCol >= this->getNColumns()) {
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

	uint64_t upos = pos;

errorf("gssrc spot 5");
g_errorfStream << "argCol is: " << argCol << std::flush;
	if (rowsPtr_ == nullptr) {
		errorf("rowsPtr_ was null");
		setErrorPtr(retError, 1);
		return {};
	}
errorf("gssrc spot 5.1");

	if (rowsPtr_->size() <= upos) {
		g_errorfStream << "rowsVector too small: tried to access [" << upos << "] size is [" << rowsPtr_->size() << "]" << std::flush; 
		setErrorPtr(retError, 1);
		return {};
	}
errorf("gssrc spot 5.2");

	if ((*rowsPtr_)[upos].size() <= argCol) {
		g_errorfStream << "column vector too small: tried to access [" << argCol << "] size is [" << (*rowsPtr_)[pos].size() << "]" << std::flush;
		setErrorPtr(retError, 1);
		return {};
	}
errorf("gssrc spot 5.3");

	g_errorfStream << "result string is: " << (*rowsPtr_)[upos][argCol] << std::flush;

errorf("gssrc spot 6");

	return (*this->rowsPtr_)[upos][argCol];
}


std::shared_ptr<std::forward_list<int64_t>> StrListWindow::getSelPositions(void) const {
	if (this->getNRows() <= 0) {
		errorf("getSelPositions getNRows <= 0");
		return {};
	}
	int64_t gotNRows = this->getNRows();

	if (this->rowSelsPtr_ == nullptr) {
		errorf("getSelPositions rowSelsPtr was null");
		return {};
	}
	auto selsPtrHolder = this->rowSelsPtr_;

	auto &rowSels = *selsPtrHolder;

	if (this->rowsPtr_ == nullptr) {
		errorf("getSelPositions rowsPtr was null");
		return {};
	}
	auto rowsPtrHolder = this->rowsPtr_;

	auto &rows = *this->rowsPtr_;

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

bool StrListWindow::setNColumns(int32_t argI) {
	if (argI < 0) {
		errorf("tried to set nColumns to less than 0");
		return false;
	}

	this->nColumns_ = argI;

	sectionHeadersPtr_ = nullptr;
	rowsPtr_ = nullptr;
	rowSelsPtr_ = nullptr;

	columnWidthsPtr_ = std::shared_ptr<std::vector<int16_t>>( new std::vector<int16_t>(argI) );

	return true;

}

bool StrListWindow::setHeaders(std::shared_ptr<std::vector<std::string>> argHeaderPtr) {
	if (!argHeaderPtr) {
		errorf("setHeaders received nullptr");
		return false;
	}

	auto &headerVec = *argHeaderPtr;

	if (headerVec.size() != (uint64_t) this->getNColumns()) {
		errorf("setHeaders vector size doesn't match nColumns");
		return false;
	} else {
		this->sectionHeadersPtr_ = argHeaderPtr;
		return true;
	}
}

bool StrListWindow::setColumnWidth(int32_t colArg, int32_t width) {
	auto columnWidthsHoldPtr = this->columnWidthsPtr_;
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
	if (width < this->kMinColWidth) {
		colWidths[colArg] = this->kMinColWidth;
	} else if (width > this->kMaxColWidth) {
		colWidths[colArg] = this->kMaxColWidth;
	}

	return true;

}

int32_t StrListWindow::getNColumns(void) const {
	return this->nColumns_;
}

int32_t StrListWindow::getTotalColumnsWidth(void) const {
	auto holdPtr = this->columnWidthsPtr_;
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

bool StrListWindow::clearRows(void) {
	this->rowsPtr_ = nullptr;

	// clear selections as well
	bool b_clearRes = this->clearSelections();

	return true;
}

bool StrListWindow::assignRows(std::shared_ptr<std::vector<std::vector<std::string>>> inputVectorPtr) {
	if (!inputVectorPtr) {
		this->rowsPtr_ = nullptr;
		this->rowSelsPtr_ = nullptr;
		this->nRows_ = 0;

		return true;
	} else {
		auto &inputVector = *inputVectorPtr;

		int32_t gotNColumns = this->getNColumns();

		for (auto &row : inputVector) {
			if (row.size() != (uint64_t) gotNColumns) {
				errorf("input row had mismatching columns");
				this->assignRows(nullptr);
				return false;
			}
		}

		this->rowsPtr_ = inputVectorPtr;
		this->nRows_ = inputVector.size();
		this->rowSelsPtr_.reset( new boost::dynamic_bitset<>(this->nRows_) );

		return true;
	}
}

int64_t StrListWindow::getNRows(void) const {
	return this->nRows_;
}

bool StrListWindow::clearSelections(void) {
	auto selsHoldPtr = this->rowSelsPtr_;

	if (selsHoldPtr) {
		auto &rowSels = *selsHoldPtr;
		rowSels.reset();
		return true;
	}
	errorf("StrListWindow::clearSelections no rowSelsPtr");
	return false;
}

LRESULT StrListWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	LPARAM &lParam = procArgs.lParam;
	WPARAM &wParam = procArgs.wParam;

errorf("strlistclass create 1");
	this->yspos_ = this->xspos_ = 0;
	this->hvrd_ = 0;
	this->isDragged_ = 0;
	this->isTimed_ = 0;
	this->lastSel_ = 0;

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
	sinfo.nMax = kRowHeight*this->nRows_-1;		// since it starts from 0 ROW_HEIGHT*nRows would be the first pixel after the area
	sinfo.nPage = rect.bottom-kStrListTopMargin;
	sinfo.nPos = this->yspos_;
	SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

	if (!(ShowScrollBar(hwnd, SB_VERT, 1))) {
		errorf("ShowScrollBar failed.");
	}

	int j = this->getTotalColumnsWidth();
	sinfo.nMax = j-1;	// the first column has its first pixel off-screen
	sinfo.nPage = rect.right;
	sinfo.nPos = this->xspos_;
	SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

errorf("strlistclass create 9");


	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK StrListWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

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
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_PAINT: {

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			HDC hdc;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hwnd, &ps);

			HDC hdc2;

			hdc2 = CreateCompatibleDC(hdc);

			HPEN hOldPen;
			hOldPen = (HPEN) GetCurrentObject(hdc2, OBJ_PEN);
			
			HBRUSH hOldBrush;
			hOldBrush = (HBRUSH) GetCurrentObject(hdc2, OBJ_BRUSH);

			HFONT hOldFont;
			hOldFont = (HFONT) SelectObject(hdc2, g_SharedWindowVar->hFont);
			SetBkMode(hdc2, TRANSPARENT);

			int64_t firstRow = this->yspos_/kRowHeight;
			// upOffset is the number of pixels outside the window in the first row; ROW_HEIGHT-upOffset is the number of pixels in the first row
			int32_t upOffset = this->yspos_%kRowHeight;

			if (!g_SharedWindowVar->hListSliceBM) {
				int64_t j = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				if (j == 0) {
					j = 1920;
				}
				g_SharedWindowVar->hListSliceBM = CreateCompatibleBitmap(hdc, j, kRowHeight);
			}
			HBITMAP hOldBM;
			hOldBM = (HBITMAP) SelectObject(hdc2, g_SharedWindowVar->hListSliceBM);

			int64_t lastRow = firstRow + (!!upOffset) + (rect.bottom-kStrListTopMargin-(kRowHeight-upOffset)%kRowHeight)/kRowHeight + (((rect.bottom-kStrListTopMargin-(kRowHeight - upOffset) % kRowHeight)) > 0) - 1;
			// (rect.bottom-STRLIST_TOP_MRG-(ROW_HEIGHT-upOffset)%ROW_HEIGHT)/ROW_HEIGHT
			// = (rect.bottom-STRLIST_TOP_MRG-(ROW_HEIGHT-upOffset))/ROW_HEIGHT + !upOffset
			// = (rect.bottom-STRLIST_TOP_MRG + upOffset))/ROW_HEIGHT + !upOffset - 1
			// !!upOffset + !upOffset - 1 = 0
			//! TODO: uncomment and test
			// int64_t lastRow = firstRow - 1 + (rect.bottom-STRLIST_TOP_MRG + upOffset))/ROW_HEIGHT + !!((rect.bottom-STRLIST_TOP_MRG + upOffset) % ROW_HEIGHT);

			auto rowSelsHoldPtr = this->rowSelsPtr_;
			auto rowsHoldPtr = this->rowsPtr_;
			auto columnWidthsHoldPtr = this->columnWidthsPtr_;
			auto headersHoldPtr = this->sectionHeadersPtr_;



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
					w1 -= this->xspos_;
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
						SelectObject(hdc2, g_SharedWindowVar->selPen);
						SelectObject(hdc2, g_SharedWindowVar->selBrush);
						sel = true;
					// row in uneven position
					} else if (uneven) {
						if (!(col % 2)) {
							SelectObject(hdc2, g_SharedWindowVar->bgPen2);
							SelectObject(hdc2, g_SharedWindowVar->bgBrush2);
						} else {
							SelectObject(hdc2, g_SharedWindowVar->bgPen4);
							SelectObject(hdc2, g_SharedWindowVar->bgBrush4);
						}
					// row in even position
					} else {
						if (!(col % 2)) {
							SelectObject(hdc2, g_SharedWindowVar->bgPen3);
							SelectObject(hdc2, g_SharedWindowVar->bgBrush3);
						} else {
							SelectObject(hdc2, g_SharedWindowVar->bgPen5);
							SelectObject(hdc2, g_SharedWindowVar->bgBrush5);
						}
					}
					Rectangle(hdc2, w1, 0, w2 + (sel && !(col+1 == gotNColumns)), kRowHeight);
					if (sel && col > 0) {
						SelectObject(hdc2, g_SharedWindowVar->selPen2);
						MoveToEx(hdc2, w1, 1, nullptr);
						LineTo(hdc2, w1, kRowHeight-1);
					}

					if (row < gotNRows) {
						std::wstring wstr = u8_to_u16(rows[row][col]);

						TextOutW(hdc2, w1+3, (kRowHeight)-(kRowHeight/2)-7, wstr.c_str(), wstr.length());

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
					BitBlt(hdc, 0, kStrListTopMargin, rect.right, kRowHeight- upOffset, hdc2, 0, upOffset, SRCCOPY);
				} else {
					BitBlt(hdc, 0, kStrListTopMargin+(row)*kRowHeight - upOffset, rect.right, kRowHeight, hdc2, 0, 0, SRCCOPY);
				}
			}
			SelectObject(hdc2, hOldPen);
			SelectObject(hdc2, hOldBrush);
			SelectObject(hdc2, hOldFont);
			DeleteDC(hdc2);

			hOldBrush = (HBRUSH) SelectObject(hdc, g_SharedWindowVar->bgBrush1);
			hOldPen = (HPEN) SelectObject(hdc, g_SharedWindowVar->bgPen1);
			hOldFont = (HFONT) SelectObject(hdc, g_SharedWindowVar->hFont);

			Rectangle(hdc, 0, 0, rect.right, kStrListTopMargin);

			SelectObject(hdc, g_SharedWindowVar->hPen1);
			MoveToEx(hdc, 0, kStrListTopMargin-1, nullptr);
			LineTo(hdc, rect.right, kStrListTopMargin-1);

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
				w1 -= this->xspos_;
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

				Rectangle(hdc, w1, 0, w2, kRowHeight);

				std::wstring wstr = u8_to_u16(headers[col]);

				TextOutW(hdc, w1 + 3, (kRowHeight)-(kRowHeight/2)-7, wstr.c_str(), wstr.length());

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
				MoveToEx(hdc, w1, 0, nullptr);
				LineTo(hdc, w1, kStrListTopMargin-1);
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

			auto widthsHoldPtr = this->columnWidthsPtr_;
			if (!widthsHoldPtr) {
				return 1;
			}

			auto &widths = *widthsHoldPtr;

			int32_t gotNColumns = this->getNColumns();

			switch (wParam) {
			case 1: {
				sinfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
				sinfo.nMin = 0;
				if (this->nRows_ == 0)
					sinfo.nMax = 0;
				else
					sinfo.nMax = kRowHeight*this->nRows_-1;
				sinfo.nPos = this->yspos_ = 0;
				sinfo.nPage = rect.bottom-kStrListTopMargin;

				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				int32_t w2 = 0;
				for (int32_t i1 = 0; i1 < this->nColumns_; w2 += widths[i1], i1++);
				sinfo.nMax = w2 - 1;
				sinfo.nPage = rect.right;
				sinfo.nPos = this->xspos_ = 0;
				SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);


				break;
			}
			case 2: {
				sinfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
				if (this->nRows_ == 0) {
					sinfo.nMax = 0;
				} else {
					sinfo.nMax = kRowHeight*this->nRows_-1;
				}
				sinfo.nPos = this->yspos_ = 0;
				sinfo.nPage = rect.bottom-kStrListTopMargin;	// don't know why setting page is necessary

				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				int32_t w2 = 0;

				for (int32_t col1 = 0; col1 < gotNColumns; w2 += widths[col1], col1++);
				sinfo.nMax = w2 - 1;
				sinfo.nPage = rect.right;
				sinfo.nPos = this->xspos_ = 0;
				SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

				if ((this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin) || (rect.bottom-kStrListTopMargin <= 0)) {
					this->yspos_ = 0;
					ShowScrollBar(hwnd, SB_VERT, 0);
				} else {
					ShowScrollBar(hwnd, SB_VERT, 1);
					if (this->yspos_ > this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin))
						this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
				}
				if ((w2 <= rect.right) || (rect.right <= 0)) {
					this->xspos_ = 0;
					ShowScrollBar(hwnd, SB_HORZ, 0);
				} else {
					ShowScrollBar(hwnd, SB_VERT, 1);
					if (this->xspos_ > w2 - rect.right)
						this->xspos_ = w2 - rect.right;
				}
				break;
			}
			}
		}
		case WM_U_SL: {
			switch (wParam) {
				case kSL_ChangePage: {
					this->yspos_ = this->xspos_ = 0;
					this->hvrd_ = 0;
					if (this->isDragged_) {
						if (!ReleaseCapture()) {
							errorf("ReleaseCapture failed");
						}
					}
					this->isDragged_ = 0;
					this->isTimed_ = 0;
					this->lastSel_ = 0;

					RECT rect;
					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					//! TODO: add a method to do this instead
					SCROLLINFO sinfo = {0};
					sinfo.cbSize = sizeof(SCROLLINFO);
					sinfo.fMask = SIF_ALL;
					sinfo.nMin = 0;
					sinfo.nMax = kRowHeight*this->getNRows()-1;		// since it starts from 0 ROW_HEIGHT*nRows would be the first pixel after the area
					sinfo.nPage = rect.bottom-kStrListTopMargin;
					sinfo.nPos = this->yspos_;
					SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

					if (!(ShowScrollBar(hwnd, SB_VERT, 1))) {
						errorf("ShowScrollBar failed.");
					}

					int j = this->getTotalColumnsWidth();
					sinfo.nMax = j-1;	// the first column has its first pixel off-screen
					sinfo.nPage = rect.right;
					sinfo.nPos = this->xspos_;
					SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

					InvalidateRect(hwnd, nullptr, 1);

					break;
				}
			}
		}
		case WM_SIZE: {

			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			auto widthsHoldPtr = this->columnWidthsPtr_;
			if (!widthsHoldPtr) {
				return 1;
			}

			auto &widths = *widthsHoldPtr;

			int32_t gotNColumns = this->getNColumns();

			if (this->nRows_*kRowHeight <= (rect.bottom-kStrListTopMargin) || (rect.bottom-kStrListTopMargin <= 0)) {
				this->yspos_ = 0;
				ShowScrollBar(hwnd, SB_VERT, 0);
			} else {
				ShowScrollBar(hwnd, SB_VERT, 1);
				if (this->yspos_ > this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin))
					this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
			}
			int32_t w2 = 0;
			for (int32_t col1 = 0; col1 < gotNColumns; w2 += widths[col1], col1++);
			if (w2 <= rect.right || rect.right <= 0) {
				this->xspos_ = 0;
				ShowScrollBar(hwnd, SB_HORZ, 0);
			} else {
				ShowScrollBar(hwnd, SB_HORZ, 1);
				if (this->xspos_ > w2 - rect.right)
					this->xspos_ = w2 - rect.right;
			}

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_PAGE | SIF_POS;
			sinfo.nPage = rect.bottom-kStrListTopMargin;
			sinfo.nPos = this->yspos_;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			sinfo.nPage = rect.right;
			sinfo.nPos = this->xspos_;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

			if (!this->isTimed_) {		// if the previous timer is a '1' timer, the top text windows will not change
				SetTimer(hwnd, 0, 16, 0);
				this->isTimed_ = 1;
			}

			break;
		}
		case WM_VSCROLL: {
			bool doAbort = false;
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			switch(LOWORD(wParam)) {

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					this->yspos_ = HIWORD(wParam);

					break;

				case SB_LINEUP:

					if (this->yspos_ <= kRowHeight) {
						if (this->yspos_ == 0) {
							doAbort = true;
						} else {
							this->yspos_ = 0;
						}
					} else
						this->yspos_ -= kRowHeight;

					break;

				case SB_LINEDOWN:

					if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin || this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin)) {	// nRows*ROW_HEIGHT is the length of the rows and rect.bottom-STRLIST_TOP_MRG is the length of the view area
						doAbort = true;																													// their difference is the amount of pixels outside of view, each increase in yspos reveals one from the bottom
					} else if (this->yspos_ >= (this->nRows_-1)*kRowHeight-(rect.bottom-kStrListTopMargin)) {	// if nRows is changed to an unsigned value all line downs and page downs will break (could be fixed with a cast or moving the removed row or page to the left side of the comparison)
						this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
					} else {
						this->yspos_ += kRowHeight;
					}
					break;

				case SB_PAGEUP:

					if (this->yspos_ <= rect.bottom-kStrListTopMargin) {
						if (this->yspos_ == 0) {
							doAbort = true;
						} else {
							this->yspos_ = 0;
						}
					} else this->yspos_ -= rect.bottom-kStrListTopMargin;

					break;

				case SB_PAGEDOWN:

					if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin || this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin)) {
						doAbort = true;
					} else if (this->yspos_ >= (this->nRows_)*kRowHeight-2*(rect.bottom-kStrListTopMargin)) {
						this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
					} else
						this->yspos_ += rect.bottom-kStrListTopMargin;

					break;

				case SB_TOP:

					if (this->yspos_ == 0) {
						doAbort = true;
					} else
						this->yspos_ = 0;

					break;

				case SB_BOTTOM:
					if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin || this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin)) {
						doAbort = true;
					} else
						this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);

					break;

				default:
					doAbort = true;
					break;
			} if (doAbort)
				break;

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->yspos_;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->isTimed_) {
				SetTimer(hwnd, 1, 34, 0);
				this->isTimed_ = 1;
			}

			break;
		}
		case WM_HSCROLL: {
			bool doAbort = false;
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			switch(LOWORD(wParam)) {

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					this->xspos_ = HIWORD(wParam);

					break;

				case SB_LINEUP:

					if (this->xspos_ <= kRowHeight) {
						if (this->xspos_ == 0) {
							doAbort = true;
						} else {
							this->xspos_ = 0;
						}
					} else
						this->xspos_ -= kRowHeight;

					break;

				case SB_LINEDOWN: {
					int32_t w1 = this->getTotalColumnsWidth();

					if (w1 <= rect.right || this->xspos_ == w1 - rect.right) {
						doAbort = true;
					} else if (this->xspos_ >= w1 - rect.right-kRowHeight) {
						this->xspos_ = w1 - rect.right;		// if nRows is changed to an unsigned value all line downs and page downs will break
					} else {
						this->xspos_ += kRowHeight;
					}
					break;
				}
				case SB_PAGEUP:

					if (this->xspos_ <= rect.right) {
						if (this->xspos_ == 0) {
							doAbort = true;
						} else {
							this->xspos_ = 0;
						}
					} else this->xspos_ -= rect.right;

					break;

				case SB_PAGEDOWN: {
					int32_t w1 = this->getTotalColumnsWidth();

					if (w1 <= rect.right || this->xspos_ == w1 - rect.right) {
						doAbort = true;
					} else if (this->xspos_ >= w1 - 2*rect.right) {
						this->xspos_ = w1 - rect.right;
					} else {
						this->xspos_ += rect.right;
					}

					break;
				}
				case SB_TOP:

					if (this->xspos_ == 0) {
						doAbort = true;
					} else
						this->xspos_ = 0;

					break;

				case SB_BOTTOM: {
					int32_t w1 = this->getTotalColumnsWidth();

					if (w1 <= rect.right || this->xspos_ == w1 - rect.right) {
						doAbort = true;
					} else {
						this->xspos_ = w1 - rect.right;
					}
					break;
				}
				default: {
					doAbort = true;
					break;
				}
			} if (doAbort) {
				break;
			}

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->xspos_;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

			if (!this->isTimed_) {
				SetTimer(hwnd, 1, 34, 0);
				this->isTimed_ = 1;
			}

			break;
		}
		case WM_MOUSEWHEEL: {
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin)
				break;

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_POS;

			if ((HIWORD(wParam))/kRowHeight/2*kRowHeight == 0)
				break;

			if (this->yspos_ <= (int16_t)(HIWORD(wParam))/kRowHeight/2*kRowHeight) {
				if (this->yspos_ == 0)
					break;
				this->yspos_ = 0;
			} else if (this->yspos_ >= (this->nRows_*kRowHeight)-(rect.bottom-kStrListTopMargin)+((int16_t)(HIWORD(wParam))/kRowHeight/2*kRowHeight)) {	// wParam is negative if scrolling down
				if (this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin))
					break;
				this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
			} else this->yspos_ -= (int16_t)(HIWORD(wParam))/kRowHeight/2*kRowHeight;
			sinfo.nPos = this->yspos_;

			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->isTimed_)  {
				SetTimer(hwnd, 1, 34, 0);
				this->isTimed_ = 1;
			}

			break;
		}
		case WM_MOUSEMOVE: {

			auto widthsHoldPtr = this->columnWidthsPtr_;
			if (!widthsHoldPtr) {
				break;
			}
			auto &widths = *widthsHoldPtr;

			if (!this->isDragged_) {
				int32_t w2 = 0;
				int32_t col1 = 0;

				for (col1 = 0, w2 = -this->xspos_; col1 < this->nColumns_-1; col1++) {
					w2 += widths[col1];
					if ((lParam & (256*256-1)) >= w2 - 2 && (lParam & (256*256-1)) <= w2 + 2) {
						this->hvrd_ = col1 + 1;
						SetCursor(g_SharedWindowVar->hCrsSideWE);
						break;
					}
				} if (col1 == this->nColumns_-1) {
					this->hvrd_ = 0;
					SetCursor(g_SharedWindowVar->hDefCrs);
				}
				break;
			}
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			int32_t col2 = this->isDragged_-1;
			widths[col2] = (((int16_t) (lParam)) - this->drgpos_);
			if (widths[col2] < 5)
				widths[col2] = 5;
			if (widths[col2] > rect.right-5+this->xspos_) {
				widths[col2] = rect.right-5+this->xspos_;
			}

			int32_t w1 = this->getTotalColumnsWidth();
			if (w1 <= rect.right || rect.right <= 0) {
				this->xspos_ = 0;
				ShowScrollBar(hwnd, SB_HORZ, 0);
			} else {
				ShowScrollBar(hwnd, SB_HORZ, 1);
				if (this->xspos_ > w1 - rect.right)
					this->xspos_ = w1 - rect.right;
			}

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			sinfo.nMin = 0;
			sinfo.nMax = w1 - 1;
			sinfo.nPos = this->xspos_;
			sinfo.nPage = rect.right;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

			if (!this->isTimed_) {
				SetTimer(hwnd, 0, 34, 0);
				this->isTimed_ = 1;
			}

			break;
		}
		case WM_LBUTTONDOWN: {
			RECT rect2 = {};

			rect2.left = lParam & (256*256-1);
			rect2.top = lParam/256/256 & (256*256-1);

			if (!this->hvrd_ || this->isDragged_) {
				if (rect2.top >= kStrListTopMargin && !this->isDragged_) {
					// the row the cursor is hovering over
					int32_t hoverRow = this->yspos_/kRowHeight+(rect2.top-kStrListTopMargin + this->yspos_%kRowHeight)/kRowHeight;	// first position is 0

					if (!(wParam & MK_CONTROL) || (this->winOptions_ & 1)) {
						this->clearSelections();
					}

					if (hoverRow < this->nRows_) {
						auto selsHoldPtr = rowSelsPtr_;
						if (!selsHoldPtr) {
							break;
						}

						auto &rowSels = *selsHoldPtr;

						if (!(wParam & MK_SHIFT) || (this->winOptions_ & 1)) {
							rowSels[hoverRow] = !rowSels[hoverRow];
							this->lastSel_ = hoverRow;
						//! TODO: with shift, select rows in range if hoverRow is selected and deselect otherwise
						} else {
							int64_t sel1 = this->lastSel_;
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
				auto widthsHoldPtr = this->columnWidthsPtr_;
				if (!widthsHoldPtr) {
					break;
				}
				auto &colWidths = *widthsHoldPtr;

				this->isDragged_ = this->hvrd_;
				this->drgpos_ = rect2.left - colWidths[this->hvrd_-1];
				SetCapture(hwnd);
			}

			break;
		}
		case WM_LBUTTONUP: {

			if (!this->isDragged_)
				break;
			if (!ReleaseCapture())
				errorf("ReleaseCapture failed");
			this->isDragged_ = 0;
			SendMessage(hwnd, WM_MOUSEMOVE, wParam, lParam);

			break;
		}
		case WM_RBUTTONDOWN: {
			RECT rect2 = {};

			this->point_.x = rect2.left = lParam & (256*256-1);
			this->point_.y = rect2.top = lParam/256/256 & (256*256-1);
			if (rect2.top < kStrListTopMargin)
				break;
			MapWindowPoints(hwnd, 0, &this->point_, 1);

			if (!(wParam & MK_CONTROL)) {
				int64_t hoverRow = this->yspos_/kRowHeight+(rect2.top-kStrListTopMargin + this->yspos_%kRowHeight)/kRowHeight;
				if (hoverRow >= this->nRows_) {
					break;
				}
				// if row is not selected, clear selections
				auto selsHoldPtr = this->rowSelsPtr_;
				if (!selsHoldPtr) {
					break;
				}
				auto &rowSels = *selsHoldPtr;

				if (!rowSels[hoverRow]) {
					this->clearSelections();

					rowSels[hoverRow] = !rowSels[hoverRow];
					this->lastSel_ = hoverRow;
				}
				if (!(InvalidateRect(hwnd, 0, 1))) {
					errorf("InvalidateRect failed");
				}
			}
			SendMessage(GetParent(hwnd), WM_U_RET_SL, 4, (LPARAM) GetMenu(hwnd));

			break;
		}
		case WM_TIMER: {
			bool doAbort = false;

			switch(wParam) {

			case 0:
			case 1:

				KillTimer(hwnd, 0);
				this->isTimed_ = 0;

				if (!(InvalidateRect(hwnd, 0, 0))) {
					errorf("InvalidateRect failed");
				}
				break;

			case 2:

				KillTimer(hwnd, 2);
				SetTimer(hwnd, 2, 67, 0);

				RECT rect;

				switch(this->lastKey_) {
				case 1:
					if (this->yspos_ == 0) {
						doAbort = true;
					} else if (this->yspos_ <= kRowHeight) {
						this->yspos_ = 0;
					} else
						this->yspos_ -= kRowHeight;

					break;

				case 2:
					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin || this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin)) {
						doAbort = true;
					}
					else if (this->yspos_ >= (this->nRows_-1)*kRowHeight-(rect.bottom-kStrListTopMargin)) {
						this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
					} else
						this->yspos_ += kRowHeight;

					break;

				case 3:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->yspos_ == 0) {
						doAbort = true;
					} else if (this->yspos_ <= rect.bottom-kStrListTopMargin) {
						this->yspos_ = 0;
					} else
						this->yspos_ -= rect.bottom-kStrListTopMargin;

					break;

				case 4:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin || this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin)) {
						doAbort = true;
					} else if (this->yspos_ >= (this->nRows_)*kRowHeight-2*(rect.bottom-kStrListTopMargin)) {
						this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
					} else
						this->yspos_ += rect.bottom-kStrListTopMargin;

					break;

				case 5:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->xspos_ <= kRowHeight) {
						if (this->xspos_ == 0) {
							doAbort = true;
						} else {
							this->xspos_ = 0;
						}
					} else
						this->xspos_ -= kRowHeight;

					break;

				case 6: {

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					int32_t w1 = this->getTotalColumnsWidth();

					if (w1 <= rect.right || this->xspos_ == w1 - rect.right) {
						doAbort = true;
					} else if (this->xspos_ >= w1 - rect.right-kRowHeight) {
						this->xspos_ = w1 - rect.right;
					} else {
						this->xspos_ += kRowHeight;
					}

					break;
				}
				default: {
					KillTimer(hwnd, 2);
					doAbort = true;
					break;
				}
				}

				if (doAbort)
					break;

				SCROLLINFO sinfo = {0};

				sinfo.fMask = SIF_POS;
				sinfo.nPos = this->yspos_;
				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);
				sinfo.nPos = this->xspos_;
				SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

				if (!this->isTimed_) {
					SetTimer(hwnd, 1, 34, 0);
					this->isTimed_ = 1;
				}

				break;
			}
			break;
		}
		case WM_KEYDOWN: {
			bool doAbort = false;
			RECT rect;
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if ((lParam & 0x40000000) != 0)
				break;

			KillTimer(hwnd, 2);

			switch(wParam) {

			case VK_HOME:
				if (this->yspos_ == 0) {
					doAbort = true;
				} else {
					this->yspos_ = 0;
				}
				break;

			case VK_END:

				if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin || this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin)) {
					doAbort = true;
				} else {
					this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
				}

				break;

			case VK_UP:

				if (this->yspos_ == 0) {
					doAbort = true;
				} else if (this->yspos_ <= kRowHeight) {
					this->yspos_ = 0;
				} else {
					this->yspos_ -= kRowHeight;
					this->lastKey_ = 1;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_DOWN:

				if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin || this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin)) {
					doAbort = true;
				} else if (this->yspos_ >= (this->nRows_-1)*kRowHeight-(rect.bottom-kStrListTopMargin)) {
					this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
				} else {
					this->yspos_ += kRowHeight;
					this->lastKey_ = 2;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_LEFT:

				if (this->xspos_ <= kRowHeight) {
					if (this->xspos_ == 0) {
						doAbort = true;
					} else {
						this->xspos_ = 0;
					}
				} else {
					this->xspos_ -= kRowHeight;
					this->lastKey_ = 5;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_RIGHT: {

				int32_t w1 = this->getTotalColumnsWidth();

				if (w1 <= rect.right || this->xspos_ == w1 - rect.right) {
					doAbort = true;
				} else if (this->xspos_ >= w1 - rect.right-kRowHeight) {
					this->xspos_ = w1 - rect.right;
				} else {
					this->xspos_ += kRowHeight;
					this->lastKey_ = 6;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;
			}

			case VK_PRIOR:

				if (this->yspos_ <= rect.bottom-kStrListTopMargin) {
					if (this->yspos_ == 0) {
						doAbort = true;
					} else {
						this->yspos_ = 0;
					}
				} else {
					this->yspos_ -= rect.bottom-kStrListTopMargin;
					this->lastKey_ = 3;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_NEXT:

				if (this->nRows_*kRowHeight <= rect.bottom-kStrListTopMargin || this->yspos_ == this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin)) {
					doAbort = true;
				} else if (this->yspos_ >= (this->nRows_)*kRowHeight-2*(rect.bottom-kStrListTopMargin)) {
					this->yspos_ = this->nRows_*kRowHeight-(rect.bottom-kStrListTopMargin);
				} else {
					this->yspos_ += rect.bottom-kStrListTopMargin;
					this->lastKey_ = 4;
					SetTimer(hwnd, 2, 500, 0);
				}
				break;

			case VK_DELETE:

				doAbort = true;
				SendMessage(GetParent(hwnd), WM_U_RET_SL, 1, (LPARAM) GetMenu(hwnd));

				break;

			case VK_F2:

				doAbort = true;
				SendMessage(GetParent(hwnd), WM_U_RET_SL, 2, (LPARAM) GetMenu(hwnd));

				break;

			} if (doAbort)
				break;

			SCROLLINFO sinfo = {0};

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->yspos_;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);
			sinfo.nPos = this->xspos_;
			SetScrollInfo(hwnd, SB_HORZ, &sinfo, TRUE);

			if (!this->isTimed_) {
				SetTimer(hwnd, 1, 34, 0);
				this->isTimed_ = 1;
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
				if (this->lastKey_ == 1) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_DOWN:
				if (this->lastKey_ == 2) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_PRIOR:
				if (this->lastKey_ == 3) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_NEXT:
				if (this->lastKey_ == 4) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_LEFT:
				if (this->lastKey_ == 5) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_RIGHT:
				if (this->lastKey_ == 6) {
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

//{ ThumbListWindow
//public:
// static
const WindowHelper ThumbListWindow::helper = WindowHelper(std::wstring(L"ThumbListWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;
	}
);

// static
HWND ThumbListWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new ThumbListWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT ThumbListWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	LPARAM &lParam = procArgs.lParam;
	WPARAM &wParam = procArgs.wParam;

	this->yspos_ = 0;
	this->hvrd_ = 0;
	this->isTimed_ = 0;

	RECT rect;
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	int rowlen, lastrow;

	rowlen = (rect.right-kThumbListLeftMargin+kDefThumbGapX) / (kDefThumbFW+kDefThumbGapX);
	if (rowlen < 1)
		rowlen = 1;
	if (rowlen >= this->nThumbs_)
		lastrow = 1;
	else
		lastrow = this->nThumbs_/rowlen + !!(this->nThumbs_%rowlen);

	SCROLLINFO sinfo = {0};

	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_ALL;
	sinfo.nMin = 0;
	sinfo.nMax = kThumbListTopMargin+lastrow*(kDefThumbFH+kDefThumbGapY)-kDefThumbGapY-1;
	sinfo.nPage = rect.bottom;
	sinfo.nPos = this->yspos_;
	SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

	if (!(ShowScrollBar(hwnd, SB_VERT, 1))) {
		errorf("ShowScrollBar failed.");
	}


	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK ThumbListWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int64_t i;
	int j, k, l, pos;
	uint8_t *buf;
	wchar_t *wbuf;
	SCROLLINFO sinfo = {0};
	//THMBLISTV *lv = 0;

	oneslnk **link;

	int len, diff, rowlen, lastrow, firstrow, sparep;
	char sel;

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_PAINT: {
			BITMAPINFO bminfo = {0};

			HDC hdc, hdc2, hdc3;
			PAINTSTRUCT ps;
			HFONT hOldFont;
			HPEN hOldPen;
			HBRUSH hOldBrush;
			HBITMAP hOldBM, hBitmap;

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			rowlen = (rect.right-kThumbListLeftMargin+kDefThumbGapX) / (kDefThumbFW+kDefThumbGapX);
			if (rowlen < 1)
				rowlen = 1;
			else if (rowlen > this->nThumbs_)
				rowlen = this->nThumbs_;
			if (rowlen > 1) {
				sparep = (((rect.right-kThumbListLeftMargin+kDefThumbGapX) - rowlen*(kDefThumbFW+kDefThumbGapX)) / (rowlen));
			} else
				sparep = 0;

			if (sparep > (kDefThumbFW+kDefThumbGapX-1)/2) { // capped at the maximum the gap could get with multiple rows
				sparep = (kDefThumbFW+kDefThumbGapX-1)/2;
			}

			firstrow = (this->yspos_-kThumbListTopMargin)/(kDefThumbFH+kDefThumbGapY);
			if (this->yspos_ < kThumbListTopMargin)
				firstrow = -1;

			diff = (this->yspos_-kThumbListTopMargin)-firstrow*(kDefThumbFH+kDefThumbGapY);		// diff is the amount of pixels outside the area

			bminfo.bmiHeader.biSize = sizeof(bminfo.bmiHeader);
			bminfo.bmiHeader.biPlanes = 1;
			bminfo.bmiHeader.biBitCount = 32;
			bminfo.bmiHeader.biCompression = BI_RGB;

			hdc = BeginPaint(hwnd, &ps);

			hdc3 = CreateCompatibleDC(hdc);
			hOldPen = (HPEN) GetCurrentObject(hdc3, OBJ_PEN);
			hOldBrush = (HBRUSH) GetCurrentObject(hdc3, OBJ_BRUSH);
//			hOldFont = (HFONT) SelectObject(hdc2, g_shared_window_vars.hFont);

			if (!g_SharedWindowVar->hThumbListBM) {
				i = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				if (i == 0) {
					i = 1920;
				} j = GetSystemMetrics(SM_CYVIRTUALSCREEN);
				if (j == 0) {
					j = 1920;
				}
				g_SharedWindowVar->hThumbListBM = CreateCompatibleBitmap(hdc, i, j);
			}
			hOldBM = (HBITMAP) SelectObject(hdc3, g_SharedWindowVar->hThumbListBM);

			SelectObject(hdc3, g_SharedWindowVar->bgBrush1);
			SelectObject(hdc3, g_SharedWindowVar->bgPen1);
			Rectangle(hdc3, 0, 0, rect.right, rect.bottom);
			SelectObject(hdc3, g_SharedWindowVar->bgPen2);
			SelectObject(hdc3, g_SharedWindowVar->bgBrush2);

			for (i = 0, lastrow = (!!diff)+(rect.bottom-(kDefThumbFH+kDefThumbGapY-diff)%(kDefThumbFH+kDefThumbGapY))/(kDefThumbFH+kDefThumbGapY)+(!!((rect.bottom-(kDefThumbFH+kDefThumbGapY-diff))%(kDefThumbFH+kDefThumbGapY))), sel = 0; i < lastrow; i++, pos++, sel = 0) {
				if (firstrow+i < 0)
					continue;
				for (j = 0; j < rowlen; j++) {
					if ((k = (firstrow+i)*rowlen+j) >= this->nThumbs_)
						break;
					SelectObject(hdc3, g_SharedWindowVar->bgPen2);
					SelectObject(hdc3, g_SharedWindowVar->bgBrush2);
					if (this->thumbSel_[k/8] & (1 << k%8)) {
						SelectObject(hdc3, g_SharedWindowVar->selPen);
						SelectObject(hdc3, g_SharedWindowVar->selBrush);
					} else if (k+1 == this->hvrd_) {
						SelectObject(hdc3, g_SharedWindowVar->selPen2);
						SelectObject(hdc3, g_SharedWindowVar->selBrush);
					}
					Rectangle(hdc3, kThumbListLeftMargin+j*(kDefThumbFW+kDefThumbGapX+sparep), i*(kDefThumbFH+kDefThumbGapY)-diff, kThumbListLeftMargin+j*(kDefThumbFW+kDefThumbGapX+sparep)+kDefThumbFW, i*(kDefThumbFH+kDefThumbGapY)-diff+kDefThumbFH);

					if (this->thumbs_[k]) {
						bminfo.bmiHeader.biWidth = this->thumbs_[k]->x;
						bminfo.bmiHeader.biHeight = -this->thumbs_[k]->y;

						if ((SetDIBitsToDevice(hdc3, kThumbListLeftMargin+j*(kDefThumbFW+kDefThumbGapX+sparep)+(kDefThumbFW-this->thumbs_[k]->x)/2, i*(kDefThumbFH+kDefThumbGapY)-diff+(kDefThumbFH - this->thumbs_[k]->y)/2, this->thumbs_[k]->x, this->thumbs_[k]->y,
						0, 0, 0, this->thumbs_[k]->y, this->thumbs_[k]->img[0], &bminfo, DIB_RGB_COLORS)) == 0) {
							g_errorfStream << "SetDIBitsToDevice failed, h: " << this->thumbs_[k]->y << ", w: " << this->thumbs_[k]->x << ", source: " << this->thumbs_[k]->img[0] << std::flush;
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
		}
		case WM_USER: {	// update scroll info
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			rowlen = (rect.right-kThumbListLeftMargin+kDefThumbGapX) / (kDefThumbFW+kDefThumbGapX);
			if (rowlen < 1)
				rowlen = 1;
			if (rowlen > this->nThumbs_)
				lastrow = 1;
			else
				lastrow = this->nThumbs_/rowlen + !!(this->nThumbs_%rowlen);

			switch (wParam) {
			case 1:
				sinfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.nMin = 0;
				sinfo.nPos = this->yspos_ = 0;
				sinfo.nMax = kThumbListTopMargin+lastrow*(kDefThumbFH+kDefThumbGapY)-kDefThumbGapY-1;
				sinfo.nPage = rect.bottom;

				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				break;

			case 2:
				sinfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
				sinfo.cbSize = sizeof(SCROLLINFO);
				if (this->nThumbs_ == 0)
					sinfo.nMax = 0;
				else
					sinfo.nMax = kThumbListTopMargin+lastrow*(kDefThumbFH+kDefThumbGapY)-kDefThumbGapY-1;
				sinfo.nPos = this->yspos_ = 0;
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
		}
		case WM_SIZE: {  // ROW_HEIGHT

			RECT rect;

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			rowlen = (rect.right-kThumbListLeftMargin+kDefThumbGapX) / (kDefThumbFW+kDefThumbGapX);
			if (rowlen < 1)
				rowlen = 1;
			if (rowlen > this->nThumbs_)
				lastrow = 1;
			else
				lastrow = this->nThumbs_/rowlen + !!(this->nThumbs_%rowlen);

			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_ALL;
			sinfo.nMin = 0;
			sinfo.nMax = kThumbListTopMargin+lastrow*(kDefThumbFH+kDefThumbGapY)-kDefThumbGapY-1;
			sinfo.nPage = rect.bottom;
			if (sinfo.nMax <= rect.bottom || rect.bottom <= 0) {
				this->yspos_ = 0;
				ShowScrollBar(hwnd, SB_VERT, 0);
			} else {
				ShowScrollBar(hwnd, SB_VERT, 1);
				if (this->yspos_ > sinfo.nMax)
					this->yspos_ = sinfo.nMax;
			}
			sinfo.nPos = this->yspos_;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->isTimed_) {		// if the previous timer is a '1' timer, the top text windows will not change
				SetTimer(hwnd, 0, 16, 0);
				this->isTimed_ = 1;
			}

			break;
		}
		case WM_VSCROLL: {
			bool doAbort = false;

			switch(LOWORD(wParam)) {

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					this->yspos_ = HIWORD(wParam);

					break;

				case SB_LINEUP:

					if (this->yspos_ <= kRowHeight) {
						if (this->yspos_ == 0) {
							doAbort = true;
						} else {
							this->yspos_ = 0;
						}
					} else
						this->yspos_ -= kRowHeight;

					break;

				case SB_LINEDOWN:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);
					if (sinfo.nMax <= rect.bottom || this->yspos_ == sinfo.nMax+1-rect.bottom) {
						doAbort = true;
					} else if (this->yspos_ >= sinfo.nMax+1-kRowHeight-rect.bottom) {	// if nThumbs is changed to an unsigned value all line downs and page downs will break (could be fixed with a cast or moving the removed row or page to the left side of the comparison)
						this->yspos_ = sinfo.nMax+1-rect.bottom;
					} else {
						this->yspos_ += kRowHeight;
					}
					break;

				case SB_PAGEUP:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->yspos_ <= rect.bottom) {
						if (this->yspos_ == 0) {
							doAbort = true;
						} else {
							this->yspos_ = 0;
						}
					} else this->yspos_ -= rect.bottom;

					break;

				case SB_PAGEDOWN:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);

					if (sinfo.nMax <= rect.bottom || this->yspos_ == sinfo.nMax+1-rect.bottom) {
						doAbort = true;
					} else if (this->yspos_ >= sinfo.nMax+1-2*rect.bottom) {
						this->yspos_ = sinfo.nMax+1-rect.bottom;
					} else
						this->yspos_ += rect.bottom;

					break;

				case SB_TOP:

					if (this->yspos_ == 0) {
						doAbort = true;
					} else
						this->yspos_ = 0;

					break;

				case SB_BOTTOM:

					if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);
					if (sinfo.nMax <= rect.bottom || this->yspos_ == sinfo.nMax+1-rect.bottom) {
						doAbort = true;
					} else
						this->yspos_ = sinfo.nMax+1-(rect.bottom);

					break;

				default:
					doAbort = true;
					break;
			} if (doAbort)
				break;

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->yspos_;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->isTimed_) {
				SetTimer(hwnd, 1, 34, 0);
				this->isTimed_ = 1;
			}

			break;
		}
		case WM_MOUSEWHEEL: {

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			sinfo.fMask = SIF_RANGE;
			GetScrollInfo(hwnd, SB_VERT, &sinfo);

			if (sinfo.nMax+1 <= rect.bottom)
				break;

			sinfo.fMask = SIF_POS;

			if ((HIWORD(wParam))/kRowHeight/2*kRowHeight == 0)
				break;

			if (this->yspos_ <= (int16_t)(HIWORD(wParam))/kRowHeight/2*kRowHeight) {
				if (this->yspos_ == 0)
					break;
				this->yspos_ = 0;
			} else if (this->yspos_ >= (sinfo.nMax+1)-rect.bottom+((int16_t)(HIWORD(wParam))/kRowHeight/2*kRowHeight)) {	// wParam is negative if scrolling down
				if (this->yspos_ == sinfo.nMax+1-rect.bottom)
					break;
				this->yspos_ = sinfo.nMax+1-rect.bottom;
			} else this->yspos_ -= (int16_t)(HIWORD(wParam))/kRowHeight/2*kRowHeight;
			sinfo.nPos = this->yspos_;

			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->isTimed_)  {
				SetTimer(hwnd, 1, 34, 0);
				this->isTimed_ = 1;
			}

			break;
		}
		case WM_MOUSEMOVE: {
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			l = this->hvrd_;
			rowlen = (rect.right-kThumbListLeftMargin+kDefThumbGapX) / (kDefThumbFW+kDefThumbGapX);
			if (rowlen < 1)
				rowlen = 1;
			else if (rowlen > this->nThumbs_)
				rowlen = this->nThumbs_;
			if (rowlen > 1) {
				sparep = (((rect.right-kThumbListLeftMargin+kDefThumbGapX) - rowlen*(kDefThumbFW+kDefThumbGapX)) / (rowlen));
			} else
				sparep = 0;

			if (sparep > (kDefThumbFW+kDefThumbGapX-1)/2) { // capped at the maximum the gap could get with multiple rows
				sparep = (kDefThumbFW+kDefThumbGapX-1)/2;
			}

			if ((j = (int16_t)(lParam & (256*256-1))) < kThumbListLeftMargin || (k = (int16_t)((lParam/256/256) & (256*256-1))+this->yspos_-kThumbListTopMargin) < 0 || (j - kThumbListLeftMargin) % (kDefThumbFW+kDefThumbGapX+sparep) >= kDefThumbFW || (k+this->yspos_) % (kDefThumbFH+kDefThumbGapY) >= kDefThumbFH) {
				this->hvrd_ = 0;
				SetCursor(g_SharedWindowVar->hDefCrs);
				if (l > 0) {
					if (!(InvalidateRect(hwnd, 0, 0))) {
						errorf("InvalidateRect failed");
					}
				} break;
			}

			this->hvrd_ = (k/(kDefThumbFH+kDefThumbGapY))*rowlen+(j - kThumbListLeftMargin)/(kDefThumbFW+kDefThumbGapX+sparep)+1;

			if (this->hvrd_ > this->nThumbs_) {
				this->hvrd_ = 0;
				SetCursor(g_SharedWindowVar->hDefCrs);
				if (l > 0) {
					if (!(InvalidateRect(hwnd, 0, 0))) {
						errorf("InvalidateRect failed");
					}
				} break;
			}
			SetCursor(g_SharedWindowVar->hDefCrs);
			if (l != this->hvrd_) {
				if (!(InvalidateRect(hwnd, 0, 0))) {
					errorf("InvalidateRect failed");
				}
			}

			break;
		}
		case WM_LBUTTONDOWN: {

			if (!(wParam & MK_CONTROL)) {
				for (i = this->nThumbs_/8+!!(this->nThumbs_%8)-1; i >= 0; this->thumbSel_[i] = 0, i--);
			}

			if (this->hvrd_ > 0 && this->hvrd_ <= this->nThumbs_) {
				pos = this->hvrd_-1;
				if (!(wParam & MK_SHIFT)) {
					this->thumbSel_[pos/8] ^= (1 << pos%8);
					this->lastSel_ = pos;
				} else {
					if ((i = this->lastSel_) <= pos)
						for (; i <= pos; i++)
							this->thumbSel_[i/8] |= (1 << i%8);
					else
						for (; i >= pos; i--)
							this->thumbSel_[i/8] |= (1 << i%8);
				}
			}
			if (!(InvalidateRect(hwnd, 0, 1))) {
				errorf("InvalidateRect failed");
			}
			SetFocus(hwnd);

			break;
		}
		case WM_LBUTTONUP: {



			break;
		}
		case WM_RBUTTONDOWN: {


			this->point_.x = rect.left = lParam & (256*256-1);
			this->point_.y = rect.top = lParam/256/256 & (256*256-1);
			if (rect.top < 0)
				break;
			MapWindowPoints(hwnd, 0, &this->point_, 1);

			if (!(wParam & MK_CONTROL)) {
				if (this->hvrd_ > this->nThumbs_ || this->hvrd_ < 1)
					break;
				pos = this->hvrd_-1;
				if (!(this->thumbSel_[pos/8] & (1 << pos%8))) {
					for (i = this->nThumbs_/8+!!(this->nThumbs_%8)-1; i >= 0; this->thumbSel_[i] = 0, i--);
					this->thumbSel_[pos/8] ^= (1 << pos%8);
					this->lastSel_ = pos;
				}
				if (!(InvalidateRect(hwnd, 0, 1))) {
					errorf("InvalidateRect failed");
				}
			}
			SendMessage(GetParent(hwnd), WM_U_RET_TL, (WPARAM) GetMenu(hwnd), 4);

			break;
		}
		case WM_TIMER: {
			bool doAbort = false;

			switch(wParam) {

			case 0:
			case 1:

				KillTimer(hwnd, 0);
				this->isTimed_ = 0;

				if (!(InvalidateRect(hwnd, 0, 0))) {
					errorf("InvalidateRect failed");
				}
				break;

			case 2:

				KillTimer(hwnd, 2);
				SetTimer(hwnd, 2, 67, 0);

				switch(this->lastKey_) {
				case 1:
					if (this->yspos_ == 0) {
						doAbort = true;
					} else if (this->yspos_ <= kRowHeight) {
						this->yspos_ = 0;
					} else
						this->yspos_ -= kRowHeight;

					break;

				case 2:
					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);
					if (sinfo.nMax <= rect.bottom || this->yspos_ == sinfo.nMax+1-rect.bottom) {
						doAbort = true;
					}
					else if (this->yspos_ >= sinfo.nMax+1-kRowHeight-rect.bottom) {
						this->yspos_ = sinfo.nMax+1-rect.bottom;
					} else
						this->yspos_ += kRowHeight;

					break;

				case 3:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}

					if (this->yspos_ == 0) {
						doAbort = true;
					} else if (this->yspos_ <= rect.bottom) {
						this->yspos_ = 0;
					} else
						this->yspos_ -= rect.bottom;

					break;

				case 4:

					if (GetClientRect(hwnd, &rect) == 0) {
						errorf("GetClientRect failed");
					}
					sinfo.fMask = SIF_RANGE;
					GetScrollInfo(hwnd, SB_VERT, &sinfo);

					if (sinfo.nMax <= rect.bottom || this->yspos_ == sinfo.nMax+1-rect.bottom) {
						doAbort = true;
					} else if (this->yspos_ >= sinfo.nMax+1-2*rect.bottom) {
						this->yspos_ = sinfo.nMax+1-rect.bottom;
					} else
						this->yspos_ += rect.bottom;

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
					doAbort = true;
					break;
				} if (doAbort)
					break;

				sinfo.fMask = SIF_POS;
				sinfo.nPos = this->yspos_;
				SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

				if (!this->isTimed_) {
					SetTimer(hwnd, 1, 34, 0);
					this->isTimed_ = 1;
				}

				break;
			}
			break;
		}
		case WM_KEYDOWN: {
			bool doAbort = false;

			if ((lParam & 0x40000000) != 0)
				break;

			KillTimer(hwnd, 2);

			switch(wParam) {

			case VK_HOME:
				if (this->yspos_ == 0) {
					doAbort = true;
				} else {
					this->yspos_ = 0;
				}
				break;

			case VK_END:
				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}
				sinfo.fMask = SIF_RANGE;
				GetScrollInfo(hwnd, SB_VERT, &sinfo);
				if (sinfo.nMax <= rect.bottom || this->yspos_ == sinfo.nMax+1-rect.bottom) {
					doAbort = true;
				} else
					this->yspos_ = sinfo.nMax+1-(rect.bottom);

				break;

			case VK_UP:

				if (this->yspos_ == 0) {
					doAbort = true;
				} else if (this->yspos_ <= kRowHeight) {
					this->yspos_ = 0;
				} else {
					this->yspos_ -= kRowHeight;
					this->lastKey_ = 1;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_DOWN:

				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}
				sinfo.fMask = SIF_RANGE;
				GetScrollInfo(hwnd, SB_VERT, &sinfo);

				if (sinfo.nMax <= rect.bottom || this->yspos_ == sinfo.nMax+1-rect.bottom) {
					doAbort = true;
				} else if (this->yspos_ >= sinfo.nMax+1-kRowHeight-rect.bottom) {
					this->yspos_ = sinfo.nMax+1-rect.bottom;
				} else {
					this->yspos_ += kRowHeight;
					this->lastKey_ = 2;
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
					doAbort = true;
				} else if (0) {
				} else {
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_PRIOR:

				if (GetClientRect(hwnd, &rect) == 0) {		// forgot this part
					errorf("GetClientRect failed");
				}

				if (this->yspos_ <= rect.bottom) {
					if (this->yspos_ == 0) {
						doAbort = true;
					} else {
						this->yspos_ = 0;
					}
				} else {
					this->yspos_ -= rect.bottom;
					this->lastKey_ = 3;
					SetTimer(hwnd, 2, 500, 0);
				}

				break;

			case VK_NEXT:

				if (GetClientRect(hwnd, &rect) == 0) {
					errorf("GetClientRect failed");
				}
				sinfo.fMask = SIF_RANGE;
				GetScrollInfo(hwnd, SB_VERT, &sinfo);

				if (sinfo.nMax <= rect.bottom || this->yspos_ == sinfo.nMax+1-rect.bottom) {
					doAbort = true;
				} else if (this->yspos_ >= sinfo.nMax+1-2*rect.bottom) {
					this->yspos_ = sinfo.nMax+1-rect.bottom;
				} else {
					this->yspos_ += rect.bottom;
					this->lastKey_ = 4;
					SetTimer(hwnd, 2, 500, 0);
				}
				break;

			case VK_DELETE:

				doAbort = true;
				SendMessage(GetParent(hwnd), WM_U_RET_TL, (WPARAM) GetMenu(hwnd), 1);

				break;

			case VK_F2:

				doAbort = true;
				SendMessage(GetParent(hwnd), WM_U_RET_TL, (WPARAM) GetMenu(hwnd), 2);

				break;
			case VK_BACK:
				PostMessage(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);

				break;
			} if (doAbort)
				break;

			sinfo.fMask = SIF_POS;
			sinfo.nPos = this->yspos_;
			SetScrollInfo(hwnd, SB_VERT, &sinfo, TRUE);

			if (!this->isTimed_) {
				SetTimer(hwnd, 1, 34, 0);
				this->isTimed_ = 1;
			}
			break;
		}
		case WM_SYSKEYDOWN: {

			if (lParam & (1UL << 29)) {
				switch(wParam) {
					case VK_F2:
						SendMessage(GetParent(hwnd), WM_U_RET_TL, (WPARAM) GetMenu(hwnd), 3);

						break;
				}
			}

			break;
		}
		case WM_KEYUP: {

			switch(wParam) {
			case VK_UP:
				if (this->lastKey_ == 1) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_DOWN:
				if (this->lastKey_ == 2) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_PRIOR:
				if (this->lastKey_ == 3) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_NEXT:
				if (this->lastKey_ == 4) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_LEFT:
				if (this->lastKey_ == 5) {
					KillTimer(hwnd, 2);
				}

				break;

			case VK_RIGHT:
				if (this->lastKey_ == 6) {
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
		case WM_DESTROY: {
			return 0;
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ ViewImageWindow

ViewImageWindow::ViewImageWindow() :
	WindowClass(),
	hThumbListBitmap(nullptr)
	{}

//public:
// static
const WindowHelper ViewImageWindow::helper = WindowHelper(std::wstring(L"ViewImageWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = 0;
		wc.hCursor = nullptr;
	}
);

// static
HWND ViewImageWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new ViewImageWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT ViewImageWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	LPARAM &lParam = procArgs.lParam;
	WPARAM &wParam = procArgs.wParam;

	IMGVIEWV *ev = 0;

	if (!(ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0))) { // get mem from parent
		DestroyWindow(GetParent(hwnd));
		errorf("No external variables returned");
		return 0;
	}

	RECT rect;
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	if (ev->imgPath.empty()) {
		errorf("no imgPath");
		DestroyWindow(GetParent(hwnd));
		return 0;
	}
	if (!(ev->FullImage = ReadImage(ev->imgPath))) {	//! will leak if an image already existed
		g_errorfStream << "couldn't load image: " << ev->imgPath << std::flush;
		DestroyWindow(GetParent(hwnd));
		return 0;
	}
	ev->fit = kBitVI_FitModeEnabled;
	ev->DispImage = nullptr;
	ev->zoomp = 0;

	if (!kBitVI_FitModeEnabled) {
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
	if (!(ev->fit & kBitVI_FitDispIsOrig)) {
		if (ev->DispImage) {
			ev->DispImage = nullptr;
		}
	} else {
		ev->fit &= ~kBitVI_FitDispIsOrig;
	}

	if ( (long) ev->FullImage->x > rect.right || (long) ev->FullImage->y > rect.bottom) {
		ev->DispImage = FitImage(ev->FullImage, rect.right, rect.bottom, 0, 0);
	} else {
		ev->fit |= kBitVI_FitDispIsOrig;
		ev->DispImage = ev->FullImage;
	}
	ev->xpos = ((int64_t)ev->DispImage->x - rect.right)/2;
	ev->ypos = ((int64_t)ev->DispImage->y - rect.bottom)/2;
	if (ev->DispImage == nullptr) {
		errorf("no DispImage");
		DestroyWindow(GetParent(hwnd));
		return 0;
	}

// g_errorfStream << "1: ev->midX: " << ev->midX << ", ev->midY: " << ev->midY << ", x: " << ev->FullImage->x << ", y: " << ev->FullImage->y << std::flush;


	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK ViewImageWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

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

	double ratio, midpos;

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_PAINT: {
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == nullptr) {
				errorf("ev is null");
				break;
			}

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			std::string prgDir = g_SharedWindowVar->prgDir;

			bminfo.bmiHeader.biSize = sizeof(bminfo.bmiHeader);
			bminfo.bmiHeader.biPlanes = 1;
			bminfo.bmiHeader.biBitCount = 32;
			bminfo.bmiHeader.biCompression = BI_RGB;

			hdc = BeginPaint(hwnd, &ps);

			hdc2 = CreateCompatibleDC(hdc);
			hOldPen = (HPEN) GetCurrentObject(hdc2, OBJ_PEN);
			hOldBrush = (HBRUSH) GetCurrentObject(hdc2, OBJ_BRUSH);
			hOldFont = (HFONT) SelectObject(hdc2, g_SharedWindowVar->hFont);

			SetBkMode(hdc2, TRANSPARENT);

			if (!hThumbListBitmap) {
				i = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				if (i == 0) {
					i = 1920;
				} j = GetSystemMetrics(SM_CYVIRTUALSCREEN);
				if (j == 0) {
					j = 1920;
				}
				hThumbListBitmap = CreateCompatibleBitmap(hdc, i, j);
			}
			hOldBM = (HBITMAP) SelectObject(hdc2, hThumbListBitmap);

			SelectObject(hdc2, g_SharedWindowVar->bgBrush1);
			SelectObject(hdc2, g_SharedWindowVar->bgPen1);
			Rectangle(hdc2, 0, 0, rect.right, rect.bottom);

// g_errorfStream << "ev->xpos: " << ev->xpos << ", ev->ypos: " << ev->ypos << std::flush; 
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
					g_errorfStream << "SetDIBitsToDevice failed, h: " << ev->DispImage->y << ", w: " << ev->DispImage->x << ", source: " << ev->DispImage->img[ev->dispframe] << std::flush;
					
				}
			}
			if (1 || ev->zoomp) { //! remove the "1 || " later
				SelectObject(hdc2, g_SharedWindowVar->hPen1);
				Rectangle(hdc2, 5, 5, 5+6+5*kCharWidth, 5+kRowHeight);
				buf = utoc(ev->zoomp);
				TextOutA(hdc2, 5+3, 5+(kRowHeight)-(kRowHeight/2)-7, buf, strlen(buf));
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
		}
		case WM_SIZE: {
/*			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == nullptr) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
*/
			PostMessage(hwnd, WM_USER, 0, 0);	// so no black border for expanded area
			break;
		}
		case WM_USER: {	// refresh image to window size
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == nullptr) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			std::string prgDir = g_SharedWindowVar->prgDir;

			if (ev->fit & kBitVI_FitModeEnabled) {

				if (!(ev->fit & kBitVI_FitDispIsOrig)) {
					if (ev->DispImage) {
						ev->DispImage = nullptr;
					}
				} else {
					ev->fit &= ~kBitVI_FitDispIsOrig;
				}

				if ((long) ev->FullImage->x > rect.right || (long) ev->FullImage->y > rect.bottom) {
					ev->DispImage = FitImage(ev->FullImage, rect.right, rect.bottom, 0, 0);
				} else {
					ev->fit |= kBitVI_FitDispIsOrig;
					ev->DispImage = ev->FullImage;
				}
				ev->xpos = ((int64_t)ev->DispImage->x - rect.right)/2;
				ev->ypos = ((int64_t)ev->DispImage->y - rect.bottom)/2;
				if (ev->DispImage == nullptr) {
					errorf("no DispImage");
					DestroyWindow(GetParent(hwnd));
					return 0;
				}

			} else {
				if (ev->fit & (kBitVI_FitResizeFitsX | kBitVI_FitResizeFitsY) && ev->xdisp < rect.right && ev->ydisp < rect.bottom) {	// if zoomed image fits
					ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
					ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
				} else {
					if (ev->xdisp <= rect.right) {
						l = ev->xdisp;		// width of the retrieved image is the same as the whole image zoomed
						ev->xpos = 0;
						ev->midX = (long double)ev->FullImage->x / 2;
					} else {
						l = rect.right;		// width is the width of window

						if (!(ev->fit & kBitVI_FitResizeFitsX)) {
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

						if (!(ev->fit & kBitVI_FitResizeFitsY)) {
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
					ev->DispImage = ResizeImageChunk(ev->FullImage, (double) ev->zoomp / 100, l, m, ev->xpos, ev->ypos, prgDir.c_str());

					ev->fit &= ~(kBitVI_FitResizeFitsX | kBitVI_FitResizeFitsY);
					if (ev->DispImage) {
						if (ev->xdisp <= rect.right) {
							ev->fit |= kBitVI_FitResizeFitsX;
							ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
						}
						if (ev->ydisp <= rect.bottom) {
							ev->fit |= kBitVI_FitResizeFitsY;
							ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
						}
					}
				}
			}
			InvalidateRect(hwnd, 0, 0);

			break;
		}
		case WM_MOUSEWHEEL: {		//! have zooming refresh dragging
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == nullptr) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if ((i = (int16_t)(HIWORD(wParam))/WHEEL_DELTA) == 0) {
				break;
			}
			if (!(ev->fit & kBitVI_FitModeEnabled) && ev->DispImage != nullptr) {	// to calculate middle position -- not needed if whole image in screen
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

			if (ev->fit & kBitVI_FitDispIsOrig) {	// clean up and initializing zoomp if previously fit to screen
				ev->fit &= ~kBitVI_FitDispIsOrig;
				ev->fit &= ~kBitVI_FitModeEnabled;
				ev->zoomp = 100;
			} else {
				if (ev->DispImage) {
					ev->DispImage = nullptr;
				}
				if (ev->fit & kBitVI_FitModeEnabled) {
					double zoom1, zoom2;
					zoom1 = (double) rect.right / ev->FullImage->x;
					zoom2 = (double) rect.bottom / ev->FullImage->y;
					if (zoom1 > zoom2) {
						zoom1 = zoom2;
					}
					ev->zoomp = zoom1*100;
					ev->fit &= ~kBitVI_FitModeEnabled;
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

				std::string prgDir = g_SharedWindowVar->prgDir;
				ev->DispImage = ResizeImageChunk(ev->FullImage, (double) ev->zoomp / 100, l, m, ev->xpos, ev->ypos, prgDir.c_str());
			}
			ev->fit &= ~(kBitVI_FitResizeFitsX | kBitVI_FitResizeFitsY);

			if (ev->DispImage) {
				if (ev->xdisp <= rect.right) {
					ev->fit |= kBitVI_FitResizeFitsX;
					ev->xpos = ((int64_t)ev->xdisp - rect.right)/2;
				}
				if (ev->ydisp <= rect.bottom) {
					ev->fit |= kBitVI_FitResizeFitsY;
					ev->ypos = ((int64_t)ev->ydisp - rect.bottom)/2;
				}
			}
			InvalidateRect(hwnd, 0, 0);

			break;
		}
		case WM_LBUTTONDOWN: {
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == nullptr) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			if (ev->drag)
				break;

			if ((ev->fit & kBitVI_FitResizeFitsX && ev->fit & kBitVI_FitResizeFitsY) || ev->fit & kBitVI_FitModeEnabled) {
				break;
			}
			ev->drag = 1;
			ev->xdrgpos = lParam & (256*256-1);
			ev->ydrgpos = lParam/256/256 & (256*256-1);
			ev->xdrgstart = ev->xpos;
			ev->ydrgstart = ev->ypos;

			SetCapture(hwnd);

			break;
		}
		case WM_LBUTTONUP: {
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == nullptr) {
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
		}
		case WM_MOUSEMOVE: {
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == nullptr) {
				errorf("ev is null");
				break;
			}

			SetCursor(g_SharedWindowVar->hDefCrs);
			if (!ev->drag) {
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			if (!(ev->fit & kBitVI_FitResizeFitsX)) {
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
			if (!(ev->fit & kBitVI_FitResizeFitsY)) {
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
					ev->DispImage = nullptr;
				}
				errorf("calling resizeimagechunk 3");
				std::string prgDir = g_SharedWindowVar->prgDir;
				ev->DispImage = ResizeImageChunk(ev->FullImage, (double) ev->zoomp / 100, l, m, j, k, prgDir.c_str());
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
		}
		case WM_TIMER: {	//! zooming and changing stretch mode in the future should kill the timer
			ev = (IMGVIEWV*) SendMessage(GetParent(hwnd), WM_U_RET_VI, (WPARAM) GetMenu(hwnd), 0);
			if (ev == nullptr) {
				errorf("ev is null");
				break;
			}
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			if (wParam == 1) {

				if (!(ev->fit & kBitVI_FitResizeFitsX)) {
					l = rect.right;
					j = ev->xpos;
				} else {
					l = ev->xdisp;
					j = 0;
				}
				if (!(ev->fit & kBitVI_FitResizeFitsY)) {
					m = rect.bottom;
					k = ev->ypos;
				} else {
					m = ev->ydisp;
					k = 0;
				}
				if (ev->DispImage) {
					ev->DispImage = nullptr;
				}
				if (ev->xpos < 0 && !(ev->fit & kBitVI_FitResizeFitsX)) {
					g_errorfStream << "ev->xpos is less than 0 under WM:TIMER: " << ev->xpos << std::flush;
//					ev->xpos = 0;
				}
				if (ev->ypos < 0 && !(ev->fit & kBitVI_FitResizeFitsY)) {
					g_errorfStream << "ev->ypos is less than 0 under WM_TIMER: " << ev->ypos << std::flush;
//					ev->ypos = 0;
				}
				errorf("calling resizeimagechunk BIT_VI_FIT_RESIZE_FITS_X");
				std::string prgDir = g_SharedWindowVar->prgDir;
				ev->DispImage = ResizeImageChunk(ev->FullImage, (double) ev->zoomp / 100, l, m, j, k, prgDir.c_str());
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
				if (ev->dispframe >= (long) (ev->FullImage->n - 1)) {
					ev->dispframe = 0;
				} else {
					ev->dispframe++;
				}

				KillTimer(hwnd, 2);
				SetTimer(hwnd, 2, ev->FullImage->dur[ev->dispframe]*10, 0);
				InvalidateRect(hwnd, 0, 0);
			}
			break;
		}
		case WM_KEYDOWN: {
			switch(wParam) {
			case VK_BACK:
				PostMessage(GetParent(hwnd), WM_U_VI, (WPARAM) GetMenu(hwnd), 1);

				break;
			}
			break;
		}
		case WM_ERASEBKGND: {
			return 1;
			break;
		}
		case WM_DESTROY: {
			return 0;
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ FileTagEditWindow
//public:
// static
const WindowHelper FileTagEditWindow::helper = WindowHelper(std::wstring(L"FileTagEditWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		wc.hCursor = nullptr;
	}
);

// static
HWND FileTagEditWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new FileTagEditWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT FileTagEditWindow::onCreate(WinProcArgs procArgs) {

/*

errorf("fte create1");
	ev = AllocWindowMem(hwnd, sizeof(FTEV));
	ev->dnum = 0, ev->fnumchn = ev->tagnumchn = ev->aliaschn = 0;
	ev->rmnaliaschn = ev->regaliaschn = ev->remtagnumchn = ev->addtagnumchn = 0;

	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	EditWindowSuperClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 5, 5, 350, 200, hwnd, (HMENU) 1, nullptr, nullptr));

	SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_shared_window_vars.hFont, 1);
//			SendDlgItemMessageW(hwnd, 1, EM_SETTEXTMODE, TM_PLAINTEXT, 0);

	EnableWindow(GetDlgItem(hwnd, 1), 0); // disable textedit until it is filled by tags

	CreateWindowW(L"Button", L"Apply", WS_VISIBLE | WS_CHILD, 5, 225, 80, 25, hwnd, (HMENU) 2, nullptr, nullptr);
	SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) g_shared_window_vars.hFont2, 1);
errorf("fte create9");
*/


	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK FileTagEditWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

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
		case WM_CREATE: {

			break;
		}
		case WM_PAINT: {

			break;
		}
		case WM_SIZE: {

			break;
		}
		case WM_MOUSEMOVE: {
			SetCursor(g_shared_window_vars.hDefCrs);

			break;
		}
		case WM_USER: {	// input dnum or fnums
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
		}
		case WM_USER+1: {	// apply changes
			ev = GetWindowMem(hwnd);

errorf("applying changes");
			if (ev->rmnaliaschn) {
				if (!(CreateAliasWindow::createWindowInstance().create(0, L"", WS_VISIBLE | WS_POPUP, 5, 5, 200, 200, hwnd, 0, nullptr, nullptr))) {
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
g_errrofStream << "addtagnumchn" << ev->addtagnumchn << ", regaliaschn" << ev->regaliashchn << ", remtagnumchn: " << ev->remtagnumchn << std::flush;
link1 = ev->regaliaschn;
while (link1) {
	g_errorfStream << "regalias: " << ev->regaliaschn->str << std::flush;
	link1 = link1->next;
}
				twoWayTagChainRegAddRem(ev->dnum, ev->fnumchn, ev->addtagnumchn, ev->regaliaschn, ev->remtagnumchn, 1+2+4);

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
		}
		case WM_USER+2: {		load tags
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

				if (chainFileReadTag(ev->dnum, ev->fnumchn, 0, &link1, 1)) {
					errorf("chainFileReadTag failed");
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

					chainTagRead(ev->dnum, ev->tagnumchn, &ev->aliaschn, 0, 1);
errorf("made it after chainTagRead");

					if (!ev->aliaschn) {
						errorf("chainTagRead failed");
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
		}
		case WM_ERASEBKGND: {
//			return 1;
			break;
		}
		case WM_COMMAND: {
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

						if ((WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, len*4, nullptr, nullptr)) == 0) {
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
	// g_errorfStream << "added aliases: " << link1->str << std::flush;
	// link1 = link1->next;
// }
// link1 = remaliaschn;
// while (link1) {
	// g_errorfStream << "removed aliases: " << link->str << std::flush;
	// link1 = link1->next;
// }

					if (remaliaschn) {
errorf("aliases to be removed exist");
link1 = remaliaschn;
while (link1) {
	g_errorfStream << "removing tag with alias: " << link->str << std::flush;
	link1 = link1->next;
}
						ev->remtagnumchn = tagNumFromAlias(ev->dnum, remaliaschn, &ev->rmnaliaschn);	//! add option to limit to primary aliases
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
	g_errorfStream << "removing tag num: " << link1->ull << std::flush;
	link1 = link1->next;
}
					} else {
						ev->remtagnumchn = 0;
					}
					if (addaliaschn) {
						ev->addtagnumchn = tagNumFromAlias(ev->dnum, addaliaschn, &ev->rmnaliaschn);
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

g_errorfStream << "ev->rmnaliaschn" << ev->rmnaliaschn << ", ev->addtagnumchn: " << ev->addtagnumchn << ", ev->remtagnumchn: " << ev->remtagnumchn << std::flush;
link1 = ev->rmnaliaschn;
while (link1) {
	g_errorfStream << "remaining aliases: " << link1->str << std::flush;
	link1 = link1->next;
}
link1 = ev->addtagnumchn;
while (link1) {
	g_errorfStream << "adding tag num: " << link1->ull << std::flush;
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
	g_errorfStream << "ev->rmnaliaschn" << ", ev->regaliaschn" << ev->regaliaschn << ", ev->remtagnumchn" << ev->remtagnumchn << ", ev->addtagnumchn: " << ev->addtagnumchn << std::flush;
	

}
			}
errorf("fte comm 9");
			break;
		}
		case WM_DESTROY: {
errorf("fte destroy1");
			ev = GetWindowMem(hwnd);
errorf("fte destroy2");
			if (ev->fnumchn) {
				killoneschn(ev->fnumchn, 1);
			}
errorf("fte destroy2.1");
g_errorfStream << "ev->tagnumchn: " << ev->tagnumchn << std::flush;
link1 = ev->tagnumchn;
while (link1) {
	g_errorfStream << "num: " << link1->ull << std::flush;
	g_errorfStream << "link1->next: " << link1->next << std::flush;
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

g_errorfStream << "parent: " << GetParent(hwnd) << std::flush;
			SetFocus(GetParent(hwnd));

			return 0;
		}
	}
*/
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ CreateAliasWindow
//public:
// static
const WindowHelper CreateAliasWindow::helper = WindowHelper(std::wstring(L"CreateAliasWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = 0;
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		wc.hCursor = nullptr;
	}
);

// static
HWND CreateAliasWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new CreateAliasWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CreateAliasWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	LPARAM &lParam = procArgs.lParam;
	WPARAM &wParam = procArgs.wParam;

	ShowWindow(hwnd, 0);
	HWND thwnd = GetParent(hwnd);
	if(!(thwnd)) {
		errorf("couldn't get createalias parent");
		return -1;
	}
	//! TODO: make the casting safer
	if (!(this->parent_ = std::dynamic_pointer_cast<FileTagEditWindow>(WindowClass::getWindowPtr(thwnd)))) {
		errorf("couldn't get parent window memory for createalias");
		return -1;
	}

	PostMessage(hwnd, WM_USER, 0, 0);


	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK CreateAliasWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int64_t i;

	HDC hdc;
	PAINTSTRUCT ps;
	HFONT hOldFont;
	HPEN hOldPen;
	HBRUSH hOldBrush;

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_PAINT: {

			break;
		}
		case WM_USER: {
			HWND thwnd = GetParent(hwnd);
			if(!(thwnd)) {
				errorf("couldn't get createalias parent");
				return -1;
			}

			this->parent_->regaliaschn_ = this->parent_->rmnaliaschn_;
			this->parent_->rmnaliaschn_ = 0;
			PostMessage(thwnd, WM_USER+1, 0, 0);
			DestroyWindow(hwnd);

			break;
		}
		case WM_SIZE: {

			break;
		}
		case WM_ERASEBKGND: {
//			return 1;
			break;
		}
		case WM_COMMAND: {
		}
		case WM_DESTROY: {
			return 0;
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//}

//{ EditWindowSuperClass
//public:
// static
DeferredRegWindowHelper EditWindowSuperClass::helper(DeferredRegWindowHelper(std::wstring(L"EditWindowSuperClass"),
	[](WNDCLASSW &wc) -> void {
		WNDCLASSW t_wc = {0};
errorf("ESC helper 1");
		int c = GetClassInfoW(nullptr, MSFTEDIT_CLASS, &t_wc);
		if (!c) {
			g_errorfStream << "GetClassInfoW failed" << "\n";
		}
		if (t_wc.lpfnWndProc == nullptr) {
			errorf("t_wc has no proc");
		}

		wc.style = t_wc.style;
		wc.cbClsExtra = t_wc.cbClsExtra;
		wc.cbWndExtra = t_wc.cbWndExtra;
		wc.hIcon = t_wc.hIcon;
		wc.hCursor = t_wc.hCursor;
		wc.hbrBackground = t_wc.hbrBackground;
		wc.lpszMenuName = t_wc.lpszMenuName;

		g_SharedWindowVar->g_OldEditProc = t_wc.lpfnWndProc;
errorf("ESC helper 2");

	}
));

// static
HWND EditWindowSuperClass::createWindowInstance(WinInstancer wInstancer) {
	// has to be registered before window instance is created
	if (!EditWindowSuperClass::helper.isRegistered()) {
		errorf("createWindowInstance: ESC not registered");
		return nullptr;
	}
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new EditWindowSuperClass(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT EditWindowSuperClass::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	LPARAM &lParam = procArgs.lParam;
	WPARAM &wParam = procArgs.wParam;

errorf("creating editsuperclass inner");
	//ShowWindow(hwnd, 0);

	//PostMessage(hwnd, WM_USER, 0, 0);

	return CallWindowProcW(g_SharedWindowVar->g_OldEditProc, hwnd, procArgs.msg, wParam, lParam);

	// return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK EditWindowSuperClass::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	//return CallWindowProcW(g_shared_window_vars.g_OldEditProc, hwnd, msg, wParam, lParam);

	RECT rect;
	HWND thwnd;

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_KEYDOWN: {
			switch(wParam) {
			case VK_TAB:
				break;
			case VK_RETURN:
				break;
			default:
				return CallWindowProcW(g_SharedWindowVar->g_OldEditProc, hwnd, msg, wParam, lParam);
			}
			break;
		}
		case WM_USER: {
			ShowWindow(hwnd, 1);
		}
		case WM_CHAR: {
			switch(wParam) {
			case '\t':
//				break;
			case '\n':
//				break;
			case '\r':
//				break;
			default:
				return CallWindowProcW(g_SharedWindowVar->g_OldEditProc, hwnd, msg, wParam, lParam);
			}
			break;
		}
		default:
			return CallWindowProcW(g_SharedWindowVar->g_OldEditProc, hwnd, msg, wParam, lParam);
	}
	return CallWindowProcW(g_SharedWindowVar->g_OldEditProc, hwnd, msg, wParam, lParam);
}


//}

//{ SearchBarWindow
//public:
// static
const WindowHelper SearchBarWindow::helper = WindowHelper(std::wstring(L"SearchBarWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		wc.hCursor = nullptr;
	}
);

// static
HWND SearchBarWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new SearchBarWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT SearchBarWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	LPARAM &lParam = procArgs.lParam;
	WPARAM &wParam = procArgs.wParam;

	// TODO: make enum instead

	RECT rect;
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}
	EditWindowSuperClass::createWindowInstance(WinInstancer(0, L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 5, 5, rect.right - 5 - kSearchBarButtonX - 5 - 5, kSearchBarTextbH, hwnd, (HMENU) 1, nullptr, nullptr));
	CreateWindowW(L"Button", L"", WS_VISIBLE | WS_CHILD, rect.right - 5 - kSearchBarButtonX, 5, kSearchBarButtonX, kSearchBarButtonX, hwnd, (HMENU) 2, nullptr, nullptr);

	return WindowClass::onCreate(procArgs);
}

LRESULT CALLBACK SearchBarWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_SIZE: {
			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}

			thwnd = GetDlgItem(hwnd, 1);
			MoveWindow(thwnd, 5, 5, rect.right - 5 - kSearchBarButtonX - 5 - 5, kSearchBarTextbH, 0);
			thwnd = GetDlgItem(hwnd, 2);
			MoveWindow(thwnd, rect.right - 5 - kSearchBarButtonX, 5, kSearchBarButtonX, kSearchBarButtonX, 0);
		}
		case WM_COMMAND: {
			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) == 2) {
					thwnd = GetDlgItem(hwnd, 1);
					CleanEditText(thwnd);

					wchar_t *wbuf;
					int64_t len;
					char *buf = nullptr;

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
		}
		case WM_KEYDOWN: {
		}
		case WM_CHAR: {
		}
		default:
			return CallWindowProcW(g_SharedWindowVar->g_OldEditProc, hwnd, msg, wParam, lParam);
	}
}


//} 

//{ TextEditDialogWindow
//public:
// static
const WindowHelper TextEditDialogWindow::helper = WindowHelper(std::wstring(L"TextEditDialogWindow"),
	[](WNDCLASSW &wc) -> void {
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		wc.hCursor = nullptr;
	}
);

LRESULT TextEditDialogWindow::onCreate(WinProcArgs procArgs) {
	HWND &hwnd = procArgs.hwnd;
	LPARAM &lParam = procArgs.lParam;
	WPARAM &wParam = procArgs.wParam;

	RECT rect;
	if (GetClientRect(hwnd, &rect) == 0) {
		errorf("GetClientRect failed");
	}

	EditWindowSuperClass::createWindowInstance(WinInstancer(0, L"Default name", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 5, 5, 350, 200, hwnd, (HMENU) 1, nullptr, nullptr));

	SendDlgItemMessageW(hwnd, 1, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont, 1);
	SendDlgItemMessageW(hwnd, 1, EM_SETTEXTMODE, TM_PLAINTEXT, 0);

	CreateWindowW(L"Button", L"Apply", WS_VISIBLE | WS_CHILD, 5, 225, 80, 25, hwnd, (HMENU) 2, nullptr, nullptr);
	SendDlgItemMessageW(hwnd, 2, WM_SETFONT, (WPARAM) g_SharedWindowVar->hFont2, 1);

	//PostMessage(hwnd, WM_USER+1, 0, 0);

	return WindowClass::onCreate(procArgs);
}

// static
HWND TextEditDialogWindow::createWindowInstance(WinInstancer wInstancer) {
	std::shared_ptr<WinCreateArgs> winArgs = std::shared_ptr<WinCreateArgs>(new WinCreateArgs([](void) -> WindowClass* { return new TextEditDialogWindow(); }));
	return wInstancer.create(winArgs, helper.winClassName_);
}

LRESULT CALLBACK TextEditDialogWindow::winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	HWND thwnd;

	int64_t i, len;

	HDC hdc;
	PAINTSTRUCT ps;
	HFONT hOldFont;
	HPEN hOldPen;
	HBRUSH hOldBrush;

	switch(msg) {
		case WM_CREATE: {
			return this->onCreate(WinProcArgs(hwnd, msg, wParam, lParam));

			break;
		}
		case WM_PAINT: {

			break;
		}
		case WM_SIZE: {

			break;
		}
		case WM_MOUSEMOVE: {
			SetCursor(g_SharedWindowVar->hDefCrs);

			break;
		}
		case WM_USER+1: {
			//ev = GetWindowMem(hwnd);

			//FreeWindowMem(hwnd);

			//SetFocus(GetParent(hwnd));

			{
				const char *buf = "Default name";

				SendMessage(GetParent(hwnd), WM_U_RET_TED, (WPARAM) buf, (LPARAM) 0);
				DestroyWindow(hwnd);

			}

			//free(buf);

			break;
		}
		case WM_COMMAND: {
			if (HIWORD(wParam) == EN_UPDATE && !this->clean_flag_) {
				thwnd = GetDlgItem(hwnd, 1);
				this->clean_flag_ = 1;
				CleanEditText(thwnd);
				this->clean_flag_ = 0;
			} else if (HIWORD(wParam) == BN_CLICKED) { //! handle getting pressed when already pressed
				if (LOWORD(wParam) == 2) {

					thwnd = GetDlgItem(hwnd, 1);
					this->clean_flag_ = 1;
					CleanEditText(thwnd);
					this->clean_flag_ = 0;

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

						if ((WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, len*4, nullptr, nullptr)) == 0) {
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
		}
		case WM_NCDESTROY: {
			//ev = GetWindowMem(hwnd);

			//FreeWindowMem(hwnd);

			SetFocus(GetParent(hwnd));

			return 0;
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

//}