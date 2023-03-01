#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}

#include "prgdir.hpp"

#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

std::fs::path GetPrgDir(void) {
	std::error_code ec;
	auto prgDir = std::filesystem::current_path(ec);
	
	if (ec) {
		return std::fs::path();
	} else if (!std::fs::is_directory(prgDir)) {
		errorf("prgDir was not directory");
		return std::fs::path();
	}
	else {
		return prgDir;
	}
}