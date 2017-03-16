set (GLOG_VERSION       0.3.4)
set (GLOG_URL_MD5       df92e05c9d02504fb96674bc776a41cb)

ExternalProject_Add(glog
    DOWNLOAD_NAME       glog-${GLOG_VERSION}.tar.gz
    URL                 https://github.com/google/glog/archive/v${GLOG_VERSION}.tar.gz
    URL_MD5             ${GLOG_URL_MD5}
    CONFIGURE_COMMAND   <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND       make
    INSTALL_COMMAND     make install
)

ExternalProject_Get_Property(glog INSTALL_DIR)

set (GLOG_ROOT_DIR          ${INSTALL_DIR})
set (GLOG_INCLUDE_PATH      ${GLOG_ROOT_DIR}/include)
set (GLOG_LIBRARY           ${GLOG_ROOT_DIR}/lib/libglog.a)
set (GLOG_FOUND             YES)

add_library(GLOG_LIBRARY STATIC IMPORTED)
add_dependencies(GLOG_LIBRARY glog)
mark_as_advanced(GLOG_LIBRARY GLOG_INCLUDE_PATH)