#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef MAX_PATH
//#define MAX_PATH 1040
#endif

#include <stdio.h>

typedef struct dirstruct {
	char firstread;
	HANDLE search;
	WIN32_FIND_DATAW data;
} DIRSTRUCT;

typedef struct subdirstruct {
	DIRSTRUCT *ds;
	char *prepend;
} SUBDIRSTRUCT;
 
int fseek64(FILE *file, long long offset, int origin);

long long ftell64(FILE *file);

FILE *MBfopen(char const *buf, char const *mode);

int MBrename(char const *oldname, char const *newname);

int MBremove(char const *path);

int casestrcmp(char const *str1, char const *str2);

char make_directory(char const *path);

char checkfiletype(char const *fname);

unsigned long long getfilemodified(char const *fname);

char existsdir(char const *fname);

char *mb_from_wide(wchar_t const *wbuf);

wchar_t *wide_from_mb(char const *buf);



DIRSTRUCT *diropen(char const *dir);

char dirread(DIRSTRUCT *dirp, char const *buf);

int dirclose(DIRSTRUCT *dirp);