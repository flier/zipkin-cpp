#pragma once

#include <librdkafka/rdkafkacpp.h>

#include "Collector.h"

namespace zipkin
{

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
    std::shared_ptr<MessageCodec> m_message_codec;

  public:
    KafkaCollector(std::unique_ptr<RdKafka::Producer> &producer,
                   std::unique_ptr<RdKafka::Topic> &topic,
                   std::unique_ptr<RdKafka::DeliveryReportCb> reporter = nullptr,
                   std::unique_ptr<RdKafka::PartitionerCb> partitioner = nullptr,
                   int partition = RdKafka::Topic::PARTITION_UA,
                   std::shared_ptr<MessageCodec> message_codec = MessageCodec::binary)
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
    std::shared_ptr<MessageCodec> message_codec;

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
        : initial_brokers(brokers), topic_name(name), topic_partition(partition), compression_codec(none), message_codec(MessageCodec::binary), batch_num_messages(0), queue_buffering_max_messages(0), queue_buffering_max_kbytes(0), queue_buffering_max_ms(0), message_send_max_retries(0)
    {
    }

    /**
    * \brief Create KafkaCollector base on the configuration
    */
    KafkaCollector *create(void) const;
};

} // namespace zipkin
