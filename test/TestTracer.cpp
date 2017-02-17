#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Collector.h"
#include "Tracer.h"

class MockCollector : public zipkin::Collector
{
  public:
    MOCK_METHOD1(submit, void(zipkin::Span *span));
};

TEST(tracer, properties)
{
    std::unique_ptr<zipkin::Tracer> tracer(zipkin::Tracer::create(nullptr, "test"));

    ASSERT_TRUE(tracer);
    ASSERT_NE(tracer->id(), 0);
    ASSERT_EQ(tracer->name(), "test");

    auto t = static_cast<zipkin::CachedTracer *>(tracer.get());

    ASSERT_EQ(t->cache().message_size(), zipkin::CachedTracer::default_cache_message_size);
}

TEST(tracer, span)
{
    MockCollector collector;

    std::unique_ptr<zipkin::Tracer> tracer(zipkin::Tracer::create(&collector, "test"));
    std::unique_ptr<zipkin::Span> span(tracer->span("test"));

    ASSERT_TRUE(span);
    ASSERT_NE(span->id(), 0);
    ASSERT_EQ(span->name(), "test");
    ASSERT_EQ(span->tracer(), tracer.get());

    EXPECT_CALL(collector, submit(span.get())).Times(1);

    tracer->submit(span.get());
}

TEST(tracer, cache)
{
    std::unique_ptr<zipkin::Tracer> tracer(zipkin::Tracer::create(nullptr, "test"));
    std::unique_ptr<zipkin::Span> span(tracer->span("test"));

    auto t = static_cast<zipkin::CachedTracer *>(tracer.get());

    ASSERT_TRUE(t->cache().empty());

    auto id = span->id();
    auto s = span.release();

    s->release();

    ASSERT_FALSE(t->cache().empty());

    span.reset(tracer->span("test2"));

    ASSERT_EQ(span.get(), s);
    ASSERT_TRUE(t->cache().empty());
    ASSERT_NE(span->id(), id);
    ASSERT_EQ(span->name(), "test2");
}
