set (LibRDKafka_VERSION         0.9.4)

ExternalProject_Add(LibRDKafka
    DOWNLOAD_NAME   LibRDKafka-${LibRDKafka_VERSION}.tar.gz
    URL             https://github.com/edenhill/librdkafka/archive/v${LibRDKafka_VERSION}.tar.gz
    URL_MD5         6f40198e6068475c34ae8c9faafa6e8a
    CMAKE_ARGS      -DRDKAFKA_BUILD_EXAMPLES=OFF -DRDKAFKA_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

ExternalProject_Get_Property(LibRDKafka INSTALL_DIR)

set (LibRDKafka_ROOT_DIR        ${INSTALL_DIR})
set (LibRDKafka_LIBRARY_DIR     ${LibRDKafka_ROOT_DIR}/lib)
set (LibRDKafka_INCLUDE_DIR     ${LibRDKafka_ROOT_DIR}/include)
set (LibRDKafka_LIBRARY_PATH    ${LibRDKafka_LIBRARY_DIR}/librdkafka++.a)
set (LibRDKafka_C_LIBRARY_PATH  ${LibRDKafka_LIBRARY_DIR}/librdkafka.a)

find_library(LibRDKafka_LIBRARY
    NAMES rdkafka++
    PATH ${LibRDKafka_LIBRARY_DIR}
    NO_DEFAULT_PATH NO_SYSTEM_ENVIRONMENT_PATH
)
find_library(LibRDKafka_C_LIBRARY
    NAMES rdkafka
    PATH ${LibRDKafka_LIBRARY_DIR}
    NO_DEFAULT_PATH NO_SYSTEM_ENVIRONMENT_PATH
)

set (LibRDKafka_LIBRARIES       ${LibRDKafka_LIBRARY_PATH} ${LibRDKafka_C_LIBRARY_PATH})
set (LibRDKafka_C_LIBRARIES     ${LibRDKafka_C_LIBRARY_PATH})

find_package_handle_standard_args(LibRDKafka
    REQUIRED_ARGS
        LibRDKafka_LIBRARIES LibRDKafka_C_LIBRARIES
        LibRDKafka_LIBRARY_PATH LibRDKafka_C_LIBRARY_PATH
        LibRDKafka_INCLUDE_DIR
)

add_library(LibRDKafka_LIBRARY_PATH STATIC IMPORTED)
add_library(LibRDKafka_C_LIBRARY_PATH STATIC IMPORTED)
add_library(LibRDKafka_LIBRARY SHARED IMPORTED)
add_library(LibRDKafka_C_LIBRARY SHARED IMPORTED)
add_dependencies(LibRDKafka_LIBRARY_PATH LibRDKafka)
add_dependencies(LibRDKafka_C_LIBRARY_PATH LibRDKafka)
mark_as_advanced(LibRDKafka_LIBRARIES LibRDKafka_C_LIBRARIES LibRDKafka_INCLUDE_DIR)