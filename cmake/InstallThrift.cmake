if (NOT THRIFT_FOUND OR USE_BUNDLED_THRIFT)
    if (NOT THRIFT_VERSION_STRING OR USE_BUNDLED_THRIFT)
        set (THRIFT_VERSION_STRING        0.10.0)
        set (THRIFT_URL_MD5               795c5dd192e310ffff38cfd9430d6b29)
    endif ()

    ExternalProject_Add(Thrift
        URL                 http://www-us.apache.org/dist/thrift/${THRIFT_VERSION_STRING}/thrift-${THRIFT_VERSION_STRING}.tar.gz
        URL_MD5             ${THRIFT_URL_MD5}
        CONFIGURE_COMMAND   CXXFLAGS=-I<BINARY_DIR>/lib/cpp/src
                            <SOURCE_DIR>/configure
                            --prefix=<INSTALL_DIR> ${WITH_OPENSSL}
                            --with-boost=${BOOST_ROOT}
                            --without-c_glib
                            --without-csharp
                            --without-python
                            --without-java
                            --without-nodejs
                            --without-lua
                            --without-ruby
                            --without-php
                            --without-erlang
                            --without-go
                            --without-nodejs
                            --without-qt4
                            --without-qt5
                            --disable-plugin
        BUILD_COMMAND       CXXFLAGS=-I<BINARY_DIR>/lib/cpp/src make
        INSTALL_COMMAND     make install
        TEST_COMMAND        ""
    )

    ExternalProject_Get_Property(Thrift INSTALL_DIR)

    set (THRIFT_ROOT_DIR        ${INSTALL_DIR})
    set (THRIFT_INCLUDE_DIRS    ${THRIFT_ROOT_DIR}/include)
    set (THRIFT_LIBRARY_DIRS    ${THRIFT_ROOT_DIR}/lib)
    set (THRIFT_LIBRARIES       ${THRIFT_LIBRARY_DIRS}/libthrift.a)
    set (THRIFT_FOUND           YES)

    mark_as_advanced(THRIFT_LIBRARIES THRIFT_INCLUDE_DIRS)

    message(STATUS "Use bundled thrift v${THRIFT_VERSION_STRING}")
endif ()
