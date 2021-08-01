#include "dirfiles.c"
#ifndef FILE
#include <stdio.h>
#endif

char *PrgDir = "F:\\TobiFalowo\\Documents\\Programming\\FTI";

int main(int argc, char **argv) {
	oneslnk *flink,  *link;
	FILE *file;
	char buf[MAX_PATH*4];
	
	flink = listfileschn(argv[1]);
	sprintf(buf, "%s\\test.txt", PrgDir);
	file = fopen(buf, "w");
	for (link = flink; link != 0; link = link->next) {
		if (link->str)
			fprintf(file, "%s\n", link->str);
	}
	fclose(file);
}