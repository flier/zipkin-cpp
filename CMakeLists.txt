cmake_minimum_required (VERSION 3.0.0)

project(zipkin-cpp
    VERSION 0.2.0
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost REQUIRED COMPONENTS regex system)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(CURL REQUIRED)
find_package(Thrift REQUIRED)
find_package(LibRDKafka REQUIRED)
find_package(Doxygen QUIET)

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

set(zipkin_DEPENDENCIES
    ${CONAN_LIBS_GLOG}
    ${CONAN_LIBS_GFLAGS}
    ${CURL_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LibRDKafka_LIBRARIES}
    ${Boost_LIBRARIES}
    )

message (STATUS "Link with ${zipkin_DEPENDENCIES}")

enable_testing()

add_subdirectory(docs)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(bench)
add_subdirectory(examples)