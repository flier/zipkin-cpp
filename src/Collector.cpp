#include "Collector.h"

#include <thrift/protocol/TBinaryProtocol.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

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

} // namespace zipkin