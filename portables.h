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

FILE *MBfopen(char *buf, char *mode);

int MBrename(char *oldname, char *newname);

int MBremove(char *path);

void errorf(char *, ...);

int casestrcmp(char *str1, char *str2);

char make_directory(char *path);

char checkfiletype(char *fname);

unsigned long long getfilemodified(char *fname);

char existsdir(char *fname);

char *mb_from_wide(wchar_t *wbuf);

wchar_t *wide_from_mb(char *buf);



DIRSTRUCT *diropen(char *dir);

char dirread(DIRSTRUCT *dirp, char *buf);

int dirclose(DIRSTRUCT *dirp);