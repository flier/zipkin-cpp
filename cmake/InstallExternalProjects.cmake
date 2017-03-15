include(ExternalProject)
include(FindPackageHandleStandardArgs)

set (EXTERNAL_PROJECT_DIR ${CMAKE_BINARY_DIR}/externals)
set_directory_properties(PROPERTIES EP_PREFIX ${EXTERNAL_PROJECT_DIR})

message(STATUS "Install external dependencies to ${EXTERNAL_PROJECT_DIR}")

include_directories(BEFORE SYSTEM ${EXTERNAL_PROJECT_DIR}/include)
link_directories(${EXTERNAL_PROJECT_DIR}/lib)

macro(install_external_project project_name)
    include(Install${project_name})
endmacro()

if (NOT ZLIB_FOUND OR USE_BUNDLED_ZLIB)
    install_external_project(ZLib)

    message(STATUS "Use bundled zlib v${ZLIB_VERSION_STRING}")
endif ()

if (NOT CURL_FOUND OR USE_BUNDLED_CURL)
    install_external_project(CURL)

    message(STATUS "Use bundled curl v${CURL_VERSION_STRING}")
endif ()

if (NOT RAPIDJSON_FOUND OR USE_BUNDLED_RAPIDJSON)
    install_external_project(RapidJSON)

    message(STATUS "Use bundled RapidJSON v${RAPIDJSON_VERSION}")
endif ()

if (NOT FOLLY_FOUND OR USE_BUNDLED_FOLLY)
    install_external_project(Folly)

    message(STATUS "Use bundled folly v${FOLLY_VERSION}")
endif ()

if (NOT LIBRDKAFKA_FOUND OR USE_BUNDLED_LIBRDKAFKA)
    install_external_project(LibRDKafka)

    message(STATUS "Use bundled librdkafka v${LibRDKafka_VERSION}")
endif ()

if (NOT DOUBLE_CONVERSION_FOUND OR USE_BUNDLED_DOUBLE_CONVERSION)
    install_external_project(DoubleConversion)

    message(STATUS "Use bundled double_conversion v${DOUBLE_CONVERSION_VERSION}")
endif ()

if (NOT GFLAGS_FOUND OR USE_BUNDLED_GFLAGS)
    install_external_project(GFlags)

    message(STATUS "Use bundled gflags v${GFLAGS_VERSION}")
endif ()

if (NOT GLOG_FOUND OR USE_BUNDLED_GLOG)
    install_external_project(GLog)

    message(STATUS "Use bundled glog v${GLOG_VERSION}")
endif ()

if (NOT GTEST_FOUND OR USE_BUNDLED_GTEST)
    install_external_project(GTest)

    message(STATUS "Use bundled gtest v${GTEST_VERSION}")
endif ()

if (NOT GBENCH_FOUND OR USE_BUNDLED_GBENCH)
    install_external_project(GBench)

    message(STATUS "Use bundled google-benchmark v${GBENCH_VERSION}")
endif ()