#include "stringchains.h"

#include <stdlib.h>
#include "dupstr.h"

#define COPYCHN_STRLIM 10000

#include <stdio.h>
#define errorf(...) fprintf(stderr, __VA_ARGS__)

char killoneschn(oneslnk *slink, unsigned char flag) { // 0 for string, 1 for ull
	oneslnk *buf;
	if (!slink) {
		errorf("no slink (kosc)");
		return 2;
	}
	if (flag > 1) {
		errorf("flag > 1");
		return 2;
	}
	while (slink != 0) {
		if (!flag && slink->str != 0) {
			free(slink->str);
		}
		buf = slink;
		slink = slink->next;
		free(buf);		
	}
	return 0;
}

char killoneschnchn(oneslnk *slink, unsigned char flag) { // 0 for string, 1 for ull
	oneslnk *buf;
	if (!slink) {
		errorf("no slink (koscc)");
		return 2;
	}
	while (slink != 0) {
		if (!flag && slink->vp != 0) {
			killoneschn(slink->vp, flag);
		}
		buf = slink;
		slink = slink->next;
		free(buf);		
	}
	return 0;
}

oneslnk *copyoneschn(oneslnk *slink, unsigned char flag) {	// 0 for null-terminated string, 1 for ull
	oneslnk *buf, *flink;
	int i;
	
	if (!slink || flag > 1) {
		errorf("no slink or flag over 1");
		return 0;
	}
	if (!(buf = flink = malloc(sizeof(oneslnk)))) {
		errorf("malloc failed");
		return 0;
	}
	flink->str = 0;
	while (slink != 0) {
		if (!(buf->next = malloc(sizeof(oneslnk)))) {
			errorf("malloc failed");
			killoneschn(flink, flag);
			return 0;
		}
		buf = buf->next;
		buf->next = 0;
		if (flag == 0) {
			if (slink->str != 0) {
				if (!(buf->str = dupstr(slink->str, COPYCHN_STRLIM, 0))) {
					errorf("copied string is NULL");
				}
			} else {
				buf->str = 0;
			}
		} else {
			buf->ull = slink->ull;
		}
		slink = slink->next;
	}
	buf = flink;
	flink = flink->next;
	free(buf);

	return flink;
}

char sortoneschn(oneslnk *slink, int (*compare)(void *, void *), unsigned char descending) {	// the contents of the nodes are moved
	long long i, j, nlinks, left, right, limright;
	oneslnk *link, **larray;
	void **spcarray;
	int c;
	
	if (!slink) {
		errorf("no slink-2");
		return 1;
	}
	descending = !!descending;
	
	for (nlinks = 1, link = slink; link->next != 0; nlinks++, link = link->next);
	
	if (!(larray = malloc(nlinks*sizeof(oneslnk*)))) {
		errorf("first malloc failed in sortoneschn");
		return 2;
	}
	if (!(spcarray = malloc(nlinks*sizeof(void *)))) {
		errorf("second malloc failed in sortoneschn");
		free(larray);
		return 2;
	}
	
	
	for (i = 0, link = slink; link != 0 && i < nlinks; larray[i] = link, link = link->next, i++);
	
	for (i = 1; i < nlinks; i *= 2) {
		for (j = 0; j < nlinks; spcarray[j] = larray[j]->vp, j++);
		
		for (j = 0; j+i < nlinks; j += 2*i) {
			if (j+2*i > nlinks) {
				limright = nlinks-j-i;
			} else {
				limright = i;
			}
			for (left = 0, right = 0;;) {
				if ((left < i) && ((right == limright) || (((c = compare(spcarray[j+left], spcarray[j+i+right])) < 0) ^ descending) || (c == 0))) { // stable sort
					larray[j+left+right]->vp = spcarray[j+left];
					left++;
				} else if (left == i && right == 0) {	// could even make it so that the left elements aren't added until at least one right element has been
					break;
				} else if (right < limright) {
					larray[j+left+right]->vp = spcarray[j+i+right];
					right++;
				} else {
					break;
				}
			}
		}
	}
	free(larray);
	free(spcarray);
	
	return 0;
}

char sortoneschnull(oneslnk *slink, unsigned char descending) {	// the contents of the nodes are moved
	long long i, j, nlinks, left, right, limright;
	unsigned long long *spcarray;
	oneslnk *link, **larray;
	int c;
	
	if (!slink) {
		errorf("no slink-3");
		return 1;
	}
	descending = !!descending;
	
	for (nlinks = 1, link = slink; link->next != 0; nlinks++, link = link->next);
	
	if (!(larray = malloc(nlinks*sizeof(oneslnk*)))) {
		errorf("first malloc failed in sortoneschn");
		return 2;
	}
	if (!(spcarray = malloc(nlinks*sizeof(unsigned long long)))) {
		errorf("second malloc failed in sortoneschn");
		free(larray);
		return 2;
	}
	
	
	for (i = 0, link = slink; link != 0 && i < nlinks; larray[i] = link, link = link->next, i++);
	
	for (i = 1; i < nlinks; i *= 2) {
		for (j = 0; j < nlinks; spcarray[j] = larray[j]->ull, j++);
		
		for (j = 0; j+i < nlinks; j += 2*i) {
			if (j+2*i > nlinks) {
				limright = nlinks-j-i;
			} else {
				limright = i;
			}
			for (left = 0, right = 0;;) {
				if ((left < i) && ((right == limright) || ((spcarray[j+left] < spcarray[j+i+right]) ^ descending) || (spcarray[j+left] == spcarray[j+i+right]))) { // stable sort
					larray[j+left+right]->ull = spcarray[j+left];
					left++;
				} else if (left == i && right == 0) {	// could even make it so that the left elements aren't added until at least one right element has been
					break;
				} else if (right < limright) {
					larray[j+left+right]->ull = spcarray[j+i+right];
					right++;
				} else {
					break;
				}
			}
		}
	}
	free(larray);
	free(spcarray);
	
	return 0;
}

char killtwoschn(twoslnk *slink, unsigned char flag) {	// from lowest-order bit to highest 0 for string, 1 for ull
	twoslnk *buf;
	if (!slink) {
		errorf("no slink-4");
		return 2;
	}
	while (slink != 0) {
		if (slink->u[0].str != 0 && !(flag & 1)) {
			free(slink->u[0].str);
		}
		if (slink->u[1].str != 0 && !(flag & 2)) {
			free(slink->u[1].str);
		}
		buf = slink;
		slink = slink->next;
		free(buf);		
	}
	return 1;
}

twoslnk *copytwoschn(twoslnk *slink, unsigned char flag) {	// from lowest-order bit to highest 0 for null-terminated string, 1 for ull
	twoslnk *buf, *flink;
	int i;
	
	if (!slink || flag > 3) {
		errorf("no slink or flag over 3");
		return 0;
	}	
	if (!(buf = flink = malloc(sizeof(twoslnk)))) {
		errorf("malloc failed");
		return 0;
	}
	flink->u[0].str = 0, flink->u[1].str = 0;
	
	while (slink != 0) {
		if (!(buf->next = malloc(sizeof(twoslnk)))) {
			errorf("malloc failed");
			killtwoschn(flink, flag);
			return 0;
		}
		buf = buf->next;
		buf->next = 0;
		if (!(flag & 1)) {
			if (slink->u[0].str != 0) {
				if (!(buf->u[0].str = dupstr(slink->u[0].str, COPYCHN_STRLIM, 0))) {
					errorf("copied string is NULL");
				}
			} else {
				buf->u[0].str = 0;
			}
		} else {
			buf->u[0].ull = slink->u[0].ull;
		}
		if (!(flag & 2)) {
			if (slink->u[1].str != 0) {
				if (!(buf->u[1].str = dupstr(slink->u[1].str, COPYCHN_STRLIM, 0))) {
					errorf("copied string is NULL");
				}
			} else {
				buf->u[1].str = 0;
			}
		} else {
			buf->u[1].ull = slink->u[1].ull;
		}
		slink = slink->next;
	}
	buf = flink;
	flink = flink->next;
	free(buf);

	return flink;
}

char sorttwoschn(twoslnk **slink, int (*compare)(void *, void *), unsigned char sel, unsigned char descending) {	//! untested
	long long i, j, nlinks, left, right, limright;
	twoslnk *link, **larray, **bufarray;
	int c;
	
	if (!slink || !*slink || (sel > 1)) {
		errorf("no slink or sel over 1");
		return 1;
	}
	
	for (nlinks = 1, link = *slink; link->next != 0; nlinks++, link = link->next);
	
	if (!(larray = malloc(nlinks*sizeof(twoslnk*)))) {
		errorf("first malloc failed in sortoneschn");
		return 2;
	}
	if (!(bufarray = malloc(nlinks*sizeof(twoslnk*)))) {
		errorf("second malloc failed in sortoneschn");
		free(larray);
		return 2;
	}
	
	
	for (i = 0, link = *slink; link != 0 && i < nlinks; larray[i] = link, link = link->next, i++);
	
	for (i = 1; i < nlinks; i *= 2) {
		for (j = 0; j < nlinks; bufarray[j] = larray[j], j++);
		
		for (j = 0; j+i < nlinks; j += 2*i) {
			if (j+2*i > nlinks) {
				limright = nlinks-j-i;
			} else {
				limright = i;
			}
			for (left = 0, right = 0;;) {
				if ((left < i) && ((right == limright) || (((c = compare(bufarray[j+left]->u[sel].vp, bufarray[j+i+right]->u[sel].vp)) < 0) ^ descending) || (c == 0))) {
					larray[j+left+right] = bufarray[j+left];
					left++;
				} else if (left == i && right == 0) {	// could even make it so that the left elements aren't added until at least one right element has been
					break;
				} else if (right < limright) {
					larray[j+left+right] = bufarray[j+i+right];
					right++;
				} else {
					break;
				}
			}
		}
	}
	free(bufarray);
	for (i = 0, larray[nlinks-1]->next = 0; i < nlinks-1; larray[i]->next = larray[i+1], i++);
	*slink = larray[0];
	free(larray);
	
	return 0;
}

char sorttwoschnull(twoslnk **slink, unsigned char sel, unsigned char descending) {	//! untested
	long long i, j, nlinks, left, right, limright;
	twoslnk *link, **larray, **bufarray;
	int c;
	
	if (!slink || !*slink || (sel > 1)) {
		errorf("no slink or sel over 1");
		return 1;
	}
	
	for (nlinks = 1, link = *slink; link->next != 0; nlinks++, link = link->next);
	
	if (!(larray = malloc(nlinks*sizeof(twoslnk*)))) {
		errorf("first malloc failed in sortoneschn");
		return 2;
	}
	if (!(bufarray = malloc(nlinks*sizeof(twoslnk*)))) {
		errorf("second malloc failed in sortoneschn");
		free(larray);
		return 2;
	}
	
	
	for (i = 0, link = *slink; link != 0 && i < nlinks; larray[i] = link, link = link->next, i++);
	
	for (i = 1; i < nlinks; i *= 2) {
		for (j = 0; j < nlinks; bufarray[j] = larray[j], j++);
		
		for (j = 0; j+i < nlinks; j += 2*i) {
			if (j+2*i > nlinks) {
				limright = nlinks-j-i;
			} else {
				limright = i;
			}
			for (left = 0, right = 0;;) {
				if ((left < i) && ((right == limright) || ((bufarray[j+left]->u[sel].ull < bufarray[j+i+right]->u[sel].ull) ^ descending) || (bufarray[j+left]->u[sel].ull == bufarray[j+i+right]->u[sel].ull))) {
					larray[j+left+right] = bufarray[j+left];
					left++;
				} else if (left == i && right == 0) {	// could even make it so that the left elements aren't added until at least one right element has been
					break;
				} else if (right < limright) {
					larray[j+left+right] = bufarray[j+i+right];
					right++;
				} else {
					break;
				}
			}
		}
	}
	free(bufarray);
	for (i = 0, larray[nlinks-1]->next = 0; i < nlinks-1; larray[i]->next = larray[i+1], i++);
	*slink = larray[0];
	free(larray);
	
	return 0;
}