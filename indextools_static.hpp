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

extern std::filesystem::path g_fsPrgDirPath;

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
g_errorfStream << "first executing: " << to << std::flush;
		if (this->firstExecuted) {
			errorf("(FileRenameOp) already firstExecuted");
			return false;
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
	
	bool secondExecute(void) {
g_errorfStream << "second executing: " << to << std::flush;
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
		if (this->secondExecuted) {
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

class TopIndex;
class SubIndex;

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
	static removeHandlerRefs(IndexSessionHandler &handler, const IndexID &indexID, const IndexSession *session_ptr);
};

//! TODO: make reference to parent (handler?) atomic

class IndexSession {
public:
	// indexID is assumed to be stored statically
	const IndexID &indexID;

	IndexSession() = delete;
	IndexSession(IndexSessionHandler &handler_, const IndexID &indexID_) : handler{handler_}, indexID{indexID_} {
		
	}
	IndexSessionHandler &getHandler(void) {
		return this->handler;
	}
	
	~IndexSession(void) {
		this->removeHandlerRefs();
	}
	
	// forward declared
	void removeHandlerRefs(void);
	
protected:
	IndexSessionHandler &handler;
	
};

//{ Index Subclasses

class TopIndex : public IndexSession {
public:
	TopIndex(IndexSessionHandler &handler_, const IndexID &indexID_) : IndexSession(handler_, indexID_) {}
	
	virtual ~TopIndex(void) {}
	
};

class SubIndex : public IndexSession {
public:
	
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

template <class T>
struct FixedIOSpec;

// give no failbit if only read EOF -- give failbit if read characters and then EOF
template<>
struct FixedIOSpec<uint64_t> {
	static void write(std::ostream &ios, const uint64_t &entry) {
if (ios.fail()) {
	errorf("already failed");
	return;
} else {
	errorf("not failed");
}
		ios << entry;
if (ios.fail()) {
	errorf("failed after");
}
	}
	
	static uint64_t read(std::istream &ios) {
		if (ios.peek() == EOF) {
			ios.setstate(std::ios_base::eofbit);
		}
		uint64_t entry;
		ios >> entry;
		return entry;
	}
	
	static void skip(std::istream &ios) {
		if (ios.peek() == EOF) {
			ios.setstate(std::ios_base::eofbit);
		}
		uint64_t entry;
		ios >> entry;
	}
};

template <class KeyT, class InnerKeyT, class EntryT, class InnerEntryT>
class StandardIndex {
public:
	bool removeEntry(KeyT);
	KeyT replaceEntry(KeyT, EntryT);
	
	virtual ~StandardIndex(void) {

	}
	
};

template <class KeyT, class InnerKeyT, class EntryT, class InnerEntryT>
class StandardManualKeyIndex : public StandardIndex<KeyT, InnerKeyT, EntryT, InnerEntryT> {
public:
	KeyT addEntry(KeyT, EntryT);
	
	virtual ~StandardManualKeyIndex(void) {

	}
};

// not actually specialized for key types other than scalar types
template <class KeyT, class InnerKeyT, class EntryT, class InnerEntryT>
class StandardAutoKeyIndex : public StandardIndex<KeyT, InnerKeyT, EntryT, InnerEntryT> {
public:
	typedef std::pair<KeyT, EntryT> EntryPair;
	
	//typedef std::conditional<true, int, double>::type InnerKeyT;
	
	virtual const std::string getFileStrBase(void) = 0;
	
	virtual const std::filesystem::path getDirPath(void) = 0;
	
	virtual bool isValidInputEntry(EntryT) = 0;
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList) = 0;
	
	virtual ~StandardAutoKeyIndex(void) {

	}
	
	KeyT skipEntryGetKey(std::istream &ios) {
		KeyT key = IOSpec<KeyT>::read(ios);

		if (ios.fail() || ios.eof()) {
			return key;
		}
		
		IOSpec<EntryT>::skip(ios);
		
		//! TODO: maybe verify content
		
		return key;
	}
	
	std::shared_ptr<EntryPair> writeEntryGetPair(std::ostream &ios, const KeyT entryKey, const EntryT &inputEntry) {
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
	
	/*
	void skipEntry(std::istream &ios) {
		
		//! TODO: maybe verify content
		KeyT key = 
	}
	*/
	
	bool clearRec(void) {
		//! TODO:
		return false;
	}
	
	KeyT getLastKey(void) {
		//! TODO:
		
		
		return 0;
	}
	
	uint64_t createRec(int32_t &error) {
		error = 0;
		
		std::filesystem::path indexRecPath = this->getDirPath() / (this->getFileStrBase() + ".rec.bin");
		
		std::filesystem::path tempIndexRecPath = this->getDirPath() / (this->getFileStrBase() + ".rec.tmp");
		
		if (tempIndexRecPath.extension() != ".tmp") {
			errorf("failed to create tempIndexRecPath");
			error = 1;
			return 0;
		}
		
		// remove indexRec
		{	
			std::error_code ec;
			std::filesystem::remove(indexRecPath, ec); // returns (bool) false if path didn't exist or if there's an error
			if (ec) {
				errorf("failed to remove indexRecPath");
				g_errorfStream << "tried to remove: " << indexRecPath << std::flush;
				error = 1;
				return 0;
			}
		}
		
		// remove tempIndexRec
		{	
			std::error_code ec;
			std::filesystem::remove(tempIndexRecPath, ec); // returns (bool) false if path didn't exist or if there's an error
			if (ec) {
				errorf("failed to remove tempIndexRecPath");
				g_errorfStream << "tried to remove: " << tempIndexRecPath << std::flush;
				error = 1;
				return 0;
			}
		}
		
		std::filesystem::path indexFilePath = this->getDirPath() / (this->getFileStrBase() + ".bin");
			
		std::ifstream indexStream = std::ifstream(indexFilePath, std::ios::binary | std::ios::in);
		
		//! TODO: should not be error if indexFile doesn't exist
		if (!indexStream.is_open()) {
			errorf("failed to open indexStream for read");
			error = 1;
			return 0;
		}
		
		std::shared_ptr<FilePathDeleter> tempRecDeleterPtr; // assuming class deleters are called in reverse order of appearance
			
		std::ofstream tempRecStream = std::ofstream(tempIndexRecPath, std::ios::binary | std::ios::out | std::ios::trunc | std::ios::in);
		
		if (!tempRecStream.is_open()) {
			errorf("failed to open tempRecStream for write");
			error = 1;
			return 0;
		}
		
		tempRecDeleterPtr = std::make_shared<FilePathDeleter>(tempIndexRecPath);
		
		// get the last entry key
		//! TODO: 
		KeyT lastEntryKey = 0;
		{
			KeyT tempEntryKey = this->skipEntryGetKey(indexStream);
			while (!indexStream.eof()) {
				if (tempEntryKey <= lastEntryKey) {
					errorf("tempEntryKey <= lastEntryKey");
					error = 1;
					return 0;
				} else {
					lastEntryKey = tempEntryKey;
				}
				tempEntryKey = this->skipEntryGetKey(indexStream);
			}
			if (indexStream.fail()) {
				errorf("indexStream entry read fail (1)");
				error = 1;
				return 0;
			}
			indexStream.clear();
		}
		
		indexStream.close();
		
		FixedIOSpec<uint64_t>::write(tempRecStream, lastEntryKey);
		
		if (tempRecStream.fail()) {
			errorf("failed to write to tempRecStream");
			error = 1;
			return 0;
		}
		
		tempRecStream.close();
		tempRecDeleterPtr->release();
		
		/*
		std::list<FileRenameOp> renameOpList;
		std::filesystem::path bakIndexRecPath = this->getDirPath() / (this->getFileStrBase() + ".rec.bin.bak");
		renameOpList.emplace_back(tempIndexRecPath, indexRecPath, bakIndexRecPath);
		
		bool success = executeRenameOpList(renameOpList);
		if (!success) {
			errorf("executeRenameOpList failed");
			//! TODO: remove tmp files
			error = 1;
			return 0;
		}
		*/
		
		std::error_code ec;
		std::fs::rename(tempIndexRecPath, indexRecPath, ec);
		if (ec) {
			g_errorfStream << "createRec rename failed: " << tempIndexRecPath << " to " << indexRecPath << std::flush;
			error = 1;
			return 0;
		}
		
		error = 0;
		return lastEntryKey;
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
	
	std::pair<bool, std::list<FileRenameOp>> addToLastEntryNum(long long num, uint64_t spot) {
		//! TODO:
		return std::pair(true, std::list<FileRenameOp>());
		
		return {};
	}

	KeyT addEntry(EntryT &argEntry) {
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
	
	KeyT addEntry(InnerEntryT *argEntry) {
		static_assert(!std::is_same<InnerEntryT, EntryT>::value);
		
		auto argPtr = &argEntry;		
		const auto inputList = std::forward_list<InnerEntryT *>{ argPtr };
		
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
				tempEntryKey = this->skipEntryGetKey(tempIndexStream);
				while (!tempIndexStream.eof()) {
					if (tempEntryKey <= lastEntryKey) {
						errorf("tempEntryKey <= lastEntryKey");
						return {};
					} else {
						lastEntryKey = tempEntryKey;
					}
					tempEntryKey = this->skipEntryGetKey(tempIndexStream);
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
				return {};
			}
		}
		
		std::forward_list<EntryPair> regEntryPairList;
		
		// write the entríes and save entry pairs
		{
			auto insertPos = regEntryPairList.before_begin();
			
			for (auto &inputEntry : inputList) {
				lastEntryKey++;
				std::shared_ptr<EntryPair> entryPairPtr = this->writeEntryGetPair(tempIndexStream, lastEntryKey, inputEntry);
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
		
		//! TODO: return an object for atomic multi-file operation
		std::pair<bool, std::list<FileRenameOp>> reverseOpList = this->reverseAddEntries(regEntryPairList);
		
		if (!reverseOpList.first) {
			errorf("(addEntries) reverseAddEntries failed");
			return {};
		}
		
		uint64_t nRegEntries = std::distance(regEntryPairList.begin(), regEntryPairList.end());
		
		//! TODO: add start and entry range as file positions
		bool addToLastFail = false;
		std::pair<bool, std::list<FileRenameOp>> addToLastOpList = this->addToLastEntryNum(nRegEntries, 0);
		
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

	//std::forward_list<KeyT> addEntries(const std::forward_list<std::shared_ptr<const EntryT>>);
	
};

template <class KeyT, class EntryT>
class StandardAutoKeyIndexI : public StandardAutoKeyIndex<KeyT, KeyT, EntryT, EntryT> {
public:
	
	virtual ~StandardAutoKeyIndexI(void) {
		
	}
};

template <class InnerKeyT, class InnerEntryT>
class StandardAutoKeyIndexI<std::shared_ptr<InnerKeyT>, std::shared_ptr<InnerEntryT>> : public StandardAutoKeyIndex<std::shared_ptr<InnerKeyT>, InnerKeyT, std::shared_ptr<InnerEntryT>, InnerEntryT> {
public:
	
	
	virtual ~StandardAutoKeyIndexI(void) {
		
	}
};

template <class InnerKeyT, class EntryT>
class StandardAutoKeyIndexI<std::shared_ptr<InnerKeyT>, EntryT> : public StandardAutoKeyIndex<std::shared_ptr<InnerKeyT>, InnerKeyT, EntryT, EntryT> {
public:
	virtual ~StandardAutoKeyIndexI(void) {
		
	}
};

template <class KeyT, class InnerEntryT>
class StandardAutoKeyIndexI<KeyT, std::shared_ptr<InnerEntryT>> : public StandardAutoKeyIndex<KeyT, KeyT, std::shared_ptr<InnerEntryT>, InnerEntryT> {
public:
	virtual ~StandardAutoKeyIndexI(void) {
		
	}
};

class MainIndexIndex : public TopIndex, public StandardAutoKeyIndexI<uint64_t, std::string>  {
public:
	static const IndexID indexID;
	
	static constexpr uint64_t MaxEntryLen = 500*4;
	
	MainIndexIndex(IndexSessionHandler &handler_) : TopIndex(handler_, this->indexID) {}
	
	virtual const std::string getFileStrBase(void) override {
		return "miIndex";
	}
	
	virtual const std::filesystem::path getDirPath(void) override {
		return g_fsPrgDirPath;
	}
	
	virtual bool isValidInputEntry(std::string entryString) override {
		if (entryString == "") {
			return false;
		} else if (entryString.size() >= this->MaxEntryLen) {
			return false;
		}
		
		return true;
	}
	
	virtual std::pair<bool, std::list<FileRenameOp>> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList) override {
		//! TODO:
		return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
		
		return {};
	}
	
	virtual ~MainIndexIndex(void) {
errorf("mii deleter");
	}
	
};

const IndexID MainIndexIndex::indexID = IndexID("1");

class MainIndexExtras {
public:
	//addEntry(std::string entryName);
	//removeEntry(uint64_t inum);
	//replaceEntry(uint64_t inum, std::string newName);
	
};

class DirIndex {
public:
	addEntry(std::string dirPath);
	removeEntry(uint64_t inum);
	replaceEntry(uint64_t inum, std::string newPath);
	
};

class SubDirIndex {
public:
	addEntry(std::string subdirPath);
	removeEntry(uint64_t inum);
	replaceEntry(uint64_t inum, std::string newSubpath);
	
};

class FileIndex {
public:
	addEntry(std::string subdirPath);
	removeEntry(uint64_t inum);
	replaceEntry(uint64_t inum, std::string newSubpath);
};

class TagIndex {
public:
	addEntry(std::string subdirPath);
	removeEntry(uint64_t inum);
	replaceEntry(uint64_t inum, std::string newSubpath);
};

class TagAliasIndex {
public:
	addEntry(std::string subdirPath);
	removeEntry(uint64_t inum);
	replaceEntry(uint64_t inum, std::string newSubpath);
};

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
	template <class U, class ... Ts>
	std::shared_ptr<U> openSession(Ts&& ... args) {
		if constexpr (std::is_base_of<TopIndex, U>::value == true) {
			auto findIt = openSessions.find(U::indexID);
			if (findIt != openSessions.end()) {
				// downcast guaranteed by indexID of entry
				return std::static_pointer_cast<U>(findIt->second.lock());
			} else {
				//! probably could just construct to shared without the assistant function
				std::shared_ptr<U> session_ptr = g_MakeSharedIndexSession<U>(*this, args...);
				if (session_ptr != nullptr) {
					auto inputPair = std::pair<IndexID, std::weak_ptr<TopIndex>>(U::indexID, session_ptr);
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
		} else if constexpr (std::is_base_of<SubIndex, U>::value == true) {
			
		}
		return std::shared_ptr<U>();
	}
	
protected:
	std::map<IndexID, std::weak_ptr<TopIndex>> openSessions{};
	
	// get around mutual dependency and privacy
	friend class HandlerAccessor;
	
	bool removeRefs(const IndexID &indexID, const IndexSession *session_ptr) {
		auto findIt = openSessions.find(indexID);
		if (findIt == openSessions.end()) {
			errorf("(removeRefs) couldn't find indexID entry");
		} else {
			if ((IndexSession *) findIt->second.lock().get() == session_ptr || (IndexSession *) findIt->second.lock().get() == nullptr) {
				
				openSessions.erase(findIt);
				return true;
			}
		}
		return false;
	}
	
};

HandlerAccessor::removeHandlerRefs(IndexSessionHandler &handler, const IndexID &indexID, const IndexSession *session_ptr) {
	handler.removeRefs(indexID, session_ptr);
}

void IndexSession::removeHandlerRefs(void) {
	HandlerAccessor::removeHandlerRefs(this->handler, this->indexID, this);
}

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