set (RAPIDJSON_VERSION          1.1.0)
set (RAPIDJSON_URL_MD5          badd12c511e081fec6c89c43a7027bce)

ExternalProject_Add(RapidJSON
    DOWNLOAD_NAME   RapidJSON-${RAPIDJSON_VERSION}.tar.gz
    URL             https://github.com/miloyip/rapidjson/archive/v${RAPIDJSON_VERSION}.tar.gz
    URL_MD5         ${RAPIDJSON_URL_MD5}
    CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

ExternalProject_Get_Property(RapidJSON INSTALL_DIR)

set (RAPIDJSON_ROOT_DIR     ${INSTALL_DIR})
set (RAPIDJSON_INCLUDES     ${RAPIDJSON_ROOT_DIR}/include)

find_package_handle_standard_args(RapidJSON
    REQUIRED_ARGS RAPIDJSON_INCLUDES
)

mark_as_advanced(RAPIDJSON_INCLUDES)