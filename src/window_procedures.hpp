#pragma once

#include <windows.h>
#include <cstdint>

struct SharedWindowVariables {
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

void PingExistingProcess(HWND);

void MainInitHandles(SharedWindowVariables &);
void MainDeInitHandles(SharedWindowVariables &);

bool SetSharedWindowVariables(std::shared_ptr<SharedWindowVariables> sharedWindowVar);