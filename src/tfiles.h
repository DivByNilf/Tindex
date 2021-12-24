char *numtotfilestr(long long num);

long long tfilestrtonum(char *str);

char releasetfile(char *str, unsigned long long nsegments);

FILE *opentfile(char *str, unsigned long long nsegment, char *mode);

char *reservetfile(void);

char removetfile(char *str, unsigned long long nsegment);

char mergetfiles(char *str, long long nsegments, unsigned char nstrs, unsigned char strtypes, int (*compare)(unsigned char *, unsigned char *), unsigned char sel, unsigned char descending);

int tempfoldercheck(void);

char sorttfile(char *str, unsigned char nstrs, unsigned char strtypes, int (*compare)(unsigned char *, unsigned char *), unsigned char sel, unsigned char descending);

int ullstrcmp(unsigned char *str1, unsigned char *str2);