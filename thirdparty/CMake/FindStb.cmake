# - Try to find STB
#
# The following variables are optionally searched for defaults
#  STB_ROOT_DIR:            Base directory where all STB components are found
#
# The following are set after configuration is done: 
#  STB_FOUND
#  STB_INCLUDE_DIRS
#  STB_LIBRARIES

include(FindPackageHandleStandardArgs)

set(STB_ROOT_DIR ${THIS_THIRDPARTY_PATH}/stb)

if(WIN32)
  find_path(STB_INCLUDE_DIR stb.h PATHS ${STB_ROOT_DIR})
elseif(ANDROID OR IOS)
  # because android.toolchain.cmake set CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY
  # so need set NO_CMAKE_FIND_ROOT_PATH
  find_path(STB_INCLUDE_DIR stb.h HINTS ${STB_ROOT_DIR} NO_CMAKE_FIND_ROOT_PATH)
else()
  find_path(STB_INCLUDE_DIR stb.h PATHS ${STB_ROOT_DIR})
endif()

find_package_handle_standard_args(STB DEFAULT_MSG STB_INCLUDE_DIR )

if(STB_FOUND)
  set(STB_INCLUDE_DIRS ${STB_INCLUDE_DIR})
endif()

message("STB_FOUND= ${STB_FOUND}")

