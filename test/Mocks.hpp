#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
using namespace testing;

#include "Span.h"
#include "Tracer.h"
#include "Collector.h"
#include "KafkaCollector.h"
#ifdef WITH_CURL
#include "HttpCollector.h"
#endif

class MockTracer : public zipkin::Tracer
{
public:
  MOCK_CONST_METHOD0(sample_rate, size_t(void));

  MOCK_METHOD1(set_sample_rate, void(size_t sample_rate));

  MOCK_CONST_METHOD0(userdata, userdata_t(void));

  MOCK_METHOD1(set_userdata, void(userdata_t userdata));

  MOCK_CONST_METHOD0(collector, zipkin::Collector *(void));

  MOCK_METHOD3(span, zipkin::Span *(const std::string &, span_id_t, userdata_t));

  MOCK_METHOD1(submit, void(zipkin::Span *));

  MOCK_METHOD1(release, void(zipkin::Span *));
};

class MockCollector : public zipkin::Collector
{
public:
  MOCK_CONST_METHOD0(name, const char *(void));

  MOCK_METHOD1(submit, void(zipkin::Span *span));

  MOCK_METHOD1(flush, bool(std::chrono::milliseconds timeout_ms));

  MOCK_METHOD1(shutdown, void(std::chrono::milliseconds timeout_ms));
};

class MockProducer : public RdKafka::Producer
{
public:
  // Mock RdKafka::Handle
  MOCK_CONST_METHOD0(name, const std::string());

  MOCK_CONST_METHOD0(memberid, const std::string());

  MOCK_METHOD1(poll, int(int timeout_ms));

  MOCK_METHOD0(outq_len, int());

  MOCK_METHOD4(metadata, RdKafka::ErrorCode(bool all_topics, const RdKafka::Topic *only_rkt,
                                            RdKafka::Metadata **metadatap, int timeout_ms));

  MOCK_METHOD1(pause, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &partitions));

  MOCK_METHOD1(resume, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &partitions));

  MOCK_METHOD5(query_watermark_offsets, RdKafka::ErrorCode(const std::string &topic,
                                                           int32_t partition,
                                                           int64_t *low, int64_t *high,
                                                           int timeout_ms));

  MOCK_METHOD4(get_watermark_offsets, RdKafka::ErrorCode(const std::string &topic,
                                                         int32_t partition,
                                                         int64_t *low, int64_t *high));

  MOCK_METHOD2(offsetsForTimes, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &offsets, int timeout_ms));

  MOCK_METHOD1(get_partition_queue, RdKafka::Queue *(const RdKafka::TopicPartition *partition));

  MOCK_METHOD1(set_log_queue, RdKafka::ErrorCode(RdKafka::Queue *queue));

  // Mock RdKafka::Producer
  MOCK_METHOD7(produce,
               RdKafka::ErrorCode(RdKafka::Topic *topic, int32_t partition,
                                  int msgflags,
                                  void *payload, size_t len,
                                  const std::string *key,
                                  void *msg_opaque));

  MOCK_METHOD8(produce, RdKafka::ErrorCode(RdKafka::Topic *topic, int32_t partition,
                                           int msgflags,
                                           void *payload, size_t len,
                                           const void *key, size_t key_len,
                                           void *msg_opaque));

  MOCK_METHOD9(produce, RdKafka::ErrorCode(const std::string topic_name, int32_t partition,
                                           int msgflags,
                                           void *payload, size_t len,
                                           const void *key, size_t key_len,
                                           int64_t timestamp,
                                           void *msg_opaque));

  MOCK_METHOD5(produce, RdKafka::ErrorCode(RdKafka::Topic *topic, int32_t partition,
                                           const std::vector<char> *payload,
                                           const std::vector<char> *key,
                                           void *msg_opaque));

  MOCK_METHOD1(flush, RdKafka::ErrorCode(int timeout_ms));
};

class MockTopic : public RdKafka::Topic
{
public:
  MOCK_CONST_METHOD0(name, const std::string());

  MOCK_CONST_METHOD1(partition_available, bool(int32_t partition));

  MOCK_METHOD2(offset_store, RdKafka::ErrorCode(int32_t partition, int64_t offset));
};