#pragma once

extern "C" {
#include "stringchains.h"
}

#include <stdio.h>
#include <stdint.h>

#include <filesystem>
#include <forward_list>

uint64_t rmiread(char *entrystr);

char rmirmv(char *entrystr);

char rmirer(uint64_t source, char *dest);

char rmirmvnum(uint64_t inum);

char crmireg(twoslnk *regchn);

char crmirer(oneslnk *rmchn, twoslnk *addchn);

char crmirmvnum(oneslnk *rmchn);

char mifrmv(uint64_t inum);

char cmifrmv(oneslnk *inumchm);

char setmilastchecked(uint64_t inum, uint64_t ulltime);

uint64_t getmilastchecked(uint64_t inum);

uint64_t getlastminum(void);

uint64_t mireg(char *entrystr);

char *miread(uint64_t inum);

char mirmv(uint64_t inum);

char mirer(uint64_t inum, char *entrystr);

int cmireg(oneslnk *entrystrchn);

oneslnk *cmiread(oneslnk *inumchn);

int cmirmv(oneslnk *inumchn);

int cmirer(twoslnk *rerchn);

std::shared_ptr<std::forward_list<std::string>> imiread(uint64_t start, uint64_t intrvl);

void verMII(void);




char rdreg(uint64_t minum, char *dpath, uint64_t dnum);

uint64_t rdread(uint64_t minum, char *dpath);

char rdrmv(uint64_t minum, char *dpath);

char rdrer(uint64_t minum, uint64_t source, char *dest);

char rdrmvnum(uint64_t minum, uint64_t dnum);

char crdreg(uint64_t minum, twoslnk *regchn);

char crdrer(uint64_t minum, oneslnk *rmchn, twoslnk *addchn);

char crdrmvnum(uint64_t minum, oneslnk *rmchn);

char dfrmv(uint64_t minum, uint64_t dnum);

char cdfrmv(uint64_t minum, oneslnk *dnumchm);

uint64_t getlastdnum(uint64_t minum);

//uint64_t dreg(uint64_t minum, char *dpath);

uint64_t dreg(const uint64_t &minum, const std::filesystem::path &dpath);

char *dread(uint64_t minum, uint64_t dnum);

char drmv(uint64_t minum, uint64_t dnum);

char drer(uint64_t minum, uint64_t dnum, char *dpath);

int cdreg(uint64_t minum, oneslnk *dpathchn);

oneslnk *cdread(uint64_t minum, oneslnk *dnumchn);

int cdrmv(uint64_t minum, oneslnk *dnumchn);

int cdrer(uint64_t minum, twoslnk *rerchn);

std::shared_ptr<std::forward_list<std::filesystem::path>> idread(uint64_t minum, uint64_t start, uint64_t intrvl);

void verDI(uint64_t minum);




uint64_t getlastsdnum(uint64_t minum);

oneslnk *isdread(uint64_t minum, uint64_t start, uint64_t intrvl);	// returns malloc memory (the whole chain)

/*

char raddtolastfnum(uint64_t dnum, long long num);

char rfinit(uint64_t dnum);

char rfireg(uint64_t dnum, char *fname, uint64_t fnum);

uint64_t rfiread(uint64_t dnum, char *fname);

unsigned char crfireg(uint64_t dnum, twoslnk *regchn);

unsigned char addtolastfnum(uint64_t dnum, long long num, uint64_t spot);

uint64_t getlastfnum(uint64_t dnum);

uint64_t finitpos(uint64_t dnum, uint64_t ifnum);

uint64_t fireg(uint64_t dnum, char *fname);

char cfireadtag(uint64_t dnum, oneslnk *fnums, oneslnk **retfname, oneslnk **rettags, unsigned char presort);

char *firead(uint64_t dnum, uint64_t fnum);

oneslnk *cfiread(uint64_t dnum, oneslnk *fnumchn);

unsigned char firmv(uint64_t dnum, uint64_t fnum);

int cfireg(uint64_t dnum, oneslnk *fnamechn);

oneslnk *ifiread(uint64_t dnum, uint64_t start, uint64_t intrvl);

unsigned char addremfnumctagc(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *remtagnums);

unsigned char addfnumctag(uint64_t dnum, oneslnk *fnums, uint64_t tagnum);

uint64_t pfireg(char *fpath, unsigned char flag);

unsigned char dirfreg(char *path, unsigned char flag);

SEARCHSTRUCT *initftagsearch(char *searchstr, unsigned char *retarg, uint64_t dnum);

int srexpvalue(SREXP *sexp, SEARCHSTRUCT *sstruct);

unsigned char ftagcheck(FILE *file, uint64_t fnum, SEARCHSTRUCT *sstruct);

void killsearchexpr(struct searchexpr *sexp);

void endsearch(SEARCHSTRUCT *sstruct);

char *ffireadtagext(uint64_t dnum, char *searchstr, char *exts);

uint64_t getframount(char *tfstr);

uint64_t frinitpos(char *tfstr, uint64_t inpos, uint64_t *retnpos);

oneslnk *ifrread(char *tfstr, uint64_t start, uint64_t intrvl);

char reftaliasrec(uint64_t dnum);

uint64_t tainitpos(uint64_t dnum, char *aliasstr);

char aliasentryto(FILE *fromfile, FILE *tofile, uint64_t aliascode);

char crtreg(uint64_t dnum, oneslnk *tagnames, oneslnk *tagnums);

char rtreg(uint64_t dnum, char *tname, uint64_t tagnum);

oneslnk *tnumfromalias(uint64_t dnum, oneslnk *aliaschn, oneslnk **retalias);

uint64_t tinitpos(uint64_t dnum, uint64_t ifnum);

unsigned char addtolasttnum(uint64_t dnum, long long num, uint64_t spot);

uint64_t getlasttnum(uint64_t dnum);

char tcregaddrem(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *regtagnames, oneslnk **newtagnums, oneslnk *remtagnums);

uint64_t treg(uint64_t dnum, char *tname, oneslnk *fnums);

char ctread(uint64_t dnum, oneslnk *tagnums, oneslnk **rettagname, oneslnk **retfnums, unsigned char presort);

char twowaytcregaddrem(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *regtagnames, oneslnk *remtagnums, unsigned char presort);
*/