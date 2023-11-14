#include "indextools_private.hpp"

#include "errorf.hpp"

#include <type_traits> //!
#include <utility> //!

#include "ioextras.hpp"

// a global indexSessionHandler is assumed to exist (from indextools.cpp)
extern std::shared_ptr<TopIndexSessionHandler> g_indexSessionHandlerPtr;

FileCloser::FileCloser(FILE *fp) : fp_{fp} {}

FileCloser::~FileCloser() {
	if (!released_) {
		fclose(fp_);
	}
}

void FileCloser::release(void) {
	released_ = true;
}


FilePathDeleter::FilePathDeleter(std::filesystem::path &fpath) :
	fpath_{fpath},
	released_{false}
{}

FilePathDeleter::~FilePathDeleter() {
		if (!released_) {
			bool success = std::fs::remove(fpath_);
			if (!success) {
				errorf(std::cerr, "FilePathDeleter remove failed");
			}
		}
	}
	
void FilePathDeleter::release(void) {
	released_ = true;
}

// FileRenameOp

FileRenameOp::FileRenameOp(std::filesystem::path from, std::filesystem::path to, std::filesystem::path bak) :
	from_{from},
	to_(to),
	bak_(bak)
{}

bool FileRenameOp::hasFirstExecuted(void) const {
	return firstExecuted_;
}

bool FileRenameOp::hasSecondExecuted(void) const {
	return secondExecuted_;
}

bool FileRenameOp::hasReversed(void) const {
	return reversed_;
}

bool FileRenameOp::firstExecute(void) {
std::cerr << "first-executing: " << to_ << std::endl;
	if (this->firstExecuted_) {
		errorf(std::cerr, "(FileRenameOp) already firstExecuted");
		return false;
	} else {
		std::error_code ec1;
		bool exists = std::fs::exists(to_);
		if (ec1) {
			errorf(std::cerr, "exists check failed in firstExecute");
		} else if (!exists) {
			this->hasOriginal_ = false;
			this->firstExecuted_ = true;
			return true;
		} else {
			std::error_code ec;
			std::filesystem::rename(to_, bak_, ec);
			if (ec) {
				std::cerr << "rename op failed: " << to_ << " to " << bak_ << std::endl;
				return false;
			} else {
errorf(std::cerr, "firstExecute success");
				this->firstExecuted_ = true;
				return true;
			}
		}
	}
	return false;
}

bool FileRenameOp::secondExecute(void) {
std::cerr << "second-executing: " << to_ << std::endl;
	if (!this->firstExecuted_) {
		errorf(std::cerr, "(FileRenameOp) haven't firstExecuted");
		return false;
	} else {
		std::error_code ec;
		std::filesystem::rename(from_, to_, ec);
		if (ec) {
			std::cerr << "rename op failed: " << from_ << " to " << to_ << std::endl;
			return false;
		} else {
errorf(std::cerr, "secondExecute success");
			this->secondExecuted_ = true;
			return true;
		}
	}
}

bool FileRenameOp::reverse(void) {
	if (!this->firstExecuted_ && !this->secondExecuted_) {
		return true;
	} else if (this->secondExecuted_) {
		std::filesystem::path afterBak(to_);
		afterBak += ".after.bak";
		
		std::error_code ec;
		std::filesystem::rename(to_, afterBak, ec);
		if (ec) {
			std::cerr << "rename op failed: " << to_ << " to " << afterBak << std::endl;
			return false;
		} else {
			std::error_code ec;
			std::filesystem::rename(bak_, to_, ec);
			if (ec) {
				std::cerr << "rename op failed: " << bak_ << " to " << to_ << std::endl;
				return false;
			} else {
				this->reversed_ = true;
				return true;
			}
		}
	}
	return false;
}

FileRenameOp::~FileRenameOp(void) {
	if (this->secondExecuted_ && hasOriginal_) {
		bool success = std::fs::remove(bak_);
		if (!success) {
			std::cerr << "FileRenameOp remove failed for: " << bak_ << std::endl;
		}
	}
}

bool executeRenameOpList(std::list<FileRenameOp> opList) {
	for (auto &op : opList) {
		bool success = op.firstExecute();
		if (!success) {
			errorf(std::cerr, "firstExecute failed in list");
			
			bool reverseRes = reverseRenameOpList(opList);
			if (!reverseRes) {
				errorf(std::cerr, "reverseRenameOpList failed");
			}
			
			return false;
		}
	}
	for (auto &op : opList) {
		bool success = op.secondExecute();
		if (!success) {
			errorf(std::cerr, "secondExecute failed in list");
			
			bool reverseRes = reverseRenameOpList(opList);
			if (!reverseRes) {
				errorf(std::cerr, "reverseRenameOpList failed");
			}
			
			return false;
		}
	}
errorf(std::cerr, "executeRenameOpList success");
	return true;
}

/// independent function

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

/// IndexID

IndexID::IndexID(std::string str) : str_{str} {} // class init

std::strong_ordering IndexID::operator<=>(const IndexID& rhs) const {
	return( (this->str_) <=> (rhs.str_) );
}

bool IndexID::operator==(const IndexID& rhs) const {
	return this->str_ == rhs.str_;
}
bool IndexID::operator<(const IndexID& rhs) const {
	return this->str_ < rhs.str_;
}
	
IndexSessionHandler &IndexSession::getHandler(void) {
	return this->handler_;
}

/// IndexSession

IndexSession::IndexSession(IndexSessionHandler &handler, const IndexID &indexID) :
	handler_{handler},
	indexID_{indexID}
{}

/// TopIndex

TopIndex::TopIndex(IndexSessionHandler &handler, const IndexID &indexID, const std::fs::path prgDir) :
	prgDir_{prgDir},
	IndexSession(handler, indexID) {}

const std::filesystem::path TopIndex::getDirPath(void) const {
	return prgDir_;
}

TopIndex::~TopIndex(void) {
	this->removeHandlerRefs();
}

/// SubIndex

SubIndex::SubIndex(std::shared_ptr<SubIndexSessionHandler> &handlerPtr, const IndexID &indexID, const std::fs::path prgDir) :
	handlerPtr_{handlerPtr},
	prgDir_{prgDir},
	// without cast, it segfaults on virtual method invocation
	// actually it was because of using the pointer from member instead of constructor argument
	IndexSession( *handlerPtr, indexID )
{}

const std::filesystem::path SubIndex::getDirPathFor(std::fs::path prgDir, uint64_t miNum) {
	return prgDir / "i" / ( std::to_string(miNum) );
}

const std::filesystem::path SubIndex::getDirPath(void) const {
	return this->getDirPathFor(prgDir_, this->getMINum());
}
	
uint64_t SubIndex::getMINum(void) const {
	return this->handlerPtr_->getMINum();
}

SubIndex::~SubIndex() {
	this->removeHandlerRefs();
}

/// IOSpec<uint64_t>

void IOSpec<uint64_t>::write(std::ostream &ios, const uint64_t &entry) {
	put_u64_stream_pref(ios, entry);
}

uint64_t IOSpec<uint64_t>::read(std::istream &ios) {
	bool b_gotNull = false;
	uint64_t uint = get_u64_stream_pref(ios, b_gotNull);
	if (b_gotNull) {
		errorf(std::cerr, "IOSpec<uint64_t>::get -- b_gotNull");
		ios.setstate(std::ios_base::failbit);
		return 0;
	} else {
		return uint;
	}
}

void IOSpec<uint64_t>::skip(std::istream &ios) {
	bool b_gotNull = false;
	uint64_t uint = get_u64_stream_pref(ios, b_gotNull);
	if (b_gotNull) {
		errorf(std::cerr, "IOSpec<uint64_t>::skip -- b_gotNull");
		ios.setstate(std::ios_base::failbit);
	}
}

/// IOSpec<std::string>

void IOSpec<std::string>::write(std::ostream &ios, const std::string &entry) {
	ios << entry << '\0';
}

std::string IOSpec<std::string>::read(std::istream &ios) {
	std::string str;
	
	//! TODO: limit reading to MAX_PATH or something
	std::getline(ios, str, '\0');
	
	if (ios.eof()) {
		if (str != "") {
errorf(std::cerr, "string set failbit");
			ios.setstate(std::ios_base::failbit);
		} else  {
errorf(std::cerr, "string cleared failbit");
		// don't set failbit at EOF and read no characters
		ios.clear(std::ios_base::eofbit);
		}
	}
errorf(std::cerr, "read string: " + str);
	return str;
}

void IOSpec<std::string>::skip(std::istream &ios) {
	std::string str;
	
	//! TODO: limit reading to MAX_PATH or something
	std::getline(ios, str, '\0');
	
	if (ios.eof()) {
		if (str != "") {
errorf(std::cerr, "string set failbit");
			ios.setstate(std::ios_base::failbit);
		} else  {
errorf(std::cerr, "string cleared failbit");
		// don't set failbit at EOF and read no characters
		ios.clear(std::ios_base::eofbit);
		}
	}
errorf(std::cerr, "skipped string: " + str);
}

/// IOSpec<std::fs::path>

void IOSpec<std::fs::path>::write(std::ostream &ios, const std::fs::path &entry) {
	std::string pathString = entry.generic_string();
	ios << pathString << '\0';
}

std::fs::path IOSpec<std::fs::path>::read(std::istream &ios) {
	std::string str;
	
	//! TODO: limit reading to MAX_PATH or something
	std::getline(ios, str, '\0');
	
	if (ios.eof()) {
		if (str != "") {
errorf(std::cerr, "string set failbit");
			ios.setstate(std::ios_base::failbit);
		} else  {
errorf(std::cerr, "string cleared failbit");
		// don't set failbit at EOF and read no characters
		ios.clear(std::ios_base::eofbit);
		}
	}
errorf(std::cerr, "read string: " + str);
	return std::fs::path(str);
}

void IOSpec<std::fs::path>::skip(std::istream &ios) {
	std::string str;
	
	//! TODO: limit reading to MAX_PATH or something
	std::getline(ios, str, '\0');
	
	if (ios.eof()) {
		if (str != "") {
errorf(std::cerr, "string set failbit");
			ios.setstate(std::ios_base::failbit);
		} else  {
errorf(std::cerr, "string cleared failbit");
		// don't set failbit at EOF and read no characters
		ios.clear(std::ios_base::eofbit);
		}
	}
errorf(std::cerr, "skipped string: " + str);
}

/// FixedIOSpec<uint64_t>

void FixedIOSpec<uint64_t>::write(std::ostream &ios, const uint64_t &entry) {
	put_u64_stream_fixed(ios, entry);
}

uint64_t FixedIOSpec<uint64_t>::read(std::istream &ios) {
	uint64_t uint = get_u64_stream_fixed(ios);
	
	return uint;
}

void FixedIOSpec<uint64_t>::skip(std::istream &ios) {
	uint64_t uint = get_u64_stream_fixed(ios);
}

/// StandardIndex<KeyT, EntryT>

// only template methods

/// StandardIndex<KeyT, EntryT>

// only template methods

/// MainIndexIndex

MainIndexIndex::MainIndexIndex(IndexSessionHandler &handler) :
	TopIndex(handler, this->indexID, prgDir_),
	StandardAutoKeyIndex<uint64_t, std::string>()
{}

bool TopIndexSessionHandler::removeRefs(const IndexID &indexID, const IndexSession *sessionPtr) {
	auto findIt = openSessions_.find(indexID);
	if (findIt == openSessions_.end()) {
		std::cerr << "(TopIndexSessionHandler::removeRefs) couldn't find indexID entry (indexID: " << indexID.str_ << ")" << std::endl;
	} else {
		if ((IndexSession *) findIt->second.lock().get() == sessionPtr || (IndexSession *) findIt->second.lock().get() == nullptr) {
			openSessions_.erase(findIt);
			return true;
		}
	}
	return false;
}

const std::string MainIndexIndex::getFileStrBase(void) const {
	return "miIndex";
}

const std::filesystem::path MainIndexIndex::getDirPath(void) const {
	return this->TopIndex::getDirPath();
}

bool MainIndexIndex::isValidInputEntry(const std::string &entryString) const {
	if (entryString == "") {
		return false;
	} else if (entryString.size() >= this->k_MaxEntryLen) {
		return false;
	}
	
	return true;
}

std::pair<bool, std::list<FileRenameOp>> MainIndexIndex::reverseAddEntries(std::forward_list<std::pair<uint64_t, std::string>> regEntryPairList) {
	//! TODO:
	return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
	
	return {};
}

const IndexID MainIndexIndex::indexID = IndexID("1"); // static init

void HandlerAccessor::removeHandlerRefs(IndexSessionHandler &handler, const IndexID &indexID, const IndexSession *sessionPtr) {
	handler.removeRefs(indexID, sessionPtr);
}

void IndexSession::removeHandlerRefs(void) {
	HandlerAccessor::removeHandlerRefs(this->handler_, this->indexID_, this);
}

/// TopIndexSessionHandler

TopIndexSessionHandler::TopIndexSessionHandler(const std::fs::path &prgDir) :
	openSessions_{},
	subHandlers_{},
	prgDir_{prgDir},
	IndexSessionHandler()
{}

bool TopIndexSessionHandler::removeSubHandlerRefs(const uint64_t &miNum, const SubIndexSessionHandler *handlerPtr) {
	auto findIt = subHandlers_.find(miNum);
	if (findIt == subHandlers_.end()) {
		errorf(std::cerr, "(removeSubHandlerRefs) couldn't find miNum entry");
	} else {
		if ((SubIndexSessionHandler *) findIt->second.lock().get() == handlerPtr || (SubIndexSessionHandler *) findIt->second.lock().get() == nullptr) {
			
			subHandlers_.erase(findIt);
			return true;
		}
	}
	return false;
}

/// SubIndexSessionHandler
	
SubIndexSessionHandler::SubIndexSessionHandler(TopIndexSessionHandler &parent, const uint64_t &miNum) :
	parent_{parent},
	miNum_{miNum},
	openSessions_{},
	IndexSessionHandler()
{
	if (miNum_ == 0) {
		errorf(std::cerr, "SubIndexSessionHandler miNum was 0");
	} else if (!existsMainIndex(miNum)) {
		std::cerr << "existsmainindex returned false for: " << miNum << std::endl;
	} else {
		std::fs::path miPath = SubIndex::getDirPathFor(parent_.prgDir_, miNum);
		std::error_code error;
		bool success = std::fs::create_directory(miPath, error);
		if (error) {
			std::cerr << "(SubIndexSessionHandler) failed to create directory: " << miPath << std::endl;
		}
	}
}

uint64_t SubIndexSessionHandler::getMINum(void) {
	return this->miNum_;
}

SubIndexSessionHandler::~SubIndexSessionHandler(void) {
	this->parent_.removeSubHandlerRefs(this->getMINum(), this);
}

bool SubIndexSessionHandler::removeRefs(const IndexID &indexID, const IndexSession *sessionPtr) {
/*
	auto findIt = openSessions_.find(indexID);
errorf(std::cerr, "Sub::removeRefs spot 2");
	if (findIt == openSessions_.end()) {
errorf(std::cerr, "Sub::removeRefs spot 3");
		std::cerr << "(SubIndexSessionHandler::removeRefs) couldn't find indexID entry (indexID: " << indexID.str_ << ")" << std::endl; 
		//errorf(std::cerr, "(removeRefs) couldn't find indexID entry");
	} else {
errorf(std::cerr, "Sub::removeRefs spot 4");
		if ((IndexSession *) findIt->second.lock().get() == sessionPtr || (IndexSession *) findIt->second.lock().get() == nullptr) {
			
			openSessions_.erase(findIt);
			
			return true;
		}
	}
errorf(std::cerr, "Sub::removeRefs spot 5");
*/
	return false;
}

/// TopIndexSessionHandler

/// independent function

// TODO: remove g_indexSessionHandlerPtr
bool existsMainIndex(const uint64_t &miNum) {
	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandlerPtr->openSession<MainIndexIndex>();
	
	if (indexSession == nullptr) {
		errorf(std::cerr, "existsMainIndex could not open session");
		return false;
	}
	
	return indexSession->hasUndeletedEntry(miNum);
}

/// MainIndexReverse
	
MainIndexReverse::MainIndexReverse(IndexSessionHandler &handler_) :
	TopIndex(handler_, this->indexID, prgDir_),
	StandardManualKeyIndex<std::string, std::forward_list<uint64_t>>()
{}

const std::string MainIndexReverse::getFileStrBase(void) const {
	return "miReverseIndex";
}

const std::filesystem::path MainIndexReverse::getDirPath(void) const {
	return this->TopIndex::getDirPath();
}

bool MainIndexReverse::isValidInputEntry(const std::forward_list<uint64_t> &list) const {
	auto iterator1 = list.begin();
	if (iterator1 == list.end()) {
		return false;
	}
	
	return true;
}

std::pair<bool, std::list<FileRenameOp>> MainIndexReverse::reverseAddEntries(std::forward_list<std::pair<std::string, std::forward_list<uint64_t>>> regEntryPairList) {
	return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
}

const IndexID MainIndexReverse::indexID = IndexID("MainIndexReverse"); // static assignment

/// MainIndexExtras

const IndexID MainIndexExtras::indexID = IndexID("MainIndexExtras");  // static assignment

/// DirIndex

DirIndex::DirIndex(std::shared_ptr<SubIndexSessionHandler> &handler_) :
	SubIndex(handler_, this->indexID, prgDir_),
	StandardAutoKeyIndex<uint64_t, std::fs::path>()
{}

const std::string DirIndex::getFileStrBase(void) const {
	return "dIndex";
}

const std::filesystem::path DirIndex::getDirPath(void) const {
	return this->SubIndex::getDirPath();
}

bool DirIndex::isValidInputEntry(const std::fs::path &entryPath) const {
	if (entryPath.empty()) {
		return false;
	} else if (entryPath.generic_u8string().size() >= this->k_MaxEntryLen) {
		return false;
	} else if (entryPath.is_relative() && (entryPath.begin() == entryPath.end() || *entryPath.begin() != "." )) {
		return false;
	}
	
	return true;
}

std::pair<bool, std::list<FileRenameOp>> DirIndex::reverseAddEntries(std::forward_list<std::pair<uint64_t, std::fs::path>> regEntryPairList) {
	//! TODO:
	return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
	
	return {};
}

const IndexID DirIndex::indexID = IndexID("DirIndex"); // static assignment

/// SubDirEntry

SubDirEntry::SubDirEntry(uint64_t &parentInum, std::fs::path &subPath) :
	parentInum_{parentInum},
	subPath_{subPath}
{}

bool SubDirEntry::isValid(void) const {
	return false;
}

/// IOSpec<SubDirEntry>

void IOSpec<SubDirEntry>::write(std::ostream &ios, const SubDirEntry &entry) {
	ios.setstate(std::ios_base::badbit | std::ios_base::failbit);
}

std::shared_ptr<SubDirEntry> IOSpec<SubDirEntry>::read(std::istream &ios) {
	ios.setstate(std::ios_base::failbit);

	// TODO: figure out something to return if read fails
	return {};
}

void IOSpec<SubDirEntry>::skip(std::istream &ios) {
	ios.setstate(std::ios_base::failbit);
}

/// SubDirIndex

SubDirIndex::SubDirIndex(std::shared_ptr<SubIndexSessionHandler> &handler_) :
	SubIndex(handler_, this->indexID, prgDir_),
	StandardAutoKeyIndex<uint64_t, SubDirEntry>()
{}

const std::string SubDirIndex::getFileStrBase(void) const {
	return "sdIndex";
}

const std::filesystem::path SubDirIndex::getDirPath(void) const {
	return this->SubIndex::getDirPath();
}

bool SubDirIndex::isValidInputEntry(const SubDirEntry &entry) const {
	return entry.isValid();
}

std::pair<bool, std::list<FileRenameOp>> SubDirIndex::reverseAddEntries(std::forward_list<std::pair<uint64_t, SubDirEntry>> regEntryPairList) {
	//! TODO:
	return std::pair<bool, std::list<FileRenameOp>>(true, std::list<FileRenameOp>());
	
	return {};
}

const IndexID SubDirIndex::indexID = IndexID("SubDirIndex"); // static assignment

/// FileIndexEntry
	
FileIndexEntry::FileIndexEntry(std::fs::path &filePath, std::list<uint64_t> tagList) :
	filePath_{filePath},
	tagList_{tagList}
{}

/// FileIndex

const IndexID FileIndex::indexID = IndexID("FileIndex");


/// TagIndexEntry

/// TagIndex

const IndexID TagIndex::indexID = IndexID("TagIndex");

/// TagAliasEntry

/// TagAliasIndex

const IndexID TagAliasIndex::indexID = IndexID("TagAliasIndex");

/// template instantiations (maybe not needed after all)

// template std::shared_ptr<MainIndexIndex> TopIndexSessionHandler::openSession(void);
//template std::shared_ptr<DirIndex> TopIndexSessionHandler::openSession(uint64_t &);


//template class StandardAutoKeyIndex<uint64_t, std::string>;

