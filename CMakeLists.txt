cmake_minimum_required(VERSION 3.20)

project(FTI)

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 20)

### macros

# define a simple macro
macro(assert TEST COMMENT)
  if(NOT ${TEST})
    message("Assertion failed: ${COMMENT}")
  endif()
endmacro()

###

# search for pkg-config
include (FindPkgConfig)
if (NOT PKG_CONFIG_FOUND)
    message (FATAL_ERROR "pkg-config not found")
endif ()

# check for libpng
pkg_check_modules (LIBPNG libpng16 REQUIRED)
if (NOT LIBPNG_FOUND)
    message(FATAL_ERROR "You don't seem to have libpng16 development libraries installed")
else ()
    include_directories (${LIBPNG_INCLUDE_DIRS})
    link_directories (${LIBPNG_LIBRARY_DIRS})
    link_libraries (${LIBPNG_LIBRARIES})
endif ()

pkg_check_modules (LIBJPEG libjpeg REQUIRED)
if (NOT LIBJPEG_FOUND)
    message(FATAL_ERROR "You don't seem to have libjpeg development libraries installed")
else ()
    include_directories (${LIBJPEG_INCLUDE_DIRS})
    link_directories (${LIBJPEG_LIBRARY_DIRS})
    link_libraries (${LIBJPEG_LIBRARIES})
endif ()

pkg_check_modules (ICUUC icu-uc REQUIRED)
if (NOT ICUUC_FOUND)
    message(FATAL_ERROR "You don't seem to have icu-uc development libraries installed")
else ()
    include_directories (${ICUUC_INCLUDE_DIRS})
    link_directories (${ICUUC_LIBRARY_DIRS})
    link_libraries (${ICUUC_LIBRARIES})
endif ()

### main

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set (CMAKE_FLAGS "")
list(APPEND CMAKE_CFLAGS " -ggdb")
# list(APPEND CMAKE_CFLAGS "-Wreturn-type -Wuninitialized")
list(APPEND CMAKE_CFLAGS "-Wall")

# warnings to errors
# list(APPEND CMAKE_CFLAGS "-Werror")

list(APPEND CMAKE_CFLAGS "-Wno-error=unused-variable -Wno-error=unused-but-set-variable")
# list(APPEND CMAKE_CFLAGS "-Wextra")
# list(APPEND CMAKE_CFLAGS "-Wpedantic")
list(APPEND CMAKE_CFLAGS "-fmax-errors=10")
# list(APPEND CMAKE_CFLAGS "-w")
# set(CMAKE_CXX_FLAGS "${CMAKE_CFLAGS} -std=c++20 -fpermissive")
list(APPEND CMAKE_CXX_FLAGS ${CMAKE_CFLAGS})
list(APPEND CMAKE_CXX_FLAGS "-std=c++20")
# list(APPEND CMAKE_CXX_FLAGS "-fpermissive")

list(JOIN CMAKE_CFLAGS " " CMAKE_CFLAGS)
list(JOIN CMAKE_CXX_FLAGS " " CMAKE_CXX_FLAGS)

set(CMAKE_RC_COMPILER_INIT windres)
ENABLE_LANGUAGE(RC)
SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT> --use-temp-file")

set(FTI_SRC_DIR ${CMAKE_BINARY_DIR}/src)

add_subdirectory(src)
add_subdirectory(data)

add_executable(fti WIN32 $<TARGET_OBJECTS:main_src_objects> $<TARGET_OBJECTS:main_data_objects>)

# TODO: link OpenCL in a better way if possible (without direct path to file)
SET_TARGET_PROPERTIES(fti PROPERTIES LINK_FLAGS "-municode -mwindows C:/Windows/System32/OpenCL.dll -lcomctl32 -lole32 -luuid")

# TODO: later make the image libs only required as part of an image api
target_link_libraries(fti LIBPNG)
target_link_libraries(fti LIBJPEG)
# TODO: fix (fix what??)
target_link_libraries(fti GIF)

target_link_libraries(fti ICUUC)

### GoogleTest

add_subdirectory(tests)