#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}

#include "prgdir.hpp"
#include "prgdir.h"

#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

bool PrgDirInit(void) {
	std::error_code ec;
	g_fsPrgDir = std::filesystem::current_path(ec);
	
	if (ec) {
		return false;
	} else if (!std::fs::is_directory(g_fsPrgDir)) {
		errorf("Preinit: g_fsPrgDir was not directory");
		return false;
	}
	else {
		return true;
	}
}