#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <iostream>

#include "ioextras.hpp"

#include "errorf.hpp" //! 

#define errorf(str) g_errorfStdStr(str) //!

void put_u64_stream_pref(std::ostream &ios, uint64_t uint) {
	unsigned char str[9];
	
	int i = 8;
	for (; i > 0; i--) {
		str[i] = uint % 256;
		uint /= 256;
		if (uint == 0) {
			break;
		}
	}
	
	if (i <= 0) {
		errorf("error: i <= 0");
		ios.setstate(std::ios_base::badbit | std::ios_base::failbit);
	} else {
		int len = 9-i;
		i--;
		str[i] = len;
g_errorfStream << "writing: " << uint << std::flush;
g_errorfStream << "i: " << i << ", len: " << len << std::flush;
	
		ios.write((char *) &str[i], len+1);
	}
}

uint64_t get_u64_stream_pref(std::istream &ios, bool &b_gotNull) {
	int_fast16_t uc = ios.get();
	if (uc > 8 || ios.fail()) {
		if (ios.eof()) {
			ios.clear(std::ios_base::eofbit);
		} else {
			ios.setstate(std::ios_base::failbit);
		}
		return 0;
	} else if (uc <= 0) {
		b_gotNull = true;
		return 0;
	} else {
		uint64_t uint = 0;
		for (int_fast16_t c = 0; uc > 0; uc--) {
			c = ios.get();
			if (ios.eof()) {
				// note: it is set regardless by ios.get()
				ios.setstate(std::ios_base::failbit);
				return 0;
			} else {
				uint += c;
			}
		}
		return uint;
	}
}

void put_u64_stream_fixed(std::ostream &ios, uint64_t uint) {
	unsigned char str[9];
	
	int i = 8;
	for (; i > 0; i--) {
		str[i] = uint % 256;
		uint /= 256;
	}
	
	if (i < 0) {
		errorf("error: i < 0");
		ios.setstate(std::ios_base::badbit | std::ios_base::failbit);
	} else {
		int len = 8;
		i = 1;
	
		ios.write((char *) &str[i], len);
	}
}

uint64_t get_u64_stream_fixed(std::istream &ios) {
	int_fast16_t uc = 8;
	
	if (ios.peek() == EOF) {
		ios.setstate(std::ios_base::eofbit);
	} else {
		uint64_t uint = 0;
		for (int_fast16_t c = 0; uc > 0; uc--) {
			c = ios.get();
			if (ios.eof()) {
				// note: it is set regardless by ios.get()
				ios.setstate(std::ios_base::failbit);
				return 0;
			} else {
				uint += c;
			}
		}
		return uint;
	}
	return 0;
}

/*
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

char null_fgets(char *str, int32_t n, FILE *stream) {
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

char pref_fgets(char *str, int32_t n, FILE *stream) {
	int32_t i, c, d;
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

uint64_t fgetull_pref(FILE *stream, int32_t *feedback) {
	int32_t c, i, d;
	uint64_t result = 0;
	
	c = getc(stream);
	if (c == EOF) {
		if (feedback)
			*feedback = ULL_READ_EOF;
		return 0;
	} if (c >= 9) {
		if (feedback)
			*feedback = ULL_READ_OVERFLOW;
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
				*feedback = ULL_READ_LATE_EOF;
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
*/