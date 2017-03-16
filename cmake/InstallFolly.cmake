ExternalProject_Add(folly
    DOWNLOAD_NAME       folly-${FOLLY_VERSION}.tar.gz
    URL                 https://github.com/facebook/folly/archive/v${FOLLY_VERSION}.tar.gz
    URL_MD5             ${FOLLY_URL_MD5}
    CONFIGURE_COMMAND   cd <SOURCE_DIR>/folly &&
                        autoreconf -vi &&
                        CXXFLAGS=-I<INSTALL_DIR>/include
                        LDFLAGS=-L<INSTALL_DIR>/lib
                        PKG_CONFIG_PATH=<INSTALL_DIR>/lib/pkgconfig
                        GFLAGS_CFLAGS=-I<INSTALL_DIR>/include
                        GFLAGS_LIBS=<INSTALL_DIR>/lib/libgflags.a
                        <SOURCE_DIR>/folly/configure
                        --prefix=<INSTALL_DIR> ${WITH_OPENSSL}
                        --with-boost=${BOOST_ROOT}
    BUILD_COMMAND       cd <SOURCE_DIR>/folly && make
    INSTALL_COMMAND     cd <SOURCE_DIR>/folly && make install
)

ExternalProject_Get_Property(folly INSTALL_DIR)

set (FOLLY_ROOT_DIR         ${INSTALL_DIR})
set (FOLLY_STATIC_LIBRARY   "${FOLLY_ROOT_DIR}/lib/libfolly.a")
set (FOLLY_LIBRARIES        ${FOLLY_STATIC_LIBRARY})
set (FOLLY_FOUND            YES)

find_path(FOLLY_INCLUDE_DIR "folly/String.h"
    PATHS       ${FOLLY_ROOT_DIR}/include)

add_library(FOLLY_STATIC_LIBRARY STATIC IMPORTED)
add_dependencies(FOLLY_STATIC_LIBRARY folly)
mark_as_advanced(FOLLY_LIBRARIES FOLLY_STATIC_LIBRARY FOLLY_INCLUDE_DIR)