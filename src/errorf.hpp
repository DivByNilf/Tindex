#include <string>
#include <iostream>
#include <sstream>

class FlushToFileBuf : public std::basic_stringbuf<char> {
private:
	virtual int sync(void);
public:
	FlushToFileBuf(const std::string &fileName);
	FlushToFileBuf() = delete;

	std::string fileName_;
};

template <typename T>
class StringBufFlusher {
public:
	std::basic_stringbuf<T> &buf;
	
	StringBufFlusher() = delete;
	
	StringBufFlusher(std::basic_stringbuf<T> &buf_);
	
	virtual ~StringBufFlusher();
};

struct ErrorfData {
    shared_ptr<std::ostream> errorfStreamPtr;
    FlushToFileBuf outerBuf;
    StringBufFlusher<char> flushToFileHelper;
};

void ErrorfStdStr(const std::string str, const std::string fileName);

void ErrorfDialogStdStr(const std::string &str);

// extern std::basic_ostream<char> g_errorfStream;

bool SetErrorfStaticPrgDir(const std::string prgDir);

std::string CreateErrorFileName(std::string prgDir);

ErrorfData &&MakeErrorfStream(const std::string prgDir);

void errorf(std::ostream &errorfStream, const std::string &str);