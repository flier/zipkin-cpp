#include "Collector.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <functional>

#include <glog/logging.h>

#include <boost/smart_ptr.hpp>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include <librdkafka/rdkafkacpp.h>

#include "../gen-cpp/zipkinCore_types.h"

#include "Tracer.h"

namespace zipkin
{

class HashPartitioner : public RdKafka::PartitionerCb
{
    std::hash<std::string> hasher;

  public:
    // Implement RdKafka::PartitionerCb
    virtual int32_t partitioner_cb(const RdKafka::Topic *topic,
                                   const std::string *key,
                                   int32_t partition_cnt,
                                   void *msg_opaque) override
    {
        return hasher(*key) % partition_cnt;
    }
};

class KafkaCollector : public Collector
{
    std::unique_ptr<RdKafka::Producer> m_producer;
    std::unique_ptr<RdKafka::Topic> m_topic;
    std::unique_ptr<RdKafka::PartitionerCb> m_partitioner;
    int m_partition;

  public:
    KafkaCollector(std::unique_ptr<RdKafka::Producer> &producer,
                   std::unique_ptr<RdKafka::Topic> &topic,
                   std::unique_ptr<RdKafka::PartitionerCb> &partitioner,
                   int partition)
        : m_producer(std::move(producer)), m_topic(std::move(topic)), m_partitioner(std::move(partitioner)), m_partition(partition)
    {
    }
    virtual ~KafkaCollector() override
    {
        m_producer->flush(200);
    }

    // Implement Collector
    virtual void submit(const Span *span) override;
};

void KafkaCollector::submit(const Span *span)
{
    boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf(new apache::thrift::transport::TMemoryBuffer());
    boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> protocol(new apache::thrift::protocol::TBinaryProtocol(buf));

    size_t size = span->write(protocol.get());
    uint8_t *ptr = nullptr;
    uint32_t len = 0;

    buf->getBuffer(&ptr, &len);

    RdKafka::ErrorCode err = m_producer->produce(m_topic.get(),
                                                 m_partition,
                                                 RdKafka::Producer::RK_MSG_COPY, // msgflags
                                                 (void *)ptr, len,               // payload
                                                 &span->name,                    // key
                                                 this);                          // msg_opaque

    if (RdKafka::ErrorCode::ERR_NO_ERROR != err)
    {
        LOG(WARNING) << "fail to submit message to Kafka, " << err2str(err);
    }
}

const std::string KafkaConf::to_string(CompressionCodec codec)
{
    switch (codec)
    {
    case CompressionCodec::none:
        return "none";
    case CompressionCodec::gzip:
        return "gzip";
    case CompressionCodec::snappy:
        return "snappy";
    case CompressionCodec::lz4:
        return "lz4";
    }
}

bool kafka_conf_set(std::unique_ptr<RdKafka::Conf> &conf, const std::string &name, const std::string &value)
{
    std::string errstr;

    bool ok = RdKafka::Conf::CONF_OK == conf->set(name, value, errstr);

    if (!ok)
    {
        LOG(ERROR) << "fail to set " << name << " to " << value << ", " << errstr;
    }

    return ok;
}

Collector *KafkaConf::create(void) const
{
    std::string errstr;

    std::unique_ptr<RdKafka::Conf> producer_conf(RdKafka::Conf::create(RdKafka::Conf::ConfType::CONF_GLOBAL));
    std::unique_ptr<RdKafka::Conf> topic_conf(RdKafka::Conf::create(RdKafka::Conf::ConfType::CONF_TOPIC));
    std::unique_ptr<RdKafka::PartitionerCb> partitioner;

    if (!kafka_conf_set(producer_conf, "metadata.broker.list", brokers))
        return nullptr;

    if (compression_codec != CompressionCodec::none && !kafka_conf_set(producer_conf, "compression.codec", to_string(compression_codec)))
        return nullptr;

    if (batch_num_messages && !kafka_conf_set(producer_conf, "batch.num.messages", std::to_string(batch_num_messages)))
        return nullptr;

    if (queue_buffering_max_messages && !kafka_conf_set(producer_conf, "queue.buffering.max.messages", std::to_string(queue_buffering_max_messages)))
        return nullptr;

    if (queue_buffering_max_kbytes && !kafka_conf_set(producer_conf, "queue.buffering.max.kbytes", std::to_string(queue_buffering_max_kbytes)))
        return nullptr;

    if (queue_buffering_max_ms.count() && !kafka_conf_set(producer_conf, "queue.buffering.max.ms", std::to_string(queue_buffering_max_ms.count())))
        return nullptr;

    if (message_send_max_retries && !kafka_conf_set(producer_conf, "message.send.max.retries", std::to_string(message_send_max_retries)))
        return nullptr;

    if (partition == RdKafka::Topic::PARTITION_UA)
    {
        partitioner.reset(new HashPartitioner());

        if (RdKafka::Conf::CONF_OK != topic_conf->set("partitioner_cb", partitioner.get(), errstr))
        {
            LOG(ERROR) << "fail to set partitioner, " << errstr;
            return nullptr;
        }
    }

    if (VLOG_IS_ON(2))
    {
        VLOG(2) << "# Global config";

        std::unique_ptr<std::list<std::string>> producer_conf_items(producer_conf->dump());

        for (auto it = producer_conf_items->begin(); it != producer_conf_items->end();)
        {
            VLOG(2) << *it++ << " = " << *it++;
        }

        VLOG(2) << "# Topic config";

        std::unique_ptr<std::list<std::string>> topic_conf_items(topic_conf->dump());

        for (auto it = topic_conf_items->begin(); it != topic_conf_items->end();)
        {
            VLOG(2) << *it++ << " = " << *it++;
        }
    }

    std::unique_ptr<RdKafka::Producer> producer(RdKafka::Producer::create(producer_conf.get(), errstr));

    if (!producer)
    {
        LOG(ERROR) << "fail to connect Kafka broker @ " << brokers << ", " << errstr;

        return nullptr;
    }

    std::unique_ptr<RdKafka::Topic> topic(RdKafka::Topic::create(producer.get(), topic_name, topic_conf.get(), errstr));

    if (!topic)
    {
        LOG(ERROR) << "fail to create topic `" << topic_name << "`, " << errstr;

        return nullptr;
    }

    return new KafkaCollector(producer, topic, partitioner, partition);
}

} // namespace zipkin