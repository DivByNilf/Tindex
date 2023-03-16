#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

#include "userinterface.hpp"
#include "window_procedures.hpp"
#include "portables.hpp"
#include "prgdir.hpp"

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
int RunMessageLoop(void);

int CheckInitMutex(HANDLE &hMutex, std::string prgDir);
int MainInit(MainInitStruct &ms, std::shared_ptr<SharedWindowVariables> sharedWindowVars);
int MainDeInit(MainInitStruct &ms, std::shared_ptr<SharedWindowVariables> sharedWindowVars);

void MainInitHandles(SharedWindowVariables &);
void MainDeInitHandles(SharedWindowVariables &);

/// global variables
std::filesystem::path g_fsPrgDir;

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
	auto sharedWindowVars = std::make_shared<SharedWindowVariables>();
	sharedWindowVars->ghInstance = hInstance;

	{
		bool res = SetSharedWindowVariables(sharedWindowVars);
		if (!res) {
			g_errorfStream << "Preinit: SetSharedWindowVariables failed" << std::flush;
			return 1;
		}
	}

	MainInitStruct ms = {};
	{
		int32_t i = 0;
		if ((i = MainInit(ms, sharedWindowVars)) != 0) {
			if (i == kMainInitMutexReserved) { // mutex already reserved
				return 0;
			} else {
				g_errorfStream << "Preinit: MainInit failed: " << i << std::flush;
				return 1;
			}
		}
	}
	HWND hMsgHandler = MsgHandler::createWindowInstance(WinInstancer(0, L"FileTagIndex", WS_OVERLAPPEDWINDOW, 100, 100, 100, 100, HWND_MESSAGE, nullptr, hInstance, nullptr));

	// try to prevent program opening and taking mutex with no real window
	if (!hMsgHandler || !WindowClass::getWindowPtr(hMsgHandler)) {
		errorf("Failed to create MsgHandler (main)");
		return 1;
	}

	int loopReturn = RunMessageLoop();
	{
		int32_t i = 0;
		if ((i = MainDeInit(ms, sharedWindowVars)) != 0) {
			g_errorfStream << "MainDeInit failed: " << i << std::flush;
			return 1;
		}
	}
	return loopReturn;
}

int RunMessageLoop(void) {
	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

int MainInit(MainInitStruct &ms, std::shared_ptr<SharedWindowVariables> sharedWindowVars) {
	if (ms != MainInitStruct()) {
		errorf("Preinit: MainInitStruct was not empty");
		return kMainInitFail;
	}
	{
		g_fsPrgDir = GetPrgDir();
		if (g_fsPrgDir.empty()) {
			errorf("Preinit: GetPrgDir failed");
			return kMainInitFail;
		}
		sharedWindowVars->prgDir = g_fsPrgDir.generic_string();

		bool res = SetErrorfStaticPrgDir(g_fsPrgDir.generic_string());
		if (!res) {
			errorf("Preinit: SetErrorfStaticPrgDir failed");
			return kMainInitFail;
		}
	}

	if (CheckInitMutex(ms.hMutex, g_fsPrgDir.string()) != 0) { // requires PrgDir to be inited
		errorf("Preinit: CheckInitMutex failed");
	}

	if (ms.hMutex == nullptr) {
		return kMainInitMutexReserved;
	}
	// load dll for richedit windows
	{
		HMODULE hModule = LoadLibraryW(L"Msftedit.dll");
		if (hModule == nullptr) {
			errorf("failed to load Msftedit.dll");
			return kMainInitDllLoadFail;
		}
	}
	{
		bool b1 = EditWindowSuperClass::helper.registerWindowClass();
		if (!b1) {
			errorf("ESC helper registerWindowClass failed");
			return kMainInitRegisterClassFail;
		}
	}

	MainInitHandles(*sharedWindowVars);

	return 0;

}

int CheckInitMutex(HANDLE &hMutex, std::string prgDir) {
	std::string mutexName = prgDir;
	std::replace(mutexName.begin(), mutexName.end(), '\\', '/');
	mutexName = "Global\\" + mutexName;

	hMutex = CreateMutexW(nullptr, TRUE, u8_to_u16(mutexName).c_str());

	if (hMutex == nullptr) {
		g_errorfStream << "Preinit: CreateMutex" << "\n  error: " << GetLastError() << "\n  mutexName" << mutexName << std::flush;
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
			g_errorfStream << "Preinit: Could not open file mapping object (" << GetLastError() << ")." << std::flush;
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
			g_errorfStream << "Preinit: Could not map view of file (" << GetLastError() << ")." << std::flush;
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

int MainDeInit(MainInitStruct &ms, std::shared_ptr<SharedWindowVariables> sharedWindowVars) {
	if (ms.hMutex != nullptr) {
		CloseHandle(ms.hMutex);
	}

	MainDeInitHandles(*sharedWindowVars);

	return 0;
}