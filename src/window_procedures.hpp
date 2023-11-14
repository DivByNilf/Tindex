#pragma once

#include <windows.h>
#include <cstdint>

struct WinProcArgs;
struct SharedWindowData;

struct WinRegStruct {
	std::wstring winClassName;
	std::function<void(WNDCLASSW &wc)> modifyWinStruct;
};

struct WinRegStructs {
	struct WinRegStruct msgHandler;
	struct WinRegStruct tabContainer;
	struct WinRegStruct tabWindow;
	struct WinRegStruct mainIndexMan;
	struct WinRegStruct dirMan;
	struct WinRegStruct subDirMan;
	struct WinRegStruct thumbMan;
	struct WinRegStruct pageList;
	struct WinRegStruct strList;
	struct WinRegStruct thumbList;
	struct WinRegStruct viewImage;
	struct WinRegStruct fileTagEdit;
	struct WinRegStruct createAlias;
	struct WinRegStruct editWindowSuper;
	struct WinRegStruct searchBar;
	struct WinRegStruct textEditDialog;
};

class WindowClass {
public:
	virtual ~WindowClass(void) = default;
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, WinProcArgs &args) = 0;

	virtual LRESULT onCreate(WinProcArgs &procArgs);
};

struct WinProcArgs {
	HWND hwnd;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
	SharedWindowData &sharedWinData;
	const std::function<std::shared_ptr<WindowClass>(HWND)> getWindowPtr;
};

struct SharedWindowData {
	HINSTANCE ghInstance;
	DWORD color;
	HFONT hFont, hFont2, hFontUnderlined;
	HPEN bgPen1, bgPen2, bgPen3, bgPen4, bgPen5, selPen, selPen2, hPen1;
	HBRUSH bgBrush1, bgBrush2, bgBrush3, bgBrush4, bgBrush5, selBrush;
	HBITMAP hListSliceBM, hThumbListBM;
	WNDPROC g_OldEditProc;
	uint64_t wMemArrLen, nwMemWnds;
    std::string prgDir;
	struct WinRegStructs winRegs;

	HCURSOR hDefCrs, hCrsSideWE, hCrsHand;
    std::shared_ptr<void> hMapFile;
};

struct WinProcData {
	std::map<HWND, std::shared_ptr<WindowClass>> winMemMap;
	SharedWindowData sharedWinData;
};

//}

class WinCreateArgs;

void PingExistingProcess(HWND);

void MainInitHandles(WinProcData &);
void MainDeInitHandles(WinProcData &);

bool SetWinProcData(std::shared_ptr<WinProcData> winProcData);
void ReleaseWinProcData();

struct WinRegStructs GetWinRegStructs(void);
int32_t RegisterWindowClasses(const WinRegStructs &winRegs);
void registerWinClass(const struct WinRegStruct &winRegStruct);