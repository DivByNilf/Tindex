//functions dealing with file entries

//to do: directory index record

#include "indextools.hpp"
#include "indextools_static.hpp"

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

extern char *g_prgDir;

#define RD_MAX_CLEN 1000
#define RF_MAX_CLEN 1000
#define DIRFREG_MAX_CLEN 1000
#define MAX_ALIAS 100
#define ALLOW_TAGGING_NOTHING 0
#define SRCH_IMPLICIT_AND 1
#define SRCH_IMPLICIT_OR 0
#define TOLERATE_UNCLOSED 0

#define SREXP_TYPE_BLANK 0
#define SREXP_TYPE_AND 1
#define SREXP_TYPE_OR 2
#define SREXP_TYPE_TBD 3
#define SREXP_TYPE_NEG 4
#define SREXP_TYPE_ROOT 5
#define SREXP_TYPE_SUBEXP_MARKER 6
#define SREXP_TYPE_NULL 7
#define SREXP_TYPE_TAGNUM 8

#define SRPAR_TYPE_AND 1
#define SRPAR_TYPE_OR 2
#define SRPAR_TYPE_NEG 3
#define SRPAR_TYPE_SUBEXPR_START 4
#define SRPAR_TYPE_SUBEXPR_END 5
#define SRPAR_TYPE_ALIAS 6
#define SRPAR_NO_EXPR 7

//! TODO: make references to sessionhandler atomic and thread safe
TopIndexSessionHandler g_indexSessionHandler = TopIndexSessionHandler();

//{ main index

uint64_t getlastminum(void) {
	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();
	
	if (indexSession == nullptr) {
		errorf("getlastminum could not open session");
		return 0;
	}
	
	int32_t error = 0;
	return indexSession->getNofVirtualEntries(error);
}

uint64_t mireg(char *miname) {
	if (miname == NULL) {
		errorf("miname was NULL");
		return 0;
	}
	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();
	if (indexSession == nullptr) {
		errorf("(mireg) could not open session");
		return 0;
	}
	std::string str = std::string(miname);
	uint64_t res = indexSession->addEntry(str);
	if (!res) {
		errorf("failed to register miname");
		return 0;
	} else {
		return res;
	}

	return 0;
}

int cmireg(oneslnk *minamechn) {
	
	errorf("cmireg unimplemented");
	
	return 0;
}

char *miread_old(uint64_t minum) {	// returns malloc memory
	return 0;
}

char mirmv(uint64_t dnum) {
	
}

oneslnk *cmiread(oneslnk *minumchn) {		//! not done
	
	return NULL; //!
}

int cmirmv(oneslnk *dnumchn) {
	
}


int cmirer(twoslnk *rerchn) {		//! untested

}

oneslnk *imiread(uint64_t start, uint64_t intrvl) {	// returns malloc memory (the whole chain)

	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();
	
	if (indexSession == nullptr) {
		errorf("imiread could not open session");
		return 0;
	}
	
	std::forward_list<std::string> retList = indexSession->readIntervalEntries(start, intrvl);
	
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
	
	if (i > intrvl) {
		errorf("imiread got more than intrvl entries");
		killoneschn(flnk, 0);
		return 0;
	}
	
	lastlnk = flnk->next;
	free(flnk);
	return lastlnk;
}

std::shared_ptr<std::forward_list<std::string>> imiread(uint64_t start, uint64_t intrvl) {

	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>(minum);
	
	if (indexSession == nullptr) {
		errorf("imiread could not open session");
		return {};
	}
	
	std::shared_ptr<std::forward_list<std::string>> retListPtr = indexSession->readIntervalEntries(start, intrvl);
	
	if (retListPtr == nullptr) {
		return {};
	}
	
	auto &retList = *retListPtr;
	
	int32_t i = 0;
	for (auto &entry : retList) {
		i++;
	}
	
	if (i > intrvl) {
		errorf("imiread got more than intrvl entries");
		return 0;
	}
	
	return retListPtr;
}

void verDI(void) { // check for order, duplicates, gaps, 0 character strings
	
}
 
bool existsmi(const uint64_t &minum, int32_t &error) {
	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();
	
	if (indexSession == nullptr) {
		errorf("imiread could not open session");
		error = 1;
		return false;
	}
	
	return indexSession->hasUndeletedEntry(minum);
}
 
bool existsmi(const uint64_t &minum) {
	int32_t error = 0;
	return existsmi(minum, error);
}
 
//}

//{ directory

uint64_t getlastdnum(uint64_t minum) {
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(minum);
	
	if (indexSession == nullptr) {
		errorf("getlastminum could not open session");
		return 0;
	}
	
	int32_t error = 0;
	return indexSession->getNofVirtualEntries(error);
}

/*
uint64_t dreg(uint64_t minum, char *dpath) {
	if (dpath == NULL) {
		errorf("dpath was NULL");
		return 0;
	}
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(minum);
	if (indexSession == nullptr) {
		errorf("(dreg) could not open session");
		return 0;
	}
	std::string str = std::string(dpath);
	uint64_t res = indexSession->addEntry(str);
	if (!res) {
		errorf("failed to register dpath");
		return 0;
	} else {
		return res;
	}
}
*/

uint64_t dreg(const uint64_t &minum, const std::fs::path &dpath) {
	if (dpath.empty()) {
		errorf("dpath was empty");
		return 0;
	}
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(minum);
	if (indexSession == nullptr) {
		errorf("(dreg) could not open session");
		return 0;
	}
	uint64_t res = indexSession->addEntry(dpath);
	if (!res) {
		errorf("failed to register dpath");
		return 0;
	} else {
		return res;
	}
}

std::fs::path dread(const uint64_t &minum, const uint64_t &dnum) {	// returns malloc memory
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(minum);
	
	if (indexSession == nullptr) {
		errorf("getlastminum could not open session");
		return {};
	}
	
	std::fs::path dir = indexSession->readEntry(dnum);
	
	return dir;
}

char drmv(uint64_t minum, uint64_t dnum) {
	//! TODO:
}

char drer(uint64_t minum, uint64_t dnum, char *dpath) {		// reroute
	//! TODO:
}

int cdreg(uint64_t minum, oneslnk *dpathchn) {
	errorf("cdreg unimplemented");
	return 0;
}

int cdreg(const uint64_t &minum, const std::forward_list<std::fs::path> inputList) { //! untested
	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(minum);
	
	if (indexSession == nullptr) {
		errorf("getlastminum could not open session");
		return 1;
	}
	
	auto retlist = indexSession->addEntries(inputList);
	
	if (retlist.empty()) {
		errorf("cdreg addEntries failed");
		return 1;
	} else {
		return 0;
	}
}

oneslnk *cdread(uint64_t minum, oneslnk *dnumchn) {
	//! TODO:
}

int cdrmv(uint64_t minum, oneslnk *dnumchn) {
	//! TODO:
}

int cdrer(uint64_t minum, twoslnk *rerchn) {		//! untested
	//! TODO:
}

oneslnk *idread(uint64_t minum, uint64_t start, uint64_t intrvl) {	// returns malloc memory (the whole chain)

	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(minum);
	
	if (indexSession == nullptr) {
		errorf("imiread could not open session");
		return 0;
	}
	
	std::forward_list<std::fs::path> retList = indexSession->readIntervalEntries(start, intrvl);
	
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
	
	if (i > intrvl) {
		errorf("imiread got more than intrvl entries");
		killoneschn(flnk, 0);
		return 0;
	}
	
	lastlnk = flnk->next;
	free(flnk);
	return lastlnk;
	
}

std::shared_ptr<std::forward_list<std::fs::path>> idread(uint64_t minum, uint64_t start, uint64_t intrvl) {

	std::shared_ptr<DirIndex> indexSession = g_indexSessionHandler.openSession<DirIndex>(minum);
	
	if (indexSession == nullptr) {
		errorf("idread could not open session");
		return {};
	}
	
	std::shared_ptr<std::forward_list<std::fs::path>> retListPtr = indexSession->readIntervalEntries(start, intrvl);
	
	if (retListPtr == nullptr) {
		return {};
	}
	
	auto &retList = *retListPtr;
	
	int32_t i = 0;
	for (auto &entry : retList) {		
		i++;
	}
	
	if (i > intrvl) {
		errorf("idread got more than intrvl entries");
		return 0;
	}
	
	return retListPtr;
}

void verDI(const uint64_t &minum) { // check for order, duplicates, gaps, 0 character strings
	
}

//}

//{	subdir

int readSubdirEntryTo(FILE *sourcef, struct subdirentry *dest) {
	
}

uint64_t getlastsdnum(uint64_t minum) {
	std::shared_ptr<SubDirIndex> indexSession = g_indexSessionHandler.openSession<SubDirIndex>(minum);
	
	if (indexSession == nullptr) {
		errorf("getlastsdnum could not open session");
		return 0;
	}
	
	int32_t error = 0;
	return indexSession->getNofVirtualEntries(error);
}

oneslnk *isdread(uint64_t minum, uint64_t start, uint64_t intrvl) {	// returns malloc memory (the whole chain)
	return nullptr;
}

//}

//{ file

/*

uint64_t getlastfnum(uint64_t minum) {
	std::shared_ptr<FileIndex> indexSession = g_indexSessionHandler.openSession<FileIndex>(minum);
	
	if (indexSession == nullptr) {
		errorf("getlastminum could not open session");
		return 0;
	}
	
	int32_t error = 0;
	return indexSession->getNofVirtualEntries(error);
}
*/

/*

uint64_t fireg(uint64_t dnum, char *fname) {
	if (fname == NULL) {
		errorf("fname was NULL");
		return 0;
	}
	std::shared_ptr<FileIndex> indexSession = g_indexSessionHandler.openSession<FileIndex>(minum);
	if (indexSession == nullptr) {
		errorf("(dreg) could not open session");
		return 0;
	}
	std::string str = std::string(dpath);
	uint64_t res = indexSession->addEntry(str);
	if (!res) {
		errorf("failed to register dpath");
		return 0;
	} else {
		return res;
	}
}

*/

uint64_t fireg(const uint64_t &minum, const std::string &fname) {
	/*
	std::shared_ptr<FileIndex> indexSession = g_indexSessionHandler.openSession<FileIndex>(minum);
	if (indexSession == nullptr) {
		errorf("(dreg) could not open session");
		return 0;
	}
	uint64_t res = indexSession->addEntry(fname);
	if (!res) {
		errorf("failed to register dpath");
		return 0;
	} else {
		return res;
	}
	*/
}

char cfireadtag(uint64_t dnum, oneslnk *fnums, oneslnk **retfname, oneslnk **rettags, unsigned char presort) {	//! untested
/*
	FILE *fIndex;
	unsigned char buf[MAX_PATH*4], *p;
	int c, i;
	uint64_t tnum;
	oneslnk *tlink, *link1, *link2, *link3, *link4, *link5;
	
	if (retfname)
		*retfname = 0;
	if (rettags)
		*rettags = 0;
	if (dnum == 0) {
		errorf("dnum is 0");
		return 1;
	} if (fnums == 0) {
		errorf("fnums is 0");
		return 1;
	}
	if (!(retfname || rettags)) {
		errorf("no address to return file names or tags");
		return 1;
	}
	
	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dnum);
	if ((fIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("fIndex file not found (firead)");
		return 1;
	}
	
	if (!presort) {
		if (!(fnums = copyoneschn(fnums, 1))) {
			errorf("copyoneschn failed");
			return 1;
		}
		if (sortoneschnull(fnums, 0)) {
			errorf("sortoneschnull failed");
			return 1;
		}
	}
	if (fnums->ull == 0) {
		errorf("passed 0 fnum to cfireadtag");
		fclose(fIndex), presort? 0:killoneschn(fnums, 1);
		return 1;
	}
	link1 = fnums;
	if (retfname) {
		link2 = link3 = malloc(sizeof(oneslnk));
		link2->str = 0;
	} if (rettags) {
		link4 = link5 = malloc(sizeof(oneslnk));
		link4->vp = 0;
	}
	
	fseek64(fIndex, finitpos(dnum, fnums->ull), SEEK_SET);	//! also add fseeking if the distance to next entry is larger than fnum/(sqrt(lastfnum)+fnum%sqrt(lastfnum)
	
	while (link1) {
		tnum = fgetull_pref(fIndex, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf_old("fIndex num read failed: %d", c);
			fclose(fIndex), presort? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
			return 1;
		}
		if (link1->ull > tnum) {
			if ((c = null_fgets(0, MAX_PATH*4, fIndex)) != 0) {
				errorf_old("fIndex read error: %d", c);
				fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			while ((c = getc(fIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in fIndex");
					fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				if (fseek(fIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
			}
		} else if (link1->ull == tnum) {
			if ((c = null_fgets(buf, MAX_PATH*4, fIndex)) != 0) {
				errorf_old("fIndex read error: %d", c);
				fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			if (retfname) {
				link3 = link3->next = malloc(sizeof(oneslnk));
				if (!(link3->str = dupstr(buf, MAX_PATH*4, 0))) {
					errorf_old("failed to duplicate buf: %s", buf);
				}
			}
			if (rettags) {
				if ((link5 = link5->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed for link5");
					fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				if ((tlink = link5->vp = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed for link5->vp");
					fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				while (1) {
					tnum = fgetull_pref(fIndex, &c);
					if (c != 0) {
						if (c == 4) {
							break;
						}
						errorf_old("fIndex tag read failed: %d", c);
						tlink->next = 0, fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0, tlink->next = 0;
						return 1;
					} tlink = tlink->next = malloc(sizeof(oneslnk));
					tlink->ull = tnum;
				}
				tlink->next = 0, tlink = link5->vp, link5->vp = tlink->next, free(tlink);
			} else {
				while ((c = getc(fIndex)) != 0) {
					if (c == EOF) {
						errorf("EOF before end of tags in fIndex");
						fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
						return 1;
					}
					if (fseek(fIndex, c, SEEK_CUR)) {
						errorf("fseek failed");
						fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
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
		errorf_old("tried to read non-existent fnum: %d -- last read %d", link1->ull, tnum);
		presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
		return 1;
	}
	presort ? 0:killoneschn(fnums, 1);
	
	if (retfname) {
		link3->next = 0;
		*retfname = link2->next;
		free(link2);
	} if (rettags) {
		link5->next = 0;
		*rettags = link4->next;
		free(link4);
	}
	
	return 0;
*/
	return 1;
	
}

char *firead(uint64_t dnum, uint64_t fnum) {	//! untested // returns malloc memory

}

oneslnk *cfiread(uint64_t dnum, oneslnk *fnumchn) {
	/*
	int c;
	oneslnk *retlnk;
	
	retlnk = 0;
	c = cfireadtag(dnum, fnumchn, &retlnk, NULL, 0);
	
	if (!c) {
		return retlnk;
	} else {
		return NULL;
	}
	*/
	return NULL;
}

unsigned char firmv(uint64_t dnum, uint64_t fnum) {		//! not done
	
	return 0; //!
}

int cfireg(uint64_t dnum, oneslnk *fnamechn) {
	return 1;
}

oneslnk *ifiread(uint64_t minum, uint64_t start, uint64_t intrvl) {	// returns malloc memory (the whole chain)

	std::shared_ptr<MainIndexIndex> indexSession = g_indexSessionHandler.openSession<MainIndexIndex>();
	
	if (indexSession == nullptr) {
		errorf("imiread could not open session");
		return 0;
	}
	
	std::forward_list<std::string> retList = indexSession->readIntervalEntries(start, intrvl);
	
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
	
	if (i > intrvl) {
		errorf("imiread got more than intrvl entries");
		killoneschn(flnk, 0);
		return 0;
	}
	
	lastlnk = flnk->next;
	free(flnk);
	return lastlnk;
	
}

unsigned char addremfnumctagc(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *remtagnums) {		//! untested
/*
	FILE *fIndex, *tfIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], done;
	int c, i, j;
	oneslnk *flink, *link1, *link2;
	uint64_t tnum;
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 1;
	}
	if (!fnums || !(addtagnums || remtagnums)) {
		errorf("no fnums or no tagnums in addfnumtagc");
		return 1;
	}
	
	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dnum);
	if ((fIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("fIndex file not found (firead)");
		return 1;
	}
	sprintf(baf, "%s\\i\\%llu\\fIndex.tmp", g_prgDir, dnum);
	if ((tfIndex = MBfopen(baf, "wb")) == NULL) {
		errorf_old("couldn't create i\\%llu\\fIndex.tmp", dnum);
		fclose(fIndex);
		return 2;
	}
	
	if (!(flink = fnums = copyoneschn(fnums, 1))) {
		errorf("copyoneschn failed");
		fclose(fIndex), fclose(tfIndex), MBremove(baf);
		return 2;
	}
	if (sortoneschnull(flink, 0)) {
		errorf("sortoneschnull failed");
		fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1);
		return 2;
	}
	if (fnums->ull == 0) {
		errorf("passed 0 tag num to add in addremfnumctagc");
		fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1);
		return 2;
	}
	
	if (addtagnums) {
		if (!(addtagnums = copyoneschn(addtagnums, 1))) {
			errorf("copyoneschn failed");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1);
			return 2;
		}
		if (sortoneschnull(addtagnums, 0)) {
			errorf("sortoneschnull failed");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), killoneschn(addtagnums, 1);
			return 2;
		}
		if (addtagnums->ull == 0) {
			errorf("passed 0 tag num to remove in addremfnumctagc");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), killoneschn(addtagnums, 1);
			return 2;
		}
	}
	
	if (remtagnums) {
		if (!(remtagnums = copyoneschn(remtagnums, 1))) {
			errorf("copyoneschn failed");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addtagnums ? killoneschn(addtagnums, 1) : 0;
			return 2;
		}
		if (sortoneschnull(remtagnums, 0)) {
			errorf("sortoneschnull failed");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addtagnums ? killoneschn(addtagnums, 1) : 0, killoneschn(remtagnums, 1);
			return 2;
		}
		if (remtagnums->ull == 0) {
			errorf("passed 0 dnum to cdrmv");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addtagnums ? killoneschn(addtagnums, 1) : 0, killoneschn(remtagnums, 1);
			return 2;
		}
	}
		
	while (1) {
		if (!fnums) {
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
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addtagnums ? killoneschn(addtagnums, 1) : 0, remtagnums ? killoneschn(remtagnums, 1) : 0;
			return 2;
		} putull_pref(tnum, tfIndex);

		for (i = 0; (c = getc(fIndex)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in fIndex");
				fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addtagnums ? killoneschn(addtagnums, 1) : 0, remtagnums ? killoneschn(remtagnums, 1) : 0;
				return 2;
			} else
				putc(c, tfIndex);
		}
		putc(c, tfIndex);
		if (i == MAX_PATH*4) {
			errorf("too long string in fIndex");
			fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addtagnums ? killoneschn(addtagnums, 1) : 0, remtagnums ? killoneschn(remtagnums, 1) : 0;
			return 2;
		}
		if (tnum < fnums->ull) {
			while (1) {
				tnum = fgetull_pref(fIndex, &c);
				if (c != 0) {
					if (c == 4) {
						break;
					}
					errorf_old("fIndex tag read failed: %d", c);
					fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addtagnums ? killoneschn(addtagnums, 1) : 0, remtagnums ? killoneschn(remtagnums, 1) : 0;
					return 1;
				}
				putull_pref(tnum, tfIndex);
			} putc('\0', tfIndex);
		} else if (tnum == fnums->ull) {
			link1 = addtagnums, link2 = remtagnums;
			while (1) {
				tnum = fgetull_pref(fIndex, &c);
				if (c != 0) {
					if (c == 4) {
						break;
					}
					errorf_old("fIndex tag read failed: %d", c);
					fclose(fIndex), fclose(tfIndex), MBremove(baf), killoneschn(flink, 1), addtagnums ? killoneschn(addtagnums, 1) : 0, remtagnums ? killoneschn(remtagnums, 1) : 0;
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
			fnums = fnums->next;
		} else {
			break;
		}
	}
	fclose(fIndex); fclose(tfIndex), addtagnums ? killoneschn(addtagnums, 1) : 0, remtagnums ? killoneschn(remtagnums, 1) : 0;
	
	if (fnums != 0 || c != EOF) {
		errorf("addremfnumctagc broke early");
		MBremove(baf), killoneschn(flink, 1);
		return 1;
	}
	char bif[MAX_PATH*4];
	sprintf(bif, "%s\\i\\%llu\\fIndex.bak", g_prgDir, dnum);
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
	
	addtolastfnum(dnum, 0, flink->ull);
	killoneschn(flink, 1);
	MBremove(bif);
	
	return 0;
	
*/
return 1;
}

unsigned char addfnumctag(uint64_t dnum, oneslnk *fnums, uint64_t tagnum) {	//!untested
/*
	int c;
	oneslnk *link;
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 1;
	}
	
	link = malloc(sizeof(oneslnk));
	link->next = 0, link->ull = tagnum;
	
	c = addremfnumctagc(dnum, fnums, link, 0);
	free(link);
	return c;
*/
	return 1;
}
/*
uint64_t pfireg(char *fpath, unsigned char flag) {	// Registers file from path --  bit 1 of flag: register directory if not registered, 2: check whether file is already registered first, 3: return 0 if already registered
	char buf1[MAX_PATH*4], buf2[MAX_PATH*4];
	uint64_t dnum, fnum;
	
	breakpathdf(fpath, buf1, buf2);
	if ((dnum = rdread(buf1)) == 0) {
		if (flag & 1) {
			if ((dnum = dreg(buf1)) == 0) {
				return 0;
			}
		} else {
			return 0;
		}
	}
	
	if (flag & 2) {
		if ((fnum = rfiread(dnum, buf2)) != 0) {
			if (flag & 4)
				return 0;
			return fnum;
		}
	}
	return fireg(dnum, buf2);
}
*/
/*
unsigned char dirfreg(char *path, unsigned char flag) {	// bit 1: register directory if not registered, 2: remove files that only exist in index (maybe only if they have no tags or associated things), 3: register subdirectories, 4: reinitialize rfIndex, 5: set dir checked date, 6: don't mark missing files as missing, 7: register files from directories recursively
	uint64_t dnum;
	char *s, *s2, buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], firecfin = 0, keep = 0, empty = 0, skipdirs = 1, recdirs = 0;
	FILE *tfile, *tfile2, *rfIndex;
	int c, i;
	oneslnk *flink, *link;
	
	skipdirs = !(flag & (1 << (3-1)));
	recdirs = !!(flag & (1 << (7-1)));
	
	uint64_t ulltime = time(0);
	
	if ((dnum = rdread(path)) == 0) {
		if (flag & 1) {
			if ((dnum = dreg(path)) == 0) {
				return 1;
			}
		} else {
			return 1;
		}
	}
	
	sprintf(buf, "%s\\i\\%llu\\rfIndex.bin", g_prgDir, dnum);

	if ((flag & 8) || !(rfIndex = MBfopen(buf, "rb"))) {
		if (rfinit(dnum) == 1) {
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
					firmv(dnum, rfiread(dnum, buf2));
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
			cfireg(dnum, link);
			killoneschn(link, 0);
			link = flink = malloc(sizeof(oneslnk)), flink->str = 0;
			i = 0;
		}
	}
	fclose(tfile2), releasetfile(s2, 1), link->next = 0;
	
	if (i > 0) {
		link = flink->next, free(flink);
		cfireg(dnum, link);
	}
	killoneschn(link, 0);
	if (flag & 16) {
		setdlastchecked(dnum, ulltime);
	}
	return 0;
}
*/
int freesrexp(SREXP *sexp) {
	if (sexp == NULL) return 0;
	
	SREXP *next;
	
	if (sexp->exprtype == SREXP_TYPE_ROOT || sexp->exprtype == SREXP_TYPE_SUBEXP_MARKER) {
		next = sexp->expr1;
		free(sexp);
		freesrexp(next);
	} else if (sexp->exprtype == SREXP_TYPE_AND || sexp->exprtype == SREXP_TYPE_OR) {
		freesrexp(sexp->expr1);
		next = sexp->expr2;
		free(sexp);
		freesrexp(next);
	} else {
		free(sexp);
	}
	
	return 0;
}	

unsigned char parse_srexp(char **s) {
	while (**s == ' ') (*s)++;
	
	if (**s != '\0') {
		if (**s == '&' || **s == '+') {
			(*s)++;
			return SRPAR_TYPE_AND;
		} else if (**s == '|') {
			(*s)++;
			return SRPAR_TYPE_OR;			
		} else if (**s == '-' || **s == '!') {
			(*s)++;
			return SRPAR_TYPE_NEG;			
		} else if (**s == '(') {
			(*s)++;
			return SRPAR_TYPE_SUBEXPR_START;			
		} else if (**s == ')') {
			(*s)++;
			return SRPAR_TYPE_SUBEXPR_END;			
		} else {
			return SRPAR_TYPE_ALIAS;
		}
	} else {
		return 0;
	}
}


int stack_srch_alias(char **sp, twoslnk **aliasstack, SREXP *sexp) {
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
	astack->next = *aliasstack;
	astack->u[0].vp = sexp;
	astack->u[1].str = buf;
	*aliasstack = astack;
	
	return 0;
}
		
SEARCHSTRUCT *initftagsearch(char *searchstr, unsigned char *retarg, uint64_t dnum) { //! implement
	retarg? *retarg = 0:0;
	if (!searchstr) {	
		return NULL;
	}
	long long i;
	uint64_t subexp_layers;
	unsigned char uc, do_implicit, saved_expr, neg, anyneg;
	twoslnk *numstack, *aliasstack, *nextlnk;	// one for the pointer back, one for the number
	SREXP *sexp1, *sexp2, *rootexp;
	SREXPLIST *buildstack, *nextonbstack;
	SUPEXPSTACK *superexp, *nextsuperexp;
	
	aliasstack = numstack = NULL;
	buildstack = NULL;
	sexp1 = sexp2 = NULL;
	superexp = nextsuperexp = NULL;
	subexp_layers = 0;
	do_implicit = 0;
	neg = 0;
	anyneg = 0;
	uc = saved_expr = SRPAR_NO_EXPR;
	
	
	rootexp = malloc(sizeof(SREXP));
	rootexp->exprtype = SREXP_TYPE_ROOT;
	sexp1 = rootexp->expr1 = malloc(sizeof(SREXP));
	sexp1->exprtype = SREXP_TYPE_BLANK;
	
	while (1) {
errorf("loop");
		if (!do_implicit) { // if an operand is found without an operator, fill in implicit operator
			uc = parse_srexp(&searchstr);
			if (uc == 0) {
				break;
			}
		} else if (do_implicit == 1) {
			if (SRCH_IMPLICIT_AND) {
				uc = SRPAR_TYPE_AND;
			} else if (SRCH_IMPLICIT_OR) {
				uc = SRPAR_TYPE_OR;
			} else {
				errorf("do_implicit is 1 without SRCH_IMPLICIT_AND or SRCH_IMPLICIT_OR");
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
		case SRPAR_TYPE_ALIAS:
		case SRPAR_TYPE_NEG: // negation
		case SRPAR_TYPE_SUBEXPR_START: // left parenthesis
			if (sexp1->exprtype != SREXP_TYPE_BLANK) {	// the selected search expression isn't blank
				if (SRCH_IMPLICIT_AND || SRCH_IMPLICIT_OR) {
					do_implicit = 1;
					saved_expr = uc;
					continue;
				} else {
					errorf("syntax error: operand without operator and no implicit operator specified");
					retarg? *retarg = 1:0;
					goto search_cleanup1;	// buildstack needs to be climbed and erased, aliasstack erased and all expressions erased
				}
			} else {
				if (uc == SRPAR_TYPE_NEG) {
errorf("neg2");
					neg = !neg;
				} else {
					if (neg) {
						sexp1->exprtype = SREXP_TYPE_NEG;
						sexp1 = sexp1->expr1 = malloc(sizeof(SREXP));
						sexp1->exprtype = SREXP_TYPE_BLANK;
					}
					if (uc == SRPAR_TYPE_ALIAS) {
errorf("srpar alias");
						sexp1->exprtype = SREXP_TYPE_TBD;
						if (stack_srch_alias(&searchstr, &aliasstack, sexp1)) {
							errorf("alias syntax error");
							goto search_cleanup1;
						};
					} else if (uc == SRPAR_TYPE_SUBEXPR_START) {
						if (neg) {
							nextonbstack = buildstack;
							buildstack = malloc(sizeof(SREXPLIST));
							buildstack->next = nextonbstack;
							buildstack->expr = sexp1;
						}
						
						if (!(buildstack == NULL || buildstack->expr->exprtype == SREXP_TYPE_SUBEXP_MARKER)) {
							sexp1->exprtype = SREXP_TYPE_SUBEXP_MARKER;
							
							nextonbstack = buildstack;
							buildstack = malloc(sizeof(SREXPLIST));
							buildstack->next = nextonbstack;
							buildstack->expr = sexp1;
							
							nextsuperexp = superexp;
							superexp = malloc(sizeof(SUPEXPSTACK));
							superexp->next = nextsuperexp;
							superexp->subexp_layers = subexp_layers;
							
							subexp_layers = 0;
							
							sexp1 = sexp1->expr1 = malloc(sizeof(SREXP));
							sexp1->exprtype = SREXP_TYPE_BLANK;
						}
						subexp_layers++;
					} else {
						errorf("switch error 1");
						retarg? *retarg = 1:0;
						goto search_cleanup1;
					}
					neg = 0;
				}
			}
			break;
		case SRPAR_TYPE_AND:	// and
		case SRPAR_TYPE_OR:	// or
			if (sexp1->exprtype == SREXP_TYPE_BLANK) {
				errorf("AND or OR operator instead of operand");
				retarg? *retarg = 1:0;
				goto search_cleanup1;
			}

			sexp2 = malloc(sizeof(SREXP));
			if (uc == SRPAR_TYPE_AND) {
				sexp2->exprtype = SREXP_TYPE_AND;
			} else {
				sexp2->exprtype = SREXP_TYPE_OR;
			}
			if (uc == SRPAR_TYPE_OR) {	// climb out of "and" chain -- "or" has lesser priority
				while (buildstack != NULL && buildstack->expr->exprtype == SREXP_TYPE_AND) {
					nextonbstack = buildstack->next;
					free(buildstack);
					buildstack = nextonbstack;
				}
			}
			
			if (buildstack == NULL) {
				sexp2->expr1 = rootexp->expr1;
				rootexp->expr1 = sexp2;
				nextonbstack = buildstack;
			} else if (buildstack->expr->exprtype == SREXP_TYPE_SUBEXP_MARKER) {
				sexp2->expr1 = buildstack->expr->expr1;
				buildstack->expr->expr1 = sexp2;
				nextonbstack = buildstack;
			} else if (buildstack->expr->exprtype == SREXP_TYPE_OR || buildstack->expr->exprtype == SREXP_TYPE_AND) {
				if (buildstack->expr->exprtype == SREXP_TYPE_OR && uc == SRPAR_TYPE_AND) {
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
				retarg? *retarg = 1:0;
				goto search_cleanup1;
			}
			
			sexp2->expr2 = sexp1 = malloc(sizeof(SREXP));
			sexp1->exprtype = SREXP_TYPE_BLANK;
			buildstack = malloc(sizeof(SREXPLIST));
			buildstack->next = nextonbstack;
			buildstack->expr = sexp2;
			
			break;
		case SRPAR_TYPE_SUBEXPR_END: // right parenthesis
			if (sexp1->exprtype == SREXP_TYPE_BLANK) {
				errorf("right parenthesis instead of operand");
				retarg? *retarg = 1:0;
				goto search_cleanup1;
			}
			if (subexp_layers < 1) {
				errorf("right parenthesis without left one");
				retarg? *retarg = 1:0;
				goto search_cleanup1;
			}
			subexp_layers--;
			while (buildstack != NULL && buildstack->expr->exprtype != SREXP_TYPE_SUBEXP_MARKER) {
				nextonbstack = buildstack->next;
				free(buildstack);
				buildstack = nextonbstack;
			}
			if (subexp_layers == 0 && buildstack != NULL) { // if buildstack is null the following operator will just continue from root
				subexp_layers = superexp->subexp_layers;
				sexp1 = buildstack->expr->expr1;
				free(buildstack->expr);
				
				nextonbstack = buildstack;
				buildstack = buildstack->next;
				free(nextonbstack);
				
				// impossible for the next expression to be SUBEXP_MARKER as well because otherwise _layers would just be incremented and other operands stack
				if (buildstack->expr->exprtype == SREXP_TYPE_OR || buildstack->expr->exprtype == SREXP_TYPE_AND) {
					buildstack->expr->expr2 = sexp1;
				} else if (buildstack->expr->exprtype == SREXP_TYPE_NEG) {
					buildstack->expr->expr1 = sexp1;
				} else { //! probably segfault or memory leak
					errorf("unknown parent expression (2)");
					free(sexp1);
					retarg? *retarg = 1:0;
					goto search_cleanup1;
				}
			}
			
			break;
		default:
			errorf("unknown search expression");
			retarg? *retarg = 1:0;
			goto search_cleanup1;
		}
		
		if (do_implicit > 2) {
			do_implicit = 0;
			saved_expr = SRPAR_NO_EXPR;
		}
	}
	
errorf("initftag2");
if (aliasstack) {
	errorf_old("alias: %s", aliasstack->u[1].str);
}
	
	while (buildstack != NULL) {
		nextonbstack = buildstack->next;
		free(buildstack);
		buildstack = nextonbstack;
	}
	
	while (superexp != NULL) {
		nextsuperexp = superexp->next;
		free(superexp);
		superexp = nextsuperexp;
	}
	
	if (rootexp->expr1->exprtype == SREXP_TYPE_BLANK) {
		// no expressions at all
		while (aliasstack != NULL) {
			nextlnk = aliasstack->next;
			
			if (aliasstack->u[1].str != NULL)
				free(aliasstack->u[1].str);
			
			free(aliasstack);
			aliasstack = nextlnk;
		}
		freesrexp(rootexp);
		
		return NULL;
		
	} else if (sexp1->exprtype == SREXP_TYPE_BLANK) {
		errorf("unfilled blank expression");
		retarg? *retarg = 1:0;
		goto search_cleanup1;
	}
	
	if (!TOLERATE_UNCLOSED && subexp_layers > 0) {
		errorf("unclosed subexpression");
		retarg? *retarg = 1:0;
		goto search_cleanup1;
	}
	
	{
		tnumfromalias2(dnum, &aliasstack);
		
		twoslnk *lnk;
		
		lnk = aliasstack;
		i = 0;
		uint64_t prev = 0;
		
		while (lnk != NULL) {
			nextlnk = lnk->next;
			sexp1 = lnk->u[0].vp;
			
			if (lnk->u[1].ull == 0) {
				sexp1->exprtype = SREXP_TYPE_NULL;
			} else {
				sexp1->exprtype = SREXP_TYPE_TAGNUM;
				if (prev == 0) {
					prev = lnk->u[1].ull;
				} else if (prev != lnk->u[1].ull) {
					i++;
					prev = lnk->u[1].ull;
				}
				sexp1->refnum = i;
			}
			
			lnk = nextlnk;
		}
	
		unsigned long ntagnums = i+1;
		
		TAGNUMNODE *tagnumarray = malloc(sizeof(TAGNUMNODE)*(ntagnums));
		
		lnk = aliasstack;
		i = 0;
		prev = 0;
		
		while (lnk != NULL) {
			nextlnk = lnk->next;
			
			if (lnk->u[1].ull == 0) {
			} else {
				if (prev == 0) {
					prev = tagnumarray[i].tagnum = lnk->u[1].ull;
				} else if (prev != lnk->u[1].ull) {
					i++;
					prev = tagnumarray[i].tagnum = lnk->u[1].ull;
				}
			}
			
			lnk = nextlnk;
		}
		
		killtwoschn(aliasstack, 3);
		
		SEARCHSTRUCT *retstruct = malloc(sizeof(SEARCHSTRUCT));
		
		retstruct->tagnumarray = tagnumarray;
		retstruct->ntagnums = ntagnums;
		retstruct->rootexpr = rootexp->expr1;
		retstruct->dnum = dnum;
		free(rootexp);
		
		return retstruct;
		
		// build the search expression and when encountering tag aliases, link them to 
		
		return NULL;
	
	}
	
	search_cleanup1: {
		retarg && *retarg == 0? *retarg = 1:0;
		
		while (buildstack != NULL) {
			nextonbstack = buildstack->next;
			free(buildstack);
			buildstack = nextonbstack;
		}
		
		while (superexp != NULL) {
			nextsuperexp = superexp->next;
			free(superexp);
			superexp = nextsuperexp;
		}
		
		while (aliasstack != NULL) {
			nextlnk = aliasstack->next;
			
			if (aliasstack->u[1].str != NULL)
				free(aliasstack->u[1].str);
			
			free(aliasstack);
			aliasstack = nextlnk;
		}
		
		/*
		while (numstack != NULL) {
			nextlnk = numstack->next;
			
			
			
			free(numstack);
			numstack = nextlnk;
		}
		*/
		
		freesrexp(rootexp);
		
		return NULL;
	}
}

void killsearchexpr(struct searchexpr *sexp) {
	// if tagnum just free self, otherwise free non-null child expressions, error if first child is null but second one isn't
}

int srexpvalue(SREXP *sexp, SEARCHSTRUCT *sstruct) {
	uint16_t exprtype = sexp->exprtype;
	if (exprtype == SREXP_TYPE_TAGNUM) {
		return sstruct->tagnumarray[sexp->refnum].state;
	} else if (exprtype == SREXP_TYPE_AND) {
		if (srexpvalue(sexp->expr1, sstruct) == 0)
			return 0;
		else
			return srexpvalue(sexp->expr2, sstruct);
	} else if (exprtype == SREXP_TYPE_OR) {
		if (srexpvalue(sexp->expr1, sstruct) == 1)
			return 1;
		else
			return srexpvalue(sexp->expr2, sstruct);
	} else if (exprtype == SREXP_TYPE_NEG) {
		return !srexpvalue(sexp->expr1, sstruct);
	} else if (exprtype == SREXP_TYPE_NULL) {
		return 0;
	} else {
		errorf_old("unknown srexp (%d) (srexpvalue)", exprtype);
		return 0;
	}
}

unsigned char ftagcheck(FILE *file, uint64_t fnum, SEARCHSTRUCT *sstruct) {
	long long i;
	uint64_t tagnum;
	int c;
	
	if (sstruct->ntagnums <= 0) {
		return 1;
	}
	
	i = 0;
	while (1) {
	tagnum = fgetull_pref(file, &c);
		if (c != 0) {
			if (c == 1 || c == 4)
				break;
			errorf_old("fIndex tag num read failed: %d", c);
			return 2;
		}
		c = 0;
		
		while (i < sstruct->ntagnums && sstruct->tagnumarray[i].tagnum <= tagnum) {
			if (sstruct->tagnumarray[i].tagnum == tagnum) {
				sstruct->tagnumarray[i].state = 1;
			} else {
				sstruct->tagnumarray[i].state = 0;
			}
			i++;
		}
	}
	
	while (i < sstruct->ntagnums) {
		sstruct->tagnumarray[i].state = 0;
		i++;
	}
	
	c = srexpvalue(sstruct->rootexpr, sstruct);
	
	if (c == 0)
		return 1;
	else if (c == 1)
		return 0;
	else
		return 2;
}

void endsearch(SEARCHSTRUCT *sstruct) {
	if (!sstruct) {
//		errorf("endsearch with sstruct null");
		return;
	}
	if (sstruct->tagnumarray)
		free(sstruct->tagnumarray);
	if (sstruct->rootexpr)
		freesrexp(sstruct->rootexpr);
	free(sstruct);
}

char *ffireadtagext(uint64_t dnum, char *searchstr, char *exts) {	// returns tfile
	FILE *fIndex, *tfile, *tfilerec;
	unsigned char buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], *p, *p2, *p3, ascending = 0, uc;
	int c, i, flag;
	uint64_t tnum, nPut = 0, gap, nPos, fPos;
	char *tfstr;
	SEARCHSTRUCT *sstruct;
	
	if (dnum == 0) {
		errorf("dnum is 0");
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
	
	if (!(sstruct = initftagsearch(searchstr, &uc, dnum)) && uc != 0) {
		errorf("initftagsearch failed from searchstr");
		return 0;
	}

	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dnum);
	if ((fIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("fIndex file not found (ffireadtagext)");
		endsearch(sstruct);
		return 0;
	}
	if ((tfstr = reservetfile()) == 0) {
		errorf("reservetfile failed");
		fclose(fIndex), endsearch(sstruct);
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
			fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endsearch(sstruct);
			return 0;
		}
		if ((c = null_fgets(buf, MAX_PATH*4, fIndex)) != 0) {
			errorf_old("fIndex read error: %d", c);
			fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endsearch(sstruct);
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
		if (flag || !sstruct) {
			while ((c = getc(fIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in fIndex");
					fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endsearch(sstruct);
					return 0;
				}
				if (fseek(fIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endsearch(sstruct);
					return 0;
				}
			}
		} else {
			flag = ftagcheck(fIndex, tnum, sstruct);
		}
		if (!flag) {
			putull_pref(tnum, tfile);
			nPut++;
		}
	}
	
	fclose(fIndex), fclose(tfile), endsearch(sstruct);
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
}

uint64_t getframount(char *tfstr) {
	int i, c;
	uint64_t fram = 0;
	FILE *file;
	
	if (tfstr == 0) {
		errorf("tfstr is NULL");
		return 0;
	}
	
	if (((file = opentfile(tfstr, 1, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
		if (file != NULL)
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
		errorf("tfstr is NULL");
		return 0;
	}
	
	if (((file = opentfile(tfstr, 1, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
		if (file != NULL)
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
}

oneslnk *ifrread(char *tfstr, uint64_t start, uint64_t intrvl) {
	FILE *tfile;
	unsigned char buf[MAX_PATH*4];
	int c, i;
	uint64_t tnum, npos;
	oneslnk *flnk, *lastlnk;
	
	if (tfstr == 0) {
		errorf("tfstr is NULL");
		return 0;
	}
	if (!intrvl) {
		errorf("ifrread without intrvl");
		return 0;
	}
	
	if ((tfile = opentfile(tfstr, 0, "rb")) == NULL) {
		if (tfile != NULL)
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

			while (intrvl > 0) {
				if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed");
					lastlnk->next = 0, fclose(tfile), killoneschn(flnk, 1);
					return 0;
				}
				lastlnk->ull = tnum;
				intrvl--;
				
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
	return 0;
}
//}
//{ tag 2nd layer

char reftaliasrec(uint64_t dnum) {	//ref for refresh		//! not done
	if (dnum == 0) {
		errorf("dnum is zero in refaliasrec");
		return 1;
	}
	return 0;
}

uint64_t tainitpos(uint64_t dnum, char *aliasstr) {
	return 0;
}

char aliasentryto(FILE *fromfile, FILE *tofile, uint64_t aliascode) {	// doesn't write aliascode		//! untested
	uint64_t tnum;
	int c;
	
	switch (aliascode) {
	case 0: case 1:		// 0 for tag name, 1 for alternative tag name
		tnum = fgetull_pref(fromfile, &c);
		if (c) {
			errorf_old("aliasentryto fgetull_pref: %d", c);
			return 1;
		} putull_pref(tnum, tofile);
		break;
	default:
		errorf("unknown aliascode");
		return 1;
		break;
	}
	return 0;
}

char crtreg(uint64_t dnum, oneslnk *tagnames, oneslnk *tagnums) { //! untested
	FILE *tAlias, *ttAlias;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4];
	int i, c, j;
	uint64_t tnum;
	oneslnk *link1, *link2;
	twoslnk *ftwolink, *twolink, twolink2;
	
	if (dnum == 0) {
		errorf("dnum is zero in rtreg");
		return 1;
	}
	if (tagnames) {
		for (i = 0, link1 = tagnames; link1; i++, link1 = link1->next) {
			if (!link1->str || link1->str[0] == '\0') {
				errorf("tried to register blank alias");
				return 1;
			}
		}
	} else {
		errorf("no tagnames in crtreg");
		return 1;
	}
	if (tagnums) {
		for (j = 0, link1 = tagnums; link1; link1 = link1->next, j++) {
			if (!link1->ull) {
				errorf("tried to register alias to tagnum 0");
				return 1;
			}
		}
	} else {
		errorf("no tagnums in crtreg");
		return 1;
	}
	if (i != j) {
		errorf("amount of tagnums differs from amount of tagnames in crtreg");
	}
	twolink = ftwolink = malloc(sizeof(twoslnk));	// must be freed as if both unions were ull since the strings are borrowed
	for (link1 = tagnames, link2 = tagnums; link1 && link2; twolink = twolink->next = malloc(sizeof(twoslnk)), twolink->u[0].str = link1->str, twolink->u[1].ull = link2->ull, link1 = link1->next, link2 = link2->next);
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
	
	sprintf(buf, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dnum);
	
	tAlias = MBfopen(buf, "rb");
	sprintf(baf, "%s\\ttAlias.bin", g_prgDir);
	if ((ttAlias = MBfopen(baf, "wb")) == NULL) {
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
			if (c = aliasentryto(tAlias, ttAlias, tnum)) {	// doesn't write the alias code itself
				errorf("aliasentryto failed");
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
	
	sprintf(buf, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dnum);
	char bif[MAX_PATH*4];
	if (tAlias) {
		sprintf(bif, "%s\\i\\%llu\\tAlias.bak", g_prgDir, dnum);
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
	
	reftaliasrec(dnum);
	if (tAlias) {
		MBremove(bif);
	}
	return 0;
}

char rtreg(uint64_t dnum, char *tname, uint64_t tagnum) { //! untested
	int i;
	oneslnk *link1, *link2;
	
	if (dnum == 0) {
		errorf("dnum is zero in rtreg");
		return 1;
	}
	if (tagnum == 0) {
		errorf("tagnum in rdreg is 0");
		return 1;
	}
	
	if (tname) {
		for (i = 0; tname[i] != '\0' && i != MAX_ALIAS*4; i++);
		if (i >= MAX_ALIAS*4) {
			errorf("alias too long");
			return 1;
		}
	} if (i == 0 || tname == 0) {
		errorf("empty string");
		return 1;
	}
	
	link1 = malloc(sizeof(oneslnk));
	link2 = malloc(sizeof(oneslnk));
	link1->next = link2->next = 0;
	link1->str = tname, link2->ull = tagnum;
	crtreg(dnum, link1, link2);
	free(link1), free(link2);
	
	return 0;
}

oneslnk *tnumfromalias(uint64_t dnum, oneslnk *aliaschn, oneslnk **retalias) { //! untested	//! in the future: resolve multiple tags from alias and alias from alias -- option to read only original tag aliases
	FILE *tAlias;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], done = 0;
	int i, c, d;
	uint64_t tnum;
	oneslnk *retchn, *retlnk, *aretlnk, *flink;
	
	if (retalias) {
		*retalias = 0;
	}
	if (dnum == 0) {
		errorf("dnum is zero in tnumfromalias");
		return 0;
	}
	if (aliaschn == 0) {
		errorf("aliaschn in tnumfromalias is 0");
		return 0;
	}
	
	sprintf(buf, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dnum);
	tAlias = MBfopen(buf, "rb");
	
	if (!(flink = aliaschn = copyoneschn(aliaschn, 0))) {
		errorf("copyoneschn failed");
		tAlias?fclose(tAlias):0;
		return 0;
	}
	if (sortoneschn(flink, (int(*)(void*,void*)) strcmp, 0)) {
		errorf("sortoneschn failed");
		tAlias?fclose(tAlias):0, killoneschn(flink, 0);
		return 0;
	}
	if (aliaschn->str == 0) {
		errorf("passed 0 dnum to cfiread");
		tAlias?fclose(tAlias):0, killoneschn(flink, 0);
		return 0;
	}
	
	retchn = retlnk = malloc(sizeof(oneslnk));
	retlnk->ull = 0;
	
	if (retalias) {		// unmatched aliases
		*retalias = aretlnk = malloc(sizeof(oneslnk));
		aretlnk->str = 0;
	}
	
	if (tAlias) {
		fseek64(tAlias, tainitpos(dnum, aliaschn->str), SEEK_SET);
		
		while (aliaschn) {
			if ((c = null_fgets(buf, MAX_ALIAS*4, tAlias)) != 0) {
				if (c == 1)
					break;
				errorf_old("null_fgets: %d", c);
				fclose(tAlias);
				return 0;
			}
			tnum = fgetull_pref(tAlias, &c);
			if (c != 0) {
				errorf_old("tnumfromalias - tAlias num read failed: %d", c);
				fclose(tAlias);
				return 0;
			}
			
			while (aliaschn && (c = strcmp(aliaschn->str, buf)) < 0) {
				if (retalias) {
					aretlnk = aretlnk->next = malloc(sizeof(oneslnk));
					aretlnk->str = dupstr(aliaschn->str, MAX_ALIAS*4, 0);
				}
				aliaschn = aliaschn->next;
			}
			if (!aliaschn)
				break;
			
			if (c != 0 || !(tnum == 0 || tnum == 1)) {
				if (c = aliasentryto(tAlias, 0, tnum)) {
					errorf("aliasentryto failed");
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
				while (aliaschn && !strcmp(aliaschn->str, buf)) {
					retlnk = retlnk->next = malloc(sizeof(oneslnk));
					retlnk->ull = tnum;
					aliaschn = aliaschn->next;
				}
			}
		}
		fclose(tAlias);
	}
	retlnk->next = 0, retlnk = retchn->next, free(retchn);
	if (retalias) {
		while (aliaschn) {
			aretlnk = aretlnk->next = malloc(sizeof(oneslnk));
			aretlnk->str = malloc(strlen(aliaschn->str)+1);
			strncpy(aretlnk->str, aliaschn->str, strlen(aliaschn->str)+1);
			aliaschn = aliaschn->next;
		}
		aretlnk->next = 0, aretlnk = *retalias, *retalias = (*retalias)->next, free(aretlnk);
	}
	killoneschn(flink, 0);
	return retlnk;
}

int tnumfromalias2(uint64_t dnum, twoslnk **sourcelist) {
	FILE *tAlias;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], done = 0;
	int i, c, d;
	uint64_t tnum;
	oneslnk *retchn, *retlnk, *aretlnk;
	
	if (dnum == 0) {
		errorf("dnum is zero in tnumfromalias");
		return 1;
	}
	if (sourcelist == 0) {
		errorf("sourcelist is 0 (2)");
		return 1;
	}
	
	sprintf(buf, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dnum);
	tAlias = MBfopen(buf, "rb");
	
	if (sorttwoschn(sourcelist, (int(*)(void*,void*)) strcmp, 1, 0)) {
		errorf("sorttwoschn failed");
		tAlias?fclose(tAlias):0;
		return 1;
	}
	twoslnk *aliaschn = *sourcelist;
	
	if (aliaschn->u[1].str == 0) {
		errorf("passed 0 dnum to cfiread");
		tAlias?fclose(tAlias):0;
		return 1;
	}
	
	if (tAlias) {
		if (aliaschn)
			fseek64(tAlias, tainitpos(dnum, aliaschn->u[1].str), SEEK_SET);
		
		while (aliaschn) {
			if ((c = null_fgets(buf, MAX_ALIAS*4, tAlias)) != 0) {
				if (c == 1)
					break;
				errorf_old("null_fgets: %d", c);
				fclose(tAlias);
				return 1;
			}
			tnum = fgetull_pref(tAlias, &c);
			if (c != 0) {
				errorf_old("tnumfromalias - tAlias num read failed: %d", c);
				fclose(tAlias);
				return 1;
			}
			
			while (aliaschn && (c = strcmp(aliaschn->u[1].str, buf)) < 0) {
				free(aliaschn->u[1].str);
				aliaschn->u[1].ull = 0;
				aliaschn = aliaschn->next;
			}
			if (!aliaschn)
				break;
			
			if (c != 0 || !(tnum == 0 || tnum == 1)) {
				if (c = aliasentryto(tAlias, 0, tnum)) {
					errorf("aliasentryto failed");
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
				while (aliaschn && !strcmp(aliaschn->u[1].str, buf)) {
					free(aliaschn->u[1].str);
					aliaschn->u[1].ull = tnum;
					aliaschn = aliaschn->next;
				}
			}
		}
		fclose(tAlias);
	}
	
	if (sorttwoschnull(sourcelist, 1, 0)) {
		errorf("sorttwoschnull failed");
		return 1;
	}
	
	return 0;
}

//}
//{ tag
uint64_t tinitpos(uint64_t dnum, uint64_t ifnum) { /*		//! not done
	char buf[MAX_PATH*4], ch;
	int i, c;
	uint64_t dnum, pos = 0;
	FILE *direc;
	
	sprintf(buf, "%s\\diRec.bin", g_prgDir);
	
	if (((direc = MBfopen(buf, "rb")) == NULL) || ((i = getc(direc)) == EOF)) {
		if (direc != NULL)
			fclose(direc);
		addtolastdnum(0, 0);
		if (((direc = MBfopen(buf, "rb")) == NULL) || ((i = getc(direc)) == EOF)) {
			if (direc != NULL)
				fclose(direc);
			errorf("couldn't access direc");
			return 0;
		}		
	}
	fseek(direc, i, SEEK_CUR);
	while ((c = getc(direc)) != EOF) {
		for (ch = c, i = 1, dnum = 0; i <= ch && (c = getc(direc)) != EOF; i++, dnum *= 256, dnum += c);
		if (dnum > idnum)
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

unsigned char addtolasttnum(uint64_t dnum, long long num, uint64_t spot) { // 0 as num to just refresh from spot		//! not done -- make sure it refreshes from beginning even when num is positive

	return 0; //!
}

uint64_t getlasttnum(uint64_t dnum) {		//! not done
	return 0;
	/*
	char buf[MAX_PATH*4];
	int i, c;
	uint64_t lastdnum = 0;
	FILE *file;
	
	sprintf(buf, "%s\\diRec.bin", g_prgDir);
	
	if (((file = MBfopen(buf, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
		if (file != NULL)
			fclose(file);
		addtolastdnum(0, 0);
		if (((file = MBfopen(buf, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
			if (file != NULL)
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

char tcregaddrem(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *regtagnames, oneslnk **newtagnums, oneslnk *remtagnums) {	//! untested
	FILE *tIndex, *ttIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4];
	int c, i;
	uint64_t j, tnum, lasttnum = 0;
	oneslnk *link1, *link2, *faddtagnums, *fregtagnames, *fnewtagnums, *fremtagnums;

	if (dnum == 0) {
		errorf("dnum is zero in tcregaddrem");
		return 1;
	}
	if (!(addtagnums || remtagnums || regtagnames)) {
		errorf("no tags to add or remove from or to register");
		return 1;
	} if ((addtagnums || remtagnums) && !fnums) {
		errorf("tags to add or remove from without fnums");
		return 1;
	}
	if (newtagnums) {
		*newtagnums = 0;
	}
	if (fnums) {
		if (!(fnums = copyoneschn(fnums, 1))) {
			errorf("copyoneschn failed");
			return 0;
		}
		if (sortoneschnull(fnums, 0)) {
			errorf("sortoneschnull failed");
			killoneschn(fnums, 1);
			return 0;
		}
		if (fnums->ull == 0) {
			errorf("passed 0 fnum to tcregadd");
			killoneschn(fnums, 1);
			return 0;
		}
	}
	if (addtagnums) {
		if (!(faddtagnums = addtagnums = copyoneschn(addtagnums, 1))) {
			errorf("copyoneschn failed");
			killoneschn(fnums, 1);
			return 0;
		}
		if (sortoneschnull(faddtagnums, 0)) {
			errorf("sortoneschnull failed");
			killoneschn(faddtagnums, 1), killoneschn(fnums, 1);
			return 0;
		}
		if (addtagnums->ull == 0) {
			errorf("passed 0 tagnum to tcregadd");
			killoneschn(faddtagnums, 1), killoneschn(fnums, 1);
			return 0;
		}
	} else {
		faddtagnums = 0;
	}
	if (remtagnums) {
		if (!(fremtagnums = remtagnums = copyoneschn(remtagnums, 1))) {
			errorf("copyoneschn failed");
			killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0;
			return 0;
		}
		if (sortoneschnull(fremtagnums, 0)) {
			errorf("sortoneschnull failed");
			killoneschn(fremtagnums, 1), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0;
			return 0;
		}
		if (remtagnums->ull == 0) {
			errorf("passed 0 tagnum to tcregadd");
			killoneschn(fremtagnums, 1), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0;
			return 0;
		}
	} else {
		fremtagnums = 0;
	}
	
	sprintf(buf, "%s\\i\\%llu\\tIndex.bin", g_prgDir, dnum);
	if ((tIndex = MBfopen(buf, "rb+")) == NULL) {
		if ((tIndex = MBfopen(buf, "wb+")) == NULL) {
			errorf("tIndex file not created");
			killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
			return 1;
		}
	}
	sprintf(baf, "%s\\i\\%llu\\tIndex.tmp", g_prgDir, dnum);
	if ((ttIndex = MBfopen(baf, "wb+")) == NULL) {
		errorf("tIndex file not created");
		fclose(tIndex), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
		return 1;
	}
	
	if (!(addtagnums || remtagnums) && (lasttnum = getlasttnum(dnum))) {
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
				fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
				return 1;
			} putull_pref(tnum, ttIndex);
			lasttnum = tnum;

			for (i = 0; (c = getc(tIndex)) != '\0' && i < MAX_PATH*4; i++) {
				if (c == EOF) {
					errorf("EOF before null terminator in tIndex");
					fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
					return 1;
				} else
					putc(c, ttIndex);
			} putc(c, ttIndex);
			if (i == MAX_PATH*4) {
				errorf("too long string in tIndex");
				fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
				return 1;
			}
			if ((!addtagnums || tnum < addtagnums->ull) && (!remtagnums || tnum < remtagnums->ull)) {
				while (1) {
					tnum = fgetull_pref(tIndex, &c);
					if (c != 0) {
						if (c == 4) {
							break;
						}
						errorf_old("tIndex tag read failed: %d", c);
						fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
						return 1;
					}
					putull_pref(tnum, ttIndex);
				} putc('\0', ttIndex);
			} else {
				if (ALLOW_TAGGING_NOTHING) { // non-existent tagnums will be skipped
					while (remtagnums && remtagnums->ull < tnum)
						remtagnums = remtagnums->next;
					while (addtagnums && addtagnums->ull < tnum)
						addtagnums = remtagnums->next;
				}
				if (remtagnums && remtagnums->ull < tnum || addtagnums && addtagnums->ull < tnum) {
					break;
				}
	
				if (!(remtagnums && remtagnums->ull == tnum) && addtagnums && addtagnums->ull == tnum) {
					link1 = fnums;
					while (1) {
						tnum = fgetull_pref(tIndex, &c);
						if (c != 0) {
							if (c == 4) {
								break;
							}
							errorf_old("tIndex tag read failed: %d", c);
							fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
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
				} else if (!(addtagnums && addtagnums->ull == tnum) && remtagnums && remtagnums->ull == tnum) {
					link1 = fnums;
					while (1) {
						tnum = fgetull_pref(tIndex, &c);
						if (c != 0) {
							if (c == 4) {
								break;
							}
							errorf_old("tIndex tag read failed: %d", c);
							fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
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
							fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
							return 1;
						}
						putull_pref(tnum, ttIndex);
					} putc('\0', ttIndex);
				}
				while (remtagnums && remtagnums->ull == lasttnum)
					remtagnums = remtagnums->next;
				while (addtagnums && addtagnums->ull == lasttnum) {
					errorf_old("add matched: %llu", lasttnum);
					addtagnums = addtagnums->next;
				}
			}
		}
		if (!ALLOW_TAGGING_NOTHING && (addtagnums || remtagnums)) {
			errorf("tried to add or remove fnums to non-existent tagnum");
			if (addtagnums) errorf_old("addtagnums->ull: %llu", addtagnums->ull);
			if (remtagnums) errorf_old("remtagnums->ull: %llu", remtagnums->ull);
		}
		faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
		if (addtagnums || remtagnums) {
			fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1);
			return 1;
		}
	}
	fclose(tIndex);
		
	fregtagnames = regtagnames;
	link1 = fnewtagnums = malloc(sizeof(oneslnk));
	j = 0;
	while (regtagnames) {
		if (regtagnames->str) {
			for (i = 0; regtagnames->str[i] != '\0' && i != MAX_ALIAS*4; i++);
			if (i >= MAX_ALIAS*4) {
				errorf_old("regtagnames->str too long -- max: %d", MAX_ALIAS*4);
				fclose(ttIndex), MBremove(baf), link1->next = 0, killoneschn(fnewtagnums, 1), killoneschn(fnums, 1);
				return 1;
			}
		}
		link1 = link1->next = malloc(sizeof(oneslnk)); 
		if (!(i == 0 || regtagnames->str == 0)) {
			putull_pref(++lasttnum, ttIndex);
			term_fputs(regtagnames->str, ttIndex);
			if (fnums) {
				link2 = fnums;
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
			fclose(ttIndex), MBremove(baf), link1->next = 0, killoneschn(fnewtagnums, 1), killoneschn(fnums, 1);
			return 1;
		}
		regtagnames = regtagnames->next;
	}
	fclose(ttIndex), killoneschn(fnums, 1);
	link1->next = 0, link1 = fnewtagnums, fnewtagnums = fnewtagnums->next, free(link1);
	
	char bif[MAX_PATH*4];
	sprintf(bif, "%s\\i\\%llu\\tIndex.bak", g_prgDir, dnum);
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
		if (c = crtreg(dnum, fregtagnames, fnewtagnums)) {
			errorf_old("crtreg failed: %d", c);
			MBremove(buf), MBrename(bif, buf);
			return 1;
		}
	}
	if (newtagnums) {
		*newtagnums = fnewtagnums;
	} else if (fnewtagnums) {
		killoneschn(fnewtagnums, 1);
	}
	addtolasttnum(dnum, j, 0);
	
	MBremove(bif);
	return 0;
}

uint64_t treg(uint64_t dnum, char *tname, oneslnk *fnums) {
	int c, i;
	uint64_t lasttnum = 0;
	oneslnk *link, *rettagnum;

	if (tname) {
		for (i = 0; tname[i] != '\0' && i != MAX_ALIAS*4; i++);
		if (i >= MAX_ALIAS*4) {
			errorf("tname too long");
			return 0;
		}
	} if (i == 0 || tname == 0) {
		errorf("empty tname string");
		return 0;
	}
	link = malloc(sizeof(oneslnk));
	link->next = 0; link->str = tname;
	tcregaddrem(dnum, fnums, 0, link, &rettagnum, 0);
	free(link);
	if (rettagnum) {
		lasttnum = rettagnum->ull, killoneschn(rettagnum, 1);
	} else {
		lasttnum = 0;
	}
	return lasttnum;
}

char ctread(uint64_t dnum, oneslnk *tagnums, oneslnk **rettagname, oneslnk **retfnums, unsigned char presort) {	//! untested
	FILE *tIndex;
	unsigned char buf[MAX_PATH*4];
	int c, i;
	uint64_t tnum;
	oneslnk *link1, *link2, *link3, *link4, *link5, *tlink;
	
	if (rettagname)
		*rettagname = 0;
	if (retfnums)
		*retfnums = 0;
	if (dnum == 0) {
		errorf("dnum is 0 ctread");
		return 1;
	} if (tagnums == 0) {
		errorf("tagnums is 0 ctread");
		return 1;
	}
	if (!(rettagname || retfnums)) {
		errorf("no address to return tag names or fnums");
		return 1;
	}
	
	sprintf(buf, "%s\\i\\%llu\\tIndex.bin", g_prgDir, dnum);
	if ((tIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("tIndex file not found (ctread)");
		return 1;
	}
	
	if (!presort) {
		if (!(link1 = tagnums = copyoneschn(tagnums, 1))) {
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
	if (tagnums->ull == 0) {
		errorf("passed 0 tagnum to ctread");
		fclose(tIndex), presort? 0:killoneschn(tagnums, 1);
		return 1;
	}
	link1 = tagnums;
	if (rettagname) {
		link2 = link3 = malloc(sizeof(oneslnk));
		link2->str = 0;
	} if (retfnums) {
		link4 = link5 = malloc(sizeof(oneslnk));
		link4->vp = 0;
	}
	
	fseek64(tIndex, tinitpos(dnum, tagnums->ull), SEEK_SET);
	
	while (link1) {
		tnum = fgetull_pref(tIndex, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf_old("tIndex num read failed: %d", c);
			fclose(tIndex), presort? 0:killoneschn(tagnums, 1), rettagname? killoneschn(link2, 0):0, retfnums? killoneschnchn(link4, 1):0;
			return 1;
		}
		if (link1->ull > tnum) {
			if ((c = null_fgets(0, MAX_ALIAS*4, tIndex)) != 0) {
				errorf_old("tIndex read error: %d", c);
				fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			while ((c = getc(tIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in tIndex");
					fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				if (fseek(tIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
			}
		} else if (link1->ull == tnum) {
			if ((c = null_fgets(buf, MAX_ALIAS*4, tIndex)) != 0) {
				errorf_old("tIndex read error: %d", c);
				fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			if (rettagname) {
				link3 = link3->next = malloc(sizeof(oneslnk));
				if (!(link3->str = dupstr(buf, MAX_ALIAS*4, 0))) {
					errorf_old("failed to duplicate buf: %s", buf);
				}
			}
			if (retfnums) {
				if ((link5 = link5->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed for link5");
					fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				if ((tlink = link5->vp = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed for link5->vp");
					fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
					return 1;
				}
				while (1) {
					tnum = fgetull_pref(tIndex, &c);
					if (c != 0) {
						if (c == 4) {
							break;
						}
						errorf_old("tIndex tag read failed: %d", c);
						tlink->next = 0, fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0, tlink->next = 0;
						return 1;
					} tlink = tlink->next = malloc(sizeof(oneslnk));
					tlink->ull = tnum;
				}
				tlink->next = 0, tlink = link5->vp, link5->vp = tlink->next, free(tlink);
			} else {
				while ((c = getc(tIndex)) != 0) {
					if (c == EOF) {
						errorf("EOF before end of tags in tIndex");
						fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
						return 1;
					}
					if (fseek(tIndex, c, SEEK_CUR)) {
						errorf("fseek failed");
						fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
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
		errorf_old("tried to read non-existent tagnum: %d -- last read %d", link1->ull, tnum);
		presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
		return 1;
	}
	presort ? 0:killoneschn(tagnums, 1);
	
	if (rettagname) {
		link3->next = 0;
		*rettagname = link2->next;
		free(link2);
	} if (retfnums) {
		link5->next = 0;
		*retfnums = link4->next;
		free(link4);
	}
	return 0;
}

char twowaytcregaddrem(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *regtagnames, oneslnk *remtagnums, unsigned char presort) { // presort for 1: fnums, 2: addtagnums, 4: remtagnums	//! add presort functionality
	oneslnk *tnums, *link1, *link2;
	char buf1[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], buf4[MAX_PATH*4];
	int c;
	FILE *file1, *file2;
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 1;
	}
	if (!fnums) {
		errorf("no fnums");
		return 1;
	}
	if (!(addtagnums || regtagnames || remtagnums)) {
		errorf("no addtagnums, remtagnums or regtagnames");
		return 1;
	}

	sprintf(buf1, "%s\\i\\%llu\\tIndex.bin", g_prgDir, dnum);
	sprintf(buf2, "%s\\i\\%llu\\tIndex.bak2", g_prgDir, dnum);
	
	
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

	sprintf(buf3, "%s\\i\\%llu\\tAlias.bin", g_prgDir, dnum);
	sprintf(buf4, "%s\\i\\%llu\\tAlias.bak2", g_prgDir, dnum);
	
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
	if (tcregaddrem(dnum, fnums, addtagnums, regtagnames, &link1, remtagnums)) {
		errorf("tcregaddrem failed");
		MBremove(buf2), MBremove(buf4);
		return 1;		
	}
	if (addtagnums) {
		tnums = copyoneschn(addtagnums, 1);
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
	if (addremfnumctagc(dnum, fnums, tnums, remtagnums)) {
		errorf("addremfnumctagc failed");
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
	
	return 0;
}

//}