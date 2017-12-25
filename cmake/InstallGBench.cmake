if (NOT GBENCH_FOUND OR USE_BUNDLED_GBENCH)
    if (NOT GBENCH_VERSION OR USE_BUNDLED_GBENCH)
        set (GBENCH_VERSION     1.3.0)
        set (GBENCH_URL_MD5     19ce86516ab82d6ad3b17173cf307aac)
    endif ()

    ExternalProject_Add(GBench
        DOWNLOAD_NAME       GBench-${GBENCH_VERSION}.tar.gz
        URL                 https://github.com/google/benchmark/archive/v${GBENCH_VERSION}.tar.gz
        URL_MD5             ${GBENCH_URL_MD5}
        CMAKE_ARGS          -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                            -DBENCHMARK_ENABLE_TESTING=OFF
                            -DBENCHMARK_ENABLE_LTO=ON
        TEST_COMMAND        ""
    )

    ExternalProject_Get_Property(GBench INSTALL_DIR)

    set (GBENCH_ROOT_DIR        ${INSTALL_DIR})
    set (GBENCH_INCLUDE_PATH    ${GBENCH_ROOT_DIR}/include)
    set (GBENCH_LIBRARY         ${GBENCH_ROOT_DIR}/lib/libbenchmark.a)
    set (GBENCH_FOUND           YES)

    add_library(GBENCH_LIBRARY STATIC IMPORTED)
    add_dependencies(GBENCH_LIBRARY GBench)
    mark_as_advanced(GBENCH_LIBRARY GBENCH_INCLUDE_PATH)

    message(STATUS "Use bundled google-benchmark v${GBENCH_VERSION}")
endif ()
