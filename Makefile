OBJDIR = obj
SRCDIR = src
BUILDDIR = build
DATADIR = data
BINDIR = bin

OPENCLPATH = C:\Windows\System32\OpenCL.dll

CC = g++

objects0 = main.o arrayarithmetic.o bytearithmetic.o resources.o portables.o breakpath.o dirfiles.o indextools.o images.o ioextras.o stringchains.o tfiles.o dupstr.o openclfunc.o errorfcpp.o uiutils.o portablescpp.o ioextrascpp.o prgdir.o errorobj.o

objects = $(addprefix $(OBJDIR)/, $(objects0) )

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

fti: $(BINDIR)/fti.exe $(BINDIR)/openclkernels.cl

$(BINDIR)/fti.exe: $(objects) Makefile
	$(CC) $(objects) -o $(BINDIR)/fti $(ICUARG) $(OPENCLPATH) -municode -mwindows -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
	
$(BINDIR)/openclkernels.cl:
	cp "./$(DATADIR)/openclkernels.cl" "./$(BINDIR)/openclkernels.cl"
	
#listdir: listdirtofile.c $(listdirobj) Makefile
#	$(CC) listdirtofile.c -o listdir $(objects) $(ICUARG) -municode -mwindows -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
#	$(CC) listdirtofile.c -o listdir $(listdirobj) $(ICUARG) -ljpeg -lpng -lgif -licuuc -lcomctl32 -lole32 -luuid
	
clean:
	rm $(OBJDIR)/*.o
	
$(OBJDIR):
	mkdir -p $(OBJDIR)

obj/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/resources.o: $(BUILDDIR)/resources.rc $(BUILDDIR)/Application.manifest
	windres $(BUILDDIR)/resources.rc -o $(OBJDIR)/resources.o --use-temp-file

main.o : $(objects)

indextools.o : indextools.hpp indextools_static.hpp ioextras.hpp