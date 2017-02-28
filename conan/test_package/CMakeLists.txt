project(ZipkinTest)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_compile_options(-std=c++14)

add_executable(test main.cpp)
target_link_libraries(test ${CONAN_LIBS})