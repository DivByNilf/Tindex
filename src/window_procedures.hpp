#pragma once

#include <windows.h>
#include <cstdint>

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

	HCURSOR hDefCrs, hCrsSideWE, hCrsHand;
    std::shared_ptr<void> hMapFile;
};

struct WinProcArgs {
	HWND hwnd;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
	std::shared_ptr<SharedWindowData> sharedWinData;
	std::ostream &errorfStream;
	const std::function<std::shared_ptr<WindowClass>(HWND)> getWindowPtr;
};

struct WinProcData {
	std::map<HWND, std::shared_ptr<WindowClass>> winMemMap;
	std::ostream &errorfStream;
	std::shared_ptr<SharedWindowData> sharedWinData;
};

//}

class WinCreateArgs;

void PingExistingProcess(HWND);

void MainInitHandles(WinProcData &, std::ostream &errorfStream);
void MainDeInitHandles(WinProcData &);

bool SetWinProcData(std::shared_ptr<WinProcData> winProcData);
void ReleaseWinProcData();