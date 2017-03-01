cmake_minimum_required (VERSION 2.8.12)

project(zipkin C CXX)

set (zipkin_VERSION_MAJOR 0)
set (zipkin_VERSION_MINOR 2)
set (zipkin_VERSION_PATCH 0)
set (zipkin_VERSION ${zipkin_VERSION_MAJOR}.${zipkin_VERSION_MINOR}.${zipkin_VERSION_PATCH})

include(${PROJECT_SOURCE_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

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

find_package(Boost REQUIRED)
find_package(Thrift REQUIRED)
find_package(LibRDKafka REQUIRED)
find_package(Doxygen QUIET)

if (NOT Boost_FOUND)
    message(FATAL_ERROR "boost not found")
endif()

if (NOT THRIFT_FOUND)
    message(FATAL_ERROR "Thrift runtime not found")
endif()

if (NOT LIBRDKAFKA_FOUND)
    message(FATAL_ERROR "librdkafka not found")
endif()

if (${DOXYGEN_FOUND} AND ${RELEASE_BUILD})
    set (BUILD_DOCS ON)
else()
    set (BUILD_DOCS OFF)
endif()

option (WITH_FPIC "Build with -fPIC for shared library" OFF)
option (SHARED_LIB "Build shared library" OFF)
option (BUILD_DOCS "Build API documentation (requires Doxygen)" BUILD_DOCS)

if (APPLE)
    set (CMAKE_MACOSX_RPATH ON)
endif ()

if (RELEASE_BUILD)
    add_compile_options(-O2)
else()
    add_compile_options(-g)
endif()

if (WITH_FPIC)
    message(STATUS "Build with -fPIC")
    add_compile_options(-fPIC)
endif()

set (BINDIR "${PROJECT_BINARY_DIR}/bin")
set (INCDIR "${PROJECT_BINARY_DIR}/include")
set (LIBDIR "${PROJECT_BINARY_DIR}/lib")
set (GENDIR "${PROJECT_BINARY_DIR}/gen-cpp")

if(CMAKE_GENERATOR STREQUAL Xcode)
    set(XCODE TRUE)
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/gen-cpp)
include_directories(${PROJECT_BINARY_DIR})
include_directories(SYSTEM include)

set(zipkin_DEPENDENCIES
    ${CONAN_LIBS_GLOG}
    ${CONAN_LIBS_GFLAGS}
    ${THRIFT_LIBRARIES}
    ${LibRDKafka_LIBRARIES}
    )

enable_testing()

add_subdirectory(docs)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(examples)