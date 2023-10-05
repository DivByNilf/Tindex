#include <stdio.h>
#include <windows.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

#include "portables.h"
#include "portables.hpp"

#include "errorf.hpp"

#define MAX_LEN MAX_PATH*4 + 1000
#define W_MAX_LEN MAX_PATH*2 + 1000

void ErrorfStdStrF(const std::string &str, const std::string &fileName) {
	static bool s_bFirstErr = true;
	
	if (fileName == "") {
		// TODO: maybe defer to some kind of buffer
		ErrorfDialogStdStr("(ErrorfStdStr file doesn't exist doesn't exist) " + str);
		return;
	}
	
	std::ofstream ofs(fileName, std::ios::app | std::ios::ate);
	
	if (!ofs.is_open()) {
		ErrorfDialogStdStr("Failed to open append file: \"" + fileName + "\"");
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

void ErrorfStdStr(const std::string &str, const std::string &fileName) {
	ErrorfStdStrF(str, fileName);
}

int FlushToFileBuf::sync() {
	if (this->std::basic_stringbuf<char>::sync()) {
		ErrorfStdStrF("basic_stringbuf sync failed", fileName_);
		return -1;
	}
	ErrorfStdStrF(std::string(this->str()), fileName_);
	this->str("");
	return 0;
}

FlushToFileBuf::FlushToFileBuf(const std::string &fileName) :
	std::basic_stringbuf<char>(),
	fileName_(fileName)
{}

template <typename T>
StringBufFlusher<T>::StringBufFlusher(std::basic_stringbuf<T> &buf_) : buf{buf_} {};

template <typename T>
StringBufFlusher<T>::~StringBufFlusher() {
	// should not flush if nothing written
	if (buf.str().length() >= 1) {
		this->buf.pubsync();
	}
}

// static FlushToFileBuf g_outerBuf(g_fileName);
// std::basic_ostream<char> errorfStream(&g_outerBuf);
// flush the errorf stream when the program exits
// static StringBufFlusher<char> g_flushToFileHelper(g_outerBuf);

void ErrorfDialogStdStr(const std::string &str) {
	std::wstring wstr = u8_to_u16(str);
	MessageBoxW(0, wstr.c_str(), 0, 0);
}

/* bool SetErrorfStaticPrgDir(const std::string &prgDir) {
	fileName = prgDir + "/errorfile.log";
	outerBuf.fileName_ = fileName;
	return true;
} */

std::string CreateErrorFileName(std::string prgDir) {
	return prgDir + "/errorfile.log";
}

ErrorfData &&MakeErrorfStream(const std::string prgDir) {
	std::string fileName = prgDir + "/errorfile.log";
	FlushToFileBuf outerBuf(fileName);
	auto errorfStreamPtr = std::make_shared<std::ostream>(&outerBuf);
	StringBufFlusher<char> flushToFileHelper(outerBuf);
	return ErrorfData {
		errorfStreamPtr,
		std::move(outerBuf),
		std::move(flushToFileHelper)
	};
}

void errorf(std::ostream &errorfStream, const std::string &str) {
	errorfStream << str << std::flush;
}

//for compatability

#include <stdio.h>
#include <stdarg.h>

#include "portables.h"

#define MAX_LEN MAX_PATH*4 + 1000
#define W_MAX_LEN MAX_PATH*2 + 1000

// TODO: remove all this
extern "C" {
	void c_errorf(const char *file_name, const char *str, ...) {
		char buf[MAX_LEN];
		va_list args;
		
		va_start(args, str);
		vsnprintf(buf, MAX_LEN, str, args);
		va_end(args);
		
		std::string cpp_str(buf);
		std::string fileName(file_name);
		ErrorfStdStrF(cpp_str, fileName);
	}

	void ErrorfDialog(const char *str, ...) {
		char buf[MAX_LEN];
		va_list args;
		
		va_start(args, str);
		vsnprintf(buf, MAX_LEN, str, args);
		va_end(args);
		
		std::string cpp_str(buf);
		ErrorfDialogStdStr(cpp_str);
	}
}