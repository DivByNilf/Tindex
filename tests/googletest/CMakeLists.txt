### GoogleTest

# TODO: move this to the tests cmakelists

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


#find_package(gtest REQUIRED)

add_executable(
    all_tests
    all_tests.cc
    hello_test.cc
    indextools_private_tests.cc
    $<TARGET_OBJECTS:main_src_objects>
)

target_link_libraries(
    all_tests
    #hello_test
    gtest_main
)
  
include(GoogleTest)
gtest_discover_tests(all_tests)

#target_include_directories(indextools_static_tests.cc.obj PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# just the main source directory of the project (./src)
target_include_directories(all_tests PRIVATE ${FTI_SRC_DIR})

set(OUT )

get_target_property(OUT fti LINK_LIBRARIES)

# get things such as libpng, libjpeg

target_link_libraries(all_tests ${OUT})

# OpenCL references

SET_TARGET_PROPERTIES(all_tests PROPERTIES LINK_FLAGS "C:/Windows/System32/OpenCL.dll")



#message("CMAKE_CURRENT_SOURCE_DIR: " ${FTI_SRC_DIR})
#include_directories(${FTI_SRC_DIR})
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)