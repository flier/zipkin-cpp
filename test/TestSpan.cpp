#include "Mocks.hpp"

#include <utility>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

TEST(endpoint, properties)
{
    zipkin::Endpoint endpoint("test", "127.0.0.1", 80);

    ASSERT_EQ(endpoint.service_name(), "test");
    ASSERT_TRUE(endpoint.addr().is_v4());
    ASSERT_EQ(endpoint.addr().to_v4().to_ulong(), 0x7f000001); // host bytes
    ASSERT_EQ(endpoint.port(), 80);
}

TEST(endpoint, with_addr)
{
    zipkin::Endpoint endpoint;

    auto v4addr = endpoint.with_addr("127.0.0.1", 8004).sockaddr();

    ASSERT_EQ(v4addr->sa_family, AF_INET);
    ASSERT_EQ(reinterpret_cast<const sockaddr_in *>(v4addr.get())->sin_port, htons(8004));
    ASSERT_EQ(reinterpret_cast<const sockaddr_in *>(v4addr.get())->sin_addr.s_addr, 0x100007f);

    auto v6addr = endpoint.with_addr("::1", 8006).sockaddr();

    ASSERT_EQ(v6addr->sa_family, AF_INET6);
    ASSERT_EQ(reinterpret_cast<const sockaddr_in6 *>(v6addr.get())->sin6_port, htons(8006));
    ASSERT_THAT(reinterpret_cast<const sockaddr_in6 *>(v6addr.get())->sin6_addr.s6_addr,
                ElementsAreArray({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}));

    endpoint.with_addr(v4addr.get());

    ASSERT_TRUE(endpoint.addr().is_v4());
    ASSERT_EQ(endpoint.addr().to_v4().to_string(), "127.0.0.1");
    ASSERT_EQ(endpoint.port(), 8004);

    endpoint.with_addr(v6addr.get());

    ASSERT_TRUE(endpoint.addr().is_v6());
    ASSERT_EQ(endpoint.addr().to_v6().to_string(), "::1");
    ASSERT_EQ(endpoint.port(), 8006);
}

TEST(span, properties)
{
    MockTracer tracer;

    zipkin::Span span(&tracer, "test");

    ASSERT_NE(span.id(), 0);
    ASSERT_EQ(span.name(), "test");
    ASSERT_EQ(span.tracer(), &tracer);

    const ::Span &msg = span.message();

    ASSERT_NE(msg.trace_id, 0);
    ASSERT_NE(msg.trace_id_high, 0);
    ASSERT_EQ(msg.name, "test");
    ASSERT_EQ(msg.id, span.id());
    ASSERT_EQ(msg.parent_id, 0);
    ASSERT_TRUE(msg.annotations.empty());
    ASSERT_TRUE(msg.binary_annotations.empty());
    ASSERT_FALSE(msg.debug);
    ASSERT_NE(msg.timestamp, 0);
    ASSERT_EQ(msg.duration, 0);
}

TEST(span, child_span)
{
    MockTracer tracer;

    zipkin::Span span(&tracer, "test");

    span.with_debug(true).with_sampled(false);

    zipkin::Span *child = span.span("child");

    ASSERT_TRUE(child);

    // inherited properties
    ASSERT_EQ(child->parent_id(), span.id());
    ASSERT_EQ(child->trace_id(), span.trace_id());
    ASSERT_EQ(child->trace_id_high(), span.trace_id_high());
    ASSERT_TRUE(child->debug());
    ASSERT_FALSE(child->sampled());
}

TEST(span, submit)
{
    MockTracer tracer;

    zipkin::Span span(&tracer, "test");

    const ::Span &msg = span.message();

    ASSERT_NE(msg.timestamp, 0);
    ASSERT_EQ(msg.duration, 0);

    EXPECT_CALL(tracer, submit(&span))
        .Times(1);

    span.submit();

    ASSERT_NE(msg.timestamp, 0);
    ASSERT_NE(msg.duration, 0);
}

TEST(span, annotate)
{
    zipkin::Span span(nullptr, "test");

    const ::Span &msg = span.message();

    ASSERT_TRUE(msg.annotations.empty());

    span.client_send();

    ASSERT_FALSE(msg.annotations.empty());

    const ::Annotation &annotation = msg.annotations.back();

    ASSERT_EQ(annotation.value, g_zipkinCore_constants.CLIENT_SEND);
    ASSERT_NE(annotation.timestamp, 0);
    ASSERT_EQ(annotation.__isset.host, 0);

    span.http_host("www.google.com");

    ASSERT_FALSE(msg.binary_annotations.empty());

    const ::BinaryAnnotation &binary_annotation = msg.binary_annotations.back();

    ASSERT_EQ(binary_annotation.key, g_zipkinCore_constants.HTTP_HOST);
    ASSERT_EQ(binary_annotation.value, "www.google.com");
    ASSERT_EQ(binary_annotation.annotation_type, AnnotationType::type::STRING);
    ASSERT_EQ(binary_annotation.__isset.host, 0);

    span.annotate("bool", true);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::BOOL);
    ASSERT_STREQ(msg.binary_annotations.back().value.c_str(), "\x01");

    span.annotate("i16", (int16_t)-123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I16);
    ASSERT_STREQ(msg.binary_annotations.back().value.c_str(), "\xff\x85");

    span.annotate("u16", (uint16_t)123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I16);
    ASSERT_STREQ(msg.binary_annotations.back().value.c_str(), "\x00\x7b");

    span.annotate("i32", (int32_t)-123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I32);
    ASSERT_STREQ(msg.binary_annotations.back().value.c_str(), "\xff\xff\xff\x85");

    span.annotate("u32", (uint32_t)123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I32);
    ASSERT_STREQ(msg.binary_annotations.back().value.c_str(), "\x00\x00\x00\x7b");

    span.annotate("i64", (int64_t)-123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I64);
    ASSERT_STREQ(msg.binary_annotations.back().value.c_str(), "\xff\xff\xff\xff\xff\xff\xff\x85");

    span.annotate("u64", (uint64_t)123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I64);
    ASSERT_STREQ(msg.binary_annotations.back().value.c_str(), "\x00\x00\x00\x00\x00\x00\x00\x7b");

    span.annotate("double", (double)12.3);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::DOUBLE);
    ASSERT_STREQ(msg.binary_annotations.back().value.c_str(), "\x9A\x99\x99\x99\x99\x99\x28\x40");

    span.annotate("string", std::wstring(L"测试"));
    ASSERT_EQ(msg.binary_annotations.back().value, "测试");
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::STRING);

    span.annotate("string", L"测试");
    ASSERT_EQ(msg.binary_annotations.back().value, "测试");
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::STRING);

    uint8_t bytes[] = {1, 2, 3};

    span.annotate("bytes", bytes);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::BYTES);
}

static const char *json_template = R"###({
    "traceId": "%016llx%016llx",
    "name": "test",
    "id": "%016llx",
    "parentId": "%016llx",
    "annotations": [
        {
            "endpoint": {
                "serviceName": "host",
                "ipv4": "127.0.0.1",
                "port": 80
            },
            "timestamp": %lld,
            "value": "cs"
        }
    ],
    "binaryAnnotations": [
        {
            "endpoint": {
                "serviceName": "host",
                "ipv4": "127.0.0.1",
                "port": 80
            },
            "key": "bool",
            "value": true
        },
        {
            "key": "i16",
            "value": 123,
            "type": "I16"
        },
        {
            "key": "i32",
            "value": 123,
            "type": "I32"
        },
        {
            "key": "i64",
            "value": 123,
            "type": "I64"
        },
        {
            "key": "double",
            "value": 12.3,
            "type": "DOUBLE"
        },
        {
            "key": "string",
            "value": "测试"
        },
        {
            "key": "bytes",
            "value": "AQID",
            "type": "BYTES"
        }
    ],
    "debug": false,
    "timestamp": %lld
})###";

TEST(span, serialize_json)
{
    MockTracer tracer;

    zipkin::Span span(&tracer, "test", zipkin::Span::next_id());

    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(80);
    zipkin::Endpoint host("host", &addr);

    span.client_send(&host);
    span.annotate("bool", true, &host);
    span.annotate("i16", (int16_t)123);
    span.annotate("i32", (int32_t)123);
    span.annotate("i64", (int64_t)123);
    span.annotate("double", 12.3);
    span.annotate("string", std::wstring(L"测试"));

    uint8_t bytes[] = {1, 2, 3};

    span.annotate("bytes", bytes);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

    span.serialize_json(writer);

    char str[2048] = {0};
    int str_len = snprintf(str, sizeof(str), json_template, span.trace_id_high(), span.trace_id(), span.id(), span.parent_id(),
                           span.message().annotations[0].timestamp, span.message().timestamp);

    ASSERT_EQ(std::string(buffer.GetString(), buffer.GetSize()), std::string(str, str_len));
}

TEST(span, scope)
{
    MockTracer tracer;

    zipkin::Span span(&tracer, "test");

    EXPECT_CALL(tracer, submit(&span))
        .Times(1);

    zipkin::Span::Scope scope(span);
}

TEST(span, scope_cancel)
{
    MockTracer tracer;

    zipkin::Span *span = new zipkin::Span(&tracer, "test");

    zipkin::Span::Scope scope(*span);

    scope.cancel();
}

TEST(span, annotate_stream)
{
    MockTracer tracer;

    zipkin::Span span(&tracer, "test");

    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 80;
    zipkin::Endpoint host("host", &addr);

    span << zipkin::TraceKeys::CLIENT_SEND
         << std::string("hello") << host
         << std::make_pair("key", "hello")
         << std::make_pair("string", L"测试") << host
         << std::make_pair("key", std::string("world")) << host
         << std::make_pair("bool", true) << host
         << std::make_pair("i16", (int16_t)123) << host
         << std::make_pair("i32", (int32_t)123) << host
         << std::make_pair("i64", (int64_t)123) << host
         << std::make_pair("double", (double)12.3) << host;

    ASSERT_EQ(span.message().annotations.size(), 2);
    ASSERT_EQ(span.message().binary_annotations.size(), 8);
}