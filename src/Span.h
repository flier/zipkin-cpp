#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <locale>
#include <codecvt>
#include <memory>
#include <chrono>

#include <thrift/protocol/TProtocol.h>

#include "../gen-cpp/zipkinCore_constants.h"

typedef uint64_t span_id_t;
typedef uint64_t trace_id_t;
typedef std::chrono::microseconds timestamp_t;
typedef std::chrono::microseconds duration_t;
typedef void *userdata_t;

namespace zipkin
{

/**
 * Indicates the network context of a service recording an annotation with two exceptions.
 */
class Endpoint
{
    ::Endpoint m_host;

  public:
    Endpoint()
    {
    }
    Endpoint(const ::Endpoint &host) : m_host(host)
    {
    }
    Endpoint(const std::string &service)
    {
        m_host.__set_service_name(service);
    }
    Endpoint(const sockaddr_in &addr)
    {
        m_host.__set_ipv4(addr.sin_addr.s_addr);
        m_host.__set_port(addr.sin_port);
    }
    Endpoint(const std::string &service, const sockaddr_in &addr)
    {
        m_host.__set_service_name(service);
        m_host.__set_ipv4(addr.sin_addr.s_addr);
        m_host.__set_port(addr.sin_port);
    }

    /**
    * Classifier of a source or destination in lowercase, such as "zipkin-server".
    *
    * <p>This is the primary parameter for trace lookup, so should be intuitive as possible, for
    * example, matching names in service discovery.
    *
    * <p>Conventionally, when the service name isn't known, service_name = "unknown". However, it is
    * also permissible to set service_name = "" (empty string). The difference in the latter usage is
    * that the span will not be queryable by service name unless more information is added to the
    * span with non-empty service name, e.g. an additional annotation from the server.
    *
    * <p>Particularly clients may not have a reliable service name at ingest. One approach is to set
    * service_name to "" at ingest, and later assign a better label based on binary annotations, such
    * as user agent.
    */
    inline const std::string &service_name(void) const { return m_host.service_name; }
    inline Endpoint &with_service_name(const std::string &service_name)
    {
        m_host.service_name = service_name;
        return *this;
    }

    /**
    * IPv4 endpoint address
    */
    inline const sockaddr_in addr(void) const
    {
        sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = m_host.ipv4;
        addr.sin_port = m_host.port;

        return addr;
    }
    inline Endpoint &with_addr(const sockaddr_in &addr)
    {
        m_host.ipv4 = addr.sin_addr.s_addr;
        m_host.port = addr.sin_port;

        return *this;
    }

    inline Endpoint &with_ipv4(const std::string &ip)
    {
        m_host.ipv4 = inet_addr(ip.c_str());

        return *this;
    }
    inline Endpoint &with_port(uint16_t port)
    {
        m_host.port = port;

        return *this;
    }

    inline const ::Endpoint &host(void) const { return m_host; }
};

struct Tracer;

class CachedTracer;

class Annotation
{
    ::Annotation &m_annotation;

  public:
    Annotation(::Annotation &annotation) : m_annotation(annotation) {}

    timestamp_t timestamp(void) const { return timestamp_t(m_annotation.timestamp); }
    Annotation &with_timestamp(timestamp_t timestamp)
    {
        m_annotation.timestamp = timestamp.count();
        return *this;
    }

    const std::string &value(void) const { return m_annotation.value; }
    Annotation &with_value(const std::string &value)
    {
        m_annotation.value = value;
        return *this;
    }

    const Endpoint endpoint(void) const { return Endpoint(m_annotation.host); }
    Annotation &with_endpoint(const Endpoint &endpoint)
    {
        m_annotation.host = endpoint.host();
        return *this;
    }
};

using AnnotationType = ::AnnotationType::type;

class BinaryAnnotation
{
    ::BinaryAnnotation &m_annotation;

  public:
    BinaryAnnotation(::BinaryAnnotation &annotation) : m_annotation(annotation) {}

    AnnotationType type(void) const { return m_annotation.annotation_type; }

    const std::string &value(void) const { return m_annotation.value; }

    template <typename T>
    BinaryAnnotation &with_value(const T &value);

    inline BinaryAnnotation &with_value(const uint8_t *value, size_t size)
    {
        m_annotation.value = std::string(reinterpret_cast<const char *>(value), size);
        m_annotation.annotation_type = AnnotationType::BYTES;
        return *this;
    }
    template <size_t N>
    inline BinaryAnnotation &with_value(const uint8_t (&value)[N])
    {
        return with_value(value, N);
    }
    inline BinaryAnnotation &with_value(const std::vector<uint8_t> &value)
    {
        return with_value(value.data(), value.size());
    }
    inline BinaryAnnotation &with_value(const std::string &value)
    {
        m_annotation.value = value;
        m_annotation.annotation_type = AnnotationType::STRING;
        return *this;
    }
    inline BinaryAnnotation &with_value(const char *value, int len = -1)
    {
        return with_value(len >= 0 ? std::string(value, len) : std::string(value));
    }
    inline BinaryAnnotation &with_value(const std::wstring &value)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

        return with_value(converter.to_bytes(value));
    }
    inline BinaryAnnotation &with_value(const wchar_t *value, int len = -1)
    {
        return with_value(len >= 0 ? std::wstring(value, len) : std::wstring(value));
    }

    const Endpoint endpoint(void) const { return Endpoint(m_annotation.host); }
    BinaryAnnotation &with_endpoint(const Endpoint &endpoint)
    {
        m_annotation.host = endpoint.host();
        return *this;
    }
};

/**
* A trace is a series of spans (often RPC calls) which form a latency tree.
*
* <p>Spans are usually created by instrumentation in RPC clients or servers, but can also
* represent in-process activity. Annotations in spans are similar to log statements, and are
* sometimes created directly by application developers to indicate events of interest, such as a
* cache miss.
*
* <p>The root span is where {@link #parent_id} is 0; it usually has the longest {@link #duration} in the
* trace.
*
* <p>Span identifiers are packed into longs, but should be treated opaquely. ID encoding is
* 16 or 32 character lower-hex, to avoid signed interpretation.
*/
struct Span
{
  protected:
    Tracer *m_tracer;
    ::Span m_span;
    userdata_t m_userdata;

    static const ::Endpoint host(const Endpoint *endpoint);

  public:
    /**
     * Construct a span
     */
    Span(Tracer *tracer, const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr);

    /**
     * Reset a span
     */
    void reset(const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr);

    /**
     * Submit a span to {@link Tracer}
     */
    void submit(void);

    /**
     * Associated {@link Tracer}
     */
    Tracer *tracer(void) const { return m_tracer; }

    const ::Span &message(void) const { return m_span; }

    /**
     * Unique 8-byte identifier for a trace, set on all spans within it.
     */
    trace_id_t trace_id(void) const { return m_span.trace_id; }

    /** @see Span#trace_id */
    void set_trace_id(trace_id_t trace_id) { m_span.trace_id = trace_id; }

    /**
    * Unique 8-byte identifier of this span within a trace.
    *
    * <p>A span is uniquely identified in storage by ({@linkplain #trace_id}, {@code #id}).
    */
    span_id_t id(void) const { return m_span.id; }

    /** @see Span#id */
    void set_id(span_id_t id) { m_span.id = id; }

    /**
    * Span name in lowercase, rpc method for example.
    *
    * <p>Conventionally, when the span name isn't known, name = "unknown".
    */
    const std::string &name(void) const { return m_span.name; }

    /** @see Span#name */
    void set_name(const std::string &name) { m_span.name = name; }

    /**
    * The parent's {@link #id} or 0 if this the root span in a trace.
    */
    span_id_t parent_id(void) const { return m_span.parent_id; }

    /** @see Span#parent_id */
    void set_parent_id(span_id_t id) { m_span.parent_id = id; }

    /**
    * Epoch microseconds of the start of this span, possibly absent if this an incomplete span.
    */
    timestamp_t timestamp(void) const { return timestamp_t(m_span.timestamp); }

    /** @see Span#timestamp */
    void set_timestamp(timestamp_t timestamp) { m_span.timestamp = timestamp.count(); }

    /**
    * Measurement in microseconds of the critical path, if known. Durations of less than one
    * microsecond must be rounded up to 1 microsecond.
    */
    duration_t duration(void) const { return duration_t(m_span.duration); }

    /** @see Span#duration */
    void set_duration(duration_t duration) { m_span.duration = duration.count(); }

    userdata_t userdata(void) const { return m_userdata; }

    /** @see Span#userdata */
    void set_userdata(userdata_t userdata) { m_userdata = userdata; }

    virtual Span *span(const std::string &name, userdata_t userdata = nullptr) const
    {
        return new Span(m_tracer, name, m_span.id, userdata ? userdata : m_userdata);
    };

    static uint64_t next_id();

    static timestamp_t now();

    /**
    * Associates events that explain latency with a timestamp.
    */
    Annotation annotate(const std::string &value, const Endpoint *endpoint = nullptr);

    inline Annotation client_send(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.CLIENT_SEND, endpoint);
    }
    inline Annotation client_recv(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.CLIENT_RECV, endpoint);
    }
    inline Annotation server_send(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.SERVER_SEND, endpoint);
    }
    inline Annotation server_recv(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.SERVER_RECV, endpoint);
    }
    inline Annotation wire_send(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.WIRE_SEND, endpoint);
    }
    inline Annotation wire_recv(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.WIRE_RECV, endpoint);
    }
    inline Annotation client_send_fragment(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.CLIENT_SEND_FRAGMENT, endpoint);
    }
    inline Annotation client_recv_fragment(const Endpoint *endpoint = nullptr) { return annotate(g_zipkinCore_constants.CLIENT_RECV_FRAGMENT, endpoint); }
    inline Annotation server_send_fragment(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.SERVER_SEND_FRAGMENT, endpoint);
    }
    inline Annotation server_recv_fragment(const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.SERVER_RECV_FRAGMENT, endpoint);
    }

    /**
    * Tags a span with context, usually to support query or aggregation.
    */
    template <typename T>
    BinaryAnnotation annotate(const std::string &key, const T &value, const Endpoint *endpoint = nullptr);
    BinaryAnnotation annotate(const std::string &key, const uint8_t *value, size_t size, const Endpoint *endpoint = nullptr);

    template <size_t N>
    inline BinaryAnnotation annotate(const std::string &key, const uint8_t (&value)[N], const Endpoint *endpoint = nullptr)
    {
        return annotate(key, value, N, endpoint);
    }
    inline BinaryAnnotation annotate(const std::string &key, const std::vector<uint8_t> &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(key, value.data(), value.size(), endpoint);
    }

    BinaryAnnotation annotate(const std::string &key, const std::string &value, const Endpoint *endpoint = nullptr);

    inline BinaryAnnotation annotate(const std::string &key, const char *value, int len = -1, const Endpoint *endpoint = nullptr)
    {
        return annotate(key, len >= 0 ? std::string(value, len) : std::string(value), endpoint);
    }

    BinaryAnnotation annotate(const std::string &key, const std::wstring &value, const Endpoint *endpoint = nullptr);

    inline BinaryAnnotation annotate(const std::string &key, const wchar_t *value, int len = -1, const Endpoint *endpoint = nullptr)
    {
        return annotate(key, len >= 0 ? std::wstring(value, len) : std::wstring(value), endpoint);
    }

    inline BinaryAnnotation http_host(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_HOST, value, endpoint);
    }
    inline BinaryAnnotation http_method(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_METHOD, value, endpoint);
    }
    inline BinaryAnnotation http_path(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_PATH, value, endpoint);
    }
    inline BinaryAnnotation http_url(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_URL, value, endpoint);
    }
    inline BinaryAnnotation http_status_code(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_STATUS_CODE, value, endpoint);
    }
    inline BinaryAnnotation http_request_size(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_REQUEST_SIZE, value, endpoint);
    }
    inline BinaryAnnotation http_response_size(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_RESPONSE_SIZE, value, endpoint);
    }
    inline BinaryAnnotation local_component(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.LOCAL_COMPONENT, value, endpoint);
    }
    inline BinaryAnnotation client_addr(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.CLIENT_ADDR, value, endpoint);
    }
    inline BinaryAnnotation server_addr(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.SERVER_ADDR, value, endpoint);
    }

    size_t serialize_binary(apache::thrift::protocol::TProtocol &protocol) const
    {
        return m_span.write(&protocol);
    }

    template <class Writer>
    void serialize_json(Writer &writer) const;
};

class CachedSpan : public Span
{
    uint8_t m_buf[0] __attribute__((aligned));

  public:
    CachedSpan(Tracer *tracer, const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr)
        : Span(tracer, name, parent_id, userdata)
    {
    }

    static void *operator new(size_t size, CachedTracer *tracer) noexcept;
    static void operator delete(void *ptr, std::size_t sz) noexcept;

    static size_t cache_offset(void) { return offsetof(CachedSpan, m_buf); }

    uint8_t *cache_ptr(void) { return &m_buf[0]; }
    size_t cache_size(void) const;

    void release(void);

    virtual Span *span(const std::string &name, userdata_t userdata = nullptr) const override;

} __attribute__((aligned));

namespace __impl
{
template <typename T>
struct __annotation
{
};
template <>
struct __annotation<bool>
{
    static const AnnotationType type = AnnotationType::BOOL;
};
template <>
struct __annotation<int16_t>
{
    static const AnnotationType type = AnnotationType::I16;
};
template <>
struct __annotation<int32_t>
{
    static const AnnotationType type = AnnotationType::I32;
};
template <>
struct __annotation<int64_t>
{
    static const AnnotationType type = AnnotationType::I64;
};
template <>
struct __annotation<double>
{
    static const AnnotationType type = AnnotationType::DOUBLE;
};
} // namespace __impl

template <typename T>
BinaryAnnotation &BinaryAnnotation::with_value(const T &value)
{
    auto ptr = reinterpret_cast<uint8_t *>(&value);

    m_annotation.value = std::string(reinterpret_cast<char *>(ptr), sizeof(T));

    return *this;
}

template <typename T>
inline BinaryAnnotation Span::annotate(const std::string &key, const T &value, const Endpoint *endpoint)
{
    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(std::string(reinterpret_cast<const char *>(&value), sizeof(T)));
    annotation.__set_annotation_type(__impl::__annotation<T>::type);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);

    return BinaryAnnotation(m_span.binary_annotations.back());
}

template <class Writer>
void Span::serialize_json(Writer &writer) const
{
    auto serialize_endpoint = [&writer](const ::Endpoint &host) {
        writer.StartObject();

        writer.Key("serviceName");
        writer.String(host.service_name);

        writer.Key("ipv4");
        writer.String(inet_ntoa({static_cast<in_addr_t>(host.ipv4)}));

        writer.Key("port");
        writer.Int(host.port);

        writer.EndObject();
    };

    auto serialize_value = [&writer](const std::string &data, AnnotationType type) {
        switch (type)
        {
        case AnnotationType::BOOL:
            writer.Bool(*reinterpret_cast<const bool *>(data.c_str()));
            break;

        case AnnotationType::I16:
            writer.Int(*reinterpret_cast<const int16_t *>(data.c_str()));
            break;

        case AnnotationType::I32:
            writer.Int(*reinterpret_cast<const int32_t *>(data.c_str()));
            break;

        case AnnotationType::I64:
            writer.Int64(*reinterpret_cast<const int64_t *>(data.c_str()));
            break;

        case AnnotationType::DOUBLE:
            writer.Double(*reinterpret_cast<const double *>(data.c_str()));
            break;

        case AnnotationType::BYTES:
            writer.StartArray();

            for (auto c : data)
            {
                writer.Int(c);
            }

            writer.EndArray(data.size());

            break;

        case AnnotationType::STRING:
            writer.String(data);
            break;
        }
    };

    char str[64];

    writer.StartObject();

    writer.Key("traceId");
    writer.String(str, snprintf(str, sizeof(str), "%016llx", m_span.trace_id));

    writer.Key("name");
    writer.String(m_span.name);

    writer.Key("id");
    writer.String(str, snprintf(str, sizeof(str), "%016llx", m_span.id));

    if (m_span.__isset.parent_id)
    {
        writer.Key("parentId");
        writer.String(str, snprintf(str, sizeof(str), "%016llx", m_span.parent_id));
    }

    writer.Key("annotations");
    writer.StartArray();

    for (auto &annotation : m_span.annotations)
    {
        writer.StartObject();

        if (annotation.__isset.host)
        {
            writer.Key("endpoint");
            serialize_endpoint(annotation.host);
        }

        writer.Key("timestamp");
        writer.Int64(annotation.timestamp);

        writer.Key("value");
        writer.String(annotation.value);

        writer.EndObject();
    }

    writer.EndArray(m_span.annotations.size());

    writer.Key("binary_annotations");
    writer.StartArray();

    for (auto &annotation : m_span.binary_annotations)
    {
        writer.StartObject();

        if (annotation.__isset.host)
        {
            writer.Key("endpoint");
            serialize_endpoint(annotation.host);
        }

        writer.Key("key");
        writer.String(annotation.key);

        writer.Key("value");
        serialize_value(annotation.value, annotation.annotation_type);

        writer.EndObject();
    }

    writer.EndArray(m_span.binary_annotations.size());

    if (m_span.__isset.debug)
    {
        writer.Key("debug");
        writer.Bool(m_span.debug);
    }

    if (m_span.__isset.timestamp)
    {
        writer.Key("timestamp");
        writer.Int64(m_span.timestamp);
    }

    if (m_span.__isset.duration)
    {
        writer.Key("duration");
        writer.Int64(m_span.duration);
    }

    writer.EndObject();
}

} // namespace zipkin