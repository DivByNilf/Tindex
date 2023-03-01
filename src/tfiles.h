char *numtotfilestr(long long num);

long long tfilestrtonum(char const *str);

char releasetfile(char const *str, unsigned long long nsegments);

FILE *opentfile(char const *str, unsigned long long nsegment, char const *mode);

char *reservetfile(void);

char removetfile(char const *str, unsigned long long nsegment);

char mergetfiles(char const *str, long long nsegments, unsigned char nstrs, unsigned char strtypes, int (*compare)(unsigned char *, unsigned char *), unsigned char sel, unsigned char descending);

int tempfoldercheck(const char *prgDir);

char sorttfile(char const *str, unsigned char nstrs, unsigned char strtypes, int (*compare)(unsigned char *, unsigned char *), unsigned char sel, unsigned char descending);

int ullstrcmp(unsigned char const *str1, unsigned char const *str2);