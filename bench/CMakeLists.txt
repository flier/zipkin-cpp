set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -std=c++11 -Wno-invalid-offsetof")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS}")

set (zipkin_bench_SRCS
    BenchSpan.cpp
    )

add_executable(bench ${zipkin_bench_SRCS})

target_link_libraries(bench
                      zipkin
                      ${zipkin_DEPENDENCIES}
                      ${GBENCH_LIBRARY}
                      )