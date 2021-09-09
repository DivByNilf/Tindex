#pragma once

extern "C" {
#include "stringchains.h"
}

#include <stdio.h>
#include <stdint.h>

#include <map>
#include <forward_list>
#include <list>
#include <string>
#include <memory>
#include <filesystem>
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
			bool success = remove(fpath);
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
	
	
	bool firstExecute(void) {
		if (firstExecuted) {
			errorf("already firstExecuted");
			return false;
		} else {
			return false;
		}
	}
	
	bool secondExecute(void) {
		if (!firstExecuted) {
			errorf("haven't firstExecuted");
			return false;
		} else {
			return false;
		}
	}
	
protected:
	bool firstExecuted = false;
	bool secondExecuted = false;
	
};

bool executeRenameOpList(const std::list<FileRenameOp> opList) {
	for (auto op : opList) {
		bool success = op.firstExecute();
		if (!success) {
			errorf("firstExecute failed in list");
			return false;
		}
	}
	for (auto op : opList) {
		bool success = op.secondExecute();
		if (!success) {
			errorf("secondExecute failed in list");
			return false;
		}
	}
	return true;
}

///////////////////////

class IndexSession;
class IndexSessionHandler;

class TopIndex;
class SubIndex;

class MainIndexIndex; //!

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
	IndexSession(IndexSessionHandler &handler_, const IndexID &indexID_) : handler{handler_}, indexID{indexID_}, nrefs{0}, nHandlerRefs{0} {
		
	}
	IndexSessionHandler &getHandler(void) {
		return this->handler;
	}
	
	~IndexSession(void) {
errorf("IndexSession deleter");
	}
	
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

template <class KeyT>
class AutoKey;

/*
template <class KeyT>
class AutoKey<uint64_t> {
	
}
*/

template <class T>
struct IOSpec;

template<>
struct IOSpec<uint64_t> {
	static void write(std::fstream &fs, uint64_t &entry) {
		put_u64_stream_pref(fs, entry);
	}
	
	static uint64_t read(std::fstream &fs) {
		bool b_gotNull = false;
		uint64_t uint = get_u64_stream_pref(fs, b_gotNull);
		if (b_gotNull) {
			errorf("IOSpec<uint64_t>::get -- b_gotNull");
			fs.setstate(std::ios_base::failbit);
			return 0;
		} else {
			return uint;
		}
	}
	
	static void skip(std::fstream &fs) {
		bool b_gotNull = false;
		uint64_t uint = get_u64_stream_pref(fs, b_gotNull);
		if (b_gotNull) {
			errorf("IOSpec<uint64_t>::skip -- b_gotNull");
			fs.setstate(std::ios_base::failbit);
		}
	}
};

template<>
struct IOSpec<std::string> {
	static void write(std::fstream &fs, const std::string &entry) {
		fs << entry;
	}
	
	static std::string read(std::fstream &fs) {
		std::string str;
		
		//! TODO: limit reading to MAX_PATH or something
		std::getline(fs, str, '\0');
		return str;
	}
	
	static void skip(std::fstream &fs) {
		std::string str;
		
		//! TODO: limit reading to MAX_PATH or something
		std::getline(fs, str, '\0');
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

template <class KeyT, class InnerKeyT, class EntryT, class InnerEntryT>
class StandardAutoKeyIndex : public StandardIndex<KeyT, InnerKeyT, EntryT, InnerEntryT> {
public:
	typedef std::pair<KeyT, EntryT> EntryPair;
	
	//typedef std::conditional<true, int, double>::type InnerKeyT;
	
	virtual const std::string getFileStrBase(void) = 0;
	
	virtual const std::filesystem::path getDirPath(void) = 0;
	
	virtual bool isValidInputEntry(EntryT) = 0;
	
	virtual std::list<FileRenameOp> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList, bool &argFail) = 0;
	
	virtual ~StandardAutoKeyIndex(void) {

	}
	
	KeyT skipEntryGetKey(std::fstream &fs) {
		KeyT key = IOSpec<KeyT>::read(fs);
		
		IOSpec<EntryT>::skip(fs);
		
		//! TODO: maybe verify content
		
		return key;
	}
	
	std::shared_ptr<EntryPair> writeEntryGetPair(std::fstream &fs, KeyT entryKey, EntryT &inputEntry) {
		IOSpec<KeyT>::write(fs, entryKey);
		
		if (fs.fail() || fs.bad()) {
			fs.setstate(std::ios_base::badbit);
			return {};
		}
		
		IOSpec<EntryT>::write(fs, inputEntry);
		
		if (fs.fail() || fs.bad()) {
			fs.setstate(std::ios_base::badbit);
			return {};
		}
		
		return std::make_shared<EntryPair>(entryKey, inputEntry);
	}
	
	/*
	void skipEntry(std::fstream &fs) {
		
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
	
	std::list<FileRenameOp> addToLastEntryNum(long long num, uint64_t spot, bool &b_argFail) {
		//! TODO:
		b_argFail = true;
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
			errorf("addEntry received empty list");
		} else if (std::next(retList.begin()) != retList.end()) {
			errorf("addEntry received multiple keys");
		} else {
			return (KeyT) (*retList.begin());
		}
		
		return 0;
	}
	
	/*
	std::shared_ptr<KeyT> addEntry(std::shared_ptr<const EntryT> argPtr) {
		const auto inputList = std::forward_list<std::shared_ptr<const EntryT>>(argPtr);
		std::forward_list<KeyT> retList = this->addEntries(inputList);
		if (retList.empty()) {
			errorf("addEntry received empty list");
		} else if (retList.end() - retList.begin() > 1) {
			errorf("addEntry received multiple keys");
		} else {
			return std::shared_ptr<KeyT>(retList.begin());
		}
	}
	*/

	std::forward_list<KeyT> addEntries(const std::forward_list<EntryT> inputList) {
		if (inputList.empty()) {
			errorf("(StandardAutoKeyIndex.addEntries) inputList was empty");
			return {};
		}
		/*
		if (inputchn == 0) {
			errorf("inputchn is null");
			return 1;
		}
		*/
		
		/*
		long int maxentrylen = MAX_PATH*4;
		
		oneslnk *inputchn = minamechn;
		char *fileStrBase = "miIndex";
		#define getlastentrynum() getlastminum()
		#define addtolastinum(num1, num2) addtolastminum(num1, num2)
		#define crentryreg(numentrychn) crmireg(numentrychn)
		
		unsigned char buf[MAX_PATH*4];
		*/
		
		for (auto inputEntry : inputList) {
			if (!this->isValidInputEntry(inputEntry)) {
				errorf("(StandardAutoKeyIndex.addEntries) inputList contained invalid entry");
				return {}; 
			}
		}
		
		/*
		for (oneslnk *link = inputchn; link != 0; link = link->next) {
			int i = 0;
			if (link->str) {
				for (i = 0; link->str[i] != '\0' && i != maxentrylen; i++);
				if (i >= maxentrylen) {
					errorf("input string too long");
					return 1;
				}
			} if (i == 0 || link->str == 0) {
				errorf("empty string");
				return 1;
			}
		}
		*/
		
		std::filesystem::path indexFilePath = this->getDirPath() / (this->getFileStrBase() + ".bin");
		
		std::filesystem::path tempIndexPath = (indexFilePath);
		tempIndexPath.replace_extension("tmp");
		
		if (tempIndexPath.extension() != ".tmp") {
			errorf("failed to create tempIndexPath");
			return {};
		}
		std::shared_ptr<FilePathDeleter> fpathDeleterPtr; // assuming class deleters are called in reverse order of appearance
		
		std::fstream tempIndexStream;
		
		FILE *indexF;
		char created = 0;
		
		uint64_t lastEntryKey = 0;
		//! TODO: replace by something that checks existence otherwise (e.g. if renamed to .bak)
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
			
			//! TODO: make a deleter for the copy
			fpathDeleterPtr = std::make_shared<FilePathDeleter>(tempIndexPath);
			
			tempIndexStream = std::fstream(tempIndexPath, std::ios::binary | std::ios::out | std::ios::in);
	
			if (!tempIndexStream.is_open()) {
				errorf("failed to open tempIndexStream for read and write");
				return {};
			}
			
			//! skip to end
			
			if (lastEntryKey > 0) {
				//sets the output position
				tempIndexStream.seekp(0, std::ios::end);
				if (tempIndexStream.fail()) {
					errorf("seekp failed");
					return {};
				}
			} else {
				//! skip to end manually
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
				errorf("failed to open tempIndexStream for read and write");
				return {};
			}
		}
		
		/*
		
		sprintf(buf, "%s\\%s.bin", g_prgDir, fileStrBase);
		if ((indexF = MBfopen(buf, "rb+")) == NULL) {
			if ((indexF = MBfopen(buf, "wb+")) == NULL) {
				errorf("couldn't create indexF");
				errorf_old("tried to create: \"%s\"", buf);
				return {};
			}
			created = 1;
		}
		
		uint64_t lastentrynum = 0;
		
		//if (lastdnum = getlastdnum()) {
		if ( (!created) && ((lastentrynum = getlastentrynum()) > 0) ) {
			fseek(indexF, 0, SEEK_END);
		} else {
			int c;
			while ((c = getc(indexF)) != EOF) {
				if (fseek(indexF, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(indexF);
					return {};
				}
				
				if ((c = null_fgets(0, maxentrylen, indexF)) != 0) {
					errorf_old("indexF read error: %d", c);
					fclose(indexF);
					return {};
				}
				lastentrynum++;		// assuming there are no entry number gaps
			}
		}
		
		*/
		
		std::forward_list<EntryPair> regEntryPairList;
		
		{
			auto insertPos = regEntryPairList.begin();
			
			for (auto inputEntry : inputList) {
				lastEntryKey++;
				std::shared_ptr<EntryPair> entryPairPtr = this->writeEntryGetPair(tempIndexStream, lastEntryKey, inputEntry);
				if (tempIndexStream.bad()) {
					errorf("tempIndexStream write failed");
					return {};
				} else if (!entryPairPtr) {
					errorf("entryPairPtr is null");
					return {};
				} else {
					insertPos = regEntryPairList.insert_after(insertPos, *entryPairPtr);
				}
			}
		}
		
		tempIndexStream.close();
		
		if (tempIndexStream.fail()) {
			errorf("tempIndexStream close fail");
			return {};
		}
		
		/*
		
		twoslnk *numentrychn, *fnumentry;
		
		fnumentry = numentrychn = malloc(sizeof(twoslnk));
		numentrychn->u[0].str = 0;
		
		int i = 0;
		{
			oneslnk *link = inputchn;
			for (; link != 0; link = link->next, i++) {
				putull_pref(++lastentrynum, indexF);
				term_fputs(link->str, indexF);
				numentrychn = numentrychn->next = malloc(sizeof(twoslnk));
				numentrychn->u[0].str = link->str, numentrychn->u[1].ull = lastentrynum;
			}
		}
		fclose(indexF);
		
		numentrychn->next = 0;
		numentrychn = fnumentry->next, free(fnumentry);
		*/
		
		//! TODO: return an object for atomic multi-file operation
		bool reverseAddSuccess = false;
		std::list<FileRenameOp> reverseOpList = this->reverseAddEntries(regEntryPairList, reverseAddSuccess);
		
		if (!reverseAddSuccess) {
			return {};
		}
		
		uint64_t nRegEntries = std::distance(regEntryPairList.begin(), regEntryPairList.end());
		
		//! TODO: add start and entry range as file positions
		bool addToLastFail = false;
		std::list<FileRenameOp> addToLastOpList = this->addToLastEntryNum(nRegEntries, 0, addToLastFail);
		
		//! TODO: automatically remove the .tmp files of opList on return
		
		if (addToLastFail) {
			//! TODO: cancel each in reverseOpList 
			
			return {};
		}
		
		/*
		crentryreg(numentrychn);
		killtwoschn(numentrychn, 3);
		addtolastinum(i, 0);
		*/
		
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
		
		
		//! TODO: do rename from temp
		std::list<FileRenameOp> renameOpList;
		renameOpList.emplace_back(tempIndexPath, indexFilePath, bakIndexPath);
		renameOpList.insert(renameOpList.end(), reverseOpList.begin(), reverseOpList.end());
		renameOpList.insert(renameOpList.end(), addToLastOpList.begin(), addToLastOpList.end());
		
		bool success = executeRenameOpList(renameOpList);
		if (!success) {
			errorf("executeRenameOpList failed");
			//! TODO: remove tmp files
			return {};
		}
		
		std::forward_list<KeyT> retList;
		
		for (auto entryPair : regEntryPairList) {
			retList.insert_after(retList.end(), entryPair.first);
		}
		
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
	
	MainIndexIndex(IndexSessionHandler &handler_) : TopIndex(handler_, this->indexID) {}
	
	const std::string getFileStrBase(void) {
		return "miIndex";
	}
	
	const std::filesystem::path getDirPath(void) {
		return g_fsPrgDirPath;
	}
	
	bool isValidInputEntry(std::string) {
		//! TODO:
		return false;
	}
	
	std::list<FileRenameOp> reverseAddEntries(std::forward_list<EntryPair> regEntryPairList, bool &argFail) {
		//! TODO:
		argFail = true;
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