set (EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -std=c++11 -Wno-invalid-offsetof")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS}")

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
)

set (zipkin_INCS
    Collector.h
    Span.h
    Tracer.h
    ${INCDIR}/zipkin.h
    ${INCDIR}/zipkin.hpp
    )

set (zipkin_SRCS
    CApi.cpp
    Collector.cpp
    Span.cpp
    Tracer.cpp
    )



if (SHARED_LIB)
    set (LIB_TYPE SHARED)
else ()
    set (LIB_TYPE STATIC)
endif ()

message(STATUS "Library type: ${LIB_TYPE}")

add_library (zipkin ${LIB_TYPE} ${zipkin_SRCS} ${zipkin_core_SRCS})
target_link_libraries (zipkin ${zipkin_DEPENDENCIES})

set (PREFIX ${CMAKE_INSTALL_PREFIX})
set (VERSION ${zipkin_VERSION})

configure_file (${PROJECT_SOURCE_DIR}/zipkin.pc.in
                ${PROJECT_BINARY_DIR}/zipkin.pc
                @ONLY
                )

install (
    TARGETS zipkin
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install (
    FILES ${zipkin_INCS} ${zipkin_core_INCS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/zipkin
)

install (
    FILES ${PROJECT_BINARY_DIR}/zipkin.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig
)