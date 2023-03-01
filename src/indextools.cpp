//functions dealing with file entries

//to do: directory index record

#include "indextools.hpp"
#include "indextools_private.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <string>

extern "C" {
#include "stringchains.h"
#include "arrayarithmetic.h"
#include "bytearithmetic.h"
#include "portables.h"
#include "tfiles.h"
#include "ioextras.h"
#include "dupstr.h"
#include "breakpath.h"
#include "dirfiles.h"

//! TEMP:
#include "errorf.h"
}

#include "errorf.hpp"

//#define errorf(...) g_errorf(__VA_ARGS__)

#define errorf(str) g_errorfStdStr(str)
//! TEMP
#define errorf_old(...) g_errorf(__VA_ARGS__)

extern std::string g_prgDir;

enum : int32_t {
	RD_MAX_CLEN = 1000,
	RF_MAX_CLEN = 1000,
	DIRFREG_MAX_CLEN = 1000,

	kOptionMaxAlias = 100,
	kOptionAllowTaggingNothing = 0,
	kOptionSearchImplicitAnd = 1,
	kOptionSearchImplicitOr = 0,
	kOptionTolerateUnclosed = 0,

	kSrExpTypeBlank = 0,
	kSrExpTypeAnd = 1,
	kSrExpTypeOr = 2,
	kSrExpTypeTBD = 3,
	kSrExpTypeNeg = 4,
	kSrExpTypeRoot = 5,
	kSrExpTypeSubExprMarker = 6,
	kSrExpTypeNull = 7,
	kSrExpTypeTagNum = 8,

	kSearchParseTypeAnd = 1,
	kSearchParseTypeOr = 2,
	kSearchParseTypeNeg = 3,
	kSearchParseTypeSubExprStart = 4,
	kSearchParseTypeSubExprEnd = 5,
	kSearchParseTypeAlias = 6,
	kSearchParseTypeNoExpr = 7

};

//! TODO: make references to sessionhandler atomic and thread safe
TopIndexSessionHandler g_indexSessionHandler = TopIndexSessionHandler();

//{ main index

uint64_t getLastMINum(void) {
errorf("getLastMINum before openSession");
	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();
errorf("getLastMINum after openSession");

	if (indexSession == nullptr) {
		errorf("getLastMINum could not open session");
		return 0;
	}

	int32_t error = 0;
errorf("getLastMINum before getNofVirtualEntries");
	return indexSession->getNofVirtualEntries(error);
errorf("getLastMINum after getNofVirtualEntries");
}

uint64_t miReg(char *miName) {
	if (miName == nullptr) {
		errorf("miName was nullptr");
		return 0;
	}
	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();
	if (indexSession == nullptr) {
		errorf("(miReg) could not open session");
		return 0;
	}
	std::string str = std::string(miName);
	uint64_t res = indexSession->addEntry(str);
	if (!res) {
		errorf("failed to register miName");
		return 0;
	} else {
		return res;
	}

	return 0;
}

int chainMiReg(oneslnk *miNameChain) {

	errorf("cmireg unimplemented");

	return 0;
}

char *miRead_old(uint64_t miNum) {	// returns malloc memory
	return 0;
}

char miRemove(uint64_t dNum) {
	return 1;
}

oneslnk *chainMiRead(oneslnk *miNumChain) {		//! not done

	return nullptr; //!
}

int chainMiRmv(oneslnk *dNumChain) {
	return 1;
}


int chainMiReroute(twoslnk *rerouteChain) {		//! untested
	return 1;
}

/*
oneslnk *intervalMiRead(uint64_t start, uint64_t interval) {	// returns malloc memory (the whole chain)

	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();

	if (indexSession == nullptr) {
		errorf("intervalMiRead could not open session");
		return 0;
	}

	std::forward_list<std::string> retList = indexSession->readIntervalEntries(start, interval);

	oneslnk *flnk, *lastlnk;
	if ((flnk = malloc(sizeof(oneslnk))) == 0) {
		errorf("malloc failed");
		return 0;
	} lastlnk = flnk;
	flnk->str = 0;
	flnk->next = nullptr;

	int32_t i = 0;
	for (auto &entry : retList) {
		lastlnk = lastlnk->next = malloc(sizeof(oneslnk));
		if (lastlnk == nullptr) {
			errorf("malloc failed");
			killoneschn(flnk, 0);
			return 0;
		} else {
			lastlnk->next = nullptr;
			lastlnk->str = nullptr;

			lastlnk->str = dupstr(entry.c_str(), 10000, 0);

			if (lastlnk->str == nullptr) {
				errorf("dupstr fail");
				killoneschn(flnk, 0);f
				return 0;
			}
		}

		i++;
	}

	if (i > interval) {
		errorf("intervalMiRead got more than interval entries");
		killoneschn(flnk, 0);
		return 0;
	}

	lastlnk = flnk->next;
	free(flnk);
	return lastlnk;
}
*/

std::shared_ptr<std::forward_list<std::string>> intervalMiRead(uint64_t start, uint64_t interval) {

	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();

	if (indexSession == nullptr) {
		errorf("intervalMiRead could not open session");
		return {};
	}

	std::shared_ptr<std::forward_list<std::string>> retListPtr = indexSession->readIntervalEntries(start, interval);

	if (retListPtr == nullptr) {
		return {};
	}

	auto &retList = *retListPtr;

	int32_t i = 0;
	for (auto &entry : retList) {
		i++;
	}

	if (i > interval) {
		errorf("intervalMiRead got more than interval entries");
		return 0;
	}

	return retListPtr;
}

void verDI(void) { // check for order, duplicates, gaps, 0 character strings

}

bool existsMI(const uint64_t &miNum, int32_t &error) {
	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();

	if (indexSession == nullptr) {
		errorf("intervalMiRead could not open session");
		error = 1;
		return false;
	}

	return indexSession->hasUndeletedEntry(miNum);
}

bool existsMI(const uint64_t &miNum) {
	int32_t error = 0;
	return existsMI(miNum, error);
}

//}

//{ directory

uint64_t getLastDNum(uint64_t miNum) {
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(miNum);

	if (indexSession == nullptr) {
		errorf("getLastDNum could not open session");
		return 0;
	}

	int32_t error = 0;
errorf("getLastDNUm spot 5");
	return indexSession->getNofVirtualEntries(error);
}

/*
uint64_t dReg(uint64_t miNum, char *dPath) {
	if (dPath == nullptr) {
		errorf("dPath was nullptr");
		return 0;
	}
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(miNum);
	if (indexSession == nullptr) {
		errorf("(dReg) could not open session");
		return 0;
	}
	std::string str = std::string(dPath);
	uint64_t res = indexSession->addEntry(str);
	if (!res) {
		errorf("failed to register dPath");
		return 0;
	} else {
		return res;
	}
}
*/

uint64_t dReg(const uint64_t &miNum, const std::fs::path &dPath) {
	if (dPath.empty()) {
		errorf("dPath was empty");
		return 0;
	}
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(miNum);
	if (indexSession == nullptr) {
		errorf("(dReg) could not open session");
		return 0;
	}
	uint64_t res = indexSession->addEntry(dPath);
	if (!res) {
		errorf("failed to register dPath");
		return 0;
	} else {
		return res;
	}
}

std::fs::path dRead(const uint64_t &miNum, const uint64_t &dNum) {	// returns malloc memory
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(miNum);

	if (indexSession == nullptr) {
		errorf("getLastMINum could not open session");
		return {};
	}

	std::fs::path dir = indexSession->readEntry(dNum);

	return dir;
}

char dRemove(uint64_t miNum, uint64_t dNum) {
	//! TODO:
	return 1;
}

char dReroute(uint64_t miNum, uint64_t dNum, char *dPath) {		// reroute
	//! TODO:
	return 1;
}

int cDReg(uint64_t miNum, oneslnk *dPathChain) {
	errorf("cDReg unimplemented");
	return 0;
}

int cDReg(const uint64_t &miNum, const std::forward_list<std::fs::path> inputList) { //! untested
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(miNum);

	if (indexSession == nullptr) {
		errorf("getLastMINum could not open session");
		return 1;
	}

	auto retlist = indexSession->addEntries(inputList);

	if (retlist.empty()) {
		errorf("cDReg addEntries failed");
		return 1;
	} else {
		return 0;
	}
}

oneslnk *chainDRead(uint64_t miNum, oneslnk *dNumChain) {
	//! TODO:
	return nullptr;
}

int chainDRemove(uint64_t miNum, oneslnk *dNumChain) {
	//! TODO:
	return 1;
}

int chainDReroute(uint64_t miNum, twoslnk *rerouteChain) {		//! untested
	//! TODO:
	return 1;
}

/*
oneslnk *intervalDRead(uint64_t miNum, uint64_t start, uint64_t interval) {	// returns malloc memory (the whole chain)

	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(miNum);

	if (indexSession == nullptr) {
		errorf("intervalMiRead could not open session");
		return 0;
	}

	std::forward_list<std::fs::path> retList = indexSession->readIntervalEntries(start, interval);

	oneslnk *flnk, *lastlnk;
	if ((flnk = malloc(sizeof(oneslnk))) == 0) {
		errorf("malloc failed");
		return 0;
	} lastlnk = flnk;
	flnk->str = 0;
	flnk->next = nullptr;

	int32_t i = 0;
	for (auto &entry : retList) {
		lastlnk = lastlnk->next = malloc(sizeof(oneslnk));
		if (lastlnk == nullptr) {
			errorf("malloc failed");
			killoneschn(flnk, 0);
			return 0;
		} else {
			lastlnk->next = nullptr;
			lastlnk->str = nullptr;

			lastlnk->str = dupstr(entry.generic_string().c_str(), 10000, 0);

			if (lastlnk->str == nullptr) {
				errorf("dupstr fail");
				killoneschn(flnk, 0);
				return 0;
			}
		}

		i++;
	}

	if (i > interval) {
		errorf("intervalMiRead got more than interval entries");
		killoneschn(flnk, 0);
		return 0;
	}

	lastlnk = flnk->next;
	free(flnk);
	return lastlnk;

}
*/

std::shared_ptr<std::forward_list<std::fs::path>> intervalDRead(uint64_t miNum, uint64_t start, uint64_t interval) {

	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(miNum);

	if (indexSession == nullptr) {
		errorf("intervalDRead could not open session");
		return {};
	}

	std::shared_ptr<std::forward_list<std::fs::path>> retListPtr = indexSession->readIntervalEntries(start, interval);

	if (retListPtr == nullptr) {
		return {};
	}

	auto &retList = *retListPtr;

	int32_t i = 0;
	for (auto &entry : retList) {
		i++;
	}

	if (i > interval) {
		errorf("intervalDRead got more than interval entries");
		return 0;
	}

	return retListPtr;
}

void verDI(const uint64_t &miNum) { // check for order, duplicates, gaps, 0 character strings

}

//}

//{	subdir

int readSubdirEntryTo(FILE *sourceFile, struct SubDirEntry *dest) {
	return 1;
}

uint64_t getLastSubDirNum(uint64_t miNum) {
	std::shared_ptr<SubDirIndex> indexSession = g_indexSessionHandler.openSession<SubDirIndex>(miNum);

	if (indexSession == nullptr) {
		errorf("getLastSubDirNum could not open session");
		return 0;
	}

	int32_t error = 0;
	return indexSession->getNofVirtualEntries(error);
}

oneslnk *intervalSubDirRead(uint64_t miNum, uint64_t start, uint64_t interval) {	// returns malloc memory (the whole chain)
	return nullptr;
}

//}

//{ file

/*

uint64_t getlastfnum(uint64_t miNum) {
	std::shared_ptr<FileIndex> indexSession = g_indexSessionHandler.openSession<FileIndex>(miNum);

	if (indexSession == nullptr) {
		errorf("getLastMINum could not open session");
		return 0;
	}

	int32_t error = 0;
	return indexSession->getNofVirtualEntries(error);
}
*/

/*

uint64_t fileReg(uint64_t dNum, char *fname) {
	if (fname == nullptr) {
		errorf("fname was nullptr");
		return 0;
	}
	std::shared_ptr<FileIndex> indexSession = g_indexSessionHandler.openSession<FileIndex>(miNum);
	if (indexSession == nullptr) {
		errorf("(dReg) could not open session");
		return 0;
	}
	std::string str = std::string(dPath);
	uint64_t res = indexSession->addEntry(str);
	if (!res) {
		errorf("failed to register dPath");
		return 0;
	} else {
		return res;
	}
}

*/

uint64_t fileReg(const uint64_t &miNum, const std::string &fname) {
	/*
	std::shared_ptr<FileIndex> indexSession = g_indexSessionHandler.openSession<FileIndex>(miNum);
	if (indexSession == nullptr) {
		errorf("(dReg) could not open session");
		return 0;
	}
	uint64_t res = indexSession->addEntry(fname);
	if (!res) {
		errorf("failed to register dPath");
		return 0;
	} else {
		return res;
	}
	*/
	return 1;
}

char chainFileReadTag(uint64_t dNum, oneslnk *fileNums, oneslnk **retFileName, oneslnk **retTags, unsigned char presort) {	//! untested
/*
	FILE *fIndex;
	unsigned char buf[MAX_PATH*4], *p;
	int c, i;
	uint64_t tnum;
	oneslnk *tlink, *link1, *link2, *link3, *link4, *link5;

	if (retFileName)
		*retFileName = 0;
	if (retTags)
		*retTags = 0;
	if (dNum == 0) {
		errorf("dNum is 0");
		return 1;
	} if (fileNums == 0) {
		errorf("fileNums is 0");
		return 1;
	}
	if (!(retFileName || retTags)) {
		errorf("no address to return file names or tags");
		return 1;
	}

	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dNum);
	if ((fIndex = MBfopen(buf, "rb")) == nullptr) {
		errorf("fIndex file not found (fileRead)");
		return 1;
	}

	if (!presort) {
		if (!(fileNums = copyoneschn(fileNums, 1))) {
			errorf("copyoneschn failed");
			return 1;
		}
		if (sortoneschnull(fileNums, 0)) {
			errorf("sortoneschnull failed");
			return 1;
		}
	}
	if (fileNums->ull == 0) {
		errorf("passed 0 fNum to chainFileReadTag");
		fclose(fIndex), presort? 0:killoneschn(fileNums, 1);
		return 1;
	}
	link1 = fileNums;
	if (retFileName) {
		link2 = link3 = malloc(sizeof(oneslnk));
		link2->str = 0;
	} if (retTags) {
		link4 = link5 = malloc(sizeof(oneslnk));
		link4->vp = 0;
	}

	fseek64(fIndex, finitpos(dNum, fileNums->ull), SEEK_SET);	//! also add fseeking if the distance to next entry is larger than fNum/(sqrt(lastfnum)+fNum%sqrt(lastfnum)

	while (link1) {
		tnum = fgetull_pref(fIndex, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf_old("fIndex num read failed: %d", c);
			fclose(fIndex), presort? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
			return 1;
		}
		if (link1->ull > tnum) {
			if ((c = null_fgets(0, MAX_PATH*4, fIndex)) != 0) {
				errorf_old("fIndex read error: %d", c);
				fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			while ((c = getc(fIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in fIndex");
					fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				if (fseek(fIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
			}
		} else if (link1->ull == tnum) {
			if ((c = null_fgets(buf, MAX_PATH*4, fIndex)) != 0) {
				errorf_old("fIndex read error: %d", c);
				fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			if (retFileName) {
				link3 = link3->next = malloc(sizeof(oneslnk));
				if (!(link3->str = dupstr(buf, MAX_PATH*4, 0))) {
					errorf_old("failed to duplicate buf: %s", buf);
				}
			}
			if (retTags) {
				if ((link5 = link5->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed for link5");
					fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				if ((tlink = link5->vp = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed for link5->vp");
					fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				while (1) {
					tnum = fgetull_pref(fIndex, &c);
					if (c != 0) {
						if (c == 4) {
							break;
						}
						errorf_old("fIndex tag read failed: %d", c);
						tlink->next = 0, fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0, tlink->next = 0;
						return 1;
					} tlink = tlink->next = malloc(sizeof(oneslnk));
					tlink->ull = tnum;
				}
				tlink->next = 0, tlink = link5->vp, link5->vp = tlink->next, free(tlink);
			} else {
				while ((c = getc(fIndex)) != 0) {
					if (c == EOF) {
						errorf("EOF before end of tags in fIndex");
						fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
						return 1;
					}
					if (fseek(fIndex, c, SEEK_CUR)) {
						errorf("fseek failed");
						fclose(fIndex), presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
						return 1;
					}
				}
			}
			link1 = link1->next;
		} else {	// or maybe just skip
			break;
		}
	}
	fclose(fIndex);

	if (link1) {
		errorf_old("tried to read non-existent fNum: %d -- last read %d", link1->ull, tnum);
		presort ? 0:killoneschn(fileNums, 1), retFileName? (link3->next = 0, killoneschn(link2, 0)):0, retTags? (link5->next = 0, killoneschnchn(link4, 1)):0;
		return 1;
	}
	presort ? 0:killoneschn(fileNums, 1);

	if (retFileName) {
		link3->next = 0;
		*retFileName = link2->next;
		free(link2);
	} if (retTags) {
		link5->next = 0;
		*retTags = link4->next;
		free(link4);
	}

	return 0;
*/
	return 1;

}

char *fileRead(uint64_t dNum, uint64_t fNum) {	//! untested // returns malloc memory
	return nullptr;
}

oneslnk *chainFileRead(uint64_t dNum, oneslnk *fileNumChain) {
	/*
	int c;
	oneslnk *retlnk;

	retlnk = 0;
	c = chainFileReadTag(dNum, fileNumChain, &retlnk, nullptr, 0);

	if (!c) {
		return retlnk;
	} else {
		return nullptr;
	}
	*/
	return nullptr;
}

unsigned char fileRemove(uint64_t dNum, uint64_t fNum) {		//! not done

	return 1; //!
}

int cfireg(uint64_t dNum, oneslnk *fileNameChain) {
	return 1;
}

/*
oneslnk *ifiread(uint64_t miNum, uint64_t start, uint64_t interval) {	// returns malloc memory (the whole chain)

	std::shared_ptr<FileIndex> indexSession = g_indexSessionHandler.openSession<FileIndex>(miNum);

	if (indexSession == nullptr) {
		errorf("intervalMiRead could not open session");
		return 0;
	}

	std::shared_ptr<std::forward_list<std::string>> retList = indexSession->readIntervalEntries(start, interval);

	oneslnk *flnk, *lastlnk;
	if ((flnk = malloc(sizeof(oneslnk))) == 0) {
		errorf("malloc failed");
		return 0;
	} lastlnk = flnk;
	flnk->str = 0;
	flnk->next = nullptr;

	int32_t i = 0;
	for (auto &entry : retList) {
		lastlnk = lastlnk->next = malloc(sizeof(oneslnk));
		if (lastlnk == nullptr) {
			errorf("malloc failed");
			killoneschn(flnk, 0);
			return 0;
		} else {
			lastlnk->next = nullptr;
			lastlnk->str = nullptr;

			lastlnk->str = dupstr(entry.c_str(), 10000, 0);

			if (lastlnk->str == nullptr) {
				errorf("dupstr fail");
				killoneschn(flnk, 0);
				return 0;
			}
		}

		i++;
	}

	if (i > interval) {
		errorf("intervalMiRead got more than interval entries");
		killoneschn(flnk, 0);
		return 0;
	}

	lastlnk = flnk->next;
	free(flnk);
	return lastlnk;

}
*/

unsigned char addRemoveFileNumTagChain(uint64_t dNum, oneslnk *fileNums, oneslnk *addTagNums, oneslnk *remTagNums) {		//! untested
/*
	FILE *fIndex, *tfIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], done;
	int c, i, j;
	oneslnk *flink, *link1, *link2;
	uint64_t tnum;

	if (dNum == 0) {
		errorf("dNum is 0");
		return 1;
	}
	if (!fileNums || !(addTagNums || remTagNums)) {
		errorf("no fileNums or no tagNums in addfnumtagc");
		return 1;
	}

	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dNum);
	if ((fIndex = MBfopen(buf, "rb")) == nullptr) {
		errorf("fIndex file not found (fileRead)");
		return 1;
	}
	sprintf(baf, "%s\\i\\%llu\\fIndex.tmp", g_prgDir, dNum);
	if ((tfIndex = MBfopen(baf, "wb")) == nullptr) {
		errorf_old("couldn't create i\\%llu\\fIndex.tmp", dNum);
		fclose(fIndex);
		return 2;
	}

	if (!(flink = fileNums = copyoneschn(fileNums, 1))) {
		errorf("copyoneschn failed");
		fclose(fIndex), fclose(tfIndex), MBremove(baf);
		return 2;
	}
	if (sortoneschnull(flink, 0)) {
		errorf("sortoneschnull failed");
		fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1);
		return 2;
	}
	if (fileNums->ull == 0) {
		errorf("passed 0 tag num to add in addRemoveFileNumTagChain");
		fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1);
		return 2;
	}

	if (addTagNums) {
		if (!(addTagNums = copyoneschn(addTagNums, 1))) {
			errorf("copyoneschn failed");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1);
			return 2;
		}
		if (sortoneschnull(addTagNums, 0)) {
			errorf("sortoneschnull failed");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), killoneschn(addTagNums, 1);
			return 2;
		}
		if (addTagNums->ull == 0) {
			errorf("passed 0 tag num to remove in addRemoveFileNumTagChain");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), killoneschn(addTagNums, 1);
			return 2;
		}
	}

	if (remTagNums) {
		if (!(remTagNums = copyoneschn(remTagNums, 1))) {
			errorf("copyoneschn failed");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addTagNums ? killoneschn(addTagNums, 1) : 0;
			return 2;
		}
		if (sortoneschnull(remTagNums, 0)) {
			errorf("sortoneschnull failed");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addTagNums ? killoneschn(addTagNums, 1) : 0, killoneschn(remTagNums, 1);
			return 2;
		}
		if (remTagNums->ull == 0) {
			errorf("passed 0 dNum to chainDRemove");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addTagNums ? killoneschn(addTagNums, 1) : 0, killoneschn(remTagNums, 1);
			return 2;
		}
	}

	while (1) {
		if (!fileNums) {
			while ((c = getc(fIndex)) != EOF) {
				putc(c, tfIndex);
			}
			break;
		}
		tnum = fgetull_pref(fIndex, &c);
		if (c != 0) {
			if (c == 1) {
				c = EOF;
				break;
			}
			errorf_old("fIndex num read failed: %d", c);
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addTagNums ? killoneschn(addTagNums, 1) : 0, remTagNums ? killoneschn(remTagNums, 1) : 0;
			return 2;
		} putull_pref(tnum, tfIndex);

		for (i = 0; (c = getc(fIndex)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in fIndex");
				fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addTagNums ? killoneschn(addTagNums, 1) : 0, remTagNums ? killoneschn(remTagNums, 1) : 0;
				return 2;
			} else
				putc(c, tfIndex);
		}
		putc(c, tfIndex);
		if (i == MAX_PATH*4) {
			errorf("too long string in fIndex");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addTagNums ? killoneschn(addTagNums, 1) : 0, remTagNums ? killoneschn(remTagNums, 1) : 0;
			return 2;
		}
		if (tnum < fileNums->ull) {
			while (1) {
				tnum = fgetull_pref(fIndex, &c);
				if (c != 0) {
					if (c == 4) {
						break;
					}
					errorf_old("fIndex tag read failed: %d", c);
					fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addTagNums ? killoneschn(addTagNums, 1) : 0, remTagNums ? killoneschn(remTagNums, 1) : 0;
					return 1;
				}
				putull_pref(tnum, tfIndex);
			} putc('\0', tfIndex);
		} else if (tnum == fileNums->ull) {
			link1 = addTagNums, link2 = remTagNums;
			while (1) {
				tnum = fgetull_pref(fIndex, &c);
				if (c != 0) {
					if (c == 4) {
						break;
					}
					errorf_old("fIndex tag read failed: %d", c);
					fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addTagNums ? killoneschn(addTagNums, 1) : 0, remTagNums ? killoneschn(remTagNums, 1) : 0;
					return 1;
				}
				while (link1 && link1->ull <= tnum) {
					while (link2 && link2->ull < link1->ull) {
						link2 = link2->next;	//! maybe terminate instead  of skipping
					} if (!(link2 && link2->ull == link1->ull || link1->ull == tnum)) {
						putull_pref(link1->ull, tfIndex);
					}
					link1 = link1->next;
				} while (link2 && link2->ull < tnum) {	//! maybe terminate instead of skipping
					link2 = link2->next;
				}
				if (!(link2 && link2->ull == tnum) || (link1 && link1->ull == tnum)) {
					putull_pref(tnum, tfIndex);
				}
			} while (link1) {
				while (link2 && link2->ull < link1->ull) {
					link2 = link2->next;
				} if (!(link2 && link2->ull == link1->ull)) {
					putull_pref(link1->ull, tfIndex);
				}
				link1 = link1->next;
			}
			putc('\0', tfIndex);
			fileNums = fileNums->next;
		} else {
			break;
		}
	}
	fclose(fIndex); fclose(tfIndex), addTagNums ? killoneschn(addTagNums, 1) : 0, remTagNums ? killoneschn(remTagNums, 1) : 0;

	if (fileNums != 0 || c != EOF) {
		errorf("addRemoveFileNumTagChain broke early");
		MBremove(baf), killoneschn(flink, 1);
		return 1;
	}
	char bif[MAX_PATH*4];
	sprintf(bif, "%s\\i\\%llu\\fIndex.bak", g_prgDir, dNum);
	MBremove(bif);
	if (MBrename(buf, bif)) {
		errorf("rename1 failed");
		killoneschn(flink, 1);
		return 3;
	}
	if (MBrename(baf, buf)) {
		killoneschn(flink, 1);
		MBrename(bif, buf);
		errorf("rename2 failed");
		return 3;
	}

	addtolastfnum(dNum, 0, flink->ull);
	killoneschn(flink, 1);
	MBremove(bif);

	return 0;

*/
	return 1;
}

unsigned char addFileNumChainTag(uint64_t dNum, oneslnk *fileNums, uint64_t tagNum) {	//!untested
/*
	int c;
	oneslnk *link;

	if (dNum == 0) {
		errorf("dNum is 0");
		return 1;
	}

	link = malloc(sizeof(oneslnk));
	link->next = 0, link->ull = tagNum;

	c = addRemoveFileNumTagChain(dNum, fileNums, link, 0);
	free(link);
	return c;
*/
	return 1;
}
/*
uint64_t pfireg(char *fpath, unsigned char flag) {	// Registers file from path --  bit 1 of flag: register directory if not registered, 2: check whether file is already registered first, 3: return 0 if already registered
	char buf1[MAX_PATH*4], buf2[MAX_PATH*4];
	uint64_t dNum, fNum;

	breakpathdf(fpath, buf1, buf2);
	if ((dNum = rdread(buf1)) == 0) {
		if (flag & 1) {
			if ((dNum = dReg(buf1)) == 0) {
				return 0;
			}
		} else {
			return 0;
		}
	}

	if (flag & 2) {
		if ((fNum = rfiread(dNum, buf2)) != 0) {
			if (flag & 4)
				return 0;
			return fNum;
		}
	}
	return fileReg(dNum, buf2);
}
*/
/*
unsigned char dirfreg(char *path, unsigned char flag) {	// bit 1: register directory if not registered, 2: remove files that only exist in index (maybe only if they have no tags or associated things), 3: register subdirectories, 4: reinitialize rfIndex, 5: set dir checked date, 6: don't mark missing files as missing, 7: register files from directories recursively
	uint64_t dNum;
	char *s, *s2, buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], firecfin = 0, keep = 0, empty = 0, skipdirs = 1, recdirs = 0;
	FILE *tfile, *tfile2, *rfIndex;
	int c, i;
	oneslnk *flink, *link;

	skipdirs = !(flag & (1 << (3-1)));
	recdirs = !!(flag & (1 << (7-1)));

	uint64_t ulltime = time(0);

	if ((dNum = rdread(path)) == 0) {
		if (flag & 1) {
			if ((dNum = dReg(path)) == 0) {
				return 1;
			}
		} else {
			return 1;
		}
	}

	sprintf(buf, "%s\\i\\%llu\\rfIndex.bin", g_prgDir, dNum);

	if ((flag & 8) || !(rfIndex = MBfopen(buf, "rb"))) {
		if (rfinit(dNum) == 1) {
			empty = firecfin = 1;
		} else if (!(rfIndex = MBfopen(buf, "rb"))) {
			return 3;
		}
	}

	s = reservetfile();
	sprintf(buf, "%s\\temp\\%s-0", g_prgDir, s);
	listfilesfilesorted(path, buf, skipdirs, recdirs);		//! add support for ignoring directories or otherwise add it when filtering files for type

	if (!(tfile = opentfile(s, 0, "rb"))) {
		releasetfile(s, 1), (empty?0:fclose(rfIndex));
		return 2;
	}

	s2 = reservetfile();
	if (!(tfile2 = opentfile(s2, 0, "wb"))) {
		(empty?0:fclose(rfIndex)), fclose(tfile), releasetfile(s, 1), releasetfile(s2, 1);
	}
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, tfile)) != 0) {
			if (c == 1)
				break;
			errorf_old("1-read failed: %d", c);
			(empty?0:fclose(rfIndex)), fclose(tfile), releasetfile(s, 1), fclose(tfile2), releasetfile(s2, 1);
			return 4;
		}
		if (!firecfin && !keep) {
			if ((c = null_fgets(buf2, MAX_PATH*4, rfIndex)) != 0) {
				if (c == 1) {
					firecfin = 1;
				} else {
					errorf_old("2-read failed: %d", c);
					(empty?0:fclose(rfIndex)), fclose(tfile), releasetfile(s, 1), fclose(tfile2), releasetfile(s2, 1);
					return 4;
				}
			} else {
				if ((c = pref_fgets(0, 9, rfIndex)) != 0) {
					errorf_old("3-read failed: %d", c);
					(empty?0:fclose(rfIndex)), fclose(tfile), releasetfile(s, 1), fclose(tfile2), releasetfile(s2, 1);
					return 4;
				}
			}
		}
		if (firecfin || ((c = strcmp(buf, buf2)) < 0)) {
			keep = 1;
			term_fputs(buf, tfile2);
			sprintf(buf3, "%s\\%s", path, buf);
			uint64_t ull = getfilemodified(buf3);
			putull_pref(ull, tfile2);
//char *timetostr(uint64_t ulltime);
//char *buf6 = timetostr(getfilemodified(buf3));
//errorf_old("file: %s \ntime: %llu\nreal: %s", buf3, ull, buf6);
//free(buf6);

		} else {
			if (c == 1) {
				if (flag & 2) {
					fileRemove(dNum, rfiread(dNum, buf2));
				}
			}
			keep = 0;
		}
	}
	(empty?0:fclose(rfIndex)), fclose(tfile), releasetfile(s, 1);
	fclose(tfile2);

	if (sorttfile(s2, 2, 0 | (1 << 1), ullstrcmp, 2, 0) != 0) { //! shouldn't crash when this fails
		errorf("sorttfile failed");
		releasetfile(s2, 1);
		return 0;
	};

	tfile2 = opentfile(s2, 0, "rb");

	flink = link = malloc(sizeof(oneslnk));
	flink->str = 0, i = 0;

	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, tfile2)) != 0) {
			if (c == 1)
				break;
			errorf_old("4-read failed: %d", c);
			fclose(tfile2), releasetfile(s2, 1), link->next = 0, killoneschn(flink, 0);
			return 4;
		}
		fgetull_pref(tfile2, &c);

//uint64_t ull = fgetull_pref(tfile2, &c);
//errorf_old("2: file: %s \n time: %llu", buf, ull);
		if (c) {
			errorf("fgetull_pref error");
			fclose(tfile2), releasetfile(s2, 1), link->next = 0, killoneschn(flink, 0);
			return 4;
		}
		link = link->next = malloc(sizeof(oneslnk));
		link->str = malloc((c = strlen(buf)+1));
		snprintf(link->str, c+1, "%s", buf);
		i++;
		if (i >= DIRFREG_MAX_CLEN) {
			link->next = 0, link = flink->next, free(flink);
			cfireg(dNum, link);
			killoneschn(link, 0);
			link = flink = malloc(sizeof(oneslnk)), flink->str = 0;
			i = 0;
		}
	}
	fclose(tfile2), releasetfile(s2, 1), link->next = 0;

	if (i > 0) {
		link = flink->next, free(flink);
		cfireg(dNum, link);
	}
	killoneschn(link, 0);
	if (flag & 16) {
		setdlastchecked(dNum, ulltime);
	}
	return 0;
}
*/
int freeSearchExpr(SREXP *searchExpr) {
	if (searchExpr == nullptr) return 0;

	SREXP *next;

	if (searchExpr->exprType == kSrExpTypeRoot || searchExpr->exprType == kSrExpTypeSubExprMarker) {
		next = searchExpr->expr1;
		free(searchExpr);
		freeSearchExpr(next);
	} else if (searchExpr->exprType == kSrExpTypeAnd || searchExpr->exprType == kSrExpTypeOr) {
		freeSearchExpr(searchExpr->expr1);
		next = searchExpr->expr2;
		free(searchExpr);
		freeSearchExpr(next);
	} else {
		free(searchExpr);
	}

	return 0;
}

unsigned char parseSearchExpr(char **s) {
	while (**s == ' ') (*s)++;

	if (**s != '\0') {
		if (**s == '&' || **s == '+') {
			(*s)++;
			return kSearchParseTypeAnd;
		} else if (**s == '|') {
			(*s)++;
			return kSearchParseTypeOr;
		} else if (**s == '-' || **s == '!') {
			(*s)++;
			return kSearchParseTypeNeg;
		} else if (**s == '(') {
			(*s)++;
			return kSearchParseTypeSubExprStart;
		} else if (**s == ')') {
			(*s)++;
			return kSearchParseTypeSubExprEnd;
		} else {
			return kSearchParseTypeAlias;
		}
	} else {
		return 0;
	}
}


int stackSearchAlias(char **sp, twoslnk **aliasStack, SREXP *searchExpr) {
/*
	long long i, j, innerpar;
	signed char quotes, outerpar, innerquotes;
	char *s = *sp;
	i = j = innerpar = 0;
	quotes = outerpar = 0;

	while (s[i] != '\0' && !(s[i] == ' ' && quotes != 1)) {
		if (i == 0 && s[i] == '"')
			quotes = 1;
		else if (s[i] == '\\' && s[i+1] != '\0') {
			if (!outerpar)
				j++;
			i++;
		} else if (!(quotes || innerquotes)) {
			if (s[i] == '(') {
				if (outerpar == 0)
					outerpar = 1;
				else
					innerpar++;
			} else if (s[i] == ')') {
				if (innerpar > 0) {
					innerpar--;
				} else if (outerpar == 1) {
					outerpar = 2;
					i++;
					break;
				} else {
					i;
					break;
				}
			}
		} else if (s[i] == '"') {
			if (outerpar) {
				innerquotes = !innerquotes;
			} else if (quotes) {
				quotes = 2;
				i++;
				break;
			}
		}

		i++;
	}

	if (quotes == 1 || innerquotes || outerpar == 1) {
		return 1;
	}

	if (outerpar == 2) {
		return 1; //! change this when adding functions in search
	}

	char *buf = malloc(i+1-j-quotes);

	errorf_old("i: %d, j: %d", i, j);

	i = j = 0;
	outerpar = 0;

	if (quotes) i++, j++;

	while (s[i] != '\0' && !(s[i] == ' ' && !quotes)) {
		if (s[i] == '\\' && s[i+1] != '\0') {
			if (!outerpar)
				j++;
			i++;
		} else if (!(quotes || innerquotes)) {
			if (s[i] == '(') {
				if (outerpar == 0)
					outerpar = 1;
				else
					innerpar++;
			} else if (s[i] == ')') {
				if (innerpar > 0) {
					innerpar--;
				} else if (outerpar == 1) {
					outerpar = 2;
					i++;
					break;
				} else {
					i;
					break;
				}
			}
		} else if (s[i] == '"') {
			if (outerpar) {
				innerquotes = !innerquotes;
			} else if (quotes) {
				i++;
				break;
			}
		}

		buf[i-j] = s[i];
		i++;
	}

	errorf_old("i: %d, j: %d", i, j);

	buf[i-j-!!quotes] = '\0';

	j = 0;

	while (j < i) j++, (*sp)++;

errorf_old("s: %s", s);
errorf_old("buf: %s", buf);

	twoslnk *astack = malloc(sizeof(twoslnk));
	astack->next = *aliasStack;
	astack->u[0].vp = searchExpr;
	astack->u[1].str = buf;
	*aliasStack = astack;
*/

	return 0;
}

SEARCHSTRUCT *initFileTagSearch(char *searchStr, unsigned char *retArg, uint64_t dNum) { //! implement
/*

	retArg? *retArg = 0:0;
	if (!searchStr) {
		return nullptr;
	}
	long long i;
	uint64_t subExpLayers;
	unsigned char uc, do_implicit, saved_expr, neg, anyneg;
	twoslnk *numstack, *aliasStack, *nextlnk;	// one for the pointer back, one for the number
	SREXP *sexp1, *sexp2, *rootexp;
	SREXPLIST *buildstack, *nextonbstack;
	SUPEXPSTACK *superexp, *nextsuperexp;

	aliasStack = numstack = nullptr;
	buildstack = nullptr;
	sexp1 = sexp2 = nullptr;
	superexp = nextsuperexp = nullptr;
	subExpLayers = 0;
	do_implicit = 0;
	neg = 0;
	anyneg = 0;
	uc = saved_expr = kSearchParseTypeNoExpr;


	rootexp = malloc(sizeof(SREXP));
	rootexp->exprType = kSrExpTypeRoot;
	sexp1 = rootexp->expr1 = malloc(sizeof(SREXP));
	sexp1->exprType = kSrExpTypeBlank;

	while (1) {
errorf("loop");
		if (!do_implicit) { // if an operand is found without an operator, fill in implicit operator
			uc = parseSearchExpr(&searchStr);
			if (uc == 0) {
				break;
			}
		} else if (do_implicit == 1) {
			if (kOptionSearchImplicitAnd) {
				uc = kSearchParseTypeAnd;
			} else if (kOptionSearchImplicitOr) {
				uc = kSearchParseTypeOr;
			} else {
				errorf("do_implicit is 1 without kOptionSearchImplicitAnd or kOptionSearchImplicitOr");
				goto search_cleanup1;
			}
			do_implicit = 2;
		} else if (do_implicit == 2) {
			uc = saved_expr;
			do_implicit = 3;
		} else {
			errorf("do_implicit not 0, 1 or 2");
			goto search_cleanup1;
		}
		switch (uc) {
		case kSearchParseTypeAlias:
		case kSearchParseTypeNeg: // negation
		case kSearchParseTypeSubExprStart: // left parenthesis
			if (sexp1->exprType != kSrExpTypeBlank) {	// the selected search expression isn't blank
				if (kOptionSearchImplicitAnd || kOptionSearchImplicitOr) {
					do_implicit = 1;
					saved_expr = uc;
					continue;
				} else {
					errorf("syntax error: operand without operator and no implicit operator specified");
					retArg? *retArg = 1:0;
					goto search_cleanup1;	// buildstack needs to be climbed and erased, aliasStack erased and all expressions erased
				}
			} else {
				if (uc == kSearchParseTypeNeg) {
errorf("neg2");
					neg = !neg;
				} else {
					if (neg) {
						sexp1->exprType = kSrExpTypeNeg;
						sexp1 = sexp1->expr1 = malloc(sizeof(SREXP));
						sexp1->exprType = kSrExpTypeBlank;
					}
					if (uc == kSearchParseTypeAlias) {
errorf("srpar alias");
						sexp1->exprType = kSrExpTypeTBD;
						if (stackSearchAlias(&searchStr, &aliasStack, sexp1)) {
							errorf("alias syntax error");
							goto search_cleanup1;
						};
					} else if (uc == kSearchParseTypeSubExprStart) {
						if (neg) {
							nextonbstack = buildstack;
							buildstack = malloc(sizeof(SREXPLIST));
							buildstack->next = nextonbstack;
							buildstack->expr = sexp1;
						}

						if (!(buildstack == nullptr || buildstack->expr->exprType == kSrExpTypeSubExprMarker)) {
							sexp1->exprType = kSrExpTypeSubExprMarker;

							nextonbstack = buildstack;
							buildstack = malloc(sizeof(SREXPLIST));
							buildstack->next = nextonbstack;
							buildstack->expr = sexp1;

							nextsuperexp = superexp;
							superexp = malloc(sizeof(SUPEXPSTACK));
							superexp->next = nextsuperexp;
							superexp->subExpLayers = subExpLayers;

							subExpLayers = 0;

							sexp1 = sexp1->expr1 = malloc(sizeof(SREXP));
							sexp1->exprType = kSrExpTypeBlank;
						}
						subExpLayers++;
					} else {
						errorf("switch error 1");
						retArg? *retArg = 1:0;
						goto search_cleanup1;
					}
					neg = 0;
				}
			}
			break;
		case kSearchParseTypeAnd:	// and
		case kSearchParseTypeOr:	// or
			if (sexp1->exprType == kSrExpTypeBlank) {
				errorf("AND or OR operator instead of operand");
				retArg? *retArg = 1:0;
				goto search_cleanup1;
			}

			sexp2 = malloc(sizeof(SREXP));
			if (uc == kSearchParseTypeAnd) {
				sexp2->exprType = kSrExpTypeAnd;
			} else {
				sexp2->exprType = kSrExpTypeOr;
			}
			if (uc == kSearchParseTypeOr) {	// climb out of "and" chain -- "or" has lesser priority
				while (buildstack != nullptr && buildstack->expr->exprType == kSrExpTypeAnd) {
					nextonbstack = buildstack->next;
					free(buildstack);
					buildstack = nextonbstack;
				}
			}

			if (buildstack == nullptr) {
				sexp2->expr1 = rootexp->expr1;
				rootexp->expr1 = sexp2;
				nextonbstack = buildstack;
			} else if (buildstack->expr->exprType == kSrExpTypeSubExprMarker) {
				sexp2->expr1 = buildstack->expr->expr1;
				buildstack->expr->expr1 = sexp2;
				nextonbstack = buildstack;
			} else if (buildstack->expr->exprType == kSrExpTypeOr || buildstack->expr->exprType == kSrExpTypeAnd) {
				if (buildstack->expr->exprType == kSrExpTypeOr && uc == kSearchParseTypeAnd) {
					sexp2->expr1 = buildstack->expr->expr2;
					buildstack->expr->expr2 = sexp2;
					nextonbstack = buildstack;
				} else { // if better chaining is done, all changes in this 'case' will be here
					sexp2->expr1 = buildstack->expr->expr2;
					buildstack->expr->expr2 = sexp2;
					nextonbstack = buildstack->next; // in a chain of "or" or chain of "and" the chain doesn't need to be linked to
					free(buildstack);
				}
			} else {
				free(sexp2);
				errorf("unknown parent expression");
				retArg? *retArg = 1:0;
				goto search_cleanup1;
			}

			sexp2->expr2 = sexp1 = malloc(sizeof(SREXP));
			sexp1->exprType = kSrExpTypeBlank;
			buildstack = malloc(sizeof(SREXPLIST));
			buildstack->next = nextonbstack;
			buildstack->expr = sexp2;

			break;
		case kSearchParseTypeSubExprEnd: // right parenthesis
			if (sexp1->exprType == kSrExpTypeBlank) {
				errorf("right parenthesis instead of operand");
				retArg? *retArg = 1:0;
				goto search_cleanup1;
			}
			if (subExpLayers < 1) {
				errorf("right parenthesis without left one");
				retArg? *retArg = 1:0;
				goto search_cleanup1;
			}
			subExpLayers--;
			while (buildstack != nullptr && buildstack->expr->exprType != kSrExpTypeSubExprMarker) {
				nextonbstack = buildstack->next;
				free(buildstack);
				buildstack = nextonbstack;
			}
			if (subExpLayers == 0 && buildstack != nullptr) { // if buildstack is null the following operator will just continue from root
				subExpLayers = superexp->subExpLayers;
				sexp1 = buildstack->expr->expr1;
				free(buildstack->expr);

				nextonbstack = buildstack;
				buildstack = buildstack->next;
				free(nextonbstack);

				// impossible for the next expression to be SUBEXP_MARKER as well because otherwise _layers would just be incremented and other operands stack
				if (buildstack->expr->exprType == kSrExpTypeOr || buildstack->expr->exprType == kSrExpTypeAnd) {
					buildstack->expr->expr2 = sexp1;
				} else if (buildstack->expr->exprType == kSrExpTypeNeg) {
					buildstack->expr->expr1 = sexp1;
				} else { //! probably segfault or memory leak
					errorf("unknown parent expression (2)");
					free(sexp1);
					retArg? *retArg = 1:0;
					goto search_cleanup1;
				}
			}

			break;
		default:
			errorf("unknown search expression");
			retArg? *retArg = 1:0;
			goto search_cleanup1;
		}

		if (do_implicit > 2) {
			do_implicit = 0;
			saved_expr = kSearchParseTypeNoExpr;
		}
	}

errorf("initftag2");
if (aliasStack) {
	errorf_old("alias: %s", aliasStack->u[1].str);
}

	while (buildstack != nullptr) {
		nextonbstack = buildstack->next;
		free(buildstack);
		buildstack = nextonbstack;
	}

	while (superexp != nullptr) {
		nextsuperexp = superexp->next;
		free(superexp);
		superexp = nextsuperexp;
	}

	if (rootexp->expr1->exprType == kSrExpTypeBlank) {
		// no expressions at all
		while (aliasStack != nullptr) {
			nextlnk = aliasStack->next;

			if (aliasStack->u[1].str != nullptr)
				free(aliasStack->u[1].str);

			free(aliasStack);
			aliasStack = nextlnk;
		}
		freeSearchExpr(rootexp);

		return nullptr;

	} else if (sexp1->exprType == kSrExpTypeBlank) {
		errorf("unfilled blank expression");
		retArg? *retArg = 1:0;
		goto search_cleanup1;
	}

	if (!TOLERATE_UNCLOSED && subExpLayers > 0) {
		errorf("unclosed subexpression");
		retArg? *retArg = 1:0;
		goto search_cleanup1;
	}

	{
		tnumfromalias2(dNum, &aliasStack);

		twoslnk *lnk;

		lnk = aliasStack;
		i = 0;
		uint64_t prev = 0;

		while (lnk != nullptr) {
			nextlnk = lnk->next;
			sexp1 = lnk->u[0].vp;

			if (lnk->u[1].ull == 0) {
				sexp1->exprType = kSrExpTypeNull;
			} else {
				sexp1->exprType = kSrExpTypeTagNum;
				if (prev == 0) {
					prev = lnk->u[1].ull;
				} else if (prev != lnk->u[1].ull) {
					i++;
					prev = lnk->u[1].ull;
				}
				sexp1->refNum = i;
			}

			lnk = nextlnk;
		}

		unsigned long ntagnums = i+1;

		TAGNUMNODE *tagnumarray = malloc(sizeof(TAGNUMNODE)*(ntagnums));

		lnk = aliasStack;
		i = 0;
		prev = 0;

		while (lnk != nullptr) {
			nextlnk = lnk->next;

			if (lnk->u[1].ull == 0) {
			} else {
				if (prev == 0) {
					prev = tagnumarray[i].tagNum = lnk->u[1].ull;
				} else if (prev != lnk->u[1].ull) {
					i++;
					prev = tagnumarray[i].tagNum = lnk->u[1].ull;
				}
			}

			lnk = nextlnk;
		}

		killtwoschn(aliasStack, 3);

		SEARCHSTRUCT *retstruct = malloc(sizeof(SEARCHSTRUCT));

		retstruct->tagnumarray = tagnumarray;
		retstruct->ntagnums = ntagnums;
		retstruct->rootexpr = rootexp->expr1;
		retstruct->dNum = dNum;
		free(rootexp);

		return retstruct;

		// build the search expression and when encountering tag aliases, link them to

		return nullptr;

	}

	search_cleanup1: {
		retArg && *retArg == 0? *retArg = 1:0;

		while (buildstack != nullptr) {
			nextonbstack = buildstack->next;
			free(buildstack);
			buildstack = nextonbstack;
		}

		while (superexp != nullptr) {
			nextsuperexp = superexp->next;
			free(superexp);
			superexp = nextsuperexp;
		}

		while (aliasStack != nullptr) {
			nextlnk = aliasStack->next;

			if (aliasStack->u[1].str != nullptr)
				free(aliasStack->u[1].str);

			free(aliasStack);
			aliasStack = nextlnk;
		}

		//while (numstack != nullptr) {
		//	nextlnk = numstack->next;
		//	free(numstack);
		//	numstack = nextlnk;
		//}

		freeSearchExpr(rootexp);

		return nullptr;
	}
	*/
	return nullptr;
}

void killSearchExpr(struct SearchExpr *searchExpr) {
	// if tagNum just free self, otherwise free non-null child expressions, error if first child is null but second one isn't
}

int searchExprValue(SREXP *searchExpr, SEARCHSTRUCT *searchStruct) {
	uint16_t exprType = searchExpr->exprType;
	if (exprType == kSrExpTypeTagNum) {
		return searchStruct->tagNumArray[searchExpr->refNum].state;
	} else if (exprType == kSrExpTypeAnd) {
		if (searchExprValue(searchExpr->expr1, searchStruct) == 0)
			return 0;
		else
			return searchExprValue(searchExpr->expr2, searchStruct);
	} else if (exprType == kSrExpTypeOr) {
		if (searchExprValue(searchExpr->expr1, searchStruct) == 1)
			return 1;
		else
			return searchExprValue(searchExpr->expr2, searchStruct);
	} else if (exprType == kSrExpTypeNeg) {
		return !searchExprValue(searchExpr->expr1, searchStruct);
	} else if (exprType == kSrExpTypeNull) {
		return 0;
	} else {
		errorf_old("unknown srexp (%d) (searchExprValue)", exprType);
		return 0;
	}
}

unsigned char fileTagCheck(FILE *file, uint64_t fNum, SEARCHSTRUCT *searchStruct) {
	long long i;
	uint64_t tagNum;
	int c;

	if (searchStruct->nTagNums <= 0) {
		return 1;
	}

	i = 0;
	while (1) {
	tagNum = fgetull_pref(file, &c);
		if (c != 0) {
			if (c == 1 || c == 4)
				break;
			errorf_old("fIndex tag num read failed: %d", c);
			return 2;
		}
		c = 0;

		while (i < searchStruct->nTagNums && searchStruct->tagNumArray[i].tagNum <= tagNum) {
			if (searchStruct->tagNumArray[i].tagNum == tagNum) {
				searchStruct->tagNumArray[i].state = 1;
			} else {
				searchStruct->tagNumArray[i].state = 0;
			}
			i++;
		}
	}

	while (i < searchStruct->nTagNums) {
		searchStruct->tagNumArray[i].state = 0;
		i++;
	}

	c = searchExprValue(searchStruct->rootExpr, searchStruct);

	if (c == 0)
		return 1;
	else if (c == 1)
		return 0;
	else
		return 2;
}

void endSearch(SEARCHSTRUCT *searchStruct) {
	if (!searchStruct) {
//		errorf("endSearch with searchStruct null");
		return;
	}
	if (searchStruct->tagNumArray)
		free(searchStruct->tagNumArray);
	if (searchStruct->rootExpr)
		freeSearchExpr(searchStruct->rootExpr);
	free(searchStruct);
}

char *ffiReadTagExt(uint64_t dNum, char *searchStr, char *exts) {	// returns tfile
/*
	FILE *fIndex, *tfile, *tfilerec;
	unsigned char buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], *p, *p2, *p3, ascending = 0, uc;
	int c, i, flag;
	uint64_t tnum, nPut = 0, gap, nPos, fPos;
	char *tfstr;
	SEARCHSTRUCT *searchStruct;

	if (dNum == 0) {
		errorf("dNum is 0");
		return 0;
	}
	if (exts != 0) {
		p = buf, p2 = buf2, p2[0] = '\0';
		i = 0, ascending = 1;
		while (1) {
			if (exts[i++] != '.') {
				if (exts[--i] == '\0')
					break;
				errorf("exts broken format");
				return 0;
			}
			for (c = 0; (p[c] = exts[i]) != '.' && p[c] != '\0'; i++, c++);
			p[c] = '\0';
			if (casestrcmp(p, p2) < 0) {
				ascending = 0;
				break;
			}
			p3 = p, p = p2, p2 = p3;
		}
	}
	if (!ascending) {	//maybe create sorting later
		errorf("not ascending");
		return 0;
	}
	uc = 1;

	if (!(searchStruct = initFileTagSearch(searchStr, &uc, dNum)) && uc != 0) {
		errorf("initFileTagSearch failed from searchStr");
		return 0;
	}

	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dNum);
	if ((fIndex = MBfopen(buf, "rb")) == nullptr) {
		errorf("fIndex file not found (ffiReadTagExt)");
		endSearch(searchStruct);
		return 0;
	}
	if ((tfstr = reservetfile()) == 0) {
		errorf("reservetfile failed");
		fclose(fIndex), endSearch(searchStruct);
		return 0;
	} if ((tfile = opentfile(tfstr, 0, "wb")) == 0) {
		errorf("opentfile failed");
		fclose(fIndex), releasetfile(tfstr, 1);
	}

	while (1) {
		tnum = fgetull_pref(fIndex, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf_old("fIndex num read failed: %d", c);
			fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endSearch(searchStruct);
			return 0;
		}
		if ((c = null_fgets(buf, MAX_PATH*4, fIndex)) != 0) {
			errorf_old("fIndex read error: %d", c);
			fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endSearch(searchStruct);
			return 0;
		}
		c = 0;
		if (exts != 0) {
// errorf_old("buf: %s", buf);
			breakfname(buf, 0, buf2);
			for (i = 0; exts[i++] == '.';) {
				for (c = 0; exts[i] != '.' && exts[i] != '\0'; i++, c++) {
					buf3[c] = exts[i];
				} buf3[c] = '\0';
				if ((flag = casestrcmp(buf2, buf3)) == 0 || (flag < 0 && ascending)) {
					break;
				}
			} if (exts[--i] == '\0')
				flag = -1;
		} else {
			flag = 0;
		}
		if (flag || !searchStruct) {
			while ((c = getc(fIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in fIndex");
					fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endSearch(searchStruct);
					return 0;
				}
				if (fseek(fIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endSearch(searchStruct);
					return 0;
				}
			}
		} else {
			flag = fileTagCheck(fIndex, tnum, searchStruct);
		}
		if (!flag) {
			putull_pref(tnum, tfile);
			nPut++;
		}
	}

	fclose(fIndex), fclose(tfile), endSearch(searchStruct);
	if (nPut != 0) {
		if (!(tfile = opentfile(tfstr, 0, "rb"))) {
			errorf("opening tfile failed");
			releasetfile(tfstr, 1);
			return 0;
		}
		if (!(tfilerec = opentfile(tfstr, 1, "wb"))) {
			errorf("opening tfilerec failed");
			releasetfile(tfstr, 1);
		}

		putull_pref(nPut, tfilerec);
		gap = sqrt(nPut);
		nPos = 1;
		while (1) {
			fPos = ftell64(tfile);

			tnum = fgetull_pref(tfile, &c);
			if (c != 0) {
				if (c == 1)
					break;
				errorf_old("tfile num read failed: %d", c);
				fclose(tfile), fclose(tfilerec), releasetfile(tfstr, 2);
				return 0;
			}

			if (nPos % gap == 0) {
				putull_pref(nPos, tfilerec);
				putull_pref(fPos, tfilerec);
			}
			nPos++;
		}
		fclose(tfile), fclose(tfilerec);

		return tfstr;
	} else {
		errorf("nPut is 0");
		return 0;
	}
*/
	return nullptr;
}

uint64_t getframount(char *tfstr) {
	int i, c;
	uint64_t fram = 0;
	FILE *file;

	if (tfstr == 0) {
		errorf("tfstr is nullptr");
		return 0;
	}

	if (((file = opentfile(tfstr, 1, "rb")) == nullptr) || ((i = getc(file)) == EOF)) {
		if (file != nullptr)
			fclose(file);
		errorf("couldn't access fr file 1");
		return 0;
	}
	for (; i > 0; i--) {
		fram *= 256;
		fram += c = getc(file);
		if (c == EOF) {
			errorf("EOF before end of number in diRec");
			fclose(file);
			return 0;
		}
	}
	fclose(file);
	return fram;
}

uint64_t frinitpos(char *tfstr, uint64_t inpos, uint64_t *retnpos) {
/*
	char buf[MAX_PATH*4], ch;
	int i, c;
	uint64_t npos, pos = 0, lastnpos = 0;
	FILE *file;

	if (!retnpos) {
		errorf("frinitpos without retnpos");
		return 0;
	} else {
		*retnpos = 0;
	}
	if (tfstr == 0) {
		errorf("tfstr is nullptr");
		return 0;
	}

	if (((file = opentfile(tfstr, 1, "rb")) == nullptr) || ((i = getc(file)) == EOF)) {
		if (file != nullptr)
			fclose(file);
		errorf("couldn't access fr file 1");
		return 0;
	}
	fseek(file, i, SEEK_CUR);
	while ((c = getc(file)) != EOF) {
		for (ch = c, i = 1, npos = 0; i <= ch && (c = getc(file)) != EOF; i++, npos *= 256, npos += c);
		if (npos > inpos)
			break;
		for (ch = c = getc(file), i = 1, pos = 0; i <= ch && (c = getc(file)) != EOF; i++, pos *= 256, pos += c);
		if (c == EOF) {
			errorf("EOF before finished position (frinitpos)");
			fclose(file);
			return 0;
		}
		lastnpos = npos;
	}
	fclose(file);
	if (retnpos)
		*retnpos = lastnpos;
	return pos;
*/
	return 0;
}

oneslnk *ifrread(char *tfstr, uint64_t start, uint64_t interval) {
/*
	FILE *tfile;
	unsigned char buf[MAX_PATH*4];
	int c, i;
	uint64_t tnum, npos;
	oneslnk *flnk, *lastlnk;

	if (tfstr == 0) {
		errorf("tfstr is nullptr");
		return 0;
	}
	if (!interval) {
		errorf("ifrread without interval");
		return 0;
	}

	if ((tfile = opentfile(tfstr, 0, "rb")) == nullptr) {
		if (tfile != nullptr)
			fclose(tfile);
		errorf("couldn't access fr file 0");
		return 0;
	}

	fseek64(tfile, frinitpos(tfstr, start, &npos), SEEK_SET);
	if (npos == 0) {
		npos = 1;
	}
	while (1) {
		tnum = fgetull_pref(tfile, &c);
		if (c != 0) {
			if (c == 1) {
				break;
			}
			errorf_old("tfile num read failed: %d", c);
			fclose(tfile);
			return 0;
		}

		if (npos < start) {	// meant to just skip
		} else {
			if ((flnk = malloc(sizeof(oneslnk))) == 0) {
				errorf("malloc failed");
				fclose(tfile);
				return 0;
			} lastlnk = flnk;
			flnk->str = 0;

			while (interval > 0) {
				if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed");
					lastlnk->next = 0, fclose(tfile), killoneschn(flnk, 1);
					return 0;
				}
				lastlnk->ull = tnum;
				interval--;

				tnum = fgetull_pref(tfile, &c);
				if (c != 0) {
					if (c == 1)
						break;
					errorf_old("2 - tfile num read failed: %d", c);
					lastlnk->next = 0, fclose(tfile), killoneschn(flnk, 1);
					return 0;
				}
			} fclose(tfile);

			lastlnk->next = 0;
			lastlnk = flnk->next;
			free(flnk);
			return lastlnk;
		}
		npos++;
	}
	fclose(tfile);
*/
	return 0;
}
//}
//{ tag 2nd layer

char refreshTagAliasRecord(uint64_t dNum) {	//ref for refresh		//! not done
	if (dNum == 0) {
		errorf("dNum is zero in refaliasrec");
		return 1;
	}
	return 0;
}

uint64_t tagAliasInitPos(uint64_t dNum, char *aliasstr) {
	return 0;
}

char aliasEntryFileToFile(FILE *fromfile, FILE *tofile, uint64_t aliasCode) {	// doesn't write aliasCode		//! untested
	uint64_t tnum;
	int c;

	switch (aliasCode) {
	case 0: case 1:		// 0 for tag name, 1 for alternative tag name
		tnum = fgetull_pref(fromfile, &c);
		if (c) {
			errorf_old("aliasEntryFileToFile fgetull_pref: %d", c);
			return 1;
		} putull_pref(tnum, tofile);
		break;
	default:
		errorf("unknown aliasCode");
		return 1;
		break;
	}
	return 0;
}

char crtreg(uint64_t dNum, oneslnk *tagNames, oneslnk *tagNums) { //! untested
/*
	FILE *tAlias, *ttAlias;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4];
	int i, c, j;
	uint64_t tnum;
	oneslnk *link1, *link2;
	twoslnk *ftwolink, *twolink, twolink2;

	if (dNum == 0) {
		errorf("dNum is zero in reverseTagReg");
		return 1;
	}
	if (tagNames) {
		for (i = 0, link1 = tagNames; link1; i++, link1 = link1->next) {
			if (!link1->str || link1->str[0] == '\0') {
				errorf("tried to register blank alias");
				return 1;
			}
		}
	} else {
		errorf("no tagNames in crtreg");
		return 1;
	}
	if (tagNums) {
		for (j = 0, link1 = tagNums; link1; link1 = link1->next, j++) {
			if (!link1->ull) {
				errorf("tried to register alias to tagNum 0");
				return 1;
			}
		}
	} else {
		errorf("no tagNums in crtreg");
		return 1;
	}
	if (i != j) {
		errorf("amount of tagNums differs from amount of tagNames in crtreg");
	}
	twolink = ftwolink = malloc(sizeof(twoslnk));	// must be freed as if both unions were ull since the strings are borrowed
	for (link1 = tagNames, link2 = tagNums; link1 && link2; twolink = twolink->next = malloc(sizeof(twoslnk)), twolink->u[0].str = link1->str, twolink->u[1].ull = link2->ull, link1 = link1->next, link2 = link2->next);
	twolink->next = 0, twolink = ftwolink, ftwolink = ftwolink->next, free(twolink);

	sorttwoschn(&ftwolink, (int(*)(void*,void*)) strcmp, 0, 0);
	if (ftwolink->u[0].str == 0) {
		errorf("passed null string to crtreg");
	}
	for (twolink = ftwolink; twolink->next; twolink = twolink->next) {	// prevent duplicate registration of alias
		if (!strcmp(twolink->next->u[0].str, twolink->u[0].str)) {
			errorf("tried to register alias twice");
			killtwoschn(ftwolink, 3);
			return 4;
		}
	}

	sprintf(buf, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dNum);

	tAlias = MBfopen(buf, "rb");
	sprintf(baf, "%s\\ttAlias.bin", g_prgDir);
	if ((ttAlias = MBfopen(baf, "wb")) == nullptr) {
		fclose(tAlias), killtwoschn(ftwolink, 3);
		return 3;
	}
	twolink = ftwolink;
	if (tAlias) {
		while (1) {
			if ((c = null_fgets(buf, MAX_PATH*4, tAlias)) != 0) {
				if (c == 1)
					break;
				errorf_old("null_fgets: %d", c);
				fclose(tAlias), fclose(ttAlias), MBremove(baf), killtwoschn(ftwolink, 3);
				return 1;
			}

			while (twolink && (c = strcmp(twolink->u[0].str, buf)) <= 0) {
				if (c == 0) {
					errorf("tried to register existing alias");
					fclose(tAlias), fclose(ttAlias), MBremove(baf), killtwoschn(ftwolink, 3);
					return 5;
				}
				if (!(twolink->u[1].ull && twolink->u[0].str)) {
					errorf("trying to register alias with no string or tag num of 0");
					fclose(tAlias), fclose(ttAlias), MBremove(baf), killtwoschn(ftwolink, 3);
					return 1;
				}
				term_fputs(twolink->u[0].str, ttAlias);
				putull_pref(0, ttAlias);
				putull_pref(twolink->u[1].ull, ttAlias);
				twolink = twolink->next;
			}
			term_fputs(buf, ttAlias);
			tnum = fgetull_pref(tAlias, &c);
			if (c != 0) {
				errorf_old("alias code read failed: %d", c);
				fclose(tAlias), fclose(ttAlias), MBremove(baf), killtwoschn(ftwolink, 3);
				return 6;
			}
			putull_pref(tnum, ttAlias);
			if (c = aliasEntryFileToFile(tAlias, ttAlias, tnum)) {	// doesn't write the alias code itself
				errorf("aliasEntryFileToFile failed");
				fclose(tAlias), fclose(ttAlias), MBremove(baf), killtwoschn(ftwolink, 3);
				return 7;
			}
		}
		fclose(tAlias);
	}
	while (twolink) {
		if (!(twolink->u[1].ull && twolink->u[0].str)) {
			errorf("trying to register alias with no string or tag num of 0");
			fclose(tAlias), fclose(ttAlias), MBremove(baf), killtwoschn(ftwolink, 3);
			return 1;
		}
		term_fputs(twolink->u[0].str, ttAlias);
		putull_pref(0, ttAlias);
		putull_pref(twolink->u[1].ull, ttAlias);
		twolink = twolink->next;
	}
	fclose(ttAlias), killtwoschn(ftwolink, 3);

	sprintf(buf, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dNum);
	char bif[MAX_PATH*4];
	if (tAlias) {
		sprintf(bif, "%s\\i\\%llu\\tAlias.bak", g_prgDir, dNum);
		MBremove(bif);
		if (MBrename(buf, bif)) {
			errorf("rename1 failed");
			return 8;
		}
	}
	if (MBrename(baf, buf)) {
		errorf("rename2 failed");
		return 9;
	}

	refreshTagAliasRecord(dNum);
	if (tAlias) {
		MBremove(bif);
	}
*/
	return 0;
}

char reverseTagReg(uint64_t dNum, char *tagName, uint64_t tagNum) { //! untested
/*
	int i;
	oneslnk *link1, *link2;

	if (dNum == 0) {
		errorf("dNum is zero in reverseTagReg");
		return 1;
	}
	if (tagNum == 0) {
		errorf("tagNum in rdreg is 0");
		return 1;
	}

	if (tagName) {
		for (i = 0; tagName[i] != '\0' && i != kOptionMaxAlias*4; i++);
		if (i >= kOptionMaxAlias*4) {
			errorf("alias too long");
			return 1;
		}
	} if (i == 0 || tagName == 0) {
		errorf("empty string");
		return 1;
	}

	link1 = malloc(sizeof(oneslnk));
	link2 = malloc(sizeof(oneslnk));
	link1->next = link2->next = 0;
	link1->str = tagName, link2->ull = tagNum;
	crtreg(dNum, link1, link2);
	free(link1), free(link2);
*/
	return 0;
}

oneslnk *tagNumFromAlias(uint64_t dNum, oneslnk *aliasChain, oneslnk **retAlias) { //! untested	//! in the future: resolve multiple tags from alias and alias from alias -- option to read only original tag aliases
/*
	FILE *tAlias;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], done = 0;
	int i, c, d;
	uint64_t tnum;
	oneslnk *retchn, *retlnk, *aretlnk, *flink;

	if (retAlias) {
		*retAlias = 0;
	}
	if (dNum == 0) {
		errorf("dNum is zero in tagNumFromAlias");
		return 0;
	}
	if (aliasChain == 0) {
		errorf("aliasChain in tagNumFromAlias is 0");
		return 0;
	}

	sprintf(buf, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dNum);
	tAlias = MBfopen(buf, "rb");

	if (!(flink = aliasChain = copyoneschn(aliasChain, 0))) {
		errorf("copyoneschn failed");
		tAlias?fclose(tAlias):0;
		return 0;
	}
	if (sortoneschn(flink, (int(*)(void*,void*)) strcmp, 0)) {
		errorf("sortoneschn failed");
		tAlias?fclose(tAlias):0, killoneschn(flink, 0);
		return 0;
	}
	if (aliasChain->str == 0) {
		errorf("passed 0 dNum to chainFileRead");
		tAlias?fclose(tAlias):0, killoneschn(flink, 0);
		return 0;
	}

	retchn = retlnk = malloc(sizeof(oneslnk));
	retlnk->ull = 0;

	if (retAlias) {		// unmatched aliases
		*retAlias = aretlnk = malloc(sizeof(oneslnk));
		aretlnk->str = 0;
	}

	if (tAlias) {
		fseek64(tAlias, tagAliasInitPos(dNum, aliasChain->str), SEEK_SET);

		while (aliasChain) {
			if ((c = null_fgets(buf, kOptionMaxAlias*4, tAlias)) != 0) {
				if (c == 1)
					break;
				errorf_old("null_fgets: %d", c);
				fclose(tAlias);
				return 0;
			}
			tnum = fgetull_pref(tAlias, &c);
			if (c != 0) {
				errorf_old("tagNumFromAlias - tAlias num read failed: %d", c);
				fclose(tAlias);
				return 0;
			}

			while (aliasChain && (c = strcmp(aliasChain->str, buf)) < 0) {
				if (retAlias) {
					aretlnk = aretlnk->next = malloc(sizeof(oneslnk));
					aretlnk->str = dupstr(aliasChain->str, kOptionMaxAlias*4, 0);
				}
				aliasChain = aliasChain->next;
			}
			if (!aliasChain)
				break;

			if (c != 0 || !(tnum == 0 || tnum == 1)) {
				if (c = aliasEntryFileToFile(tAlias, 0, tnum)) {
					errorf("aliasEntryFileToFile failed");
					fclose(tAlias);
					return 0;
				}
			} else {
				tnum = fgetull_pref(tAlias, &c);
				if (c != 0) {
					errorf_old("tAlias num read failed: %d", c);
					fclose(tAlias);
					return 0;
				}
				while (aliasChain && !strcmp(aliasChain->str, buf)) {
					retlnk = retlnk->next = malloc(sizeof(oneslnk));
					retlnk->ull = tnum;
					aliasChain = aliasChain->next;
				}
			}
		}
		fclose(tAlias);
	}
	retlnk->next = 0, retlnk = retchn->next, free(retchn);
	if (retAlias) {
		while (aliasChain) {
			aretlnk = aretlnk->next = malloc(sizeof(oneslnk));
			aretlnk->str = malloc(strlen(aliasChain->str)+1);
			strncpy(aretlnk->str, aliasChain->str, strlen(aliasChain->str)+1);
			aliasChain = aliasChain->next;
		}
		aretlnk->next = 0, aretlnk = *retAlias, *retAlias = (*retAlias)->next, free(aretlnk);
	}
	killoneschn(flink, 0);
	return retlnk;
}

int tnumfromalias2(uint64_t dNum, twoslnk **sourcelist) {
	FILE *tAlias;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], done = 0;
	int i, c, d;
	uint64_t tnum;
	oneslnk *retchn, *retlnk, *aretlnk;

	if (dNum == 0) {
		errorf("dNum is zero in tagNumFromAlias");
		return 1;
	}
	if (sourcelist == 0) {
		errorf("sourcelist is 0 (2)");
		return 1;
	}

	sprintf(buf, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dNum);
	tAlias = MBfopen(buf, "rb");

	if (sorttwoschn(sourcelist, (int(*)(void*,void*)) strcmp, 1, 0)) {
		errorf("sorttwoschn failed");
		tAlias?fclose(tAlias):0;
		return 1;
	}
	twoslnk *aliasChain = *sourcelist;

	if (aliasChain->u[1].str == 0) {
		errorf("passed 0 dNum to chainFileRead");
		tAlias?fclose(tAlias):0;
		return 1;
	}

	if (tAlias) {
		if (aliasChain)
			fseek64(tAlias, tagAliasInitPos(dNum, aliasChain->u[1].str), SEEK_SET);

		while (aliasChain) {
			if ((c = null_fgets(buf, kOptionMaxAlias*4, tAlias)) != 0) {
				if (c == 1)
					break;
				errorf_old("null_fgets: %d", c);
				fclose(tAlias);
				return 1;
			}
			tnum = fgetull_pref(tAlias, &c);
			if (c != 0) {
				errorf_old("tagNumFromAlias - tAlias num read failed: %d", c);
				fclose(tAlias);
				return 1;
			}

			while (aliasChain && (c = strcmp(aliasChain->u[1].str, buf)) < 0) {
				free(aliasChain->u[1].str);
				aliasChain->u[1].ull = 0;
				aliasChain = aliasChain->next;
			}
			if (!aliasChain)
				break;

			if (c != 0 || !(tnum == 0 || tnum == 1)) {
				if (c = aliasEntryFileToFile(tAlias, 0, tnum)) {
					errorf("aliasEntryFileToFile failed");
					fclose(tAlias);
					return 1;
				}
			} else {
				tnum = fgetull_pref(tAlias, &c);
				if (c != 0) {
					errorf_old("tAlias num read failed: %d", c);
					fclose(tAlias);
					return 1;
				}
				while (aliasChain && !strcmp(aliasChain->u[1].str, buf)) {
					free(aliasChain->u[1].str);
					aliasChain->u[1].ull = tnum;
					aliasChain = aliasChain->next;
				}
			}
		}
		fclose(tAlias);
	}

	if (sorttwoschnull(sourcelist, 1, 0)) {
		errorf("sorttwoschnull failed");
		return 1;
	}
*/
	return 0;
}

//}
//{ tag
uint64_t tagInitPos(uint64_t dNum, uint64_t ifnum) { /*		//! not done
	char buf[MAX_PATH*4], ch;
	int i, c;
	uint64_t dNum, pos = 0;
	FILE *direc;

	sprintf(buf, "%s\\diRec.bin", g_prgDir);

	if (((direc = MBfopen(buf, "rb")) == nullptr) || ((i = getc(direc)) == EOF)) {
		if (direc != nullptr)
			fclose(direc);
		addtolastdnum(0, 0);
		if (((direc = MBfopen(buf, "rb")) == nullptr) || ((i = getc(direc)) == EOF)) {
			if (direc != nullptr)
				fclose(direc);
			errorf("couldn't access direc");
			return 0;
		}
	}
	fseek(direc, i, SEEK_CUR);
	while ((c = getc(direc)) != EOF) {
		for (ch = c, i = 1, dNum = 0; i <= ch && (c = getc(direc)) != EOF; i++, dNum *= 256, dNum += c);
		if (dNum > idnum)
			break;
		for (ch = c = getc(direc), i = 1, pos = 0; i <= ch && (c = getc(direc)) != EOF; i++, pos *= 256, pos += c);
		if (c == EOF) {
			errorf("EOF before finished diRec entry (dinitpos)");
			fclose(direc);
			return 0;
		}
	}
	fclose(direc);

	return pos;
	*/
	return 0;
}

unsigned char addtolasttnum(uint64_t dNum, long long num, uint64_t spot) { // 0 as num to just refresh from spot		//! not done -- make sure it refreshes from beginning even when num is positive

	return 0; //!
}

uint64_t getlasttnum(uint64_t dNum) {		//! not done
	return 0;
	/*
	char buf[MAX_PATH*4];
	int i, c;
	uint64_t lastdnum = 0;
	FILE *file;

	sprintf(buf, "%s\\diRec.bin", g_prgDir);

	if (((file = MBfopen(buf, "rb")) == nullptr) || ((i = getc(file)) == EOF)) {
		if (file != nullptr)
			fclose(file);
		addtolastdnum(0, 0);
		if (((file = MBfopen(buf, "rb")) == nullptr) || ((i = getc(file)) == EOF)) {
			if (file != nullptr)
				fclose(file);
			errorf("couldn't access direc");
			return 0;
		}
	}
	for (; i > 0; i--) {
		lastdnum *= 256;
		lastdnum += c = getc(file);
		if (c == EOF) {
			errorf("EOF before end of number in diRec");
			fclose(file);
			return 0;
		}
	}
	fclose(file);
	return lastdnum;
	*/
}

char tcregaddrem(uint64_t dNum, oneslnk *fileNums, oneslnk *addTagNums, oneslnk *regTagNames, oneslnk **newTagNums, oneslnk *remTagNums) {	//! untested
/*
	FILE *tIndex, *ttIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4];
	int c, i;
	uint64_t j, tnum, lasttnum = 0;
	oneslnk *link1, *link2, *faddtagnums, *fregtagnames, *fnewtagnums, *fremtagnums;

	if (dNum == 0) {
		errorf("dNum is zero in tcregaddrem");
		return 1;
	}
	if (!(addTagNums || remTagNums || regTagNames)) {
		errorf("no tags to add or remove from or to register");
		return 1;
	} if ((addTagNums || remTagNums) && !fileNums) {
		errorf("tags to add or remove from without fileNums");
		return 1;
	}
	if (newTagNums) {
		*newTagNums = 0;
	}
	if (fileNums) {
		if (!(fileNums = copyoneschn(fileNums, 1))) {
			errorf("copyoneschn failed");
			return 0;
		}
		if (sortoneschnull(fileNums, 0)) {
			errorf("sortoneschnull failed");
			killoneschn(fileNums, 1);
			return 0;
		}
		if (fileNums->ull == 0) {
			errorf("passed 0 fNum to tcregadd");
			killoneschn(fileNums, 1);
			return 0;
		}
	}
	if (addTagNums) {
		if (!(faddtagnums = addTagNums = copyoneschn(addTagNums, 1))) {
			errorf("copyoneschn failed");
			killoneschn(fileNums, 1);
			return 0;
		}
		if (sortoneschnull(faddtagnums, 0)) {
			errorf("sortoneschnull failed");
			killoneschn(faddtagnums, 1), killoneschn(fileNums, 1);
			return 0;
		}
		if (addTagNums->ull == 0) {
			errorf("passed 0 tagNum to tcregadd");
			killoneschn(faddtagnums, 1), killoneschn(fileNums, 1);
			return 0;
		}
	} else {
		faddtagnums = 0;
	}
	if (remTagNums) {
		if (!(fremtagnums = remTagNums = copyoneschn(remTagNums, 1))) {
			errorf("copyoneschn failed");
			killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0;
			return 0;
		}
		if (sortoneschnull(fremtagnums, 0)) {
			errorf("sortoneschnull failed");
			killoneschn(fremtagnums, 1), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0;
			return 0;
		}
		if (remTagNums->ull == 0) {
			errorf("passed 0 tagNum to tcregadd");
			killoneschn(fremtagnums, 1), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0;
			return 0;
		}
	} else {
		fremtagnums = 0;
	}

	sprintf(buf, "%s\\i\\%llu\\tIndex.bin", g_prgDir, dNum);
	if ((tIndex = MBfopen(buf, "rb+")) == nullptr) {
		if ((tIndex = MBfopen(buf, "wb+")) == nullptr) {
			errorf("tIndex file not created");
			killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
			return 1;
		}
	}
	sprintf(baf, "%s\\i\\%llu\\tIndex.tmp", g_prgDir, dNum);
	if ((ttIndex = MBfopen(baf, "wb+")) == nullptr) {
		errorf("tIndex file not created");
		fclose(tIndex), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
		return 1;
	}

	if (!(addTagNums || remTagNums) && (lasttnum = getlasttnum(dNum))) {
		while ((c = getc(tIndex)) != EOF) {
			putc(c, ttIndex);
		}
	} else {
		while (1) {
			tnum = fgetull_pref(tIndex, &c);
			if (c != 0) {
				if (c == 1) {
					c = EOF;
					break;
				}
				errorf_old("tIndex num read failed: %d", c);
				fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
				return 1;
			} putull_pref(tnum, ttIndex);
			lasttnum = tnum;

			for (i = 0; (c = getc(tIndex)) != '\0' && i < MAX_PATH*4; i++) {
				if (c == EOF) {
					errorf("EOF before null terminator in tIndex");
					fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
					return 1;
				} else
					putc(c, ttIndex);
			} putc(c, ttIndex);
			if (i == MAX_PATH*4) {
				errorf("too long string in tIndex");
				fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
				return 1;
			}
			if ((!addTagNums || tnum < addTagNums->ull) && (!remTagNums || tnum < remTagNums->ull)) {
				while (1) {
					tnum = fgetull_pref(tIndex, &c);
					if (c != 0) {
						if (c == 4) {
							break;
						}
						errorf_old("tIndex tag read failed: %d", c);
						fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
						return 1;
					}
					putull_pref(tnum, ttIndex);
				} putc('\0', ttIndex);
			} else {
				if (kOptionAllowTaggingNothing) { // non-existent tagNums will be skipped
					while (remTagNums && remTagNums->ull < tnum)
						remTagNums = remTagNums->next;
					while (addTagNums && addTagNums->ull < tnum)
						addTagNums = remTagNums->next;
				}
				if (remTagNums && remTagNums->ull < tnum || addTagNums && addTagNums->ull < tnum) {
					break;
				}

				if (!(remTagNums && remTagNums->ull == tnum) && addTagNums && addTagNums->ull == tnum) {
					link1 = fileNums;
					while (1) {
						tnum = fgetull_pref(tIndex, &c);
						if (c != 0) {
							if (c == 4) {
								break;
							}
							errorf_old("tIndex tag read failed: %d", c);
							fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
							return 1;
						}
						while (link1 && tnum <= link1->ull) {
							if (tnum != link1->ull)
								putull_pref(link1->ull, ttIndex);
							link1 = link1->next;
						}
						putull_pref(tnum, ttIndex);
					} while (link1) {
						putull_pref(link1->ull, ttIndex);
						link1 = link1->next;
					}
					putc('\0', ttIndex);
				} else if (!(addTagNums && addTagNums->ull == tnum) && remTagNums && remTagNums->ull == tnum) {
					link1 = fileNums;
					while (1) {
						tnum = fgetull_pref(tIndex, &c);
						if (c != 0) {
							if (c == 4) {
								break;
							}
							errorf_old("tIndex tag read failed: %d", c);
							fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
							return 1;
						}
						while (link1 && tnum < link1->ull) {
							link1 = link1->next;
						}
						if (!link1 || tnum != link1->ull) {
							putull_pref(tnum, ttIndex);
						}
					}
					putc('\0', ttIndex);
				} else {
					while (1) {
						tnum = fgetull_pref(tIndex, &c);
						if (c != 0) {
							if (c == 4) {
								break;
							}
							errorf_old("tIndex tag read failed: %d", c);
							fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fileNums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
							return 1;
						}
						putull_pref(tnum, ttIndex);
					} putc('\0', ttIndex);
				}
				while (remTagNums && remTagNums->ull == lasttnum)
					remTagNums = remTagNums->next;
				while (addTagNums && addTagNums->ull == lasttnum) {
					errorf_old("add matched: %llu", lasttnum);
					addTagNums = addTagNums->next;
				}
			}
		}
		if (!kOptionAllowTaggingNothing && (addTagNums || remTagNums)) {
			errorf("tried to add or remove fileNums to non-existent tagNum");
			if (addTagNums) errorf_old("addTagNums->ull: %llu", addTagNums->ull);
			if (remTagNums) errorf_old("remTagNums->ull: %llu", remTagNums->ull);
		}
		faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
		if (addTagNums || remTagNums) {
			fclose(ttIndex), MBremove(baf), killoneschn(fileNums, 1);
			return 1;
		}
	}
	fclose(tIndex);

	fregtagnames = regTagNames;
	link1 = fnewtagnums = malloc(sizeof(oneslnk));
	j = 0;
	while (regTagNames) {
		if (regTagNames->str) {
			for (i = 0; regTagNames->str[i] != '\0' && i != kOptionMaxAlias*4; i++);
			if (i >= kOptionMaxAlias*4) {
				errorf_old("regTagNames->str too long -- max: %d", kOptionMaxAlias*4);
				fclose(ttIndex), MBremove(baf), link1->next = 0, killoneschn(fnewtagnums, 1), killoneschn(fileNums, 1);
				return 1;
			}
		}
		link1 = link1->next = malloc(sizeof(oneslnk));
		if (!(i == 0 || regTagNames->str == 0)) {
			putull_pref(++lasttnum, ttIndex);
			term_fputs(regTagNames->str, ttIndex);
			if (fileNums) {
				link2 = fileNums;
				while (link2) {
					putull_pref(link2->ull, ttIndex);
					link2 = link2->next;
				}
			}
			putc('\0', ttIndex);		// to terminate the file segment
			link1->ull = lasttnum;
			j++;
		} else {
//			link1->ull = 0;
			errorf("tried to register blank tag string");
			fclose(ttIndex), MBremove(baf), link1->next = 0, killoneschn(fnewtagnums, 1), killoneschn(fileNums, 1);
			return 1;
		}
		regTagNames = regTagNames->next;
	}
	fclose(ttIndex), killoneschn(fileNums, 1);
	link1->next = 0, link1 = fnewtagnums, fnewtagnums = fnewtagnums->next, free(link1);

	char bif[MAX_PATH*4];
	sprintf(bif, "%s\\i\\%llu\\tIndex.bak", g_prgDir, dNum);
	MBremove(bif);
	if (MBrename(buf, bif)) {
		errorf("rename1 failed");
		errorf_old("%s -> %s", buf, bif);
		fnewtagnums? killoneschn(fnewtagnums, 1) : 0;
		return 1;
	}
	if (MBrename(baf, buf)) {
		fnewtagnums? killoneschn(fnewtagnums, 1) : 0, MBrename(bif, buf);
		errorf("rename2 failed");
		return 1;
	}

	if (fregtagnames) {
		if (c = crtreg(dNum, fregtagnames, fnewtagnums)) {
			errorf_old("crtreg failed: %d", c);
			MBremove(buf), MBrename(bif, buf);
			return 1;
		}
	}
	if (newTagNums) {
		*newTagNums = fnewtagnums;
	} else if (fnewtagnums) {
		killoneschn(fnewtagnums, 1);
	}
	addtolasttnum(dNum, j, 0);

	MBremove(bif);
*/
	return 0;
}

uint64_t tagChainReg(uint64_t dNum, char *tagName, oneslnk *fileNums) {
/*
	int c, i;
	uint64_t lasttnum = 0;
	oneslnk *link, *rettagnum;

	if (tagName) {
		for (i = 0; tagName[i] != '\0' && i != kOptionMaxAlias*4; i++);
		if (i >= kOptionMaxAlias*4) {
			errorf("tagName too long");
			return 0;
		}
	} if (i == 0 || tagName == 0) {
		errorf("empty tagName string");
		return 0;
	}
	link = malloc(sizeof(oneslnk));
	link->next = 0; link->str = tagName;
	tcregaddrem(dNum, fileNums, 0, link, &rettagnum, 0);
	free(link);
	if (rettagnum) {
		lasttnum = rettagnum->ull, killoneschn(rettagnum, 1);
	} else {
		lasttnum = 0;
	}
	return lasttnum;
*/
	return 0;
}

char chainTagRead(uint64_t dNum, oneslnk *tagNums, oneslnk **retTagName, oneslnk **retFileNums, unsigned char presort) {	//! untested
/*
	FILE *tIndex;
	unsigned char buf[MAX_PATH*4];
	int c, i;
	uint64_t tnum;
	oneslnk *link1, *link2, *link3, *link4, *link5, *tlink;

	if (retTagName)
		*retTagName = 0;
	if (retFileNums)
		*retFileNums = 0;
	if (dNum == 0) {
		errorf("dNum is 0 chainTagRead");
		return 1;
	} if (tagNums == 0) {
		errorf("tagNums is 0 chainTagRead");
		return 1;
	}
	if (!(retTagName || retFileNums)) {
		errorf("no address to return tag names or fileNums");
		return 1;
	}

	sprintf(buf, "%s\\i\\%llu\\tIndex.bin", g_prgDir, dNum);
	if ((tIndex = MBfopen(buf, "rb")) == nullptr) {
		errorf("tIndex file not found (chainTagRead)");
		return 1;
	}

	if (!presort) {
		if (!(link1 = tagNums = copyoneschn(tagNums, 1))) {
			errorf("copyoneschn failed");
			fclose(tIndex);
			return 1;
		}
		if (sortoneschnull(link1, 0)) {
			errorf("sortoneschnull failed");
			fclose(tIndex), killoneschn(link1, 1);
			return 1;
		}
	}
	if (tagNums->ull == 0) {
		errorf("passed 0 tagNum to chainTagRead");
		fclose(tIndex), presort? 0:killoneschn(tagNums, 1);
		return 1;
	}
	link1 = tagNums;
	if (retTagName) {
		link2 = link3 = malloc(sizeof(oneslnk));
		link2->str = 0;
	} if (retFileNums) {
		link4 = link5 = malloc(sizeof(oneslnk));
		link4->vp = 0;
	}

	fseek64(tIndex, tagInitPos(dNum, tagNums->ull), SEEK_SET);

	while (link1) {
		tnum = fgetull_pref(tIndex, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf_old("tIndex num read failed: %d", c);
			fclose(tIndex), presort? 0:killoneschn(tagNums, 1), retTagName? killoneschn(link2, 0):0, retFileNums? killoneschnchn(link4, 1):0;
			return 1;
		}
		if (link1->ull > tnum) {
			if ((c = null_fgets(0, kOptionMaxAlias*4, tIndex)) != 0) {
				errorf_old("tIndex read error: %d", c);
				fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			while ((c = getc(tIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in tIndex");
					fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				if (fseek(tIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
			}
		} else if (link1->ull == tnum) {
			if ((c = null_fgets(buf, kOptionMaxAlias*4, tIndex)) != 0) {
				errorf_old("tIndex read error: %d", c);
				fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			if (retTagName) {
				link3 = link3->next = malloc(sizeof(oneslnk));
				if (!(link3->str = dupstr(buf, kOptionMaxAlias*4, 0))) {
					errorf_old("failed to duplicate buf: %s", buf);
				}
			}
			if (retFileNums) {
				if ((link5 = link5->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed for link5");
					fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				if ((tlink = link5->vp = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed for link5->vp");
					fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				while (1) {
					tnum = fgetull_pref(tIndex, &c);
					if (c != 0) {
						if (c == 4) {
							break;
						}
						errorf_old("tIndex tag read failed: %d", c);
						tlink->next = 0, fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0, tlink->next = 0;
						return 1;
					} tlink = tlink->next = malloc(sizeof(oneslnk));
					tlink->ull = tnum;
				}
				tlink->next = 0, tlink = link5->vp, link5->vp = tlink->next, free(tlink);
			} else {
				while ((c = getc(tIndex)) != 0) {
					if (c == EOF) {
						errorf("EOF before end of tags in tIndex");
						fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
						return 1;
					}
					if (fseek(tIndex, c, SEEK_CUR)) {
						errorf("fseek failed");
						fclose(tIndex), presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
						return 1;
					}
				}
			}
			link1 = link1->next;
		} else {
			break;
		}
	}
	fclose(tIndex);

	if (link1) {
		errorf_old("tried to read non-existent tagNum: %d -- last read %d", link1->ull, tnum);
		presort ? 0:killoneschn(tagNums, 1), retTagName? (link3->next = 0, killoneschn(link2, 0)):0, retFileNums? (link5->next = 0, killoneschnchn(link4, 1)):0;
		return 1;
	}
	presort ? 0:killoneschn(tagNums, 1);

	if (retTagName) {
		link3->next = 0;
		*retTagName = link2->next;
		free(link2);
	} if (retFileNums) {
		link5->next = 0;
		*retFileNums = link4->next;
		free(link4);
	}
*/
	return 0;
}

char twoWayTagChainRegAddRem(uint64_t dNum, oneslnk *fileNums, oneslnk *addTagNums, oneslnk *regTagNames, oneslnk *remTagNums, unsigned char presort) { // presort for 1: fileNums, 2: addTagNums, 4: remTagNums	//! add presort functionality
/*
	oneslnk *tnums, *link1, *link2;
	char buf1[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], buf4[MAX_PATH*4];
	int c;
	FILE *file1, *file2;

	if (dNum == 0) {
		errorf("dNum is 0");
		return 1;
	}
	if (!fileNums) {
		errorf("no fileNums");
		return 1;
	}
	if (!(addTagNums || regTagNames || remTagNums)) {
		errorf("no addTagNums, remTagNums or regTagNames");
		return 1;
	}

	sprintf(buf1, "%s\\i\\%llu\\tIndex.bin", g_prgDir, dNum);
	sprintf(buf2, "%s\\i\\%llu\\tIndex.bak2", g_prgDir, dNum);


	if (file1 = MBfopen(buf1, "rb")) {
		if (file2 = MBfopen(buf2, "wb")) {
			while ((c = getc(file1)) != EOF) {
				putc(c, file2);
			}
			fclose(file2);
		} else {
			errorf_old("couldn't create %s", buf2);
		}
		fclose(file1);
	}

	sprintf(buf3, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dNum);
	sprintf(buf4, "%s\\i\\%llu\\tAlias.bak2", g_prgDir, dNum);

	if (file1 = MBfopen(buf3, "rb")) {
		if (file2 = MBfopen(buf4, "wb")) {
			while ((c = getc(file1)) != EOF) {
				putc(c, file2);
			}
			fclose(file2);
		} else {
			errorf_old("couldn't create %s", buf4);
		}
		fclose(file1);
	}

errorf("twoway 1");
	if (tcregaddrem(dNum, fileNums, addTagNums, regTagNames, &link1, remTagNums)) {
		errorf("tcregaddrem failed");
		MBremove(buf2), MBremove(buf4);
		return 1;
	}
	if (addTagNums) {
		tnums = copyoneschn(addTagNums, 1);
	} else {
		tnums = 0;
	}
errorf("twoway 2");
	if (link1) {
		if (!tnums) {
			tnums = link1;
		} else {
			link2 = tnums;
			while (link2->next) {
				link2 = link2->next;
			}
			link2->next = link1;
		}
	}
errorf("twoway 3");
	if (addRemoveFileNumTagChain(dNum, fileNums, tnums, remTagNums)) {
		errorf("addRemoveFileNumTagChain failed");
		if (tnums) {
			killoneschn(tnums, 1);
		}
		MBremove(buf1), MBrename(buf2, buf1), MBremove(buf3), MBrename(buf4, buf3);
		return 2;
	}
errorf("twoway 4");
	if (tnums) {
		killoneschn(tnums, 1);
	}
	MBremove(buf2), MBremove(buf4);
*/
	return 0;
}

//}
