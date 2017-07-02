#include "Collector.h"

#include <thrift/protocol/TBinaryProtocol.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <folly/Uri.h>

#include "KafkaCollector.h"
#ifdef WITH_CURL
#include "HttpCollector.h"
#endif
#include "ScribeCollector.h"
#include "XRayCollector.h"

namespace zipkin
{

std::shared_ptr<BinaryCodec> MessageCodec::binary(new BinaryCodec());
std::shared_ptr<JsonCodec> MessageCodec::json(new JsonCodec());
std::shared_ptr<PrettyJsonCodec> MessageCodec::pretty_json(new PrettyJsonCodec());

CompressionCodec parse_compression_codec(const std::string &codec)
{
    if (codec == "gzip")
        return CompressionCodec::gzip;
    if (codec == "snappy")
        return CompressionCodec::snappy;
    if (codec == "lz4")
        return CompressionCodec::lz4;

    return CompressionCodec::none;
}

const std::string to_string(CompressionCodec codec)
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

std::shared_ptr<MessageCodec> MessageCodec::parse(const std::string &codec)
{
    if (codec == "binary")
        return binary;
    if (codec == "json")
        return json;
    if (codec == "pretty_json")
        return pretty_json;

    return nullptr;
}

size_t BinaryCodec::encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans)
{
    apache::thrift::protocol::TBinaryProtocol protocol(buf);

    size_t wrote = protocol.writeListBegin(apache::thrift::protocol::T_STRUCT, spans.size());

    for (auto &span : spans)
    {
        wrote += span->serialize_binary(protocol);
    }

    return wrote + protocol.writeListEnd();
}

size_t JsonCodec::encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans)
{
    rapidjson::StringBuffer buffer;

    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartArray();

    for (auto &span : spans)
    {
        span->serialize_json(writer);
    }

    writer.EndArray();

    buf->write((const uint8_t *)buffer.GetString(), buffer.GetSize());

    return buffer.GetSize();
}

size_t PrettyJsonCodec::encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans)
{
    rapidjson::StringBuffer buffer;

    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

    writer.StartArray();

    for (auto &span : spans)
    {
        span->serialize_json(writer);
    }

    writer.EndArray();

    buf->write((const uint8_t *)buffer.GetString(), buffer.GetSize());

    return buffer.GetSize();
}

Collector *Collector::create(const std::string &uri)
{
    folly::Uri u(uri);

    if (u.scheme() == "kafka")
    {
        KafkaConf *conf = new KafkaConf(u);

        return conf->create();
    }

#ifdef WITH_CURL
    if (u.scheme() == "http" || u.scheme() == "https")
    {
        HttpConf *conf = new HttpConf(u);

        return conf->create();
    }
#endif

    if (u.scheme() == "scribe" || u.scheme() == "thrift")
    {
        ScribeConf *conf = new ScribeConf(u);

        return conf->create();
    }

    if (u.scheme() == "xray")
    {
        XRayConf *conf = new XRayConf(u);

        return conf->create();
    }

    return nullptr;
}

void BaseCollector::submit(Span *span)
{
    while (m_queued_spans >= m_conf->backlog)
    {
        if (drop_front_span())
            m_queued_spans--;
    }

    if (m_spans.push(span))
        m_queued_spans++;

    if (m_queued_spans >= m_conf->batch_size)
        flush(std::chrono::milliseconds(0));
}

bool BaseCollector::drop_front_span()
{
    CachedSpan *span = nullptr;

    if (m_spans.pop(span) && span)
    {
        LOG(WARNING) << "Drop Span `" << std::hex << span->id() << " exceed backlog";

        span->release();

        return true;
    }

    return false;
}

bool BaseCollector::flush(std::chrono::milliseconds timeout_ms)
{
    std::unique_lock<std::mutex> lock(m_sending);

    m_flush.notify_one();

    if (m_terminated)
    {
        VLOG(3) << "shutdown " << name() << " collector and wait " << timeout_ms.count() << " ms";
    }
    else if (m_spans.empty())
    {
        VLOG(3) << "no pendding spans to flush";
    }
    else
    {
        VLOG(3) << "flush pendding " << m_queued_spans << " spans and wait " << timeout_ms.count() << " ms";
    }

    return std::cv_status::no_timeout == m_sent.wait_for(lock, timeout_ms) && m_spans.empty();
}

void BaseCollector::shutdown(std::chrono::milliseconds timeout_ms)
{
    if (m_terminated.exchange(true)) return;

    if (!flush(timeout_ms) && m_worker.joinable())
    {
        VLOG(3) << "join thread " << m_worker.get_id();

        m_worker.join();
    }

    m_worker.detach();
}

void BaseCollector::run(BaseCollector *collector)
{
    LOG(INFO) << collector->name() << " collector started";

    do
    {
        collector->try_send_spans();

        if (collector->m_terminated)
        {
            VLOG(3) << "collector thread " << std::this_thread::get_id() << " terminated";

            break;
        }

        std::this_thread::yield();
    } while (!collector->m_terminated);
}

void BaseCollector::try_send_spans(void)
{
    std::unique_lock<std::mutex> lock(m_sending);

    if (m_flush.wait_for(lock, m_conf->batch_interval, [this] { return m_terminated || !m_spans.empty(); }))
    {
        if (!m_spans.empty())
        {
            send_spans();
        }
        else
        {
            VLOG(1) << name() << " collector " << (m_terminated ? "terminated" : "flushed");
        }

        m_sent.notify_all();
    }
}

void BaseCollector::send_spans(void)
{
    VLOG(2) << "sending " << m_queued_spans << " spans";

    std::vector<Span *> spans;

    m_queued_spans -= m_spans.consume_all([&spans](Span *span) {
        spans.push_back(span);
    });

    if (!spans.empty())
    {
        boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf(new apache::thrift::transport::TMemoryBuffer());

        VLOG(1) << "encode " << spans.size() << " spans with `" << m_conf->message_codec->name() << "` codec";

        m_conf->message_codec->encode(buf, spans);

        for (auto span : spans)
        {
            span->release();
        }

        uint8_t *msg = nullptr;
        uint32_t size = 0;

        buf->getBuffer(&msg, &size);

        send_message(msg, size);
    }
}

} // namespace zipkin
