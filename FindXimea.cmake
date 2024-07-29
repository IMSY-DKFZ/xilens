# FindXimea.cmake
# -----------------
#
# Find the Ximea library
# This module defines:
# Ximea_FOUND, whether the library was found or not
# Ximea_INCLUDE_DIRS, where to find the headers
# Ximea_LIBRARIES, the libraries to link against to use Ximea

FIND_PATH(Ximea_INCLUDE_DIR NAMES xiApi.h
          PATHS
          /opt/XIMEA/include
)

FIND_LIBRARY(Ximea_LIBRARY NAMES m3api
             PATHS
             /usr/lib
)

SET(Ximea_LIBRARIES ${Ximea_LIBRARY})
SET(Ximea_INCLUDE_DIRS ${Ximea_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ximea DEFAULT_MSG Ximea_INCLUDE_DIRS Ximea_LIBRARIES)

MARK_AS_ADVANCED(Ximea_INCLUDE_DIRS Ximea_LIBRARIES)
