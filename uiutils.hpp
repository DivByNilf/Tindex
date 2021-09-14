#include "windows.h"

#include <filesystem>

std::fs::path SeekDir(const HWND &hwnd, HRESULT *result_ptr = nullptr);

std::filesystem::path makePathRelativeToProgDir(const std::filesystem::path &argPath, std::string *retError = nullptr);