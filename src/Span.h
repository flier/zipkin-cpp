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
 * \brief Indicates the network context of a service recording an annotation with two exceptions.
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
        with_service_name(service);
    }
    Endpoint(const std::string &service, const sockaddr *addr)
    {
        assert(addr);

        with_service_name(service);
        with_addr(addr);
    }
    Endpoint(const std::string &service, const sockaddr_in &addr)
    {
        with_service_name(service);
        with_addr(addr);
    }
    Endpoint(const std::string &service, const sockaddr_in6 &addr)
    {
        with_service_name(service);
        with_addr(addr);
    }

    /**
    * \brief Classifier of a source or destination in lowercase, such as "zipkin-server".
    *
    * This is the primary parameter for trace lookup, so should be intuitive as possible, for
    * example, matching names in service discovery.
    *
    * Conventionally, when the service name isn't known, service_name = "unknown". However, it is
    * also permissible to set service_name = "" (empty string). The difference in the latter usage is
    * that the span will not be queryable by service name unless more information is added to the
    * span with non-empty service name, e.g. an additional annotation from the server.
    *
    * Particularly clients may not have a reliable service name at ingest. One approach is to set
    * service_name to "" at ingest, and later assign a better label based on binary annotations, such
    * as user agent.
    */
    inline const std::string &service_name(void) const { return m_host.service_name; }

    inline Endpoint &with_service_name(const std::string &service_name);

    std::unique_ptr<const sockaddr> addr(void);

    inline uint16_t port(void) const { return m_host.port; }

    Endpoint &with_addr(const sockaddr *addr);

    /**
    * \brief with IPv4 address
    */
    inline Endpoint &with_addr(const sockaddr_in &addr);

    /**
    * \brief with IPv6 address
    */
    inline Endpoint &with_addr(const sockaddr_in6 &addr);

    /**
    * \brief with IPv4 address
    */
    inline Endpoint &with_ipv4(const std::string &ip);

    /**
    * \brief with IPv6 address
    */
    inline Endpoint &with_ipv6(const std::string &ip);

    inline Endpoint &with_port(uint16_t port)
    {
        m_host.__set_port(port);
        return *this;
    }

    inline const ::Endpoint &host(void) const { return m_host; }
};

struct Tracer;

class CachedTracer;

/**
* \brief Associates an event that explains latency with a timestamp.
*
* Unlike log statements, annotations are often codes: Ex. {@link TraceKeys#SERVER_RECV "sr"}.
*/
class Annotation
{
    ::Annotation &m_annotation;

  public:
    Annotation(::Annotation &annotation) : m_annotation(annotation) {}

    /**
    * \brief Microseconds from epoch.
    */
    timestamp_t timestamp(void) const { return timestamp_t(m_annotation.timestamp); }

    /** \sa Annotation#timestamp */
    Annotation &with_timestamp(timestamp_t timestamp)
    {
        m_annotation.timestamp = timestamp.count();
        return *this;
    }

    /**
    * \brief Usually a short tag indicating an event, like {@link TraceKeys#SERVER_RECV "sr"}. or {@link
    * TraceKeys#ERROR "error"}
    */
    const std::string &value(void) const { return m_annotation.value; }

    /** \sa Annotation#value */
    Annotation &with_value(const std::string &value)
    {
        m_annotation.value = value;
        return *this;
    }

    /**
     * \brief The host that recorded #value, primarily for query by service name.
     */
    const Endpoint endpoint(void) const { return Endpoint(m_annotation.host); }

    /** \sa Annotation#endpoint */
    Annotation &with_endpoint(const Endpoint &endpoint)
    {
        m_annotation.host = endpoint.host();
        return *this;
    }
};

/**
* \brief A subset of thrift base types, except BYTES.
*/
using AnnotationType = ::AnnotationType::type;

/**
* \enum AnnotationType
*
* \var AnnotationType AnnotationType::BOOL
* \p Set to 0x01 when key is CLIENT_ADDR or SERVER_ADDR
*
* \var AnnotationType AnnotationType::BYTES
* No encoding, or type is unknown.
*
* \var AnnotationType AnnotationType::I16
* \p int16_t
*
* \var AnnotationType AnnotationType::I32
* \p int32_t
*
* \var AnnotationType AnnotationType::I64
* \p int64_t
*
* \var AnnotationType AnnotationType::DOUBLE
* \p double
*
* \var AnnotationType AnnotationType::STRING
* The only type zipkin v1 supports search against.
*/

#define DEF_TRACE_KEY(name) static constexpr const std::string &name = g_zipkinCore_constants.name;

/**
* \brief Span tracing key
*/
struct TraceKeys
{
    /**
    * \brief The client sent ("cs") a request to a server.
    *
    * There is only one send per span. For example, if there's a transport error,
    * each attempt can be logged as a #WIRE_SEND annotation.
    */
    DEF_TRACE_KEY(CLIENT_SEND)
    /**
    * \brief The client received ("cr") a response from a server.
    *
    * There is only one receive per span. For example, if duplicate responses were received,
    * each can be logged as a #WIRE_RECV annotation.
    */
    DEF_TRACE_KEY(CLIENT_RECV)
    /**
    * \brief The server sent ("ss") a response to a client.
    *
    * There is only one response per span. If there's a transport error,
    * each attempt can be logged as a #WIRE_SEND annotation.
    */
    DEF_TRACE_KEY(SERVER_SEND)
    /**
    * \brief The server received ("sr") a request from a client.
    *
    * There is only one request per span.  For example, if duplicate responses were received,
    * each can be logged as a #WIRE_RECV annotation.
    */
    DEF_TRACE_KEY(SERVER_RECV)
    /**
    * \brief Optionally logs an attempt to send a message on the wire.
    *
    * Multiple wire send events could indicate network retries.
    * A lag between client or server send and wire send might indicate
    * queuing or processing delay.
    */
    DEF_TRACE_KEY(WIRE_SEND)
    /**
    * \brief Optionally logs an attempt to receive a message from the wire.
    *
    * Multiple wire receive events could indicate network retries.
    * A lag between wire receive and client or server receive might
    * indicate queuing or processing delay.
    */
    DEF_TRACE_KEY(WIRE_RECV)
    /**
    * \brief Optionally logs progress of a (#CLIENT_SEND, #WIRE_SEND).
    *
    * For example, this could be one chunk in a chunked request.
    */
    DEF_TRACE_KEY(CLIENT_SEND_FRAGMENT)
    /**
    * \brief Optionally logs progress of a (#CLIENT_RECV, #WIRE_RECV).
    *
    * For example, this could be one chunk in a chunked response.
    */
    DEF_TRACE_KEY(CLIENT_RECV_FRAGMENT)
    /**
    * \brief Optionally logs progress of a (#SERVER_SEND, #WIRE_SEND).
    *
    * For example, this could be one chunk in a chunked response.
    */
    DEF_TRACE_KEY(SERVER_SEND_FRAGMENT)
    /**
    * \brief Optionally logs progress of a (#SERVER_RECV, #WIRE_RECV).
    *
    * For example, this could be one chunk in a chunked request.
    */
    DEF_TRACE_KEY(SERVER_RECV_FRAGMENT)
    /**
    * \brief The {@link BinaryAnnotation#value value} of "lc" is the component or namespace of a local
    * span.
    *
    * <p>{@link BinaryAnnotation#endpoint} adds service context needed to support queries.
    *
    * <p>Local Component("lc") supports three key features: flagging, query by service and filtering
    * Span.name by namespace.
    *
    * <p>While structurally the same, local spans are fundamentally different than RPC spans in how
    * they should be interpreted. For example, zipkin v1 tools center on RPC latency and service
    * graphs. Root local-spans are neither indicative of critical path RPC latency, nor have impact
    * on the shape of a service graph. By flagging with "lc", tools can special-case local spans.
    *
    * <p>Zipkin v1 Spans are unqueryable unless they can be indexed by service name. The only path
    * to a {@link Endpoint#service_name service name} is via {@link BinaryAnnotation#endpoint
    * host}. By logging "lc", a local span can be queried even if no other annotations are logged.
    *
    * <p>The value of "lc" is the namespace of {@link Span#name}. For example, it might be
    * "finatra2", for a span named "bootstrap". "lc" allows you to resolves conflicts for the same
    * Span.name, for example "finatra/bootstrap" vs "finch/bootstrap". Using local component, you'd
    * search for spans named "bootstrap" where "lc=finch"
    */
    DEF_TRACE_KEY(LOCAL_COMPONENT)
    /**
    * \brief When present, {@link BinaryAnnotation#endpoint} indicates a client address ("ca") in a span.
    * Most likely, there's only one. Multiple addresses are possible when a client changes its ip or
    * port within a span.
    */
    DEF_TRACE_KEY(CLIENT_ADDR)
    /**
    * \brief When present, {@link BinaryAnnotation#endpoint} indicates a server address ("sa") in a span.
    * Most likely, there's only one. Multiple addresses are possible when a client is redirected, or
    * fails to a different server ip or port.
    */
    DEF_TRACE_KEY(SERVER_ADDR)
    /**
    * \brief When an {@link Annotation#value}, this indicates when an error occurred. When a {@link
    * BinaryAnnotation#key}, the value is a human readable message associated with an error.
    *
    * <p>Due to transient errors, an ERROR annotation should not be interpreted as a span failure,
    * even the annotation might explain additional latency. Instrumentation should add the ERROR
    * binary annotation when the operation failed and couldn't be recovered.
    *
    * <p>Here's an example: A span has an ERROR annotation, added when a WIRE_SEND failed. Another
    * WIRE_SEND succeeded, so there's no ERROR binary annotation on the span because the overall
    * operation succeeded.
    *
    * <p>Note that RPC spans often include both client and server hosts: It is possible that only one
    * side perceived the error.
    */
    DEF_TRACE_KEY(ERROR)

    /**
    * \brief HTTP \p Host header
    */
    DEF_TRACE_KEY(HTTP_HOST)
    /**
    * \brief HTTP request method
    */
    DEF_TRACE_KEY(HTTP_METHOD)
    /**
    * \brief HTTP request path
    */
    DEF_TRACE_KEY(HTTP_PATH)
    /**
    * \brief HTTP request URL
    */
    DEF_TRACE_KEY(HTTP_URL)
    /**
    * \brief HTTP status code
    */
    DEF_TRACE_KEY(HTTP_STATUS_CODE)
    /**
    * \brief HTTP request size
    */
    DEF_TRACE_KEY(HTTP_REQUEST_SIZE)
    /**
    * \brief HTTP response size
    */
    DEF_TRACE_KEY(HTTP_RESPONSE_SIZE)
};

/**
* \brief Binary annotations are tags applied to a Span to give it context. For example, a binary
* annotation of {@link TraceKeys#HTTP_PATH "http.path"} could the path to a resource in a RPC call.
*
* <p>Binary annotations of type {@link AnnotationType#STRING} are always queryable, though more a historical
* implementation detail than a structural concern.
*
* <p>Binary annotations can repeat, and vary on the host. Similar to Annotation, the host
* indicates who logged the event. This allows you to tell the difference between the client and
* server side of the same key. For example, the key "http.path" might be different on the client and
* server side due to rewriting, like "/api/v1/myresource" vs "/myresource. Via the host field, you
* can see the different points of view, which often help in debugging.
*/
class BinaryAnnotation
{
    ::BinaryAnnotation &m_annotation;

  public:
    BinaryAnnotation(::BinaryAnnotation &annotation) : m_annotation(annotation) {}

    /**
    * \brief The thrift type of value, most often AnnotationType#STRING.
    *
    * Note: type shouldn't vary for the same key.
    */
    AnnotationType type(void) const { return m_annotation.annotation_type; }

    /**
    * \brief Name used to lookup spans, such as {@link TraceKeys#HTTP_PATH "http.path"} or {@link
    * TraceKeys#ERROR "error"}
    */
    const std::string &key(void) const { return m_annotation.key; }
    /**
    * \brief Serialized thrift bytes, in TBinaryProtocol format.
    *
    * For legacy reasons, byte order is big-endian. See THRIFT-3217.
    */
    const std::string &value(void) const { return m_annotation.value; }

    /**
    * \brief Annotate with value
    *
    * \sa BinaryAnnotation#value
    */
    template <typename T>
    BinaryAnnotation &with_value(const T &value);

    /**
    * \brief Annotate with AnnotationType#BYTES
    *
    * \sa BinaryAnnotation#value
    */
    inline BinaryAnnotation &with_value(const uint8_t *value, size_t size)
    {
        m_annotation.value = std::string(reinterpret_cast<const char *>(value), size);
        m_annotation.annotation_type = AnnotationType::BYTES;
        return *this;
    }
    /**
    * \brief Annotate with AnnotationType#BYTES
    *
    * \sa BinaryAnnotation#value
    */
    template <size_t N>
    inline BinaryAnnotation &with_value(const uint8_t (&value)[N])
    {
        return with_value(value, N);
    }
    /**
    * \brief Annotate with AnnotationType#BYTES
    *
    * \sa BinaryAnnotation#value
    */
    inline BinaryAnnotation &with_value(const std::vector<uint8_t> &value)
    {
        return with_value(value.data(), value.size());
    }
    /**
    * \brief Annotate with AnnotationType#STRING
    *
    * \sa BinaryAnnotation#value
    */
    inline BinaryAnnotation &with_value(const std::string &value)
    {
        m_annotation.value = value;
        m_annotation.annotation_type = AnnotationType::STRING;
        return *this;
    }
    /**
    * \brief Annotate with AnnotationType#STRING
    *
    * \sa BinaryAnnotation#value
    */
    inline BinaryAnnotation &with_value(const char *value, int len = -1)
    {
        return with_value(len >= 0 ? std::string(value, len) : std::string(value));
    }
    /**
    * \brief Annotate with AnnotationType#STRING
    *
    * \sa BinaryAnnotation#value
    */
    inline BinaryAnnotation &with_value(const std::wstring &value)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

        return with_value(converter.to_bytes(value));
    }
    /**
    * \brief Annotate with AnnotationType#STRING
    *
    * \sa BinaryAnnotation#value
    */
    inline BinaryAnnotation &with_value(const wchar_t *value, int len = -1)
    {
        return with_value(len >= 0 ? std::wstring(value, len) : std::wstring(value));
    }

    /**
    * \brief The host that recorded {@link #value}, allowing query by service name or address.
    *
    * There are two exceptions: when {@link #key} is {@link TraceKeys#CLIENT_ADDR} or {@link
    * TraceKeys#SERVER_ADDR}, this is the source or destination of an RPC. This exception allows
    * zipkin to display network context of uninstrumented services, such as browsers or databases.
    */
    const Endpoint endpoint(void) const { return Endpoint(m_annotation.host); }
    /**
    * \brief Annotate with Endpoint
    *
    * \sa BinaryAnnotation#endpoint
    */
    BinaryAnnotation &with_endpoint(const Endpoint &endpoint)
    {
        m_annotation.host = endpoint.host();
        return *this;
    }
};

/**
* \brief A trace is a series of spans (often RPC calls) which form a latency tree.
*
* Spans are usually created by instrumentation in RPC clients or servers, but can also
* represent in-process activity. Annotations in spans are similar to log statements, and are
* sometimes created directly by application developers to indicate events of interest, such as a
* cache miss.
*
* The root span is where #parent_id is 0; it usually has the longest #duration in the trace.
*
* Span identifiers are packed into longs, but should be treated opaquely. ID encoding is
* 16 or 32 character lower-hex, to avoid signed interpretation.
*/
class Span
{
  protected:
    Tracer *m_tracer;
    ::Span m_span;
    userdata_t m_userdata;

    static const ::Endpoint host(const Endpoint *endpoint);

  public:
    /**
     * \brief Construct a span
     */
    Span(Tracer *tracer, const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr);

    /**
     * \brief Reset a span
     */
    void reset(const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr);

    /**
     * \brief Submit a span to Tracer
     */
    void submit(void);

    /**
     * \brief Associated Tracer
     */
    inline Tracer *tracer(void) const { return m_tracer; }

    /**
     * \brief Raw thrift message
     */
    inline const ::Span &message(void) const { return m_span; }

    /**
     * \brief Unique 8-byte identifier for a trace, set on all spans within it.
     */
    inline trace_id_t trace_id(void) const { return m_span.trace_id; }

    /**
    * When non-zero, the trace containing this span uses 128-bit trace identifiers.
    */
    inline trace_id_t trace_id_high(void) const { return m_span.trace_id_high; }

    /** \sa Span#trace_id */
    inline Span &with_trace_id(trace_id_t trace_id)
    {
        m_span.trace_id = trace_id;
        return *this;
    }

    /**
    * \brief Unique 8-byte identifier of this span within a trace.
    */
    inline span_id_t id(void) const { return m_span.id; }

    /** \sa Span#id */
    inline Span &with_id(span_id_t id)
    {
        m_span.id = id;
        return *this;
    }

    /**
    * \brief Span name in lowercase, rpc method for example.
    *
    * Conventionally, when the span name isn't known, name = "unknown".
    */
    inline const std::string &name(void) const { return m_span.name; }

    /** \sa Span#name */
    inline Span &with_name(const std::string &name)
    {
        m_span.name = name;
        return *this;
    }

    /**
    * \brief The parent's #id or 0 if this the root span in a trace.
    */
    inline span_id_t parent_id(void) const { return m_span.parent_id; }

    /** \sa Span#parent_id */
    inline Span &with_parent_id(span_id_t id)
    {
        m_span.parent_id = id;
        return *this;
    }

    /**
    * \brief Epoch microseconds of the start of this span, possibly absent if this an incomplete span.
    */
    inline timestamp_t timestamp(void) const { return timestamp_t(m_span.timestamp); }

    /** \sa Span#timestamp */
    inline Span &with_timestamp(timestamp_t timestamp)
    {
        m_span.timestamp = timestamp.count();
        return *this;
    }

    /**
    * \brief Measurement in microseconds of the critical path, if known.
    *
    * Durations of less than one microsecond must be rounded up to 1 microsecond.
    */
    inline duration_t duration(void) const { return duration_t(m_span.duration); }

    /** \sa Span#duration */
    inline Span &with_duration(duration_t duration)
    {
        m_span.duration = duration.count();
        return *this;
    }

    /**
     * \brief Associated user data
     */
    inline userdata_t userdata(void) const { return m_userdata; }

    /** \sa Span#userdata */
    inline Span &with_userdata(userdata_t userdata)
    {
        m_userdata = userdata;
        return *this;
    }

    virtual inline Span *span(const std::string &name, userdata_t userdata = nullptr) const
    {
        return new Span(m_tracer, name, m_span.id, userdata ? userdata : m_userdata);
    };

    /**
    * \brief Generatea a random unique id for Span or Tracer;
    */
    static span_id_t next_id();

    /**
    * \brief Get the current time
    */
    static timestamp_t now();

    /**
    * \brief Associates events that explain latency with a timestamp.
    */
    Annotation annotate(const std::string &value, const Endpoint *endpoint = nullptr);

    /// \brief Annotate TraceKeys#CLIENT_SEND event
    inline Annotation client_send(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::CLIENT_SEND, endpoint);
    }
    /// \brief Annotate TraceKeys#CLIENT_RECV event
    inline Annotation client_recv(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::CLIENT_RECV, endpoint);
    }
    /// \brief Annotate TraceKeys#SERVER_SEND event
    inline Annotation server_send(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::SERVER_SEND, endpoint);
    }
    /// \brief Annotate TraceKeys#CLIENT_RECV event
    inline Annotation server_recv(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::SERVER_RECV, endpoint);
    }
    /// \brief Annotate TraceKeys#WIRE_SEND event
    inline Annotation wire_send(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::WIRE_SEND, endpoint);
    }
    /// \brief Annotate TraceKeys#CLIENT_RECV event
    inline Annotation wire_recv(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::WIRE_RECV, endpoint);
    }
    /// \brief Annotate TraceKeys#CLIENT_SEND_FRAGMENT event
    inline Annotation client_send_fragment(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::CLIENT_SEND_FRAGMENT, endpoint);
    }
    /// \brief Annotate TraceKeys#CLIENT_RECV_FRAGMENT event
    inline Annotation client_recv_fragment(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::CLIENT_RECV_FRAGMENT, endpoint);
    }
    /// \brief Annotate TraceKeys#SERVER_SEND_FRAGMENT event
    inline Annotation server_send_fragment(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::SERVER_SEND_FRAGMENT, endpoint);
    }
    /// \brief Annotate TraceKeys#SERVER_RECV_FRAGMENT event
    inline Annotation server_recv_fragment(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::SERVER_RECV_FRAGMENT, endpoint);
    }

    /**
    * \brief Tags a span with context, usually to support query or aggregation.
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

    /// \brief Annotate TraceKeys#HTTP_HOST event
    inline BinaryAnnotation http_host(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::HTTP_HOST, value, endpoint);
    }
    /// \brief Annotate TraceKeys#HTTP_METHOD event
    inline BinaryAnnotation http_method(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::HTTP_METHOD, value, endpoint);
    }
    /// \brief Annotate TraceKeys#HTTP_PATH event
    inline BinaryAnnotation http_path(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::HTTP_PATH, value, endpoint);
    }
    /// \brief Annotate TraceKeys#HTTP_URL event
    inline BinaryAnnotation http_url(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::HTTP_URL, value, endpoint);
    }
    /// \brief Annotate TraceKeys#HTTP_STATUS_CODE event
    inline BinaryAnnotation http_status_code(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::HTTP_STATUS_CODE, value, endpoint);
    }
    /// \brief Annotate TraceKeys#HTTP_REQUEST_SIZE event
    inline BinaryAnnotation http_request_size(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::HTTP_REQUEST_SIZE, value, endpoint);
    }
    /// \brief Annotate TraceKeys#HTTP_REQUEST_SIZE event
    inline BinaryAnnotation http_response_size(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::HTTP_RESPONSE_SIZE, value, endpoint);
    }
    /// \brief Annotate TraceKeys#LOCAL_COMPONENT event
    inline BinaryAnnotation local_component(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::LOCAL_COMPONENT, value, endpoint);
    }
    /// \brief Annotate TraceKeys#CLIENT_ADDR event
    inline BinaryAnnotation client_addr(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::CLIENT_ADDR, value, endpoint);
    }
    /// \brief Annotate TraceKeys#SERVER_ADDR event
    inline BinaryAnnotation server_addr(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::SERVER_ADDR, value, endpoint);
    }
    /// \brief Annotate TraceKeys#ERROR event
    inline Annotation error(const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::ERROR, endpoint);
    }
    /// \brief Annotate TraceKeys#ERROR event
    inline BinaryAnnotation error(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(TraceKeys::ERROR, value, endpoint);
    }

    inline size_t serialize_binary(apache::thrift::protocol::TProtocol &protocol) const
    {
        return m_span.write(&protocol);
    }

    template <class RapidJsonWriter>
    void serialize_json(RapidJsonWriter &writer) const;
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

Endpoint &Endpoint::with_service_name(const std::string &service_name)
{
    m_host.service_name = service_name;
    return *this;
}

Endpoint &Endpoint::with_addr(const sockaddr_in &addr)
{
    m_host.__isset.ipv6 = 0;
    m_host.__set_ipv4(addr.sin_addr.s_addr);
    m_host.__set_port(addr.sin_port);

    return *this;
}

Endpoint &Endpoint::with_addr(const sockaddr_in6 &addr)
{
    m_host.__isset.ipv6 = 1;
    m_host.__set_ipv6(std::string(reinterpret_cast<const char *>(addr.sin6_addr.s6_addr), sizeof(addr.sin6_addr)));
    m_host.__set_port(addr.sin6_port);

    return *this;
}

Endpoint &Endpoint::with_ipv4(const std::string &ip)
{
    m_host.__set_ipv4(inet_addr(ip.c_str()));

    return *this;
}

Endpoint &Endpoint::with_ipv6(const std::string &ip)
{
    struct in6_addr addr;

    if (inet_pton(AF_INET6, ip.c_str(), addr.s6_addr) > 0)
        m_host.__set_ipv6(std::string(reinterpret_cast<const char *>(addr.s6_addr), sizeof(addr)));

    return *this;
}

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

template <class RapidJsonWriter>
void Span::serialize_json(RapidJsonWriter &writer) const
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

    if (m_span.trace_id_high)
    {
        writer.Key("traceIdHigh");
        writer.String(str, snprintf(str, sizeof(str), "%016llx", m_span.trace_id_high));
    }

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