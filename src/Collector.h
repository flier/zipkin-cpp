#pragma once

#include <string>
#include <chrono>

#include <librdkafka/rdkafkacpp.h>

#include "Span.h"

namespace zipkin
{

/**
* \brief Push Span as messages to transport
*/
struct Collector
{
  virtual ~Collector() = default;

  /**
  * \brief Async submit Span to transport
  *
  * \param span encoded to message and send to the transport,
  *
  * Span#release will be call after the message was sent.
  */
  virtual void submit(Span *span) = 0;

  /**
  * \brief Wait until all outstanding messages, et.al, are sent.
  *
  * \param timeout_ms the maximum amount of time (in milliseconds) that the call will block waiting.
  * \return \c true if all outstanding messages were sent, or \c false if the \p timeout_ms was reached.
  */
  virtual bool flush(std::chrono::milliseconds timeout_ms) = 0;
};

/**
 * \brief use for compressing message sets.
 *
 * \sa https://cwiki.apache.org/confluence/display/KAFKA/Compression
 */
enum CompressionCodec
{
  none,   ///< No compression
  gzip,   ///< GZIP compression
  snappy, ///< Snappy compression
  lz4     ///< LZ4 compression
};

CompressionCodec parse_compression_codec(const std::string &codec);
const std::string to_string(CompressionCodec codec);

/**
* \brief use for encoding message sets.
*/
enum MessageCodec
{
  binary,      ///< Thrift binary encoding
  json,        ///< JSON encoding
  pretty_json, ///< Pretty print JSON encoding
};

MessageCodec parse_message_codec(const std::string &codec);
const std::string to_string(MessageCodec codec);

/**
 * \brief This collector push messages to a Kafka topic
 *
 * Those message was lists of spans as JSON encoded or TBinaryProtocol big-endian encoded.
 */
class KafkaCollector : public Collector
{
  std::unique_ptr<RdKafka::Producer> m_producer;
  std::unique_ptr<RdKafka::Topic> m_topic;
  std::unique_ptr<RdKafka::DeliveryReportCb> m_reporter;
  std::unique_ptr<RdKafka::PartitionerCb> m_partitioner;
  int m_partition;
  MessageCodec m_message_codec;

public:
  KafkaCollector(std::unique_ptr<RdKafka::Producer> &producer,
                 std::unique_ptr<RdKafka::Topic> &topic,
                 std::unique_ptr<RdKafka::DeliveryReportCb> reporter = nullptr,
                 std::unique_ptr<RdKafka::PartitionerCb> partitioner = nullptr,
                 int partition = RdKafka::Topic::PARTITION_UA,
                 MessageCodec message_codec = MessageCodec::binary)
      : m_producer(std::move(producer)), m_topic(std::move(topic)), m_reporter(std::move(reporter)),
        m_partitioner(std::move(partitioner)), m_partition(partition), m_message_codec(message_codec)
  {
  }
  virtual ~KafkaCollector() override
  {
    flush(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::microseconds(500)));
  }

  /**
  * \brief Kafka producer
  */
  RdKafka::Producer *producer(void) const { return m_producer.get(); }

  /**
  * \brief Kafka topic
  */
  RdKafka::Topic *topic(void) const { return m_topic.get(); }

  /**
  * \brief Kafka topic partition
  */
  int partition(void) const { return m_partition; }

  // Implement Collector

  virtual void submit(Span *span) override;

  virtual bool flush(std::chrono::milliseconds timeout_ms) override
  {
    return RdKafka::ERR_NO_ERROR == m_producer->flush(timeout_ms.count());
  }
};

/**
* \brief Configuration for KafkaCollector
*
* \sa https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md
*/
struct KafkaConf
{
  /**
  * \brief Initial list of brokers.
  */
  std::string initial_brokers;

  /**
  * \brief The topic to produce to
  */
  std::string topic_name;

  /**
  * \brief The partition to produce to.
  *
  * default: PARTITION_UA (UnAssigned)
  */
  int topic_partition;

  /**
  * \brief Compression codec to use for compressing message sets.
  *
  * default: none
  */
  CompressionCodec compression_codec;

  /**
  * \brief Message codec to use for encoding message sets.
  *
  * default: binary
  */
  MessageCodec message_codec;

  /**
  * \brief Minimum number of messages to wait for to accumulate in the local queue before sending off a message set.
  *
  * default: 10000
  */
  size_t batch_num_messages;

  /**
  * \brief Maximum number of messages allowed on the producer queue.
  *
  * default: 100000
  */
  size_t queue_buffering_max_messages;

  /**
  * \brief Maximum total message size sum allowed on the producer queue.
  *
  * default: 4000000
  */
  size_t queue_buffering_max_kbytes;

  /**
  * \brief How long to wait for batch.num.messages to fill up in the local queue.
  *
  * default: 1000ms
  */
  std::chrono::milliseconds queue_buffering_max_ms;

  /**
  * \brief How many times to retry sending a failing MessageSet. Note: retrying may cause reordering.
  *
  * default: 2
  */
  size_t message_send_max_retries;

  /**
  * \brief Construct a configuration for KafkaCollector
  *
  * \param brokers Initial list of brokers.
  * \param name The topic to produce to
  * \param partition The partition to produce to.
  */
  KafkaConf(const std::string &brokers, const std::string &name, int partition = RdKafka::Topic::PARTITION_UA)
      : initial_brokers(brokers), topic_name(name), topic_partition(partition), compression_codec(none), message_codec(binary), batch_num_messages(0), queue_buffering_max_messages(0), queue_buffering_max_kbytes(0), queue_buffering_max_ms(0), message_send_max_retries(0)
  {
  }

  /**
  * \brief Create KafkaCollector base on the configuration
  */
  KafkaCollector *create(void) const;
};

} // namespace zipkin