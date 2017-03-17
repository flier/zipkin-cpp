if (NOT DOUBLE_CONVERSION_FOUND OR USE_BUNDLED_DOUBLE_CONVERSION)
    if (NOT DOUBLE_CONVERSION_VERSION OR USE_BUNDLED_DOUBLE_CONVERSION)
        set (DOUBLE_CONVERSION_VERSION      1.1.5)
        set (DOUBLE_CONVERSION_URL_MD5      f7c62594d7ecfbc4421da32bc341a919)
    endif ()

    ExternalProject_Add(DoubleConversion
        DOWNLOAD_NAME   double_conversion-${DOUBLE_CONVERSION_VERSION}.tar.gz
        URL             https://github.com/google/double-conversion/archive/v${DOUBLE_CONVERSION_VERSION}.tar.gz
        URL_MD5         ${DOUBLE_CONVERSION_URL_MD5}
        CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        TEST_COMMAND    ""
    )

    ExternalProject_Get_Property(DoubleConversion INSTALL_DIR)

    set (DOUBLE_CONVERSION_ROOT_DIR     ${INSTALL_DIR})
    set (DOUBLE_CONVERSION_INCLUDE_DIR  ${DOUBLE_CONVERSION_ROOT_DIR}/include)
    set (DOUBLE_CONVERSION_LIBRARY_DIR  ${DOUBLE_CONVERSION_ROOT_DIR}/lib)
    set (DOUBLE_CONVERSION_LIBRARY      ${DOUBLE_CONVERSION_LIBRARY_DIR}/libdouble-conversion.a)
    set (DOUBLE_CONVERSION_FOUND        YES)

    add_library(DOUBLE_CONVERSION_LIBRARY STATIC IMPORTED)
    add_dependencies(DOUBLE_CONVERSION_LIBRARY DoubleConversion)
    mark_as_advanced(DOUBLE_CONVERSION_LIBRARY DOUBLE_CONVERSION_INCLUDE_DIR)

    message(STATUS "Use bundled double_conversion v${DOUBLE_CONVERSION_VERSION}")
endif ()