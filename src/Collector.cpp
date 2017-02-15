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

    // Implement Collector
    virtual void Submit(const Span &span) override;
};

Collector *Collector::create(const std::string &brokers, const std::string &topic_name, int partition, const char *codec)
{
    std::string errstr;

    std::unique_ptr<RdKafka::Conf> producer_conf(RdKafka::Conf::create(RdKafka::Conf::ConfType::CONF_GLOBAL));
    std::unique_ptr<RdKafka::Conf> topic_conf(RdKafka::Conf::create(RdKafka::Conf::ConfType::CONF_TOPIC));
    std::unique_ptr<RdKafka::PartitionerCb> partitioner;

    if (codec)
    {
        if (RdKafka::Conf::CONF_OK != producer_conf->set("compression.codec", codec, errstr))
        {
            LOG(ERROR) << "fail to set compression codec to `" << codec << "`, " << errstr;
            return nullptr;
        }
    }

    if (RdKafka::Conf::CONF_OK != producer_conf->set("metadata.broker.list", brokers, errstr))
    {
        LOG(ERROR) << "fail to set broker list to [" << brokers << "], " << errstr;
        return nullptr;
    }

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

void KafkaCollector::Submit(const Span &span)
{
    ::Span msg;

    msg.trace_id = span.tracer->id;
    msg.name = span.name;
    msg.id = span.id;

    if (span.parent)
    {
        msg.__set_parent_id(span.parent);
    }

    if (span.ts)
    {
        msg.__set_timestamp(span.ts);
        msg.__set_duration(timestamp() - span.ts);
    }

    boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf(new apache::thrift::transport::TMemoryBuffer());
    boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> protocol(new apache::thrift::protocol::TBinaryProtocol(buf));

    size_t size = msg.write(protocol.get());
    uint8_t *ptr = nullptr;
    uint32_t len = 0;

    buf->getBuffer(&ptr, &len);

    RdKafka::ErrorCode err = m_producer->produce(m_topic.get(),
                                                 m_partition,
                                                 RdKafka::Producer::RK_MSG_COPY, // msgflags
                                                 (void *)ptr, len,               // payload
                                                 &span.tracer->name,             // key
                                                 this);                          // msg_opaque

    if (RdKafka::ErrorCode::ERR_NO_ERROR != err)
    {
        LOG(WARNING) << "fail to submit message to Kafka, " << err2str(err);
    }
}

} // namespace zipkin