objects = main.o arrayarithmetic.o bytearithmetic.o resources.o portables.o breakpath.o dirfiles.o indextools.o images.o ioextras.o stringchains.o tfiles.o dupstr.o openclfunc.o errorfcpp.o uiutils.o portablescpp.o ioextrascpp.o prgdir.o

#listdirobj = dupstr.c stringchains.c portables.c breakpath.c
listdirobj = arrayarithmetic.o bytearithmetic.o resources.o portables.o breakpath.o dirfiles.o fentries.o images.o ioextras.o stringchains.o tfiles.o dupstr.o

#CFLAGS = -ggdb -Wall
#CFLAGS = -ggdb -Wreturn-type -Werror
CFLAGS = -ggdb -w

CPPFLAGS = $(CFLAGS)

# for std::filesystem
CPPFLAGS += -std=c++20

# !! For C conversion:
CPPFLAGS += -fpermissive

#ICUARG = $(shell pkg-config --cflags --libs icu-uc)

fti: prog/fti.exe

prog/fti.exe: $(objects) Makefile
	g++ $(objects) -o prog/fti $(ICUARG) C:\Windows\System32\OpenCL.dll -municode -mwindows -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
	cp "openclkernels.cl" "./prog/openclkernels.cl"
	
#listdir: listdirtofile.c $(listdirobj) Makefile
#	g++ listdirtofile.c -o listdir $(objects) $(ICUARG) -municode -mwindows -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
#	g++ listdirtofile.c -o listdir $(listdirobj) $(ICUARG) -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
	
clean:
	rm *.o

resources.o: resources.rc Application.manifest
	windres resources.rc -o resources.o --use-temp-file

main.o : userinterface.hpp breakpath.h indextools.hpp stringchains.h images.h dupstr.h arrayarithmetic.h bytearithmetic.h portables.h tfiles.h errorf.hpp userinterface.hpp uiutils.hpp portables.hpp

indextools.o : indextools.hpp indextools_static.hpp ioextras.hpp