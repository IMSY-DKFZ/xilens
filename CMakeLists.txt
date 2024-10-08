#-----------------------------------------------------------------------------------------------------------------------
# Author: Intelligent Medical Systems
# License: see LICENSE.md file
#-----------------------------------------------------------------------------------------------------------------------
# CMake project configuration
#-----------------------------------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)
project(xilens VERSION 0.2.1)

#-----------------------------------------------------------------------------------------------------------------------
# Language definitions
#-----------------------------------------------------------------------------------------------------------------------

set (CMAKE_CXX_STANDARD 11)

#-----------------------------------------------------------------------------------------------------------------------
# Package variable definitions
#-----------------------------------------------------------------------------------------------------------------------
# build date definitions
string(TIMESTAMP CURRENT_DATETIME "%Y-%m-%d %H:%M:%S")
add_definitions("-DBUILD_TIMESTAMP=\"${CURRENT_DATETIME}\"")
# compiler definitions
add_definitions("-DCMAKE_CXX_COMPILER=\"${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\"")
# version definitions
add_definitions("-DPROJECT_VERSION_MAJOR=\"${PROJECT_VERSION_MAJOR}\"")
add_definitions("-DPROJECT_VERSION_MINOR=\"${PROJECT_VERSION_MINOR}\"")
add_definitions("-DPROJECT_VERSION_PATCH=\"${PROJECT_VERSION_PATCH}\"")
# git definitions
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DGIT_COMMIT=\"${GIT_COMMIT}\"")
# system definitions
add_definitions("-DCMAKE_SYSTEM=\"${CMAKE_SYSTEM}\"")
add_definitions("-DCMAKE_SYSTEM_PROCESSOR=\"${CMAKE_SYSTEM_PROCESSOR}\"")

#-----------------------------------------------------------------------------------------------------------------------
# Build type
#-----------------------------------------------------------------------------------------------------------------------
option(BUILD_STATIC "Build xilens as a static app" OFF)
if(BUILD_STATIC)
    set(LIBRARY_TYPE STATIC)
else()
    set(LIBRARY_TYPE SHARED)
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Test coverage fiddling
#-----------------------------------------------------------------------------------------------------------------------
option(ENABLE_COVERAGE "Enable coverage" OFF)
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Find external packages
#-----------------------------------------------------------------------------------------------------------------------
# Boost
find_package(Boost REQUIRED system thread timer chrono log filesystem)
include_directories(${Boost_INCLUDE_DIRS})
# BLOSC2
find_package(Blosc2 2.15.1 CONFIG REQUIRED)
# Message pack for metadata serialization
find_package(msgpack REQUIRED)
# Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Svg)
# OpenCV
set(OpenCV_STATIC ON)
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)
include_directories(${OpenCV_INCLUDE_DIRS})
# XIMEA API
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(Ximea REQUIRED)
include_directories(${Ximea_INCLUDE_DIRS})

#-----------------------------------------------------------------------------------------------------------------------
# XiLens library build
#-----------------------------------------------------------------------------------------------------------------------
set(XILENS_LIB_SRC src/mainwindow.cpp
        src/cameraInterface.cpp
        src/display.cpp
        src/displayFunctional.cpp
        src/imageContainer.cpp
        src/util.cpp
        src/camera.cpp
        src/logger.cpp
        src/constants.cpp
        src/widgets.cpp
)
set(XILENS_LIB_HDR src/mainwindow.h
        src/cameraInterface.h
        src/display.h
        src/displayFunctional.h
        src/imageContainer.h
        src/util.h
        src/camera.h
        src/logger.h
        src/xiAPIWrapper.h
        src/constants.h
        src/widgets.h
)
set(XILENS_LIB_UI src/mainwindow.ui)

qt6_wrap_cpp(XILENS_LIB_HDR_MOC ${XILENS_LIB_HDR})
qt6_wrap_ui(XILENS_LIB_UI_MOC ${XILENS_LIB_UI})

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_BINARY_DIR})

add_library(XILENS_LIB ${LIBTYPE}
        ${XILENS_LIB_SRC}
        ${XILENS_LIB_HDR}
        ${XILENS_LIB_HDR_MOC}
        ${XILENS_LIB_UI_MOC}
)
target_link_libraries(XILENS_LIB Qt6::Widgets Qt6::Svg)
target_link_libraries(XILENS_LIB ${OpenCV_LIBS})
target_link_libraries(XILENS_LIB ${Boost_LIBRARIES})
target_link_libraries(XILENS_LIB ${Ximea_LIBRARIES})
target_link_libraries(XILENS_LIB Blosc2::blosc2_shared)

#-----------------------------------------------------------------------------------------------------------------------
# XiLens executable
#-----------------------------------------------------------------------------------------------------------------------
set(XILENS_BIN_SRC src/CLI.cpp src/CLI11.h)
qt6_add_resources(XILENS_BIN_SRC resources/resources.qrc)
qt6_add_resources(XILENS_BIN_SRC resources/theme_resources.qrc)
add_executable(xilens ${XILENS_BIN_SRC})
target_link_libraries(xilens XILENS_LIB)
file(COPY resources/XiLensCameraProperties.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#-----------------------------------------------------------------------------------------------------------------------
# Install and uninstall
#-----------------------------------------------------------------------------------------------------------------------
# Setup the installation target
install(TARGETS xilens XILENS_LIB
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(FILES ${CMAKE_SOURCE_DIR}/resources/XiLensCameraProperties.json  DESTINATION /etc/xilens)
install(FILES ${CMAKE_SOURCE_DIR}/resources/icon.png DESTINATION share/pixmaps RENAME xilens.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/xilens.desktop DESTINATION share/applications)

# uninstall configuration
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

#-----------------------------------------------------------------------------------------------------------------------
# Google Tests
#-----------------------------------------------------------------------------------------------------------------------
find_package(GTest REQUIRED)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
enable_testing()
add_executable(
        XILENS_TESTS
        tests/main.cpp
        tests/mocks.h
        tests/utilTest.cpp
        tests/cameraInterfaceTest.cpp
        tests/displayersTest.cpp
        tests/imageContainerTest.cpp
        tests/bloscTest.cpp
        tests/mainWindowTest.cpp
        tests/constantsTest.cpp
        ${XILENS_LIB_UI_MOC}
)
target_link_libraries(XILENS_TESTS GTest::gtest_main XILENS_LIB Blosc2::blosc2_shared)
include(GoogleTest)
gtest_discover_tests(XILENS_TESTS)

#-----------------------------------------------------------------------------------------------------------------------
# Packaging
#-----------------------------------------------------------------------------------------------------------------------
include(InstallRequiredSystemLibraries)
file(STRINGS user-requirements.txt deps)
string(REPLACE ";" ", " deps "${deps}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${deps}")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Leonardo Ayala")
set(CPACK_PACKAGE_DESCRIPTION "GUI application used to record data with XIMEA cameras")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/IMSY-DKFZ/xilens/issues")
include(CPack)
