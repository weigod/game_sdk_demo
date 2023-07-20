cmake_minimum_required(VERSION 2.8.12)
include(CMakeDependentOption)

project(SDKDemo)

# cpp 17 support
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

set(THIS_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/thirdparty)
#set(THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/../../thirdparty)
message("THIRDPARTY_PATH= ${THIRDPARTY_PATH}")

set(CMAKE_MODULE_PATH
  ${THIS_THIRDPARTY_PATH}/CMake
  ${THIRDPARTY_PATH}/CMake
  ${CMAKE_MODULE_PATH}
)

# Force out-of-source build
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR
    "This project requires an out of source build. Remove the file 'CMakeCache.txt' found in this directory before continuing, create a separate build directory and run 'cmake [options] <srcs>' from there."
  )
endif()

message("CMAKE_SOURCE_DIR= ${CMAKE_SOURCE_DIR}")
message("CMAKE_BINARY_DIR= ${CMAKE_BINARY_DIR}")

# TODO: allow other compilers under windows in the future
if(WIN32 AND NOT MSVC)
  message(FATAL_ERROR
    "Under Windows other compiler than Microsoft Visual Studio are not supported."
  )
endif()

# TODO: remove this in the future! it has only been added to make the switch easier for end users
if(PREFIX)
  message(FATAL_ERROR "The parameter PREFIX has been removed. Please re-run CMake and use CMAKE_INSTALL_PREFIX instead to define your installation location!")
endif()

set(md "md")
option(MT "MSVC-MT" OFF)
option(MediaPipeline_DCheckAlwaysOn "RTC_DCHECK always on, even release builds" OFF)
option(MediaPipeline_EnableVLD "enable VLD" OFF)

if(${MT})
if(MSVC) 
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
endif(MSVC)
endif()

# Override configuration-types - we don't use anything else than debug and release
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Release Debug)
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
    "Reset the configurations to what we need"
    FORCE)
endif()

# set_target_properties(xxxx PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#output-artifacts

# CMAKE_ARCHIVE_OUTPUT_DIRECTORY 
# https://cmake.org/cmake/help/latest/variable/CMAKE_ARCHIVE_OUTPUT_DIRECTORY.html

# CMAKE_LIBRARY_OUTPUT_DIRECTORY 
# https://cmake.org/cmake/help/latest/variable/CMAKE_LIBRARY_OUTPUT_DIRECTORY.html

# CMAKE_RUNTIME_OUTPUT_DIRECTORY 
# https://cmake.org/cmake/help/latest/variable/CMAKE_RUNTIME_OUTPUT_DIRECTORY.html

set(IS_WIN64 OFF)
if (WIN32)
  # https://cmake.org/cmake/help/latest/variable/CMAKE_GENERATOR_PLATFORM.html#variable:CMAKE_GENERATOR_PLATFORM
  message(STATUS "CMAKE_GENERATOR_PLATFORM:${CMAKE_GENERATOR_PLATFORM}")
  if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "")
    SET(CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR})
  endif()

  if (NOT "${CMAKE_GENERATOR_PLATFORM}" MATCHES "(Win64|IA64|x64)")
    set(IS_WIN64 OFF)
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin32)
  else()
    set(IS_WIN64 ON)
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
  endif()

  # add /utf-8 flag for msvc compiler.
  if(MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
  endif()
else()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif()
message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY:${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
set(CONF_DIR ${CMAKE_INSTALL_PREFIX}/etc)

# If win32 put it in the bin dir not lib
if(WIN32)
  set(LIBS_DIR ${CMAKE_INSTALL_PREFIX}/bin)
else()
  set(LIBS_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

# For Unix systems set the rpath so that libraries are found
set(CMAKE_INSTALL_RPATH $ORIGIN ${LIBS_DIR})
set(CMAKE_INSTALL_NAME_DIR ${LIBS_DIR})
message("CMAKE_INSTALL_RPATH= ${CMAKE_INSTALL_RPATH}")
# Run out of build tree
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(PCH AND NOT PCHSupport_FOUND)
  set(PCH 0 CACHE BOOL
    "Use precompiled headers"
    FORCE)
  message(WARNING "No PCH for your system possible but PCH was set to 1. Resetting it."
  )
endif()
if(PCH)
  message(STATUS "Use PCH               : Yes")
else()
  message(STATUS "Use PCH               : No")
endif()

if(DEBUG)
  message(STATUS "Build in debug-mode   : Yes")
  set(CMAKE_BUILD_TYPE Debug)
else()
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Build in debug-mode   : No  (default)")
endif()
# Handle debugmode compiles (this will require further work for proper WIN32-setups)
if(UNIX)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
endif()

# Set warning levels for different builds
if(WIN32)
  message("-- Platform windows")
  # Disable warnings in Visual Studio 8 and above and add /MP
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi /wd4996 /wd4355 /wd4244 /wd4985 /wd4267 /wd4251 /wd4819 /MP")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /wd4996 /wd4355 /wd4244 /wd4267 /wd4251 /wd4819 /MP")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /wd4996 /wd4355 /wd4244 /wd4985 /wd4267 /wd4251 /wd4819 /MP")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /wd4996 /wd4355 /wd4244 /wd4985 /wd4267 /wd4251 /wd4819 /MP")
  # https://www.wintellect.com/correctly-creating-native-c-release-build-pdbs
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF /ignore:4099")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF /ignore:4099")
  add_definitions(-DNOMINMAX)
elseif(ANDROID)
  message("-- Platform android")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fPIC --no-warnings")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC -std=c++14 --no-warnings -frtti")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fPIC -Wall -Wfatal-errors -Wextra")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC -std=c++14 -Wall -Wfatal-errors -Wextra -frtti")
  if(NOT(ANDROID_NATIVE_API_LEVEL LESS 21))
    # for std::llround cmath
    add_definitions(-D_GLIBCXX_USE_C99_MATH_TR1)
  endif()
elseif(IOS)
  message("-- Platform ios")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fPIC -fvisibility=hidden --no-warnings")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC -std=c++14 -fvisibility=hidden --no-warnings")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fPIC -fvisibility=hidden -Wall -Wfatal-errors -Wextra")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC -std=c++14 -fvisibility=hidden -Wall -Wfatal-errors -Wextra")
elseif(UNIX)
  message("-- Platform unix")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fPIC -fvisibility=hidden -Werror=return-type -Wno-unused-result -Wno-deprecated")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC -std=c++14 -fvisibility=hidden -Werror=return-type -Wno-unused-result -Wno-deprecated")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fPIC -fvisibility=hidden -Wall -Wfatal-errors -Wextra -Werror=return-type -Wno-unused-result -Wno-deprecated")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC -std=c++14 -fvisibility=hidden -Wall -Wfatal-errors -Wextra -Werror=return-type -Wno-unused-result -Wno-deprecated")
  set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
endif()

# Add definitions for all build types
# Don't place this above 'dep' subdirectory! Because of defines build will crash.
set(DEFINITIONS _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)

if(MediaPipeline_DCheckAlwaysOn)
  set(DEFINITIONS ${DEFINITIONS} DCHECK_ALWAYS_ON)
endif()

set(DEFINITIONS_RELEASE NDEBUG)
set(DEFINITIONS_DEBUG _DEBUG)
if(WIN32)
  set(DEFINITIONS ${DEFINITIONS} WIN32 _WIN32 WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
  set(DEFINITIONS_RELEASE ${DEFINITIONS} ${DEFINITIONS_RELEASE} _CRT_SECURE_NO_WARNINGS)
  set(DEFINITIONS_DEBUG ${DEFINITIONS} ${DEFINITIONS_DEBUG} _CRT_SECURE_NO_WARNINGS)
endif()

if (DEFINED PRODUCT_VERSION_MAJOR)
set(DEFINITIONS ${DEFINITIONS} PRODUCT_VERSION_MAJOR=${PRODUCT_VERSION_MAJOR})
endif()

if (DEFINED PRODUCT_VERSION_MINOR)
set(DEFINITIONS ${DEFINITIONS} PRODUCT_VERSION_MINOR=${PRODUCT_VERSION_MINOR})
endif()

if (DEFINED PRODUCT_VERSION_PATCH)
set(DEFINITIONS ${DEFINITIONS} PRODUCT_VERSION_PATCH=${PRODUCT_VERSION_PATCH})
endif()

if (DEFINED PRODUCT_VERSION_RELEASE)
set(DEFINITIONS ${DEFINITIONS} PRODUCT_VERSION_RELEASE=${PRODUCT_VERSION_RELEASE})
endif()

macro(SET_COMMON_TARGET_PROPERTIES target)
  message("target:${target}")
  target_compile_definitions(${target} PUBLIC ${DEFINITIONS})
  target_compile_definitions(${target} PUBLIC $<$<CONFIG:Debug>:${DEFINITIONS_DEBUG}>)
  target_compile_definitions(${target} PUBLIC $<$<CONFIG:Release>:${DEFINITIONS_RELEASE}>)
endmacro()

if(WIN32)
  add_definitions(-DOS_WIN -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE -DWEBRTC_WIN -DWEBRTC_NS_FLOAT -DBOOST_PYTHON_STATIC_LIB)
else()
  add_definitions(-DWEBRTC_POSIX -DWEBRTC_NS_FLOAT -DBOOST_PYTHON_STATIC_LIB)
endif()

# find common package: windows linux android
add_subdirectory(base)
add_subdirectory(GameDemo)
