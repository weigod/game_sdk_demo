# - Try to find LIBYUV
#
# The following variables are optionally searched for defaults
#  LIBYUV_ROOT_DIR:            Base directory where all LIBYUV components are found
#
# The following are set after configuration is done: 
#  LIBYUV_FOUND
#  LIBYUV_INCLUDE_DIRS
#  LIBYUV_LIBRARIES
#  LIBYUV_LIBRARIES_MT       windows only, for mt & mtd

include(FindPackageHandleStandardArgs)
set(LIBYUV_ROOT_DIR ${THIS_THIRDPARTY_PATH}/libyuv)

if(WIN32)
  set(WINDOWS_PLATFORM win64)
  set(CHECK_PLATFOR_VAR ${CMAKE_GENERATOR_PLATFORM})
  if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "")
    set(CHECK_PLATFOR_VAR ${CMAKE_GENERATOR})
  endif()
  if (NOT "${CHECK_PLATFOR_VAR}" MATCHES "(Win64|IA64|x64)")
    set(WINDOWS_PLATFORM win32)
  endif()
endif()

if(WIN32)
  find_path(LIBYUV_INCLUDE_DIR libyuv.h
    PATHS ${LIBYUV_ROOT_DIR}/include)
elseif(ANDROID OR IOS)
  # because android.toolchain.cmake set CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY
  # so need set NO_CMAKE_FIND_ROOT_PATH
  find_path(LIBYUV_INCLUDE_DIR libyuv.h
    HINTS ${LIBYUV_ROOT_DIR}/include NO_CMAKE_FIND_ROOT_PATH)
else()
  find_path(LIBYUV_INCLUDE_DIR libyuv.h
    HINTS ${LIBYUV_ROOT_DIR}/include)
endif()

if(MSVC)
  find_library(LIBYUV_LIBRARY_RELEASE yuv.lib PATHS ${LIBYUV_ROOT_DIR}/lib/${WINDOWS_PLATFORM}/Release)
  find_library(LIBYUV_LIBRARY_DEBUG yuv.lib PATHS ${LIBYUV_ROOT_DIR}/lib/${WINDOWS_PLATFORM}/Debug)
  set(LIBYUV_LIBRARY optimized ${LIBYUV_LIBRARY_RELEASE} debug ${LIBYUV_LIBRARY_DEBUG})

  find_library(LIBYUV_LIBRARY_RELEASE_MT yuvmt.lib PATHS ${LIBYUV_ROOT_DIR}/lib/${WINDOWS_PLATFORM}/Release)
  find_library(LIBYUV_LIBRARY_DEBUG_MT yuvmtd.lib PATHS ${LIBYUV_ROOT_DIR}/lib/${WINDOWS_PLATFORM}/Debug)
  set(LIBYUV_LIBRARIES_MT optimized ${LIBYUV_LIBRARY_RELEASE_MT} debug ${LIBYUV_LIBRARY_DEBUG_MT})
elseif(ANDROID)
  find_library(LIBYUV_LIBRARY yuv HINTS ${LIBYUV_ROOT_DIR}/lib/android/armeabi-v7a NO_CMAKE_FIND_ROOT_PATH)
elseif(IOS)
  find_library(LIBYUV_LIBRARY yuv HINTS ${LIBYUV_ROOT_DIR}/lib/ios/arm64 NO_CMAKE_FIND_ROOT_PATH)
else()
  find_library(LIBYUV_LIBRARY yuv HINTS ${LIBYUV_ROOT_DIR}/lib/linux)
endif()

find_package_handle_standard_args(LibYUV DEFAULT_MSG
  LIBYUV_INCLUDE_DIR LIBYUV_LIBRARY)

if(LIBYUV_FOUND)
  set(LIBYUV_INCLUDE_DIRS ${LIBYUV_INCLUDE_DIR})
  set(LIBYUV_LIBRARIES ${LIBYUV_LIBRARY})

  set(ExportedTargetName LibYuv::LibYuv)
  if(NOT TARGET ${ExportedTargetName})
    add_library(${ExportedTargetName} INTERFACE IMPORTED)

    if (WIN32)
      set(HELPER_LIBRARIES_RELEASE ${LIBYUV_LIBRARY_RELEASE})
      set(HELPER_LIBRARIES_DEBUG ${LIBYUV_LIBRARY_DEBUG})
    else()
      set(HELPER_LIBRARIES_RELEASE ${LIBYUV_LIBRARY})
      set(HELPER_LIBRARIES_DEBUG ${LIBYUV_LIBRARY})
    endif()

    set_target_properties(${ExportedTargetName} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LIBYUV_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "$<$<CONFIG:Debug>:${HELPER_LIBRARIES_DEBUG}>$<$<CONFIG:Release>:${HELPER_LIBRARIES_RELEASE}>"
      INTERFACE_COMPILE_DEFINITIONS "HAVE_JPEG"
    )

    unset(HELPER_LIBRARIES_RELEASE)
    unset(HELPER_LIBRARIES_DEBUG)
  endif()
  unset(ExportedTargetName)
endif()

message("LIBYUV_FOUND= ${LIBYUV_FOUND}")

