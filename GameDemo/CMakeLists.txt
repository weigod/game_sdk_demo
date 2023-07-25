cmake_minimum_required(VERSION 3.6.3 FATAL_ERROR)
cmake_policy(VERSION 3.6.3)
find_package(ControllerSDK REQUIRED)
find_package(SpdLog REQUIRED)
find_package(Stb REQUIRED)
find_package(LibYUV REQUIRED)
set(EXECUTABLE_NAME GameDemo)

set(EXECUTABLE_SRCS
  main.cpp
  Application.h
  Application.cpp
)

include_directories(
  PRIVATE ${CMAKE_SOURCE_DIR}
  PRIVATE ${CMAKE_SOURCE_DIR}/base
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  # for audio/pixel format
  PRIVATE ${ControllerSDK_INCLUDE_DIRS}
  PRIVATE ${SPDLOG_INCLUDE_DIRS}
  PRIVATE ${STB_INCLUDE_DIRS}
  PRIVATE ${LIBYUV_INCLUDE_DIRS}
)

add_executable(${EXECUTABLE_NAME}
  ${EXECUTABLE_SRCS}
)

target_link_libraries(${EXECUTABLE_NAME}
  base
  ${ControllerSDK_LIBRARIES}
  ${LIBYUV_LIBRARIES}
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

add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD 
    COMMAND xcopy /Y /E
    "\"${ControllerSDK_BINS}\\*.*\""
    "\"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\\""
)

SET_COMMON_TARGET_PROPERTIES(${EXECUTABLE_NAME})
unset(EXECUTABLE_NAME)
