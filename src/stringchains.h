

#ifndef _STRINGCHAINS_H
#define _STRINGCHAINS_H
typedef struct tagoneslink {
	struct tagoneslink *next;
	union {
		unsigned char *str;
		unsigned long long ull;
		void *vp;
	};
} oneslnk;

typedef struct tagtwoslink {
	struct tagtwoslink *next;	
	union {
		unsigned char* str;
		unsigned long long ull;
		void *vp;
	} u[2];
} twoslnk;

#endif

char killoneschn(oneslnk *slink, unsigned char flag);

char killoneschnchn(oneslnk *slink, unsigned char flag);

oneslnk *copyoneschn(oneslnk *slink, unsigned char flag);

char sortoneschn(oneslnk *slink, int (*compare)(void *, void *), unsigned char descending);

char sortoneschnull(oneslnk *slink, unsigned char descending);

char killtwoschn(twoslnk *slink, unsigned char flag);

twoslnk *copytwoschn(twoslnk *slink, unsigned char flag);

char sorttwoschn(twoslnk **slink, int (*compare)(void *, void *), unsigned char sel, unsigned char descending);

char sorttwoschnull(twoslnk **slink, unsigned char sel, unsigned char descending);