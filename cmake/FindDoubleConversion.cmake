# Finds libdouble-conversion.
#
# This module defines:
# DOUBLE_CONVERSION_INCLUDE_DIR
# DOUBLE_CONVERSION_LIBRARY
#

find_path(DOUBLE_CONVERSION_INCLUDE_DIR
    NAMES
      double-conversion.h
    PATHS
      /usr/include/double-conversion
      /usr/local/include/double-conversion)
find_library(DOUBLE_CONVERSION_LIBRARY NAMES double-conversion)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(DOUBLE_CONVERSION
    REQUIRED_VARS   DOUBLE_CONVERSION_LIBRARY DOUBLE_CONVERSION_INCLUDE_DIR
)

mark_as_advanced(DOUBLE_CONVERSION_INCLUDE_DIR DOUBLE_CONVERSION_LIBRARY)