objects = FileTagIndex.o arrayarithmetic.o bytearithmetic.o errorf.o resources.o portables.o breakpath.o dirfiles.o fentries.o images.o ioextras.o stringchains.o tfiles.o dupstr.o openclfunc.o

#listdirobj = dupstr.c stringchains.c portables.c breakpath.c
listdirobj = arrayarithmetic.o bytearithmetic.o resources.o portables.o breakpath.o dirfiles.o fentries.o images.o ioextras.o stringchains.o tfiles.o dupstr.o

#CFLAGS = -ggdb -Wall
CFLAGS = -ggdb -Wreturn-type -Werror

#ICUARG = $(shell pkg-config --cflags --libs icu-uc)

fti: prog/fti

prog/fti: $(objects) Makefile
	gcc $(objects) -o prog/fti $(ICUARG) C:\Windows\System32\OpenCL.dll -municode -mwindows -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
	cp "openclkernels.cl" "./prog/openclkernels.cl"

resources.o: resources.rc Application.manifest
	windres resources.rc -o resources.o --use-temp-file
	
listdir: listdirtofile.c $(listdirobj) Makefile
#	gcc listdirtofile.c -o listdir $(objects) $(ICUARG) -municode -mwindows -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
	gcc listdirtofile.c -o listdir $(listdirobj) $(ICUARG) -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
	
clean:
	rm *.o