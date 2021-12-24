#include <stdio.h>
#include <stdlib.h>
#include "ioextras.h"

#include "bytearithmetic.h"

void term_fputs(char *string, FILE *file) {
	fputs(string, file);
	fputc('\0', file);
	
	return;
}

void pref_fputs(unsigned char *string, FILE *file) {
	int c, i;
	for	(i = 0; i <= string[0]; putc(string[i++], file));
	return;
}

void putull_pref(uint64_t num, FILE *file) {
	unsigned char *str;
	
	str = utob(num);
	pref_fputs(str, file);
	free(str);
	return;
}

char null_fgets(char *str, int n, FILE *stream) {
	int i, c;
	
	if (!str) {
		for (i = 0; (c = getc(stream)) != '\0' && i < n; i++) {
			if (c == EOF) {
				if (i == 0) {
					return 1;
				} return 2;
			}
		}
		if (i >= n) {
			return 3;
		} else
			return 0;
	}
	for (i = 0; (c = getc(stream)) != '\0' && i < n; i++) {
		if (c == EOF) {
			str[i] = '\0';
			if (i == 0) {
				return 1;
			} return 2;
		} str[i] = c;
	}
	if (i >= n) {
		str[n-1] = '\0'; 
		return 3;
	}
	str[i] = '\0';
	return 0;
}

char pref_fgets(char *str, int n, FILE *stream) {
	int i, c, d;
	c = getc(stream);
	if (c == EOF) {
		return 1;
	} if (c >= n) {
		return 3;
	} if (c == 0) {
		return 4;
	}
	if (!str) {
		for (i = 1; i <= c; i++) {
			d = getc(stream);
			if (d == EOF) {
				return 2;
			}
		}
	} else {
		for (str[0] = c, i = 1; i <= c; i++) {
			d = getc(stream);
			if (d == EOF) {
				str[0] = 0;
				return 2;
			} str[i] = d;
		}
	}
	return 0;
}

uint64_t fgetull_pref(FILE *stream, int *feedback) {
	int c, i, d;
	uint64_t result = 0;
	
	c = getc(stream);
	if (c == EOF) {
		if (feedback)
			*feedback = 1;
		return 0;
	} if (c >= 9) {
		if (feedback)
			*feedback = 3;
		return 0;
	} if (c == 0) {
		if (feedback)
			*feedback = ULL_READ_NULL;
		return 0;
	}
	for (i = 1; i <= c; i++) {
		d = getc(stream);
		if (d == EOF) {
			if (feedback)
				*feedback = 2;
			return 0;
		} result *= 256, result += d;
	}
	*feedback = 0;
	return result;
}

uint64_t fgetull_len(FILE *stream, uint8_t len, int32_t *feedback) {
	int32_t c, i, d;
	uint64_t result = 0;
	
	c = len;
	for (i = 1; i <= c; i++) {
		d = getc(stream);
		if (d == EOF) {
			if (feedback)
				*feedback = ULL_READ_LATE_EOF;
			return 0;
		} result *= 256, result += d;
	}
	*feedback = 0;
	return result;
}