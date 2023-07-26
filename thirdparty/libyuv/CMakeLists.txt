# CMakeLists for libyuv
# Originally created for "roxlu build system" to compile libyuv on windows
# Run with -DTEST=ON to build unit tests

PROJECT ( YUV C CXX )	# "C" is required even for C++ projects
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
cmake_policy(SET CMP0063 NEW)
OPTION( TEST "Built unit tests" OFF )
option( BUILD_MT "build MSVC-MT" OFF)

if(BUILD_MT AND MSVC)
  # Use the static C library for all build types MESSAGE(STATUS "link to static  C and C++ runtime lirbary(/MT /MTd)") 
  foreach(var 
          CMAKE_C_FLAGS_DEBUG
          CMAKE_C_FLAGS_RELEASE
          CMAKE_C_FLAGS_MINSIZEREL
          CMAKE_C_FLAGS_RELWITHDEBINFO
          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_MINSIZEREL
          CMAKE_CXX_FLAGS_RELWITHDEBINFO
          ) 
      if (${var} MATCHES "/MD") 
          string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}") 
          #MESSAGE(STATUS "${var}:${${var}}") 
      endif() 
  endforeach()
  set(md "mt")
  message("Runtime Lirbary(/MT)")
  unset(var)
endif()

SET ( ly_base_dir	${PROJECT_SOURCE_DIR} )
SET ( ly_src_dir	${ly_base_dir}/source )
SET ( ly_inc_dir	${ly_base_dir}/include )
SET ( ly_tst_dir	${ly_base_dir}/unit_test )
SET ( ly_lib_name	yuv )
SET ( ly_lib_static	${ly_lib_name} )
SET ( ly_lib_shared	${ly_lib_name}_shared )

FILE ( GLOB_RECURSE	ly_source_files ${ly_src_dir}/*.cc )
LIST ( SORT			ly_source_files )

FILE ( GLOB_RECURSE	ly_unittest_sources ${ly_tst_dir}/*.cc )
LIST ( SORT			ly_unittest_sources )

INCLUDE_DIRECTORIES( BEFORE ${ly_inc_dir} )

# this creates the static library (.a)
ADD_LIBRARY				( ${ly_lib_static} STATIC ${ly_source_files} )

# this creates the shared library (.so)
ADD_LIBRARY				( ${ly_lib_shared} SHARED ${ly_source_files} )
SET_TARGET_PROPERTIES	( ${ly_lib_shared} PROPERTIES OUTPUT_NAME "${ly_lib_name}" )
SET_TARGET_PROPERTIES	( ${ly_lib_shared} PROPERTIES PREFIX "lib" )

# this creates the conversion tool
ADD_EXECUTABLE			( yuvconvert ${ly_base_dir}/util/yuvconvert.cc )
TARGET_LINK_LIBRARIES	( yuvconvert ${ly_lib_static} )

# 改为使用我们第三方库里的libjpeg-turbo by Von @2020年9月9日 16点28分 
set(THIS_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/../)
message("THIS_THIRDPARTY_PATH= ${THIS_THIRDPARTY_PATH}")

set(CMAKE_MODULE_PATH
  ${THIS_THIRDPARTY_PATH}/CMake
  ${CMAKE_MODULE_PATH}
)

find_package(LibJpeg)
if (LIBJPEG_FOUND)
  include_directories( ${LIBJPEG_INCLUDE_DIRS} )
  target_link_libraries( yuvconvert ${LIBJPEG_LIBRARIES} )
  target_link_libraries( ${ly_lib_shared} ${LIBJPEG_LIBRARIES} )
  target_link_libraries( ${ly_lib_static} ${LIBJPEG_LIBRARIES} )
  add_definitions( -DHAVE_JPEG )
endif()
# modification end by Von

if(TEST)
  find_library(GTEST_LIBRARY gtest)
  if(GTEST_LIBRARY STREQUAL "GTEST_LIBRARY-NOTFOUND")
    set(GTEST_SRC_DIR /usr/src/gtest CACHE STRING "Location of gtest sources")
    if(EXISTS ${GTEST_SRC_DIR}/src/gtest-all.cc)
      message(STATUS "building gtest from sources in ${GTEST_SRC_DIR}")
      set(gtest_sources ${GTEST_SRC_DIR}/src/gtest-all.cc)
      add_library(gtest STATIC ${gtest_sources})
      include_directories(${GTEST_SRC_DIR})
      include_directories(${GTEST_SRC_DIR}/include)
      set(GTEST_LIBRARY gtest)
    else()
      message(FATAL_ERROR "TEST is set but unable to find gtest library")
    endif()
  endif()

  add_executable(libyuv_unittest ${ly_unittest_sources})
  target_link_libraries(libyuv_unittest ${ly_lib_name} ${GTEST_LIBRARY})
  find_library(PTHREAD_LIBRARY pthread)
  if(NOT PTHREAD_LIBRARY STREQUAL "PTHREAD_LIBRARY-NOTFOUND")
    target_link_libraries(libyuv_unittest pthread)
  endif()
  # 改为使用我们第三方库里的libjpeg-turbo by Von @2020年9月9日 16点28分 
  if (LIBJPEG_FOUND)
    target_link_libraries(libyuv_unittest ${LIBJPEG_LIBRARIES})
  endif()
  # modification end

  if(NACL AND NACL_LIBC STREQUAL "newlib")
    target_link_libraries(libyuv_unittest glibc-compat)
  endif()

  find_library(GFLAGS_LIBRARY gflags)
  if(NOT GFLAGS_LIBRARY STREQUAL "GFLAGS_LIBRARY-NOTFOUND")
    target_link_libraries(libyuv_unittest gflags)
    add_definitions(-DLIBYUV_USE_GFLAGS)
  endif()
endif()


# install the conversion tool, .so, .a, and all the header files
INSTALL ( PROGRAMS ${CMAKE_BINARY_DIR}/yuvconvert			DESTINATION bin )
INSTALL ( TARGETS ${ly_lib_static}						DESTINATION lib )
INSTALL ( TARGETS ${ly_lib_shared} LIBRARY				DESTINATION lib RUNTIME DESTINATION bin )
INSTALL ( DIRECTORY ${PROJECT_SOURCE_DIR}/include/		DESTINATION include )

# create the .deb and .rpm packages using cpack
INCLUDE ( CM_linux_packages.cmake )

