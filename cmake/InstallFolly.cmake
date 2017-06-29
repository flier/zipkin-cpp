if (NOT FOLLY_FOUND OR USE_BUNDLED_FOLLY)
    if (NOT FOLLY_VERSION OR USE_BUNDLED_FOLLY)
        set (FOLLY_VERSION              2017.06.26.01)
        set (FOLLY_URL_MD5              cf7a05081adb16913b5d7039ac62d46b)
    endif()

    ExternalProject_Add(Folly
        DOWNLOAD_NAME       Folly-${FOLLY_VERSION}.tar.gz
        URL                 https://github.com/facebook/folly/archive/v${FOLLY_VERSION}.tar.gz
        URL_MD5             ${FOLLY_URL_MD5}
        CONFIGURE_COMMAND   cd <SOURCE_DIR>/folly &&
                            autoreconf -vi &&
                            <SOURCE_DIR>/folly/configure
                            --prefix=<INSTALL_DIR>
                            --with-pic
                            --with-jemalloc
                            ${WITH_OPENSSL}
                            LD_LIBRARY_PATH=<INSTALL_DIR>/lib
                            LD_RUN_PATH=<INSTALL_DIR>/lib
                            LIBRARY_PATH=<INSTALL_DIR>/lib
                            LDFLAGS=-L<INSTALL_DIR>/lib
                            PKG_CONFIG_PATH=<INSTALL_DIR>/lib/pkgconfig
                            CFLAGS=-I<INSTALL_DIR>/include
                            CXXFLAGS=-I<INSTALL_DIR>/include
                            CPPFLAGS=-I<INSTALL_DIR>/include
                            LIBS=${CMAKE_THREAD_LIBS_INIT}
                            OPENSSL_CFLAGS=-I${OPENSSL_INCLUDE_DIR}
                            OPENSSL_LIBS=-L${OPENSSL_LIBRARY_DIR}
        BUILD_COMMAND       cd <SOURCE_DIR>/folly &&
                            make
        INSTALL_COMMAND     cd <SOURCE_DIR>/folly &&
                            make install
        TEST_COMMAND        ""
    )

    if (TARGET GFlags)
        add_dependencies(Folly GFlags)
    endif ()

    if (TARGET GLog)
        add_dependencies(Folly GLog)
    endif ()

    ExternalProject_Get_Property(Folly INSTALL_DIR)

    set (FOLLY_ROOT_DIR         ${INSTALL_DIR})
    set (FOLLY_STATIC_LIBRARY   "${FOLLY_ROOT_DIR}/lib/libfolly.a")
    set (FOLLY_LIBRARIES        ${FOLLY_STATIC_LIBRARY})
    set (FOLLY_FOUND            YES)

    find_path(FOLLY_INCLUDE_DIR "folly/String.h"
        PATHS       ${FOLLY_ROOT_DIR}/include)

    add_library(FOLLY_STATIC_LIBRARY STATIC IMPORTED)
    add_dependencies(FOLLY_STATIC_LIBRARY Folly)
    mark_as_advanced(FOLLY_LIBRARIES FOLLY_STATIC_LIBRARY FOLLY_INCLUDE_DIR)

    message(STATUS "Use bundled folly v${FOLLY_VERSION}")
endif ()
