if (NOT RAPIDJSON_FOUND OR USE_BUNDLED_RAPIDJSON)
    if (NOT RAPIDJSON_VERSION OR USE_BUNDLED_RAPIDJSON)
        set (RAPIDJSON_VERSION          1.1.0)
        set (RAPIDJSON_URL_MD5          badd12c511e081fec6c89c43a7027bce)
    endif ()

    ExternalProject_Add(RapidJSON
        DOWNLOAD_NAME   RapidJSON-${RAPIDJSON_VERSION}.tar.gz
        URL             https://github.com/miloyip/rapidjson/archive/v${RAPIDJSON_VERSION}.tar.gz
        URL_MD5         ${RAPIDJSON_URL_MD5}
        CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        TEST_COMMAND    ""
    )

    ExternalProject_Get_Property(RapidJSON INSTALL_DIR)

    set (RAPIDJSON_ROOT_DIR     ${INSTALL_DIR})
    set (RAPIDJSON_INCLUDES     ${RAPIDJSON_ROOT_DIR}/include)
    set (RAPIDJSON_FOUND        YES)

    mark_as_advanced(RAPIDJSON_INCLUDES)

    message(STATUS "Use bundled RapidJSON v${RAPIDJSON_VERSION}")
endif ()