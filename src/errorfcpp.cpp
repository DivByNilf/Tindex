#include <stdio.h>
#include <windows.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "portables.h"
#include "portables.hpp"

#include "errorf.hpp"

static std::string g_fileName;

#define MAX_LEN MAX_PATH*4 + 1000
#define W_MAX_LEN MAX_PATH*2 + 1000

void ErrorfStdStrF(const std::string str, const std::string fileName) {
	static bool s_bFirstErr = true;
	
	if (fileName == "") {
		// TODO: maybe defer to some kind of buffer
		g_errorfDialogStdStr("(ErrorfStdStr file doesn't exist doesn't exist) " + str);
		return;
	}
	
	// std::string fileStr = prgDir + "\\errorfile.log";
	
	std::ofstream ofs(fileName, std::ios::app | std::ios::ate);
	
	if (!ofs.is_open()) {
		g_errorfDialogStdStr("Failed to open append file: \"" + fileName + "\"");
		return;
	}
	
	if (s_bFirstErr) {
		if (ofs.tellp() != 0) {
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

void g_errorfStdStr(const std::string str) {
	ErrorfStdStrF(str, g_fileName);
}

class FlushToFileBuf : public std::basic_stringbuf<char> {
private:
	virtual int sync() override {
		if (this->std::basic_stringbuf<char>::sync()) {
			ErrorfStdStrF("basic_stringbuf sync failed", fileName_);
			return -1;
		}
		ErrorfStdStrF(std::string(this->str()), fileName_);
		this->str("");
		return 0;
	}
public:
	FlushToFileBuf(const std::string &fileName);
	FlushToFileBuf() = delete;

	std::string fileName_;
};

FlushToFileBuf::FlushToFileBuf(const std::string &fileName) :
	std::basic_stringbuf<char>(),
	fileName_(fileName)
{}

template <typename T>
class StringBufFlusher {
private:
	std::basic_stringbuf<T> &buf;
	
public:
	
	StringBufFlusher() = delete;
	
	StringBufFlusher(std::basic_stringbuf<T> &buf_) : buf{buf_} {};
	
	virtual ~StringBufFlusher() {
		// should not flush if nothing written
		if (buf.str().length() >= 1) {
			this->buf.pubsync();
		}
	}
};

static FlushToFileBuf g_outerBuf(g_fileName);
std::basic_ostream<char> g_errorfStream(&g_outerBuf);
// flush the errorf stream when the program exits
static StringBufFlusher<char> g_flushToFileHelper(g_outerBuf);

void g_errorfDialogStdStr(std::string str) {
	std::wstring wstr = u8_to_u16(str);
	MessageBoxW(0, wstr.c_str(), 0, 0);
}

bool SetErrorfStaticPrgDir(std::string prgDir) {
	g_fileName = prgDir + "/errorfile.log";
	g_outerBuf.fileName_ = g_fileName;
	return true;
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
		ErrorfStdStrF(cpp_str, g_fileName);
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