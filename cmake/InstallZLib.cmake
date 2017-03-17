if (NOT ZLIB_FOUND OR USE_BUNDLED_ZLIB)
    if (NOT ZLIB_VERSION_STRING OR USE_BUNDLED_ZLIB)
        set (ZLIB_VERSION_STRING        1.2.11)
        set (ZLIB_URL_MD5               1c9f62f0778697a09d36121ead88e08e)
    endif ()

    ExternalProject_Add(ZLib
        URL                 http://www.zlib.net/zlib-${ZLIB_VERSION_STRING}.tar.gz
        URL_MD5             ${ZLIB_URL_MD5}
        CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND       make
        INSTALL_COMMAND     make install
        TEST_COMMAND        ""
    )

    ExternalProject_Get_Property(ZLib INSTALL_DIR)

    set (ZLIB_ROOT_DIR          ${INSTALL_DIR})
    set (ZLIB_INCLUDE_DIRS      ${ZLIB_ROOT_DIR}/include)
    set (ZLIB_LIBRARY_DIRS      ${ZLIB_ROOT_DIR}/lib)
    set (ZLIB_LIBRARIES         ${ZLIB_LIBRARY_DIRS}/libz.a)
    set (ZLIB_FOUND             YES)

    mark_as_advanced(ZLIB_LIBRARIES ZLIB_INCLUDE_DIRS)

    message(STATUS "Use bundled zlib v${ZLIB_VERSION_STRING}")
endif ()