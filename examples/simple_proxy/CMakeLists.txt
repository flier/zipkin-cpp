set (simple_proxy_SRCS
    main.c
    mongoose.c
    zf_log.c
    )

add_executable(simple_proxy ${simple_proxy_SRCS})
target_link_libraries(simple_proxy zipkin ${zipkin_DEPENDENCIES})
