#include "Mocks.hpp"

TEST(tracer, properties)
{
    std::unique_ptr<zipkin::Tracer> tracer(zipkin::Tracer::create(nullptr));

    ASSERT_TRUE(tracer);
    ASSERT_EQ(tracer->sample_rate(), 1);

    tracer->set_sample_rate(100);

    ASSERT_EQ(tracer->sample_rate(), 100);

    auto t = static_cast<zipkin::CachedTracer *>(tracer.get());

    ASSERT_EQ(t->cache().message_size(), zipkin::CachedTracer::DEFAULT_CACHE_MESSAGE_SIZE);
}

TEST(tracer, span)
{
    MockCollector collector;

    std::unique_ptr<zipkin::Tracer> tracer(zipkin::Tracer::create(&collector));
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
    std::unique_ptr<zipkin::Tracer> tracer(zipkin::Tracer::create(nullptr));
    std::unique_ptr<zipkin::Span> span(tracer->span("test"));

    auto t = static_cast<zipkin::CachedTracer *>(tracer.get());

    ASSERT_TRUE(t->cache().empty());

    auto id = span->id();
    auto s = span.release();

    tracer->release(s);

    ASSERT_FALSE(t->cache().empty());

    span.reset(tracer->span("test2"));

    ASSERT_EQ(span.get(), s);
    ASSERT_TRUE(t->cache().empty());
    ASSERT_NE(span->id(), id);
    ASSERT_EQ(span->name(), "test2");
}
