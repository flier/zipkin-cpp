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
    in_addr_t addr;
    uint16_t port;
    const std::string service;

    const ::Endpoint host(void) const;
};

struct Tracer;

typedef ::Span SpanMessage;

class Span
{
    std::shared_ptr<SpanMessage> m_message;
    Tracer *m_tracer;

    static const ::Endpoint host(const Endpoint *endpoint);

  public:
    Span(Tracer *tracer, const std::string &name, span_id_t parent_id = 0);

    void reset(const std::string &name, span_id_t parent_id = 0);

    void submit(void);

    static uint64_t next_id();

    static timestamp_t now();

    std::shared_ptr<SpanMessage> message(void) const { return m_message; }

    const std::string &name(void) const;
    span_id_t id(void) const;
    span_id_t parent_id(void) const;
    std::chrono::time_point<std::chrono::system_clock> timestamp(void) const;
    std::chrono::microseconds duration(void) const;

    const Span span(const std::string &name) const;

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

    void annotate(const std::string &key, const std::vector<uint8_t> &value, const Endpoint *endpoint = nullptr);
    void annotate(const std::string &key, const std::string &value, const Endpoint *endpoint = nullptr);
    void annotate(const std::string &key, const std::wstring &value, const Endpoint *endpoint = nullptr);

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

template <typename T>
void Span::annotate(const std::string &key, T value, const Endpoint *endpoint)
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

    m_message->binary_annotations.push_back(annotation);
}

} // namespace zipkin