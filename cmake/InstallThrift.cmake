ExternalProject_Add(thrift
    URL                 http://www-us.apache.org/dist/thrift/${THRIFT_VERSION_STRING}/thrift-0.10.0.tar.gz
    URL_MD5             ${THRIFT_URL_MD5}
    CONFIGURE_COMMAND   CPPFLAGS=-I<BINARY_DIR>/lib/cpp/src <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --with-openssl=${OPENSSL_ROOT_DIR} --with-boost=${BOOST_ROOT} --without-c_glib --without-csharp --without-python --without-java --without-nodejs --without-lua --without-ruby --without-php --without-erlang --without-go --without-nodejs --without-qt4 --without-qt5 --disable-plugin
    BUILD_COMMAND       CPPFLAGS=-I<BINARY_DIR>/lib/cpp/src make
    INSTALL_COMMAND     make install
)

ExternalProject_Get_Property(thrift INSTALL_DIR)

set (THRIFT_ROOT_DIR        ${INSTALL_DIR})
set (THRIFT_INCLUDE_DIRS    ${THRIFT_ROOT_DIR}/include)
set (THRIFT_LIBRARY_DIRS    ${THRIFT_ROOT_DIR}/lib)
set (THRIFT_LIBRARIES       ${THRIFT_LIBRARY_DIRS}/libthrift.a)
set (THRIFT_FOUND           YES)

mark_as_advanced(THRIFT_LIBRARIES THRIFT_INCLUDE_DIRS)