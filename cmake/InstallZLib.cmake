ExternalProject_Add(zlib
    DOWNLOAD_NAME       zlib-${ZLIB_VERSION_STRING}.tar.gz
    URL                 http://www.zlib.net/zlib-${ZLIB_VERSION_STRING}.tar.gz
    URL_MD5             ${ZLIB_URL_MD5}
    CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND       make
    INSTALL_COMMAND     make install
)

ExternalProject_Get_Property(zlib INSTALL_DIR)

set (ZLIB_ROOT_DIR          ${INSTALL_DIR})
set (ZLIB_INCLUDE_DIRS      ${ZLIB_ROOT_DIR}/include)
set (ZLIB_LIBRARY_DIRS      ${ZLIB_ROOT_DIR}/lib)
set (ZLIB_LIBRARIES         ${ZLIB_LIBRARY_DIRS}/libz.a)
set (ZLIB_FOUND             YES)

mark_as_advanced(ZLIB_LIBRARIES ZLIB_INCLUDE_DIRS)