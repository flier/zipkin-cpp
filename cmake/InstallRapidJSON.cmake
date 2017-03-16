ExternalProject_Add(RapidJSON
    DOWNLOAD_NAME   RapidJSON-${RAPIDJSON_VERSION}.tar.gz
    URL             https://github.com/miloyip/rapidjson/archive/v${RAPIDJSON_VERSION}.tar.gz
    URL_MD5         ${RAPIDJSON_URL_MD5}
    CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

ExternalProject_Get_Property(RapidJSON INSTALL_DIR)

set (RAPIDJSON_ROOT_DIR     ${INSTALL_DIR})
set (RAPIDJSON_INCLUDES     ${RAPIDJSON_ROOT_DIR}/include)
set (RAPIDJSON_FOUND        YES)

mark_as_advanced(RAPIDJSON_INCLUDES)