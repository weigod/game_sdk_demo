# - Try to find ControllerSDK
#
# The following variables are optionally searched for defaults
#  ControllerSDK_ROOT_DIR:            Base directory where all ControllerSDK components are found
#
# The following are set after configuration is done: 
#  ControllerSDK_FOUND
#  ControllerSDK_INCLUDE_DIRS
#  ControllerSDK_LIBRARIES
#  ControllerSDK_BINS

include(FindPackageHandleStandardArgs)
set(ControllerSDK_ROOT_DIR ${THIS_THIRDPARTY_PATH}/ControllerSDK)

if(WIN32)
  find_path(ControllerSDK_INCLUDE_DIR ControllerAPI.h HINTS ${ControllerSDK_ROOT_DIR}/include)
else()
  message("ControllerSDK NOT SUPPORT LINUX RIGHT NOW!!!")
endif()

if(WIN32)
  set(WINDOWS_PLATFORM win64)
  if (NOT "${CMAKE_GENERATOR_PLATFORM}" MATCHES "(Win64|IA64|x64)")
    set(WINDOWS_PLATFORM win32)
  endif()
endif()

if(WIN32)
  find_library(ControllerSDK_LIBRARY MediaPipelineControllerSDK.lib PATHS ${ControllerSDK_ROOT_DIR}/lib/${WINDOWS_PLATFORM})
  set(ControllerSDK_BIN ${ControllerSDK_ROOT_DIR}/bin/${WINDOWS_PLATFORM})
endif()

find_package_handle_standard_args(ControllerSDK DEFAULT_MSG
  ControllerSDK_INCLUDE_DIR ControllerSDK_LIBRARY ControllerSDK_BIN)

if(ControllerSDK_FOUND)
  set(ControllerSDK_INCLUDE_DIRS ${ControllerSDK_INCLUDE_DIR})
  set(ControllerSDK_LIBRARIES ${ControllerSDK_LIBRARY})
  set(ControllerSDK_BINS ${ControllerSDK_BIN})
endif()

message("ControllerSDK_FOUND = ${ControllerSDK_FOUND}")