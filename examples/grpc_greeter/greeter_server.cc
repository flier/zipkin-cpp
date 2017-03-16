/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpc++/grpc++.h>

#include "zipkin.hpp"

#include "helloworld.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

// Logic and data behind the server's behavior.
struct GreeterServiceImpl final : public Greeter::Service
{
    std::unique_ptr<zipkin::Tracer> tracer_;

    Status SayHello(ServerContext *context, const HelloRequest *request,
                    HelloReply *reply) override
    {
        zipkin::Span &span = *tracer_->span("SayHello");
        zipkin::Span::Scope scope(span);
        zipkin::Propagation::extract(*context, span);
        zipkin::Endpoint endpoint("greeter_server");

        span << std::make_pair("name", request->name()) << endpoint
             << zipkin::TraceKeys::SERVER_RECV << endpoint;

        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());

        span << zipkin::TraceKeys::SERVER_SEND << endpoint;

        return Status::OK;
    }
};

DEFINE_string(grpc_addr, "localhost:50051", "GRPC server address");
DEFINE_string(collector_uri, "", "Collector URI for tracing");

void RunServer(std::shared_ptr<zipkin::Collector> collector)
{
    std::string server_address(FLAGS_grpc_addr);
    GreeterServiceImpl service;

    if (collector.get())
    {
        service.tracer_.reset(zipkin::Tracer::create(collector.get()));
    }

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]);
    int arg = gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::shared_ptr<zipkin::Collector> collector;

    if (!FLAGS_collector_uri.empty())
    {
        collector.reset(zipkin::Collector::create(FLAGS_collector_uri));
    }

    RunServer(collector);

    if (collector.get())
        collector->shutdown(std::chrono::seconds(5));

    google::ShutdownGoogleLogging();

    return 0;
}
