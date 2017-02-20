#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <memory>
#include <chrono>

#include "../gen-cpp/zipkinCore_constants.h"

typedef uint64_t span_id_t;
typedef uint64_t timestamp_t;

namespace zipkin
{

struct Endpoint
{
    sockaddr_in addr;
    const std::string service;

    const ::Endpoint host(void) const;
};

struct Tracer;

class CachedTracer;

struct Span
{
  protected:
    Tracer *m_tracer;
    ::Span m_span;

    static const ::Endpoint host(const Endpoint *endpoint);

  public:
    Span(Tracer *tracer, const std::string &name, span_id_t parent_id = 0);

    void reset(const std::string &name, span_id_t parent_id = 0);

    void submit(void);

    Tracer *tracer(void) const { return m_tracer; }
    const ::Span &message(void) const { return m_span; }

    span_id_t id(void) const { return m_span.id; }
    const std::string &name(void) const { return m_span.name; }

    virtual Span *span(const std::string &name) const
    {
        return new Span(m_tracer, name, m_span.id);
    };

    static uint64_t next_id();

    static timestamp_t now();

    void annotate(const std::string &value, const Endpoint *endpoint = nullptr);

    inline void client_send(void) { annotate(g_zipkinCore_constants.CLIENT_SEND); }
    inline void client_recv(void) { annotate(g_zipkinCore_constants.CLIENT_RECV); }
    inline void server_send(void) { annotate(g_zipkinCore_constants.SERVER_SEND); }
    inline void server_recv(void) { annotate(g_zipkinCore_constants.SERVER_RECV); }
    inline void wire_send(void) { annotate(g_zipkinCore_constants.WIRE_SEND); }
    inline void wire_recv(void) { annotate(g_zipkinCore_constants.WIRE_RECV); }
    inline void client_send_fragment(void) { annotate(g_zipkinCore_constants.CLIENT_SEND_FRAGMENT); }
    inline void client_recv_fragment(void) { annotate(g_zipkinCore_constants.CLIENT_RECV_FRAGMENT); }
    inline void server_send_fragment(void) { annotate(g_zipkinCore_constants.SERVER_SEND_FRAGMENT); }
    inline void server_recv_fragment(void) { annotate(g_zipkinCore_constants.SERVER_RECV_FRAGMENT); }

    template <typename T>
    void annotate(const std::string &key, T value, const Endpoint *endpoint = nullptr);

    void annotate(const std::string &key, const uint8_t *value, size_t size, const Endpoint *endpoint = nullptr);

    template <size_t N>
    inline void annotate(const std::string &key, const uint8_t (&value)[N], const Endpoint *endpoint = nullptr)
    {
        annotate(key, value, N, endpoint);
    }
    inline void annotate(const std::string &key, const std::vector<uint8_t> &value, const Endpoint *endpoint = nullptr)
    {
        annotate(key, value.data(), value.size(), endpoint);
    }
    void annotate(const std::string &key, const std::string &value, const Endpoint *endpoint = nullptr);
    inline void annotate(const std::string &key, const char *value, int len = -1, const Endpoint *endpoint = nullptr)
    {
        annotate(key, len >= 0 ? std::string(value, len) : std::string(value), endpoint);
    }
    void annotate(const std::string &key, const std::wstring &value, const Endpoint *endpoint = nullptr);
    inline void annotate(const std::string &key, const wchar_t *value, int len = -1, const Endpoint *endpoint = nullptr)
    {
        annotate(key, len >= 0 ? std::wstring(value, len) : std::wstring(value), endpoint);
    }

    inline void http_host(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_HOST, value, endpoint);
    }
    inline void http_method(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_METHOD, value, endpoint);
    }
    inline void http_path(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_PATH, value, endpoint);
    }
    inline void http_url(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_URL, value, endpoint);
    }
    inline void http_status_code(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_STATUS_CODE, value, endpoint);
    }
    inline void http_request_size(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_REQUEST_SIZE, value, endpoint);
    }
    inline void http_response_size(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.HTTP_RESPONSE_SIZE, value, endpoint);
    }
    inline void local_component(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.LOCAL_COMPONENT, value, endpoint);
    }
    inline void client_addr(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.CLIENT_ADDR, value, endpoint);
    }
    inline void server_addr(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        return annotate(g_zipkinCore_constants.SERVER_ADDR, value, endpoint);
    }
};

class CachedSpan : public Span
{
    uint8_t m_buf[0] __attribute__((aligned));

  public:
    CachedSpan(Tracer *tracer, const std::string &name, span_id_t parent_id = 0)
        : Span(tracer, name, parent_id)
    {
    }

    static void *operator new(size_t size, CachedTracer *tracer) noexcept;
    static void operator delete(void *ptr, std::size_t sz) noexcept;

    static size_t cache_offset(void) { return offsetof(CachedSpan, m_buf); }

    uint8_t *cache_ptr(void) { return &m_buf[0]; }
    size_t cache_size(void) const;

    void release(void);

    virtual Span *span(const std::string &name) const override;

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
    static const AnnotationType::type type = AnnotationType::type::BOOL;
};
template <>
struct __annotation<int16_t>
{
    static const AnnotationType::type type = AnnotationType::type::I16;
};
template <>
struct __annotation<int32_t>
{
    static const AnnotationType::type type = AnnotationType::type::I32;
};
template <>
struct __annotation<int64_t>
{
    static const AnnotationType::type type = AnnotationType::type::I64;
};
template <>
struct __annotation<double>
{
    static const AnnotationType::type type = AnnotationType::type::DOUBLE;
};
} // namespace __impl

template <>
inline void Span::annotate(const std::string &key, Endpoint *value, const Endpoint *endpoint)
{
    annotate(key, value);
}

template <typename T>
inline void Span::annotate(const std::string &key, T value, const Endpoint *endpoint)
{
    auto ptr = reinterpret_cast<uint8_t *>(&value);
    std::string data(reinterpret_cast<char *>(ptr), sizeof(T));

    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(data);
    annotation.__set_annotation_type(__impl::__annotation<T>::type);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_span.binary_annotations.push_back(annotation);
}

} // namespace zipkin