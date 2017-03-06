#include "Collector.h"

#include <thrift/protocol/TBinaryProtocol.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <folly/Uri.h>

#include "KafkaCollector.h"
#include "HttpCollector.h"
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
        return KafkaConf(u).create();

#ifdef WITH_CURL
    if (u.scheme() == "http" || u.scheme() == "https")
        return HttpConf(u).create();
#endif

    if (u.scheme() == "scribe" || u.scheme() == "thrift")
        return ScribeConf(u).create();

    if (u.scheme() == "xray")
        return XRayConf(u).create();

    return nullptr;
}

void BaseCollector::submit(Span *span)
{
    while (m_queued_spans >= conf().backlog)
    {
        if (drop_front_span())
            m_queued_spans--;
    }

    if (m_spans.push(span))
        m_queued_spans++;

    if (m_queued_spans >= conf().batch_size)
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
    VLOG(2) << "flush pendding " << m_queued_spans << " spans";

    if (m_spans.empty())
        return true;

    m_flush.notify_one();

    if (timeout_ms.count() == 0)
        return m_spans.empty();

    std::unique_lock<std::mutex> lock(m_flushing);

    return m_sent.wait_for(lock, timeout_ms, [this] { return m_spans.empty(); });
}

void BaseCollector::run(BaseCollector *collector)
{
    VLOG(1) << "HTTP collector started";

    do
    {
        collector->try_send_spans();

        std::this_thread::yield();
    } while (!collector->m_terminated);

    VLOG(1) << "HTTP collector stopped";
}

void BaseCollector::try_send_spans(void)
{
    std::unique_lock<std::mutex> lock(m_sending);

    if (m_flush.wait_for(lock, conf().batch_interval, [this] { return !m_spans.empty(); }))
    {
        send_spans();
    }
}

void BaseCollector::send_spans(void)
{
    VLOG(2) << "sending " << m_queued_spans << " spans";

    std::vector<Span *> spans;

    m_queued_spans -= m_spans.consume_all([&spans](Span *span) {
        spans.push_back(span);
    });

    boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf(new apache::thrift::transport::TMemoryBuffer());

    conf().message_codec->encode(buf, spans);

    for (auto span : spans)
    {
        span->release();
    }

    uint8_t *msg = nullptr;
    uint32_t size = 0;

    buf->getBuffer(&msg, &size);

    send_message(msg, size);

    m_sent.notify_all();
}

} // namespace zipkin