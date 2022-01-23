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

/// cut-pasted from main.cpp

// TODO: a lot of these should probably be methods of window classes instead

#include "userinterface.hpp"

extern "C" {
	#include "stringchains.h"
	#include "dupstr.h"
	#include "arrayarithmetic.h"
	#include "bytearithmetic.h"
	#include "tfiles.h"
}



int CleanEditText(HWND hwnd) {
	int32_t i, offset;
	int64_t len;
	wchar_t *buf;
	DWORD selStart, selend;

	len = GetWindowTextLengthW(hwnd);
	if (!len) {
		return 0;
	} len++;
	SendMessage(hwnd, EM_GETSEL, (WPARAM) &selStart, (LPARAM) &selend);
	if (!(buf = (wchar_t *) malloc(len*2))) {
		errorf("malloc failed");
		return 1;
	}
	GetWindowTextW(hwnd, buf, len);

	for (i = 0, offset = 0; buf[i] != '\0'; i++) {
		if (buf[i] == '\n' || buf[i] == '\t' || buf[i] == '\r') {
			if (i+offset < selStart) {
				selStart--;
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
	SendMessage(hwnd, EM_SETSEL, selStart, selend);
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
	g_errorfStream << "origchn: " << link1->str << std::flush;
	link1 = link1->next;
}
link1 = parsedchn;
while (link1) {
	g_errorfStream << "parsedchn: " << link1->str << std::flush;
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
