# - Try to find SPDLOG
#
# The following variables are optionally searched for defaults
#  SPDLOG_ROOT_DIR:            Base directory where all SPDLOG components are found
#
# The following are set after configuration is done: 
#  SPDLOG_FOUND
#  SPDLOG_INCLUDE_DIRS
#  SPDLOG_LIBRARIES

include(FindPackageHandleStandardArgs)

set(SPDLOG_ROOT_DIR ${THIS_THIRDPARTY_PATH}/spdlog-1.3.1)

if(WIN32)
  find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h
    PATHS ${SPDLOG_ROOT_DIR}/include)
elseif(ANDROID OR IOS)
  # because android.toolchain.cmake set CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY
  # so need set NO_CMAKE_FIND_ROOT_PATH
  find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h
    HINTS ${SPDLOG_ROOT_DIR}/include NO_CMAKE_FIND_ROOT_PATH)
else()
  find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h
    PATHS ${SPDLOG_ROOT_DIR}/include)
endif()

find_package_handle_standard_args(SpdLog DEFAULT_MSG
  SPDLOG_INCLUDE_DIR )

if(SPDLOG_FOUND)
  set(SPDLOG_INCLUDE_DIRS ${SPDLOG_INCLUDE_DIR})
endif()

message("SPDLOG_FOUND= ${SPDLOG_FOUND}")

