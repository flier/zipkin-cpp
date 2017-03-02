set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")

add_definitions(-DUSE_BOOST_THREAD=1)

set (zipkin_test_SRCS
    TestCollector.cpp
    TestMain.cpp
    TestSpan.cpp
    TestTracer.cpp
    )

add_executable(unittest ${zipkin_test_SRCS})
target_link_libraries(unittest
                      zipkin
                      ${zipkin_DEPENDENCIES}
                      ${CONAN_LIBS_GTEST}
                      )

add_test(
    NAME unittest
    COMMAND unittest
)