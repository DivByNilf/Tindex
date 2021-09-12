#include <stdlib.h>

#include "errorf.h"
#define errorf(...) g_errorf(__VA_ARGS__)

char *dupstr(char *str, unsigned long long maxlen, int option) {	// option to truncate string if it doesn't fit maxlen -- maxlen includes space for null terminator
	long long i;
	char *buf;
	if (maxlen == 0 || str == 0) {
		return 0;
	}
	for (i = 0; str[i] != '\0' && i < maxlen-1; i++);
	if (str[i] != '\0') {
		if (option != 1) {
			return 0;
		}
	}
	if (!(buf = malloc(i+1))) {
		errorf("malloc failed");
		return 0;
	}
	buf[i--] = '\0';
	while (i >= 0) {
		buf[i] = str[i];
		i--;
	}
	return buf;
}

#undef errorf()