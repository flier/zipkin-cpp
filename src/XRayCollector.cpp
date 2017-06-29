#include "XRayCollector.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

namespace zipkin
{

std::shared_ptr<XRayCodec> XRayConf::xray(new XRayCodec());

size_t XRayCodec::encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans)
{
    rapidjson::StringBuffer buffer;

    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key("format");
    writer.String("json");

    writer.Key("version");
    writer.Int(1);

    writer.EndObject();

    for (auto &span : spans)
    {
        char str[64];

        buffer.Put('\n');

        writer.StartObject();

        writer.Key("name");
        writer.String(span->name());

        writer.Key("id");
        writer.String(str, snprintf(str, sizeof(str), SPAN_ID_FMT, span->id()));

        writer.Key("trace_id");
        writer.String(str, snprintf(str, sizeof(str), "1-%08x-%08x" SPAN_ID_FMT,
                                    static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(span->timestamp()).count()),
                                    static_cast<uint32_t>(span->trace_id_high()), span->id()));

        std::chrono::duration<double, std::milli> start_time = span->timestamp();
        writer.Key("start_time");
        writer.Double(start_time.count());

        if (span->duration().count() == 0)
        {
            writer.Key("in_progress");
            writer.Bool(true);
        }
        else
        {
            std::chrono::duration<double, std::milli> end_time = span->timestamp() + span->duration();
            writer.Key("end_time");
            writer.Double(end_time.count());
        }

        if (span->parent_id())
        {
            writer.Key("type");
            writer.String("subsegment");

            writer.Key("parent_id");
            writer.String(str, snprintf(str, sizeof(str), SPAN_ID_FMT, span->parent_id()));
        }

        writer.EndObject();
    }

    buf->write((const uint8_t *)buffer.GetString(), buffer.GetSize());

    return buffer.GetSize();
}

XRayConf::XRayConf(folly::Uri &uri)
{
    host = folly::toStdString(uri.host());

    if (uri.port())
        port = uri.port();

    for (auto &param : uri.getQueryParams())
    {
        if (param.first == "format")
        {
            message_codec = MessageCodec::parse(folly::toStdString(param.second));
        }
        else if (param.first == "batch_size")
        {
            batch_size = folly::to<size_t>(param.second);
        }
        else if (param.first == "backlog")
        {
            backlog = folly::to<size_t>(param.second);
        }
        else if (param.first == "batch_interval")
        {
            batch_interval = std::chrono::milliseconds(folly::to<size_t>(param.second));
        }
    }

    message_codec = XRayConf::xray;
}

XRayCollector *XRayConf::create(void) const
{
    return new XRayCollector(this);
}

} // namespace zipkin
