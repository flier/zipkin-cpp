#pragma once

#include <string>
#include <chrono>

#include "Span.h"

namespace zipkin
{

struct Collector
{
  virtual ~Collector() = default;

  virtual void submit(Span *span) = 0;
};

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

  KafkaConf(const std::string &_brokers, const std::string &_topic_name, int _partition = PARTITION_UA)
      : brokers(_brokers), topic_name(_topic_name), partition(_partition), compression_codec(none), batch_num_messages(0),
        queue_buffering_max_messages(0), queue_buffering_max_kbytes(0), queue_buffering_max_ms(0), message_send_max_retries(0)
  {
  }

  Collector *create(void) const;
};

} // namespace zipkin