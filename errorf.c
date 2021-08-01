#include <stdio.h>
#include <windows.h>
#include <stdarg.h>

#include "portables.h"

extern char *PrgDir;

void derrorf(char *, ...);

void errorf(char *str, ...) {
	char buf[MAX_PATH*4];
	wchar_t wbuf[MAX_PATH*2];
	FILE *file;
	va_list args;
	static char firsterr;
	
	if (!PrgDir) {
		MessageBoxW(0, L"PrgDir doesn't exist", L"Error", MB_OK); 
		return;
	}
	sprintf(buf, "%s\\errorfile.log", PrgDir);
	
	if ((file = MBfopen(buf, "a")) == NULL) {
		derrorf("Failed to create file, \"%s\"", buf);
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

void derrorf(char *str, ...) {
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