//functions dealing with file entries

//to do: directory index record

#include "fentries.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "stringchains.h"
#include "arrayarithmetic.h"
#include "bytearithmetic.h"
#include "portables.h"
#include "tfiles.h"
#include "ioextras.h"
#include "dupstr.h"
#include "breakpath.h"
#include "dirfiles.h"

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

#include "errorf.h"
#define errorf(...) g_errorf(__VA_ARGS__)

FILE *MBfopen(char*, char*);

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

struct arg_struct1 {
	char *fileStrBase;
	uint64_t (*getlastentrynum)(void *args);
	void *getlastnum_args;
	char (*crentryreg)(void *args, twoslnk *regchn); // reverse entry reg
	void *crentryreg_args;
	unsigned char (*addtolastentrynum)(void *args, long long num, uint64_t spot);
	void *addtolastentrynum_args;
};


//{ general
int cregformat1(oneslnk *inputchn, struct arg_struct1 *args) {
	FILE *indexF;
	//dIndex
	unsigned char buf[MAX_PATH*4]; //!
	int c, i, j;
	uint64_t lastentrynum = 0;
	//lastdnum = 0;
	oneslnk *link;
	long int maxentrylen = MAX_PATH*4;
	
	char *fileStrBase = args->fileStrBase;
	uint64_t (*getlastentrynum)(void *args) = args->getlastentrynum;
	void *getlastnum_args = args->getlastnum_args;
	char (*crentryreg)(void *args, twoslnk *regchn) = args->crentryreg; // reverse entry reg
	void *crentryreg_args = args->crentryreg_args;
	unsigned char (*addtolastentrynum)(void *args, long long num, uint64_t spot) = args->addtolastentrynum;
	void *addtolastentrynum_args = args->addtolastentrynum_args;
	
	//! checks
	
	
	if (inputchn == 0) {
		errorf("inputchn is null");
		return 1;
	}
	
	for (link = inputchn; link != 0; link = link->next) {
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
	
	char created = 0;
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, fileStrBase);
	if ((indexF = MBfopen(buf, "rb+")) == NULL) {
		if ((indexF = MBfopen(buf, "wb+")) == NULL) {
			errorf("couldn't create indexF");
			errorf("tried to create: \"%s\"", buf);
			return 1;
		}
		created = 1;
	}
	
	//if (lastdnum = getlastdnum()) {
	if ( (!created) && (getlastentrynum != NULL) && ((lastentrynum = getlastentrynum(getlastnum_args)) > 0) ) {
		fseek(indexF, 0, SEEK_END);
	} else {
		while ((c = getc(indexF)) != EOF) {
			if (fseek(indexF, c, SEEK_CUR)) {
				errorf("fseek failed");
				fclose(indexF);
				return 0;
			}
			
			if ((c = null_fgets(0, maxentrylen, indexF)) != 0) {
				errorf("indexF read error: %d", c);
				fclose(indexF);
				return 0;
			}
			lastentrynum++;		// assuming there are no entry number gaps
		}
	}
	
	twoslnk *numentrychn, *fnumentry;
	
	fnumentry = numentrychn = malloc(sizeof(twoslnk));
	numentrychn->u[0].str = 0;
	
	for (i = 0, link = inputchn; link != 0; link = link->next, i++) {
		putull_pref(++lastentrynum, indexF);
		term_fputs(link->str, indexF);
		numentrychn = numentrychn->next = malloc(sizeof(twoslnk));
		numentrychn->u[0].str = link->str, numentrychn->u[1].ull = lastentrynum;
	}
	fclose(indexF);
	numentrychn->next = 0;
	numentrychn = fnumentry->next, free(fnumentry);
	if (crentryreg != NULL) {
		crentryreg(crentryreg_args, numentrychn);
	}
	killtwoschn(numentrychn, 3);
	//addtolastdnum(i, 0);
	if (addtolastentrynum != NULL) {
		addtolastentrynum(addtolastentrynum_args, i, 0);
	}
	
	return 0;
}

uint64_t funcarg_entryinitpos(uint64_t (*entryinitpos)(uint64_t), uint64_t ientrynum) {
	return entryinitpos(ientrynum);
}

uint64_t funcarg_getlastentrynum(uint64_t (*getlastentrynum)(void)) {
	return getlastentrynum();
}

char funcarg_crentryreg(char (*crentryreg)(twoslnk *), twoslnk *regchn) {
	return crentryreg(regchn);
}

unsigned char funcarg_addtolastentrynum(unsigned char (*addtolastentrynum)(long long num, uint64_t spot), long long num, uint64_t spot) {
	return addtolastentrynum(num, spot);
}
	
//}

//{ main index 2nd layer

unsigned char raddtolastminum(long long num) { // 0 to just refresh
	char buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], ch;
	int i, c;
	FILE *rirec, *trirec, *rIndex;
	uint64_t inum, gap, pos = 0;
	
	char *rindexStrBase = "rmiIndex";
	char *rrecStrBase = "rmiRec";
	char *indexStrBase = "miIndex";

	sprintf(buf, "%s\\%s.tmp", g_prgDir, rrecStrBase);
	if ((trirec = MBfopen(buf, "wb")) == NULL) {
		errorf("failed to create trirec (raddtolastminum)");
		errorf("tried to create: %s", buf);
		return 1;
	}
	sprintf(buf2, "%s\\%s.bin", g_prgDir, rrecStrBase);
	sprintf(buf3, "%s\\%s.bin", g_prgDir, indexStrBase);

	if (((rirec = MBfopen(buf2, "rb")) != NULL) && ((i = getc(rirec)) != EOF)) {
		uint64_t oldinum = 0;
		
		for (; i > 0; i--) {
			oldinum *= 256;
			oldinum += c = getc(rirec);
			if (c == EOF) {
				fclose(rirec), fclose(trirec), MBremove(buf), MBremove(buf2);
				errorf("EOF before end of number in rirec");
				return 2;
			}
		}
		if ((num < 0) && (oldinum <= -num)) {
			fclose(rirec), fclose(trirec), MBremove(buf), MBremove(buf2);
			errorf("lastinum going less than one");
			return 6;
		}
		inum = oldinum + num;
		putull_pref(inum, trirec);
		
		gap = (long long) sqrt(inum);
		fclose(rirec), MBremove(buf2);
	} else {
		if (rirec != NULL)
			fclose(rirec);
		MBremove(buf2);
		
		if ((rIndex = MBfopen(buf3, "rb")) == NULL) {
			fclose(trirec), MBremove(buf);
			errorf("rIndex file not found");
			return 3;
		}
		inum = 0;
		while (1) {
			if ((c = null_fgets(0, MAX_PATH*4, rIndex)) != 0) {
				if (c == 1) {
					break;
				} else {
					errorf("null_fgets: %d", c);
					fclose(trirec), MBremove(buf), fclose(rIndex);
					return 5;
				}
			}
			if ((c = pref_fgets(0, 9, rIndex)) != 0) {
				errorf("EOF before end of number (raddtolastminum)(1)");
				fclose(trirec), MBremove(buf), fclose(rIndex);
				return 2;
			}
			inum++;
		}
		fclose(rIndex);
		
		putull_pref(inum, trirec);
		gap = sqrt(inum);
	}
	
	if ((rIndex = MBfopen(buf3, "rb")) == NULL) {
		fclose(trirec), MBremove(buf);
		errorf("rIndex file not found");
		return 3;
	}
	char buf4[MAX_PATH*4], *pbuf, *prevpbuf, *pbufhold;
	pbuf = buf3, prevpbuf = buf4;
	
	for (pos = ftell64(rIndex), inum = 1; 1; pos = ftell64(rIndex), inum++, pbufhold = prevpbuf, prevpbuf = pbuf, pbuf = pbufhold) {
		if ((c = null_fgets(pbuf, MAX_PATH*4, rIndex)) != 0) {
			if (c == 1) {
				break;
			}
			errorf("EOF before end of number in rIndex (2)");
			fclose(trirec), MBremove(buf), fclose(rIndex);
			return 2;
		}
		if (c == 2) {
			errorf("path too long rIndex");
			return 2;
		}
		if (inum % gap == 0) {
			i = 0;
			do {
				putc(pbuf[i++], trirec);
			} while (pbuf[i-1] == prevpbuf[i-1]);
			putc('\0', trirec);
			
			putull_pref(pos, trirec);
		}
		
		if ((c = pref_fgets(0, 9, rIndex)) != 0) {
			errorf("EOF before end of number (raddtolastminum)(2)");
			fclose(trirec), MBremove(buf), fclose(rIndex);
			return 2;
		}
	}
	fclose(rIndex), fclose(trirec);
	MBrename(buf, buf2);
	return 0;
}
/* 

uint64_t rmiinitpos___(char *idir) {
	char buf[MAX_PATH*4], ch;
	int i, c, remade = 0;
	uint64_t dnum, pos = 0, lastdnum = 0, gotdnum;
	FILE *rdirec;
	
	sprintf(buf, "%s\\rdiRec.bin", g_prgDir);
	
	if (((rdirec = MBfopen(buf, "rb")) == NULL) || ((i = getc(rdirec)) == EOF)) {
		if (rdirec != NULL)
			fclose(rdirec);
		raddtolastdnum(0);
		remade = 1;
		if (((rdirec = MBfopen(buf, "rb")) == NULL) || ((i = getc(rdirec)) == EOF)) {
			if (rdirec != NULL)
				fclose(rdirec);
			errorf("couldn't access rdirec");
			return 0;
		}		
	}
	for (; i > 0; i--) {
		lastdnum *= 256;
		lastdnum += c = getc(rdirec);
		if (c == EOF) {
			errorf("EOF before end of number in rdiRec");
			fclose(rdirec);
			return 0;
		}
	}
	if (lastdnum != (gotdnum = getlastdnum())) {
		fclose(rdirec);
		if (!remade) {
			if (((rdirec = MBfopen(buf, "rb")) == NULL) || ((i = getc(rdirec)) == EOF)) {
				if (rdirec != NULL)
					fclose(rdirec);
				errorf("couldn't access rdirec (2)");
				return 0;
			}
			for (lastdnum = 0; i > 0; i--) {
				lastdnum *= 256;
				lastdnum += c = getc(rdirec);
				if (c == EOF) {
					errorf("EOF before end of number in rdiRec");
					fclose(rdirec);
					return 0;
				}
			}
			if (lastdnum != gotdnum) {
				errorf("different lastdnum in rdIndex -- 1");
				return 0;
			}
		} else {
		errorf("different lastdnum in rdIndex -- 2");
		return 0;
		}
	}
	
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, rdirec)) != 0) {
			if (c == 1)
				break;
			errorf("rdiRec num read error: %d", c);
			fclose(rdirec);
			return 0;
		}

		if ((strcmp(idir, buf)) < 0)
			break;

		if (pos = fgetull_pref(rdirec, &c) == 0) {
			errorf("rdiRec num read error %c", c);
			fclose(rdirec);
			return 0;
		}
	}
	fclose(rdirec);
	
	return pos;
}

*/

char rmiinit() {
	unsigned char buf[MAX_PATH*4], *tfilestr;
	FILE *rindexF, *indexF, *tfile;
	uint64_t cLen, ntfilesegs;
	twoslnk *flink, *link;
	int c, i;
	
	char *rindexStrBase = "rmiIndex";
	char *rrecStrBase = "rmiRec";
	char *indexStrBase = "miIndex";
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, rindexStrBase);
	MBremove(buf);
	sprintf(buf, "%s\\%s.bin", g_prgDir, rrecStrBase);
	MBremove(buf);
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, indexStrBase);
	if ((indexF = MBfopen(buf, "rb")) == NULL) {
		errorf("indexF file not found");
		errorf("looked for: %s", buf);
		return 1;
	}
	
	#define raddtolastinum(num) raddtolastminum(num)
	
	ntfilesegs = 0;
	if ((tfilestr = reservetfile()) == 0) {
		errorf("reservetfile failed");
		return 2;
	}
	
	link = flink = malloc(sizeof(twoslnk));
	flink->u[0].str = flink->u[1].str = 0, flink->next = 0;
	cLen = 0;

	while ((c = getc(indexF)) != EOF) {
		link = link->next = malloc(sizeof(twoslnk));
		link->next = 0, link->u[0].str = link->u[1].str = 0;
		
		link->u[0].str = malloc(c+1), link->u[0].str[0] = c;
		for (i = 1; i <= link->u[0].str[0]; i++) {
			link->u[0].str[i] = c = getc(indexF); 
			if (c == EOF) {
				errorf("EOF before end of number (rmiinit)");
				fclose(indexF), killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
				return 1;
			}
		}
		
		for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in indexF");
				fclose(indexF), killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
				return 1;
			}
			buf[i] = c;
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in indexF");
			fclose(indexF), killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
			return 1;
		}
		buf[i] = c;
		link->u[1].str = malloc(i+1);
		for (; i >= 0; link->u[1].str[i] = buf[i], i--);
		
		cLen++;
		if (cLen >= RD_MAX_CLEN) {
			link = flink, flink = flink->next, free(link);
			sorttwoschn(&flink, (int(*)(void*,void*))strcmp, 1, 0);
			tfile = opentfile(tfilestr, ntfilesegs++, "wb");
			for (link = flink; link != 0; link = link->next) {
				term_fputs(link->u[1].str, tfile);
				for (i = 0; i <= link->u[0].str[0]; putc(link->u[0].str[i], tfile), i++);
			}
			fclose(tfile);
			killtwoschn(flink, 0);
			link = flink = malloc(sizeof(twoslnk));
			flink->u[0].str = flink->u[1].str = 0, flink->next = 0;
			cLen = 0;
		}
	}
	fclose(indexF);
	
	if (cLen > 0) {
		link = flink, flink = flink->next, free(link);
		sorttwoschn(&flink, (int(*)(void*,void*))strcmp, 1, 0);
		tfile = opentfile(tfilestr, ntfilesegs++, "wb");
		for (link = flink; link != 0; link = link->next) {
			term_fputs(link->u[1].str, tfile);
			pref_fputs(link->u[0].str, tfile);
		}
		fclose(tfile);
	}
	killtwoschn(flink, 0);
	
	if (ntfilesegs > 1) {
		mergetfiles(tfilestr, ntfilesegs, 2, 2, (int(*)(unsigned char*,  unsigned char*)) strcmp, 1, 0);
		ntfilesegs = 1;
	}
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, rindexStrBase);
	
	if ((rindexF = MBfopen(buf, "wb")) == NULL) {
		errorf("couldn't create rindexF");
		releasetfile(tfilestr, ntfilesegs);
		return 1;
	}
	if ((tfile = opentfile(tfilestr, 0, "rb")) == NULL) {
		errorf("couldn't read temp file");
		fclose(rindexF), MBremove(buf), releasetfile(tfilestr, ntfilesegs);
		return 1;
	}
	
	while ((c = getc(tfile)) != EOF) {
		putc(c, rindexF);
	}
	fclose(rindexF), fclose(tfile);
	releasetfile(tfilestr, ntfilesegs);
	
	raddtolastinum(0);
	
	#undef raddtolastinum
	
	return 0;
}

/*
char rmireg___(char *dpath, uint64_t dnum) {
	FILE *rdIndex, *trdIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], bif[MAX_PATH*4], done = 0;
	int i, c, d;
	
	if (dnum == 0) {
		errorf("dnum in rdreg is 0");
		return 1;
	}
	
	sprintf(buf, "%s\\rdIndex.bin", g_prgDir);
	
	if (dpath) {
		for (i = 0; dpath[i] != '\0' && i != MAX_PATH*4; i++);
		if (i >= MAX_PATH*4) {
			errorf("path too long");
			MBremove(buf);
			return 1;
		}
	} if (i == 0 || dpath == 0) {
		errorf("empty string");
		MBremove(buf);
		return 1;
	}
	
	if ((rdIndex = MBfopen(buf, "rb")) == NULL) {
		if (rdinit())
			return 2;
		return 0;
	}
	sprintf(baf, "%s\\trdIndex.bin", g_prgDir);
	if ((trdIndex = MBfopen(baf, "wb")) == NULL) {
		fclose(rdIndex), MBremove(buf);
		return 3;
	}
	while ((c = getc(rdIndex)) != EOF) {
		bif[0] = c;
		for (i = 1; (bif[i] = c = getc(rdIndex)) != '\0'; i++) {
			if (c == EOF) {
				fclose(rdIndex), MBremove(buf), fclose(trdIndex), MBremove(baf);
				errorf("EOF before end of string");
				return 4;
			}
		}
		if (i >= MAX_PATH*4) {
			fclose(rdIndex), MBremove(buf), fclose(trdIndex), MBremove(baf);
			errorf("path longer than MAX_PATH");
			return 5;
		}
		if (!done && strcmp(dpath, bif) < 0) {
			term_fputs(dpath, trdIndex);
			putull_pref(dnum, trdIndex);		
			done = 1;
		}
		term_fputs(bif, trdIndex);
		putc(c = getc(rdIndex), trdIndex);
		for (i = 0; i < c; putc(d = getc(rdIndex), trdIndex), i++) {
			if (d == EOF) {
				fclose(rdIndex), MBremove(buf), fclose(trdIndex), MBremove(baf);
				errorf("EOF before end of pref-num");
				return 6;
			}
		}
	}
	fclose(rdIndex), MBremove(buf);
	if (!done) {
		term_fputs(dpath, trdIndex);
		putull_pref(dnum, trdIndex);
	}
	fclose(trdIndex);
	if (MBrename(baf, buf)) {
		MBremove(baf);
		errorf("rename failed");
		return 7;
	}
	
	raddtolastdnum(1);
	return 0;
}

uint64_t rmiread___(char *dpath) {		// reverse dread  //! untested
	uint64_t dnum = 0;
	FILE *rdIndex;
	unsigned char buf[MAX_PATH*4], *p;
	int c, i;
	
	sprintf(buf, "%s\\rdIndex.bin", g_prgDir);
	if ((rdIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("dIndex file not found");
		return 0;
	}
	
	fseek64(rdIndex, rdinitpos(dpath), SEEK_SET);
	
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, rdIndex)) != 0) {
			if (c == 1)
				break;
			errorf("null_fgets: %d", c);
			fclose(rdIndex);
			return 0;
		}
		
		if ((c = strcmp(dpath, buf)) > 0) {
			if ((i = pref_fgets(0, 9, rdIndex)) != 0) {
				errorf("pref_fgets: %d", i);
				fclose(rdIndex);
				return 0;
			}
		} else if (c == 0) {
			dnum = fgetull_pref(rdIndex, &c);
			fclose (rdIndex);
			if (c != 0) {
				errorf("rdindex read: %d", c);
				return 0;
			}
			return dnum;
		} else
			break;
	}
	fclose(rdIndex);
	return 0;
}

char rmirmv___(char *dpath) {  //! untested
	FILE *rdIndex, *trdIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], fin = 0;
	int c, i;
	
	sprintf(buf, "%s\\rdIndex.bin", g_prgDir);
	
	for (i = 0; dpath[i] != '\0' && i != MAX_PATH*4; i++);
	if (i >= MAX_PATH*4) {
		errorf("path too long");
		MBremove(buf);
		return 1;
	} if (i == 0 || dpath == 0) {
		errorf("empty string");
		MBremove(buf);
		return 1;
	}
	
	if ((rdIndex = MBfopen(buf, "rb")) == NULL) {
		MBremove(buf);
		errorf("rdIndex file not found");
		return 1;
	}
	sprintf(baf, "%s\\trdIndex.bin", g_prgDir);
	if ((trdIndex = MBfopen(baf, "wb")) == NULL) {
		fclose(rdIndex), MBremove(buf);
		return 3;
	}
	
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, rdIndex)) != 0) {
			if (c == 1)
				break;
			errorf("rdIndex read error: %d", c);
			fclose(rdIndex), MBremove(buf), fclose(trdIndex), MBremove(baf);
			return 2;
		}
		
		if (fin || (c = strcmp(dpath, buf)) > 0) {
			term_fputs(buf, trdIndex);
			for (c = getc(rdIndex), i = 1; i <= c; i++) {
				if (c == EOF) {
					errorf("EOF before end of num");
					fclose(rdIndex), MBremove(buf), fclose(trdIndex), MBremove(baf);
					return 2;
				} putc(c, trdIndex);
			}
		} else if (c == 0) {
			if ((pref_fgets(0, 9, rdIndex)) != 0) {
				errorf("EOF before end of number");
			}
			fin = 1;
		} else
			break;
	}
	fclose(rdIndex), fclose(trdIndex), MBremove(buf);
	if (fin) {
		raddtolastdnum(-1);
		MBrename(baf, buf);
		return 0;
	}
	MBremove(baf);
	return 4;
}

char rmirer___(uint64_t source, char *dest) {
	FILE *rdIndex, *trdIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], bef[MAX_PATH*4], fin1 = 0, fin2 = 0;
	int c, i;
	uint64_t dnum = 0;
	
	sprintf(bef, "%s\\rdIndex.bin", g_prgDir);
	
	for (i = 0; dest[i] != '\0' && i != MAX_PATH*4; i++);
	if (i >= MAX_PATH*4) {
		errorf("path too long");
		MBremove(bef);
		return 1;
	} if (i == 0 || dest == 0) {
		errorf("empty string");
		MBremove(bef);
		return 1;
	}
	
	if ((rdIndex = MBfopen(bef, "rb")) == NULL) {
		MBremove(bef);
		errorf("rdIndex file not found");
		return 1;
	}
	sprintf(baf, "%s\\trdIndex.bin", g_prgDir);
	if ((trdIndex = MBfopen(baf, "wb")) == NULL) {
		errorf("couldn't create trdIndex");
		fclose(rdIndex), MBremove(bef);
		return 3;
	}
	
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, rdIndex)) != 0) {
			if (c == 1)
				break;
			errorf("rdIndex read error: %d", c);
			fclose(rdIndex), MBremove(bef), fclose(trdIndex), MBremove(baf);
			return 2;
		}
		dnum = fgetull_pref(rdIndex, &c);
		if (c != 0) {
			errorf("rdIndex read error: %d", c);
			fclose(rdIndex), MBremove(bef), fclose(trdIndex), MBremove(baf);
			return 2;
		}
		
		if (!fin2 && strcmp(dest, buf) < 0) {
			fin2 = 1;
			term_fputs(dest, trdIndex);
			putull_pref(source, trdIndex);
		}
		
		if (fin1 || dnum != source) {
			term_fputs(buf, trdIndex);
			putull_pref(dnum, trdIndex);
		} else if (dnum == source) {
			fin1 = 1;
		} else
			break;
	}
	fclose(rdIndex), MBremove(bef);
	if (fin1) {
		if (!fin2) {
			term_fputs(dest, trdIndex);
			putull_pref(source, trdIndex);
		}
		fclose(trdIndex);
		MBrename(baf, bef);
		return 0;
	}
	fclose(trdIndex);
	MBremove(baf);
	return 4;
}

char rmirmvnum___(uint64_t dnum) {		//! not done
	rdinit();
	return 1;
}

*/

char crmireg(twoslnk *regchn) {		//! not done
	rmiinit();
	return 1;
}

/*

char crmirer___(oneslnk *rmchn, twoslnk *addchn) {		//! not done // assuming only one entry per dname, easily fixed by making rmchn twoslnk with dnum as well
	rdinit();
	return 1;
}

char crmirmvnum___(oneslnk *rmchn) {		//! not done
	rdinit();
	return 1;
}

char mifrmv___(uint64_t dnum) {		//! not done
	rdinit();
	return 1;
}

char cmifrmv___(oneslnk *dnumchm) {		//! not done
	rdinit();
	return 1;
}

char passmiextra___(FILE *src, FILE *dst, uint64_t ull) {
	int c;
	uint64_t ull2;
	if (ull == 0) {
		ull2 = fgetull_pref(src, &c);
		if (c != 0) {
			errorf("4 - dExtras num read failed: %d", c);
			return 1;
		}
		if (dst)
			putull_pref(ull2, dst);
		return 0;
	} else {
		errorf("unknown dExtras parameter: %llu", ull);
		return 1;
	}
}

char setmilastchecked___(uint64_t dnum, uint64_t ulltime) {		// no record for offsets in file	//! not extensively tested
	uint64_t tnum, tnum2;
	FILE *dExtras, *tdExtras;
	int c, i;
	unsigned char fin = 0;
	char buf[MAX_PATH*4], buf2[MAX_PATH*4];
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 1;
	}
	ulltime = time(0);
	
	sprintf(buf, "%s\\dExtras.bin", g_prgDir);
	if ((dExtras = MBfopen(buf, "rb")) == NULL) {
		if ((dExtras = MBfopen(buf, "wb+")) == NULL) {
			errorf("couldn't create dExtras.bin");
			return 1;
		}
	}
	sprintf(buf2, "%s\\dExtras.tmp", g_prgDir);
	if ((tdExtras = MBfopen(buf2, "wb+")) == NULL) {
		errorf("couldn't create dExtras.tmp");
		fclose(dExtras);
		return 1;
	}
	while (dExtras) {
		tnum = fgetull_pref(dExtras, &c);
		if (c != 0) {
			if (c == 1) {
				fclose(dExtras);
				dExtras = NULL;
				break;
			}
			errorf("dExtras num read failed: %d", c);
			fclose(dExtras), fclose(tdExtras), MBremove(buf2);
			return 1;
		}
		if (!fin && tnum > dnum) {
			putull_pref(dnum, tdExtras);
			putull_pref(0, tdExtras);
			putull_pref(ulltime, tdExtras);
			putc(0, tdExtras);
			fin = 1;
		}
		putull_pref(tnum, tdExtras);
		
		while (1) {
			tnum2 = fgetull_pref(dExtras, &c);
			if (c != 0) {
				if (c == ULL_READ_NULL)
					break;
				errorf("2 - dExtras num read failed: %d", c);
				fclose(dExtras), fclose(tdExtras), MBremove(buf2);
				return 1;
			}
			if (!fin && tnum == dnum && tnum2 >= 0) {
				putull_pref(0, tdExtras);
				if (tnum2 == 0) {
					fgetull_pref(dExtras, &c);
					if (c != 0) {
						errorf("3 - dExtras num read failed: %d", c);
						fclose(dExtras), fclose(tdExtras), MBremove(buf2);
						return 1;
					}
				}
				putull_pref(ulltime, tdExtras);
				fin = 1;
			} else {
				putull_pref(tnum2, tdExtras);
				if (passdextra(dExtras, tdExtras, tnum2)) {
					errorf("passdextras failed: %d", c);
					fclose(dExtras), fclose(tdExtras), MBremove(buf2);
					return 1;
				}
			}
		}
		if (!fin && tnum == dnum) {
			putull_pref(0, tdExtras);
			putull_pref(ulltime, tdExtras);
			fin = 1;			
		} putc('\0', tdExtras);
	}
	if (!fin) {
		putull_pref(dnum, tdExtras);
		putull_pref(0, tdExtras);
		putull_pref(ulltime, tdExtras);
		putc('\0', tdExtras);
	}
	
	fclose(tdExtras);
	
	char buf3[MAX_PATH*4];
	sprintf(buf3, "%s\\dExtras.bak", g_prgDir);
	MBremove(buf3);
	if (MBrename(buf, buf3)) {
		errorf("rename1 failed");
		errorf("%s->%s", buf, buf3);
		return 3;
	}
	if (MBrename(buf2, buf)) {
		MBrename(buf3, buf);
		errorf("rename2 failed");
		return 3;
	}
	MBremove(buf3);
	
	return 0;
}

uint64_t getmilastchecked___(uint64_t dnum) {
	uint64_t ulltime, tnum, tnum2;
	FILE *dExtras;
	int c, i;
	char buf[MAX_PATH*4];
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 0;
	}
	ulltime = time(0);
	
	sprintf(buf, "%s\\dExtras.bin", g_prgDir);
	if ((dExtras = MBfopen(buf, "rb")) == NULL) {
		// errorf("couldn't open dExtras");
		return 0;
	}
	while (1) {
		tnum = fgetull_pref(dExtras, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf("dExtras num read failed: %d", c);
			fclose(dExtras);
			return 0;
		}
		while (1) {
			tnum2 = fgetull_pref(dExtras, &c);
			if (c != 0) {
				if (c == 4)
					break;
				errorf("2 - dExtras num read failed: %d", c);
				fclose(dExtras);
				return 0;
			}
			if (tnum == dnum && tnum2 == 0) {
				tnum2 = fgetull_pref(dExtras, &c);
				fclose(dExtras);
				if (c != 0) {
					errorf("3 - dExtras num read failed: %d", c);
					return 0;
				}
				return tnum2;
			} else {
				if (tnum2 == 0) {
					fgetull_pref(dExtras, &c);
					if (c != 0) {
						errorf("4 - dExtras num read failed: %d", c);
						fclose(dExtras);
						return 0;
					}
				} else {
					errorf("unknown dExtras parameter (getdlast): %llu", tnum2);
					fclose(dExtras);
					return 0;
				}
			}
		}
	} fclose(dExtras);
	
	return 0;
}
 */
 
//}

//{ main index
unsigned char addtolastminum(long long num, uint64_t spot) { // 0 as num to just refresh from spot //! generalize
	char buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], ch;
	int i, c;
	FILE *irec, *tirec, *indexF;
	uint64_t inum = 0, gap = 0, pos = 0;
	
	char *indexStrBase = "dIndex";
	char *recStrBase = "diRec";

	sprintf(buf, "%s\\%s.tmp", g_prgDir, recStrBase);
	if ((tirec = MBfopen(buf, "wb")) == NULL) {
		errorf("failed to create tirec");
		errorf("tried to create: \"%s\"", buf);
		return 1;
	}
	sprintf(buf2, "%s\\%s.bin", g_prgDir, recStrBase); // rec
	sprintf(buf3, "%s\\%s.bin", g_prgDir, indexStrBase); // index
	

	if (((irec = MBfopen(buf2, "rb")) != NULL) && ((i = getc(irec)) != EOF)) {
		uint64_t oldinum = 0;
		
		for (; i > 0; i--) {
			oldinum *= 256;
			oldinum += c = getc(irec);
			if (c == EOF) {
				fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
				errorf("EOF before end of number in irec");
				return 2;
			}
		}
		if ((num < 0) && (oldinum <= -num)) {
			fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
			errorf("lastinum going less than one");
			return 6;
		}
		inum = oldinum + num;
		putull_pref(inum, tirec);
		
		if ((gap = (long long) sqrt(inum)) == (long long) sqrt(oldinum)) {
			if (num > 0) {
				if (inum / gap == oldinum / gap) {
					while ((c = getc(irec)) != EOF) {
						putc(c, tirec);
					} fclose(irec), MBremove(buf2);
				} else {
					while ((c = getc(irec)) != EOF) {
						for (putc(c, tirec), i = c; i > 0 && (c = getc(irec)) != EOF; i--, putc(c, tirec));
						for (putc((ch = getc(irec)), tirec), i = 1, pos = 0; i++ <= ch && (c = getc(irec)) != EOF; pos *= 256, pos += c, putc(c, tirec));
						if (c == EOF) {
							errorf("EOF before finished irec entry (1)");
							fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
							return 1;
						}
					} fclose(irec), MBremove(buf2);
					
					if ((indexF = MBfopen(buf3, "rb")) != NULL) {
						fseek64(indexF, pos, SEEK_SET);
						fseek(indexF, (c = getc(indexF)), SEEK_CUR);
						if (c == EOF) {
							errorf("EOF before irectory in indexF");
							fclose(indexF), fclose(tirec), MBremove(buf);
							return 1;
						}
						null_fgets(0, MAX_PATH*4, indexF);
						
						for (pos = ftell64(indexF), inum = 0; (i = getc(indexF)) != EOF; pos = ftell64(indexF), inum = 0) {
							for (; i > 0; i--) {
								inum *= 256;
								inum += c = getc(indexF);
								if (c == EOF) {
									errorf("EOF before end of number in indexF (1)");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								}
							}
							if (inum % gap == 0) {
								putull_pref(inum, tirec);
								putull_pref(pos, tirec);
							}
							
							if ((c = null_fgets(0, MAX_PATH*4, indexF)) != 0) {
								if (c == 1) {
									errorf("EOF before null terminator in indexF");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								} else if (c == 2) {
									errorf("too long string in indexF");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								}
							}
						}
						fclose(indexF);
					} else {
						errorf("Couldn't open indexF");
						fclose(tirec), MBremove(buf);
						return 1;
					}
				}
				fclose(tirec), MBrename(buf, buf2);
				return 0;
			} else {
				oldinum = inum;
				uint64_t tpos;
				while ((c = getc(irec)) != EOF) {
					for (ch = c, i = 1, inum = 0; i++ <= ch && (c = getc(irec)) != EOF; inum *= 256, inum += c);
					for (ch = getc(irec), i = 1, tpos = 0; i++ <= ch && (c = getc(irec)) != EOF; tpos *= 256, tpos += c);
					if (c == EOF) {
						errorf("EOF before finished irec entry (2)");
						fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
						return 1;
					}
					if (inum > spot || inum > oldinum) {
						break;
					}
					pos = tpos;
					putull_pref(inum, tirec);
					putull_pref(pos, tirec);
				}
			}
		}
		fclose(irec), MBremove(buf2);
		
	} else {
		if (irec != NULL) {
			fclose(irec);
		}
		MBremove(buf2);
		
		if ((indexF = MBfopen(buf3, "rb")) == NULL) {	// it is valid for the index file not to exist
			fclose(tirec), MBremove(buf);
			// errorf("indexF file not found (addtolastinum 1)");
			// errorf("looked for file: %s", buf3);
			return 3;
		}
		inum = 0;
		while ((i = getc(indexF)) != EOF) {
			for (inum = 0; i > 0; i--) {
				inum *= 256;
				inum += c = getc(indexF);
				if (c == EOF) {
					errorf("EOF before end of number (addtolastminum)");
					fclose(tirec), MBremove(buf), fclose(indexF);
					return 2;
				}
			}
			for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
				if (c == EOF) {
					errorf("EOF before null terminator in indexF");
					fclose(tirec), MBremove(buf), fclose(indexF);
					return 4;
				}
			}
			if (i == MAX_PATH*4) {
				errorf("too long string in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 5;
			}
		}
		fclose(indexF);
		
		if (inum == 0) {
			fclose(tirec), MBremove(buf);
			return 3;
		}
		
		putull_pref(inum, tirec);
		gap = sqrt(inum);
	}
	
	if ((indexF = MBfopen(buf3, "rb")) == NULL) {
		fclose(tirec), MBremove(buf);
		errorf("indexF file not found (addtolastinum 2)");
		errorf("looked for file: %s", buf3);
		return 3;
	}
	if (pos) {
		fseek64(indexF, pos, SEEK_SET);
		fseek(indexF, (c = getc(indexF)), SEEK_CUR);
		
		for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 4;
			}
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in indexF");
			fclose(tirec), MBremove(buf), fclose(indexF);
			return 5;
		}
	}
	for (pos = ftell64(indexF), inum = 0; (i = getc(indexF)) != EOF; pos = ftell64(indexF), inum = 0) {
		for (; i > 0; i--) {
			inum *= 256;
			inum += c = getc(indexF);
			if (c == EOF) {
				errorf("EOF before end of number in indexF (2)");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 2;
			}
		}
		if (inum % gap == 0) {
			putull_pref(inum, tirec);
			putull_pref(pos, tirec);
		}
		
		for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 2;
			}
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in indexF");
			fclose(tirec), MBremove(buf), fclose(indexF);
			return 2;
		}
	}
	fclose(indexF), fclose(tirec);
	MBrename(buf, buf2);
	return 0;
}

uint64_t getlastminum(void) { //! generalize
	char buf[MAX_PATH*4];
	int i, c;
	uint64_t lastinum = 0;
	FILE *file;
	
	
	char *fileStrBase = "miRec";
	unsigned char (*addtolastinum)(long long num, uint64_t spot) = addtolastminum;
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, fileStrBase);
	
	if (((file = MBfopen(buf, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
		if (file != NULL) {
			fclose(file);
		}
		if ((c = addtolastinum(0, 0)) != 0) {
			if (c != 3) { // unless index file doesn't exist
				errorf("addtolastinum failed");
			}
			return 0;
		}
		if (((file = MBfopen(buf, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
			if (file != NULL) {
				fclose(file);
			}
			errorf("couldn't access irec (getlastminum)");
			return 0;
		}		
	}
	for (; i > 0; i--) {
		lastinum *= 256;
		lastinum += c = getc(file);
		if (c == EOF) {
			errorf("EOF before end of number in irec");
			fclose(file);
			return 0;
		}
	}
	fclose(file);
	return lastinum;
}

uint64_t miinitpos(uint64_t iinum) {
	char buf[MAX_PATH*4], ch;
	int i, c;
	uint64_t inum, pos = 0;
	FILE *irec;
	
	char *fileStrBase = "miRec";
	unsigned char (*addtolastinum)(long long num, uint64_t spot) = addtolastminum;
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, fileStrBase);
	
	if (((irec = MBfopen(buf, "rb")) == NULL) || ((i = getc(irec)) == EOF)) {
		if (irec != NULL)
			fclose(irec);
		addtolastinum(0, 0);
		if (((irec = MBfopen(buf, "rb")) == NULL) || ((i = getc(irec)) == EOF)) {
			if (irec != NULL)
				fclose(irec);
			errorf("couldn't access irec (miinitpos)");
			return 0;
		}		
	}
	fseek(irec, i, SEEK_CUR);
	while ((c = getc(irec)) != EOF) {
		for (ch = c, i = 1, inum = 0; i <= ch && (c = getc(irec)) != EOF; i++, inum *= 256, inum += c);
		if (inum > iinum)
			break;
		for (ch = c = getc(irec), i = 1, pos = 0; i <= ch && (c = getc(irec)) != EOF; i++, pos *= 256, pos += c);
		if (c == EOF) {
			errorf("EOF before finished irec entry (miinitpos)");
			fclose(irec);
			return 0;
		}
	}
	fclose(irec);
	
	return pos;
}

uint64_t mireg(char *miname) {
	int c;
	oneslnk *link = malloc(sizeof(oneslnk));
	link->next = NULL;
	link->str = miname;
	
	c = cmireg(link);
	free(link);
	return c;
}

char *miread(uint64_t minum) {	// returns malloc memory
	unsigned char buf[MAX_PATH*4], *p;
	int c, i;
	uint64_t tnum;
	FILE *indexF;
	
	uint64_t entrynum = minum;
	char *fileStrBase = "miIndex";
	#define entryinitpos(entrynum) miinitpos(entrynum)
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, fileStrBase);
	if ((indexF = MBfopen(buf, "rb")) == NULL) {
		errorf("index file not found (read)");
		errorf("looked for: %s", buf);
		return 0;
	}
	
	fseek64(indexF, entryinitpos(entrynum), SEEK_SET);
	#undef entryinitpos
	
	while (1) {
		tnum = fgetull_pref(indexF, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf("indexF num read failed: %d", c);
			fclose(indexF);
			return 0;
		}
		if (entrynum > tnum) {
			if ((c = null_fgets(0, MAX_PATH*4, indexF)) != 0) {
				errorf("indexF read error: %d", c);
				fclose(indexF);
				return 0;
			}
		} else if (entrynum == tnum) {
			if ((c = null_fgets(buf, MAX_PATH*4, indexF)) != 0) {
				errorf("indexF read error: %d", c);
				fclose(indexF);
				return 0;
			}
			fclose(indexF);
			if ((p = malloc(strlen(buf)+1)) == 0) {
				errorf("malloc failed");
				return p;
			}
			for (i = 0; p[i] = buf[i]; i++);
			return p;
		} else
			break;
	}
	fclose(indexF);
	return 0;
}

// char mirmv___(uint64_t dnum) {
	// int c;
	// oneslnk *link = malloc(sizeof(oneslnk));
	// link->next = NULL;
	// link->ull = dnum;
	
	// c = cdrmv(link);
	// free(link);
	// return c;
// }

// char mirer___(uint64_t dnum, char *dpath) {		// reroute
	// FILE *dIndex, *tIndex;
	// unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], fin = 0;
	// int c, i, j;
	// uint64_t tnum;
	
	// for (i = 0; dpath[i] != '\0' && i != MAX_PATH*4; i++);
	// if (i >= MAX_PATH*4) {
		// errorf("path too long");
		// return 3;
	// } if (i == 0) {
		// errorf("empty string");
		// return 3;
	// }
	
	// sprintf(buf, "%s\\dIndex.bin", g_prgDir);
	// if ((dIndex = MBfopen(buf, "rb")) == NULL) {
		// errorf("dIndex file not found");
		// return 2;
	// }
	// sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
	// if ((tIndex = MBfopen(baf, "wb")) == NULL) {
		// errorf("couldn't create dIndex.tmp");
		// fclose(dIndex);
		// return 2;
	// }
	
	// while (1) {
		// tnum = fgetull_pref(dIndex, &c);
		// if (c != 0) {
			// if (c == 1)
				// break;
			// errorf("dIndex num read failed: %d", c);
			// fclose(dIndex);
			// return 0;
		// }
		// putull_pref(tnum, tIndex);
		
		// if (fin || dnum > tnum) {				
			// for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++)
				// if (c == EOF) {
					// errorf("EOF before null terminator in dIndex");
					// fclose(dIndex), fclose(tIndex);
					// return 2;
				// } else
					// putc(c, tIndex);
			// putc(c, tIndex);
			// if (i == MAX_PATH*4) {
				// errorf("too long string in dIndex");
				// fclose(dIndex), fclose(tIndex);
				// return 2;
			// }
		// } else if (dnum == tnum) {
			// for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++)
				// if (c == EOF) {
					// errorf("EOF before null terminator in dIndex");
					// fclose(dIndex), fclose(tIndex);
					// return 2;
				// }
			// if (i == MAX_PATH*4) {
				// errorf("too long string in dIndex");
				// fclose(dIndex), fclose(tIndex);
				// return 2;
			// }
			// for (i = 0; (putc(dpath[i], tIndex), dpath[i] != '\0') && i < MAX_PATH*4; i++);
			// if (i == MAX_PATH*4) {
				// errorf("too long string in dIndex");
				// fclose(dIndex), fclose(tIndex);
				// return 2;
			// }
			
			// while ((c = getc(dIndex)) != EOF) {
				// putc(c, tIndex);
			// }
			// fin = 1;
		// } else {
			// break;
		// }
	// }
	// fclose(dIndex), fclose(tIndex);
	
	// if (fin) {
		// sprintf(buf, "%s\\dIndex.bin", g_prgDir);
		// sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		// MBremove(baf);
		// if (MBrename(buf, baf)) {
			// errorf("rename1 failed");
			// return 3;
		// }
		// sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
		// if (MBrename(baf, buf)) {
			// sprintf(baf, "%s\\dIndex.bak", g_prgDir), MBrename(baf, buf);
			// errorf("rename2 failed");
			// return 3;
		// }
		// sprintf(baf, "%s\\dIndex.bak", g_prgDir);

		// rdrer(dnum, dpath);
		
		// addtolastdnum(0, dnum);
		// MBremove(baf);
		// return 0;
	// }
	
	// MBremove(baf);
	// return 1;		// entry number doesn't have entry
// }

int cmireg_(oneslnk *minamechn) {
	struct arg_struct1 format_args = {};
	
	format_args.fileStrBase = "miIndex";
	format_args.getlastentrynum = (uint64_t (*)(void *)) funcarg_getlastentrynum;
	format_args.getlastnum_args = getlastminum;
	format_args.crentryreg = NULL;
	format_args.crentryreg_args = NULL;
	//!
	//format_args.crentryreg = (char (*)(void *args, twoslnk *regchn)) funcarg_crentryreg;
	//format_args.crentryreg_args = crmireg;
	format_args.addtolastentrynum = (unsigned char (*)(void *args, long long num, uint64_t spot)) funcarg_addtolastentrynum;
	format_args.addtolastentrynum_args = addtolastminum;
	
	int c = cregformat1(minamechn, &format_args);
	
	return c;
}

int cmireg(oneslnk *minamechn) {
	long int maxentrylen = MAX_PATH*4;
	
	oneslnk *inputchn = minamechn;
	char *fileStrBase = "miIndex";
	#define getlastentrynum() getlastminum()
	#define addtolastinum(num1, num2) addtolastminum(num1, num2)
	#define crentryreg(numentrychn) crmireg(numentrychn)
	
	unsigned char buf[MAX_PATH*4];
	
	if (inputchn == 0) {
		errorf("inputchn is null");
		return 1;
	}
	
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
	
	FILE *indexF;
	char created = 0;
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, fileStrBase);
	if ((indexF = MBfopen(buf, "rb+")) == NULL) {
		if ((indexF = MBfopen(buf, "wb+")) == NULL) {
			errorf("couldn't create indexF");
			errorf("tried to create: \"%s\"", buf);
			return 1;
		}
		created = 1;
	}
	
	uint64_t lastentrynum = 0;
	
	//if (lastdnum = getlastdnum()) {
	if ( (!created) && ((lastentrynum = getlastentrynum()) > 0) ) {
		#undef getlastentrynum
		fseek(indexF, 0, SEEK_END);
	} else {
		int c;
		while ((c = getc(indexF)) != EOF) {
			if (fseek(indexF, c, SEEK_CUR)) {
				errorf("fseek failed");
				fclose(indexF);
				return 0;
			}
			
			if ((c = null_fgets(0, maxentrylen, indexF)) != 0) {
				errorf("indexF read error: %d", c);
				fclose(indexF);
				return 0;
			}
			lastentrynum++;		// assuming there are no entry number gaps
		}
	}
	
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
	crentryreg(numentrychn);
	#undef crentryreg
	killtwoschn(numentrychn, 3);
	addtolastinum(i, 0);
	#undef addtolastinum
	
	return 0;
}

oneslnk *cmiread(oneslnk *minumchn) {		//! not done
	
	return NULL; //!
}
/* 
int cmirmv___(oneslnk *dnumchn) {
	FILE *dIndex, *tIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4];
	int c, i, j;
	oneslnk *flink;
	uint64_t nremoved = 0, tnum;
	
	sprintf(buf, "%s\\dIndex.bin", g_prgDir);
	if ((dIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("dIndex file not found");
		return 2;
	}
	sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
	if ((tIndex = MBfopen(baf, "wb")) == NULL) {
		errorf("couldn't create dIndex.tmp");
		fclose(dIndex);
		return 2;
	}
	
	if (!(flink = dnumchn = copyoneschn(dnumchn, 1))) {
		errorf("copyoneschn failed");
		fclose(dIndex), fclose(tIndex), MBremove(baf);
		return 2;
	}
	if (sortoneschnull(flink, 0)) {
		errorf("sortoneschn failed");
		fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
		return 2;
	}
	if (dnumchn->ull == 0) {
		errorf("passed 0 dnum to cdrmv");
		fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
		return 2;
	}	
	
	while (1) {
		tnum = fgetull_pref(dIndex, &c);
		if (c != 0) {
			if (c == 1) {
				c = EOF;
				break;
			}
			errorf("dIndex num read failed: %d", c);
			fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
			return 0;
		}
		
		if (!(dnumchn) || tnum < dnumchn->ull) {
			tnum -= nremoved;
			putull_pref(tnum, tIndex);
			
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++) {
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
					return 2;
				} else
					putc(c, tIndex);
			}
			putc(c, tIndex);
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
				return 2;
			}
		} else if (tnum == dnumchn->ull) {
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
					return 2;
				}
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
				return 2;
			}
			nremoved++;
			dnumchn = dnumchn->next;
		} else {
			break;
		}
	}
	fclose(dIndex); fclose(tIndex);
	
	if (dnumchn == 0 && c == EOF) {
		sprintf(buf, "%s\\dIndex.bin", g_prgDir);
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		MBremove(baf);
		if (MBrename(buf, baf)) {
			errorf("rename1 failed");
			killoneschn(flink, 1);
			return 3;
		}
		sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
		if (MBrename(baf, buf)) {
			killoneschn(flink, 1);
			errorf("rename2 failed");
			return 3;
		}
				
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		if (cdfrmv(flink)) {
			MBremove(buf), MBrename(baf, buf);
			errorf("cdfrmv failed");
			return 4;
		}
		
		addtolastdnum(-nremoved, flink->ull);
		killoneschn(flink, 1);
		
		MBremove(baf);
		
		return 0;
	}
	MBremove(baf), killoneschn(flink, 1);
	return 1;
}

int cmirer___(twoslnk *rerchn) {		//! untested
	FILE *dIndex, *tIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], *numbuf;
	int c, i, j;
	twoslnk *flink;
	oneslnk *foslnk = 0, *oslnk;
	uint64_t tnum;
	
	sprintf(buf, "%s\\dIndex.bin", g_prgDir);
	if ((dIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("dIndex file not found");
		return 2;
	}
	sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
	if ((tIndex = MBfopen(baf, "wb")) == NULL) {
		errorf("couldn't create dIndex.tmp");
		fclose(dIndex);
		return 2;
	}
	
	if (!(flink = rerchn = copytwoschn(rerchn, 1))) {
		errorf("copytwoschn failed");
		fclose(dIndex), fclose(tIndex), MBremove(baf);
		return 2;
	}
	if (sorttwoschnull(&flink, 0, 0)) {
		errorf("sorttwoschn failed");
		fclose(dIndex), fclose(tIndex), killtwoschn(flink, 1), MBremove(baf);
		return 2;
	}
	
	if (rerchn->u[0].ull == 0) {
		errorf("passed null dnum to cdrer");
		fclose(dIndex), fclose(tIndex), killtwoschn(flink, 1), MBremove(baf);
		return 2;
	}
	foslnk = oslnk = malloc(sizeof(oneslnk));
	foslnk->str = 0, foslnk->next = 0;
	
	while (1) {		// no recourse if string is null
		tnum = fgetull_pref(dIndex, &c);
		if (c != 0) {
			if (c == 1) {
				c = EOF;
				break;
			}
			errorf("dIndex num read failed: %d", c);
			fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
			return 2;
		}
		putull_pref(tnum, tIndex);
		
		if (!(rerchn) || tnum < rerchn->u[0].ull) {
		
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
					return 2;
				} else
					putc(c, tIndex);
			putc(c, tIndex);
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
				return 2;
			}
			
		} else if (tnum == rerchn->u[0].ull) {
			if ((c = null_fgets(buf, MAX_PATH*4, dIndex)) != 0) {
				errorf("dIndex read error: %d", c);
				fclose(dIndex);
				return 2;
			}
			oslnk = oslnk->next = malloc(sizeof(oneslnk));
			oslnk->next = 0;
			if ((oslnk->str = malloc(strlen(buf)+1)) == 0) {
				errorf("malloc failed");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
				return 2;
			}
			for (i = 0; oslnk->str[i] = buf[i]; i++);
			
			if (rerchn->u[1].str == 0)
				break;
			
			for (i = 0; (putc(rerchn->u[1].str[i], tIndex), rerchn->u[1].str[i] != '\0') && i < MAX_PATH*4; i++);
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
				return 2;
			}			
			rerchn = rerchn->next;
		} else {
			break;
		}
	}
	fclose(dIndex); fclose(tIndex);
	
	if (rerchn == 0 && c == EOF) {
		sprintf(buf, "%s\\dIndex.bin", g_prgDir);
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		MBremove(baf);
		if (MBrename(buf, baf)) {
			errorf("rename1 failed");
			killtwoschn(flink, 1), killoneschn(foslnk, 1);
			return 3;
		}
		sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
		if (MBrename(baf, buf)) {
			killtwoschn(flink, 1), killoneschn(foslnk, 1);
			errorf("rename2 failed");
			return 3;
		}
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		
		oslnk = foslnk, foslnk = foslnk->next, free(oslnk);
		crdrer(foslnk, flink);
		
		addtolastdnum(0, flink->u[0].ull);
		MBremove(baf);
		
		killtwoschn(flink, 1), killoneschn(foslnk, 1);
		
		return 0;
	}
	MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
	return 1;
}

*/

oneslnk *imiread(uint64_t start, uint64_t intrvl) {	// returns malloc memory (the whole chain)
	unsigned char buf[MAX_PATH*4];
	int c, i;
	uint64_t tnum;
	oneslnk *flnk, *lastlnk;
	FILE *indexF;
				
	if (!intrvl) {
		errorf("imiread without intrvl");
		return 0;
	}
	
	uint64_t entrynum = start;
	char *fileStrBase = "miIndex";
	#define entryinitpos(ientrynum) miinitpos(ientrynum)
	
	sprintf(buf, "%s\\%s.bin", g_prgDir, fileStrBase);
	if ((indexF = MBfopen(buf, "rb")) == NULL) {
		//errorf("indexF file not found (imiread)");
		//errorf("looked for file: %s", buf);
		return 0;
	}
	
	fseek64(indexF, entryinitpos(entrynum), SEEK_SET);
	#undef entryinitpos
	
	while (1) {
		tnum = fgetull_pref(indexF, &c);
		if (c != 0) {
			if (c == 1) {
				break;
			}
			errorf("1 - indexF num read failed: %d", c);
			fclose(indexF);
			return 0;
		}
		
		if (tnum < start) {
			for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in indexF");
					fclose(indexF);
					return 0;
				}
			if (i == MAX_PATH*4) {
				errorf("too long string in indexF");
				fclose(indexF);
				return 0;
			}
		} else {			// found start entry or over
			if ((flnk = malloc(sizeof(oneslnk))) == 0) {
				errorf("malloc failed");
				fclose(indexF);
				return 0;
			} lastlnk = flnk;
			flnk->str = 0;

			while (intrvl > 0) {
				while (tnum > start) {
					if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
						errorf("malloc failed");
						lastlnk->next = 0;
						fclose(indexF), killoneschn(flnk, 0);
						return 0;
					}
					lastlnk->str = 0;
					start++;
					intrvl--;
					if (intrvl <= 0) {
						fclose(indexF);
						lastlnk->next = 0;
						lastlnk = flnk->next;
						free(flnk);
						return lastlnk;
					}
				}
				
				for (i = 0; (buf[i] = c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
					if (c == EOF) {
						errorf("EOF before null terminator in indexF");
						lastlnk->next = 0;
						fclose(indexF), killoneschn(flnk, 0);
						return 0;
					}
				}
				if (i == MAX_PATH*4) {
					errorf("too long string in indexF");
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
				
				if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed");
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
				lastlnk->str = malloc(i+1);
				snprintf(lastlnk->str, i+1, "%s", buf);
				start++;
				intrvl--;
				
				tnum = fgetull_pref(indexF, &c);
				if (c != 0) {
					if (c == 1)
						break;
					errorf("2 - indexF num read failed: %d", c);
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
			} fclose(indexF);
			
			lastlnk->next = 0;
			lastlnk = flnk->next;
			free(flnk);
			return lastlnk;
		}
	}
	fclose(indexF);
	return 0;
}

/*

void verDI___(VOID) { // check for order, duplicates, gaps, 0 character strings
	
}
 */
 
int existsmi(uint64_t minum) {
	 char *p = 0;
	 
	 if ( (p = miread(minum)) != NULL ) {
		 free(p);
		 return 1;
	 } else {
		 return 0;
	 }
}
 
//}

//{ dir 2nd layer

unsigned char raddtolastdnum(uint64_t minum, long long num) { // 0 to just refresh
	return 0;
	
	//! TODO: implement

	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	char buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], ch;
	int i, c;
	FILE *rdirec, *trdirec, *rdIndex;
	uint64_t dnum, gap, pos = 0;

	sprintf(buf, "%s\\rdiRec.tmp", g_prgDir);
	if ((trdirec = MBfopen(buf, "wb")) == NULL) {
		errorf("failed to create diRec.tmp");
		return 1;
	}
	sprintf(buf2, "%s\\rdiRec.bin", g_prgDir);
	sprintf(buf3, "%s\\rdIndex.bin", g_prgDir);

	if (((rdirec = MBfopen(buf2, "rb")) != NULL) && ((i = getc(rdirec)) != EOF)) {
		uint64_t olddnum = 0;
		
		for (; i > 0; i--) {
			olddnum *= 256;
			olddnum += c = getc(rdirec);
			if (c == EOF) {
				fclose(rdirec), fclose(trdirec), MBremove(buf), MBremove(buf2);
				errorf("EOF before end of number in rdiRec");
				return 2;
			}
		}
		if ((num < 0) && (olddnum <= -num)) {
			fclose(rdirec), fclose(trdirec), MBremove(buf), MBremove(buf2);
			errorf("lastdnum going less than one");
			return 6;
		}
		dnum = olddnum + num;
		putull_pref(dnum, trdirec);
		
		gap = (long long) sqrt(dnum);
		fclose(rdirec), MBremove(buf2);
	} else {
		if (rdirec != NULL)
			fclose(rdirec);
		MBremove(buf2);
		
		if ((rdIndex = MBfopen(buf3, "rb")) == NULL) {
			fclose(trdirec), MBremove(buf);
			errorf("rdIndex file not found");
			return 3;
		}
		dnum = 0;
		while (1) {
			if ((c = null_fgets(0, MAX_PATH*4, rdIndex)) != 0) {
				if (c == 1) {
					break;
				} else {
					errorf("null_fgets: %d", c);
					fclose(trdirec), MBremove(buf), fclose(rdIndex);
					return 5;
				}
			}
			if ((c = pref_fgets(0, 9, rdIndex)) != 0) {
				errorf("EOF before end of number");
				fclose(trdirec), MBremove(buf), fclose(rdIndex);
				return 2;
			}
			dnum++;
		}
		fclose(rdIndex);
		
		putull_pref(dnum, trdirec);
		gap = sqrt(dnum);
	}
	
	if ((rdIndex = MBfopen(buf3, "rb")) == NULL) {
		fclose(trdirec), MBremove(buf);
		errorf("rdIndex file not found");
		return 3;
	}
	char buf4[MAX_PATH*4], *pbuf, *prevpbuf, *pbufhold;
	pbuf = buf3, prevpbuf = buf4;
	
	for (pos = ftell64(rdIndex), dnum = 1; 1; pos = ftell64(rdIndex), dnum++, pbufhold = prevpbuf, prevpbuf = pbuf, pbuf = pbufhold) {
		if ((c = null_fgets(pbuf, MAX_PATH*4, rdIndex)) != 0) {
			if (c == 1) {
				break;
			}
			errorf("EOF before end of number in rdIndex (2)");
			fclose(trdirec), MBremove(buf), fclose(rdIndex);
			return 2;
		}
		if (c == 2) {
			errorf("path too long rdIndex");
			return 2;
		}
		if (dnum % gap == 0) {
			i = 0;
			do {
				putc(pbuf[i++], trdirec);
			} while (pbuf[i-1] == prevpbuf[i-1]);
			putc('\0', trdirec);
			
			putull_pref(pos, trdirec);
		}
		
		if ((c = pref_fgets(0, 9, rdIndex)) != 0) {
			errorf("EOF before end of number");
			fclose(trdirec), MBremove(buf), fclose(rdIndex);
			return 2;
		}
	}
	fclose(rdIndex), fclose(trdirec);
	MBrename(buf, buf2);
	return 0;
}
/* 

uint64_t rdinitpos(uint64_t minum, char *idir) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 0;
	}
	
	char buf[MAX_PATH*4], ch;
	int i, c, remade = 0;
	uint64_t dnum, pos = 0, lastdnum = 0, gotdnum;
	FILE *rdirec;
	
	sprintf(buf, "%s\\rdiRec.bin", g_prgDir);
	
	if (((rdirec = MBfopen(buf, "rb")) == NULL) || ((i = getc(rdirec)) == EOF)) {
		if (rdirec != NULL)
			fclose(rdirec);
		raddtolastdnum(0);
		remade = 1;
		if (((rdirec = MBfopen(buf, "rb")) == NULL) || ((i = getc(rdirec)) == EOF)) {
			if (rdirec != NULL)
				fclose(rdirec);
			errorf("couldn't access rdirec");
			return 0;
		}		
	}
	for (; i > 0; i--) {
		lastdnum *= 256;
		lastdnum += c = getc(rdirec);
		if (c == EOF) {
			errorf("EOF before end of number in rdiRec");
			fclose(rdirec);
			return 0;
		}
	}
	if (lastdnum != (gotdnum = getlastdnum())) {
		fclose(rdirec);
		if (!remade) {
			if (((rdirec = MBfopen(buf, "rb")) == NULL) || ((i = getc(rdirec)) == EOF)) {
				if (rdirec != NULL)
					fclose(rdirec);
				errorf("couldn't access rdirec (2)");
				return 0;
			}
			for (lastdnum = 0; i > 0; i--) {
				lastdnum *= 256;
				lastdnum += c = getc(rdirec);
				if (c == EOF) {
					errorf("EOF before end of number in rdiRec");
					fclose(rdirec);
					return 0;
				}
			}
			if (lastdnum != gotdnum) {
				errorf("different lastdnum in rdIndex -- 1");
				return 0;
			}
		} else {
		errorf("different lastdnum in rdIndex -- 2");
		return 0;
		}
	}
	
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, rdirec)) != 0) {
			if (c == 1)
				break;
			errorf("rdiRec num read error: %d", c);
			fclose(rdirec);
			return 0;
		}

		if ((strcmp(idir, buf)) < 0)
			break;

		if (pos = fgetull_pref(rdirec, &c) == 0) {
			errorf("rdiRec num read error %c", c);
			fclose(rdirec);
			return 0;
		}
	}
	fclose(rdirec);
	
	return pos;
}

*/

char rdinit(uint64_t minum) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (rdinit)", minum);
		return 1;
	}
	
	unsigned char buf[MAX_PATH*4], *tfilestr;
	FILE *rindexF, *indexF, *tfile;
	uint64_t cLen, ntfilesegs;
	twoslnk *flink, *link;
	int c, i;
	
	char *rindexStrBase = "rdIndex";
	char *rrecStrBase = "rdiRec";
	char *indexStrBase = "dIndex";
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, rindexStrBase);
	MBremove(buf);
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, rrecStrBase);
	MBremove(buf);
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, indexStrBase);
	if ((indexF = MBfopen(buf, "rb")) == NULL) {
		errorf("indexF file not found (rdinit)");
		errorf("looked for: %s", buf);
		return 1;
	}
	
	#define raddtolastinum(num) raddtolastdnum(minum, num)
	
	ntfilesegs = 0;
	if ((tfilestr = reservetfile()) == 0) {
		errorf("reservetfile failed");
		return 2;
	}
	
	link = flink = malloc(sizeof(twoslnk));
	flink->u[0].str = flink->u[1].str = 0, flink->next = 0;
	cLen = 0;

	while ((c = getc(indexF)) != EOF) {
		link = link->next = malloc(sizeof(twoslnk));
		link->next = 0, link->u[0].str = link->u[1].str = 0;
		
		link->u[0].str = malloc(c+1), link->u[0].str[0] = c;
		for (i = 1; i <= link->u[0].str[0]; i++) {
			link->u[0].str[i] = c = getc(indexF); 
			if (c == EOF) {
				errorf("EOF before end of number (rdinit)");
				fclose(indexF), killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
				return 1;
			}
		}
		
		for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in indexF");
				fclose(indexF), killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
				return 1;
			}
			buf[i] = c;
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in indexF");
			fclose(indexF), killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
			return 1;
		}
		buf[i] = c;
		link->u[1].str = malloc(i+1);
		for (; i >= 0; link->u[1].str[i] = buf[i], i--);
		
		cLen++;
		if (cLen >= RD_MAX_CLEN) {
			link = flink, flink = flink->next, free(link);
			sorttwoschn(&flink, (int(*)(void*,void*))strcmp, 1, 0);
			tfile = opentfile(tfilestr, ntfilesegs++, "wb");
			for (link = flink; link != 0; link = link->next) {
				term_fputs(link->u[1].str, tfile);
				for (i = 0; i <= link->u[0].str[0]; putc(link->u[0].str[i], tfile), i++);
			}
			fclose(tfile);
			killtwoschn(flink, 0);
			link = flink = malloc(sizeof(twoslnk));
			flink->u[0].str = flink->u[1].str = 0, flink->next = 0;
			cLen = 0;
		}
	}
	fclose(indexF);
	
	if (cLen > 0) {
		link = flink, flink = flink->next, free(link);
		sorttwoschn(&flink, (int(*)(void*,void*))strcmp, 1, 0);
		tfile = opentfile(tfilestr, ntfilesegs++, "wb");
		for (link = flink; link != 0; link = link->next) {
			term_fputs(link->u[1].str, tfile);
			pref_fputs(link->u[0].str, tfile);
		}
		fclose(tfile);
	}
	killtwoschn(flink, 0);
	
	if (ntfilesegs > 1) {
		mergetfiles(tfilestr, ntfilesegs, 2, 2, (int(*)(unsigned char*,  unsigned char*)) strcmp, 1, 0);
		ntfilesegs = 1;
	}
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, rindexStrBase);
	
	if ((rindexF = MBfopen(buf, "wb")) == NULL) {
		errorf("couldn't create rindexF");
		releasetfile(tfilestr, ntfilesegs);
		return 1;
	}
	if ((tfile = opentfile(tfilestr, 0, "rb")) == NULL) {
		errorf("couldn't read temp file");
		fclose(rindexF), MBremove(buf), releasetfile(tfilestr, ntfilesegs);
		return 1;
	}
	
	while ((c = getc(tfile)) != EOF) {
		putc(c, rindexF);
	}
	fclose(rindexF), fclose(tfile);
	releasetfile(tfilestr, ntfilesegs);
	
	raddtolastinum(0);
	
	#undef raddtolastinum
	
	return 0;
}

/*

char rdreg___(uint64_t minum, char *dpath, uint64_t dnum) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	
}
uint64_t rdread(uint64_t minum, char *dpath) {		// reverse dread  //! untested
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 0;
	}
	
	uint64_t dnum = 0;
	FILE *rdIndex;
	unsigned char buf[MAX_PATH*4], *p;
	int c, i;
	
	sprintf(buf, "%s\\rdIndex.bin", g_prgDir);
	if ((rdIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("dIndex file not found");
		return 0;
	}
	
	fseek64(rdIndex, rdinitpos(dpath), SEEK_SET);
	
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, rdIndex)) != 0) {
			if (c == 1)
				break;
			errorf("null_fgets: %d", c);
			fclose(rdIndex);
			return 0;
		}
		
		if ((c = strcmp(dpath, buf)) > 0) {
			if ((i = pref_fgets(0, 9, rdIndex)) != 0) {
				errorf("pref_fgets: %d", i);
				fclose(rdIndex);
				return 0;
			}
		} else if (c == 0) {
			dnum = fgetull_pref(rdIndex, &c);
			fclose (rdIndex);
			if (c != 0) {
				errorf("rdindex read: %d", c);
				return 0;
			}
			return dnum;
		} else
			break;
	}
	fclose(rdIndex);
	return 0;
}

char rdrmv(uint64_t minum, char *dpath) {  //! untested
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	FILE *rdIndex, *trdIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], fin = 0;
	int c, i;
	
	sprintf(buf, "%s\\rdIndex.bin", g_prgDir);
	
	for (i = 0; dpath[i] != '\0' && i != MAX_PATH*4; i++);
	if (i >= MAX_PATH*4) {
		errorf("path too long");
		MBremove(buf);
		return 1;
	} if (i == 0 || dpath == 0) {
		errorf("empty string");
		MBremove(buf);
		return 1;
	}
	
	if ((rdIndex = MBfopen(buf, "rb")) == NULL) {
		MBremove(buf);
		errorf("rdIndex file not found");
		return 1;
	}
	sprintf(baf, "%s\\trdIndex.bin", g_prgDir);
	if ((trdIndex = MBfopen(baf, "wb")) == NULL) {
		fclose(rdIndex), MBremove(buf);
		return 3;
	}
	
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, rdIndex)) != 0) {
			if (c == 1)
				break;
			errorf("rdIndex read error: %d", c);
			fclose(rdIndex), MBremove(buf), fclose(trdIndex), MBremove(baf);
			return 2;
		}
		
		if (fin || (c = strcmp(dpath, buf)) > 0) {
			term_fputs(buf, trdIndex);
			for (c = getc(rdIndex), i = 1; i <= c; i++) {
				if (c == EOF) {
					errorf("EOF before end of num");
					fclose(rdIndex), MBremove(buf), fclose(trdIndex), MBremove(baf);
					return 2;
				} putc(c, trdIndex);
			}
		} else if (c == 0) {
			if ((pref_fgets(0, 9, rdIndex)) != 0) {
				errorf("EOF before end of number");
			}
			fin = 1;
		} else
			break;
	}
	fclose(rdIndex), fclose(trdIndex), MBremove(buf);
	if (fin) {
		raddtolastdnum(-1);
		MBrename(baf, buf);
		return 0;
	}
	MBremove(baf);
	return 4;
}

char rdrer(uint64_t minum, uint64_t source, char *dest) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	FILE *rdIndex, *trdIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], bef[MAX_PATH*4], fin1 = 0, fin2 = 0;
	int c, i;
	uint64_t dnum = 0;
	
	sprintf(bef, "%s\\rdIndex.bin", g_prgDir);
	
	for (i = 0; dest[i] != '\0' && i != MAX_PATH*4; i++);
	if (i >= MAX_PATH*4) {
		errorf("path too long");
		MBremove(bef);
		return 1;
	} if (i == 0 || dest == 0) {
		errorf("empty string");
		MBremove(bef);
		return 1;
	}
	
	if ((rdIndex = MBfopen(bef, "rb")) == NULL) {
		MBremove(bef);
		errorf("rdIndex file not found");
		return 1;
	}
	sprintf(baf, "%s\\trdIndex.bin", g_prgDir);
	if ((trdIndex = MBfopen(baf, "wb")) == NULL) {
		errorf("couldn't create trdIndex");
		fclose(rdIndex), MBremove(bef);
		return 3;
	}
	
	while (1) {
		if ((c = null_fgets(buf, MAX_PATH*4, rdIndex)) != 0) {
			if (c == 1)
				break;
			errorf("rdIndex read error: %d", c);
			fclose(rdIndex), MBremove(bef), fclose(trdIndex), MBremove(baf);
			return 2;
		}
		dnum = fgetull_pref(rdIndex, &c);
		if (c != 0) {
			errorf("rdIndex read error: %d", c);
			fclose(rdIndex), MBremove(bef), fclose(trdIndex), MBremove(baf);
			return 2;
		}
		
		if (!fin2 && strcmp(dest, buf) < 0) {
			fin2 = 1;
			term_fputs(dest, trdIndex);
			putull_pref(source, trdIndex);
		}
		
		if (fin1 || dnum != source) {
			term_fputs(buf, trdIndex);
			putull_pref(dnum, trdIndex);
		} else if (dnum == source) {
			fin1 = 1;
		} else
			break;
	}
	fclose(rdIndex), MBremove(bef);
	if (fin1) {
		if (!fin2) {
			term_fputs(dest, trdIndex);
			putull_pref(source, trdIndex);
		}
		fclose(trdIndex);
		MBrename(baf, bef);
		return 0;
	}
	fclose(trdIndex);
	MBremove(baf);
	return 4;
}

char rdrmvnum(uint64_t minum, uint64_t dnum) {		//! not done
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	rdinit();
	return 1;
}

*/

char crdreg(uint64_t minum, twoslnk *regchn) {		//! not done
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (crdreg)", minum);
		return 1;
	}
	
	rdinit(minum);
	return 1;
}

/*

char crdrer(uint64_t minum, oneslnk *rmchn, twoslnk *addchn) {		//! not done // assuming only one entry per dname, easily fixed by making rmchn twoslnk with dnum as well
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	rdinit();
	return 1;
}

char crdrmvnum(uint64_t minum, oneslnk *rmchn) {		//! not done
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	rdinit();
	return 1;
}

char dfrmv(uint64_t minum, uint64_t dnum) {		//! not done
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	rdinit();
	return 1;
}

char cdfrmv(uint64_t minum, oneslnk *dnumchm) {		//! not done
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	rdinit();
	return 1;
}

char passdextra(uint64_t minum, FILE *src, FILE *dst, uint64_t ull) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	int c;
	uint64_t ull2;
	if (ull == 0) {
		ull2 = fgetull_pref(src, &c);
		if (c != 0) {
			errorf("4 - dExtras num read failed: %d", c);
			return 1;
		}
		if (dst)
			putull_pref(ull2, dst);
		return 0;
	} else {
		errorf("unknown dExtras parameter: %llu", ull);
		return 1;
	}
}

char setdlastchecked(uint64_t minum, uint64_t dnum, uint64_t ulltime) {		// no record for offsets in file	//! not extensively tested
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	uint64_t tnum, tnum2;
	FILE *dExtras, *tdExtras;
	int c, i;
	unsigned char fin = 0;
	char buf[MAX_PATH*4], buf2[MAX_PATH*4];
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 1;
	}
	ulltime = time(0);
	
	sprintf(buf, "%s\\dExtras.bin", g_prgDir);
	if ((dExtras = MBfopen(buf, "rb")) == NULL) {
		if ((dExtras = MBfopen(buf, "wb+")) == NULL) {
			errorf("couldn't create dExtras.bin");
			return 1;
		}
	}
	sprintf(buf2, "%s\\dExtras.tmp", g_prgDir);
	if ((tdExtras = MBfopen(buf2, "wb+")) == NULL) {
		errorf("couldn't create dExtras.tmp");
		fclose(dExtras);
		return 1;
	}
	while (dExtras) {
		tnum = fgetull_pref(dExtras, &c);
		if (c != 0) {
			if (c == 1) {
				fclose(dExtras);
				dExtras = NULL;
				break;
			}
			errorf("dExtras num read failed: %d", c);
			fclose(dExtras), fclose(tdExtras), MBremove(buf2);
			return 1;
		}
		if (!fin && tnum > dnum) {
			putull_pref(dnum, tdExtras);
			putull_pref(0, tdExtras);
			putull_pref(ulltime, tdExtras);
			putc(0, tdExtras);
			fin = 1;
		}
		putull_pref(tnum, tdExtras);
		
		while (1) {
			tnum2 = fgetull_pref(dExtras, &c);
			if (c != 0) {
				if (c == ULL_READ_NULL)
					break;
				errorf("2 - dExtras num read failed: %d", c);
				fclose(dExtras), fclose(tdExtras), MBremove(buf2);
				return 1;
			}
			if (!fin && tnum == dnum && tnum2 >= 0) {
				putull_pref(0, tdExtras);
				if (tnum2 == 0) {
					fgetull_pref(dExtras, &c);
					if (c != 0) {
						errorf("3 - dExtras num read failed: %d", c);
						fclose(dExtras), fclose(tdExtras), MBremove(buf2);
						return 1;
					}
				}
				putull_pref(ulltime, tdExtras);
				fin = 1;
			} else {
				putull_pref(tnum2, tdExtras);
				if (passdextra(dExtras, tdExtras, tnum2)) {
					errorf("passdextras failed: %d", c);
					fclose(dExtras), fclose(tdExtras), MBremove(buf2);
					return 1;
				}
			}
		}
		if (!fin && tnum == dnum) {
			putull_pref(0, tdExtras);
			putull_pref(ulltime, tdExtras);
			fin = 1;			
		} putc('\0', tdExtras);
	}
	if (!fin) {
		putull_pref(dnum, tdExtras);
		putull_pref(0, tdExtras);
		putull_pref(ulltime, tdExtras);
		putc('\0', tdExtras);
	}
	
	fclose(tdExtras);
	
	char buf3[MAX_PATH*4];
	sprintf(buf3, "%s\\dExtras.bak", g_prgDir);
	MBremove(buf3);
	if (MBrename(buf, buf3)) {
		errorf("rename1 failed");
		errorf("%s->%s", buf, buf3);
		return 3;
	}
	if (MBrename(buf2, buf)) {
		MBrename(buf3, buf);
		errorf("rename2 failed");
		return 3;
	}
	MBremove(buf3);
	
	return 0;
}

uint64_t getdlastchecked(uint64_t minum, uint64_t dnum) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 0;
	}
	
	uint64_t ulltime, tnum, tnum2;
	FILE *dExtras;
	int c, i;
	char buf[MAX_PATH*4];
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 0;
	}
	ulltime = time(0);
	
	sprintf(buf, "%s\\dExtras.bin", g_prgDir);
	if ((dExtras = MBfopen(buf, "rb")) == NULL) {
		// errorf("couldn't open dExtras");
		return 0;
	}
	while (1) {
		tnum = fgetull_pref(dExtras, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf("dExtras num read failed: %d", c);
			fclose(dExtras);
			return 0;
		}
		while (1) {
			tnum2 = fgetull_pref(dExtras, &c);
			if (c != 0) {
				if (c == 4)
					break;
				errorf("2 - dExtras num read failed: %d", c);
				fclose(dExtras);
				return 0;
			}
			if (tnum == dnum && tnum2 == 0) {
				tnum2 = fgetull_pref(dExtras, &c);
				fclose(dExtras);
				if (c != 0) {
					errorf("3 - dExtras num read failed: %d", c);
					return 0;
				}
				return tnum2;
			} else {
				if (tnum2 == 0) {
					fgetull_pref(dExtras, &c);
					if (c != 0) {
						errorf("4 - dExtras num read failed: %d", c);
						fclose(dExtras);
						return 0;
					}
				} else {
					errorf("unknown dExtras parameter (getdlast): %llu", tnum2);
					fclose(dExtras);
					return 0;
				}
			}
		}
	} fclose(dExtras);
	
	return 0;
}
*/

//}

//{ directory

unsigned char addtolastdnum(uint64_t minum, long long num, uint64_t spot) { // 0 as num to just refresh from spot
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (addtolastdnum)", minum);
		return 1;
	}
	
	char buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], ch;
	int i, c;
	FILE *irec, *tirec, *indexF;
	
	char *indexStrBase = "dIndex";
	char *recStrBase = "diRec";

	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, recStrBase);
	if ((tirec = MBfopen(buf, "wb")) == NULL) {
		errorf("failed to create tirec");
		errorf("tried to create: \"%s\"", buf);
		return 1;
	}
	sprintf(buf2, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, recStrBase); // rec
	sprintf(buf3, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, indexStrBase); // index
	
	uint64_t inum = 0, gap = 0, pos = 0;
	if (((irec = MBfopen(buf2, "rb")) != NULL) && ((i = getc(irec)) != EOF)) {
		uint64_t oldinum = 0;
		
		for (; i > 0; i--) {
			oldinum *= 256;
			oldinum += c = getc(irec);
			if (c == EOF) {
				fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
				errorf("EOF before end of number in irec");
				return 2;
			}
		}
		if ((num < 0) && (oldinum <= -num)) {
			fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
			errorf("lastinum going less than one");
			return 6;
		}
		inum = oldinum + num;
		putull_pref(inum, tirec);
errorf("new inum: %llu", inum);
errorf("old inum was: %llu", oldinum);
		
		if ((gap = (long long) sqrt(inum)) == (long long) sqrt(oldinum)) {
			if (num > 0) {
				if (inum / gap == oldinum / gap) {
					while ((c = getc(irec)) != EOF) {
						putc(c, tirec);
					} fclose(irec), MBremove(buf2);
				} else {
					while ((c = getc(irec)) != EOF) {
						for (putc(c, tirec), i = c; i > 0 && (c = getc(irec)) != EOF; i--, putc(c, tirec));
						for (putc((ch = getc(irec)), tirec), i = 1, pos = 0; i++ <= ch && (c = getc(irec)) != EOF; pos *= 256, pos += c, putc(c, tirec));
						if (c == EOF) {
							errorf("EOF before finished irec entry (1)");
							fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
							return 1;
						}
					} fclose(irec), MBremove(buf2);
					
					if ((indexF = MBfopen(buf3, "rb")) != NULL) {
						fseek64(indexF, pos, SEEK_SET);
						fseek(indexF, (c = getc(indexF)), SEEK_CUR);
						if (c == EOF) {
							errorf("EOF before irectory in indexF");
							fclose(indexF), fclose(tirec), MBremove(buf);
							return 1;
						}
						null_fgets(0, MAX_PATH*4, indexF);
						
						for (pos = ftell64(indexF), inum = 0; (i = getc(indexF)) != EOF; pos = ftell64(indexF), inum = 0) {
							for (; i > 0; i--) {
								inum *= 256;
								inum += c = getc(indexF);
								if (c == EOF) {
									errorf("EOF before end of number in indexF (1)");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								}
							}
							if (inum % gap == 0) {
								putull_pref(inum, tirec);
								putull_pref(pos, tirec);
							}
							
							if ((c = null_fgets(0, MAX_PATH*4, indexF)) != 0) {
								if (c == 1) {
									errorf("EOF before null terminator in indexF");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								} else if (c == 2) {
									errorf("too long string in indexF");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								}
							}
						}
						fclose(indexF);
					} else {
						errorf("Couldn't open indexF");
						fclose(tirec), MBremove(buf);
						return 1;
					}
				}
				fclose(tirec), MBrename(buf, buf2);
				return 0;
			} else {
				oldinum = inum;
				uint64_t tpos;
				while ((c = getc(irec)) != EOF) {
					for (ch = c, i = 1, inum = 0; i++ <= ch && (c = getc(irec)) != EOF; inum *= 256, inum += c);
					for (ch = getc(irec), i = 1, tpos = 0; i++ <= ch && (c = getc(irec)) != EOF; tpos *= 256, tpos += c);
					if (c == EOF) {
						errorf("EOF before finished irec entry (2)");
						fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
						return 1;
					}
					if (inum > spot || inum > oldinum) {
						break;
					}
					pos = tpos;
					putull_pref(inum, tirec);
					putull_pref(pos, tirec);
				}
			}
		}
		fclose(irec), MBremove(buf2);
		
	} else {
		if (irec != NULL) {
			fclose(irec);
		}
		MBremove(buf2);
		
		if ((indexF = MBfopen(buf3, "rb")) == NULL) {	// it is valid for the index file not to exist
			fclose(tirec), MBremove(buf);
			// errorf("indexF file not found (addtolastinum 1)");
			// errorf("looked for file: %s", buf3);
			return 3;
		}
		inum = 0;
		while ((i = getc(indexF)) != EOF) {
			for (inum = 0; i > 0; i--) {
				inum *= 256;
				inum += c = getc(indexF);
				if (c == EOF) {
					errorf("EOF before end of number (addtolastinum 1)");
					fclose(tirec), MBremove(buf), fclose(indexF);
					return 2;
				}
			}
			for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
				if (c == EOF) {
					errorf("EOF before null terminator in indexF");
					fclose(tirec), MBremove(buf), fclose(indexF);
					return 4;
				}
			}
			if (i == MAX_PATH*4) {
				errorf("too long string in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 5;
			}
		}
		fclose(indexF);
		
		if (inum == 0) {
			fclose(tirec), MBremove(buf);
			return 3;
		}
		
errorf("counted inum: %llu", inum);
		
		putull_pref(inum, tirec);
		gap = sqrt(inum);
	}
	
	if ((indexF = MBfopen(buf3, "rb")) == NULL) {
		fclose(tirec), MBremove(buf);
		errorf("indexF file not found (addtolastinum 2)");
		errorf("looked for file: %s", buf3);
		return 3;
	}
	if (pos) {
		fseek64(indexF, pos, SEEK_SET);
		fseek(indexF, (c = getc(indexF)), SEEK_CUR);
		
		for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 4;
			}
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in indexF");
			fclose(tirec), MBremove(buf), fclose(indexF);
			return 5;
		}
	}
	for (pos = ftell64(indexF), inum = 0; (i = getc(indexF)) != EOF; pos = ftell64(indexF), inum = 0) {
		for (; i > 0; i--) {
			inum *= 256;
			inum += c = getc(indexF);
			if (c == EOF) {
				errorf("EOF before end of number in indexF (2)");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 2;
			}
		}
		if (inum % gap == 0) {
			putull_pref(inum, tirec);
			putull_pref(pos, tirec);
		}
		
		for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 2;
			}
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in indexF");
			fclose(tirec), MBremove(buf), fclose(indexF);
			return 2;
		}
	}
	fclose(indexF), fclose(tirec);
	MBrename(buf, buf2);
	return 0;
}

uint64_t getlastdnum(uint64_t minum) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (getlastdnum)", minum);
		return 0;
	}
	
	char buf[MAX_PATH*4];
	int i, c;
	uint64_t lastinum = 0;
	FILE *file;
	
	char *fileStrBase = "diRec";
	unsigned char (*addtolastinum)(uint64_t minum, long long num, uint64_t spot) = addtolastdnum;
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, fileStrBase);
	
	if (((file = MBfopen(buf, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
errorf("had no dnum to fetch");
		if (file != NULL) {
			fclose(file);
		}
		if ((c = addtolastinum(minum, 0, 0)) != 0) {
			if (c != 3) { // unless index file doesn't exist
				errorf("addtolastinum failed");
			}
			return 0;
		}
		if (((file = MBfopen(buf, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
			if (file != NULL) {
				fclose(file);
			}
			errorf("couldn't access irec (getlastinum)");
			return 0;
		}		
	}
	for (; i > 0; i--) {
		lastinum *= 256;
		lastinum += c = getc(file);
		if (c == EOF) {
			errorf("EOF before end of number in irec");
			fclose(file);
			return 0;
		}
	}
	fclose(file);
	return lastinum;
}

uint64_t dinitpos(uint64_t minum, uint64_t idnum) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (dinitpos)", minum);
		return 0;
	}
	
	char buf[MAX_PATH*4], ch;
	int i, c;
	uint64_t inum, pos = 0;
	FILE *irec;
	
	uint64_t iinum = idnum;
	char *fileStrBase = "diRec";
	unsigned char (*addtolastinum)(uint64_t minum, long long num, uint64_t spot) = addtolastdnum;
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, inum, fileStrBase);
	
	if (((irec = MBfopen(buf, "rb")) == NULL) || ((i = getc(irec)) == EOF)) {
		if (irec != NULL)
			fclose(irec);
		addtolastinum(minum, 0, 0);
		if (((irec = MBfopen(buf, "rb")) == NULL) || ((i = getc(irec)) == EOF)) {
			if (irec != NULL)
				fclose(irec);
			errorf("couldn't access irec (dinitpos)");
			return 0;
		}		
	}
	fseek(irec, i, SEEK_CUR);
	while ((c = getc(irec)) != EOF) {
		for (ch = c, i = 1, inum = 0; i <= ch && (c = getc(irec)) != EOF; i++, inum *= 256, inum += c);
		if (inum > iinum)
			break;
		for (ch = c = getc(irec), i = 1, pos = 0; i <= ch && (c = getc(irec)) != EOF; i++, pos *= 256, pos += c);
		if (c == EOF) {
			errorf("EOF before finished irec entry (miinitpos)");
			fclose(irec);
			return 0;
		}
	}
	fclose(irec);
	
	return pos;
}

/*

*/

uint64_t dreg(uint64_t minum, char *dpath) {
	int c;
	oneslnk *link = malloc(sizeof(oneslnk));
	link->next = NULL;
	link->str = dpath;
	
	c = cdreg(minum, link);
	free(link);
	return c;
}

char *dread(uint64_t minum, uint64_t dnum) {	// returns malloc memory
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (dread)", minum);
		return 0;
	}
	
	unsigned char buf[MAX_PATH*4], *p;
	int c, i;
	uint64_t tnum;
	FILE *indexF;
	
	uint64_t entrynum = dnum;
	char *fileStrBase = "dIndex";
	#define entryinitpos(entrynum) dinitpos(minum, entrynum)
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, fileStrBase);
	if ((indexF = MBfopen(buf, "rb")) == NULL) {
		errorf("index file not found (read)");
		errorf("looked for: %s", buf);
		return 0;
	}
	
	fseek64(indexF, entryinitpos(entrynum), SEEK_SET);
	#undef entryinitpos
	
	while (1) {
		tnum = fgetull_pref(indexF, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf("indexF num read failed: %d", c);
			fclose(indexF);
			return 0;
		}
		if (entrynum > tnum) {
			if ((c = null_fgets(0, MAX_PATH*4, indexF)) != 0) {
				errorf("indexF read error: %d", c);
				fclose(indexF);
				return 0;
			}
		} else if (entrynum == tnum) {
			if ((c = null_fgets(buf, MAX_PATH*4, indexF)) != 0) {
				errorf("indexF read error: %d", c);
				fclose(indexF);
				return 0;
			}
			fclose(indexF);
			if ((p = malloc(strlen(buf)+1)) == 0) {
				errorf("malloc failed");
				return p;
			}
			for (i = 0; p[i] = buf[i]; i++);
			return p;
		} else
			break;
	}
	fclose(indexF);
	return 0;
}

/*

char drmv(uint64_t minum, uint64_t dnum) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	int c;
	oneslnk *link = malloc(sizeof(oneslnk));
	link->next = NULL;
	link->ull = dnum;
	
	c = cdrmv(link);
	free(link);
	return c;
}

char drer(uint64_t minum, uint64_t dnum, char *dpath) {		// reroute
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	FILE *dIndex, *tIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], fin = 0;
	int c, i, j;
	uint64_t tnum;
	
	for (i = 0; dpath[i] != '\0' && i != MAX_PATH*4; i++);
	if (i >= MAX_PATH*4) {
		errorf("path too long");
		return 3;
	} if (i == 0) {
		errorf("empty string");
		return 3;
	}
	
	sprintf(buf, "%s\\dIndex.bin", g_prgDir);
	if ((dIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("dIndex file not found");
		return 2;
	}
	sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
	if ((tIndex = MBfopen(baf, "wb")) == NULL) {
		errorf("couldn't create dIndex.tmp");
		fclose(dIndex);
		return 2;
	}
	
	while (1) {
		tnum = fgetull_pref(dIndex, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf("dIndex num read failed: %d", c);
			fclose(dIndex);
			return 0;
		}
		putull_pref(tnum, tIndex);
		
		if (fin || dnum > tnum) {				
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(dIndex), fclose(tIndex);
					return 2;
				} else
					putc(c, tIndex);
			putc(c, tIndex);
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex);
				return 2;
			}
		} else if (dnum == tnum) {
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(dIndex), fclose(tIndex);
					return 2;
				}
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex);
				return 2;
			}
			for (i = 0; (putc(dpath[i], tIndex), dpath[i] != '\0') && i < MAX_PATH*4; i++);
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex);
				return 2;
			}
			
			while ((c = getc(dIndex)) != EOF) {
				putc(c, tIndex);
			}
			fin = 1;
		} else {
			break;
		}
	}
	fclose(dIndex), fclose(tIndex);
	
	if (fin) {
		sprintf(buf, "%s\\dIndex.bin", g_prgDir);
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		MBremove(baf);
		if (MBrename(buf, baf)) {
			errorf("rename1 failed");
			return 3;
		}
		sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
		if (MBrename(baf, buf)) {
			sprintf(baf, "%s\\dIndex.bak", g_prgDir), MBrename(baf, buf);
			errorf("rename2 failed");
			return 3;
		}
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);

		rdrer(dnum, dpath);
		
		addtolastdnum(0, dnum);
		MBremove(baf);
		return 0;
	}
	
	MBremove(baf);
	return 1;		// entry number doesn't have entry
}

*/

int cdreg(uint64_t minum, oneslnk *dpathchn) {
errorf("callign existsmi");
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (cdreg)", minum);
		return 1;
	}
errorf("after existsmi");
	
	long int maxentrylen = MAX_PATH*4;
	
	oneslnk *inputchn = dpathchn;
	char *fileStrBase = "dIndex";
	#define getlastentrynum() getlastdnum(minum)
	#define addtolastinum(num1, num2) addtolastdnum(minum, num1, num2)
	#define crentryreg(numentrychn) crdreg(minum, numentrychn)
	
	unsigned char buf[MAX_PATH*4];
	
	if (inputchn == 0) {
		errorf("inputchn is null");
		return 1;
	}
	
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
	
	FILE *indexF;
	char created = 0;
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, fileStrBase);
	if ((indexF = MBfopen(buf, "rb+")) == NULL) {
		if ((indexF = MBfopen(buf, "wb+")) == NULL) {
			errorf("couldn't create indexF");
			errorf("tried to create: \"%s\"", buf);
			return 1;
		}
		created = 1;
	}
	
	uint64_t lastentrynum = 0;
	
	//if (lastdnum = getlastdnum()) {
	if ( (!created) && ((lastentrynum = getlastentrynum()) > 0) ) {
		#undef getlastentrynum
		fseek(indexF, 0, SEEK_END);
	} else {
		int c;
		while ((c = getc(indexF)) != EOF) {
			//! TODO: check for cohesion
			if (fseek(indexF, c, SEEK_CUR)) {
				errorf("fseek failed");
				fclose(indexF);
				return 0;
			}
			
			if ((c = null_fgets(0, maxentrylen, indexF)) != 0) {
				errorf("indexF read error: %d", c);
				fclose(indexF);
				return 0;
			}
			lastentrynum++;		// assuming there are no entry number gaps
		}
	}
	
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
	
	numentrychn->next = 0;
	numentrychn = fnumentry->next, free(fnumentry);
	crentryreg(numentrychn);
	#undef crentryreg
	killtwoschn(numentrychn, 3);
	fclose(indexF);
errorf("adding to last dnum");
	// going to count wrong if indexF not closed
	addtolastinum(i, 0);
	#undef addtolastinum
	return 0;
}

/*

oneslnk *cdread(uint64_t minum, oneslnk *dnumchn) {		//! not done
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 0;
	}
	
	
	return NULL; //!
}

int cdrmv(uint64_t minum, oneslnk *dnumchn) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	FILE *dIndex, *tIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4];
	int c, i, j;
	oneslnk *flink;
	uint64_t nremoved = 0, tnum;
	
	sprintf(buf, "%s\\dIndex.bin", g_prgDir);
	if ((dIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("dIndex file not found");
		return 2;
	}
	sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
	if ((tIndex = MBfopen(baf, "wb")) == NULL) {
		errorf("couldn't create dIndex.tmp");
		fclose(dIndex);
		return 2;
	}
	
	if (!(flink = dnumchn = copyoneschn(dnumchn, 1))) {
		errorf("copyoneschn failed");
		fclose(dIndex), fclose(tIndex), MBremove(baf);
		return 2;
	}
	if (sortoneschnull(flink, 0)) {
		errorf("sortoneschn failed");
		fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
		return 2;
	}
	if (dnumchn->ull == 0) {
		errorf("passed 0 dnum to cdrmv");
		fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
		return 2;
	}	
	
	while (1) {
		tnum = fgetull_pref(dIndex, &c);
		if (c != 0) {
			if (c == 1) {
				c = EOF;
				break;
			}
			errorf("dIndex num read failed: %d", c);
			fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
			return 0;
		}
		
		if (!(dnumchn) || tnum < dnumchn->ull) {
			tnum -= nremoved;
			putull_pref(tnum, tIndex);
			
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++) {
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
					return 2;
				} else
					putc(c, tIndex);
			}
			putc(c, tIndex);
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
				return 2;
			}
		} else if (tnum == dnumchn->ull) {
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
					return 2;
				}
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killoneschn(flink, 1);
				return 2;
			}
			nremoved++;
			dnumchn = dnumchn->next;
		} else {
			break;
		}
	}
	fclose(dIndex); fclose(tIndex);
	
	if (dnumchn == 0 && c == EOF) {
		sprintf(buf, "%s\\dIndex.bin", g_prgDir);
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		MBremove(baf);
		if (MBrename(buf, baf)) {
			errorf("rename1 failed");
			killoneschn(flink, 1);
			return 3;
		}
		sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
		if (MBrename(baf, buf)) {
			killoneschn(flink, 1);
			errorf("rename2 failed");
			return 3;
		}
				
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		if (cdfrmv(flink)) {
			MBremove(buf), MBrename(baf, buf);
			errorf("cdfrmv failed");
			return 4;
		}
		
		addtolastdnum(-nremoved, flink->ull);
		killoneschn(flink, 1);
		
		MBremove(baf);
		
		return 0;
	}
	MBremove(baf), killoneschn(flink, 1);
	return 1;
}

int cdrer(uint64_t minum, twoslnk *rerchn) {		//! untested
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false", minum);
		return 1;
	}
	
	FILE *dIndex, *tIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], *numbuf;
	int c, i, j;
	twoslnk *flink;
	oneslnk *foslnk = 0, *oslnk;
	uint64_t tnum;
	
	sprintf(buf, "%s\\dIndex.bin", g_prgDir);
	if ((dIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("dIndex file not found");
		return 2;
	}
	sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
	if ((tIndex = MBfopen(baf, "wb")) == NULL) {
		errorf("couldn't create dIndex.tmp");
		fclose(dIndex);
		return 2;
	}
	
	if (!(flink = rerchn = copytwoschn(rerchn, 1))) {
		errorf("copytwoschn failed");
		fclose(dIndex), fclose(tIndex), MBremove(baf);
		return 2;
	}
	if (sorttwoschnull(&flink, 0, 0)) {
		errorf("sorttwoschn failed");
		fclose(dIndex), fclose(tIndex), killtwoschn(flink, 1), MBremove(baf);
		return 2;
	}
	
	if (rerchn->u[0].ull == 0) {
		errorf("passed null dnum to cdrer");
		fclose(dIndex), fclose(tIndex), killtwoschn(flink, 1), MBremove(baf);
		return 2;
	}
	foslnk = oslnk = malloc(sizeof(oneslnk));
	foslnk->str = 0, foslnk->next = 0;
	
	while (1) {		// no recourse if string is null
		tnum = fgetull_pref(dIndex, &c);
		if (c != 0) {
			if (c == 1) {
				c = EOF;
				break;
			}
			errorf("dIndex num read failed: %d", c);
			fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
			return 2;
		}
		putull_pref(tnum, tIndex);
		
		if (!(rerchn) || tnum < rerchn->u[0].ull) {
		
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
					return 2;
				} else
					putc(c, tIndex);
			putc(c, tIndex);
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
				return 2;
			}
			
		} else if (tnum == rerchn->u[0].ull) {
			if ((c = null_fgets(buf, MAX_PATH*4, dIndex)) != 0) {
				errorf("dIndex read error: %d", c);
				fclose(dIndex);
				return 2;
			}
			oslnk = oslnk->next = malloc(sizeof(oneslnk));
			oslnk->next = 0;
			if ((oslnk->str = malloc(strlen(buf)+1)) == 0) {
				errorf("malloc failed");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
				return 2;
			}
			for (i = 0; oslnk->str[i] = buf[i]; i++);
			
			if (rerchn->u[1].str == 0)
				break;
			
			for (i = 0; (putc(rerchn->u[1].str[i], tIndex), rerchn->u[1].str[i] != '\0') && i < MAX_PATH*4; i++);
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(dIndex), fclose(tIndex), MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
				return 2;
			}			
			rerchn = rerchn->next;
		} else {
			break;
		}
	}
	fclose(dIndex); fclose(tIndex);
	
	if (rerchn == 0 && c == EOF) {
		sprintf(buf, "%s\\dIndex.bin", g_prgDir);
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		MBremove(baf);
		if (MBrename(buf, baf)) {
			errorf("rename1 failed");
			killtwoschn(flink, 1), killoneschn(foslnk, 1);
			return 3;
		}
		sprintf(baf, "%s\\dIndex.tmp", g_prgDir);
		if (MBrename(baf, buf)) {
			killtwoschn(flink, 1), killoneschn(foslnk, 1);
			errorf("rename2 failed");
			return 3;
		}
		sprintf(baf, "%s\\dIndex.bak", g_prgDir);
		
		oslnk = foslnk, foslnk = foslnk->next, free(oslnk);
		crdrer(foslnk, flink);
		
		addtolastdnum(0, flink->u[0].ull);
		MBremove(baf);
		
		killtwoschn(flink, 1), killoneschn(foslnk, 1);
		
		return 0;
	}
	MBremove(baf), killtwoschn(flink, 1), killoneschn(foslnk, 1);
	return 1;
}

*/

oneslnk *idread(uint64_t minum, uint64_t start, uint64_t intrvl) {	// returns malloc memory (the whole chain)
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (idread)", minum);
		return 0;
	}
	
	unsigned char buf[MAX_PATH*4];
	int c, i;
	uint64_t tnum;
	oneslnk *flnk, *lastlnk;
	FILE *indexF;
				
	if (!intrvl) {
		errorf("idread without intrvl");
		return 0;
	}
	
	uint64_t entrynum = start;
	char *fileStrBase = "dIndex";
	#define entryinitpos(ientrynum) dinitpos(minum, ientrynum)
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, fileStrBase);
	if ((indexF = MBfopen(buf, "rb")) == NULL) {
		//errorf("indexF file not found (imiread)");
		//errorf("looked for file: %s", buf);
		return 0;
	}
	
	fseek64(indexF, entryinitpos(entrynum), SEEK_SET);
	#undef entryinitpos
	
	while (1) {
		tnum = fgetull_pref(indexF, &c);
		if (c != 0) {
			if (c == 1) {
				break;
			}
			errorf("1 - indexF num read failed: %d", c);
			fclose(indexF);
			return 0;
		}
		
		if (tnum < start) {
			for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in indexF");
					fclose(indexF);
					return 0;
				}
			if (i == MAX_PATH*4) {
				errorf("too long string in indexF");
				fclose(indexF);
				return 0;
			}
		} else {			// found start entry or over
			if ((flnk = malloc(sizeof(oneslnk))) == 0) {
				errorf("malloc failed");
				fclose(indexF);
				return 0;
			} lastlnk = flnk;
			flnk->str = 0;

			while (intrvl > 0) {
				while (tnum > start) {
					if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
						errorf("malloc failed");
						lastlnk->next = 0;
						fclose(indexF), killoneschn(flnk, 0);
						return 0;
					}
					lastlnk->str = 0;
					start++;
					intrvl--;
					if (intrvl <= 0) {
						fclose(indexF);
						lastlnk->next = 0;
						lastlnk = flnk->next;
						free(flnk);
						return lastlnk;
					}
				}
				
				for (i = 0; (buf[i] = c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
					if (c == EOF) {
						errorf("EOF before null terminator in indexF");
						lastlnk->next = 0;
						fclose(indexF), killoneschn(flnk, 0);
						return 0;
					}
				}
				if (i == MAX_PATH*4) {
					errorf("too long string in indexF");
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
				
				if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed");
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
				lastlnk->str = malloc(i+1);
				snprintf(lastlnk->str, i+1, "%s", buf);
				start++;
				intrvl--;
				
				tnum = fgetull_pref(indexF, &c);
				if (c != 0) {
					if (c == 1)
						break;
					errorf("2 - indexF num read failed: %d", c);
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
			} fclose(indexF);
			
			lastlnk->next = 0;
			lastlnk = flnk->next;
			free(flnk);
			return lastlnk;
		}
	}
	fclose(indexF);
	return 0;
}

/*

void verDI(VOID) { // check for order, duplicates, gaps, 0 character strings
	
}


 */

//}

//{	subdir

int readSubdirEntryTo(FILE *sourcef, struct subdirentry *dest) {
	
}

unsigned char addtolastsdnum(uint64_t minum, long long num, uint64_t spot) { // 0 as num to just refresh from spot
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (addtolastsdnum)", minum);
		return 1;
	}
	
	char buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], ch;
	int i, c;
	FILE *irec, *tirec, *indexF;
	
	char *indexStrBase = "dIndex";
	char *recStrBase = "diRec";

	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, recStrBase);
	if ((tirec = MBfopen(buf, "wb")) == NULL) {
		errorf("failed to create tirec");
		errorf("tried to create: \"%s\"", buf);
		return 1;
	}
	sprintf(buf2, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, recStrBase); // rec
	sprintf(buf3, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, indexStrBase); // index
	
	uint64_t inum = 0, gap = 0, pos = 0;
	if (((irec = MBfopen(buf2, "rb")) != NULL) && ((i = getc(irec)) != EOF)) {
		uint64_t oldinum = 0;
		
		for (; i > 0; i--) {
			oldinum *= 256;
			oldinum += c = getc(irec);
			if (c == EOF) {
				fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
				errorf("EOF before end of number in irec");
				return 2;
			}
		}
		if ((num < 0) && (oldinum <= -num)) {
			fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
			errorf("lastinum going less than one");
			return 6;
		}
		inum = oldinum + num;
		putull_pref(inum, tirec);
errorf("new inum: %d", inum);
		
		if ((gap = (long long) sqrt(inum)) == (long long) sqrt(oldinum)) {
			if (num > 0) {
				if (inum / gap == oldinum / gap) {
					while ((c = getc(irec)) != EOF) {
						putc(c, tirec);
					} fclose(irec), MBremove(buf2);
				} else {
					while ((c = getc(irec)) != EOF) {
						for (putc(c, tirec), i = c; i > 0 && (c = getc(irec)) != EOF; i--, putc(c, tirec));
						for (putc((ch = getc(irec)), tirec), i = 1, pos = 0; i++ <= ch && (c = getc(irec)) != EOF; pos *= 256, pos += c, putc(c, tirec));
						if (c == EOF) {
							errorf("EOF before finished irec entry (1)");
							fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
							return 1;
						}
					} fclose(irec), MBremove(buf2);
					
					if ((indexF = MBfopen(buf3, "rb")) != NULL) {
						fseek64(indexF, pos, SEEK_SET);
						fseek(indexF, (c = getc(indexF)), SEEK_CUR);
						if (c == EOF) {
							errorf("EOF before irectory in indexF");
							fclose(indexF), fclose(tirec), MBremove(buf);
							return 1;
						}
						null_fgets(0, MAX_PATH*4, indexF);
						
						for (pos = ftell64(indexF), inum = 0; (i = getc(indexF)) != EOF; pos = ftell64(indexF), inum = 0) {
							for (; i > 0; i--) {
								inum *= 256;
								inum += c = getc(indexF);
								if (c == EOF) {
									errorf("EOF before end of number in indexF (1)");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								}
							}
							if (inum % gap == 0) {
								putull_pref(inum, tirec);
								putull_pref(pos, tirec);
							}
							
							if ((c = null_fgets(0, MAX_PATH*4, indexF)) != 0) {
								if (c == 1) {
									errorf("EOF before null terminator in indexF");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								} else if (c == 2) {
									errorf("too long string in indexF");
									fclose(tirec), MBremove(buf), fclose(indexF);
									return 2;
								}
							}
						}
						fclose(indexF);
					} else {
						errorf("Couldn't open indexF");
						fclose(tirec), MBremove(buf);
						return 1;
					}
				}
				fclose(tirec), MBrename(buf, buf2);
				return 0;
			} else {
				oldinum = inum;
				uint64_t tpos;
				while ((c = getc(irec)) != EOF) {
					for (ch = c, i = 1, inum = 0; i++ <= ch && (c = getc(irec)) != EOF; inum *= 256, inum += c);
					for (ch = getc(irec), i = 1, tpos = 0; i++ <= ch && (c = getc(irec)) != EOF; tpos *= 256, tpos += c);
					if (c == EOF) {
						errorf("EOF before finished irec entry (2)");
						fclose(irec), fclose(tirec), MBremove(buf), MBremove(buf2);
						return 1;
					}
					if (inum > spot || inum > oldinum) {
						break;
					}
					pos = tpos;
					putull_pref(inum, tirec);
					putull_pref(pos, tirec);
				}
			}
		}
		fclose(irec), MBremove(buf2);
		
	} else {
		if (irec != NULL) {
			fclose(irec);
		}
		MBremove(buf2);
		
		if ((indexF = MBfopen(buf3, "rb")) == NULL) {	// it is valid for the index file not to exist
			fclose(tirec), MBremove(buf);
			// errorf("indexF file not found (addtolastinum 1)");
			// errorf("looked for file: %s", buf3);
			return 3;
		}
		inum = 0;
		while ((i = getc(indexF)) != EOF) {
			for (inum = 0; i > 0; i--) {
				inum *= 256;
				inum += c = getc(indexF);
				if (c == EOF) {
					errorf("EOF before end of number (addtolastinum)");
					fclose(tirec), MBremove(buf), fclose(indexF);
					return 2;
				}
			}
			for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
				if (c == EOF) {
					errorf("EOF before null terminator in indexF");
					fclose(tirec), MBremove(buf), fclose(indexF);
					return 4;
				}
			}
			if (i == MAX_PATH*4) {
				errorf("too long string in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 5;
			}
		}
		fclose(indexF);
		
		if (inum == 0) {
			fclose(tirec), MBremove(buf);
			return 3;
		}
		
		putull_pref(inum, tirec);
		gap = sqrt(inum);
	}
	
	if ((indexF = MBfopen(buf3, "rb")) == NULL) {
		fclose(tirec), MBremove(buf);
		errorf("indexF file not found (addtolastinum 2)");
		errorf("looked for file: %s", buf3);
		return 3;
	}
	if (pos) {
		fseek64(indexF, pos, SEEK_SET);
		fseek(indexF, (c = getc(indexF)), SEEK_CUR);
		
		for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 4;
			}
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in indexF");
			fclose(tirec), MBremove(buf), fclose(indexF);
			return 5;
		}
	}
	for (pos = ftell64(indexF), inum = 0; (i = getc(indexF)) != EOF; pos = ftell64(indexF), inum = 0) {
		for (; i > 0; i--) {
			inum *= 256;
			inum += c = getc(indexF);
			if (c == EOF) {
				errorf("EOF before end of number in indexF (2)");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 2;
			}
		}
		if (inum % gap == 0) {
			putull_pref(inum, tirec);
			putull_pref(pos, tirec);
		}
		
		for (i = 0; (c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in indexF");
				fclose(tirec), MBremove(buf), fclose(indexF);
				return 2;
			}
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in indexF");
			fclose(tirec), MBremove(buf), fclose(indexF);
			return 2;
		}
	}
	fclose(indexF), fclose(tirec);
	MBrename(buf, buf2);
	return 0;
}

uint64_t getlastsdnum(uint64_t minum) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false ([func] getlastsdnum)", minum);
		return 0;
	}
	
	char buf[MAX_PATH*4];
	int i, c;
	uint64_t lastinum = 0;
	FILE *file;
	
	char *fileStrBase = "sdiRec";
	unsigned char (*addtolastinum)(uint64_t minum, long long num, uint64_t spot) = addtolastsdnum;
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, fileStrBase);
	
	if (((file = MBfopen(buf, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
		if (file != NULL) {
			fclose(file);
		}
		if ((c = addtolastinum(minum, 0, 0)) != 0) {
			if (c != 3) { // unless index file doesn't exist
				errorf("addtolastinum failed ([func] getlastsdnum)");
			}
			return 0;
		}
		if (((file = MBfopen(buf, "rb")) == NULL) || ((i = getc(file)) == EOF)) {
			if (file != NULL) {
				fclose(file);
			}
			errorf("couldn't access irec ([func] getlastsdnum)");
			return 0;
		}		
	}
	for (; i > 0; i--) {
		lastinum *= 256;
		lastinum += c = getc(file);
		if (c == EOF) {
			errorf("EOF before end of number in irec ([func] getlastsdnum)");
			fclose(file);
			return 0;
		}
	}
	fclose(file);
	return lastinum;
}

uint64_t sdinitpos(uint64_t minum, uint64_t idnum) {
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false (dinitpos)", minum);
		return 0;
	}
	
	
	char buf[MAX_PATH*4], ch;
	int i, c;
	uint64_t inum, pos = 0;
	FILE *irec;
	
	uint64_t iinum = idnum;
	char *fileStrBase = "sdiRec";
	unsigned char (*addtolastinum)(uint64_t minum, long long num, uint64_t spot) = addtolastdnum;
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, inum, fileStrBase);
	
	if (((irec = MBfopen(buf, "rb")) == NULL) || ((i = getc(irec)) == EOF)) {
		if (irec != NULL)
			fclose(irec);
		addtolastinum(minum, 0, 0);
		if (((irec = MBfopen(buf, "rb")) == NULL) || ((i = getc(irec)) == EOF)) {
			if (irec != NULL)
				fclose(irec);
			errorf("couldn't access irec (sdinitpos)");
			return 0;
		}		
	}
	fseek(irec, i, SEEK_CUR);
	while ((c = getc(irec)) != EOF) {
		for (ch = c, i = 1, inum = 0; i <= ch && (c = getc(irec)) != EOF; i++, inum *= 256, inum += c);
		if (inum > iinum)
			break;
		for (ch = c = getc(irec), i = 1, pos = 0; i <= ch && (c = getc(irec)) != EOF; i++, pos *= 256, pos += c);
		if (c == EOF) {
			errorf("EOF before finished irec entry (miinitpos)");
			fclose(irec);
			return 0;
		}
	}
	fclose(irec);
	
	return pos;
}

oneslnk *isdread(uint64_t minum, uint64_t start, uint64_t intrvl) {	// returns malloc memory (the whole chain)
	if (!existsmi(minum)) {
		errorf("existsmi(%llu) returned false ([func] isdread)", minum);
		return 0;
	}
	
	unsigned char buf[MAX_PATH*4];
	int c, i;
	uint64_t tnum;
	oneslnk *flnk, *lastlnk;
	FILE *indexF;
				
	if (!intrvl) {
		errorf("idread without intrvl ([func] isdread)");
		return 0;
	}
	
	uint64_t entrynum = start;
	char *fileStrBase = "sdIndex";
	#define entryinitpos(ientrynum) sdinitpos(minum, ientrynum)
	
	sprintf(buf, "%s\\i\\%llu\\%s.bin", g_prgDir, minum, fileStrBase);
	if ((indexF = MBfopen(buf, "rb")) == NULL) {
		//errorf("indexF file not found (imiread)");
		//errorf("looked for file: %s", buf);
		return 0;
	}
	
	fseek64(indexF, entryinitpos(entrynum), SEEK_SET);
	#undef entryinitpos
	
	struct subdirentry sd_struct = {0};
	
	while (1) {
		tnum = fgetull_pref(indexF, &c);
		if (c != 0) {
			if (c == 1) {
				break;
			}
			errorf("1 - indexF num read failed: %d ([func] isdread)", c);
			fclose(indexF);
			return 0;
		}
		
		if (tnum < start) {
			c = readSubdirEntryTo(indexF, &sd_struct);
			
			if (c != 0) {
				errorf("1 - indexF entry read failed: %d ([func] isdread)", c);
				fclose(indexF);
				return 0;
			}
		} else {			// found start entry or over
			if ((flnk = malloc(sizeof(oneslnk))) == 0) {
				errorf("malloc failed ([func] isdread)");
				fclose(indexF);
				return 0;
			} lastlnk = flnk;
			flnk->str = 0;

			while (intrvl > 0) {
				while (tnum > start) {
					if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
						errorf("malloc failed ([func] isdread)");
						lastlnk->next = 0;
						fclose(indexF), killoneschn(flnk, 0);
						return 0;
					}
					lastlnk->str = 0;
					start++;
					intrvl--;
					if (intrvl <= 0) {
						fclose(indexF);
						lastlnk->next = 0;
						lastlnk = flnk->next;
						free(flnk);
						return lastlnk;
					}
				}
				
				for (i = 0; (buf[i] = c = getc(indexF)) != '\0' && i < MAX_PATH*4; i++) {
					if (c == EOF) {
						errorf("EOF before null terminator in indexF ([func] isdread)");
						lastlnk->next = 0;
						fclose(indexF), killoneschn(flnk, 0);
						return 0;
					}
				}
				if (i == MAX_PATH*4) {
					errorf("too long string in indexF ([func] isdread)");
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
				
				if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed");
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
				lastlnk->str = malloc(i+1);
				snprintf(lastlnk->str, i+1, "%s", buf);
				start++;
				intrvl--;
				
				tnum = fgetull_pref(indexF, &c);
				if (c != 0) {
					if (c == 1)
						break;
					errorf("2 - indexF num read failed: %d ([func] isdread)", c);
					lastlnk->next = 0;
					fclose(indexF), killoneschn(flnk, 0);
					return 0;
				}
			} fclose(indexF);
			
			lastlnk->next = 0;
			lastlnk = flnk->next;
			free(flnk);
			return lastlnk;
		}
	}
	fclose(indexF);
	return 0;
}

//}

//{ file 2nd layer
char raddtolastfnum(uint64_t dnum, long long num) {		//! not done
	if (dnum == 0) {
		errorf("dnum is zero in raddtolastfnum");
		return 0;
	}
	return 0; //!
}

char rfinit(uint64_t dnum) { //! untested
	unsigned char buf[MAX_PATH*4], *tfilestr;
	FILE *rfIndex, *fIndex, *tfile;
	uint64_t cLen, ntfilesegs;
	twoslnk *flink, *link;
	int c, i;

	if (dnum == 0) {
		errorf("dnum is zero in rfinit");
		return 0;
	}
	sprintf(buf, "%s\\i\\%llu\\rfIndex.bin", g_prgDir, dnum);
	MBremove(buf);
	sprintf(buf, "%s\\i\\%llu\\rfiRec.bin", g_prgDir, dnum);
	MBremove(buf);
	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dnum);
	if ((fIndex = MBfopen(buf, "rb")) == NULL) {
	//	errorf("fIndex file not found");
		return 1;
	}
	
	ntfilesegs = 0;
	if ((tfilestr = reservetfile()) == 0) {
		errorf("reservetfile failed");
		return 2;
	}
	
	link = flink = malloc(sizeof(twoslnk));
	flink->u[0].str = flink->u[1].str = 0, flink->next = 0;
	cLen = 0;

	while ((c = getc(fIndex)) != EOF) {
		link = link->next = malloc(sizeof(twoslnk));
		link->u[0].str = link->u[1].str = 0;
		
		link->u[0].str = malloc(c+1), link->u[0].str[0] = c;
		for (i = 1; i <= link->u[0].str[0]; i++) {
			link->u[0].str[i] = c = getc(fIndex); 
			if (c == EOF) {
				errorf("EOF before end of number (rfinit)");
				fclose(fIndex), link->next = 0, killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
				return 2;
			}
		}
		
		for (i = 0; (c = getc(fIndex)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in fIndex");
				fclose(fIndex), link->next = 0, killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
				return 2;
			}
			buf[i] = c;
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in fIndex");
			fclose(fIndex), link->next = 0, killtwoschn(flink, 0), releasetfile(tfilestr, ntfilesegs);
			return 2;
		}
		buf[i] = c;
		link->u[1].str = malloc(i+1);
		for (; i >= 0; link->u[1].str[i] = buf[i], i--);
			
		while ((c = getc(fIndex)) != 0) {
			if (c == EOF) {
				errorf("EOF before end of tags in fIndex");
				fclose(fIndex);
				return 2;
			}
			if (fseek(fIndex, c, SEEK_CUR)) {
				errorf("fseek failed");
				fclose(fIndex);
				return 2;
			}
		}
		
		cLen++;
		if (cLen >= RF_MAX_CLEN) {
			link->next = 0, link = flink, flink = flink->next, free(link);
			sorttwoschn(&flink, (int(*)(void*,void*)) strcmp, 1, 0);
			tfile = opentfile(tfilestr, ntfilesegs++, "wb");
			for (link = flink; link != 0; link = link->next) {
				term_fputs(link->u[1].str, tfile);
				for (i = 0; i <= link->u[0].str[0]; putc(link->u[0].str[i], tfile), i++);
			}
			fclose(tfile);
			killtwoschn(flink, 0);
			link = flink = malloc(sizeof(twoslnk));
			flink->u[0].str = flink->u[1].str = 0, flink->next = 0;
			cLen = 0;
		}
	}
	fclose(fIndex), link->next = 0;
	
	if (cLen > 0) {
		link->next = 0, link = flink, flink = flink->next, free(link);
		sorttwoschn(&flink, (int(*)(void*,void*)) strcmp, 1, 0);
		tfile = opentfile(tfilestr, ntfilesegs++, "wb");
		for (link = flink; link != 0; link = link->next) {
			term_fputs(link->u[1].str, tfile);
			pref_fputs(link->u[0].str, tfile);
		}
		fclose(tfile);
	}
	killtwoschn(flink, 0);
	
	if (ntfilesegs > 1) {
		mergetfiles(tfilestr, ntfilesegs, 2, 2, (int(*)(unsigned char*,  unsigned char*)) strcmp, 1, 0);
		ntfilesegs = 1;
	}
	
	sprintf(buf, "%s\\i\\%llu\\rfIndex.bin", g_prgDir, dnum);
	
	if ((rfIndex = MBfopen(buf, "wb")) == NULL) {
		errorf("couldn't create rfIndex");
		releasetfile(tfilestr, ntfilesegs);
		return 2;
	}
	if ((tfile = opentfile(tfilestr, 0, "rb")) == NULL) {
		errorf("couldn't read temp file");
		fclose(rfIndex), MBremove(buf), releasetfile(tfilestr, ntfilesegs);
		return 2;
	}
	
	while ((c = getc(tfile)) != EOF) {
		putc(c, rfIndex);
	}
	fclose(rfIndex), fclose(tfile);
	releasetfile(tfilestr, ntfilesegs);
	
	raddtolastfnum(dnum, 0);
	
	return 0;
}

char rfireg(uint64_t dnum, char *fname, uint64_t fnum) { //! untested
	FILE *rfIndex, *trfIndex;
	unsigned char buf[MAX_PATH*4], baf[MAX_PATH*4], bif[MAX_PATH*4], done = 0;
	int i, c, d;
	
	if (dnum == 0) {
		errorf("dnum is zero in rfireg");
		return 0;
	}
	if (fnum == 0) {
		errorf("fnum in rdreg is 0");
		return 1;
	}
	
	sprintf(buf, "%s\\i\\%llu\\rfIndex.bin", g_prgDir, dnum);
	
	if (fname) {
		for (i = 0; fname[i] != '\0' && i != MAX_PATH*4; i++);
		if (i >= MAX_PATH*4) {
			errorf("path too long");
			MBremove(buf);
			return 1;
		}
	} if (i == 0 || fname == 0) {
		errorf("empty string");
		MBremove(buf);
		return 1;
	}
	
	if ((rfIndex = MBfopen(buf, "rb")) == NULL) {
		if (rfinit(dnum))
			return 2;
		return 0;
	}
	sprintf(baf, "%s\\trfIndex.bin", g_prgDir);
	if ((trfIndex = MBfopen(baf, "wb")) == NULL) {
		fclose(rfIndex), MBremove(buf);
		return 3;
	}
	while ((c = getc(rfIndex)) != EOF) {
		bif[0] = c;
		for (i = 1; (bif[i] = c = getc(rfIndex)) != '\0'; i++) {
			if (c == EOF) {
				fclose(rfIndex), MBremove(buf), fclose(trfIndex), MBremove(baf);
				errorf("EOF before end of string");
				return 4;
			}
		}
		if (i >= MAX_PATH*4) {
			fclose(rfIndex), MBremove(buf), fclose(trfIndex), MBremove(baf);
			errorf("path longer than MAX_PATH");
			return 5;
		}
		if (!done && strcmp(fname, bif) < 0) {
			term_fputs(fname, trfIndex);
			putull_pref(fnum, trfIndex);		
			done = 1;
		}
		term_fputs(bif, trfIndex);
		putc(c = getc(rfIndex), trfIndex);
		for (i = 0; i < c; putc(d = getc(rfIndex), trfIndex), i++) {
			if (d == EOF) {
				fclose(rfIndex), MBremove(buf), fclose(trfIndex), MBremove(baf);
				errorf("EOF before end of pref-num");
				return 6;
			}
		}
	}
	fclose(rfIndex), MBremove(buf);
	if (!done) {
		term_fputs(fname, trfIndex);
		putull_pref(fnum, trfIndex);
	}
	fclose(trfIndex);
	if (MBrename(baf, buf)) {
		MBremove(baf);
		errorf("rename failed");
		return 7;
	}
	
	raddtolastfnum(dnum, 1);
	return 0;
	
}

uint64_t rfiread(uint64_t dnum, char *fname) { // Returns file entry number if entry exists or zero if it doesn't exist.		//! not done

	return 0; //!
}

unsigned char crfireg(uint64_t dnum, twoslnk *regchn) {		//! not done
	rfinit(dnum);
	return 0; //!
}

char passfextra(FILE *src, FILE *dst, uint64_t ull) {
	int c;
	uint64_t ull2;
	if (ull == 0) {
		ull2 = fgetull_pref(src, &c);
		if (c != 0) {
			errorf("4 - fExtras num read failed: %d", c);
			return 1;
		}
		if (dst)
			putull_pref(ull2, dst);
		return 0;
	} else {
		errorf("unknown fExtras parameter");
		return 1;
	}
}

int regfiledates(uint64_t dnum, oneslnk *fnums) {
	unsigned char buf[MAX_PATH*4], buf2[MAX_PATH*4], nofile = 0;
	FILE *fextras, *dst;
	uint64_t ull1, ull2;
	int c;
	
	uint64_t ulltime = time(0);
	
	if (dnum == 0) {
		errorf("dnum is 0 in regfiledates");
		return 1;
	}
	if (!fnums) {
		errorf("no fnums");
		return 1;
	}
	
	oneslnk *lnk = fnums->next;
	ull1 = fnums->ull;
	while (lnk) {
		if (lnk->ull <= ull1) {
			errorf("fnums not ascending");
			return 1;
		}
		ull1 = lnk->ull;
		lnk = lnk->next;
	}
	
	sprintf(buf2, "%s\\i\\%llu\\fExtras.tmp", g_prgDir, dnum);
	if (!(dst = MBfopen(buf2, "wb"))) {
		errorf("failed to create fextras temp");
	}
	sprintf(buf, "%s\\i\\%llu\\fExtras.bin", g_prgDir, dnum);
	fextras = MBfopen(buf, "rb");
	if (!fextras) nofile = 1;
	
	while (fextras) {
		ull1 = fgetull_pref(fextras, &c);
		if (c != 0) {
			if (c == 1) {
				fclose(fextras);
				fextras = NULL;
				break;
			}
			errorf("fExtras num read failed: %d", c);
			fclose(fextras), fclose(dst), MBremove(buf2);
			return 1;
		}
		while (fnums && ull1 > fnums->ull) {
			putull_pref(fnums->ull, dst);
			putull_pref(0, dst);
			putull_pref(ulltime, dst);
			putc(0, dst);
			fnums = fnums->next;
		}
		putull_pref(ull1, dst);
		
		while (1) {
			ull2 = fgetull_pref(fextras, &c);
			if (c != 0) {
				if (c == ULL_READ_NULL)
					break;
				errorf("2 - fExtras num read failed: %d", c);
				fclose(fextras), fclose(dst), MBremove(buf2);
				return 1;
			}
			if (fnums && ull1 == fnums->ull && ull2 >= 0) {
				putull_pref(0, dst);
				if (ull2 == 0) {
					fgetull_pref(fextras, &c);
					if (c != 0) {
						errorf("3 fextras num read");
						fclose(fextras), fclose(dst), MBremove(buf2);
						return 1;
					}
				}
				putull_pref(ulltime, dst);
				fnums = fnums->next;
			} else {
				putull_pref(ull2, dst);
				if (passfextra(fextras, dst, ull2)) {
					errorf("passfextra failed");
					fclose(fextras), fclose(dst), MBremove(buf2);
					return 1;
				}
			}
		}
		if (fnums && ull1 == fnums->ull) {
			putull_pref(0, dst);
			putull_pref(ulltime, dst);
			fnums = fnums->next;
		}
		putc(0, dst);
	}
	
	while (fnums) {
		putull_pref(fnums->ull, dst);
		putull_pref(0, dst);
		putull_pref(ulltime, dst);
		putc(0, dst);
		fnums = fnums->next;
	}
	
	fclose(dst);
	
	char buf3[MAX_PATH*4];
	
	if (!nofile) {
		sprintf(buf3, "%s\\i\\%llu\\fExtras.bak", g_prgDir, dnum);
		MBremove(buf3);
		if (MBrename(buf, buf3)) {
			errorf("rename1 failed");
			errorf("%s->%s", buf, buf3);
			return 3;
		}
	}
	if (MBrename(buf2, buf)) {
		MBrename(buf3, buf);
		errorf("rename2 failed");
		return 3;
	}
	if (!nofile)
		MBremove(buf3);
	
	return 0;
}

//}

//{ file
unsigned char addtolastfnum(uint64_t dnum, long long num, uint64_t spot) { // 0 as num to just refresh from spot		//! not done
/*
	char buf[MAX_PATH*4], buf2[MAX_PATH*4], buf3[MAX_PATH*4], ch;
	int i, c;
	FILE *direc, *tdirec, *dIndex;
	uint64_t dnum, gap, pos = 0;

	sprintf(buf, "%s\\diRec.tmp", g_prgDir);
	if ((tdirec = MBfopen(buf, "wb")) == NULL) {
		errorf("failed to create diRec.tmp");
		return 1;
	}
	sprintf(buf2, "%s\\diRec.bin", g_prgDir);
	sprintf(buf3, "%s\\dIndex.bin", g_prgDir);

	if (((direc = MBfopen(buf2, "rb")) != NULL) && ((i = getc(direc)) != EOF)) {
		uint64_t olddnum = 0;
		
		for (; i > 0; i--) {
			olddnum *= 256;
			olddnum += c = getc(direc);
			if (c == EOF) {
				fclose(direc), fclose(tdirec), MBremove(buf), MBremove(buf2);
				errorf("EOF before end of number in diRec");
				return 2;
			}
		}
		if ((num < 0) && (olddnum <= -num)) {
			fclose(direc), fclose(tdirec), MBremove(buf), MBremove(buf2);
			errorf("lastdnum going less than one");
			return 6;
		}
		dnum = olddnum + num;
		putull_pref(dnum, tdirec);
		
		if ((gap = (long long) sqrt(dnum)) == (long long) sqrt(olddnum)) {
			if (num > 0) {
				if (dnum / gap == olddnum / gap) {
					while ((c = getc(direc)) != EOF) {
						putc(c, tdirec);
					} fclose(direc), MBremove(buf2);
				} else {
					while ((c = getc(direc)) != EOF) {
						for (putc(c, tdirec), i = c; i > 0 && (c = getc(direc)) != EOF; i--, putc(c, tdirec));
						for (putc((ch = getc(direc)), tdirec), i = 1, pos = 0; i++ <= ch && (c = getc(direc)) != EOF; pos *= 256, pos += c, putc(c, tdirec));
						if (c == EOF) {
							errorf("EOF before finished diRec entry (1)");
							fclose(direc), fclose(tdirec), MBremove(buf), MBremove(buf2);
							return 1;
						}
					} fclose(direc), MBremove(buf2);
					
					if ((dIndex = MBfopen(buf3, "rb")) != NULL) {
						fseek64(dIndex, pos, SEEK_SET);
						fseek(dIndex, (c = getc(dIndex)), SEEK_CUR);
						if (c == EOF) {
							errorf("EOF before directory in dIndex");
							fclose(dIndex), fclose(tdirec), MBremove(buf);
							return 1;
						}
						null_fgets(0, MAX_PATH*4, dIndex);
						
						for (pos = ftell64(dIndex), dnum = 0; (i = getc(dIndex)) != EOF; pos = ftell64(dIndex), dnum = 0) {
							for (; i > 0; i--) {
								dnum *= 256;
								dnum += c = getc(dIndex);
								if (c == EOF) {
									errorf("EOF before end of number in dIndex (1)");
									fclose(tdirec), MBremove(buf), fclose(dIndex);
									return 2;
								}
							}
							if (dnum % gap == 0) {
								putull_pref(dnum, tdirec);
								putull_pref(pos, tdirec);
							}
							
							if ((c = null_fgets(0, MAX_PATH*4, dIndex)) != 0) {
								if (c == 1) {
									errorf("EOF before null terminator in dIndex");
									fclose(tdirec), MBremove(buf), fclose(dIndex);
									return 2;
								} else if (c == 2) {
									errorf("too long string in dIndex");
									fclose(tdirec), MBremove(buf), fclose(dIndex);
									return 2;
								}
							}
						}
						fclose(dIndex);
					} else {
						errorf("Couldn't open dIndex");
						fclose(tdirec), MBremove(buf);
						return 1;
					}
				}
				fclose(tdirec), MBrename(buf, buf2);
				return 0;
			} else {
				olddnum = dnum;
				uint64_t tpos;
				while ((c = getc(direc)) != EOF) {
					for (ch = c, i = 1, dnum = 0; i++ <= ch && (c = getc(direc)) != EOF; dnum *= 256, dnum += c);
					for (ch = getc(direc), i = 1, tpos = 0; i++ <= ch && (c = getc(direc)) != EOF; tpos *= 256, tpos += c);
					if (c == EOF) {
						errorf("EOF before finished diRec entry (2)");
						fclose(direc), fclose(tdirec), MBremove(buf), MBremove(buf2);
						return 1;
					}
					if (dnum > spot || dnum > olddnum) {
						break;
					}
					pos = tpos;
					putull_pref(dnum, tdirec);
					putull_pref(pos, tdirec);
				}
			}
		}
		fclose(direc), MBremove(buf2);
		
	} else {
		if (direc != NULL)
			fclose(direc);
		MBremove(buf2);
		
		if ((dIndex = MBfopen(buf3, "rb")) == NULL) {
			fclose(tdirec), MBremove(buf);
			errorf("dIndex file not found");
			return 3;
		}
		while ((i = getc(dIndex)) != EOF) {
			for (dnum = 0; i > 0; i--) {
				dnum *= 256;
				dnum += c = getc(dIndex);
				if (c == EOF) {
					errorf("EOF before end of number");
					fclose(tdirec), MBremove(buf), fclose(dIndex);
					return 2;
				}
			}
			for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++) {
				if (c == EOF) {
					errorf("EOF before null terminator in dIndex");
					fclose(tdirec), MBremove(buf), fclose(dIndex);
					return 4;
				}
			}
			if (i == MAX_PATH*4) {
				errorf("too long string in dIndex");
				fclose(tdirec), MBremove(buf), fclose(dIndex);
				return 5;
			}
		}
		fclose(dIndex);
		
		putull_pref(dnum, tdirec);
		gap = sqrt(dnum);
	}
	
	if ((dIndex = MBfopen(buf3, "rb")) == NULL) {
		fclose(tdirec), MBremove(buf);
		errorf("dIndex file not found");
		return 3;
	}
	if (pos) {
		fseek64(dIndex, pos, SEEK_SET);
		fseek(dIndex, (c = getc(dIndex)), SEEK_CUR);
		
		for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in dIndex");
				fclose(tdirec), MBremove(buf), fclose(dIndex);
				return 4;
			}
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in dIndex");
			fclose(tdirec), MBremove(buf), fclose(dIndex);
			return 5;
		}
	}
	for (pos = ftell64(dIndex), dnum = 0; (i = getc(dIndex)) != EOF; pos = ftell64(dIndex), dnum = 0) {
		for (; i > 0; i--) {
			dnum *= 256;
			dnum += c = getc(dIndex);
			if (c == EOF) {
				errorf("EOF before end of number in dIndex (2)");
				fclose(tdirec), MBremove(buf), fclose(dIndex);
				return 2;
			}
		}
		if (dnum % gap == 0) {
			putull_pref(dnum, tdirec);
			putull_pref(pos, tdirec);
		}
		
		for (i = 0; (c = getc(dIndex)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == EOF) {
				errorf("EOF before null terminator in dIndex");
				fclose(tdirec), MBremove(buf), fclose(dIndex);
				return 2;
			}
		}
		if (i == MAX_PATH*4) {
			errorf("too long string in dIndex");
			fclose(tdirec), MBremove(buf), fclose(dIndex);
			return 2;
		}
	}
	fclose(dIndex), fclose(tdirec);
	MBrename(buf, buf2);
	return 0;
	*/
	return 0; //!
}

uint64_t getlastfnum(uint64_t dnum) {		//! not done
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
	return 0; //!
}

uint64_t finitpos(uint64_t dnum, uint64_t ifnum) { /*		//! not done
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

uint64_t fireg(uint64_t dnum, char *fname) {
	int c;
	
	oneslnk *fnamechn = malloc(sizeof(oneslnk));
	fnamechn->next = NULL;
	fnamechn->str = fname;
	
	c = cfireg(dnum, fnamechn);
	free(fnamechn);
	
	return c;
}

uint64_t fireg_old(uint64_t dnum, char *fname) {
	FILE *fIndex;
	unsigned char buf[MAX_PATH*4];
	int c, i, j;
	uint64_t lastfnum = 0;

	if (dnum == 0) {
		errorf("dnum is zero in fireg");
		return 0;
	}
	if (fname) {
		for (i = 0; fname[i] != '\0' && i != MAX_PATH*4; i++);
		if (i >= MAX_PATH*4) {
			errorf("fname too long");
			return 0;
		}
	} if (i == 0 || fname == 0) {
		errorf("empty string");
		return 0;
	}
	
	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dnum);
	if ((fIndex = MBfopen(buf, "rb+")) == NULL) {
		if ((fIndex = MBfopen(buf, "wb+")) == NULL) {
			errorf("fIndex file not created");
			return 0;
		}
	}
	if (lastfnum = getlastfnum(dnum)) {
		fseek(fIndex, 0, SEEK_END);
	} else {		
		while ((c = getc(fIndex)) != EOF) {		// assuming there are no entry number gaps
			if (fseek(fIndex, c, SEEK_CUR)) {
				errorf("fseek failed");
				fclose(fIndex);
				return 0;
			}
			
			if ((c = null_fgets(0, MAX_PATH*4, fIndex)) != 0) {
				errorf("fIndex read error: %d", c);
				fclose(fIndex);
				return 0;
			}
			
			while ((c = getc(fIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in fIndex");
					fclose(fIndex);
					return 0;
				}
				if (fseek(fIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(fIndex);
					return 0;
				}
			}
			lastfnum++;
		}
	}
	putull_pref(++lastfnum, fIndex);
	term_fputs(fname, fIndex);
	putc('\0', fIndex);		// to terminate the tag segment
	fclose(fIndex);
	rfireg(dnum, fname, lastfnum);
	addtolastfnum(dnum, 1, 0);
	return lastfnum;
}

char cfireadtag(uint64_t dnum, oneslnk *fnums, oneslnk **retfname, oneslnk **rettags, unsigned char presort) {	//! untested
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
			errorf("fIndex num read failed: %d", c);
			fclose(fIndex), presort? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
			return 1;
		}
		if (link1->ull > tnum) {
			if ((c = null_fgets(0, MAX_PATH*4, fIndex)) != 0) {
				errorf("fIndex read error: %d", c);
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
				errorf("fIndex read error: %d", c);
				fclose(fIndex), presort ? 0:killoneschn(fnums, 1), retfname? (link3->next = 0, killoneschn(link2, 0)):0, rettags? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			if (retfname) {
				link3 = link3->next = malloc(sizeof(oneslnk));
				if (!(link3->str = dupstr(buf, MAX_PATH*4, 0))) {
					errorf("failed to duplicate buf: %s", buf);
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
						errorf("fIndex tag read failed: %d", c);
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
		errorf("tried to read non-existent fnum: %d -- last read %d", link1->ull, tnum);
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
}

char *firead(uint64_t dnum, uint64_t fnum) {	//! untested // returns malloc memory		
	FILE *fIndex;
	unsigned char buf[MAX_PATH*4], *p;
	int c, i;
	uint64_t tnum;
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 0;
	}
	
	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dnum);
	if ((fIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("fIndex file not found (firead)");
		return 0;
	}
	
	fseek64(fIndex, finitpos(dnum, fnum), SEEK_SET);
	
	while (1) {
		tnum = fgetull_pref(fIndex, &c);
		if (c != 0) {
			if (c == 1)
				break;
			errorf("fIndex num read failed: %d", c);
			fclose(fIndex);
			return 0;
		}
		if (fnum > tnum) {
			if ((c = null_fgets(0, MAX_PATH*4, fIndex)) != 0) {
				errorf("fIndex read error: %d", c);
				fclose(fIndex);
				return 0;
			}
			while ((c = getc(fIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in fIndex");
					fclose(fIndex);
					return 0;
				}
				if (fseek(fIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(fIndex);
					return 0;
				}
			}
		} else if (fnum == tnum) {
			if ((c = null_fgets(buf, MAX_PATH*4, fIndex)) != 0) {
				errorf("fIndex read error: %d", c);
				fclose(fIndex);
				return 0;
			}
			fclose(fIndex);
			if ((p = malloc(strlen(buf)+1)) == 0) {
				errorf("malloc failed");
				return p;
			}
			for (i = 0; p[i] = buf[i]; i++);
			return p;
		} else
			break;
	}
	fclose(fIndex);
	return 0;
}

oneslnk *cfiread(uint64_t dnum, oneslnk *fnumchn) {
	int c;
	oneslnk *retlnk;
	
	retlnk = 0;
	c = cfireadtag(dnum, fnumchn, &retlnk, NULL, 0);
	
	if (!c) {
		return retlnk;
	} else {
		return NULL;
	}
}

unsigned char firmv(uint64_t dnum, uint64_t fnum) {		//! not done
	
	return 0; //!
}

int cfireg(uint64_t dnum, oneslnk *fnamechn) {
	FILE *fIndex;
	unsigned char buf[MAX_PATH*4];
	int c, i, j;
	uint64_t lastfnum = 0;
	oneslnk *link, *link2, *link3;
	twoslnk *numnamechn, *fnumname;
	
	
	if (dnum == 0) {
		errorf("dnum is zero in cfireg");
		return 0;
	}
	if (fnamechn == 0) {
		errorf("fnamehchn is null");
		return 1;
	}
	
	for (link = fnamechn; link != 0; link = link->next) {
		if (link->str) {
			for (i = 0; link->str[i] != '\0' && i != MAX_PATH*4; i++);
			if (i >= MAX_PATH*4) {
				errorf("path too long");
				return 1;
			}
		} if (i == 0 || link->str == 0) {
			errorf("empty string");
			return 1;
		}
	}
	
	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dnum);
	if ((fIndex = MBfopen(buf, "rb+")) == NULL) {
		if ((fIndex = MBfopen(buf, "wb+")) == NULL) {
			errorf("couldn't create fIndex (3)");
			errorf(buf);
			return 1;
		}
	}
	
	if (lastfnum = getlastfnum(dnum)) {
		fseek(fIndex, 0, SEEK_END);
	} else {		
		while ((c = getc(fIndex)) != EOF) {		// assuming there are no entry number gaps
			if (fseek(fIndex, c, SEEK_CUR)) {
				errorf("fseek failed");
				fclose(fIndex);
				return 0;
			}
			
			if ((c = null_fgets(0, MAX_PATH*4, fIndex)) != 0) {
				errorf("fIndex read error: %d", c);
				fclose(fIndex);
				return 0;
			}
			
			while ((c = getc(fIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in fIndex");
					fclose(fIndex);
					return 0;
				}
				if (fseek(fIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(fIndex);
					return 0;
				}
			}
			lastfnum++;
		}
	}
	
	fnumname = numnamechn = malloc(sizeof(twoslnk));
	numnamechn->u[0].str = 0;
	
	link2 = link3 = malloc(sizeof(oneslnk));
	for (i = 0, link = fnamechn; link != 0; link = link->next, i++) {
		putull_pref(++lastfnum, fIndex);
		term_fputs(link->str, fIndex);
		putc('\0', fIndex);		// to terminate the tag segment
		numnamechn = numnamechn->next = malloc(sizeof(twoslnk));
		numnamechn->u[0].str = link->str, numnamechn->u[1].ull = lastfnum;
		link2 = link2->next = malloc(sizeof(oneslnk));
		link2->ull = lastfnum;
	}
	fclose(fIndex);
	numnamechn->next = 0;
	numnamechn = fnumname->next, free(fnumname);
	link2->next = 0;
	link2 = link3->next, free(link3);
	
	crfireg(dnum, numnamechn);
	regfiledates(dnum, link2);
	
	killtwoschn(numnamechn, 3);
	killoneschn(link2, 1);
	
	addtolastfnum(dnum, i, 0);
	return 0;
}

oneslnk *ifiread(uint64_t dnum, uint64_t start, uint64_t intrvl) {	// returns malloc memory (the whole chain)
	FILE *fIndex;
	unsigned char buf[MAX_PATH*4];
	int c, i;
	uint64_t tnum;
	oneslnk *flnk, *lastlnk;
	
	if (dnum == 0) {
		errorf("dnum is 0");
		return 0;
	}
	if (!intrvl) {
		errorf("ifiread without intrvl");
		return 0;
	}
	
	sprintf(buf, "%s\\i\\%llu\\fIndex.bin", g_prgDir, dnum);
	if ((fIndex = MBfopen(buf, "rb")) == NULL) {
		errorf("fIndex file not found");
		return 0;
	}
	
	fseek64(fIndex, finitpos(dnum, start), SEEK_SET);
	
	while (1) {
		tnum = fgetull_pref(fIndex, &c);
		if (c != 0) {
			if (c == 1) {
				break;
			}
			errorf("1 - fIndex num read failed: %d", c);
			fclose(fIndex);
			return 0;
		}
		
		if (tnum < start) {
			for (i = 0; (c = getc(fIndex)) != '\0' && i < MAX_PATH*4; i++)
				if (c == EOF) {
					errorf("EOF before null terminator in fIndex");
					fclose(fIndex);
					return 0;
				}
			if (i == MAX_PATH*4) {
				errorf("too long string in fIndex");
				fclose(fIndex);
				return 0;
			}
			while ((c = getc(fIndex)) != 0) {
				if (c == EOF) {
					errorf("EOF before end of tags in fIndex");
					fclose(fIndex);
					return 0;
				}
				if (fseek(fIndex, c, SEEK_CUR)) {
					errorf("fseek failed");
					fclose(fIndex);
					return 0;
				}
			}
		} else {			// found start entry or over
			if ((flnk = malloc(sizeof(oneslnk))) == 0) {
				errorf("malloc failed");
				fclose(fIndex);
				return 0;
			} lastlnk = flnk;
			flnk->str = 0;

			while (intrvl > 0) {
				while (tnum > start) {
					if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
						errorf("malloc failed");
						fclose(fIndex), killoneschn(flnk, 0);
						return 0;
					}
					lastlnk->str = 0;
					start++;
					intrvl--;
					if (intrvl <= 0) {
						fclose(fIndex);
						lastlnk->next = 0;
						lastlnk = flnk->next;
						free(flnk);
						return lastlnk;
					}
				}
				
				for (i = 0; (buf[i] = c = getc(fIndex)) != '\0' && i < MAX_PATH*4; i++)
					if (c == EOF) {
						errorf("EOF before null terminator in fIndex");
						lastlnk->next = 0, fclose(fIndex), killoneschn(flnk, 0);
						return 0;
					}
				if (i == MAX_PATH*4) {
					errorf("too long string in fIndex");
					lastlnk->next = 0, fclose(fIndex), killoneschn(flnk, 0);
					return 0;
				}
				while ((c = getc(fIndex)) != 0) {
					if (c == EOF) {
						errorf("EOF before end of tags in fIndex");
						lastlnk->next = 0, fclose(fIndex), killoneschn(flnk, 0);
						return 0;
					}
					if (fseek(fIndex, c, SEEK_CUR)) {
						errorf("fseek failed");
						lastlnk->next = 0, fclose(fIndex), killoneschn(flnk, 0);
						return 0;
					}
				}
				
				if ((lastlnk = lastlnk->next = malloc(sizeof(oneslnk))) == 0) {
					errorf("malloc failed");
					lastlnk->next = 0, fclose(fIndex), killoneschn(flnk, 0);
					return 0;
				}
				lastlnk->str = malloc(i+1);
				snprintf(lastlnk->str, i+1, "%s", buf);
				start++;
				intrvl--;
				
				tnum = fgetull_pref(fIndex, &c);
				if (c != 0) {
					if (c == 1)
						break;
					errorf("2 - fIndex num read failed: %d", c);
					lastlnk->next = 0, fclose(fIndex), killoneschn(flnk, 0);
					return 0;
				}
			} fclose(fIndex);
			
			lastlnk->next = 0;
			lastlnk = flnk->next;
			free(flnk);
			return lastlnk;
		}
	}
	fclose(fIndex);
	return 0;
}

unsigned char addremfnumctagc(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *remtagnums) {		//! untested
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
		errorf("couldn't create i\\%llu\\fIndex.tmp", dnum);
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
			errorf("fIndex num read failed: %d", c);
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
					errorf("fIndex tag read failed: %d", c);
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
					errorf("fIndex tag read failed: %d", c);
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
}

unsigned char addfnumctag(uint64_t dnum, oneslnk *fnums, uint64_t tagnum) {	//!untested
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
			errorf("1-read failed: %d", c);
			(empty?0:fclose(rfIndex)), fclose(tfile), releasetfile(s, 1), fclose(tfile2), releasetfile(s2, 1);
			return 4;
		}
		if (!firecfin && !keep) {
			if ((c = null_fgets(buf2, MAX_PATH*4, rfIndex)) != 0) {
				if (c == 1) {
					firecfin = 1;
				} else {
					errorf("2-read failed: %d", c);
					(empty?0:fclose(rfIndex)), fclose(tfile), releasetfile(s, 1), fclose(tfile2), releasetfile(s2, 1);
					return 4;
				}
			} else {
				if ((c = pref_fgets(0, 9, rfIndex)) != 0) {
					errorf("3-read failed: %d", c);
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
//errorf("file: %s \ntime: %llu\nreal: %s", buf3, ull, buf6);
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
			errorf("4-read failed: %d", c);
			fclose(tfile2), releasetfile(s2, 1), link->next = 0, killoneschn(flink, 0);
			return 4;
		}
		fgetull_pref(tfile2, &c);
		
//uint64_t ull = fgetull_pref(tfile2, &c);
//errorf("2: file: %s \n time: %llu", buf, ull);
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
	
	errorf("i: %d, j: %d", i, j);
	
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
	
	errorf("i: %d, j: %d", i, j);
	
	buf[i-j-!!quotes] = '\0';
	
	j = 0;
	
	while (j < i) j++, (*sp)++;

errorf("s: %s", s);
errorf("buf: %s", buf);
	
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
	errorf("alias: %s", aliasstack->u[1].str);
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
	
	search_cleanup1:
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
		errorf("unknown srexp (%d) (srexpvalue)", exprtype);
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
			errorf("fIndex tag num read failed: %d", c);
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
			errorf("fIndex num read failed: %d", c);
			fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endsearch(sstruct);
			return 0;
		}
		if ((c = null_fgets(buf, MAX_PATH*4, fIndex)) != 0) {
			errorf("fIndex read error: %d", c);
			fclose(fIndex), fclose(tfile), releasetfile(tfstr, 1), endsearch(sstruct);
			return 0;
		}
		c = 0;
		if (exts != 0) {
// errorf("buf: %s", buf);
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
				errorf("tfile num read failed: %d", c);
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
			errorf("tfile num read failed: %d", c);
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
					errorf("2 - tfile num read failed: %d", c);
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
			errorf("aliasentryto fgetull_pref: %d", c);
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
				errorf("null_fgets: %d", c);
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
				errorf("alias code read failed: %d", c);
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
				errorf("null_fgets: %d", c);
				fclose(tAlias);
				return 0;
			}
			tnum = fgetull_pref(tAlias, &c);
			if (c != 0) {
				errorf("tnumfromalias - tAlias num read failed: %d", c);
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
					errorf("tAlias num read failed: %d", c);
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
				errorf("null_fgets: %d", c);
				fclose(tAlias);
				return 1;
			}
			tnum = fgetull_pref(tAlias, &c);
			if (c != 0) {
				errorf("tnumfromalias - tAlias num read failed: %d", c);
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
					errorf("tAlias num read failed: %d", c);
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
				errorf("tIndex num read failed: %d", c);
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
						errorf("tIndex tag read failed: %d", c);
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
							errorf("tIndex tag read failed: %d", c);
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
							errorf("tIndex tag read failed: %d", c);
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
							errorf("tIndex tag read failed: %d", c);
							fclose(tIndex), fclose(ttIndex), MBremove(baf), killoneschn(fnums, 1), faddtagnums? killoneschn(faddtagnums, 1) : 0, fremtagnums? killoneschn(fremtagnums, 1) : 0;
							return 1;
						}
						putull_pref(tnum, ttIndex);
					} putc('\0', ttIndex);
				}
				while (remtagnums && remtagnums->ull == lasttnum)
					remtagnums = remtagnums->next;
				while (addtagnums && addtagnums->ull == lasttnum) {
					errorf("add matched: %llu", lasttnum);
					addtagnums = addtagnums->next;
				}
			}
		}
		if (!ALLOW_TAGGING_NOTHING && (addtagnums || remtagnums)) {
			errorf("tried to add or remove fnums to non-existent tagnum");
			if (addtagnums) errorf("addtagnums->ull: %llu", addtagnums->ull);
			if (remtagnums) errorf("remtagnums->ull: %llu", remtagnums->ull);
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
				errorf("regtagnames->str too long -- max: %d", MAX_ALIAS*4);
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
		errorf("%s -> %s", buf, bif);
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
			errorf("crtreg failed: %d", c);
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
			errorf("tIndex num read failed: %d", c);
			fclose(tIndex), presort? 0:killoneschn(tagnums, 1), rettagname? killoneschn(link2, 0):0, retfnums? killoneschnchn(link4, 1):0;
			return 1;
		}
		if (link1->ull > tnum) {
			if ((c = null_fgets(0, MAX_ALIAS*4, tIndex)) != 0) {
				errorf("tIndex read error: %d", c);
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
				errorf("tIndex read error: %d", c);
				fclose(tIndex), presort ? 0:killoneschn(tagnums, 1), rettagname? (link3->next = 0, killoneschn(link2, 0)):0, retfnums? (link5->next = 0, killoneschnchn(link4, 1)):0;
				return 1;
			}
			if (rettagname) {
				link3 = link3->next = malloc(sizeof(oneslnk));
				if (!(link3->str = dupstr(buf, MAX_ALIAS*4, 0))) {
					errorf("failed to duplicate buf: %s", buf);
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
						errorf("tIndex tag read failed: %d", c);
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
		errorf("tried to read non-existent tagnum: %d -- last read %d", link1->ull, tnum);
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
			errorf("couldn't create %s", buf2);
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
			errorf("couldn't create %s", buf4);
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