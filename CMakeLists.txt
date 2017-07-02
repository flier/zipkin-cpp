cmake_minimum_required (VERSION 3.0.0)

project(zipkin-cpp
    VERSION 0.3.1
    LANGUAGES C CXX)

include(CTest)
enable_testing()

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_STANDARD_REQUIRED ON)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_COLOR_MAKEFILE ON)

if (APPLE)
    set (CMAKE_MACOSX_RPATH ON)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Default build type 'Release with debug info'")
    set(CMAKE_BUILD_TYPE RELWITHDEBINFO CACHE STRING "" FORCE )
else()
    string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

if(CMAKE_BUILD_TYPE MATCHES RELEASE|RELWITHDEBINFO)
    set(RELEASE_BUILD TRUE)
else()
    set(RELEASE_BUILD FALSE)
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(Boost_USE_MULTITHREADED      ON)

find_package(Boost REQUIRED COMPONENTS regex system thread)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB)
find_package(Thrift)
find_package(LibRDKafka)
find_package(Folly)
find_package(RapidJSON)
find_package(DoubleConversion)
find_package(GFlags)
find_package(GLog)
find_package(GMock)
find_package(GBench)
find_package(CURL)
find_package(Protobuf)
find_package(GRPC)
find_package(Gperftools)
find_package(Doxygen)

if (OPENSSL_FOUND)
    list(GET OPENSSL_LIBRARIES 0 openssl_libname)
    get_filename_component(OPENSSL_LIBRARY_DIR ${openssl_libname} DIRECTORY)
    get_filename_component(OPENSSL_ROOT_DIR ${OPENSSL_LIBRARY_DIR} DIRECTORY)

    if (APPLE)
        set (WITH_OPENSSL   "--with-openssl=${OPENSSL_ROOT_DIR}")
    endif ()

    message(STATUS "Found OpenSSL ${OPENSSL_VERSION}, inc=${OPENSSL_INCLUDE_DIR}, libs=${OPENSSL_LIBRARIES} root=${OPENSSL_ROOT_DIR}")
endif ()

include(InstallExternalProjects)

if (CURL_FOUND AND ZLIB_FOUND)
    set (WITH_CURL ON)
else ()
    set (WITH_CURL OFF)
endif ()

if (GRPC_FOUND)
    set (WITH_GRPC ON)
else ()
    set (WITH_GRPC OFF)
endif ()

# gperftools - Google Performance Tool
#
# https://github.com/gperftools/gperftools
if (GPERFTOOLS_FOUND)
    set (WITH_TCMALLOC ON)
    if (RELEASE_BUILD)
        set (WITH_PROFILER OFF)
    else ()
        set (WITH_PROFILER ON)
    endif ()
else ()
    set (WITH_TCMALLOC OFF)
    set (WITH_PROFILER OFF)
endif ()

if (DOXYGEN_FOUND AND RELEASE_BUILD)
    set (BUILD_DOCS ON)
else ()
    set (BUILD_DOCS OFF)
endif ()

option (WITH_CURL "Build with cURL propagation" WITH_CURL)
option (WITH_GRPC "Build with gRPC propagation" WITH_GRPC)
option (WITH_FPIC "Build with -fPIC for shared library" OFF)
option (WITH_TCMALLOC "Build with tcmalloc library" WITH_TCMALLOC)
option (WITH_PROFILER "Build with CPU profiler" WITH_PROFILER)
option (SHARED_LIB "Build shared library" OFF)
option (BUILD_DOCS "Build API documentation (requires Doxygen)" BUILD_DOCS)

if (RELEASE_BUILD)
    add_compile_options(-O2)
else()
    add_compile_options(-g)
endif()

if (WITH_FPIC)
    message(STATUS "Build with -fPIC")
    add_compile_options(-fPIC)
endif()

set (INCDIR "${PROJECT_SOURCE_DIR}/include")
set (BINDIR "${PROJECT_BINARY_DIR}/bin")
set (LIBDIR "${PROJECT_BINARY_DIR}/lib")
set (GENDIR "${PROJECT_BINARY_DIR}/gen-cpp")

if(CMAKE_GENERATOR STREQUAL Xcode)
    set(XCODE TRUE)
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/gen-cpp)
include_directories(${PROJECT_BINARY_DIR}/src)
include_directories(SYSTEM include)

set (zipkin_DEPENDENCIES
    ${GLOG_LIBRARY}
    ${GFLAGS_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LibRDKafka_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${FOLLY_STATIC_LIBRARY}
    ${DOUBLE_CONVERSION_LIBRARY}
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    )

if (WITH_CURL)
    if (NOT CURL_FOUND)
        message(SEND_ERROR "cURL not found")
    endif()

    if (NOT ZLIB_FOUND)
        message(SEND_ERROR "zlib not found")
    endif()

    message(STATUS "Build with cURL propagation")

    list (APPEND zipkin_DEPENDENCIES ${CURL_LIBRARIES})

    set (CURL_ENABLED 1)
else()
    message(STATUS "cURL supports disabled")
endif()

if (WITH_GRPC)
    if (NOT GRPC_FOUND)
        message(SEND_ERROR "gRPC not found")
    endif()

    message(STATUS "Build with gRPC propagation")

    list(APPEND zipkin_DEPENDENCIES ${GRPC_LIBRARIES})

    set (GRPC_ENABLED 1)
else()
    message(STATUS "gRPC supports disabled")
endif()

if (WITH_TCMALLOC OR WITH_PROFILER)
    if (NOT GPERFTOOLS_FOUND)
        message(SEND_ERROR "gperftools not found")
    endif()

    if (WITH_TCMALLOC)
        add_compile_options(-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free)
    endif ()

    if (WITH_TCMALLOC AND WITH_PROFILER)
        message(STATUS "Build with tcmalloc and profiler library")

        list(APPEND zipkin_DEPENDENCIES ${GPERFTOOLS_LIBRARIES})
    elseif (WITH_TCMALLOC)
        message(STATUS "Build with tcmalloc library")

        list(APPEND zipkin_DEPENDENCIES ${GPERFTOOLS_TCMALLOC})
    elseif (WITH_PROFILER)
        message(STATUS "Build with profiler library")

        list(APPEND zipkin_DEPENDENCIES ${GPERFTOOLS_PROFILER})
    endif ()
endif ()

message (STATUS "Link with ${zipkin_DEPENDENCIES}")

add_subdirectory(docs)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(bench)
add_subdirectory(examples)
