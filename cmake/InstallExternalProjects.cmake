include(ExternalProject)

set (EXTERNAL_PROJECT_DIR ${CMAKE_BINARY_DIR}/externals)
set_directory_properties(PROPERTIES EP_PREFIX ${EXTERNAL_PROJECT_DIR})

list(APPEND CMAKE_MODULE_PATH ${EXTERNAL_PROJECT_DIR}/lib/cmake)

message(STATUS "Install external dependencies to ${EXTERNAL_PROJECT_DIR}")

include_directories(BEFORE SYSTEM ${EXTERNAL_PROJECT_DIR}/include)
link_directories(${EXTERNAL_PROJECT_DIR}/lib)

macro(install_external_project project_name)
    include(Install${project_name})
endmacro()

install_external_project(ZLib)
install_external_project(CURL)
install_external_project(RapidJSON)
install_external_project(LibRDKafka)
install_external_project(DoubleConversion)
install_external_project(GFlags)
install_external_project(GLog)
install_external_project(GTest)
install_external_project(GBench)
install_external_project(Folly)
install_external_project(Thrift)
