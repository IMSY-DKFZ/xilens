cmake_minimum_required(VERSION 3.5)
project(SUSICAM VERSION 0.2.1)

#################################
##### compiler fiddling     #####
#################################

set (CMAKE_CXX_STANDARD 11)

#################################
##### definitions           #####
#################################

# Add definitions for project build details
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

#################################
##### coverage fiddling     #####
#################################
option(ENABLE_COVERAGE "Enable coverage" OFF)
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

#################################
##### set external packages #####
#################################

# platform specific settings

cmake_host_system_information(RESULT UBUNTU_VERSION QUERY OS_VERSION)
message("Found OS_VERSION: ${UBUNTU_VERSION} in Ubuntu")

IF (UBUNTU_VERSION MATCHES "20.04")
    # needed when using ubuntu 20
    message([STATUS] "UBUNTU version found ${UBUNTU_VERSION}")
    set(CMAKE_CXX_FLAGS "-luuid")
    # boost
    find_package(Boost REQUIRED python system thread timer chrono log filesystem)
    include_directories(${Boost_INCLUDE_DIRS})
ELSE ()
    # boost
    find_package(Boost REQUIRED system thread timer chrono log filesystem)
    include_directories(${Boost_INCLUDE_DIRS})
ENDIF ()

IF (WIN32)
    # if on windows: enter the path to your qt here.
    set(CMAKE_PREFIX_PATH "C:\\Qt\\5.6\\msvc2015_64\\")

    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    # for some reason windows doesn't find the shared libaries
    set(LIBTYPE STATIC)

    # because of boost asio
    add_definitions(-DWIN32_LEAN_AND_MEAN)
ELSE ()
    set(LIBTYPE SHARED)
    # apparently, the boost logging module needs the following.
    # looks a little weird to me, shouldn't it be static?
    # maybe, boost needs to be setup a little differently
    add_definitions(-DBOOST_LOG_DYN_LINK)
ENDIF ()

# BLOSC2
find_package(Blosc2 2.15.0 CONFIG REQUIRED)

# Message pack for metadata serialization
find_package(msgpack REQUIRED)

# qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Svg)

# opencv
set(OpenCV_STATIC ON)
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)
include_directories(${OpenCV_INCLUDE_DIRS})
# boost
find_package(Boost REQUIRED python system thread timer chrono log filesystem)
include_directories(${Boost_INCLUDE_DIRS})
# ximea (camera)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(Ximea REQUIRED)
include_directories(${Ximea_INCLUDE_DIRS})

######################
##### susiCamLib #####
######################
set(SUSICAM_LIB_SRC src/mainwindow.cpp
        src/cameraInterface.cpp
        src/display.cpp
        src/displayFunctional.cpp
        src/imageContainer.cpp
        src/util.cpp
        src/camera.cpp
        src/logger.cpp
        src/constants.cpp
)
set(SUSICAM_LIB_HDR src/mainwindow.h
        src/cameraInterface.h
        src/display.h
        src/displayFunctional.h
        src/imageContainer.h
        src/util.h
        src/camera.h
        src/logger.h
        src/xiAPIWrapper.h
        src/constants.h
)
set(SUSICAM_LIB_UI src/mainwindow.ui)

qt6_wrap_cpp(SUSICAM_LIB_HDR_MOC ${SUSICAM_LIB_HDR})
qt6_wrap_ui(SUSICAM_LIB_UI_MOC ${SUSICAM_LIB_UI})

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

add_library(SUSICAM_LIB ${LIBTYPE}
        ${SUSICAM_LIB_SRC}
        ${SUSICAM_LIB_HDR}
        ${SUSICAM_LIB_HDR_MOC}
        ${SUSICAM_LIB_UI_MOC}
)
target_link_libraries(SUSICAM_LIB Qt6::Widgets Qt6::Svg)
target_link_libraries(SUSICAM_LIB ${OpenCV_LIBS})
target_link_libraries(SUSICAM_LIB ${Boost_LIBRARIES})
target_link_libraries(SUSICAM_LIB ${Ximea_LIBRARIES})
target_link_libraries(SUSICAM_LIB Blosc2::blosc2_shared)

###################
##### susiCam #####
###################
set(SUSICAM_BIN_SRC src/CLI.cpp src/CLI11.h)
qt6_add_resources(SUSICAM_BIN_SRC resources/resources.qrc)
qt6_add_resources(SUSICAM_BIN_SRC resources/theme_resources.qrc)
add_executable(SUSICAM ${SUSICAM_BIN_SRC})
target_link_libraries(SUSICAM SUSICAM_LIB)
file(COPY resources/XiLensCameraProperties.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#################################
##### Install and uninstall #####
#################################
# Setup the installation target
install(TARGETS SUSICAM SUSICAM_LIB
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(FILES ${CMAKE_SOURCE_DIR}/resources/XiLensCameraProperties.json  DESTINATION /etc/susicam)
install(FILES ${CMAKE_SOURCE_DIR}/resources/icon.png DESTINATION share/pixmaps RENAME susicam.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/susicam.desktop DESTINATION share/applications)

# uninstall configuration
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

#####################
### Google Tests ####
#####################
find_package(GTest REQUIRED)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

enable_testing()

add_executable(
        SUSICAM_TESTS
        tests/main.cpp
        tests/mocks.h
        tests/utilTest.cpp
        tests/cameraInterfaceTest.cpp
        tests/displayersTest.cpp
        tests/imageContainerTest.cpp
        tests/bloscTest.cpp
        tests/mainWindowTest.cpp
        tests/constantsTest.cpp
)
target_link_libraries(SUSICAM_TESTS GTest::gtest_main SUSICAM_LIB Blosc2::blosc2_shared)

include(GoogleTest)
gtest_discover_tests(SUSICAM_TESTS)

#####################
### DEB package  ####
#####################
include(InstallRequiredSystemLibraries)
file(STRINGS requirements.txt deps)
string(REPLACE ";" ", " deps "${deps}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${deps}")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Leonardo Ayala")
set(CPACK_PACKAGE_DESCRIPTION "GUI application used to record data with XIMEA cameras")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://git.dkfz.de/imsy/issi/susicam")
include(CPack)
