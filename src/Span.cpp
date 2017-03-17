#include <random>
#include <utility>

#include <ios>
#include <cstdlib>

#include <arpa/inet.h>

#include <glog/logging.h>

#include <boost/algorithm/string.hpp>
#include <boost/thread/tss.hpp>

#include "Span.h"
#include "Tracer.h"
#include "Base64.h"

namespace zipkin
{

#define DEFINE_TRACE_KEY(name) const std::string &TraceKeys::name = g_zipkinCore_constants.name;

DEFINE_TRACE_KEY(CLIENT_SEND)
DEFINE_TRACE_KEY(CLIENT_RECV)
DEFINE_TRACE_KEY(SERVER_SEND)
DEFINE_TRACE_KEY(SERVER_RECV)
DEFINE_TRACE_KEY(WIRE_SEND)
DEFINE_TRACE_KEY(WIRE_RECV)
DEFINE_TRACE_KEY(CLIENT_SEND_FRAGMENT)
DEFINE_TRACE_KEY(CLIENT_RECV_FRAGMENT)
DEFINE_TRACE_KEY(SERVER_SEND_FRAGMENT)
DEFINE_TRACE_KEY(SERVER_RECV_FRAGMENT)
DEFINE_TRACE_KEY(LOCAL_COMPONENT)
DEFINE_TRACE_KEY(CLIENT_ADDR)
DEFINE_TRACE_KEY(SERVER_ADDR)
DEFINE_TRACE_KEY(ERROR)
DEFINE_TRACE_KEY(HTTP_HOST)
DEFINE_TRACE_KEY(HTTP_METHOD)
DEFINE_TRACE_KEY(HTTP_PATH)
DEFINE_TRACE_KEY(HTTP_URL)
DEFINE_TRACE_KEY(HTTP_STATUS_CODE)
DEFINE_TRACE_KEY(HTTP_REQUEST_SIZE)
DEFINE_TRACE_KEY(HTTP_RESPONSE_SIZE)

std::unique_ptr<const struct sockaddr> Endpoint::sockaddr(void) const
{
    std::unique_ptr<sockaddr_storage> addr(new sockaddr_storage());

    if (m_host.__isset.ipv6)
    {
        auto v6 = reinterpret_cast<sockaddr_in6 *>(addr.get());

        v6->sin6_family = AF_INET6;
        memcpy(v6->sin6_addr.s6_addr, m_host.ipv6.c_str(), m_host.ipv6.size());
        v6->sin6_port = htons(m_host.port);
    }
    else
    {
        auto v4 = reinterpret_cast<sockaddr_in *>(addr.get());

        v4->sin_family = AF_INET;
        v4->sin_addr.s_addr = htonl(m_host.ipv4);
        v4->sin_port = htons(m_host.port);
    }

    return std::unique_ptr<const struct sockaddr>(reinterpret_cast<const struct sockaddr *>(addr.release()));
}

ip::address Endpoint::addr(void) const
{
    if (m_host.__isset.ipv6)
    {
        ip::address_v6::bytes_type bytes;
        std::copy(m_host.ipv6.begin(), m_host.ipv6.end(), bytes.begin());
        return ip::address_v6(bytes);
    }

    return ip::address_v4(m_host.ipv4);
}

Endpoint &Endpoint::with_addr(const struct sockaddr *addr)
{
    assert(addr);
    assert(addr->sa_family);

    switch (addr->sa_family)
    {
    case AF_INET:
    {
        auto v4 = reinterpret_cast<const struct sockaddr_in *>(addr);

        m_host.__set_ipv4(ntohl(v4->sin_addr.s_addr));
        m_host.__set_port(ntohs(v4->sin_port));
        m_host.__isset.ipv6 = false;
        break;
    }

    case AF_INET6:
    {
        auto v6 = reinterpret_cast<const struct sockaddr_in6 *>(addr);

        m_host.__set_ipv6(std::string(reinterpret_cast<const char *>(v6->sin6_addr.s6_addr), sizeof(v6->sin6_addr)));
        m_host.__set_port(ntohs(v6->sin6_port));
        break;
    }
    }

    return *this;
}

Endpoint &Endpoint::with_addr(const std::string &addr, port_t port)
{
    boost::asio::ip::address ip = boost::asio::ip::address::from_string(addr);

    if (ip.is_v6())
    {
        auto bytes = ip.to_v6().to_bytes();

        m_host.__set_ipv6(std::string(reinterpret_cast<char *>(bytes.data()), bytes.size()));
    }
    else
    {
        auto bytes = ip.to_v4().to_bytes();

        m_host.__set_ipv4(ntohl(*reinterpret_cast<uint32_t *>(bytes.data())));
        m_host.__isset.ipv6 = false;
    }

    m_host.__set_port(port);

    return *this;
}

const char *to_string(AnnotationType type)
{
    switch (type)
    {
    case AnnotationType::BOOL:
        return "BOOL";
    case AnnotationType::BYTES:
        return "BYTES";
    case AnnotationType::I16:
        return "I16";
    case AnnotationType::I32:
        return "I32";
    case AnnotationType::I64:
        return "I64";
    case AnnotationType::DOUBLE:
        return "DOUBLE";
    case AnnotationType::STRING:
        return "STRING";
    }
}

void Span::reset(const std::string &name, span_id_t parent_id, userdata_t userdata, bool sampled)
{
    m_span.debug = false;
    m_span.duration = 0;
    m_span.annotations.clear();
    m_span.binary_annotations.clear();
    m_span.__isset = _Span__isset();

    m_span.__set_name(name);
    m_span.__set_trace_id(next_id());
    m_span.__set_trace_id_high(next_id());
    m_span.__set_id(next_id());
    m_span.__set_debug(0);
    m_span.__set_timestamp(now().count());

    if (parent_id)
    {
        m_span.__set_parent_id(parent_id);
    }
    else
    {
        m_span.parent_id = 0;
    }

    m_userdata = userdata;
    m_sampled = sampled;
}

void Span::submit(void)
{
    if (m_span.timestamp)
        m_span.__set_duration(now().count() - m_span.timestamp);

    if (m_tracer)
        m_tracer->submit(this);
}

static boost::thread_specific_ptr<std::mt19937_64> g_rand_gen;

span_id_t Span::next_id()
{
    if (!g_rand_gen.get())
    {
        g_rand_gen.reset(new std::mt19937_64((std::chrono::system_clock::now().time_since_epoch().count() << 32) + std::random_device()()));
    }

    std::mt19937_64 &rand_gen = *g_rand_gen.get();

    return rand_gen();
}

timestamp_t Span::now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
}

Annotation Span::annotate(const std::string &value, const Endpoint *endpoint)
{
    ::Annotation annotation;

    annotation.__set_timestamp(now().count());
    annotation.__set_value(value);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.annotations.push_back(annotation);

    return Annotation(*this, m_span.annotations.back());
}

BinaryAnnotation Span::annotate(const std::string &key, const uint8_t *value, size_t size, const Endpoint *endpoint)
{
    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(std::string(reinterpret_cast<const char *>(value), size));
    annotation.__set_annotation_type(AnnotationType::BYTES);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);

    return BinaryAnnotation(*this, m_span.binary_annotations.back());
}

BinaryAnnotation Span::annotate(const std::string &key, const std::string &value, const Endpoint *endpoint)
{
    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(value);
    annotation.__set_annotation_type(AnnotationType::STRING);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);

    return BinaryAnnotation(*this, m_span.binary_annotations.back());
}

BinaryAnnotation Span::annotate(const std::string &key, const std::wstring &value, const Endpoint *endpoint)
{
    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(boost::locale::conv::utf_to_utf<char>(value));
    annotation.__set_annotation_type(AnnotationType::STRING);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);

    return BinaryAnnotation(*this, m_span.binary_annotations.back());
}

size_t CachedSpan::cache_size(void) const
{
    return static_cast<CachedTracer *>(m_tracer)->cache().message_size() - cache_offset();
}

void *CachedSpan::operator new(size_t size, CachedTracer *tracer) noexcept
{
    size_t sz = tracer ? tracer->cache().message_size() : size;
    void *p = nullptr;

    if (posix_memalign(&p, CachedTracer::CACHE_LINE_SIZE, sz))
        return nullptr;

    VLOG(3) << "Span @ " << p << " allocated with " << sz << " bytes";

    return p;
}

void CachedSpan::operator delete(void *ptr, std::size_t sz) noexcept
{
    if (!ptr)
        return;

    CachedSpan *span = static_cast<CachedSpan *>(ptr);

    VLOG(3) << "Span @ " << ptr << " deleted with " << sz << " bytes, id=" << std::hex << span->id();

    free(ptr);
}

Span *CachedSpan::span(const std::string &name, userdata_t userdata) const
{
    std::unique_ptr<Span> span;

    if (m_tracer)
    {
        span.reset(m_tracer->span(m_span.name, m_span.id, userdata));
    }
    else
    {
        span.reset(new (nullptr) CachedSpan(nullptr, m_span.name, m_span.id, userdata));
    }

    span->with_trace_id(trace_id());
    span->with_trace_id_high(trace_id_high());

    return span.release();
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