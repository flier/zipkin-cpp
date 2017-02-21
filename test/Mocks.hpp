#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
using namespace testing;

#include "Span.h"
#include "Collector.h"
#include "Tracer.h"

class MockTracer : public zipkin::Tracer
{
public:
  MOCK_CONST_METHOD0(id, trace_id_t(void));

  MOCK_CONST_METHOD0(name, const std::string &(void));

  MOCK_CONST_METHOD0(collector, zipkin::Collector *(void));

  MOCK_METHOD3(span, zipkin::Span *(const std::string &name, span_id_t parent_id, userdata_t userdata));

  MOCK_METHOD1(submit, void(zipkin::Span *span));

  MOCK_METHOD1(release, void(zipkin::Span *span));
};

class MockCollector : public zipkin::Collector
{
public:
  MOCK_METHOD1(submit, void(zipkin::Span *span));
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