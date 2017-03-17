# Find Google Logging

find_path(GLOG_INCLUDE_PATH NAMES glog/logging.h)
find_library(GLOG_LIBRARY NAMES glog)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glog
    REQUIRED_VARS   GLOG_INCLUDE_PATH GLOG_LIBRARY
)
mark_as_advanced(GLOG_INCLUDE_PATH GLOG_LIBRARY)

if(GLOG_FOUND)
    if(NOT GLOG_FIND_QUIETLY)
        message(STATUS "Found GLOG: ${GLOG_LIBRARY}; includes - ${GLOG_INCLUDE_PATH}")
    endif(NOT GLOG_FIND_QUIETLY)
else(GLOG_FOUND)
    if(GLOG_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find GLOG library.")
    endif(GLOG_FIND_REQUIRED)
endif(GLOG_FOUND)