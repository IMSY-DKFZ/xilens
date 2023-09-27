cmake_minimum_required(VERSION 3.5)
project(susiCam)

#################################
##### compiler fiddling     #####
#################################

set (CMAKE_CXX_STANDARD 11)

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

# qt
find_package(Qt6Core REQUIRED)
find_package(Qt6Widgets)
# opencv
set(OpenCV_STATIC ON)
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)
include_directories(${OpenCV_INCLUDE_DIRS})
# boost
find_package(Boost REQUIRED python system thread timer chrono log filesystem)
include_directories(${Boost_INCLUDE_DIRS})
# ximea (camera)
find_library(Ximea_Lib libm3api DOC "Ximeas Xispec library." HINTS "/usr/lib/libm3api.so.0")
find_path(Ximea_Include_Dir xiApi.h DOC "XiApi include directory" HINTS "/opt/XIMEA/include/xiApi.h")
include_directories(${Ximea_Include_Dir})

######################
##### susiCamLib #####
######################
include(src/version.cmake)

set(susiCamLib_src src/mainwindow.cpp
        src/version.cpp
        src/camera_interface.cpp
        src/display.cpp
        src/displayRaw.cpp
        src/displayFunctional.cpp
        src/image_container.cpp
        src/util.cpp
)
set(susiCamLib_hdr src/mainwindow.h
        src/camera_interface.h
        src/display.h
        src/displayRaw.h
        src/displayFunctional.h
        src/image_container.h
        src/util.h
)
set(susiCamLib_ui src/mainwindow.ui)

qt6_wrap_cpp(susiCamLib_hdr_moc ${susiCamLib_hdr})
qt6_wrap_ui(susiCamLib_ui_moc ${susiCamLib_ui})

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

add_library(susiCamLib ${LIBTYPE}
        ${susiCamLib_src}
        ${susiCamLib_hdr}
        ${susiCamLib_hdr_moc}
        ${susiCamLib_ui_moc}
)
target_link_libraries(susiCamLib Qt6::Widgets)
target_link_libraries(susiCamLib ${OpenCV_LIBS})
target_link_libraries(susiCamLib ${Boost_LIBRARIES})
target_link_libraries(susiCamLib ${Ximea_Lib})

###################
##### susiCam #####
###################

set(susiCamBin_src src/mainSusiCam.cpp)
qt6_add_resources(susiCamBin_src resources/resources.qrc)
add_executable(susiCam ${susiCamBin_src})
target_link_libraries(susiCam susiCamLib)

####################
####Google Tests####
####################
#include(FetchContent)
#FetchContent_Declare(
#        googletest
#        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
#        DOWNLOAD_EXTRACT_TIMESTAMP true
#)
## For Windows: Prevent overriding the parent project's compiler/linker settings
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#FetchContent_MakeAvailable(googletest)
#
#enable_testing()
#
#add_executable(
#        util_tests
#        tests/util_test.cpp
#)
#target_link_libraries(
#        util_tests
#        GTest::gtest_main
#)
#
#include(GoogleTest)
#gtest_discover_tests(util_tests)
