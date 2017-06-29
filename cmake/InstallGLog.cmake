if (NOT GLOG_FOUND OR USE_BUNDLED_GLOG)
    if (NOT GLOG_VERSION OR USE_BUNDLED_GLOG)
        set (GLOG_VERSION       0.3.5)
        set (GLOG_URL_MD5       5df6d78b81e51b90ac0ecd7ed932b0d4)
    endif ()

    ExternalProject_Add(GLog
        DOWNLOAD_NAME       GLog-${GLOG_VERSION}.tar.gz
        URL                 https://github.com/google/glog/archive/v${GLOG_VERSION}.tar.gz
        URL_MD5             ${GLOG_URL_MD5}
        CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND       make
        INSTALL_COMMAND     make install
        TEST_COMMAND        ""
    )

    ExternalProject_Get_Property(GLog INSTALL_DIR)

    set (GLOG_ROOT_DIR          ${INSTALL_DIR})
    set (GLOG_INCLUDE_PATH      ${GLOG_ROOT_DIR}/include)
    set (GLOG_LIBRARY           ${GLOG_ROOT_DIR}/lib/libglog.a)
    set (GLOG_FOUND             YES)

    add_library(GLOG_LIBRARY STATIC IMPORTED)
    add_dependencies(GLOG_LIBRARY GLog)
    mark_as_advanced(GLOG_LIBRARY GLOG_INCLUDE_PATH)

    message(STATUS "Use bundled glog v${GLOG_VERSION}")
endif ()
