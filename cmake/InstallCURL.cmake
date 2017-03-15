set (CURL_VERSION_STRING        7.53.1)
set (CURL_URL_MD5               9e49bb4cb275bf4464e7b69eb48613c0)

ExternalProject_Add(curl
    URL                 https://curl.haxx.se/download/curl-${CURL_VERSION_STRING}.tar.gz
    URL_MD5             ${CURL_URL_MD5}
    CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --without-ssl --without-nghttp2 --disable-ldap
    BUILD_COMMAND       make
    INSTALL_COMMAND     make install
)

ExternalProject_Get_Property(curl INSTALL_DIR)

set (CURL_ROOT_DIR          ${INSTALL_DIR})
set (CURL_INCLUDE_DIRS      ${CURL_ROOT_DIR}/include)
set (CURL_LIBRARY_DIRS      ${CURL_ROOT_DIR}/lib)
set (CURL_LIBRARIES         ${CURL_LIBRARY_DIRS}/libcurl.a)

find_package_handle_standard_args(curl
    REQUIRED_ARGS CURL_LIBRARIES CURL_INCLUDE_DIRS
)

mark_as_advanced(CURL_LIBRARIES CURL_INCLUDE_DIRS)