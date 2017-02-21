#pragma once

#include <string>
#include <chrono>
using namespace std::chrono_literals;

#include <librdkafka/rdkafkacpp.h>

#include "Span.h"

namespace zipkin
{

struct Collector
{
  virtual ~Collector() = default;

  virtual void submit(Span *span) = 0;

  virtual bool flush(std::chrono::milliseconds timeout) = 0;
};

class KafkaCollector : public Collector
{
  std::unique_ptr<RdKafka::Producer> m_producer;
  std::unique_ptr<RdKafka::Topic> m_topic;
  std::unique_ptr<RdKafka::DeliveryReportCb> m_reporter;
  std::unique_ptr<RdKafka::PartitionerCb> m_partitioner;
  int m_partition;

public:
  KafkaCollector(std::unique_ptr<RdKafka::Producer> &producer,
                 std::unique_ptr<RdKafka::Topic> &topic,
                 std::unique_ptr<RdKafka::DeliveryReportCb> reporter = nullptr,
                 std::unique_ptr<RdKafka::PartitionerCb> partitioner = nullptr,
                 int partition = RdKafka::Topic::PARTITION_UA)
      : m_producer(std::move(producer)), m_topic(std::move(topic)),
        m_reporter(std::move(reporter)), m_partitioner(std::move(partitioner)), m_partition(partition)
  {
  }
  virtual ~KafkaCollector() override
  {
    flush();
  }

  RdKafka::Producer *producer(void) const { return m_producer.get(); }
  RdKafka::Topic *topic(void) const { return m_topic.get(); }

  // Implement Collector
  virtual void submit(Span *span) override;

  virtual bool flush(std::chrono::milliseconds timeout = 500ms) override
  {
    return RdKafka::ERR_NO_ERROR == m_producer->flush(timeout.count());
  }
};

struct KafkaConf
{
  // metadata.broker.list - Initial list of brokers.
  std::string brokers;

  // the topic to produce to
  std::string topic_name;

  // the partition to produce to.
  //
  // default: PARTITION_UA (UnAssigned)
  int partition;

  enum CompressionCodec
  {
    none,
    gzip,
    snappy,
    lz4
  };

  static const std::string to_string(CompressionCodec codec);

  // compression.codec - compression codec to use for compressing message sets.
  //
  // default: none
  CompressionCodec compression_codec;

  // batch.num.messages - the minimum number of messages to wait for to accumulate in the local queue before sending off a message set.
  //
  // default: 10000
  size_t batch_num_messages;

  // queue.buffering.max.messages - Maximum number of messages allowed on the producer queue.
  //
  // default: 100000
  size_t queue_buffering_max_messages;

  // queue.buffering.max.kbytes - Maximum total message size sum allowed on the producer queue.
  //
  // default: 4000000
  size_t queue_buffering_max_kbytes;

  // queue.buffering.max.ms - how long to wait for batch.num.messages to fill up in the local queue.
  //
  // default: 1000ms
  std::chrono::milliseconds queue_buffering_max_ms;

  // message.send.max.retries - How many times to retry sending a failing MessageSet. Note: retrying may cause reordering.
  //
  // default: 2
  size_t message_send_max_retries;

  KafkaConf(const std::string &_brokers, const std::string &_topic_name, int _partition = RdKafka::Topic::PARTITION_UA)
      : brokers(_brokers), topic_name(_topic_name), partition(_partition), compression_codec(none), batch_num_messages(0),
        queue_buffering_max_messages(0), queue_buffering_max_kbytes(0), queue_buffering_max_ms(0), message_send_max_retries(0)
  {
  }

  KafkaCollector *create(void) const;
};

} // namespace zipkin