#pragma once

extern "C" {
#include "stringchains.h"
}

#include <stdio.h>
#include <stdint.h>

#include <filesystem>
#include <forward_list>

// TODO: replace with something better later
bool initSessionHandler(const std::filesystem::path &prgDir);

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

uint64_t getLastMINum(void);

uint64_t miReg(char *entrystr);

char *miread(uint64_t inum);

char miRemove(uint64_t inum);

char mirer(uint64_t inum, char *entrystr);

int chainMiReg(oneslnk *entrystrchn);

oneslnk *chainMiRead(oneslnk *inumchn);

int chainMiRmv(oneslnk *inumchn);

int chainMiReroute(twoslnk *rerchn);

std::shared_ptr<std::forward_list<std::string>> intervalMiRead(uint64_t start, uint64_t intrvl);

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

uint64_t getLastDNum(uint64_t minum);

//uint64_t dReg(uint64_t minum, char *dpath);

uint64_t dReg(const uint64_t &minum, const std::filesystem::path &dpath);

char *dRead(uint64_t minum, uint64_t dnum);

char dRemove(uint64_t minum, uint64_t dnum);

char dReroute(uint64_t minum, uint64_t dnum, char *dpath);

int cDReg(uint64_t minum, oneslnk *dpathchn);

oneslnk *chainDRead(uint64_t minum, oneslnk *dnumchn);

int chainDRemove(uint64_t minum, oneslnk *dnumchn);

int chainDReroute(uint64_t minum, twoslnk *rerchn);

std::shared_ptr<std::forward_list<std::filesystem::path>> intervalDRead(uint64_t minum, uint64_t start, uint64_t intrvl);

void verDI(uint64_t minum);




uint64_t getLastSubDirNum(uint64_t minum);

oneslnk *intervalSubDirRead(uint64_t minum, uint64_t start, uint64_t intrvl);	// returns malloc memory (the whole chain)

/*

char raddtolastfnum(uint64_t dnum, long long num);

char rfinit(uint64_t dnum);

char rfireg(uint64_t dnum, char *fname, uint64_t fnum);

uint64_t rfiread(uint64_t dnum, char *fname);

unsigned char crfireg(uint64_t dnum, twoslnk *regchn);

unsigned char addtolastfnum(uint64_t dnum, long long num, uint64_t spot);

uint64_t getlastfnum(uint64_t dnum);

uint64_t finitpos(uint64_t dnum, uint64_t ifnum);

uint64_t fileReg(uint64_t dnum, char *fname);

char chainFileReadTag(uint64_t dnum, oneslnk *fnums, oneslnk **retfname, oneslnk **rettags, unsigned char presort);

char *fileRead(uint64_t dnum, uint64_t fnum);

oneslnk *chainFileRead(uint64_t dnum, oneslnk *fnumchn);

unsigned char fileRemove(uint64_t dnum, uint64_t fnum);

int cfireg(uint64_t dnum, oneslnk *fnamechn);

oneslnk *ifiread(uint64_t dnum, uint64_t start, uint64_t intrvl);

unsigned char addRemoveFileNumTagChain(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *remtagnums);

unsigned char addFileNumChainTag(uint64_t dnum, oneslnk *fnums, uint64_t tagnum);

uint64_t pfireg(char *fpath, unsigned char flag);

unsigned char dirfreg(char *path, unsigned char flag);

SEARCHSTRUCT *initFileTagSearch(char *searchstr, unsigned char *retarg, uint64_t dnum);

int searchExprValue(SREXP *sexp, SEARCHSTRUCT *sstruct);

unsigned char fileTagCheck(FILE *file, uint64_t fnum, SEARCHSTRUCT *sstruct);

void killSearchExpr(struct searchexpr *sexp);

void endsearch(SEARCHSTRUCT *sstruct);

char *ffiReadTagExt(uint64_t dnum, char *searchstr, char *exts);

uint64_t getframount(char *tfstr);

uint64_t frinitpos(char *tfstr, uint64_t inpos, uint64_t *retnpos);

oneslnk *ifrread(char *tfstr, uint64_t start, uint64_t intrvl);

char refreshTagAliasRecord(uint64_t dnum);

uint64_t tagAliasInitPos(uint64_t dnum, char *aliasstr);

char aliasentryto(FILE *fromfile, FILE *tofile, uint64_t aliascode);

char crtreg(uint64_t dnum, oneslnk *tagnames, oneslnk *tagnums);

char reverseTagReg(uint64_t dnum, char *tname, uint64_t tagnum);

oneslnk *tagNumFromAlias(uint64_t dnum, oneslnk *aliaschn, oneslnk **retalias);

uint64_t tagInitPos(uint64_t dnum, uint64_t ifnum);

unsigned char addtolasttnum(uint64_t dnum, long long num, uint64_t spot);

uint64_t getlasttnum(uint64_t dnum);

char tcregaddrem(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *regtagnames, oneslnk **newtagnums, oneslnk *remtagnums);

uint64_t tagChainReg(uint64_t dnum, char *tname, oneslnk *fnums);

char chainTagRead(uint64_t dnum, oneslnk *tagnums, oneslnk **rettagname, oneslnk **retfnums, unsigned char presort);

char twoWayTagChainRegAddRem(uint64_t dnum, oneslnk *fnums, oneslnk *addtagnums, oneslnk *regtagnames, oneslnk *remtagnums, unsigned char presort);
*/