if (NOT CURL_FOUND OR USE_BUNDLED_CURL)
    if (NOT CURL_VERSION_STRING OR USE_BUNDLED_CURL)
        set (CURL_VERSION_STRING        7.54.1)
        set (CURL_URL_MD5               21a6e5658fd55103a90b11de7b2a8a8c)
    endif ()

    ExternalProject_Add(CURL
        URL                 https://curl.haxx.se/download/curl-${CURL_VERSION_STRING}.tar.gz
        URL_MD5             ${CURL_URL_MD5}
        CONFIGURE_COMMAND   <SOURCE_DIR>/configure
                                --prefix=<INSTALL_DIR>
                                --without-ssl
                                --without-nghttp2
                                --disable-ldap
        BUILD_COMMAND       make
        INSTALL_COMMAND     make install
        TEST_COMMAND        ""
    )

    ExternalProject_Get_Property(CURL INSTALL_DIR)

    set (CURL_ROOT_DIR          ${INSTALL_DIR})
    set (CURL_INCLUDE_DIRS      ${CURL_ROOT_DIR}/include)
    set (CURL_LIBRARY_DIRS      ${CURL_ROOT_DIR}/lib)
    set (CURL_LIBRARIES         ${CURL_LIBRARY_DIRS}/libcurl.a)
    set (CURL_FOUND             YES)

    mark_as_advanced(CURL_LIBRARIES CURL_INCLUDE_DIRS)

    message(STATUS "Use bundled curl v${CURL_VERSION_STRING}")
endif ()
