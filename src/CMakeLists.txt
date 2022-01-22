set(MAIN_SOURCES main.cpp arrayarithmetic.c bytearithmetic.c portables.c breakpath.c dirfiles.c indextools.cpp indextools_private.cpp images.c ioextras.c stringchains.c tfiles.c dupstr.c openclfunc.c errorfcpp.cpp uiutils.cpp portablescpp.cpp ioextrascpp.cpp prgdir.cpp errorobj.cpp)

add_library(main_src_objects OBJECT ${MAIN_SOURCES})