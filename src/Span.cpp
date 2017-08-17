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

        set_ipv4(ntohl(v4->sin_addr.s_addr));
        set_port(ntohs(v4->sin_port));

        break;
    }

    case AF_INET6:
    {
        auto v6 = reinterpret_cast<const struct sockaddr_in6 *>(addr);

        set_ipv6(std::string(reinterpret_cast<const char *>(v6->sin6_addr.s6_addr), sizeof(v6->sin6_addr)));
        set_port(ntohs(v6->sin6_port));

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

        set_ipv6(std::string(reinterpret_cast<char *>(bytes.data()), bytes.size()));
    }
    else
    {
        auto bytes = ip.to_v4().to_bytes();

        set_ipv4(ntohl(*reinterpret_cast<uint32_t *>(bytes.data())));
    }

    set_port(port);

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

void Span::reset(const std::string &name, span_id_t parent_id, userdata_t userdata, bool sampled, bool shared)
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
    m_shared = shared;
    m_local_endpoint.reset();
    m_remote_endpoint.reset();
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


namespace __impl
{

inline bool is_set(const ::Endpoint &host) {
    return host.__isset.service_name && (host.__isset.ipv4 || host.__isset.ipv6);
}

inline bool close_enough(const ::Endpoint *lhs, const ::Endpoint *rhs) {
    return lhs->__isset.service_name && rhs->__isset.service_name &&
           lhs->service_name == rhs->service_name;
}

const std::vector<Span2> Span2::from_span(const Span *span) {
    std::vector<Span2> spans = {{ Span2 { .span = span }}};

    auto new_span = [span, &spans](const ::Endpoint *host, Kind kind=UNKNOWN) -> Span2& {
        spans.push_back(Span2 { .span = span, .kind = kind, .local_endpoint = host });

        return spans.back();
    };

    auto for_endpoint = [span, &spans, new_span](const ::Endpoint &host) -> Span2& {
        if (!is_set(host)) {
            return spans.front(); // allocate missing endpoint data to first span
        }

        for (Span2& next : spans) {
            if (!next.local_endpoint) {
                next.local_endpoint = &host;
                return next;
            }

            if (close_enough(next.local_endpoint, &host)) {
                return next;
            }
        }

        return new_span(&host);
    };

    auto maybe_timestamp_and_duration = [span, for_endpoint](const ::Annotation *begin, const ::Annotation *end) {
        Span2& span2 = for_endpoint(begin->host);

        if (span->m_span.__isset.timestamp && span->m_span.__isset.duration) {
            span2.timestamp = span->m_span.timestamp;
            span2.duration = span->m_span.duration;
        } else {
            span2.timestamp = begin->timestamp;
            span2.duration = end ? (end->timestamp - begin->timestamp) : 0;
        }
    };

    const ::Annotation *cs = nullptr, *sr = nullptr, *ss = nullptr, *cr = nullptr, *ws = nullptr, *wr = nullptr;

    // add annotations unless they are "core"
    for (const ::Annotation& annotation : span->m_span.annotations)
    {
        Span2& span2 = for_endpoint(annotation.host);

        if (annotation.value.size() == 2 && is_set(annotation.host)) {
            uint16_t key = *reinterpret_cast<const uint16_t *>(annotation.value.c_str());

            // core annotations require an endpoint. Don't give special treatment when that's missing
            switch (key) {
                case 0x7363: // CLIENT_SEND
                    span2.kind = CLIENT;
                    cs = &annotation;
                    break;

                case 0x7273: // SERVER_RECV
                    span2.kind = SERVER;
                    sr = &annotation;
                    break;

                case 0x7373: // SERVER_SEND
                    span2.kind = SERVER;
                    ss = &annotation;
                    break;

                case 0x7263: // CLIENT_RECV
                    span2.kind = CLIENT;
                    cr = &annotation;
                    break;

                case 0x7377: // WIRE_SEND
                    ws = &annotation;
                    break;

                case 0x7277: // WIRE_RECV
                    wr = &annotation;
                    break;

                default:
                    span2.annotations.push_back(&annotation);
                    break;
            }
        } else {
            span2.annotations.push_back(&annotation);
        }
    }

    if (cs && sr) {
        // in a shared span, the client side owns span duration by annotations or explicit timestamp
        maybe_timestamp_and_duration(cs, cr);

        // special-case loopback: We need to make sure on loopback there are two span2s
        Span2& client = for_endpoint(cs->host);
        bool is_client = close_enough(&cs->host, &sr->host);
        Span2& server = is_client ?
            new_span(&sr->host, SERVER) : // fork a new span for the server side
            for_endpoint(sr->host);

        if (is_client) {
            client.kind = CLIENT;
        }

        // the server side is smaller than that, we have to read annotations to find out
        server.shared = true;
        server.timestamp = sr->timestamp;

        if (ss) server.duration = ss->timestamp - sr->timestamp;
        if (!cr && !span->m_span.duration) client.duration = 0; // one-way has no duration
    } else if (cs && cr) {
        maybe_timestamp_and_duration(cs, cr);
    } else if (sr && ss) {
        maybe_timestamp_and_duration(sr, ss);
    } else {
        // otherwise, the span is incomplete. revert special-casing
        for (Span2& next : spans) {
            switch (next.kind) {
            case CLIENT:
                if (cs) next.timestamp = cs->timestamp;
                break;
            case SERVER:
                if (sr) next.timestamp = sr->timestamp;
                break;
            case UNKNOWN:
                break;
            }
        }

        if (span->m_span.timestamp) {
            Span2& span2 = spans.front();

            span2.timestamp = span->m_span.timestamp;
            span2.duration = span->m_span.duration;
        }
    }

    // Span v1 format did not have a shared flag. By convention, span.timestamp being absent
    // implied shared. When we only see the server-side, carry this signal over.
    if (!cs && (sr && !span->m_span.timestamp)) {
        for_endpoint(sr->host).shared = true;
    }

    if (ws) for_endpoint(ws->host).annotations.push_back(ws);
    if (wr) for_endpoint(wr->host).annotations.push_back(wr);

    const ::Endpoint *ca = nullptr, *sa = nullptr;

    // convert binary annotations to tags and addresses
    for (const ::BinaryAnnotation& annotation : span->m_span.binary_annotations)
    {
        if (annotation.key.size() == 2 &&
            annotation.annotation_type == AnnotationType::BOOL &&
            is_set(annotation.host))
        {
            switch (* reinterpret_cast<const uint16_t *>(annotation.key.c_str())) {
            case 0x6163: // CLIENT_ADDR
                ca = &annotation.host;
                break;

            case 0x6173: // SERVER_ADDR
                sa = &annotation.host;
                break;

            default:
                for_endpoint(annotation.host).binary_annotations.push_back(&annotation);
                break;
            }

            continue;
        }

        for_endpoint(annotation.host).binary_annotations.push_back(&annotation);
    }

    if (cs && sa && !close_enough(sa, &cs->host)) {
        for_endpoint(cs->host).remote_endpoint = sa;
    }

    if (sr && ca && !close_enough(ca, &sr->host)) {
        for_endpoint(sr->host).remote_endpoint = ca;
    }

    if ((!cs && !sr) && (ca && sa)) {
        for_endpoint(*ca).remote_endpoint = sa;
    }

    return std::move(spans);
}

} // namespace __impl

} // namespace zipkin
