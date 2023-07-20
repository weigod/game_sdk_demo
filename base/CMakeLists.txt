cmake_minimum_required(VERSION 3.6.3 FATAL_ERROR)
cmake_policy(VERSION 3.6.3)
set(EXECUTABLE_NAME base)

set(EXECUTABLE_SRCS
  base64.h
  base64.cc
  EnvUtil.h
  EnvUtil.cpp
  DxTextureAdapter.h
  DxTextureAdapter.cpp
  PixelFormatDefs.h
  scoped_refptr.h
  string_utils.h
  string_utils.cc
)

include_directories(
  PRIVATE ${CMAKE_SOURCE_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(${EXECUTABLE_NAME}
  ${EXECUTABLE_SRCS}
)

target_link_libraries(${EXECUTABLE_NAME}

)

if(WIN32)
 target_link_libraries(${EXECUTABLE_NAME}
   advapi32
   iphlpapi
   shell32
   Winmm
 )

elseif(APPLE)

  target_link_libraries(${EXECUTABLE_NAME}
    m 
    dl
  )

else()

  target_link_libraries(${EXECUTABLE_NAME}
    m 
    dl
    stdc++fs
    pthread
    rt
    dl
  )

endif()

if(APPLE)
  set(EXECUTABLE_LINK_FLAGS "-framework Carbon ${EXECUTABLE_LINK_FLAGS}")
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS
  "${EXECUTABLE_LINK_FLAGS}"
)

INSTALL(TARGETS ${EXECUTABLE_NAME}
  RUNTIME DESTINATION bin
)

set(RES_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

SET_COMMON_TARGET_PROPERTIES(${EXECUTABLE_NAME})
unset(EXECUTABLE_NAME)
