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

#include <folly/Format.h>
#include <folly/String.h>
#include <folly/Uri.h>

#include "zipkin.hpp"

#include "helloworld.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class GreeterClient
{
  public:
    GreeterClient(std::shared_ptr<Channel> channel, std::shared_ptr<zipkin::Collector> collector)
        : stub_(Greeter::NewStub(channel)), tracer_(zipkin::Tracer::create(collector.get(), "greeter"))
    {
    }

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    std::string SayHello(const std::string &user)
    {
        zipkin::Span &span = *tracer_->span("SayHello");
        zipkin::Span::Scope scope(span);

        // Data we are sending to the server.
        HelloRequest request;
        request.set_name(user);

        // Container for the data we expect from the server.
        HelloReply reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        zipkin::Endpoint endpoint("greeter");

        span << std::make_pair("name", user) << endpoint;

        auto send_header = [&context](const char *key, const std::string &value) {
            std::string lower_key(key);
            folly::toLowerAscii(const_cast<char *>(lower_key.data()), lower_key.size());
            context.AddMetadata(lower_key, value);
        };

        send_header(ZIPKIN_X_TRACE_ID, folly::to<std::string>(span.trace_id()));
        send_header(ZIPKIN_X_SPAN_ID, folly::to<std::string>(span.id()));

        if (span.parent_id())
            send_header(ZIPKIN_X_PARENT_SPAN_ID, folly::to<std::string>(span.parent_id()));
        if (span.sampled())
            send_header(ZIPKIN_X_SAMPLED, "1");
        if (span.debug())
            send_header(ZIPKIN_X_FLAGS, "1");

        span << zipkin::TraceKeys::CLIENT_SEND << endpoint;

        // The actual RPC.
        Status status = stub_->SayHello(&context, request, &reply);

        span << zipkin::TraceKeys::CLIENT_RECV << endpoint;

        // Act upon its status.
        if (status.ok())
        {
            return reply.message();
        }
        else
        {
            span << std::make_pair(zipkin::TraceKeys::ERROR, status.error_message());

            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

  private:
    std::unique_ptr<Greeter::Stub> stub_;
    std::unique_ptr<zipkin::Tracer> tracer_;
};

DEFINE_string(grpc_addr, "localhost:50051", "GRPC server address");
DEFINE_string(kafka_uri, "", "Kafka URI for tracing");
DEFINE_string(msg_codec, "pretty_json", "Message codec");

int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]);
    int arg = gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::shared_ptr<zipkin::KafkaCollector> collector;

    if (!FLAGS_kafka_uri.empty())
    {
        folly::Uri uri(FLAGS_kafka_uri);

        zipkin::KafkaConf conf(uri);

        conf.message_codec = zipkin::MessageCodec::parse(FLAGS_msg_codec);

        collector.reset(conf.create());
    }

    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    std::shared_ptr<Channel> channel = grpc::CreateChannel(FLAGS_grpc_addr, grpc::InsecureChannelCredentials());
    GreeterClient greeter(channel, collector);
    std::string user(arg < argc ? argv[arg] : "world");
    std::string reply = greeter.SayHello(user);
    std::cout << "Greeter received: " << reply << std::endl;

    collector->flush(std::chrono::seconds(5));

    google::ShutdownGoogleLogging();

    return 0;
}
