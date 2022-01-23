#pragma once

extern "C" {
#include "stringchains.h"
}

#include <stdint.h>

#include <map>
#include <forward_list>
#include <list>
#include <string>
#include <memory>
#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}
#include <fstream>
#include <type_traits> //!
#include <utility> //!

// #include "ioextras.hpp"

// #include "prgdir.hpp"

typedef struct SearchExpr {
	unsigned short exprType;
	struct {
		struct SearchExpr *expr1;
		union {
			struct SearchExpr *expr2;
			uint64_t refNum;
		};
	};
} SREXP;

typedef struct SearchExpList {
	SREXP *expr;
	struct SearchExpList *next;
} SREXPLIST;

typedef struct SuperExpStack {
	SREXP *expr;
	uint64_t subExpLayers;
	struct SuperExpStack *next;
} SUPEXPSTACK;

typedef struct TagNumStruct {
	uint64_t tagNum;
	unsigned char state;
} TAGNUMNODE;

typedef struct SearchStruct {
	TAGNUMNODE *tagNumArray;
	unsigned long nTagNums;
	struct SearchExpr *rootExpr;
	uint64_t dNum;
} SEARCHSTRUCT;

//struct SubDirEntry {
//	char *subDirStr;
//};

/// classes

class FileCloser {
public:
	bool released_ = false;
	
	FileCloser() = delete;
	FileCloser(FILE *fp);
	
	~FileCloser();
	void release(void);

protected:
	FILE *fp_;
};

class FilePathDeleter {
public:
	const std::filesystem::path fpath_;
	bool released_ = false;
	
	FilePathDeleter() = delete;
	FilePathDeleter(std::filesystem::path &fpath);
	
	~FilePathDeleter();
	
	void release(void);
};

class FileRenameOp {
public:
	const std::filesystem::path from_;
	const std::filesystem::path to_;
	const std::filesystem::path bak_;
	
	FileRenameOp() = delete;
	FileRenameOp(std::filesystem::path from, std::filesystem::path to, std::filesystem::path bak);
	
	bool hasFirstExecuted(void) const;
	
	bool hasSecondExecuted(void) const;
	
	bool hasReversed(void) const;
	
	bool firstExecute(void);
	
	bool secondExecute(void);
	
	bool reverse(void);
	
	~FileRenameOp(void);
	
protected:
	bool firstExecuted_ = false;
	bool secondExecuted_ = false;
	bool reversed_ = false;
	bool hasOriginal_ = true;
	
};

bool reverseRenameOpList(std::list<FileRenameOp> opList);

bool executeRenameOpList(std::list<FileRenameOp> opList);

bool reverseRenameOpList(std::list<FileRenameOp> opList);

/////

class IndexSession;
class IndexSessionHandler;
class SubIndexSessionHandler;

class TopIndex;
class SubIndex;

bool existsMainIndex(const uint64_t &miNum);

// should be tested that it works as a key in a map (find and remove)

class IndexID {
public:
	const std::string str_;

	IndexID(std::string str);

	std::strong_ordering operator<=>(const IndexID& rhs) const;
};

class HandlerAccessor {
	friend class IndexSession;
private:
	static void removeHandlerRefs(IndexSessionHandler &handler, const IndexID &indexID, const IndexSession *sessionPtr);
};

//! TODO: make reference to parent (handler?) atomic

class IndexSession {
public:
	// indexID is assumed to be stored statically
	const IndexID &indexID_;

	IndexSession() = delete;
	IndexSession(const IndexSession &) = delete;
	IndexSession &operator=(const IndexSession &other) = delete;
	
	IndexSession(IndexSessionHandler &handler, const IndexID &indexID);
	
	IndexSessionHandler &getHandler(void);
	
	// handler may be deleted by derived class
	virtual ~IndexSession(void) = default;
	
	void removeHandlerRefs(void);
	
protected:
	IndexSessionHandler &handler_;
	
};

//{ Index Subclasses

class TopIndex : public IndexSession {
public:

	TopIndex(IndexSessionHandler &handler, const IndexID &indexID);
	
	virtual const std::filesystem::path getDirPath(void) const;
	
	virtual ~TopIndex(void);
	
};

class SubIndex : public IndexSession {
public:
	SubIndex() = delete;

	SubIndex(std::shared_ptr<SubIndexSessionHandler> &handler, const IndexID &indexID);
	
	static const std::filesystem::path getDirPathFor(uint64_t miNum);
	
	virtual const std::filesystem::path getDirPath(void) const;
	
	uint64_t getMINum(void) const;
	
	virtual ~SubIndex();
	
protected:
	std::shared_ptr<SubIndexSessionHandler> handler_;
	
};

template <class T>
struct IOSpec;

// give no failbit if only read EOF -- give failbit if read characters and then EOF
// TODO: should the functions be static?
template<>
struct IOSpec<uint64_t> {
	static void write(std::ostream &ios, const uint64_t &entry);
	
	static uint64_t read(std::istream &ios);
	
	static void skip(std::istream &ios);
};

template<>
struct IOSpec<std::string> {
	static void write(std::ostream &ios, const std::string &entry);
	
	static std::string read(std::istream &ios);
	
	static void skip(std::istream &ios);
};

template<>
struct IOSpec<std::fs::path> {
	static void write(std::ostream &ios, const std::fs::path &entry);
	
	static std::fs::path read(std::istream &ios);
	
	static void skip(std::istream &ios);
};

template <class T>
struct FixedIOSpec;

// give no failbit if only read EOF -- give failbit if read characters and then EOF
template<>
struct FixedIOSpec<uint64_t> {
	static void write(std::ostream &ios, const uint64_t &entry);
	
	static uint64_t read(std::istream &ios);
	
	static void skip(std::istream &ios);
};

template <class KeyT, class EntryT>
class StandardIndex {
public:
	// typedef std::pair<KeyT, EntryT> EntryPair;
	
	virtual const std::string getFileStrBase(void) const = 0;
	
	virtual const std::filesystem::path getDirPath(void) const = 0;
	
	virtual bool isValidInputEntry(const EntryT&) const = 0;
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<std::pair<KeyT, EntryT>> regEntryPairList) = 0;
	
	virtual ~StandardIndex(void);
	
	EntryT makeNullEntry(void);
	
	EntryT makeNullKey(void);
	
	std::shared_ptr<std::forward_list<EntryT>> readIntervalEntries(const KeyT &start, const uint64_t interval);
	
	EntryT readEntry(const KeyT &entryKey);
	
	bool hasUndeletedEntry(const KeyT &entryKey);
	
	bool removeEntry(KeyT);

	KeyT replaceEntry(KeyT, EntryT);
	
protected:
	
	KeyT streamGetKey(std::istream &ios);
	
	EntryT streamGetEntry(std::istream &ios);
	
	void streamSkipEntry(std::istream &ios);
	
	KeyT streamSkipEntryGetKey(std::istream &ios);
	
	std::shared_ptr<std::pair<KeyT, EntryT>> streamWriteEntryGetPair(std::ostream &ios, const KeyT &entryKey, const EntryT &inputEntry);
	
	bool clearRec(void);
	
	uint64_t createRec(int32_t &error);
	
	std::pair<bool, std::list<FileRenameOp>> prepareRec(const std::fs::path &indexFilePath, std::shared_ptr<KeyT> &lastEntryKeyPtr);
	
	std::pair<bool, std::list<FileRenameOp>> addToLastEntryNum(const std::fs::path &indexFilePath, const int64_t num, const uint64_t spot);
	
	uint64_t getKeySeekPos(const KeyT &key, int32_t &error);
	
};

template <class KeyT, class EntryT>
class StandardManualKeyIndex : public StandardIndex<KeyT, EntryT> {
public:
	typedef std::pair<KeyT, EntryT> EntryPair;
	
	KeyT addEntry(KeyT, EntryT);
	
	virtual ~StandardManualKeyIndex(void) = default;
};

// not actually specialized for key types other than scalar types
template <class KeyT, class EntryT>
class StandardAutoKeyIndex : public StandardIndex<KeyT, EntryT> {
public:
	// typedef std::pair<KeyT, EntryT> EntryPair;
	
	virtual ~StandardAutoKeyIndex(void) = default;
	
	KeyT getLastKey(void);
	
	// 'virtual' meaning some entries may actually be deleted
	uint64_t getNofVirtualEntries(int32_t &error);

	KeyT addEntry(const EntryT &argEntry);
	
	KeyT addEntry(EntryT *argEntry);
	
	KeyT addEntry(std::shared_ptr<EntryT> argEntry);

	std::forward_list<KeyT> addEntries(const std::forward_list<EntryT> inputList);

};

template <class T, class ... Ts>
static std::shared_ptr<T> g_MakeSharedIndexSession(Ts&& ... args);

// every session knows an instance of IndexSessionHandler
// IndexSessionHandler contains a list of every session or sub-sessionhandler containing the session
// when there are no more references to a session, the session is removed from the list

class IndexSessionHandler {
public:
	IndexSessionHandler() = default;
	IndexSessionHandler(const IndexSessionHandler &) = delete;
	IndexSessionHandler &operator=(const IndexSessionHandler &other) = delete;
	
	virtual ~IndexSessionHandler() = default;
	
protected:
	
	// get around mutual dependency and privacy
	friend class HandlerAccessor;
	
	virtual bool removeRefs(const IndexID &indexID, const IndexSession *sessionPtr) = 0;
	
};

class TopIndexSessionHandler : public IndexSessionHandler {
public:

	TopIndexSessionHandler();

	template <class U, class ... Ts>
	std::shared_ptr<U> openSession(Ts&& ... args);
	
protected:
	std::map<IndexID, std::weak_ptr<TopIndex>> openSessions_;
	std::map<uint64_t, std::weak_ptr<SubIndexSessionHandler>> subHandlers_;
	
	virtual bool removeRefs(const IndexID &indexID, const IndexSession *sessionPtr);
	
	friend class SubIndexSessionHandler;
	
	bool removeSubHandlerRefs(const uint64_t &miNum, const SubIndexSessionHandler *handlerPtr);
	
private:
	
	template <class U, class ... Ts>
	std::shared_ptr<U> openTopIndexSession(Ts&& ... args);
	
	template <class U, class ... Ts>
	std::shared_ptr<U> openSubIndexSession(uint64_t miNum, Ts&& ... args);
	
	
};

class SubIndexSessionHandler : public IndexSessionHandler {
public:
	SubIndexSessionHandler() = delete;
	
	SubIndexSessionHandler(TopIndexSessionHandler &parent, const uint64_t &miNum);

	template <class U, class ... Ts>
	std::shared_ptr<U> openSession(std::shared_ptr<SubIndexSessionHandler> &shared_this, Ts&& ... args);
	
	uint64_t getMINum(void);
	
	virtual ~SubIndexSessionHandler(void);
	
protected:
	std::map<IndexID, std::weak_ptr<SubIndex>> openSessions_;
	const uint64_t miNum_;
	TopIndexSessionHandler &parent_;
	
	virtual bool removeRefs(const IndexID &indexID, const IndexSession *sessionPtr);

};

/// independent function
// TODO: figure the placement of this or the need for its existence
bool existsMainIndex(const uint64_t &miNum);

class MainIndexIndex : public TopIndex, public StandardAutoKeyIndex<uint64_t, std::string> {
public:
	static const IndexID indexID;
	
	static constexpr uint64_t k_MaxEntryLen = 500*4;

	//
	
	MainIndexIndex(IndexSessionHandler &handler);
	
	virtual const std::string getFileStrBase(void) const override;
	
	virtual const std::filesystem::path getDirPath(void) const override;
	
	virtual bool isValidInputEntry(const std::string &entryString) const override;
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<std::pair<uint64_t, std::string>> regEntryPairList) override;
	
	virtual ~MainIndexIndex(void) = default;
	
};

class MainIndexReverse : public TopIndex, public StandardManualKeyIndex<std::string, std::forward_list<uint64_t>> {
public:
	static const IndexID indexID;
	
	static constexpr uint64_t k_MaxKeyLen = MainIndexIndex::k_MaxEntryLen;

	//
	
	MainIndexReverse(IndexSessionHandler &handler);
	
	virtual const std::string getFileStrBase(void) const override;
	
	virtual const std::filesystem::path getDirPath(void) const override;
	virtual bool isValidInputEntry(const std::forward_list<uint64_t> &list) const override;
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<std::pair<std::string, std::forward_list<uint64_t>>> regEntryPairList) override;
	
	virtual ~MainIndexReverse(void) = default;
	
};

class MainIndexExtras {
public:
	static const IndexID indexID;
};

class DirIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, std::fs::path> {
public:
	static const IndexID indexID;
	
	static constexpr uint64_t k_MaxEntryLen = 260*4;

	//
	
	DirIndex(std::shared_ptr<SubIndexSessionHandler> &handler);
	
	virtual const std::string getFileStrBase(void) const override;
	
	virtual const std::filesystem::path getDirPath(void) const override;
	
	virtual bool isValidInputEntry(const std::fs::path &entryPath) const override;
	
	// TODO: do something to avoid repeating the index template types
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<std::pair<uint64_t, std::fs::path>> regEntryPairList) override;
	
	virtual ~DirIndex(void) = default;
};

class SubDirEntry {
public:
	uint64_t parentInum_;
	std::fs::path subPath_;
	
	int64_t startDepth_ = -1;
	int64_t endDepth_ = -1;
	
	std::list<std::string> excludeDirs_;
	std::list<std::string> excludeSubPaths_;

	SubDirEntry() = delete;
	
	SubDirEntry(uint64_t &parentInum, std::fs::path &subPath);
	
	bool isValid(void) const;
	
};

template <>
struct IOSpec<SubDirEntry> {
public:
	static void write(std::ostream &ios, const SubDirEntry &entry);
	
	static SubDirEntry read(std::istream &ios);
	
	static void skip(std::istream &ios);
};

class SubDirIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, SubDirEntry> {
public:
	static const IndexID indexID;

	//
	
	SubDirIndex(std::shared_ptr<SubIndexSessionHandler> &handler);
	
	virtual const std::string getFileStrBase(void) const override;
	
	virtual const std::filesystem::path getDirPath(void) const override;
	
	virtual bool isValidInputEntry(const SubDirEntry &entry) const override;
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<std::pair<uint64_t, SubDirEntry>> regEntryPairList) override;
		
};

class FileIndexEntry {
	std::fs::path filePath_;
	std::list<uint64_t> tagList_;

	FileIndexEntry() = delete;
	
	FileIndexEntry(std::fs::path &filePath, std::list<uint64_t> tagList);
	
};

class FileIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, FileIndexEntry> {
public:
	static const IndexID indexID;

};

class TagIndexEntry {
	
};

class TagIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, TagIndexEntry> {
public:
	static const IndexID indexID;


};

class TagAliasEntry {
	
};

class TagAliasIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, FileIndexEntry> {
public:
	static const IndexID indexID;

};

/// function declarations for use in indextools.cpp

unsigned char raddtolastmiNum(long long num);

uint64_t rmiinitpos(char *idir);

char rmiinit();

char rmireg(char *entrystr, uint64_t inum);

unsigned char addtolastmiNum(long long num, uint64_t spot);

uint64_t miinitpos(uint64_t iinum);

unsigned char raddtolastdnum(uint64_t miNum, long long num);

uint64_t rdinitpos(uint64_t miNum, char *idir);

char rdinit(uint64_t miNum);

// // char setdlastchecked(uint64_t miNum, uint64_t dnum, uint64_t ulltime);

// // uint64_t getdlastchecked(uint64_t miNum, uint64_t dnum);

unsigned char addtolastdnum(uint64_t miNum, long long num, uint64_t spot);

uint64_t dinitpos(uint64_t miNum, uint64_t idnum);

unsigned char addtolastsdnum(uint64_t miNum, long long num, uint64_t spot);

uint64_t sdinitpos(uint64_t miNum, uint64_t idnum);

int cfireg(uint64_t dnum, oneslnk *fnamechn); //!

int tnumfromalias2(uint64_t dnum, twoslnk **sourcelist);

//// template methods

// has to be undefined since this is a header file
#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

/// StandardIndex<KeyT, EntryT>

template <class KeyT, class EntryT>
StandardIndex<KeyT, EntryT>::~StandardIndex(void) {
	// do nothing
}

template <class KeyT, class EntryT>
EntryT StandardIndex<KeyT, EntryT>::makeNullEntry(void) {
	return EntryT();
}

template <class KeyT, class EntryT>
EntryT StandardIndex<KeyT, EntryT>::makeNullKey(void) {
	return KeyT();
}

template <class KeyT, class EntryT>
std::shared_ptr<std::forward_list<EntryT>> StandardIndex<KeyT, EntryT>::readIntervalEntries(const KeyT &start, const uint64_t intrvl) {
	if (!intrvl) {
		errorf("readIntervalEntries without intrvl");
		return {};
	}
	
	if (start == 0) {
		errorf("readInterval start was 0");
		return {};
	}
	
	std::filesystem::path indexFilePath = this->getDirPath() / (this->getFileStrBase() + ".bin");
	
	if (!std::filesystem::exists(indexFilePath)) {
		return {};
	}
	
	std::ifstream indexStream = std::ifstream(indexFilePath, std::ios::binary | std::ios::in);

	if (!indexStream.is_open()) {
		errorf("(readIntervalEntries) failed to open indexStream for read");
		return {};
	}
	
	int32_t seekPosError = 0;
	
	uint64_t filePos = getKeySeekPos(start, seekPosError);
	
	if (seekPosError) {
		errorf("getKeySeekPos failed");
		return {};
	}
	
	indexStream.seekg(filePos, std::ios_base::beg);
	
	if (indexStream.fail()) {
		g_errorfStream << "indexStream seek failed -- seekpos: " << filePos << std::flush;
		return {};
	}
	
	std::shared_ptr<std::forward_list<EntryT>> retListPtr(new std::forward_list<EntryT>());
	
	if (retListPtr == nullptr) {
		errorf("shared_ptr construct fail");
		return {};
	}
	
	std::forward_list<EntryT> &retList = *retListPtr;
	
	{
		auto insertPos = retList.before_begin();
		
		KeyT nextGetKey = start;
		KeyT stopGetKey = start + intrvl;
		
		KeyT lastEntryKey = 0;
		KeyT tempEntryKey = this->streamGetKey(indexStream);
		
		while (!indexStream.eof()) {
			if (tempEntryKey <= lastEntryKey) {
				errorf("tempEntryKey <= lastEntryKey");
				return {};
			} else {
				lastEntryKey = tempEntryKey;
			}
				
			while (nextGetKey < tempEntryKey && nextGetKey < stopGetKey) {
				insertPos = retList.insert_after(insertPos, std::move(this->makeNullEntry()));
				nextGetKey++;
			}
			
			if (nextGetKey == stopGetKey) {
				break;
			}
			
			if (tempEntryKey == nextGetKey) {
				EntryT tempEntry = this->streamGetEntry(indexStream);
				if (indexStream.fail()) {
					errorf("indexStream getEntry fail");
					return {};
				} else {
					insertPos = retList.insert_after(insertPos, std::move(tempEntry));
					nextGetKey++;
				}
			} else {
				this->streamSkipEntry(indexStream);
				if (indexStream.fail()) {
					errorf("indexStream skipEntry fail");
					return {};
				}
			}
			tempEntryKey = this->streamGetKey(indexStream);
		}
		
		if (indexStream.fail()) {
			errorf("indexStream getKey fail (1)");
			return {};
		}
		
	}

	int64_t retListLen = std::distance(retList.begin(), retList.end());
	
	if (retListLen < 0) {
		errorf("retList negative length");
		return {};
	}
	
	return retListPtr;
	
}

template <class KeyT, class EntryT>
EntryT StandardIndex<KeyT, EntryT>::readEntry(const KeyT &entryKey) {
	auto retListPtr = readIntervalEntries(entryKey, 1);
	if (retListPtr == nullptr) {
		return makeNullEntry();
	}
	auto &retList = *retListPtr;
	
	if (retList.begin() != retList.end()) {
		return *retList.begin();
	} else {
		return makeNullEntry();
	}
}

template <class KeyT, class EntryT>
bool StandardIndex<KeyT, EntryT>::hasUndeletedEntry(const KeyT &entryKey) {
	if (this->readEntry(entryKey) != this->makeNullEntry()) {
		return true;
	} else {
		return false;
	}
}

template <class KeyT, class EntryT>
KeyT StandardIndex<KeyT, EntryT>::streamGetKey(std::istream &ios) {
	KeyT key = IOSpec<KeyT>::read(ios);
	
	return key;
}

template <class KeyT, class EntryT>
EntryT StandardIndex<KeyT, EntryT>::streamGetEntry(std::istream &ios) {
	EntryT entry = IOSpec<EntryT>::read(ios);
	
	return entry;
}

template <class KeyT, class EntryT>
void StandardIndex<KeyT, EntryT>::streamSkipEntry(std::istream &ios) {
	IOSpec<EntryT>::skip(ios);
}

template <class KeyT, class EntryT>
KeyT StandardIndex<KeyT, EntryT>::streamSkipEntryGetKey(std::istream &ios) {
	KeyT key = IOSpec<KeyT>::read(ios);

	if (ios.fail() || ios.eof()) {
		return key;
	}
	
	IOSpec<EntryT>::skip(ios);
	
	//! TODO: maybe verify content
	
	return key;
}

///



/// StandardIndex<KeyT, EntryT>
	
template <class KeyT, class EntryT>
std::shared_ptr<std::pair<KeyT, EntryT>> StandardIndex<KeyT, EntryT>::streamWriteEntryGetPair(std::ostream &ios, const KeyT &entryKey, const EntryT &inputEntry) {
errorf("writeEntryGetPair start");
	IOSpec<KeyT>::write(ios, entryKey);
	
	if (ios.fail() || ios.bad()) {
errorf("write failed at key");
		ios.setstate(std::ios_base::badbit);
		return {};
	}
errorf("writeEntryGetPair writing entry");
	
	IOSpec<EntryT>::write(ios, inputEntry);
	
	if (ios.fail() || ios.bad()) {
errorf("write failed at EntryT");
		ios.setstate(std::ios_base::badbit);
		return {};
	}
errorf("writeEntryGetPair after writing entry");
	
	return std::make_shared<std::pair<KeyT, EntryT>>(entryKey, inputEntry);
}

template <class KeyT, class EntryT>
bool StandardIndex<KeyT, EntryT>::clearRec(void) {
	std::filesystem::path indexRecPath = this->getDirPath() / (this->getFileStrBase() + ".rec.bin");
	std::filesystem::path indexRecBakPath = this->getDirPath() / (this->getFileStrBase() + ".rec.bin.bak");
	
	{	
		std::error_code error;
		bool success = std::fs::remove(indexRecPath, error);
		if (error) {
			g_errorfStream << "(clearRec) failed to remove: " << indexRecPath << std::flush;
			return false;
		}
	}
	
	{
		std::error_code error;
		bool success = std::fs::remove(indexRecBakPath, error);
		if (error) {
			g_errorfStream << "(clearRec) failed to remove: " << indexRecBakPath << std::flush;
			return false;
		}
	}
	
	return true;
}

template <class KeyT, class EntryT>
uint64_t StandardIndex<KeyT, EntryT>::createRec(int32_t &error) {
	error = 0;
	
	std::filesystem::path indexFilePath = this->getDirPath() / (this->getFileStrBase() + ".bin");
	
	if (!std::filesystem::exists(indexFilePath)) {
		error = 0;
		return 0;
	}
	
	std::shared_ptr<KeyT> lastEntryKeyPtr;
	
	auto resPair = prepareRec(indexFilePath, lastEntryKeyPtr);
	
	if (resPair.first == false) {
		errorf("(createRec) prepareRec failed");
		error = 1;
		return 0;
	} else if (lastEntryKeyPtr == nullptr) {
		errorf("returned lastEntryKeyPtr is null");
		error = 1;
		return 0;
	}
	
	auto &renameOpList = resPair.second;
	
	bool success = executeRenameOpList(renameOpList);
	if (!success) {
		errorf("executeRenameOpList failed");
		//! TODO: remove tmp files
		error = 1;
		return 0;
	}
	
	error = 0;
	return *lastEntryKeyPtr;
}

template <class KeyT, class EntryT>
std::pair<bool, std::list<FileRenameOp>> StandardIndex<KeyT, EntryT>::prepareRec(const std::fs::path &indexFilePath, std::shared_ptr<KeyT> &lastEntryKeyPtr) {
	
	std::filesystem::path indexRecPath = this->getDirPath() / (this->getFileStrBase() + ".rec.bin");
	
	std::filesystem::path tempIndexRecPath = this->getDirPath() / (this->getFileStrBase() + ".rec.tmp");
	
	if (tempIndexRecPath.extension() != ".tmp") {
		errorf("failed to create tempIndexRecPath");
		return {};
	}
	
	// remove indexRec
	{
		std::error_code ec;
		std::filesystem::remove(indexRecPath, ec); // returns (bool) false if path didn't exist or if there's an error
		if (ec) {
			errorf("failed to remove indexRecPath");
			g_errorfStream << "tried to remove: " << indexRecPath << std::flush;
			return {};
		}
	}
	
	// remove tempIndexRec
	{	
		std::error_code ec;
		std::filesystem::remove(tempIndexRecPath, ec); // returns (bool) false if path didn't exist or if there's an error
		if (ec) {
			errorf("failed to remove tempIndexRecPath");
			g_errorfStream << "tried to remove: " << tempIndexRecPath << std::flush;
			return {};
		}
	}
		
	std::ifstream indexStream = std::ifstream(indexFilePath, std::ios::binary | std::ios::in);
	
	// should not be error if indexFile doesn't exist
	if (!indexStream.is_open()) {
		errorf("(createRec) failed to open indexStream for read");
		g_errorfStream << "path was: " << indexFilePath << std::flush;
		return {};
	}
	
	std::shared_ptr<FilePathDeleter> tempRecDeleterPtr; // assuming class deleters are called in reverse order of appearance
		
	std::ofstream tempRecStream = std::ofstream(tempIndexRecPath, std::ios::binary | std::ios::out | std::ios::trunc | std::ios::in);
	
	if (!tempRecStream.is_open()) {
		errorf("failed to open tempRecStream for write");
		return {};
	}
	
	tempRecDeleterPtr = std::make_shared<FilePathDeleter>(tempIndexRecPath);
	
	// get the last entry key
	//! TODO: 
	KeyT lastEntryKey = 0;
	{
		KeyT tempEntryKey = this->streamSkipEntryGetKey(indexStream);
		while (!indexStream.eof()) {
			if (tempEntryKey <= lastEntryKey) {
				errorf("tempEntryKey <= lastEntryKey");
				return {};
			} else {
				lastEntryKey = tempEntryKey;
			}
			tempEntryKey = this->streamSkipEntryGetKey(indexStream);
		}
		if (indexStream.fail()) {
			errorf("indexStream entry read fail (1)");
			return {};
		}
		indexStream.clear();
	}
	
	indexStream.close();
	
	FixedIOSpec<uint64_t>::write(tempRecStream, lastEntryKey);
	
	if (tempRecStream.fail()) {
		errorf("failed to write to tempRecStream");
		return {};
	}
	
	tempRecStream.close();
	tempRecDeleterPtr->release();
	
	std::list<FileRenameOp> renameOpList;
	std::filesystem::path bakIndexRecPath = this->getDirPath() / (this->getFileStrBase() + ".rec.bin.bak");
	renameOpList.emplace_back(tempIndexRecPath, indexRecPath, bakIndexRecPath);
	
	lastEntryKeyPtr = std::shared_ptr<KeyT>(new KeyT(lastEntryKey));
	
	return {true, renameOpList};
}

template <class KeyT, class EntryT>
std::pair<bool, std::list<FileRenameOp>> StandardIndex<KeyT, EntryT>::addToLastEntryNum(const std::fs::path &indexFilePath, const int64_t num, const uint64_t spot) {
	//! TODO:
	
	//! temp
	std::shared_ptr<KeyT> temp_ptr;
	return prepareRec(indexFilePath, temp_ptr);
	
	//return std::pair(true, std::list<FileRenameOp>());
	
	//return {};
}

template <class KeyT, class EntryT>
uint64_t StandardIndex<KeyT, EntryT>::getKeySeekPos(const KeyT &key, int32_t &error) {
	error = 0;
	
	if (key == 0) {
		//! TODO:
		0;
	}
	
	error = 0;
	return 0;
}

/// StandardAutoKeyIndex<KeyT, EntryT>

template <class KeyT, class EntryT>
KeyT StandardAutoKeyIndex<KeyT, EntryT>::getLastKey(void) {
	//! TODO:
	
	
	return 0;
}

// 'virtual' meaning some entries may actually be deleted
template <class KeyT, class EntryT>
uint64_t StandardAutoKeyIndex<KeyT, EntryT>::getNofVirtualEntries(int32_t &error) {
	//! TODO:
	error = 0;
	
	std::filesystem::path indexRecPath = this->getDirPath() / (this->getFileStrBase() + ".rec.bin");
	
	if (!std::filesystem::exists(indexRecPath)) {
		int32_t retError = 0;
		uint64_t uint = this->createRec(retError);
		
		if (retError) {
			errorf("createRec failed");
			return 0;
		} else {
			return uint;
		}
	} else {
		std::ifstream inputRecStream(indexRecPath, std::ios::binary | std::ios::in);
	
		if (!inputRecStream.is_open()) {
			
			errorf("(getNumEntries) failed to open inputRecStream for read");
			error = 1;
			return 0;
		}
		
		uint64_t uint = FixedIOSpec<uint64_t>::read(inputRecStream);
		if (inputRecStream.fail()) {
			errorf("inputRecStream fixed uint64_t read fail");
			error = 3;
			
			return 0;
		} else if (inputRecStream.eof()) {
errorf("inputRecStream empty");
			// could set this to -1
			error = 0;
			
			return 0;
		} else {
			return uint;
		}
	}
}

template <class KeyT, class EntryT>
KeyT StandardAutoKeyIndex<KeyT, EntryT>::addEntry(const EntryT &argEntry) {
	const auto inputList = std::forward_list<EntryT>{ argEntry };
	
	std::forward_list<KeyT> retList = this->addEntries(inputList);
	
	if (retList.empty()) {
		errorf("addEntry received empty list");
	} else if (std::next(retList.begin()) != retList.end()) {
		errorf("addEntry received multiple keys");
	} else {
		return (KeyT) (*retList.begin());
	}
	
	return 0;
}

// TODO: functionality to add entry from pointer
/*
template <class KeyT, class EntryT>
KeyT StandardAutoKeyIndex<KeyT, EntryT>::addEntry(EntryT *argEntry) {
	
	//auto argPtr = &argEntry;
	auto argPtr = argEntry;
	const auto inputList = std::forward_list<EntryT *>{ argPtr };
	
	std::forward_list<KeyT> retList = this->addEntries(inputList);
	
	if (retList.empty()) {
		errorf("addEntry(raw) received empty list");
	} else if (std::next(retList.begin()) != retList.end()) {
		errorf("addEntry(raw) received multiple keys");
	} else {
		return (KeyT) (*retList.begin());
	}
	
	return 0;
}
*/

// TODO: functionality to add entry from shared_ptr
/*
template <class KeyT, class EntryT>
KeyT StandardAutoKeyIndex<KeyT, EntryT>::addEntry(std::shared_ptr<EntryT> argEntry) {
	
	const auto inputList = std::forward_list<std::shared_ptr<EntryT>>{ argEntry };
	
	std::forward_list<KeyT> retList = this->addEntries(inputList);
	
	if (retList.empty()) {
		errorf("addEntry(raw) received empty list");
	} else if (std::next(retList.begin()) != retList.end()) {
		errorf("addEntry(raw) received multiple keys");
	} else {
		return (KeyT) (*retList.begin());
	}
	
	return 0;
}
*/

template <class KeyT, class EntryT>
std::forward_list<KeyT> StandardAutoKeyIndex<KeyT, EntryT>::addEntries(const std::forward_list<EntryT> inputList) {
	if (inputList.empty()) {
		errorf("(StandardAutoKeyIndex.addEntries) inputList was empty");
		return {};
	}
	
	for (auto &inputEntry : inputList) {
		if (!this->isValidInputEntry(inputEntry)) {
			errorf("(StandardAutoKeyIndex.addEntries) inputList contained invalid entry");
			return {}; 
		}
	}
	
	std::filesystem::path indexFilePath = this->getDirPath() / (this->getFileStrBase() + ".bin");
	
	std::filesystem::path tempIndexPath = (indexFilePath);
	tempIndexPath.replace_extension("tmp");
	
	if (tempIndexPath.extension() != ".tmp") {
		errorf("failed to create tempIndexPath");
		return {};
	}
	std::shared_ptr<FilePathDeleter> fpathDeleterPtr; // assuming class deleters are called in reverse order of appearance
	
	std::fstream tempIndexStream;
	
	uint64_t lastEntryKey = 0;
	
	//! TODO: replace by something that checks existence otherwise (e.g. if renamed to .bak)
	
	// open file for writing and skip to end
	if (std::filesystem::exists(indexFilePath)) {
		lastEntryKey = this->getLastKey();
		
		std::error_code ec;
		bool copyres = std::filesystem::copy_file(indexFilePath, tempIndexPath, ec);
		
		if (!copyres) {
			errorf("(StandardAutoKeyIndex.addEntries) failed to copy to temp file");
			g_errorfStream << "tried to copy \"" << indexFilePath << "\" to \"" << tempIndexPath << "\"" << std::flush;
			g_errorfStream << "error code was: " << ec << std::flush;
			return {};
		}
		
		fpathDeleterPtr = std::make_shared<FilePathDeleter>(tempIndexPath);
		
		tempIndexStream = std::fstream(tempIndexPath, std::ios::binary | std::ios::out | std::ios::in);

		if (!tempIndexStream.is_open()) {
			errorf("failed to open tempIndexStream for read and write");
			g_errorfStream << "path was: " << tempIndexPath << std::flush;
			return {};
		}
		
		// skip to end
		if (lastEntryKey > 0) {
			//sets the output position to the end
			tempIndexStream.seekp(0, std::ios::end);
			if (tempIndexStream.fail()) {
				errorf("seekp failed");
				return {};
			}
		// skip to end manually
		} else {
			KeyT tempEntryKey = lastEntryKey;
			tempEntryKey = this->streamSkipEntryGetKey(tempIndexStream);
			while (!tempIndexStream.eof()) {
				if (tempEntryKey <= lastEntryKey) {
					errorf("tempEntryKey <= lastEntryKey");
					return {};
				} else {
					lastEntryKey = tempEntryKey;
				}
				tempEntryKey = this->streamSkipEntryGetKey(tempIndexStream);
			}
			if (tempIndexStream.fail()) {
				errorf("tempIndexStream entry read fail (1)");
				return {};
			}
			tempIndexStream.clear();
			
			//sets the output position
			tempIndexStream.seekp(0, std::ios::end);
			if (tempIndexStream.fail()) {
				errorf("seekp failed (2)");
				return {};
			}
		}
	} else {
		bool clearRes = this->clearRec();
		
		if (!clearRes) {
			errorf("clearRec() failed");
			return {};
		}
		
		tempIndexStream = std::fstream(tempIndexPath, std::ios::binary | std::ios::out | std::ios::trunc);
		fpathDeleterPtr = std::make_shared<FilePathDeleter>(tempIndexPath);
		
		if (!tempIndexStream.is_open()) {
			errorf("failed to create tempIndexStream for read and write");
			g_errorfStream << "path was: " << tempIndexPath << std::flush;
			return {};
		}
	}
	
	std::forward_list<std::pair<KeyT, EntryT>> regEntryPairList;
	
	// write the entríes and save entry pairs
	{
		auto insertPos = regEntryPairList.before_begin();
		
		for (auto &inputEntry : inputList) {
			lastEntryKey++;
			std::shared_ptr<std::pair<KeyT, EntryT>> entryPairPtr = this->streamWriteEntryGetPair(tempIndexStream, lastEntryKey, inputEntry);
			if (tempIndexStream.bad()) {
				errorf("tempIndexStream write failed");
				return {};
			} else if (entryPairPtr == nullptr) {
				errorf("entryPairPtr is null");
				return {};
			} else {
				insertPos = regEntryPairList.emplace_after(insertPos, *entryPairPtr);
			}
		}
	}
	
	tempIndexStream.close();
	
	if (tempIndexStream.fail()) {
		errorf("tempIndexStream close fail");
		return {};
	}
	
	int64_t nRegEntries = std::distance(regEntryPairList.begin(), regEntryPairList.end());
	
	if (nRegEntries < 0) {
		errorf("regEntryPairList negative length");
		return {};
	}
	
	//! TODO: return an object for atomic multi-file operation
	std::pair<bool, std::list<FileRenameOp>> reverseOpList = this->reverseAddEntries(regEntryPairList);
	
	if (!reverseOpList.first) {
		errorf("(addEntries) reverseAddEntries failed");
		return {};
	}
	
	//! TODO: add start and entry range as file positions
	bool addToLastFail = false;
	std::pair<bool, std::list<FileRenameOp>> addToLastOpList = this->addToLastEntryNum(tempIndexPath, nRegEntries, 0);
	
	//! TODO: automatically remove the .tmp files of opList on return
	
	if (!addToLastOpList.first) {
		errorf("(addEntries) addToLastEntryNum failed");
		//! TODO: cancel each in reverseOpList 
		
		return {};
	}
	
	if (fpathDeleterPtr) {
		fpathDeleterPtr->release();
	} else {
		errorf("fpathDeleterPtr is null");
		return {};
	}
	
	std::filesystem::path bakIndexPath = (indexFilePath);
	bakIndexPath.replace_extension("bak");
	
	if (bakIndexPath.extension() != ".bak") {
		errorf("failed to create bakIndexPath");
		return {};
	}
	
	std::list<FileRenameOp> renameOpList;
	renameOpList.emplace_back(tempIndexPath, indexFilePath, bakIndexPath);
	renameOpList.insert(renameOpList.end(), reverseOpList.second.begin(), reverseOpList.second.end());
	renameOpList.insert(renameOpList.end(), addToLastOpList.second.begin(), addToLastOpList.second.end());
	
	bool success = executeRenameOpList(renameOpList);
	if (!success) {
		errorf("executeRenameOpList failed");
		//! TODO: remove tmp files
		return {};
	}
	
	std::forward_list<KeyT> retList;
	
	{
		auto insertPos = retList.before_begin();
		
		for (auto &entryPair : regEntryPairList) {
			insertPos = retList.emplace_after(insertPos, entryPair.first);
		}
	}
errorf("returning retList");
	
	return retList;
}

/// MainIndexIndex

template <class U, class ... Ts>
std::shared_ptr<U> TopIndexSessionHandler::openSession(Ts&& ... args) {
	if constexpr (std::is_base_of<TopIndex, U>::value == true) {
		return this->openTopIndexSession<U>(args...);
	} else if constexpr (std::is_base_of<SubIndex, U>::value == true) {
		return this->openSubIndexSession<U>(args...);
	}
	return std::shared_ptr<U>();
}

template <class U, class ... Ts>
std::shared_ptr<U> TopIndexSessionHandler::openTopIndexSession(Ts&& ... args) {
	static_assert(std::is_base_of<TopIndex, U>::value == true);
	
	auto findIt = openSessions_.find(U::indexID);
	if (findIt != openSessions_.end()) {
		// downcast guaranteed by indexID of entry
		return std::static_pointer_cast<U>(findIt->second.lock());
	} else {
		//! probably could just construct to shared without the assistant function
		std::shared_ptr<U> sessionPtr = g_MakeSharedIndexSession<U>(*this, args...);
		if (sessionPtr != nullptr) {
			auto inputPair = std::pair<IndexID, std::weak_ptr<TopIndex>>(U::indexID, sessionPtr);
			if (inputPair.second.lock() != nullptr) {
				auto resPair = openSessions_.emplace(inputPair);
				if (resPair.first == openSessions_.end()) {
					errorf("failed to emplace handler_session_ptr");
				} else if (resPair.second == false) {
					errorf("indexID already registered");
				} else {
					return sessionPtr;
				}
			} else {
				errorf("(IndexSessionHandler) failed to create weak_ptr in pair");
			}
		} else {
			errorf("(IndexSessionHandler) failed to create sessionPtr");
		}
	}
	
	return std::shared_ptr<U>();
}

/// independent template function

template <class T, class ... Ts>
static std::shared_ptr<T> g_MakeSharedIndexSession(Ts&& ... args) {
	std::shared_ptr<T> created_ptr = std::shared_ptr<T>(new T(args...));
	return created_ptr;
}

/// SubIndexSessionHandler

template <class U, class ... Ts>
std::shared_ptr<U> SubIndexSessionHandler::openSession(std::shared_ptr<SubIndexSessionHandler> &shared_this, Ts&& ... args) {
	static_assert(std::is_base_of<SubIndex, U>::value == true);
	
	if (shared_this.get() == this) {
		auto findIt = openSessions_.find(U::indexID);
		if (findIt != openSessions_.end()) {
			// downcast guaranteed by indexID of entry
			return std::static_pointer_cast<U>(findIt->second.lock());
		} else {
			//! probably could just construct to shared without the assistant function
			std::shared_ptr<U> sessionPtr = g_MakeSharedIndexSession<U>(shared_this, args...);
			if (sessionPtr != nullptr) {
				auto inputPair = std::pair<IndexID, std::weak_ptr<SubIndex>>(U::indexID, sessionPtr);
				if (inputPair.second.lock() != nullptr) {
					auto resPair = openSessions_.emplace(inputPair);
					if (resPair.first == openSessions_.end()) {
						errorf("failed to emplace handler_session_ptr");
					} else if (resPair.second == false) {
						errorf("indexID already registered");
					} else {
						return sessionPtr;
					}
				} else {
					errorf("(IndexSessionHandler) failed to create weak_ptr in pair");
				}
			} else {
				errorf("(IndexSessionHandler) failed to create sessionPtr");
			}
		}
	}
	return std::shared_ptr<U>();
}

/// TopIndexSessionHandler

template <class U, class ... Ts>
std::shared_ptr<U> TopIndexSessionHandler::openSubIndexSession(uint64_t miNum, Ts&& ... args) {
	static_assert(std::is_base_of<SubIndex, U>::value == true);
	
	if (miNum > 0) {
		auto findIt = subHandlers_.find(miNum);
		if (findIt != subHandlers_.end()) {
			// downcast guaranteed by indexID of entry
			std::shared_ptr<SubIndexSessionHandler> handlerPtr(findIt->second.lock());
			if (handlerPtr != nullptr) {
				return handlerPtr->openSession<U>(handlerPtr, args...);
			}
		} else {
			std::shared_ptr<SubIndexSessionHandler> handlerPtr(new SubIndexSessionHandler(*this, miNum));
			if (handlerPtr != nullptr && handlerPtr->getMINum() > 0) {
				auto inputPair = std::pair<uint64_t, std::weak_ptr<SubIndexSessionHandler>>(miNum, handlerPtr);
				if (inputPair.second.lock() != nullptr) {
					auto resPair = subHandlers_.emplace(inputPair);
					if (resPair.first == subHandlers_.end()) {
						errorf("failed to emplace inputPair");
					} else if (resPair.second == false) {
						errorf("indexID already registered");
					} else {
						return handlerPtr->openSession<U>(handlerPtr, args...);
					}
				} else {
					errorf("(IndexSessionHandler) failed to create weak_ptr in pair");
				}
			} else {
				errorf("(IndexSessionHandler) failed to create handlerPtr");
			}
		}
	}
	
	return std::shared_ptr<U>();
}


#undef errorf

///

