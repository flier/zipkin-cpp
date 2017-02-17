#include "Mocks.hpp"

TEST(span, properties)
{
    MockTracer tracer;

    EXPECT_CALL(tracer, id())
        .Times(1)
        .WillOnce(Return(123));

    zipkin::Span span(&tracer, "test");

    ASSERT_NE(span.id(), 0);
    ASSERT_EQ(span.name(), "test");
    ASSERT_EQ(span.tracer(), &tracer);

    const ::Span &msg = span.message();

    ASSERT_EQ(msg.trace_id, 123);
    ASSERT_EQ(msg.name, "test");
    ASSERT_EQ(msg.id, span.id());
    ASSERT_EQ(msg.parent_id, 0);
    ASSERT_TRUE(msg.annotations.empty());
    ASSERT_TRUE(msg.binary_annotations.empty());
    ASSERT_FALSE(msg.debug);
    ASSERT_NE(msg.timestamp, 0);
    ASSERT_EQ(msg.duration, 0);
}

TEST(span, submit)
{
    MockTracer tracer;

    EXPECT_CALL(tracer, id())
        .Times(1)
        .WillOnce(Return(123));

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

    span.annotate("i16", (int16_t)123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I16);

    span.annotate("i32", (int32_t)123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I32);

    span.annotate("i64", (int64_t)123);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::I64);

    span.annotate("double", 12.3);
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::DOUBLE);

    span.annotate("string", std::wstring(L"测试"));
    ASSERT_EQ(msg.binary_annotations.back().value, "测试");
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::STRING);

    span.annotate("string", L"测试");
    ASSERT_EQ(msg.binary_annotations.back().value, "测试");
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::STRING);

    span.annotate("bytes", {1, 2, 3});
    ASSERT_EQ(msg.binary_annotations.back().annotation_type, AnnotationType::type::BYTES);
}