set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")

set (zipkin_core_INCS
    ${GENDIR}/zipkinCore_constants.h
    ${GENDIR}/zipkinCore_types.h
    )

set (zipkin_core_SRCS
    ${GENDIR}/zipkinCore_constants.cpp
    ${GENDIR}/zipkinCore_types.cpp
    )

add_custom_command (
    OUTPUT ${zipkin_core_INCS} ${zipkin_core_SRCS}
    COMMAND ${THRIFT_COMPILER} --gen cpp -o ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/zipkinCore.thrift
    DEPENDS zipkinCore.thrift
    COMMENT "Generating zipkin Thrift binding files"
)

set (scribe_INCS
    ${GENDIR}/scribe_constants.h
    ${GENDIR}/scribe_types.h
    ${GENDIR}/Scribe.h
    )

set (scribe_SRCS
    ${GENDIR}/scribe_constants.cpp
    ${GENDIR}/scribe_types.cpp
    ${GENDIR}/Scribe.cpp
    )

add_custom_command (
    OUTPUT ${scribe_INCS} ${scribe_SRCS}
    COMMAND ${THRIFT_COMPILER} --gen cpp -o ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/scribe.thrift
    DEPENDS scribe.thrift
    COMMENT "Generating scribe Thrift binding files"
)

set (zipkin_INCS
    ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    ${CMAKE_CURRENT_BINARY_DIR}/Config.h
    Base64.h
    Span.h
    Tracer.h
    Propagation.h
    Collector.h
    KafkaCollector.h

    ScribeCollector.h
    XRayCollector.h
    ${INCDIR}/zipkin.h
    ${INCDIR}/zipkin.hpp
    )

set (zipkin_SRCS
    Span.cpp
    Tracer.cpp
    Propagation.cpp
    Collector.cpp
    KafkaCollector.cpp
    ScribeCollector.cpp
    XRayCollector.cpp
    CApi.cpp
    )

if (WITH_CURL)
    list(APPEND zipkin_INCS HttpCollector.h)
    list(APPEND zipkin_SRCS HttpCollector.cpp)
endif()

if (SHARED_LIB)
    set (LIB_TYPE SHARED)
else()
    set (LIB_TYPE STATIC)
endif()

message(STATUS "Library type: ${LIB_TYPE}")

add_library (zipkin ${LIB_TYPE} ${zipkin_SRCS} ${zipkin_core_SRCS} ${scribe_SRCS})
target_link_libraries (zipkin ${zipkin_DEPENDENCIES})

set (PREFIX ${CMAKE_INSTALL_PREFIX})
set (VERSION ${zipkin_VERSION})

configure_file (${PROJECT_SOURCE_DIR}/zipkin.pc.in
                ${PROJECT_BINARY_DIR}/zipkin.pc
                @ONLY
                )

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in
                ${CMAKE_CURRENT_BINARY_DIR}/Config.h
                @ONLY
                )

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in
                ${CMAKE_CURRENT_BINARY_DIR}/Version.h
                @ONLY
                )

install (
    TARGETS zipkin
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install (
    FILES ${zipkin_INCS} ${zipkin_core_INCS} ${scribe_INCS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/zipkin
)

install (
    FILES ${PROJECT_BINARY_DIR}/zipkin.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig
)
