#include <windows.h>
#include <string.h>

#include "stringchains.h"
#include "portables.h"
#include "ioextras.h"
#include "tfiles.h"
#include "dirfiles.h"
#include "dupstr.h"

#define LF_MAX_CLEN 1000	//! should test with lower ones

#include <stdio.h>
#define errorf(...) fprintf(stderr, __VA_ARGS__)

oneslnk *listfileschn(char *dir) {		// file name without dir path
	int i;
	oneslnk *first, *link;
	char buf[MAX_PATH*4];
	
	DIRSTRUCT *dirp = diropen(dir);
	
	first = link = malloc(sizeof(oneslnk));
	first->str = 0, first->next = 0;
	
	while (dirread(dirp, buf)) {
		if (buf[0] == '.')
			if (buf[1] == '\0' || buf[1] == '.' && buf[2] == '\0')
				continue;

		for (i = 0; buf[i] != '\0'; i++);
		link->next = malloc(sizeof(oneslnk));
		link = link->next;
		link->str = malloc(i+1);
		while (i >= 0) {
			link->str[i] = buf[i];
			i--;
		}
	}
	dirclose(dirp);
	
	link->next = 0;
	link = first, first = first->next, free(link);
	return first;
}

char listfilesfilesorted(char *dir, char *dst, char skipdirs, char recursive) {
	int i, c;
	char isdir;
	unsigned long long clen, ntfilesegs = 0;
	oneslnk *first, *link, *link2, *stack;
	char _buf[MAX_PATH*4], _buf2[MAX_PATH*4], *tfilestr, *ltfilestr = "-listf", *prepend, *s;
	char *buf = _buf, *buf2 = _buf2;
	FILE *dstfile, *tfile;
	
	DIRSTRUCT *dirp = diropen(dir);
	
	first = link = malloc(sizeof(oneslnk));
	first->str = 0, first->next = 0;
	clen = 0;
	
	tfilestr = malloc(strlen(ltfilestr)+1), strncpy(tfilestr, ltfilestr, strlen(ltfilestr)+1);
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
			
			if (!isdir || !skipdirs) {
				for (i = 0; buf[i] != '\0'; i++);
				link = link->next = malloc(sizeof(oneslnk));
				link->str = malloc(i+1);
				while (i >= 0) {
					link->str[i] = buf[i];
					i--;
				}
				clen++;
				if (clen >= LF_MAX_CLEN) {
					link->next = 0, link = first, first = first->next, free(link);
					sortoneschn(first, (int(*)(void*,void*)) strcmp, 0);
					tfile = opentfile(tfilestr, ntfilesegs++, "wb");
					for (link = first; link != 0; link = link->next) {
						term_fputs(link->str, tfile);
					}
					fclose(tfile);
					killoneschn(first, 0);
					first = link = malloc(sizeof(oneslnk));
					first->str = 0, first->next = 0;
					clen = 0;
				}
			}
			if (isdir && recursive) {
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
	
	if (clen > 0) {
		link->next = 0, link = first, first = first->next, free(link);
		sortoneschn(first, (int(*)(void*,void*)) strcmp, 0);
		tfile = opentfile(tfilestr, ntfilesegs++, "wb");
		for (link = first; link != 0; link = link->next) {
			term_fputs(link->str, tfile);
		}
		fclose(tfile);
	}
	killoneschn(first, 0);
	
	if (ntfilesegs > 1) {
		if (mergetfiles(tfilestr, ntfilesegs, 1, 0, (int(*)(unsigned char*,  unsigned char*)) strcmp, 1, 0)) {
			errorf("mergetfiles failed");
		}
		ntfilesegs = 1;
	}
	
	dstfile = MBfopen(dst, "wb");
	if (ntfilesegs > 0) {
		if ((tfile = opentfile(tfilestr, 0, "rb")) == NULL) {
			errorf("couldn't read temp file");
			fclose(tfile), releasetfile(tfilestr, ntfilesegs);
			return 1;
		}
	
		while ((c = getc(tfile)) != EOF) {
			putc(c, dstfile);
		}
		fclose(tfile), releasetfile(tfilestr, ntfilesegs);
	}
	fclose(dstfile);
	
	return 0;
}