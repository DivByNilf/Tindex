#include "windows.h"
#include <shobjidl.h>

#include "errorf.hpp"
extern "C" {
#include "errorf.h"
}

#include "uiutils.hpp"

#define errorf(str) g_errorfStdStr(str)
//! TEMP
#define errorf_old(...) g_errorf(__VA_ARGS__)

HRESULT SeekDir(HWND hwnd, char *retstr) {
	// CoCreate the File Open Dialog object.
	//IFileDialog comes from <shobjidl.h>
	IFileDialog *pfd = NULL; 	// the pointer to the object about to be created
	HRESULT hr;
	
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(hr) || 1) {
		if (!SUCCEEDED(hr)) {
			errorf_old("hr: %d", hr);
			errorf_old("%d, %d %d, %d, %d, %d", E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED, S_OK, S_FALSE, RPC_E_CHANGED_MODE);
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
		errorf_old("hr: %d", hr);
		errorf_old("%d, %d %d, %d, %d, %d", E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED, S_OK, S_FALSE, RPC_E_CHANGED_MODE);
		errorf("Can't initialize COM"); 
	}
	return hr;
}