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

#include "errorf.hpp"

#define errorf(str) g_errorfStdStr(str)

#include "ioextras.hpp"

#include "prgdir.hpp"

typedef struct searchexpr {
	unsigned short exprtype;
	struct {
		struct searchexpr *expr1;
		union {
			struct searchexpr *expr2;
			uint64_t refnum;
		};
	};
} SREXP;

typedef struct srexplist {
	SREXP *expr;
	struct srexplist *next;
} SREXPLIST;

typedef struct superexpstack {
	SREXP *expr;
	uint64_t subexp_layers;
	struct superexpstack *next;
} SUPEXPSTACK;

typedef struct tagnumstruct {
	uint64_t tagnum;
	unsigned char state;
} TAGNUMNODE;

typedef struct sstruct {
	TAGNUMNODE *tagnumarray;
	unsigned long ntagnums;
	struct searchexpr *rootexpr;
	uint64_t dnum;
} SEARCHSTRUCT;

struct subdirentry {
	char *subdirstr;
};

class FileCloser {
public:
	const FILE *fp;
	bool released = false;
	
	FileCloser() = delete;
	FileCloser(FILE *fp_) : fp{fp_} {}
	
	~FileCloser() {
		if (!released) {
			fclose(fp);
		}
	}
	
	void release(void) {
		released = true;
	}
};

class FilePathDeleter {
public:
	const std::filesystem::path fpath;
	bool released = false;
	
	FilePathDeleter() = delete;
	FilePathDeleter(std::filesystem::path &fpath_) : fpath{fpath_} {}
	
	~FilePathDeleter() {
		if (!released) {
			bool success = std::fs::remove(fpath);
			if (!success) {
				errorf("FilePathDeleter remove failed");
			}
		}
	}
	
	void release(void) {
		released = true;
	}
};

class FileRenameOp {
public:
	const std::filesystem::path from;
	const std::filesystem::path to;
	const std::filesystem::path bak;
	
	FileRenameOp() = delete;
	FileRenameOp(std::filesystem::path from_, std::filesystem::path to_, std::filesystem::path bak_) : from{from_}, to(to_), bak(bak_) {}
	
	bool hasFirstExecuted(void) const {
		return firstExecuted;
	}
	
	bool hasSecondExecuted(void) const {
		return secondExecuted;
	}
	
	bool hasReversed(void) const {
		return reversed;
	}
	
	bool firstExecute(void) {
g_errorfStream << "first-executing: " << to << std::flush;
		if (this->firstExecuted) {
			errorf("(FileRenameOp) already firstExecuted");
			return false;
		} else {
			std::error_code ec1;
			bool exists = std::fs::exists(to);
			if (ec1) {
				errorf("exists check failed in firstExecute");
			} else if (!exists) {
				this->hasOriginal = false;
				this->firstExecuted = true;
				return true;
			} else {
				std::error_code ec;
				std::filesystem::rename(to, bak, ec);
				if (ec) {
					g_errorfStream << "rename op failed: " << to << " to " << bak << std::flush;
					return false;
				} else {
errorf("firstExecute success");
					this->firstExecuted = true;
					return true;
				}
			}
		}
	}
	
	bool secondExecute(void) {
g_errorfStream << "second-executing: " << to << std::flush;
		if (!this->firstExecuted) {
			errorf("(FileRenameOp) haven't firstExecuted");
			return false;
		} else {
			std::error_code ec;
			std::filesystem::rename(from, to, ec);
			if (ec) {
				g_errorfStream << "rename op failed: " << from << " to " << to << std::flush;
				return false;
			} else {
errorf("secondExecute success");
				this->secondExecuted = true;
				return true;
			}
		}
	}
	
	bool reverse(void) {
		if (!this->firstExecuted && !this->secondExecuted) {
			return true;
		} else if (this->secondExecuted) {
			std::filesystem::path afterBak(to);
			afterBak += ".after.bak";
			
			std::error_code ec;
			std::filesystem::rename(to, afterBak, ec);
			if (ec) {
				g_errorfStream << "rename op failed: " << to << " to " << afterBak << std::flush;
				return false;
			} else {
				std::error_code ec;
				std::filesystem::rename(bak, to, ec);
				if (ec) {
					g_errorfStream << "rename op failed: " << bak << " to " << to << std::flush;
					return false;
				} else {
					this->reversed = true;
					return true;
				}
			}
		}
	}
	
	~FileRenameOp(void) {
		if (this->secondExecuted && hasOriginal) {
			bool success = std::fs::remove(bak);
			if (!success) {
				g_errorfStream << "FileRenameOp remove failed for: " << bak << std::flush;
			}
		}
	}
	
protected:
	bool firstExecuted = false;
	bool secondExecuted = false;
	bool reversed = false;
	bool hasOriginal = true;
	
};

bool reverseRenameOpList(std::list<FileRenameOp> opList);

bool executeRenameOpList(std::list<FileRenameOp> opList) {
	for (auto &op : opList) {
		bool success = op.firstExecute();
		if (!success) {
			errorf("firstExecute failed in list");
			
			bool reverseRes = reverseRenameOpList(opList);
			if (!reverseRes) {
				errorf("reverseRenameOpList failed");
			}
			
			return false;
		}
	}
	for (auto &op : opList) {
		bool success = op.secondExecute();
		if (!success) {
			errorf("secondExecute failed in list");
			
			bool reverseRes = reverseRenameOpList(opList);
			if (!reverseRes) {
				errorf("reverseRenameOpList failed");
			}
			
			return false;
		}
	}
errorf("executeRenameOpList success");
	return true;
}

bool reverseRenameOpList(std::list<FileRenameOp> opList) {
	bool notFailed = true;
	for (auto &op : opList) {
		bool success = op.reverse();
		if (!success) {
			notFailed = false;
		}
	}
	return notFailed;
}

///////////////////////

class IndexSession;
class IndexSessionHandler;
class SubIndexSessionHandler;

class TopIndex;
class SubIndex;

bool existsMainIndex(const uint64_t &minum);

class IndexID {
public:
	const std::string str;
	IndexID(std::string str_) : str{str_} {}
	bool operator==(const IndexID& rhs) const {
		this->str == rhs.str;
	}
	bool operator<(const IndexID& rhs) const {
		this->str < rhs.str;
	}
};

class HandlerAccessor {
	friend class IndexSession;
private:
	static void removeHandlerRefs(IndexSessionHandler &handler, const IndexID &indexID, const IndexSession *session_ptr);
};

//! TODO: make reference to parent (handler?) atomic

class IndexSession {
public:
	// indexID is assumed to be stored statically
	const IndexID &indexID;

	IndexSession() = delete;
	IndexSession(const IndexSession &) = delete;
	IndexSession &operator=(const IndexSession &other) = delete;
	
	IndexSession(IndexSessionHandler &handler_, const IndexID &indexID_) : handler{handler_}, indexID{indexID_} {}
	
	IndexSessionHandler &getHandler(void) {
		return this->handler;
	}
	
	// handler may be deleted by derived class
	virtual ~IndexSession(void) {}
	
	// forward declared
	void removeHandlerRefs(void);
	
protected:
	IndexSessionHandler &handler;
	
};

//{ Index Subclasses

class TopIndex : public IndexSession {
public:

	TopIndex(IndexSessionHandler &handler_, const IndexID &indexID_) : IndexSession(handler_, indexID_) {}
	
	virtual const std::filesystem::path getDirPath(void) const {
		return g_fsPrgDir;
	}
	
	virtual ~TopIndex(void) {
		this->removeHandlerRefs();
	}
	
};

class SubIndex : public IndexSession {
public:
	SubIndex() = delete;

	SubIndex(std::shared_ptr<SubIndexSessionHandler> &handler_, const IndexID &indexID_);
	
	static const std::filesystem::path getDirPathFor(uint64_t minum) {
		return g_fsPrgDir / "i" / ( std::to_string(minum) );
	}
	
	virtual const std::filesystem::path getDirPath(void) const {
		return this->getDirPathFor(this->getMINum());
	}
	
	uint64_t getMINum(void) const ;
	
	virtual ~SubIndex() {
		this->removeHandlerRefs();
	}
	
protected:
	std::shared_ptr<SubIndexSessionHandler> handler;
	
};

template <class T>
struct IOSpec;

// give no failbit if only read EOF -- give failbit if read characters and then EOF
template<>
struct IOSpec<uint64_t> {
	static void write(std::ostream &ios, const uint64_t &entry) {
		put_u64_stream_pref(ios, entry);
	}
	
	static uint64_t read(std::istream &ios) {
		bool b_gotNull = false;
		uint64_t uint = get_u64_stream_pref(ios, b_gotNull);
		if (b_gotNull) {
			errorf("IOSpec<uint64_t>::get -- b_gotNull");
			ios.setstate(std::ios_base::failbit);
			return 0;
		} else {
			return uint;
		}
	}
	
	static void skip(std::istream &ios) {
		bool b_gotNull = false;
		uint64_t uint = get_u64_stream_pref(ios, b_gotNull);
		if (b_gotNull) {
			errorf("IOSpec<uint64_t>::skip -- b_gotNull");
			ios.setstate(std::ios_base::failbit);
		}
	}
};

template<>
struct IOSpec<std::string> {
	static void write(std::ostream &ios, const std::string &entry) {
		ios << entry << '\0';
	}
	
	static std::string read(std::istream &ios) {
		std::string str;
		
		//! TODO: limit reading to MAX_PATH or something
		std::getline(ios, str, '\0');
		
		if (ios.eof()) {
			if (str != "") {
errorf("string set failbit");
				ios.setstate(std::ios_base::failbit);
			} else  {
errorf("string cleared failbit");
			// don't set failbit at EOF and read no characters
			ios.clear(std::ios_base::eofbit);
			}
		}
errorf("read string: " + str);
		return str;
	}
	
	static void skip(std::istream &ios) {
		std::string str;
		
		//! TODO: limit reading to MAX_PATH or something
		std::getline(ios, str, '\0');
		
		if (ios.eof()) {
			if (str != "") {
errorf("string set failbit");
				ios.setstate(std::ios_base::failbit);
			} else  {
errorf("string cleared failbit");
			// don't set failbit at EOF and read no characters
			ios.clear(std::ios_base::eofbit);
			}
		}
errorf("skipped string: " + str);
	}
};

template<>
struct IOSpec<std::fs::path> {
	static void write(std::ostream &ios, const std::fs::path &entry) {
		std::string pathString = entry.generic_string();
		ios << pathString << '\0';
	}
	
	static std::fs::path read(std::istream &ios) {
		std::string str;
		
		//! TODO: limit reading to MAX_PATH or something
		std::getline(ios, str, '\0');
		
		if (ios.eof()) {
			if (str != "") {
errorf("string set failbit");
				ios.setstate(std::ios_base::failbit);
			} else  {
errorf("string cleared failbit");
			// don't set failbit at EOF and read no characters
			ios.clear(std::ios_base::eofbit);
			}
		}
errorf("read string: " + str);
		return std::fs::path(str);
	}
	
	static void skip(std::istream &ios) {
		std::string str;
		
		//! TODO: limit reading to MAX_PATH or something
		std::getline(ios, str, '\0');
		
		if (ios.eof()) {
			if (str != "") {
errorf("string set failbit");
				ios.setstate(std::ios_base::failbit);
			} else  {
errorf("string cleared failbit");
			// don't set failbit at EOF and read no characters
			ios.clear(std::ios_base::eofbit);
			}
		}
errorf("skipped string: " + str);
	}
};

template <class T>
struct FixedIOSpec;

// give no failbit if only read EOF -- give failbit if read characters and then EOF
template<>
struct FixedIOSpec<uint64_t> {
	static void write(std::ostream &ios, const uint64_t &entry) {
		put_u64_stream_fixed(ios, entry);
	}
	
	static uint64_t read(std::istream &ios) {
		uint64_t uint = get_u64_stream_fixed(ios);
		
		return uint;
	}
	
	static void skip(std::istream &ios) {
		uint64_t uint = get_u64_stream_fixed(ios);
	}
};

template <class KeyT, class EntryT>
class StandardIndex {
public:
	typedef std::pair<KeyT, EntryT> EntryPair;
	
	virtual const std::string getFileStrBase(void) const = 0;
	
	virtual const std::filesystem::path getDirPath(void) const = 0;
	
	virtual bool isValidInputEntry(const EntryT&) const = 0;
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList) = 0;
	
	virtual ~StandardIndex(void) {

	}
	
	
	
	EntryT makeNullEntry(void) {
		return EntryT();
	}
	
	EntryT makeNullKey(void) {
		return KeyT();
	}
	
	std::shared_ptr<std::forward_list<EntryT>> readIntervalEntries(const KeyT &start, const uint64_t intrvl) {
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
	
	EntryT readEntry(const KeyT &entrykey) {
		auto retListPtr = readIntervalEntries(entrykey, 1);
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
	
	bool hasUndeletedEntry(const KeyT &entrykey) {
		if (this->readEntry(entrykey) != this->makeNullEntry()) {
			return true;
		} else {
			return false;
		}
	}
	
	
	
	bool removeEntry(KeyT);
	KeyT replaceEntry(KeyT, EntryT);
	
	
	
protected:
	
	KeyT streamGetKey(std::istream &ios) {
		KeyT key = IOSpec<KeyT>::read(ios);
		
		return key;
	}
	
	EntryT streamGetEntry(std::istream &ios) {
		EntryT entry = IOSpec<EntryT>::read(ios);
		
		return entry;
	}
	
	void streamSkipEntry(std::istream &ios) {
		IOSpec<EntryT>::skip(ios);
	}
	
	KeyT streamSkipEntryGetKey(std::istream &ios) {
		KeyT key = IOSpec<KeyT>::read(ios);

		if (ios.fail() || ios.eof()) {
			return key;
		}
		
		IOSpec<EntryT>::skip(ios);
		
		//! TODO: maybe verify content
		
		return key;
	}
	
	std::shared_ptr<EntryPair> streamWriteEntryGetPair(std::ostream &ios, const KeyT &entryKey, const EntryT &inputEntry) {
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
		
		return std::make_shared<EntryPair>(entryKey, inputEntry);
	}
	
	bool clearRec(void) {
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
	
	uint64_t createRec(int32_t &error) {
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
	
	std::pair<bool, std::list<FileRenameOp>> prepareRec(const std::fs::path &indexFilePath, std::shared_ptr<KeyT> &lastEntryKeyPtr) {
		
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
	
	std::pair<bool, std::list<FileRenameOp>> addToLastEntryNum(const std::fs::path &indexFilePath, const int64_t num, const uint64_t spot) {
		//! TODO:
		
		//! temp
		std::shared_ptr<KeyT> temp_ptr;
		return prepareRec(indexFilePath, temp_ptr);
		
		//return std::pair(true, std::list<FileRenameOp>());
		
		//return {};
	}
	
	uint64_t getKeySeekPos(const KeyT &key, int32_t &error) {
		error = 0;
		
		if (key == 0) {
			
		}
		
		
		error = 0;
		return 0;
	}
	
};

template <class KeyT, class EntryT>
class StandardManualKeyIndex : public StandardIndex<KeyT, EntryT> {
public:
	typedef std::pair<KeyT, EntryT> EntryPair;
	
	KeyT addEntry(KeyT, EntryT);
	
	virtual ~StandardManualKeyIndex(void) {

	}
};

// not actually specialized for key types other than scalar types
template <class KeyT, class EntryT>
class StandardAutoKeyIndex : public StandardIndex<KeyT, EntryT> {
public:
	typedef std::pair<KeyT, EntryT> EntryPair;
	
	virtual ~StandardAutoKeyIndex(void) {

	}
	
	KeyT getLastKey(void) {
		//! TODO:
		
		
		return 0;
	}
	
	// 'virtual' meaning some entries may actually be deleted
	uint64_t getNofVirtualEntries(int32_t &error) {
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

	KeyT addEntry(const EntryT &argEntry) {
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
	
	KeyT addEntry(EntryT *argEntry) {
		
		auto argPtr = &argEntry;		
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
	
	KeyT addEntry(std::shared_ptr<EntryT> argEntry) {
		
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

	std::forward_list<KeyT> addEntries(const std::forward_list<EntryT> inputList) {
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
		
		std::forward_list<EntryPair> regEntryPairList;
		
		// write the entríes and save entry pairs
		{
			auto insertPos = regEntryPairList.before_begin();
			
			for (auto &inputEntry : inputList) {
				lastEntryKey++;
				std::shared_ptr<EntryPair> entryPairPtr = this->streamWriteEntryGetPair(tempIndexStream, lastEntryKey, inputEntry);
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
	
	

};

class MainIndexIndex : public TopIndex, public StandardAutoKeyIndex<uint64_t, std::string> {
public:
	static const IndexID indexID;
	
	static constexpr uint64_t maxEntryLen = 500*4;
	
	MainIndexIndex(IndexSessionHandler &handler_) : TopIndex(handler_, this->indexID) {}
	
	virtual const std::string getFileStrBase(void) const override {
		return "miIndex";
	}
	
	virtual const std::filesystem::path getDirPath(void) const override {
		return this->TopIndex::getDirPath();
	}
	
	virtual bool isValidInputEntry(const std::string &entryString) const override {
		if (entryString == "") {
			return false;
		} else if (entryString.size() >= this->maxEntryLen) {
			return false;
		}
		
		return true;
	}
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList) override {
		//! TODO:
		return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
		
		return {};
	}
	
	virtual ~MainIndexIndex(void) {}
	
};

const IndexID MainIndexIndex::indexID = IndexID("1");

//}

template <class T, class ... Ts>
static std::shared_ptr<T> g_MakeSharedIndexSession(Ts&& ... args) {
	std::shared_ptr<T> created_ptr = std::shared_ptr<T>(new T(args...));
	return created_ptr;
}

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
	
	virtual bool removeRefs(const IndexID &indexID, const IndexSession *session_ptr) = 0;
	
};

void HandlerAccessor::removeHandlerRefs(IndexSessionHandler &handler, const IndexID &indexID, const IndexSession *session_ptr) {
	handler.removeRefs(indexID, session_ptr);
}

void IndexSession::removeHandlerRefs(void) {
	HandlerAccessor::removeHandlerRefs(this->handler, this->indexID, this);
}

class TopIndexSessionHandler : public IndexSessionHandler {
public:
	TopIndexSessionHandler() = default;

	template <class U, class ... Ts>
	std::shared_ptr<U> openSession(Ts&& ... args) {
		if constexpr (std::is_base_of<TopIndex, U>::value == true) {
			return this->openTopIndexSession<U>(args...);
		} else if constexpr (std::is_base_of<SubIndex, U>::value == true) {
			return this->openSubIndexSession<U>(args...);
		}
		return std::shared_ptr<U>();
	}
	
protected:
	std::map<IndexID, std::weak_ptr<TopIndex>> openSessions{};
	std::map<uint64_t, std::weak_ptr<SubIndexSessionHandler>> subHandlers{};
	
	virtual bool removeRefs(const IndexID &indexID, const IndexSession *session_ptr) {
		auto findIt = openSessions.find(indexID);
		if (findIt == openSessions.end()) {
			g_errorfStream << "(TopIndexSessionHandler::removeRefs) couldn't find indexID entry (indexID: " << indexID.str << ")" << std::flush; 
			//errorf("(TopIndexSessionHandler::removeRefs) couldn't find indexID entry");
		} else {
			if ((IndexSession *) findIt->second.lock().get() == session_ptr || (IndexSession *) findIt->second.lock().get() == nullptr) {
				
				openSessions.erase(findIt);
				return true;
			}
		}
		return false;
	}
	
	friend class SubIndexSessionHandler;
	
	bool removeSubHandlerRefs(const uint64_t &minum, const SubIndexSessionHandler *handler_ptr) {
		auto findIt = subHandlers.find(minum);
		if (findIt == subHandlers.end()) {
			errorf("(removeSubHandlerRefs) couldn't find minum entry");
		} else {
			if ((SubIndexSessionHandler *) findIt->second.lock().get() == handler_ptr || (SubIndexSessionHandler *) findIt->second.lock().get() == nullptr) {
				
				subHandlers.erase(findIt);
				return true;
			}
		}
		return false;
	}
	
private:
	
	template <class U, class ... Ts>
	std::shared_ptr<U> openTopIndexSession(Ts&& ... args) {
		static_assert(std::is_base_of<TopIndex, U>::value == true);
		
		auto findIt = openSessions.find(U::indexID);
		if (findIt != openSessions.end()) {
			// downcast guaranteed by indexID of entry
			return std::static_pointer_cast<U>(findIt->second.lock());
		} else {
			//! probably could just construct to shared without the assistant function
			std::shared_ptr<U> session_ptr = g_MakeSharedIndexSession<U>(*this, args...);
			if (session_ptr != nullptr) {
g_errorfStream << "session_ptr1: " << std::flush;
				auto inputPair = std::pair<IndexID, std::weak_ptr<TopIndex>>(U::indexID, session_ptr);
				if (inputPair.second.lock() != nullptr) {
					auto resPair = openSessions.emplace(inputPair);
					if (resPair.first == openSessions.end()) {
						errorf("failed to emplace handler_session_ptr");
					} else if (resPair.second == false) {
						errorf("indexID already registered");
					} else {
g_errorfStream << "session_ptr: " << session_ptr << std::flush;
g_errorfStream << "registered indexID: " << ((IndexID) U::indexID).str << std::flush;
						return session_ptr;
					}
				} else {
					errorf("(IndexSessionHandler) failed to create weak_ptr in pair");
				}
			} else {
				errorf("(IndexSessionHandler) failed to create session_ptr");
			}
		}
		
		return std::shared_ptr<U>();
	}
	
	template <class U, class ... Ts>
	std::shared_ptr<U> openSubIndexSession(uint64_t minum, Ts&& ... args);
	
	
};

class SubIndexSessionHandler : public IndexSessionHandler {
public:
	SubIndexSessionHandler() = delete;
	
	SubIndexSessionHandler(TopIndexSessionHandler &parent_, const uint64_t &minum_) : parent{parent_}, minum{minum_} {
		if (minum == 0) {
			errorf("SubIndexSessionHandler minum was 0");
		} else if (!existsMainIndex(minum)) {
			g_errorfStream << "existsmainindex returned false for: " << minum << std::flush;
		} else {
			std::fs::path miPath = SubIndex::getDirPathFor(minum);
			std::error_code error;
			bool success = std::fs::create_directory(miPath, error);
			if (error) {
				g_errorfStream << "(SubIndexSessionHandler) failed to create directory: " << miPath << std::flush;
			}
		}
	}

	template <class U, class ... Ts>
	std::shared_ptr<U> openSession(std::shared_ptr<SubIndexSessionHandler> &shared_this, Ts&& ... args) {
		static_assert(std::is_base_of<SubIndex, U>::value == true);
		
		if (shared_this.get() == this) {
			auto findIt = openSessions.find(U::indexID);
			if (findIt != openSessions.end()) {
				// downcast guaranteed by indexID of entry
				return std::static_pointer_cast<U>(findIt->second.lock());
			} else {
				//! probably could just construct to shared without the assistant function
				std::shared_ptr<U> session_ptr = g_MakeSharedIndexSession<U>(shared_this, args...);
				if (session_ptr != nullptr) {
					auto inputPair = std::pair<IndexID, std::weak_ptr<SubIndex>>(U::indexID, session_ptr);
					if (inputPair.second.lock() != nullptr) {
						auto resPair = openSessions.emplace(inputPair);
						if (resPair.first == openSessions.end()) {
							errorf("failed to emplace handler_session_ptr");
						} else if (resPair.second == false) {
							errorf("indexID already registered");
						} else {
							return session_ptr;
						}
					} else {
						errorf("(IndexSessionHandler) failed to create weak_ptr in pair");
					}
				} else {
					errorf("(IndexSessionHandler) failed to create session_ptr");
				}
			}
		}
		return std::shared_ptr<U>();
	}
	
	uint64_t getMINum(void) {
		return this->minum;
	}
	
	virtual ~SubIndexSessionHandler(void) {
		this->parent.removeSubHandlerRefs(this->getMINum(), this);
	}
	
protected:
	std::map<IndexID, std::weak_ptr<SubIndex>> openSessions{};
	const uint64_t minum;
	TopIndexSessionHandler &parent;
	
	virtual bool removeRefs(const IndexID &indexID, const IndexSession *session_ptr) {
		auto findIt = openSessions.find(indexID);
		if (findIt == openSessions.end()) {
			g_errorfStream << "(SubIndexSessionHandler::removeRefs) couldn't find indexID entry (indexID: " << indexID.str << ")" << std::flush; 
			//errorf("(removeRefs) couldn't find indexID entry");
		} else {
			if ((IndexSession *) findIt->second.lock().get() == session_ptr || (IndexSession *) findIt->second.lock().get() == nullptr) {
				
				openSessions.erase(findIt);
				
				return true;
			}
		}
		return false;
	}

};

SubIndex::SubIndex(std::shared_ptr<SubIndexSessionHandler> &handler_, const IndexID &indexID_) : handler{handler_}, IndexSession(*handler_.get(), indexID_) {}
	
uint64_t SubIndex::getMINum(void) const {
	return this->handler->getMINum();
}
	
template <class U, class ... Ts>
std::shared_ptr<U> TopIndexSessionHandler::openSubIndexSession(uint64_t minum, Ts&& ... args) {
	static_assert(std::is_base_of<SubIndex, U>::value == true);
	
	if (minum > 0) {
		auto findIt = subHandlers.find(minum);
		if (findIt != subHandlers.end()) {
			// downcast guaranteed by indexID of entry
			std::shared_ptr<SubIndexSessionHandler> handler_ptr(findIt->second.lock());
			if (handler_ptr != nullptr) {
				return handler_ptr->openSession<U>(handler_ptr, args...);
			}
		} else {
			std::shared_ptr<SubIndexSessionHandler> handler_ptr(new SubIndexSessionHandler(*this, minum));
			if (handler_ptr != nullptr && handler_ptr->getMINum() > 0) {
				auto inputPair = std::pair<uint64_t, std::weak_ptr<SubIndexSessionHandler>>(minum, handler_ptr);
				if (inputPair.second.lock() != nullptr) {
					auto resPair = subHandlers.emplace(inputPair);
					if (resPair.first == subHandlers.end()) {
						errorf("failed to emplace inputPair");
					} else if (resPair.second == false) {
						errorf("indexID already registered");
					} else {
						return handler_ptr->openSession<U>(handler_ptr, args...);
					}
				} else {
					errorf("(IndexSessionHandler) failed to create weak_ptr in pair");
				}
			} else {
				errorf("(IndexSessionHandler) failed to create handler_ptr");
			}
		}
	}
	
	return std::shared_ptr<U>();
}

extern TopIndexSessionHandler g_indexSessionHandler;

bool existsMainIndex(const uint64_t &minum) {
	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();
	
	if (indexSession == nullptr) {
		errorf("existsMainIndex could not open session");
		return false;
	}
	
	return indexSession->hasUndeletedEntry(minum);
}


//////////////


class MainIndexReverse : public TopIndex, public StandardManualKeyIndex<std::string, std::forward_list<uint64_t>> {
public:
	static const IndexID indexID;
	
	static constexpr uint64_t MaxKeyLen = MainIndexIndex::maxEntryLen;
	
	MainIndexReverse(IndexSessionHandler &handler_) : TopIndex(handler_, this->indexID) {}
	
	virtual const std::string getFileStrBase(void) const override {
		return "miReverseIndex";
	}
	
	virtual const std::filesystem::path getDirPath(void) const override {
		return this->TopIndex::getDirPath();
	}
	
	virtual bool isValidInputEntry(const std::forward_list<uint64_t> &list) const override {
		auto iterator1 = list.begin();
		if (iterator1 == list.end()) {
			return false;
		}
		
		return true;
	}
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList) override {
		return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
	}
	
	virtual ~MainIndexReverse(void) {}
	
};

const IndexID MainIndexReverse::indexID = IndexID("MainIndexReverse");

class MainIndexExtras {
public:
	static const IndexID indexID;
	
};

const IndexID MainIndexExtras::indexID = IndexID("MainIndexExtras");

class DirIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, std::fs::path> {
public:
	static const IndexID indexID;
	
	static constexpr uint64_t maxEntryLen = 260*4;
	
	DirIndex(std::shared_ptr<SubIndexSessionHandler> &handler_) : SubIndex(handler_, this->indexID) {}
	
	virtual const std::string getFileStrBase(void) const override {
		return "dIndex";
	}
	
	virtual const std::filesystem::path getDirPath(void) const override {
		return this->SubIndex::getDirPath();
	}
	
	virtual bool isValidInputEntry(const std::fs::path &entryPath) const override {
		if (entryPath.empty()) {
			return false;
		} else if (entryPath.generic_u8string().size() >= this->maxEntryLen) {
			return false;
		} else if (entryPath.is_relative() && (entryPath.begin() == entryPath.end() || *entryPath.begin() != "." )) {
			return false;
		}
		
		return true;
	}
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList) override {
		//! TODO:
		return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
		
		return {};
	}
	
	virtual ~DirIndex(void) {}
	
	
};
const IndexID DirIndex::indexID = IndexID("DirIndex");



class SubDirEntry {
public:
	SubDirEntry() = delete;
	
	SubDirEntry(uint64_t &parentInum_, std::fs::path &subPath_) : parentInum{parentInum_}, subPath{subPath_} {}
	
	bool isValid(void) {
		return false;
	}
	
	uint64_t parentInum;
	std::fs::path subPath;
	
	int64_t startDepth = -1;
	int64_t endDepth = -1;
	
	std::list<std::string> excludeDirs;
	std::list<std::string> excludeSubPaths;
	
};

template <>
struct IOSpec<SubDirEntry> {
public:
	static void write(std::ostream &ios, const SubDirEntry &entry) {
		ios.setstate(std::ios_base::badbit | std::ios_base::failbit);
		
		
	}
	
	static SubDirEntry read(std::istream &ios) {
		ios.setstate(std::ios_base::failbit);
		
		
	}
	
	static void skip(std::istream &ios) {
		ios.setstate(std::ios_base::failbit);
		
		
	}
};

class SubDirIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, SubDirEntry> {
public:
	static const IndexID indexID;
	
	SubDirIndex(std::shared_ptr<SubIndexSessionHandler> &handler_) : SubIndex(handler_, this->indexID) {}
	
	virtual const std::string getFileStrBase(void) const override {
		return "sdIndex";
	}
	
	virtual const std::filesystem::path getDirPath(void) const override {
		return this->SubIndex::getDirPath();
	}
	
	virtual bool isValidInputEntry(const SubDirEntry &entry) const override {
		return entry.isValid();
	}
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList) override {
		//! TODO:
		return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
		
		return {};
	}
		
};
const IndexID SubDirIndex::indexID = IndexID("SubDirIndex");

class FileIndexEntry {
	FileIndexEntry() = delete;
	
	FileIndexEntry(std::fs::path &filePath_, std::list<uint64_t> tagList_) : filePath{filePath_}, tagList{tagList_} {}
	
	std::fs::path filePath;
	std::list<uint64_t> tagList;
	
};

class FileIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, FileIndexEntry> {
public:
	static const IndexID indexID;



};
const IndexID FileIndex::indexID = IndexID("FileIndex");

class TagIndexEntry {
	
};

class TagIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, TagIndexEntry> {
public:
	static const IndexID indexID;


};
const IndexID TagIndex::indexID = IndexID("TagIndex");

class TagAliasEntry {
	
};

class TagAliasIndex : public SubIndex, public StandardAutoKeyIndex<uint64_t, FileIndexEntry> {
public:
	static const IndexID indexID;



};
const IndexID TagAliasIndex::indexID = IndexID("TagAliasIndex");

unsigned char raddtolastminum(long long num);

uint64_t rmiinitpos(char *idir);

char rmiinit();

char rmireg(char *entrystr, uint64_t inum);

unsigned char addtolastminum(long long num, uint64_t spot);

uint64_t miinitpos(uint64_t iinum);



unsigned char raddtolastdnum(uint64_t minum, long long num);

uint64_t rdinitpos(uint64_t minum, char *idir);

char rdinit(uint64_t minum);

// // char setdlastchecked(uint64_t minum, uint64_t dnum, uint64_t ulltime);

// // uint64_t getdlastchecked(uint64_t minum, uint64_t dnum);

unsigned char addtolastdnum(uint64_t minum, long long num, uint64_t spot);

uint64_t dinitpos(uint64_t minum, uint64_t idnum);




unsigned char addtolastsdnum(uint64_t minum, long long num, uint64_t spot);

uint64_t sdinitpos(uint64_t minum, uint64_t idnum);



int cfireg(uint64_t dnum, oneslnk *fnamechn); //!


int tnumfromalias2(uint64_t dnum, twoslnk **sourcelist);

#undef errorf