#pragma once

extern "C" {
#include "stringchains.h"
}

#include <stdio.h>
#include <stdint.h>

#include <map>
#include <string>
//#include <memory>

#include "errorf.hpp"

#define errorf(str) g_errorfStdStr(str)

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
};

class IndexSession;
class IndexSessionHandler;

template <class T>
class IndexSessionPointer;

template <class T>
class HandlerIndexSessionPointer;

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

//! TODO: make reference to parent atomic

class IndexSession {
public:
	// indexID is assumed to be stored statically
	const IndexID &indexID;

	IndexSession() = delete;
	IndexSession(IndexSessionHandler &handler_, IndexID &indexID_) : handler{handler_}, indexID{indexID_}, nrefs{0}, nHandlerRefs{0} {
		
	}
	IndexSessionHandler &getHandler(void) {
		return this->handler;
	}
protected:
	IndexSessionHandler &handler;
	
private:
	int32_t nrefs;
	int32_t nHandlerRefs;
	
	template<typename T> friend class IndexSessionPointer;
	template<typename T> friend class HandlerIndexSessionPointer;
	
	void addRef(void) {
		nrefs++;
	}
	// forward declared
	void removeRef(void);
	
	void addHandlerRef(void) {
		nHandlerRefs++;
	}
	void removeHandlerRef(void) {
		nHandlerRefs--;
	}
};

template <class T>
class IndexSessionPointer {
public:
	IndexSessionPointer() = delete;
	IndexSessionPointer(const IndexSessionPointer &) = delete;
	IndexSessionPointer &operator=(const IndexSessionPointer &) = delete;
	IndexSessionPointer &operator=(IndexSessionPointer &&) = delete;
	IndexSessionPointer(const volatile IndexSessionPointer &&) = delete;
	
	IndexSessionPointer(IndexSession *init_ptr) : ptr{init_ptr}  {
		//! TODO: prevent null in the first place
		if (ptr != NULL) {
			ptr->addRef();
		}
	}
	
	~IndexSessionPointer() {
		if (ptr != NULL) {
			ptr->removeRef();
		}
	}
	
	T *get() {
		return this->ptr;
	}
	
	template <class U> 
	operator HandlerIndexSessionPointer<U>() {
		return HandlerIndexSessionPointer<U>(*this);
	}
	
	template <class ... Ts, class U>
	static IndexSessionPointer<U> make(Ts&& ... args) {
		U *created_ptr = new U(args...);
		IndexSessionPointer<U> created_isp = IndexSessionPointer<U>(created_ptr);
		return created_isp;
	}
	
	///
	
	template <class U> 
	bool operator==(const IndexSessionPointer<U>& rhs) const {
		this->ptr == rhs.ptr;
	}
	
	template <class U> 
	bool operator!=(const IndexSessionPointer<U>& rhs) const {
		this->ptr != rhs.ptr;
	}
	
	template <class U> 
	bool operator==(const U* rhs) const {
		this->ptr == rhs;
	}
	
	template <class U> 
	bool operator!=(const U* rhs) const {
		this->ptr != rhs;
	}
	
protected:
	T *ptr;
};

template <class T>
struct std::hash<IndexSessionPointer<T>> {
	std::size_t operator()(IndexSessionPointer<T> const& s) const noexcept {
		return std::hash<T*>()(s.get());
    }
};

template <class T>
class HandlerIndexSessionPointer : public IndexSessionPointer<T> {
public:
	HandlerIndexSessionPointer() = delete;
	HandlerIndexSessionPointer(const HandlerIndexSessionPointer &) = delete;
	HandlerIndexSessionPointer &operator=(const HandlerIndexSessionPointer &) = delete;
	HandlerIndexSessionPointer &operator=(HandlerIndexSessionPointer &&) = delete;
	HandlerIndexSessionPointer(const volatile HandlerIndexSessionPointer &&) = delete;
	
	HandlerIndexSessionPointer(IndexSession *init_ptr) : IndexSessionPointer<T>(init_ptr) {
		init_ptr->addHandlerRef();
	};
	
	~HandlerIndexSessionPointer() {
		if (this->ptr != NULL) {
			this->ptr->removeHandlerRef();
		}
	}
	
	HandlerIndexSessionPointer(IndexSessionPointer<T> isp) : HandlerIndexSessionPointer(isp.get()) {};
	
	///
	
	template <class U> 
	bool operator==(const HandlerIndexSessionPointer<U>& rhs) const {
		this->ptr == rhs.ptr;
	}
	
	template <class U> 
	bool operator!=(const HandlerIndexSessionPointer<U>& rhs) const {
		this->ptr != rhs.ptr;
	}
	
	template <class U> 
	bool operator==(const U* rhs) const {
		this->ptr == rhs;
	}
	
	template <class U> 
	bool operator!=(const U* rhs) const {
		this->ptr != rhs;
	}
	
};

template <class T>
struct std::hash<HandlerIndexSessionPointer<T>> {
	std::size_t operator()(HandlerIndexSessionPointer<T> const& s) const noexcept {
		return std::hash<T*>()(s.get());
    }
};

// every session knows an instance of IndexSessionHandler
// IndexSessionHandler contains a list of every session or sub-sessionhandler containing the session
// when there are no more references to a session, the session is removed from the list

class IndexSessionHandler {
public:
	template <class ... Ts, class U>
	IndexSessionPointer<U> openSession(Ts&& ... args) {
		auto findIt = openSessions.find(U::indexID);
		if (findIt != openSessions.end()) {
			return findIt->second;
		} else {
			IndexSessionPointer<U> session_ptr = IndexSessionPointer<U>(*this, args...);
			HandlerIndexSessionPointer<U> handler_session_ptr = session_ptr;
			auto resPair = openSessions.emplace(U::indexID, handler_session_ptr);
			if (resPair.first == openSessions.end()) {
				errorf("failed to emplace handler_session_ptr");
			} else if (resPair.second == false) {
				errorf("indexID already registered");
			} else {
				return session_ptr;
			}
		}
		return IndexSessionPointer<U>();
	}
	
protected:
	std::map<IndexID, HandlerIndexSessionPointer<IndexSession>> openSessions{};
	
	// can't declare a private method as friend so has to be whole class
	friend class IndexSession;
	
	bool removeRefs(const IndexID &indexID, const IndexSession *session_ptr) {
		auto findIt = openSessions.find(indexID);
		if (findIt == openSessions.end()) {
			errorf("(removeRefs) couldn't find indexID entry");
		} else {
			if (findIt->second.get() == session_ptr) {
				openSessions.erase(findIt);
				return true;
			}
		}
		return false;
	}
	
};

void IndexSession::removeRef(void) {
	nrefs--;
	if (nrefs <= nHandlerRefs) {
		if (nrefs < nHandlerRefs) {
			errorf("fewer nrefs than nHandlerRefs");
		}
		handler.removeRefs(this->indexID, this);
		delete this;
	}
}

class MainIndexIndex {
public:
	addEntry(std::string entryName);
	removeEntry(uint64_t inum);
	replaceEntry(uint64_t inum, std::string newName);
	
};

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