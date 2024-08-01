cmake_minimum_required(VERSION 3.15)

project(Skeleton VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get the absolute path to the Adobe After Effects SDK
get_filename_component(AE_SDK_PATH "../../" ABSOLUTE)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "C:/Program Files/Adobe/Adobe After Effects 2023/Support Files/Plug-ins/Effects")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${AE_PLUGIN_BUILD_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")

# Specify source files
file(GLOB_RECURSE SOURCE_FILES 
  ${AE_SDK_PATH}/Util/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Skeleton_Strings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Skeleton.cpp
)

set(AESDK_INCLUDE
  ${AE_SDK_PATH}/Headers
  ${AE_SDK_PATH}/Headers/SP
  ${AE_SDK_PATH}/Util
  ${AE_SDK_PATH}/Resources
)

include_directories(
  ${AESDK_INCLUDE}
  ${CMAKE_CURRENT_SOURCE_DIR}
  "C:/Users/tjerf/vcpkg/installed/x64-windows-static/include"
  "C:/Users/tjerf/vcpkg/installed/x64-windows-static/include/python3.11"
  "C:/Users/tjerf/source/repos/PyFxCore"
)

# Specify the plugin target
add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})

# Enable verbose output
set(CMAKE_VERBOSE_MAKEFILE ON)

# For Visual Studio project generation
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -DMSWindows
    -DWIN32
    -D_WINDOWS
  )

  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".aex")

  # Custom commands for resource compilation
  if(EXISTS ${AE_SDK_PATH}/Resources/PiPLtool.exe)
    get_filename_component(
      AFX_REZ
      ${AE_SDK_PATH}/Resources/PiPLtool.exe
      ABSOLUTE
    )

    # Compile the resource format from SkeletonPiPL.r
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
      COMMAND cl
        /I ${AE_SDK_PATH}/Headers
        /I ${AE_SDK_PATH}/Headers/SP
        /I ${AE_SDK_PATH}/Util
        /I ${AE_SDK_PATH}/Resources
        /EP ${CMAKE_CURRENT_SOURCE_DIR}/SkeletonPiPL.r > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
    )

    add_custom_command(
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc
      COMMAND ${AFX_REZ} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc
    )

    add_custom_command(
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc
      COMMAND cl /D "MSWindows" /EP ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc
    )

    target_sources(
      ${PROJECT_NAME} PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc
    )
  else()
    message(FATAL_ERROR "PiPLtool.exe not found. Ensure the Adobe After Effects SDK is correctly set up.")
  endif()
endif()

# Compiler and linker options
target_compile_definitions(${PROJECT_NAME} PRIVATE MSWindows WIN32 _WINDOWS)
target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX- /Od /D "_DEBUG" /MDd /Zi /RTC1 /Gd /MP)
target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17)

# Linker options
target_link_options(${PROJECT_NAME} PRIVATE /DEBUG /SUBSYSTEM:WINDOWS /MACHINE:X64)
target_link_libraries(${PROJECT_NAME} PRIVATE 
  "C:/Users/tjerf/vcpkg/installed/x64-windows-static/debug/lib/python311_d.lib"
  "C:/Users/tjerf/vcpkg/installed/x64-windows-static/debug/lib"
  "C:/Users/tjerf/source/repos/PyFxCore/x64/Debug/PyFxCore.lib"
)
