#include "windows.h"
#include <shobjidl.h>

#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}


#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

#include "uiutils.hpp"
#include "portables.hpp"
#include "prgdir.hpp"
#include "errorobj.hpp"

//! TODO: clean away old function
/*
HRESULT SeekDir(HWND hwnd, char *retstr) {
	// CoCreate the File Open Dialog object.
	//IFileDialog comes from <shobjidl.h>
	IFileDialog *pfd = NULL; 	// the pointer to the object about to be created
	HRESULT hr;
	
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(hr) || 1) {
		if (!SUCCEEDED(hr)) {
			//errorf_old("hr: %d", hr);
			g_errorfStream << "(SeekDir) hr: " << hr << std::flush;
			//errorf_old("%d, %d %d, %d, %d, %d", E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED, S_OK, S_FALSE, RPC_E_CHANGED_MODE);
			g_errorfStream << E_INVALIDARG << ", " << E_OUTOFMEMORY << ", " << E_UNEXPECTED << ", " << S_OK << ", " << S_FALSE << ", " << RPC_E_CHANGED_MODE << std::flush;
			errorf("Can't initialize COM");
		}
		
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void *) &pfd);
		if (SUCCEEDED(hr)) {
			if (SUCCEEDED(hr)) {
				DWORD dwFlags;
				
				hr = pfd->GetOptions(&dwFlags);	// getting options first as to not override them
				if (SUCCEEDED(hr)) {
					hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
					if (SUCCEEDED(hr)) {
						hr = pfd->Show(hwnd);
						if (SUCCEEDED(hr)) {
							IShellItem *psiResult;
							hr = pfd->GetResult(&psiResult);
							if (SUCCEEDED(hr)) {
								PWSTR pszFilePath = NULL;
								hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
								if (SUCCEEDED(hr)) {
									if ((WideCharToMultiByte(65001, 0, pszFilePath, -1, retstr, MAX_PATH*4, NULL, NULL)) == 0) {
										errorf("WideCharToMultiByte Failed");
									}
									CoTaskMemFree(pszFilePath);
								}
								psiResult->Release();
							}
						}
					}
				}
			}
			pfd->Release();
		} else {
			errorf("Can't create IExample object. CoCreateInstance error");
		}
		CoUninitialize();
	} else {
		//errorf_old("hr: %d", hr);
		g_errorfStream << "(SeekDir) hr: " << hr << std::flush;
		//errorf_old("%d, %d %d, %d, %d, %d", E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED, S_OK, S_FALSE, RPC_E_CHANGED_MODE);
		g_errorfStream << E_INVALIDARG << ", " << E_OUTOFMEMORY << ", " << E_UNEXPECTED << ", " << S_OK << ", " << S_FALSE << ", " << RPC_E_CHANGED_MODE << std::flush;
		errorf("Can't initialize COM"); 
	}
	return hr;
}
*/

std::fs::path SeekDir(const HWND &hwnd, HRESULT *resultPtr) {
	if (resultPtr != nullptr) {
		resultPtr = 0;
	}
	// CoCreate the File Open Dialog object.
	//IFileDialog comes from <shobjidl.h>
	IFileDialog *pfd = NULL; 	// the pointer to the object about to be created
	HRESULT hr;
	std::fs::path retPath;
	
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	
	if (SUCCEEDED(hr) || 1) {
		if (!SUCCEEDED(hr)) {
			g_errorfStream << "(SeekDir) hr: " << hr << std::flush;
			g_errorfStream << E_INVALIDARG << ", " << E_OUTOFMEMORY << ", " << E_UNEXPECTED << ", " << S_OK << ", " << S_FALSE << ", " << RPC_E_CHANGED_MODE << std::flush;
			errorf("Can't initialize COM");
		}
		
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (LPVOID *) &pfd);
		if (SUCCEEDED(hr)) {
			if (SUCCEEDED(hr)) {
				DWORD dwFlags;
				
				hr = pfd->GetOptions(&dwFlags);	// getting options first as to not override them
				if (SUCCEEDED(hr)) {
					hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
					if (SUCCEEDED(hr)) {
						hr = pfd->Show(hwnd);
						if (SUCCEEDED(hr)) {
							IShellItem *psiResult;
							hr = pfd->GetResult(&psiResult);
							if (SUCCEEDED(hr)) {
								PWSTR pszFilePath = NULL;
								hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
								if (SUCCEEDED(hr)) {
									std::string str = u16_cstr_to_u8(pszFilePath);
									retPath = str;
									CoTaskMemFree(pszFilePath);
									if (retPath.is_relative()) {
										retPath = retPath.root_path();
										if (!retPath.is_absolute()) {
											g_errorfStream << "(SeekDir) couldn't make path absolute: " << retPath << std::flush;
											retPath.clear();
										}
									}
								}
								psiResult->Release();
							}
						}
					}
				}
			}
			pfd->Release();
		} else {
			errorf("(SeekDir) Can't create IExample object. CoCreateInstance error");
		}
		CoUninitialize();
	} else {
		g_errorfStream << "(SeekDir) hr: " << hr << std::flush;
		g_errorfStream << E_INVALIDARG << ", " << E_OUTOFMEMORY << ", " << E_UNEXPECTED << ", " << S_OK << ", " << S_FALSE << ", " << RPC_E_CHANGED_MODE << std::flush;
		errorf("Can't initialize COM"); 
	}
	if (resultPtr != nullptr) {
		*resultPtr = hr;
	}
	return retPath;
}

std::filesystem::path makePathRelativeToProgDir(const std::filesystem::path &argPath, ErrorObject *retError) {
	//int pdpdc, cpdc;		// pdpdc -- prog dir parent directory count, common pdc
	//char *rbuf;
	//int i, j;
	
	if (argPath.is_relative()) {
		errorf("makePathRelativeToProgDir received relative path");
		setErrorPtr(retError, 1);
		return {};
	}
	
	if (g_fsPrgDir.empty()) {
		errorf("g_fsPrgDir was empty");
		setErrorPtr(retError, 1);
		return {};
	}
	
	std::fs::path usePath = argPath;
	
	auto usePathIt = usePath.begin();
	auto prgDirIt = g_fsPrgDir.begin();
	
	if (usePathIt == usePath.end() || prgDirIt == g_fsPrgDir.end()) {
		errorf("first element of non-empty path was end()");
		setErrorPtr(retError, 1);
		return {};
	}
	
	if (*usePathIt != *prgDirIt) {
		setErrorPtrOrPrint(retError, g_errorfStream, ErrorObject(1, "Entered path has different root from program path"));
		return {};
	} else {
		usePathIt++;
		prgDirIt++;
	}
	
	while (usePathIt != usePath.end() && prgDirIt != g_fsPrgDir.end()) {
		if (*usePathIt == *prgDirIt) {
			usePathIt++;
			prgDirIt++;
		} else {
			break;
		}
	}
	
	std::fs::path retPath = ".";
	
	while (prgDirIt != g_fsPrgDir.end()) {
		retPath /= "..";
		prgDirIt++;
	}
	
	while (usePathIt != usePath.end()) {
		retPath /= *usePathIt;
		usePathIt++;
	}
	
	return retPath;
}

static constexpr uint64_t maxUint = UINT64_MAX;

uint64_t stringToUint(std::string str, ErrorObject *retError) {
	uint64_t ret_uint = 0;
	try {
		ret_uint = std::stoull(str);
	} catch(...) {
		if (!setErrorPtr(retError, 1)) {
			errorf("stringToUint received exception");
		}
		return 0;
	}
	
	return ret_uint;	
}