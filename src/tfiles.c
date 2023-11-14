#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "dirfiles.h"
#include "ioextras.h"
#include "portables.h"

#include <stdio.h>
#define errorf(...) fprintf(stderr, __VA_ARGS__)

static char tempfolderfound;

char *numtotfilestr(long long num) {
	char *result;
	int c, len;
	
	if (num < 0) {
		errorf("trying to convert number less than 0 to tfilestr, num: %lld", num);
		return 0;
	}
	
	unsigned int radix = '9'-'0'+1+'Z'-'A'+1;
	unsigned int frange = '9'-'0'+1;
	
	if (num == 0) {
		len = 1;
	} else {
		len = log(num)/log(radix)+1;
	}
	if (len < 1) {
		errorf("len < 1, num = %d", num);
		return 0;
	}
	result = malloc(len+1);
	result[len] = '\0';
	while (--len >= 0) {
		if (num == 0 && result[len+1] != '\0') {
			errorf("miscalculated number digits");
		}
		if ((c = num % radix) < frange) {
			result[len] = '0' + c;
		} else {
			result[len] = 'A' - frange + c;
		} num /= radix;
	}
	return result;
}

long long tfilestrtonum(char *str) {
	int c, i = 0;
	unsigned long long result = 0;
	unsigned int radix = '9'-'0'+1+'Z'-'A'+1;
	
	while (i < 8) {
		if (str[i] <= '9') {
			if (str[i] < '0') {
				if (str[i] == '\0') {
					if (i == 0) {
						errorf("passed string with only null to tfilestrtonum");
						return -1;
					}
					return result;
				}
				errorf("invalid character in tfilestr: %d, '%c' -- str: %s", (int) str[i], str[i], str);
				return -1;
			}
			result *= radix;
			result += str[i] - '0';
		} else if (str[i] >= 'A' && str[i] <= 'Z') {
			result *= radix;
			result += str[i] - 'A' + ('9'-'0')+1;
		} else {
			errorf("invalid character in tfilestr: %d, '%c' -- str: %s", (int) str[i], str[i], str);
			return -1;
		}
		i++;
	}
	errorf("tfilestr too long");
	return -1;
}

int tempfoldercheck(const char *prgDir) {
	char buf[MAX_PATH*4];
	int c;
	
	if (!tempfolderfound) {
		sprintf(buf, "%s\\temp", prgDir);
		if (c = checkfiletype(buf)) {
			if (c == 1) {
				tempfolderfound = 1;
			} else {
				return 0;
			}
		} else {
			if (make_directory(buf)) {
				return 0;
			}
			tempfolderfound = 1;
		}
	}
	return 0; //!
}

char *reservetfile(const char *prgDir) {
	char buf[MAX_PATH*4], *str, *str2;
	int i, j, c;
	long long largest = -1, nsegment;
	FILE *temp;
	
	tempfoldercheck(prgDir);
	sprintf(buf, "%s\\temp", prgDir);
	
	DIRSTRUCT *dirp = diropen(buf);
	
	while (dirread(dirp, buf)) {
		if (buf[0] == '.')
			if (buf[1] == '\0' || buf[1] == '.' && buf[2] == '\0')
				continue;
			
		for (i = 0; buf[i] != '-' && buf[i] != '\0'; i++);
		
		buf[i] = '\0';
		
		if ((nsegment = tfilestrtonum(buf)) == -1) {
			continue;
		} else if (nsegment > largest)
			largest = nsegment;
		
	}
	dirclose(dirp);
	
	str = numtotfilestr(largest+1);
	str2 = numtotfilestr(0);
	sprintf(buf, "%s\\temp\\%s-%s", prgDir, str, str2);
	free(str2);
	fclose(MBfopen(buf, "w"));
	
	return str;
}

/*
char *reservetfileold(void) {
	char buf[MAX_PATH*4], baf[MAX_PATH*4], *str, *str2;
	int i, j, c;
	long long lastnsegment = -1, nsegment;
	FILE *temp;
	
	sprintf(buf, "%s\\temp", g_prgDir);
	tempfoldercheck();
	sprintf(baf, "%s\\temp\\-rtf", g_prgDir);
	if (listfilesfilesorted(buf, baf)) {
		errorf("listfilesfilesorted failed");
		return 0;
	}
	temp = MBfopen(baf, "rb");
	while (1) {
		for (i = 0, j = -1; (buf[i] = c = getc(temp)) != '\0' && i < MAX_PATH*4; i++) {
			if (c == '-' && j == -1) {
				j = i;
			} else if (c == EOF) {
				if (i == 0)
					break;
				errorf("EOF before end of dir in filelist");
				fclose(temp), MBremove(baf);
				return 0;
			}
		}
		if (c == EOF) {
			break;		// necessary to break out of outer loop
		}
		if (i >= MAX_PATH*4) {
			errorf("reservetfile read error");
			fclose(temp), MBremove(baf);
			return 0;
		}
		if (j < 0) {
			j = 0;
		}
		buf[j] = '\0';
		if ((nsegment = tfilestrtonum(buf)) == -1) {
			continue;
		}
		if (nsegment - lastnsegment >= 2) {
			break;
		}
		lastnsegment = nsegment;
	}
	fclose(temp), MBremove(baf);
	
	str = numtotfilestr(lastnsegment+1);
	str2 = numtotfilestr(0);
	sprintf(buf, "%s\\temp\\%s-%s", g_prgDir, str, str2);
	free(str2);
	fclose(MBfopen(buf, "w"));
	
	return str;
}

*/

FILE *opentfile(char const *str, unsigned long long nsegment, char const *mode, const char *prgDir) {
	char buf[MAX_PATH*4], *segstr;
	int c;

	tempfoldercheck(prgDir);
	
	if (!(segstr = numtotfilestr(nsegment))) {
		return 0;
	}
	sprintf(buf, "%s\\temp\\%s-%s", prgDir, str, segstr);
	return MBfopen(buf, mode);
}

char removetfile(char const *str, unsigned long long nsegment, const char *prgDir) {
	char buf[MAX_PATH*4], *segstr;
	
	if (str == 0) {
		return 1;
	}
	segstr = numtotfilestr(nsegment);
	sprintf(buf, "%s\\temp\\%s-%s", prgDir, str, segstr);
	free(segstr);
	MBremove(buf);
	
	return 0;
}

char releasetfile(char *str, unsigned long long nsegments, const char *prgDir) {
	char buf[MAX_PATH*4], *segstr;
	
	if (str == 0) {
		return 1;
	}
	
	if (nsegments == 0)
		nsegments = 1;
	while (nsegments-- > 0) {
		segstr = numtotfilestr(nsegments);
		sprintf(buf, "%s\\temp\\%s-%s", prgDir, str, segstr);
		free(segstr);
		MBremove(buf);
	}
	free(str);
	return 0;
}

char mergetfiles(char const *str, long long nsegments, unsigned char nstrs, unsigned char strtypes, int (*compare)(unsigned char *, unsigned char *), unsigned char sel, unsigned char descending, const char *prgDir) {	// strtypes 0-bit for null-terminated and 1-bit for prefixed for byte lower to higher
//! not tested properly with byte prefixed or multiple strings
	unsigned char *strA[8], *strB[8], doneA, doneB, getA, getB, over;
	int i, j, c;
	long long remsegments, k, oldremseg;
	FILE *fileA, *fileB, *fileC;
	
	if (nsegments < 2) {
		errorf("merging less than two");
		return 1;
	}
	if (sel > nstrs) {
		errorf("sel over nstrs");
		return 2;
	} if (sel == 0) {
		errorf("sel is 0");
		return 2;
	}
	
	for (i = 0; i < nstrs; i++) {
		if (strtypes & (unsigned char)exp2(i)) {
			strA[i] = malloc(9);
			strB[i] = malloc(9);
		} else {
			strA[i] = malloc(MAX_PATH*4);
			strB[i] = malloc(MAX_PATH*4);
		}
	}
	
	for (remsegments = nsegments, over = 0; remsegments > 1; remsegments = remsegments/2 + remsegments%2, over = !over) {
		if (!over) {
			oldremseg = remsegments;
		}
		for (k = 1; k < remsegments; k += 2) {
			if (!over) {
				fileA = opentfile(str, k-1, "rb", prgDir);
				fileB = opentfile(str, k, "rb", prgDir);
				fileC = opentfile(str, remsegments+(k/2), "wb", prgDir);
			} else {
				fileA = opentfile(str, (oldremseg)+k-1, "rb", prgDir);
				fileB = opentfile(str, (oldremseg)+k, "rb", prgDir);
				fileC = opentfile(str, k/2, "wb", prgDir);
			}
			
			doneA = doneB = 0;
			getA = getB = 1;
			while (!doneA || !doneB) {
				if (getA) {
					for (i = 0; i < nstrs; i++) {
						strA[i][0] = c = getc(fileA);
						if (c == EOF) {
							if (i != 0) {
								errorf("EOF before end of strings in mergetfiles");
								for (i = 0; i < nstrs; i++) {
									free(strA[i]), free(strB[i]);
								}
								fclose(fileA), fclose(fileB), fclose(fileC);
							} else {
								doneA = 1;
								break;
							}
						}
						if (strtypes & (unsigned char)exp2(i)) {
							for (j = 1; j <= strA[i][0]; strA[i][j] = getc(fileA), j++);
						} else {
							for (j = 1; c != '\0'; strA[i][j] = c = getc(fileA), j++);
						}
					}
					getA = 0;
				}
				if (getB) {
					for (i = 0; i < nstrs; i++) {
						strB[i][0] = c = getc(fileB);
						if (c == EOF) {
							if (i != 0) {
								errorf("EOF before end of strings in mergetfiles");
								for (i = 0; i < nstrs; i++) {
									free(strB[i]), free(strB[i]);
								}
								fclose(fileA), fclose(fileB), fclose(fileC);
							} else {
								doneB = 1;
								break;
							}
						}
						if (strtypes & (unsigned char)exp2(i)) {
							for (j = 1; j <= strB[i][0]; strB[i][j] = getc(fileB), j++);
						} else {
							for (j = 1; c != '\0'; strB[i][j] = c = getc(fileB), j++);
						}
					}
					getB = 0;
				}
				
				if (!doneA) {
					if (!doneB) {
						c = compare(strA[sel-1], strB[sel-1]);
						if (((c < 0) ^ descending) || (c == 0)) {
							getA = 1;
						} else {
							getB = 1;
						}
					}
					else {
						getA = 1;
					}
				} else {
					if (!doneB) {
						getB = 1;
					}
				}
				
				if (getA) {
					for (i = 0; i < nstrs; i++) {
						if (strtypes & (unsigned char)exp2(i)) {
							for (j = 0; j <= strA[i][0]; putc(strA[i][j++], fileC));
						} else {
							term_fputs(strA[i], fileC);
						}
					}
				} else if (getB) {
					for (i = 0; i < nstrs; i++) {
						if (strtypes & (unsigned char)exp2(i)) {
							for (j = 0; j <= strB[i][0]; putc(strB[i][j++], fileC));
						} else {
							term_fputs(strB[i], fileC);
						}
					}
				}			
			}
			fclose(fileA), fclose(fileB), fclose(fileC);
			if (!over) {
				removetfile(str, k-1, prgDir);
				removetfile(str, k, prgDir);
			} else {
				removetfile(str, (oldremseg)+k-1, prgDir);
				removetfile(str, (oldremseg)+k, prgDir);
			}
		}
		if (k == remsegments) {
			if (!over) {
				fileA = opentfile(str, k-1, "rb", prgDir);
				fileC = opentfile(str, remsegments+(k/2), "wb", prgDir);
			} else {
				fileA = opentfile(str, (oldremseg)+k-1, "rb", prgDir);
				fileC = opentfile(str, k/2, "wb", prgDir);
			}
			for (; (c = getc(fileA)) != EOF; putc(c, fileC));
			fclose(fileA), fclose(fileC);
			if (!over) {
				removetfile(str, k-1, prgDir);
			} else {
				removetfile(str, (oldremseg)+k-1, prgDir);
			}
		}
	}
	if (over) {
		fileA = opentfile(str, 2, "rb", prgDir);
		fileC = opentfile(str, 0, "wb", prgDir);
		
		for (; (c = getc(fileA)) != EOF; putc(c, fileC));
		fclose(fileA), fclose(fileC);
		removetfile(str, 2, prgDir);
	}	
	
	for (i = 0; i < nstrs; i++) {
		free(strA[i]), free(strB[i]);
	}
	
	return 0;
}

int ullstrcmp(unsigned char const *str1, unsigned char const *str2) {
	int i, j;
	i = str1[0];
	j = str2[0];
	
	if (i > j)
		return 1;
	else if (i < j)
		return -1;
	else {
		for (i = 1; i <= j; i++) {
			if (str1[i] > str2[i])
				return 1;
			else if (str1[i] < str2[i])
				return -1;
		}
		return 0;
	}
}

char sorttfile(char const *str, unsigned char nstrs, unsigned char strtypes, int (*compare)(unsigned char *, unsigned char *), unsigned char sel, unsigned char descending, const char *prgDir)  {	// strtypes 0-bit for null-terminated and 1-bit for prefixed for byte lower to higher
//! not tested properly with byte prefixed or multiple strings
	unsigned char *strA[8], *strB[8], doneA, doneB, getA, getB, *prevA, *prevB, over, onfile, seltype, *selstrA, *selstrB, startedA, eofA, eofB, *p;
	int i, j, c;
	long long remsegments, k, oldremseg;
	FILE *fileA, *fileB, *fileC;
	
	if (sel > nstrs) {
		errorf("sel over nstrs");
		return 2;
	} if (sel == 0) {
		errorf("sel is 0");
		return 2;
	}
	sel--;
	
	seltype = !!(strtypes & (1 << sel));
	
	for (i = 0; i < nstrs; i++) {
		if (strtypes & (unsigned char)exp2(i)) {
			strA[i] = malloc(9);
			strB[i] = malloc(9);
		} else {
			strA[i] = malloc(MAX_PATH*4);
			strB[i] = malloc(MAX_PATH*4);
		}
	}
	
	if (seltype == 0) {
		prevA = malloc(MAX_PATH*4);
		prevB = malloc(MAX_PATH*4);
	} else {
		prevA = malloc(9);
		prevB = malloc(9);
	}
	selstrA = strA[sel];
	selstrB = strB[sel];
	
	for (onfile = 0, over = 0; !over; onfile = !onfile) {
//errorf("test1");
		startedA = 0;
		
		if (!onfile) {
			fileA = opentfile(str, 0, "rb", prgDir);
			fileB = opentfile(str, 0, "rb", prgDir);
			fileC = opentfile(str, 1, "wb", prgDir);
		} else {
			fileA = opentfile(str, 1, "rb", prgDir);
			fileB = opentfile(str, 1, "rb", prgDir);
			fileC = opentfile(str, 0, "wb", prgDir);
		}
		
		if (!fileA || !fileB || !fileC) {
			errorf("failed to open file");
			for (i = 0; i < nstrs; i++) {
				free(strB[i]), free(strB[i]);
			}
			fclose(fileA), fclose(fileB), fclose(fileC), free(prevA), free(prevB);
			return 1;
		}
		
		eofA = eofB = 0;
		
		prevA[0] = 0;
		prevB[0] = 0;
		getA = 1;
		getB = 0;
//unsigned long long btou(unsigned char *num);
		
		while (!eofB) {
			doneA = doneB = 0;
			
			while (1) { // set the B file to second increasing list 
				for (i = 0; i < nstrs; i++) {
					strB[i][0] = c = getc(fileB);
					if (c == EOF) {
						if (i != 0) {
							errorf("EOF before end of strings in sorttfile");
							for (i = 0; i < nstrs; i++) {
								free(strB[i]), free(strB[i]);
							}
							fclose(fileA), fclose(fileB), fclose(fileC), free(prevA), free(prevB);
							return 1;
						} else {
//errorf("eofB 0");
							doneB = 1;
							eofB = 1;
							break;
						}
					}
					if (strtypes & (unsigned char)exp2(i)) {
						for (j = 1; j <= strB[i][0]; strB[i][j] = getc(fileB), j++);
					} else {
						for (j = 1; c != '\0'; strB[i][j] = c = getc(fileB), j++);
//errorf("got string: %s", strB[i]);
					}
				}
				c = compare(prevB, selstrB);
				if (seltype == 0) {
					for (i = 0; (prevB[i] = selstrB[i]) != '\0'; prevB[i] = selstrB[i], i++);
					prevB[i] = '\0';
				} else {
//errorf("comparing: %llu vs %llu", btou(prevB), btou(selstrB));
					for (i = 0; i <= 8; prevB[i] = selstrB[i], i++);
				}
				
				if (c > 0 || eofB)
					break;
			}
			
			if (doneB && !startedA) { // the terminator
				over = 1;
				break;
			}
			
//errorf("pre startedA");
			if (startedA == 0)
				startedA = 1;
			else
				startedA = 2;
			
			while (!doneA || !doneB) {
//errorf("test5");
//errorf("%s", strA[i]);
				if (getA) {
					for (i = 0; i < nstrs; i++) {
						strA[i][0] = c = getc(fileA);
						if (c == EOF) {
							if (i != 0) {
								errorf("EOF before end of strings in sorttfile");
								for (i = 0; i < nstrs; i++) {
									free(strA[i]), free(strB[i]);
								}
								fclose(fileA), fclose(fileB), fclose(fileC), free(prevA), free(prevB);
								return 1;
							} else {
								if (!doneB) {
									errorf("fileA EOF without fileB");
									for (i = 0; i < nstrs; i++) {
										free(strA[i]), free(strB[i]);
									}
									fclose(fileA), fclose(fileB), fclose(fileC), free(prevA), free(prevB);
									return 1;
									
								}
//errorf("eofA");
								eofA = 1;
								break;
							}
						} else {
							if (strtypes & (unsigned char)exp2(i)) {
								for (j = 1; j <= strA[i][0]; strA[i][j] = getc(fileA), j++);
							} else {
								for (j = 1; c != '\0'; strA[i][j] = c = getc(fileA), j++);
//errorf("A: got string: %s", strA[i]);
							}
						}
					}
					
					if (eofA || (compare(prevA, selstrA) > 0)) {
						doneA = 1;
//errorf("doneA");
					} else {
						if (seltype == 0) {
							for (i = 0; (prevA[i] = selstrA[i]) != '\0'; prevA[i] = selstrA[i], i++);
							prevA[i] = '\0';
						} else {
							for (i = 0; i <= 8; prevA[i] = selstrA[i], i++);
						}
					}
				}
				if (getB) {
//errorf("getB 1");
					for (i = 0; i < nstrs; i++) {
						strB[i][0] = c = getc(fileB);
						if (c == EOF) {
							if (i != 0) {
								errorf("EOF before end of strings in sorttfile");
								for (i = 0; i < nstrs; i++) {
									free(strB[i]), free(strB[i]);
								}
								fclose(fileA), fclose(fileB), fclose(fileC), free(prevA), free(prevB);
								return 1;
							} else {
//errorf("eofB");
								eofB = 1;
								break;
							}
						} else {
							if (strtypes & (unsigned char)exp2(i)) {
								for (j = 1; j <= strB[i][0]; strB[i][j] = getc(fileB), j++);
							} else {
								for (j = 1; c != '\0'; strB[i][j] = c = getc(fileB), j++);
							}
						}
					}
					
					if (eofB || compare(prevB, selstrB) > 0) {
						doneB = 1;
					}
					
					if (!eofB) {
						if (seltype == 0) {
							for (i = 0; (prevB[i] = selstrB[i]) != '\0'; prevB[i] = selstrB[i], i++);
							prevB[i] = '\0';
						} else {
							for (i = 0; i <= 8; prevB[i] = selstrB[i], i++);
						}
					}
				}
				getA = getB = 0;
				
				if (!doneA) {
					if (!doneB) {
						c = compare(strA[sel], strB[sel]);
//errorf("%s vs %s", strA[0], strB[0]);
//errorf("%llu vs %llu", btou(strA[sel]), btou(strB[sel]));

						if (((c < 0) ^ descending) || (c == 0)) {
							getA = 1;
						} else {
							getB = 1;
						}
					}
					else {
						getA = 1;
					}
				} else {
					if (!doneB) {
						getB = 1;
					}
				}
				
				if (getA) {
//errorf("putA 1");
					for (i = 0; i < nstrs; i++) {
						if (strtypes & (unsigned char)exp2(i)) {
							for (j = 0; j <= strA[i][0]; putc(strA[i][j++], fileC));
						} else {
							term_fputs(strA[i], fileC);
						}
					}
				} else if (getB) {
//errorf("putB 1");
					for (i = 0; i < nstrs; i++) {
						if (strtypes & (unsigned char)exp2(i)) {
							for (j = 0; j <= strB[i][0]; putc(strB[i][j++], fileC));
						} else {
							term_fputs(strB[i], fileC);
						}
					}
				}	
			}
			if (!eofB) { // set up for next ascending runs
				for (i = 0; i < nstrs; i++) { //swap
					p = strA[i];
					strA[i] = strB[i];
					strB[i] = p;
				}
				p = selstrA;
				selstrA = selstrB;
				selstrB = p;
				if (seltype == 0) {
					for (i = 0; (prevA[i] = selstrA[i]) != '\0'; prevA[i] = selstrA[i], i++);
					prevA[i] = '\0';
				} else {
					for (i = 0; i <= 8; prevA[i] = selstrA[i], i++);
				}
				fseek64(fileA, ftell64(fileB), SEEK_SET);
				
			}
		}
		fclose(fileA), fclose(fileB), fclose(fileC);
		
		if (startedA == 1) {
//errorf("startedA was 1");
			over = 1;
		}
	}
	for (i = 0; i < nstrs; i++) {
		free(strA[i]), free(strB[i]);
	}
	free(prevA), free(prevB);
	
	if (!!onfile && !!startedA) {
		fileA = opentfile(str, 1, "rb", prgDir);
		fileC = opentfile(str, 0, "wb", prgDir);
		
		for (; (c = getc(fileA)) != EOF; putc(c, fileC));
		fclose(fileA), fclose(fileC);
	}
	removetfile(str, 1, prgDir);
	
	return 0;
}