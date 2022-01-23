#include "windows.h"

#include <filesystem>
#include <stdint.h>

#include "errorobj.hpp"

std::fs::path SeekDir(const HWND &hwnd, HRESULT *resultPtr = nullptr);

std::filesystem::path makePathRelativeToProgDir(const std::filesystem::path &argPath, ErrorObject *retError = nullptr);

uint64_t stringToUint(std::string str, ErrorObject *retError = nullptr);

/// cut-pasted from main.cpp

extern "C" {
	#include "stringchains.h"
}

int CleanEditText(HWND);

void dialogf(HWND, char*, ...);
char keepremovedandadded(oneslnk *origchn, char *buf, oneslnk **addaliaschn, oneslnk **remaliaschn, uint8_t presort);