if (NOT GFLAGS_FOUND OR USE_BUNDLED_GFLAGS)
    if (NOT GFLAGS_VERSION OR USE_BUNDLED_GFLAGS)
        set (GFLAGS_VERSION     2.2.0)
        set (GFLAGS_URL_MD5     b99048d9ab82d8c56e876fb1456c285e)
    endif ()

    ExternalProject_Add(GFlags
        DOWNLOAD_NAME       gflags-${GFLAGS_VERSION}.tar.gz
        URL                 https://github.com/gflags/gflags/archive/v${GFLAGS_VERSION}.tar.gz
        URL_MD5             ${GFLAGS_URL_MD5}
        CMAKE_ARGS          -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                            -DGFLAGS_BUILD_SHARED_LIBS=ON
                            -DGFLAGS_BUILD_STATIC_LIBS=ON
        TEST_COMMAND        ""
    )

    ExternalProject_Get_Property(GFlags INSTALL_DIR)

    set (GFLAGS_ROOT_DIR        ${INSTALL_DIR})
    set (GFLAGS_INCLUDE_DIRS    ${GFLAGS_ROOT_DIR}/include)
    set (GFLAGS_LIBRARIES_DIR   ${GFLAGS_ROOT_DIR}/lib)
    set (GFLAGS_LIBRARIES       ${GFLAGS_LIBRARIES_DIR}/libgflags.a)
    set (GFLAGS_FOUND           YES)

    add_library(GFLAGS_LIBRARIES STATIC IMPORTED)
    add_dependencies(GFLAGS_LIBRARIES gflags)
    mark_as_advanced(GFLAGS_LIBRARIES GFLAGS_LIBRARIES_DIR GFLAGS_INCLUDE_DIRS)

    message(STATUS "Use bundled gflags v${GFLAGS_VERSION}")
endif ()
