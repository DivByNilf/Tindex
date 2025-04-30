cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 23)
add_compile_options(-utf-8)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TinDex)

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

# toggle warning types

list(APPEND CMAKE_CFLAGS "-Wall")
# list(APPEND CMAKE_CFLAGS "-Wextra")
# list(APPEND CMAKE_CFLAGS "-Wpedantic")
list(APPEND CMAKE_CFLAGS "-Wno-error=unused-variable -Wno-error=unused-but-set-variable")
list(APPEND CMAKE_CFLAGS "-Werror=implicit-fallthrough -Werror=return-type")
list(APPEND CMAKE_CFLAGS "-Wimplicit-fallthrough -Wdeprecated-copy -Winfinite-recursion")
# list(APPEND CMAKE_CFLAGS "-Wreturn-type -Wuninitialized")

# warnings to errors

# list(APPEND CMAKE_CFLAGS "-Werror")
list(APPEND CMAKE_CFLAGS "-fmax-errors=15")

## Optimization

# This option instructs the compiler to assume that signed arithmetic overflow of addition, subtraction and multiplication wraps around using twos-complement representation. This flag enables some optimizations and disables others - https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html
list(APPEND CMAKE_CFLAGS "-fwrapv")

# inhibit all warnings
# list(APPEND CMAKE_CFLAGS "-w")
# set(CMAKE_CXX_FLAGS "${CMAKE_CFLAGS} -std=c++19 -fpermissive")
list(APPEND CMAKE_CXX_FLAGS ${CMAKE_CFLAGS})
## specified in CMAKE_CXX_STANDARD
# list(APPEND CMAKE_CXX_FLAGS "-std=c++20")
# list(APPEND CMAKE_CXX_FLAGS "-fpermissive")

list(JOIN CMAKE_CFLAGS " " CMAKE_CFLAGS)
list(JOIN CMAKE_CXX_FLAGS " " CMAKE_CXX_FLAGS)

set(CMAKE_RC_COMPILER_INIT windres)
ENABLE_LANGUAGE(RC)
SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT> --use-temp-file")

set(TINDEX_SRC_DIR ${CMAKE_BINARY_DIR}/src)

add_subdirectory(src)
add_subdirectory(data)

add_executable(TinDex WIN32 $<TARGET_OBJECTS:main_src_objects> $<TARGET_OBJECTS:main_data_objects>)

target_compile_features(TinDex PUBLIC cxx_std_26)

# TODO: link OpenCL in a better way if possible (without direct path to file)
SET_TARGET_PROPERTIES(TinDex PROPERTIES
    LINK_FLAGS "-municode -mwindows C:/Windows/System32/OpenCL.dll -lcomctl32 -lole32 -luuid"
)

# target_compile_options(TinDex PUBLIC -Werror -Wall -Wextra)
# target_compile_options(TinDex PUBLIC -Wimplicit-fallthrough -Wdeprecated-copy -Winfinite-recursion)

# TODO: later make the image libs only required as part of an image api
target_link_libraries(TinDex LIBPNG)
target_link_libraries(TinDex LIBJPEG)
target_link_libraries(TinDex GIF)

target_link_libraries(TinDex ICUUC)

### GoogleTest

# add_subdirectory(tests)
# ExternalProject_Add(GoogleTest [<option>...])
include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/a866428a78ab02b7072f96c8de15802ffd451a7f.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)