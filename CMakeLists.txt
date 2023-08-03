cmake_minimum_required (VERSION 3.1)
project (susiCam)

#################################
##### compiler fiddling     #####
#################################


#set (CMAKE_CXX_STANDARD 11)

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
    include_directories (${Boost_INCLUDE_DIRS})
ELSE()
    # boost
    find_package(Boost REQUIRED system thread timer chrono log filesystem)
    include_directories (${Boost_INCLUDE_DIRS})
ENDIF()

IF (WIN32)
  # if on windows: enter the path to your qt here.
  set (CMAKE_PREFIX_PATH "C:\\Qt\\5.6\\msvc2015_64\\")

  set(Boost_USE_STATIC_LIBS        ON)
  set(Boost_USE_MULTITHREADED      ON)
  set(Boost_USE_STATIC_RUNTIME    OFF)

  # for some reason windows doesn't find the shared libaries
  set(LIBTYPE STATIC)

  # because of boost asio
  add_definitions(-DWIN32_LEAN_AND_MEAN)
ELSE()
  set(LIBTYPE SHARED)
  # apparently, the boost logging module needs the following.
  # looks a little weird to me, shouldn't it be static?
  # maybe, boost needs to be setup a little differently
  add_definitions(-DBOOST_LOG_DYN_LINK)
ENDIF()

# qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets)
# opencv
set(OpenCV_STATIC ON)
find_package( OpenCV REQUIRED COMPONENTS core imgproc highgui)
include_directories(${OpenCV_INCLUDE_DIRS})
# boost
find_package(Boost REQUIRED python system thread timer chrono log filesystem)
include_directories (${Boost_INCLUDE_DIRS})
# ximea (camera)
find_library(Ximea_Lib libm3api DOC "Ximeas Xispec library." HINTS "/usr/lib/libm3api.so.0")
find_path(Ximea_Include_Dir xiApi.h DOC  "XiApi include directory"  HINTS "/opt/XIMEA/include/xiApi.h")
include_directories (${Ximea_Include_Dir})

######################
##### susiCamLib #####
######################
include(version.cmake)

set (susiCamLib_src ${PROJECT_SOURCE_DIR}/mainwindow.cpp
        ${PROJECT_SOURCE_DIR}/version.cpp
        ${PROJECT_SOURCE_DIR}/camera_interface.cpp
        ${PROJECT_SOURCE_DIR}/display.cpp
        ${PROJECT_SOURCE_DIR}/displayRaw.cpp
        ${PROJECT_SOURCE_DIR}/displayCaffe.cpp
        ${PROJECT_SOURCE_DIR}/displayDemo.cpp
        ${PROJECT_SOURCE_DIR}/image_container.cpp
        ${PROJECT_SOURCE_DIR}/util.cpp
        ${PROJECT_SOURCE_DIR}/resources.qrc
)
set (susiCamLib_hdr ${PROJECT_SOURCE_DIR}/mainwindow.h
        ${PROJECT_SOURCE_DIR}/camera_interface.h
        ${PROJECT_SOURCE_DIR}/display.h
        ${PROJECT_SOURCE_DIR}/displayRaw.h
        ${PROJECT_SOURCE_DIR}/displayCaffe.h
        ${PROJECT_SOURCE_DIR}/displayDemo.h
        ${PROJECT_SOURCE_DIR}/image_container.h
        ${PROJECT_SOURCE_DIR}/util.h
        ${PROJECT_SOURCE_DIR}/default_defines.h)
set (susiCamLib_ui  ${PROJECT_SOURCE_DIR}/mainwindow.ui)

qt5_wrap_cpp(susiCamLib_hdr_moc ${susiCamLib_hdr})
qt5_wrap_ui (susiCamLib_ui_moc  ${susiCamLib_ui})

include_directories (${PROJECT_SOURCE_DIR})
include_directories (${PROJECT_BINARY_DIR})

add_library (susiCamLib ${LIBTYPE}
    ${susiCamLib_src}
    ${susiCamLib_hdr}
    ${susiCamLib_hdr_moc}
    ${susiCamLib_ui_moc}
)
target_link_libraries(susiCamLib Qt5::Widgets)
target_link_libraries(susiCamLib ${OpenCV_LIBS} )
target_link_libraries(susiCamLib ${Boost_LIBRARIES})
target_link_libraries(susiCamLib ${Ximea_Lib})

###################
##### susiCam #####
###################

set (susiCamBin_src ${PROJECT_SOURCE_DIR}/mainSusiCam.cpp)
add_executable(susiCam ${susiCamBin_src})
target_link_libraries(susiCam susiCamLib)
