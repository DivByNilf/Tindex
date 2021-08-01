#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "dupstr.h"
#include "stringchains.h"
#include "portables.h"

char *PrgDir = "F:\\My\\Files\\Programming\\FTI\\prog";

void errorf(char *str, ...) {
	va_list args;
	
	va_start(args, str);
	vprintf(str, args);
	va_end(args);
	printf("\n");
}

//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
int main(int argc, char **argv) {
/*	int argc;
	wchar_t *cls, **argv;
	char *arg0;

	arg0 = malloc(MAX_PATH*4);
	cls = GetCommandLine();
	argv = CommandLineToArgvW(cls, &argc);
	if ((WideCharToMultiByte(65001, 0, argv[0], -1, arg0, MAX_PATH*4, NULL, NULL)) == 0) {
		errorf("WideCharToMultiByte Failed");
	}
	if (LocalFree(argv) != 0) {
		errorf("LocalFree Failed");
	}

	char *dir = mb_from_wide(argv[1]);
	char *dst =  mb_from_wide(argv[2]);
*/
	char *dir = argv[1];
	char *dst = argv[2];
	
	errorf("dir: %s, dst: %s", dir, dst);
	
	if (dir == NULL || dst == NULL) {
		return 1;
	}
	DIRSTRUCT *dirp = diropen(dir);
	
	if (!dirp) {
		return 1;
	}
	
	FILE *fp;
	
	if (!(fp = MBfopen(dst, "wb"))) {
		dirclose(dirp);
		return 1;
	}
	
//	fprintf(fp, "test");
	
	int i, c;
	char isdir;
	oneslnk *link2, *stack;
	char _buf[MAX_PATH*4], _buf2[MAX_PATH*4], *prepend, *s;
	char *buf = _buf, *buf2 = _buf2;
	
	stack = malloc(sizeof(oneslnk));
	stack->next = NULL;
	stack->vp = malloc(sizeof(SUBDIRSTRUCT));
	((SUBDIRSTRUCT*)stack->vp)->ds = dirp;
	prepend = ((SUBDIRSTRUCT*)stack->vp)->prepend = NULL;
	
	while (stack) {
		while (dirread(dirp, buf)) {
			if (buf[0] == '.') {
				if (buf[1] == '\0' || buf[1] == '.' && buf[2] == '\0') {
					continue;
				}
			} else if (strcmp(buf, "desktop.ini") == 0) {
				continue;
			} else {
				if (prepend) {
					strcpy(buf2, prepend);
					strcat(buf2, buf);
					s = buf, buf = buf2, buf2 = s;
				}
				strcpy(buf2, dir);
				strcat(buf2, "\\");
				strcat(buf2, buf);
				
				if (isdir = existsdir(buf2)) {
					strcat(buf, "\\");
				}
			}
			if (1) {
				fprintf(fp, "%s\n", buf);
			}
			if (isdir) {
				link2 = stack;
				stack = malloc(sizeof(oneslnk));
				stack->next = link2;
				stack->vp = malloc(sizeof(SUBDIRSTRUCT));
				prepend = ((SUBDIRSTRUCT*)stack->vp)->prepend = dupstr(buf, MAX_PATH*4, 0);
				dirp = ((SUBDIRSTRUCT*)stack->vp)->ds = diropen(buf2);
			}
		}
		dirclose(dirp);
		if (prepend) free(prepend);
		link2 = stack;
		stack = stack->next;
		free(link2);
		if (stack) {
			prepend = ((SUBDIRSTRUCT*)stack->vp)->prepend;
			dirp = ((SUBDIRSTRUCT*)stack->vp)->ds;
		}
	}
	
	fclose(fp);
	return 0;
}