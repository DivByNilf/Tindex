#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

#include "userinterface.hpp"
#include "window_procedures.hpp"
#include "portables.hpp"
#include "prgdir.hpp"
#include "indextools.hpp"

extern "C" {
	#include "dupstr.h"
	#include "portables.h"
}

#include <windows.h>

#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}

/// private declarations

// LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CheckInitMutex(HANDLE &hMutex, std::string prgDir);
int MainInit(MainInitStruct &ms, SharedWindowData &sharedWindowData, const std::fs::path &prgDir);
int MainDeInit(MainInitStruct &ms, SharedWindowData &sharedWindowData);

void MainInitHandles(SharedWindowData &);
void MainDeInitHandles(SharedWindowData &);

/// enums
enum MainInitReturnCodes : int32_t {
	kMainInitFail = 1, 
	kMainInitMutexReserved = 2,
	kMainInitDllLoadFail = 3,
	kMainInitRegisterClassFail = 4,
};

enum OtherOptionIntegers : int32_t {
	kHWndBufSize = 256
};

/// Initialization functions

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
	
	auto winProcData = std::make_shared<WinProcData>();
	winProcData->sharedWinData.ghInstance = hInstance;
	// TODO: make remove g_std::cerr and initialize an std::cerr here
	std::fs::path prgDir = GetPrgDir();
	if (prgDir.empty()) {
		ErrorfDialogStdStr("Preinit: GetPrgDir failed");
		return kMainInitFail;
	}
	freopen((prgDir.string() + "/errorfile.log").c_str(), "a", stderr);
	freopen((prgDir.string() + "/errorfile.log").c_str(), "a", stdout);
	std::cerr << "\n---\n";
	{
		bool res = SetWinProcData(winProcData);
		if (!res) {
			std::cerr << "Preinit: SetSharedWindowData failed" << std::flush;
			return 1;
		}
	}

	MainInitStruct ms = {};
	{
		int32_t i = 0;
		if ((i = MainInit(ms, winProcData->sharedWinData, prgDir)) != 0) {
			if (i == kMainInitMutexReserved) { // mutex already reserved
				return 0;
			} else {
				std::cerr << "Preinit: MainInit failed: " << i << std::flush;
				return 1;
			}
		}
	}
	winProcData->sharedWinData.winRegs = GetWinRegStructs();
	RegisterWindowClasses(winProcData->sharedWinData.winRegs);
	HWND hMsgHandler = MsgHandler::createWindowInstance(WinInstancer(0, L"Tindex", WS_OVERLAPPEDWINDOW, 100, 100, 100, 100, HWND_MESSAGE, nullptr, hInstance, nullptr), winProcData->sharedWinData.winRegs.msgHandler.winClassName);

	// try to prevent program opening and taking mutex with no real window
	if (!hMsgHandler || !GetWindowPtr(hMsgHandler, winProcData->winMemMap)) {
		std::cerr << "Failed to create MsgHandler (main)" << std::flush;
		return 1;
	}

	// Message Loop
	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	int loopReturn = msg.wParam;
	{
		int32_t i = 0;
		if ((i = MainDeInit(ms, winProcData->sharedWinData)) != 0) {
			std::cerr << "MainDeInit failed: " << i << std::flush;
			return 1;
		}
	}
	return loopReturn;
}

int MainInit(MainInitStruct &ms, SharedWindowData &sharedWindowData, const std::fs::path &prgDir) {
	if (ms != MainInitStruct()) {
		std::cerr << "Preinit: MainInitStruct was not empty" << std::flush;
		return kMainInitFail;
	}
	sharedWindowData.prgDir = prgDir.generic_string();

	if (CheckInitMutex(ms.hMutex, prgDir.string()) != 0) { // requires PrgDir to be inited
		std::cerr << "Preinit: CheckInitMutex failed" << std::flush;
	}

	if (ms.hMutex == nullptr) {
		return kMainInitMutexReserved;
	}
	// load dll for richedit windows
	{
		HMODULE hModule = LoadLibraryW(L"Msftedit.dll");
		if (hModule == nullptr) {
			std::cerr << "failed to load Msftedit.dll" << std::flush;
			return kMainInitDllLoadFail;
		}
	}

	MainInitHandles(sharedWindowData);

	initSessionHandler(prgDir);

	return 0;

}

int CheckInitMutex(HANDLE &hMutex, std::string prgDir) {
	std::string mutexName = prgDir;
	std::replace(mutexName.begin(), mutexName.end(), '\\', '/');
	mutexName = "Global\\" + mutexName;

	hMutex = CreateMutexW(nullptr, TRUE, u8_to_u16(mutexName).c_str());

	if (hMutex == nullptr) {
		std::cerr << "Preinit: CreateMutex" << "\n  error: " << GetLastError() << "\n  mutexName" << mutexName << std::flush;
		return 1;
	}

	DWORD dwWaitResult = WaitForSingleObject(hMutex, 0);

	bool disableMutexCheck = false;

 	// if mutex is already taken
	if (!disableMutexCheck && dwWaitResult != WAIT_OBJECT_0 && dwWaitResult != WAIT_ABANDONED) {
		CloseHandle(hMutex);
		hMutex = nullptr;

		std::string fileMapStr = prgDir;
		std::replace(fileMapStr.begin(), fileMapStr.end(), '\\', '/');
		fileMapStr += "/HWND";

		std::wstring fileMapName = u8_to_u16(fileMapStr);

		auto hMapFile = std::shared_ptr<void>(
			OpenFileMappingW(
				FILE_MAP_ALL_ACCESS, // read/write access
				FALSE, // do not inherit the name
				fileMapName.c_str()),
			 CloseHandle
		);
		
		if (hMapFile == nullptr) {
			std::cerr << "Preinit: Could not open file mapping object (" << GetLastError() << ")." << std::flush;
			return 1;
		}

		std::shared_ptr<void> pBuf = std::shared_ptr<void>(
			MapViewOfFile(
				hMapFile.get(),
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				kHWndBufSize
			),
			UnmapViewOfFile
		);

		if (pBuf == nullptr) {
			std::cerr << "Preinit: Could not map view of file (" << GetLastError() << ")." << std::flush;
			return 1;
		}

		HWND thwnd = 0;
		CopyMemory((PVOID) &thwnd, (VOID*) pBuf.get(), sizeof(HWND));

		// Send message to prior process that has the program open
		// For example: open a new window when the program is reopened
		PingExistingProcess(thwnd);
	}

	return 0;
}

int MainDeInit(MainInitStruct &ms, SharedWindowData &sharedWindowData) {
	if (ms.hMutex != nullptr) {
		CloseHandle(ms.hMutex);
	}
	ReleaseWinProcData();

	MainDeInitHandles(sharedWindowData);

	return 0;
}