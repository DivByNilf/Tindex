#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "portables.h"

#include <stdio.h>
#define errorf(...) fprintf(stderr, __VA_ARGS__)

int breakpath(char *path, char *edname, char *efname, char *eexname) {
	char a[MAX_PATH*4], dname[MAX_PATH*4], fname[MAX_PATH*4], exname[MAX_PATH*4];
	int i, j, di, fi, exi, plen, mbl;
	i = j = di = fi = exi = 0;
	
	if (path[1] == ':') {
		if (strlen(path) > MAX_PATH*4-1) {
			errorf("Error: Path too long.\n");
			return 1;
		}
		strcpy_s(a, MAX_PATH*4, path);
//		errorf("Absolute path (no conversion): %s\n", a);
	}
	else {
		char temp[MAX_PATH*4];
		getcwd(temp, MAX_PATH*4);
		if ((strlen(temp) + strlen(path)) > MAX_PATH*4-2) {
			errorf("Error: Path too long.");
			return 1;
		}
		plen = strlen(path)-1;
		if (path[plen] == ' ') {
			while (path[--plen] == ' ');
			path[plen+1] = '\0';
		}
		if (path[0] == '.' && path[1] == '\\') {
			sprintf(a, "%s\\%s", temp, path+2);
//			errorf("Absolute path (converted): %s\n", a);
		}
		else {
			sprintf(a, "%s\\%s", temp, path);
//			errorf("Absolute path (converted): %s\n", a);
		}
	}
	for (i = 0; a[i] != '\0'; i++) {
		
		switch (a[i]) {
			case '\\':
				if (fi == 0) {
					errorf("Error: two '\\' in a row.\n");
					return 1;
				}
				if (fname[fi-1] == ' ') {
					errorf("Error: directory ending with space\n");
					return 1;
				}
				fname[fi] = '\0';
				dname[di++] = '\\';
				for (j = 0; fname[j] != '\0'; j++) 
					dname[di++] = fname[j];
				fi = 0;
				break;
			case '.':
				if (fi == 0) {
					if (a[i+1] == '\\') {
						i++;
					} else {
					fname[fi++] = '.';
					break;
					}
				}
				for (i++; !(a[i] == '\\' || a[i] == '\0'); i++) {
					switch (a[i]) {
						case '.':
							fname[fi++] = '.';
							if (exi == 0)
								break;
							exname[exi] = '\0';
							for (j = 0; exname[j] != '\0'; j++) 
								fname[fi++] = exname[j];
							exi = 0;
							break;
						case '<': case '>': case '"': case '/': case '|': case '?': case '*': case ':':
							errorf("Error: Illegal character '%c'.", a[i]);
							return 1;
						default:
							exname[exi++] = a[i];
							break;
					}				
				}
				if (a[i] == '\\') {
					fname[fi] = '\0';
					dname[di++] = '\\';
					for (j = 0; fname[j] != '\0'; j++) 
						dname[di++] = fname[j];
					fi = 0;
					dname[di++] = '.';
					if (exi == 0) {
						errorf("Error: directory ending with dot\n");
						return 1;
					}
					else {
						if (exname[exi-1] == ' ') {
							errorf("Error: directory ending with space\n");
							return 1;
						}
						exname[exi] = '\0';
						for(j = 0; exname[j] != '\0'; j++)
							dname[di++] = exname[j];
						exi = 0;
					}
				}
				else if (a[i] == '\0') {
					if (exi == 0) {
						errorf("Error: file ending with dot\n");
						return 1;
					}
					i--;
				}
				break;
			case ':':
				if (i == 1 && a[2] == '\\') {
					dname[0] = a[0], dname[1] = a[1];
					di = 2;
					fi = 0;
					i++;
				}
				else {
					errorf("Error: Illegal ':'.\n");
					return 1;
				}
				break;
			case '<': case '>': case '"': case '/': case '|': case '?': case '*':
				errorf("Error: Illegal character '%c'.", a[i]);
				return 1;
			default:
				fname[fi++] = a[i];
				break;
		}
	}
	if (fi < 1) {
		errorf("Error: No file name.\n");
		return 1;
	}
	dname[di] = '\0';
	fname[fi] = '\0';
	exname[exi] = '\0';
	if (edname != NULL) {
		strcpy_s(edname, MAX_PATH*4, dname); 
	} if (efname != NULL) {
		strcpy_s(efname, MAX_PATH*4, fname);
	} if (eexname != NULL) {
		strcpy_s(eexname, MAX_PATH*4, exname);
	}
	return 0;
}

int breakpathdf(char *path, char *edname, char *efname) {
	char a[MAX_PATH*4], dname[MAX_PATH*4], fname[MAX_PATH*4];
	int i, j, di, fi, plen, mbl;
	i = j = di = fi = 0;
	
	if (path[1] == ':') {
		if (strlen(path) > MAX_PATH*4-1) {
			errorf("Error: Path too long.\n");
			return 1;
		}
		strcpy_s(a, MAX_PATH*4, path);
//		errorf("Absolute path (no conversion): %s\n", a);
	} else {
		char temp[MAX_PATH*4];
		getcwd(temp, MAX_PATH*4);
		if ((strlen(temp) + strlen(path)) > MAX_PATH*4-2) {
			errorf("Error: Path too long.");
			return 1;
		}
		plen = strlen(path)-1;
		if (path[plen] == ' ') {
			while (path[--plen] == ' ');
			path[plen+1] = '\0';
		}
		if (path[0] == '.' && path[1] == '\\') {
			sprintf(a, "%s\\%s", temp, path+2);
//			errorf("Absolute path (converted): %s\n", a);
		}
		else {
			sprintf(a, "%s\\%s", temp, path);
//			errorf("Absolute path (converted): %s\n", a);
		}
	}
	for (i = 0; a[i] != '\0'; i++) {
		
		switch (a[i]) {
			case '\\':
				if (fi == 0) {
					errorf("Error: two '\\' in a row.\n");
					return 1;
				}
				if (fname[fi-1] == ' ') {
					errorf("Error: directory ending with space\n");
					return 1;
				}
				fname[fi] = '\0';
				dname[di++] = '\\';
				for (j = 0; fname[j] != '\0'; j++) 
					dname[di++] = fname[j];
				fi = 0;
				break;
			case '.':
				if (fi == 0) {
					if (a[i+1] == '\\') {
						i++;
					} else {
					fname[fi++] = '.';
					break;
					}
				}
				if (a[i+1] == '\\') {
					errorf("Error: directory ending with dot\n");
					return 1;
				}
				else if (a[i+1] == '\0') {
					errorf("Error: file ending with dot\n");
					return 1;
				} else {
					fname[fi++] = '.';
				}
				break;
			case ':':
				if (i == 1 && a[2] == '\\') {
					dname[0] = a[0], dname[1] = a[1];
					di = 2;
					fi = 0;
					i++;
				}
				else {
					errorf("Error: Illegal ':'.\n");
					return 1;
				}
				break;
			case '<': case '>': case '"': case '/': case '|': case '?': case '*':
				errorf("Error: Illegal character '%c'.", a[i]);
				return 1;
			default:
				fname[fi++] = a[i];
				break;
		}
	}
	if (fi < 1) {
		errorf("Error: No file name.\n");
		return 1;
	}
	dname[di] = '\0';
	fname[fi] = '\0';
	if (edname != NULL) {
		strcpy_s(edname, MAX_PATH*4, dname); 
	} if (efname != NULL) {
		strcpy_s(efname, MAX_PATH*4, fname);
	}
	return 0;
}

int breakfname(char *ifname, char *efname, char *eexname) {
	char a[MAX_PATH*4], fname[MAX_PATH*4], exname[MAX_PATH*4];
	int i, j, fi, exi, plen, mbl;
	i = j = fi = exi = 0;
	
	if (strlen(ifname) > MAX_PATH*4-1) {
		errorf("Error: file name too long.\n");
		return 1;
	}
	strcpy_s(a, MAX_PATH*4, ifname);
//	errorf("Absolute path (no conversion): %s\n", a);
	
	for (i = 0; a[i] != '\0'; i++) {
		switch (a[i]) {
			case '.':
				if (fi == 0) {
					fname[fi++] = '.';
					break;
				}
				for (i++; !(a[i] == '\0'); i++) {
					switch (a[i]) {
						case '.':
							fname[fi++] = '.';
							if (exi == 0)
								break;
							exname[exi] = '\0';
							for (j = 0; exname[j] != '\0'; j++) 
								fname[fi++] = exname[j];
							exi = 0;
							break;
						case '<': case '>': case '"': case '/': case '|': case '?': case '*': case ':': case '\\':
							errorf("Error: Illegal character '%c'.", a[i]);
							return 1;
						default:
							exname[exi++] = a[i];
							break;
					}				
				}
				if (a[i] == '\0') {
					if (exi == 0) {
						errorf("Error: file ending with dot\n");
						return 1;
					}
					i--;
				}
				break;
			case '<': case '>': case '"': case '/': case '|': case '?': case '*': case ':': case '\\':
				errorf("Error: Illegal character '%c'.", a[i]);
				return 1;
			default:
				fname[fi++] = a[i];
				break;
		}
	}
	if (fi < 1) {
		errorf("Error: No file name.\n");
		return 1;
	}
	fname[fi] = '\0';
	exname[exi] = '\0';
	if (efname != NULL) {
		strcpy_s(efname, MAX_PATH*4, fname);
	} if (eexname != NULL) {
		strcpy_s(eexname, MAX_PATH*4, exname);
	}
	return 0;
}
// apparently file names and folder names can start with dots (folders only if made with mkdir), there can be dots in the middle of a file or folder name including multiple between an extension and file name, but file names and folder names cannot end with a dot.
// file names and folder names can start with spaces (if made by other than explorer) but not end with them. spaces are allowed between file name and extension on either side of the dot.}