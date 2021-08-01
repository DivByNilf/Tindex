#include <sys/stat.h>
#include <unicode/ustring.h>
#include "unicode/utypes.h"
#include <stdio.h>

#include "portables.h"
#include "breakpath.h"

#include <windows.h>
#include <shlobj.h>

void errorf(char *, ...);

int fseek64(FILE *file, long long offset, int origin) {
	return _fseeki64(file, offset, origin);
}

long long ftell64(FILE *file) {
	return _ftelli64(file);
}

char *mb_from_wide(wchar_t *wbuf) {
	unsigned long long len;
	char *buf, *buf2;
	
	len = 0;
	while (wbuf[len] != L'\0') len++;
	
	if (!(buf = malloc(len*4))) {
		printf("malloc failed");
		return 0;
	}
	
	if ((WideCharToMultiByte(65001, 0, wbuf, -1, buf, len*4, NULL, NULL)) == 0) {
		errorf("WideCharToMultiByte Failed");
	}
	
	len = 0;
	while (buf[len] != '\0') len++;
	buf2 = malloc(len+1);
	
	len = 0;
	while (buf[len] != '\0') buf2[len] = buf[len], len++;
	buf2[len] = '\0';
	
	free(buf);
	return buf2;
}

wchar_t *wide_from_mb(char *buf) {
	unsigned long long len;
	wchar_t *wbuf, *wbuf2;
	
	len = 0;
	while (buf[len] != '\0') len++;
	
	if (!(wbuf = malloc(len*2))) {
		printf("malloc failed");
		return 0;
	}
	
	if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf, len*2)) == 0) {
		errorf("MultiByteToWideChar Failed");
	}
	
	len = 0;
	while (wbuf[len] != L'\0') len++;
	wbuf2 = malloc((len+1)*2);
	
	len = 0;
	while (wbuf[len] != '\0') wbuf2[len] = wbuf[len], len++;
	wbuf2[len] = '\0';
	
	free(wbuf);
	return wbuf2;
}

FILE *MBfopen(char *buf, char *mode) {
	wchar_t wbuf[MAX_PATH*2], wmode[5];
	FILE *f;
	static int inside;
	
	if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf, MAX_PATH*2)) == 0) {
		return 0;
	}
	if ((MultiByteToWideChar(65001, 0, mode, -1, wmode, 5)) == 0) {
		return 0;
	}
	f = _wfopen(wbuf, wmode);
	if (f == NULL && (mode[0] == 'w' || mode[0] == 'a') && !inside) {
		inside = 1;
		char dir[MAX_PATH*4];
		breakpathdf(buf, dir, NULL);
		make_directory(dir);
		f = _wfopen(wbuf, wmode);
		inside = 0;
	}
	return f;
}

int MBrename(char *oldname, char *newname) {
	wchar_t woldname[MAX_PATH*2], wnewname[MAX_PATH*2];
	if ((MultiByteToWideChar(65001, 0, oldname, -1, woldname, MAX_PATH*2)) == 0) {
		return 6;
	}
	if ((MultiByteToWideChar(65001, 0, newname, -1, wnewname, MAX_PATH*2)) == 0) {
		return 6;
	}
	return _wrename(woldname, wnewname);
}

int MBremove(char *path) {
	wchar_t wpath[MAX_PATH*2];
	
	if ((MultiByteToWideChar(65001, 0, path, -1, wpath, MAX_PATH*2)) == 0) {
		return 0;
	}
	return _wremove(wpath);
}

int casestrcmp(char *str1, char *str2) {
	UChar *ustr1, *ustr2;
	UErrorCode error = 0;
	int len1, len2, c;
	ustr1 = malloc((len1 = strlen(str1)+1)*sizeof(UChar));
	ustr2 = malloc((len2 = strlen(str2)+1)*sizeof(UChar));
	u_strFromUTF8(ustr1, len1, 0, str1, -1, &error);
	if (U_FAILURE(error)) {
		free(ustr1), free(ustr2);
		errorf("u_strFromUTF8 failed 1: %d", error);
		errorf("str1: %s", str1); 
		return 0;
	}
	u_strFromUTF8(ustr2, len2, 0, str2, -1, &error);
	if (U_FAILURE(error)) {
		free(ustr1), free(ustr2);
		errorf("u_strFromUTF8 failed 2: %d", error);
		errorf("str2: %s", str2); 
		return 0;
	}
	if (len2 > len1)
		len1 = len2;
	c = u_strncasecmp(ustr1, ustr2, len1, 0);
	free(ustr1), free(ustr2);
	return c;
}

char make_directory(char *path) {
	wchar_t wpath[MAX_PATH*2];
	
errorf("making dir %s", path);
	if ((MultiByteToWideChar(65001, 0, path, -1, wpath, MAX_PATH*2)) == 0) {
		return 1;
	}
	if (SHCreateDirectoryExW(NULL, wpath, 0)) {	// will create nested
		return 0;
	} else {
		return 2;
	}	
}

char checkfiletype(char *fname) {
	wchar_t wpath[MAX_PATH*2];
	struct _stat ws_stat;
	
	if ((MultiByteToWideChar(65001, 0, fname, -1, wpath, MAX_PATH*2)) == 0) {
		return 1;
	}
	
	_wstat(wpath, &ws_stat);
	
	if (_S_IFDIR & ws_stat.st_mode)
		return 1;
	else if (_S_IFREG & ws_stat.st_mode)
		return 2;
	else
		return 0;
}

unsigned long long getfilemodified(char *fname) {
	wchar_t wpath[MAX_PATH*2];
	struct _stat ws_stat;

	if ((MultiByteToWideChar(65001, 0, fname, -1, wpath, MAX_PATH*2)) == 0) {
		return 1;
	}
	
	if (_wstat(wpath, &ws_stat) == 0) {
		return ws_stat.st_mtime;
	} else {
		return 0;
	}
}

char existsdir(char *fname) {
	wchar_t wpath[MAX_PATH*2];
	struct _stat ws_stat;
	
	if (fname == NULL) {
		return 0;
	}
	
	if ((MultiByteToWideChar(65001, 0, fname, -1, wpath, MAX_PATH*2)) == 0) {
		return 1;
	}
	
	if (_wstat(wpath, &ws_stat) == 0) {
		if (_S_IFDIR & ws_stat.st_mode)
			return 1;
		else
			return 0;
	} else {
errorf("wstat failed: %s", fname);
		return 0;
	}
}

#include <time.h>

char *timetostr(unsigned long long ulltime) {
	char *buf = malloc(20);
	struct tm *info = gmtime(&ulltime);
	strftime(buf, 20, "%Y-%m-%d %H:%M:%S", info);
	return buf;
}

DIRSTRUCT *diropen(char *dir) {
	wchar_t wdir[MAX_PATH+2];
	
	char buf[MAX_PATH*4+2];
	sprintf(buf, "%s\\*", dir);
	if ((MultiByteToWideChar(65001, 0, buf, -1, wdir, MAX_PATH)) == 0) {
		errorf("MultiByteToWideChar Failed");
	}
	
	DIRSTRUCT *ds = malloc(sizeof(DIRSTRUCT));
	ds->firstread = 1;
	
	if ((ds->search = FindFirstFileW(wdir, &ds->data)) == INVALID_HANDLE_VALUE) {
		errorf("FindFirstFileW - INVALID_HANDLE_VALUE");
		free(ds);
		return NULL;
	}
	
	return ds;
}

char dirread(DIRSTRUCT *dirp, char *buf) {
	if (!dirp->firstread) {
		if (!FindNextFileW(dirp->search, &dirp->data)) {
			return 0;
		}
	} else {
		dirp->firstread = 0;
	}
	if ((WideCharToMultiByte(65001, 0, dirp->data.cFileName, -1, buf, MAX_PATH*4, NULL, NULL)) == 0) {
		errorf("WideCharToMultiByte Failed");
	}
	
	return 1;
}

int dirclose(DIRSTRUCT *dirp) {
	FindClose(dirp->search);
	free(dirp);
	return 0;
}