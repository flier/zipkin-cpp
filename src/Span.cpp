#include <random>
#include <utility>

#include <ios>
#include <cstdlib>
#include <string>
#include <locale>
#include <codecvt>

#include <arpa/inet.h>

#include <glog/logging.h>

#include <thrift/protocol/TBinaryProtocol.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include "Span.h"
#include "Tracer.h"

namespace zipkin
{

const ::Endpoint Endpoint::host() const
{
    ::Endpoint host;

    host.__set_ipv4(addr.sin_addr.s_addr);
    host.__set_port(addr.sin_port);
    host.__set_service_name(service);

    return host;
}

Span::Span(Tracer *tracer, const std::string &name, span_id_t parent_id, userdata_t userdata) : m_tracer(tracer)
{
    if (tracer)
        m_span.__set_trace_id(tracer->id());

    reset(name, parent_id, userdata);
}

void Span::reset(const std::string &name, span_id_t parent_id, userdata_t userdata)
{
    m_span.__set_name(name);
    m_span.__set_id(next_id());
    m_span.__set_timestamp(now());
    m_span.annotations.clear();
    m_span.binary_annotations.clear();

    if (parent_id)
    {
        m_span.__set_parent_id(parent_id);
    }

    m_userdata = userdata;
}

void Span::submit(void)
{
    if (m_span.timestamp)
        m_span.__set_duration(now() - m_span.timestamp);

    if (m_tracer)
        m_tracer->submit(this);
}

uint64_t Span::next_id()
{
    thread_local std::mt19937_64 rand_gen((std::chrono::system_clock::now().time_since_epoch().count() << 32) + std::random_device()());

    return rand_gen();
}

timestamp_t Span::now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void Span::annotate(const std::string &value, const Endpoint *endpoint)
{
    ::Annotation annotation;

    annotation.__set_timestamp(now());
    annotation.__set_value(value);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.annotations.push_back(annotation);
}

void Span::annotate(const std::string &key, const uint8_t *value, size_t size, const Endpoint *endpoint)
{
    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(std::string(reinterpret_cast<const char *>(value), size));
    annotation.__set_annotation_type(AnnotationType::type::BYTES);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);
}

void Span::annotate(const std::string &key, const std::string &value, const Endpoint *endpoint)
{
    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(value);
    annotation.__set_annotation_type(AnnotationType::type::STRING);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);
}

void Span::annotate(const std::string &key, const std::wstring &value, const Endpoint *endpoint)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    const std::string utf8 = converter.to_bytes(value);
    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(utf8);
    annotation.__set_annotation_type(AnnotationType::type::STRING);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);
}

size_t Span::serialize_binary(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf) const
{
    std::unique_ptr<apache::thrift::protocol::TBinaryProtocol> protocol(new apache::thrift::protocol::TBinaryProtocol(buf));

    return protocol->writeByte(12) + // type of the list elements: 12 == struct
           protocol->writeI32(1) +   // count of spans that will follow
           m_span.write(protocol.get());
}

size_t Span::serialize_json(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, bool pretty_print) const
{
    rapidjson::StringBuffer buffer;
    boost::shared_ptr<rapidjson::Writer<rapidjson::StringBuffer>> writer(
        pretty_print ? new rapidjson::PrettyWriter<rapidjson::StringBuffer>(buffer) : new rapidjson::Writer<rapidjson::StringBuffer>(buffer));

    auto serialize_endpoint = [writer](const ::Endpoint &host) {
        writer->StartObject();

        writer->Key("serviceName");
        writer->String(host.service_name);

        writer->Key("ipv4");
        writer->String(inet_ntoa({static_cast<in_addr_t>(host.ipv4)}));

        writer->Key("port");
        writer->Int(host.port);

        writer->EndObject();
    };

    auto serialize_value = [writer](const std::string &data, AnnotationType::type type) {
        switch (type)
        {
        case AnnotationType::type::BOOL:
            writer->Bool(*reinterpret_cast<const bool *>(data.c_str()));
            break;

        case AnnotationType::type::I16:
            writer->Int(*reinterpret_cast<const int16_t *>(data.c_str()));
            break;

        case AnnotationType::type::I32:
            writer->Int(*reinterpret_cast<const int32_t *>(data.c_str()));
            break;

        case AnnotationType::type::I64:
            writer->Int64(*reinterpret_cast<const int64_t *>(data.c_str()));
            break;

        case AnnotationType::type::DOUBLE:
            writer->Double(*reinterpret_cast<const double *>(data.c_str()));
            break;

        case AnnotationType::type::BYTES:
            writer->StartArray();

            for (auto c : data)
            {
                writer->Int(c);
            }

            writer->EndArray(data.size());

            break;

        case AnnotationType::type::STRING:
            writer->String(data);
            break;
        }
    };

    char str[64];

    writer->StartObject();

    writer->Key("traceId");
    writer->String(str, snprintf(str, sizeof(str), "%llx", m_span.trace_id));

    writer->Key("name");
    writer->String(m_span.name);

    writer->Key("id");
    writer->String(str, snprintf(str, sizeof(str), "%llx", m_span.id));

    if (m_span.__isset.parent_id)
    {
        writer->Key("parentId");
        writer->String(str, snprintf(str, sizeof(str), "%llx", m_span.parent_id));
    }

    writer->Key("annotations");
    writer->StartArray();

    for (auto &annotation : m_span.annotations)
    {
        writer->StartObject();

        if (annotation.__isset.host)
        {
            writer->Key("endpoint");
            serialize_endpoint(annotation.host);
        }

        writer->Key("timestamp");
        writer->Int64(annotation.timestamp);

        writer->Key("value");
        writer->String(annotation.value);

        writer->EndObject();
    }

    writer->EndArray(m_span.annotations.size());

    writer->Key("binary_annotations");
    writer->StartArray();

    for (auto &annotation : m_span.binary_annotations)
    {
        writer->StartObject();

        if (annotation.__isset.host)
        {
            writer->Key("endpoint");
            serialize_endpoint(annotation.host);
        }

        writer->Key("key");
        writer->String(annotation.key);

        writer->Key("value");
        serialize_value(annotation.value, annotation.annotation_type);

        writer->EndObject();
    }

    writer->EndArray(m_span.binary_annotations.size());

    if (m_span.__isset.debug)
    {
        writer->Key("debug");
        writer->Bool(m_span.debug);
    }

    if (m_span.__isset.timestamp)
    {
        writer->Key("timestamp");
        writer->Int64(m_span.timestamp);
    }

    if (m_span.__isset.duration)
    {
        writer->Key("duration");
        writer->Int64(m_span.duration);
    }

    writer->EndObject();

    buf->write((const uint8_t *)buffer.GetString(), buffer.GetSize());

    return buffer.GetSize();
}

size_t CachedSpan::cache_size(void) const
{
    return static_cast<CachedTracer *>(m_tracer)->cache().message_size() - cache_offset();
}

void *CachedSpan::operator new(size_t size, CachedTracer *tracer) noexcept
{
    size_t sz = tracer ? tracer->cache().message_size() : size;
    void *p = nullptr;

    if (posix_memalign(&p, CachedTracer::cache_line_size, sz))
        return nullptr;

    VLOG(2) << "Span @ " << p << " allocated with " << sz << " bytes";

    return ::operator new(sz, p);
}

void CachedSpan::operator delete(void *ptr, std::size_t sz) noexcept
{
    if (!ptr)
        return;

    CachedSpan *span = static_cast<CachedSpan *>(ptr);

    VLOG(2) << "Span @ " << ptr << " deleted with " << sz << " bytes, id=" << std::hex << span->id();

    free(ptr);
}

Span *CachedSpan::span(const std::string &name, userdata_t userdata) const
{
    if (m_tracer)
        return m_tracer->span(m_span.name, m_span.id, userdata ? userdata : m_userdata);

    return new (nullptr) CachedSpan(nullptr, m_span.name, m_span.id, userdata ? userdata : m_userdata);
}

void CachedSpan::release(void)
{
    if (m_tracer)
    {
        m_tracer->release(this);
    }
    else
    {
        delete this;
    }
}

} // namespace zipkin