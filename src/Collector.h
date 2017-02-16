#pragma once

#include <string>
#include <chrono>

#include "Span.h"

namespace zipkin
{

struct KafkaConf
{
  // metadata.broker.list - Initial list of brokers.
  std::string brokers;

  // the topic to produce to
  std::string topic_name;

  // the configured partitioner function will be used to select a target partition.
  static const int PARTITION_UA = -1;

  // the partition to produce to.
  //
  // default: PARTITION_UA (UnAssigned)
  int partition;

  // compression.codec - compression codec to use for compressing message sets.
  //
  // default: none
  std::string compression_codec;

  // batch.num.messages - the minimum number of messages to wait for to accumulate in the local queue before sending off a message set.
  //
  // default: 10000
  size_t batch_num_messages;

  // queue.buffering.max.ms - how long to wait for batch.num.messages to fill up in the local queue.
  //
  // default: 1000ms
  std::chrono::milliseconds queue_buffering_max;

  KafkaConf(const std::string &_brokers, const std::string &_topic_name, int _partition = PARTITION_UA)
      : brokers(_brokers), topic_name(_topic_name), partition(_partition), batch_num_messages(1000), queue_buffering_max(1000)
  {
  }
};

struct Collector
{
  virtual ~Collector() = default;

  virtual void submit(const Span *span) = 0;

  // Create Kafka Collector

  static Collector *create(const KafkaConf &conf);
};

} // namespace zipkin