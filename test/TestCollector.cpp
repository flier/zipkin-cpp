#include "Mocks.hpp"

TEST(collector, submit)
{
    std::unique_ptr<RdKafka::Producer> producer(new MockProducer());
    std::unique_ptr<RdKafka::Topic> topic(new MockTopic());

    MockProducer *p = static_cast<MockProducer *>(producer.get());
    MockTopic *t = static_cast<MockTopic *>(topic.get());

    zipkin::KafkaCollector collector(producer, topic);

    std::unique_ptr<zipkin::Tracer> tracer(zipkin::Tracer::create(&collector));

    auto span = static_cast<zipkin::CachedSpan *>(tracer->span("test"));

    EXPECT_CALL(*p, produce(collector.topic(),            // topic
                            RdKafka::Topic::PARTITION_UA, // partition
                            0,                            // msgflags
                            span->cache_ptr(),            // payload
                            81,                           // len
                            &span->name(),                // key
                            span))                        // msg_opaque
        .Times(1)
        .WillOnce(Return(RdKafka::ErrorCode::ERR_NO_ERROR));

    EXPECT_CALL(*p, poll(0))
        .Times(1)
        .WillOnce(Return(RdKafka::ErrorCode::ERR_NO_ERROR));

    EXPECT_CALL(*p, flush(_))
        .Times(1)
        .WillOnce(Return(RdKafka::ErrorCode::ERR_NO_ERROR));

    collector.submit(span);

    collector.shutdown(std::chrono::milliseconds(0));
}