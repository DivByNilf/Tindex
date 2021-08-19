#include <stdio.h>
#include <windows.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "portables.h"
#include "portables.hpp"

#include "errorf.hpp"

extern char *g_prgDir;

#define MAX_LEN MAX_PATH*4 + 1000
#define W_MAX_LEN MAX_PATH*2 + 1000

void g_errorfStdStr(std::string str) {
	static bool s_bFirstErr = true;
	
	if (!g_prgDir) {
		// maybe defer to some kind of buffer
		//g_errorfDialogStdStr("PrgDir doesn't exist");
		g_errorfDialogStdStr("(PrgDir doesn't exist) " + str);
		return;
	}
	
	std::string prgDirStr(g_prgDir);
	std::string fileStr = prgDirStr + "\\errorfile.log";
	
	std::ofstream ofs(fileStr, std::ios::app | std::ios::ate);
	
	if (!ofs.is_open()) {
		g_errorfDialogStdStr("Failed to open append file: \"" + fileStr + "\"");
		return;
	}
	
	if (s_bFirstErr) {
		if (ofs.tellp() != 0) {
			//ofs << "---" << "\n";
			ofs << "\n---\n";
		}
		
		s_bFirstErr = false;
	} else {
		ofs << "\n";
	}
	
	//ofs << str << "\n";
	// if we can reliably check file length, it becomes feasible to prepend the newline instead
	ofs << str;
}

class FlushToFileBuf : public std::basic_stringbuf<char> {
private:
	virtual int sync() override {
		this->std::basic_stringbuf<char>::sync();
		g_errorfStdStr(std::string(this->str()));
		this->str("");
	}
};

template <typename T>
class StreamFlusher {
private:
	std::basic_ostream<T> &stream;
	
public:
	
	StreamFlusher() = delete;
	
	StreamFlusher(std::basic_ostream<T> &stream_) : stream{stream_} {};
	
	virtual ~StreamFlusher() {
		this->stream << std::flush;
	}
};

FlushToFileBuf outerBuf;
std::basic_ostream<char> g_errorfStream(&outerBuf);
// flush the errorf stream when the program exits
static StreamFlusher<char> FlushToFileHelper(g_errorfStream);

void g_errorfDialogStdStr(std::string str) {
	std::wstring wstr = utf8_to_utf16(str);
	MessageBoxW(0, wstr.c_str(), 0, 0);
}

//for compatability

#include <stdio.h>
#include <stdarg.h>

#include "portables.h"

#define MAX_LEN MAX_PATH*4 + 1000
#define W_MAX_LEN MAX_PATH*2 + 1000

extern "C" {
void g_errorf(const char *str, ...) {
	char buf[MAX_LEN];
	va_list args;
	
	va_start(args, str);
	vsnprintf(buf, MAX_LEN, str, args);
	va_end(args);
	
	std::string cpp_str(buf);
	g_errorfStdStr(cpp_str);
}

void g_errorfDialog(const char *str, ...) {
	char buf[MAX_LEN];
	va_list args;
	
	va_start(args, str);
	vsnprintf(buf, MAX_LEN, str, args);
	va_end(args);
	
	std::string cpp_str(buf);
	g_errorfDialogStdStr(cpp_str);
}
}