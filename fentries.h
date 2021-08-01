#ifndef _FENTRIES_H
#define _FENTRIES_H

#include "stringchains.h"

#include <stdio.h>

typedef struct searchexpr {
	unsigned short exprtype;
	struct {
		struct searchexpr *expr1;
		union {
			struct searchexpr *expr2;
			unsigned long long refnum;
		};
	};
} SREXP;

typedef struct srexplist {
	SREXP *expr;
	struct srexplist *next;
} SREXPLIST;

typedef struct superexpstack {
	SREXP *expr;
	unsigned long long subexp_layers;
	struct superexpstack *next;
} SUPEXPSTACK;

typedef struct tagnumstruct {
	unsigned long long tagnum;
	unsigned char state;
} TAGNUMNODE;

typedef struct sstruct {
	TAGNUMNODE *tagnumarray;
	unsigned long ntagnums;
	struct searchexpr *rootexpr;
	unsigned long long dnum;
} SEARCHSTRUCT;

struct subdirentry {
	char *subdirstr;
};

unsigned long long rmiread(char *entrystr);

char rmirmv(char *entrystr);

char rmirer(unsigned long long source, char *dest);

char rmirmvnum(unsigned long long inum);

char crmireg(twoslnk *regchn);

char crmirer(oneslnk *rmchn, twoslnk *addchn);

char crmirmvnum(oneslnk *rmchn);

char mifrmv(unsigned long long inum);

char cmifrmv(oneslnk *inumchm);

char setmilastchecked(unsigned long long inum, unsigned long long ulltime);

unsigned long long getmilastchecked(unsigned long long inum);

unsigned long long getlastminum(void);

unsigned long long mireg(char *entrystr);

char *miread(unsigned long long inum);

char mirmv(unsigned long long inum);

char mirer(unsigned long long inum, char *entrystr);

int cmireg(oneslnk *entrystrchn);

oneslnk *cmiread(oneslnk *inumchn);

int cmirmv(oneslnk *inumchn);

int cmirer(twoslnk *rerchn);

oneslnk *imiread(unsigned long long start, unsigned long long intrvl);

void verMII(void);




char rdreg(unsigned long long minum, char *dpath, unsigned long long dnum);

unsigned long long rdread(unsigned long long minum, char *dpath);

char rdrmv(unsigned long long minum, char *dpath);

char rdrer(unsigned long long minum, unsigned long long source, char *dest);

char rdrmvnum(unsigned long long minum, unsigned long long dnum);

char crdreg(unsigned long long minum, twoslnk *regchn);

char crdrer(unsigned long long minum, oneslnk *rmchn, twoslnk *addchn);

char crdrmvnum(unsigned long long minum, oneslnk *rmchn);

char dfrmv(unsigned long long minum, unsigned long long dnum);

char cdfrmv(unsigned long long minum, oneslnk *dnumchm);

unsigned long long getlastdnum(unsigned long long minum);

unsigned long long dreg(unsigned long long minum, char *dpath);

char *dread(unsigned long long minum, unsigned long long dnum);

char drmv(unsigned long long minum, unsigned long long dnum);

char drer(unsigned long long minum, unsigned long long dnum, char *dpath);

int cdreg(unsigned long long minum, oneslnk *dpathchn);

oneslnk *cdread(unsigned long long minum, oneslnk *dnumchn);

int cdrmv(unsigned long long minum, oneslnk *dnumchn);

int cdrer(unsigned long long minum, twoslnk *rerchn);

oneslnk *idread(unsigned long long minum, unsigned long long start, unsigned long long intrvl);

void verDI(unsigned long long minum);




unsigned long long getlastsdnum(unsigned long long minum);

oneslnk *isdread(unsigned long long minum, unsigned long long start, unsigned long long intrvl);	// returns malloc memory (the whole chain)

/*

char raddtolastfnum(unsigned long long dnum, long long num);

char rfinit(unsigned long long dnum);

char rfireg(unsigned long long dnum, char *fname, unsigned long long fnum);

unsigned long long rfiread(unsigned long long dnum, char *fname);

unsigned char crfireg(unsigned long long dnum, twoslnk *regchn);

unsigned char addtolastfnum(unsigned long long dnum, long long num, unsigned long long spot);

unsigned long long getlastfnum(unsigned long long dnum);

unsigned long long finitpos(unsigned long long dnum, unsigned long long ifnum);

unsigned long long fireg(unsigned long long dnum, char *fname);

char cfireadtag(unsigned long long dnum, oneslnk *fnums, oneslnk **retfname, oneslnk **rettags, unsigned char presort);

char *firead(unsigned long long dnum, unsigned long long fnum);

oneslnk *cfiread(unsigned long long dnum, oneslnk *fnumchn);

unsigned char firmv(unsigned long long dnum, unsigned long long fnum);

int cfireg(unsigned long long dnum, oneslnk *fnamechn);

oneslnk *ifiread(unsigned long long dnum, unsigned long long start, unsigned long long intrvl);

unsigned char addremfnumctagc(unsigned long long dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *remtagnums);

unsigned char addfnumctag(unsigned long long dnum, oneslnk *fnums, unsigned long long tagnum);

unsigned long long pfireg(char *fpath, unsigned char flag);

unsigned char dirfreg(char *path, unsigned char flag);

SEARCHSTRUCT *initftagsearch(char *searchstr, unsigned char *retarg, unsigned long long dnum);

int srexpvalue(SREXP *sexp, SEARCHSTRUCT *sstruct);

unsigned char ftagcheck(FILE *file, unsigned long long fnum, SEARCHSTRUCT *sstruct);

void killsearchexpr(struct searchexpr *sexp);

void endsearch(SEARCHSTRUCT *sstruct);

char *ffireadtagext(unsigned long long dnum, char *searchstr, char *exts);

unsigned long long getframount(char *tfstr);

unsigned long long frinitpos(char *tfstr, unsigned long long inpos, unsigned long long *retnpos);

oneslnk *ifrread(char *tfstr, unsigned long long start, unsigned long long intrvl);

char reftaliasrec(unsigned long long dnum);

unsigned long long tainitpos(unsigned long long dnum, char *aliasstr);

char aliasentryto(FILE *fromfile, FILE *tofile, unsigned long long aliascode);

char crtreg(unsigned long long dnum, oneslnk *tagnames, oneslnk *tagnums);

char rtreg(unsigned long long dnum, char *tname, unsigned long long tagnum);

oneslnk *tnumfromalias(unsigned long long dnum, oneslnk *aliaschn, oneslnk **retalias);

unsigned long long tinitpos(unsigned long long dnum, unsigned long long ifnum);

unsigned char addtolasttnum(unsigned long long dnum, long long num, unsigned long long spot);

unsigned long long getlasttnum(unsigned long long dnum);

char tcregaddrem(unsigned long long dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *regtagnames, oneslnk **newtagnums, oneslnk *remtagnums);

unsigned long long treg(unsigned long long dnum, char *tname, oneslnk *fnums);

char ctread(unsigned long long dnum, oneslnk *tagnums, oneslnk **rettagname, oneslnk **retfnums, unsigned char presort);

char twowaytcregaddrem(unsigned long long dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *regtagnames, oneslnk *remtagnums, unsigned char presort);
*/

#endif