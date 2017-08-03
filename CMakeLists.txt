cmake_minimum_required(VERSION 3.2.2)
project(RTS VERSION 0.1 LANGUAGES CXX)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/etc/cmake/Modules)
string(TOLOWER ${PROJECT_NAME} LOWER_PROJECT_NAME)
set(RTS_PROJECT_BRIEF "rts")

# Hook for VSCode CMakeTools
include(CMakeToolsHelpers OPTIONAL)

include(CTest)

if(MSVC)
  message("libm not required")
else()
	set(MATH_LIBRARIES "m")
  message("libm required")
endif()

# use foo.o rather than foo.cpp.o
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

#option(ENABLE_COVERAGE "COVERAGE" ON)
#if(ENABLE_COVERAGE)
  #if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    #add_compile_options(-fprofile-arcs -ftest-coverage -O0)
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
    #set(USE_COVERAGE ON)
  #endif()
#endif()

option(ENABLE_DOCS "DOCS" ON)
if(ENABLE_DOCS)
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    configure_file(${CMAKE_SOURCE_DIR}/etc/Doxyfile.in ${CMAKE_BINARY_DIR}/etc/Doxyfile @ONLY)
    add_custom_target(doc ALL ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/etc/Doxyfile
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  endif(DOXYGEN_FOUND)
else()
  message("Documentation disabled")
endif()

include(CheckCXXCompilerFlag)

# Boost
if(MSVC)
  #CHECK_CXX_COMPILER_FLAG("/MP" CXX_SUPPORTS_MP) 
  #if (CXX_SUPPORTS_MP)
    add_compile_options(/MP) # parallel builds, also needed to avoid write conflicts during parallel testing
  #endif()
endif()

# link with lld if available for speed
option(ENABLE_LLD "LLD" ON)
if (ENABLE_LLD)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -fuse-ld=lld -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LLD_VERSION)
  if(LLD_VERSION MATCHES "LLD")
    SET(LLD_AVAILABLE ON)
    MESSAGE(STATUS "lld linker available")
  endif()
endif()

# link with gold if available for speed
option(ENABLE_GOLD "GOLD" ON)
if (ENABLE_LLD)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE GOLD_VERSION)
  if(GOLD_VERSION MATCHES "GNU gold")
    SET(GOLD_AVAILABLE ON)
    MESSAGE(STATUS "gold linker available")
  endif()
endif()

# favor lld if both are available
if(ENABLE_LLD AND LLD_AVAILABLE)
  MESSAGE(STATUS "Using lld linker")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
  set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
elseif(ENABLE_GOLD AND GOLD_AVAILABLE)
  MESSAGE(STATUS "Using gold linker")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
  set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=gold")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
endif()

# fetch into build dir, we can't cache it in the submodule dir
SET(BOOST_DOWNLOAD_TO_BINARY_DIR ON CACHE BOOL "Download into binary directory for caching")
add_subdirectory("opt/boost-cmake")

CHECK_CXX_COMPILER_FLAG("-std=c++11" CXX_SUPPORTS_CXX11)
# for eventual fallback?

# c++14
CHECK_CXX_COMPILER_FLAG("-std=c++14" CXX_SUPPORTS_CXX14)
if(CXX_SUPPORTS_CXX14)
  if(MSVC)
    add_compile_options("-std=c++14")
  else()
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-std=c++14>")
  endif()
else()
  CHECK_CXX_COMPILER_FLAG("-std=c++1y" CXX_SUPPORTS_CXX1Y)
  if(CXX_SUPPORTS_CXX1Y)
    if(MSVC)
      add_compile_options("-std=c++1y")
    else()
      add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-std=c++1y>")
    endif()
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
  endif()
endif()

# use avx or not
if (ENABLE_NATIVE)
    add_compile_options("-march=native")
else()
OPTION(ENABLE_AVX "-mavx or -mno-avx" ON)
if (ENABLE_AVX)
  if(MSVC)
    add_compile_options("/arch:AVX")
  else()
    CHECK_CXX_COMPILER_FLAG("-mavx" CXX_SUPPORTS_MAVX)
    if(CXX_SUPPORTS_MAVX)
      add_compile_options("-mavx")
    endif()
  endif()
else()
  # MSVC disables AVX by default.
  if(NOT MSVC)
    CHECK_CXX_COMPILER_FLAG("-mno-avx" CXX_SUPPORTS_MNOAVX)
    if(CXX_SUPPORTS_MNOAVX)
      add_compile_options("-mno-avx")
    endif()
  endif()
endif()

# use avx2 or not
OPTION(ENABLE_AVX2 "-mavx2 or -mno-avx2" ON)
if (ENABLE_AVX2)
  if(MSVC)
    add_compile_options("/arch:AVX2")
  else()
    CHECK_CXX_COMPILER_FLAG("-mavx2" CXX_SUPPORTS_MAVX2)
    if(CXX_SUPPORTS_MAVX2)
      add_compile_options("-mavx2")
    endif()
  endif()
else()
  # MSVC disables AVX2 by default.
  if(NOT MSVC)
    CHECK_CXX_COMPILER_FLAG("-mno-avx2" CXX_SUPPORTS_MNOAVX2)
    if(CXX_SUPPORTS_MNOAVX2)
      add_compile_options("-mno-avx2")
    endif()
  endif()
endif()
endif() # no native

# use RTM
OPTION(ENABLE_MRTM "-mrtm or -mno-rtm" OFF)
if (ENABLE_MRTM)
  if(NOT MSVC)
    CHECK_CXX_COMPILER_FLAG("-mrtm" CXX_SUPPORTS_MRTM)
    if(CXX_SUPPORTS_MRTM)
      add_compile_options(-mrtm)
    endif()
  endif()
else()
  if(NOT MSVC)
    CHECK_CXX_COMPILER_FLAG("-mno-rtm" CXX_SUPPORTS_MNORTM)
    if(CXX_SUPPORTS_MNORTM)
      add_compile_options("-mno-rtm")
    endif()
  endif()
endif()

include_directories(${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS} ${Event_INCLUDE_DIRS})

# build the library
add_subdirectory(src)

# build the test suite
if(BUILD_TESTING)
  add_subdirectory(t)
endif()
