# - Try to find libzipkin headers and libraries.
#
# Usage of this module as follows:
#
#     find_package(Zipkin)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  LIBZIPKIN_ROOT_DIR  Set this variable to the root installation of
#                      libzipkin if the module has problems finding
#                      the proper installation path.
#
# Variables defined by this module:
#
#  LIBZIPKIN_FOUND              System has libzipkin libs/headers
#  LIBZIPKIN_LIBRARIES          The libzipkin libraries
#  LIBZIPKIN_INCLUDE_DIR        The location of libzipkin headers
#
# Initial work was done by Flier Lu https://github.com/flier/zipkin-cpp

find_path(LIBZIPKIN_ROOT_DIR
    NAMES
        include/zipkin/zipkin.h
    HINTS
        ${ZIPKIN_HOME}
        ENV ZIPKIN_HOME
        /usr/local
        /opt/local
)

find_library(LIBZIPKIN_LIBRARIES
    NAMES
        zipkin libzipkin
    HINTS
        ${LIBZIPKIN_ROOT_DIR}/lib
    PATH_SUFFIXES
        lib lib64
)

find_path(LIBZIPKIN_INCLUDE_DIR
    NAMES
        zipkin/zipkin.h
    HINTS
        ${LIBZIPKIN_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBZIPKIN DEFAULT_MSG
    LIBZIPKIN_LIBRARIES
    LIBZIPKIN_INCLUDE_DIR
)

mark_as_advanced(
    LIBZIPKIN_ROOT_DIR
    LIBZIPKIN_LIBRARIES
    LIBZIPKIN_INCLUDE_DIR
)
