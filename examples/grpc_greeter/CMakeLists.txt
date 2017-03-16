if (NOT WITH_GRPC)
    message(WARNING "Not found protobuf, gRPC or folly, skip the grpc_greeter example")
    return()
endif()

include_directories(${PROTOBUF_INCLUDE_DIRS})

# Proto file
get_filename_component(hw_proto "helloworld.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

set(greeter_PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/greeter.pb.cc")
set(greeter_PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/greeter.pb.h")
set(greeter_GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/greeter.grpc.pb.cc")
set(greeter_GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/greeter.grpc.pb.h")

# Generated sources
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(greeter_PROTO_SRCS greeter_PROTO_HDRS ${hw_proto})
protobuf_generate_grpc_cpp(greeter_GRPC_SRCS greeter_GRPC_HDRS ${hw_proto})

set (EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -std=c++11")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS}")

# Targets greeter_[async_](client|server)
foreach(_target
    greeter_client greeter_server
    greeter_async_client greeter_async_server
)
    add_executable(${_target} "${_target}.cc"
        ${greeter_PROTO_SRCS} ${greeter_PROTO_HDRS}
        ${greeter_GRPC_SRCS} ${greeter_GRPC_HDRS}
    )
    target_link_libraries(${_target}
        ${PROTOBUF_LIBRARIES}
        ${GRPC_LIBRARY}
        ${GRPCPP_LIBRARY}
        zipkin ${zipkin_DEPENDENCIES}
    )
endforeach()