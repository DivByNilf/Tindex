#include <stdio.h>
#include <windows.h>
#include <stdarg.h>

#include "portables.h"

extern char *g_prgDir;

#define MAX_LEN MAX_PATH*4 + 1000
#define W_MAX_LEN MAX_PATH*2 + 1000

void g_errorfDialog(const char *, ...);

void g_errorf(const char *str, ...) {
	char buf[MAX_LEN];
	wchar_t wbuf[W_MAX_LEN];
	FILE *file;
	va_list args;
	static char firsterr;
	
	if (!g_prgDir) {
		MessageBoxW(0, L"g_prgDir doesn't exist", L"Error", MB_OK); 
		return;
	}
	sprintf(buf, "%s\\errorfile.log", g_prgDir);
	
	if ((file = MBfopen(buf, "a")) == NULL) {
		g_errorfDialog("Failed to create file, \"%s\"", buf);
		return;
	}
	if (!firsterr) {
		fprintf(file, "---");
		firsterr = 1;
		fputc('\n', file);
	}
	
	va_start(args, str);
	vfprintf(file, str, args);
	va_end(args);
	fputc('\n', file);
	
	fclose(file);
}

void g_errorfDialog(const char *str, ...) {
	char buf[1001];
	wchar_t *wbuf;
	va_list args;
	
	va_start(args, str);
	wbuf = malloc((2*(vsprintf(buf, str, args)+1)));
	va_end(args);
	
	if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf, 1001)) == 0) {
//		errorf("MultiByteToWideChar Failed");
	}
	MessageBoxW(0, wbuf, 0, 0);
}