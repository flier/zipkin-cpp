#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <vector>

#include "../gen-cpp/zipkinCore_constants.h"

typedef uint64_t span_id_t;
typedef uint64_t timestamp_t;

uint64_t next_id();

timestamp_t timestamp();

namespace zipkin
{

struct Endpoint
{
    const char *service;
    in_addr_t addr;
    uint16_t port;
};

enum AnnotationType
{
    BOOL = 0,
    BYTES = 1,
    I16 = 2,
    I32 = 3,
    I64 = 4,
    DOUBLE = 5,
    STRING = 6
};

struct Annotation
{
    timestamp_t ts;
    const std::string value;
    const Endpoint *endpoint;
};

struct TypedAnnotation
{
    const std::string key;
    std::vector<uint8_t> value;
    AnnotationType type;
    const Endpoint *endpoint;
};

struct Tracer;

struct Span
{
    const Tracer *tracer;
    const std::string name;
    span_id_t id;
    span_id_t parent;
    bool sampled;
    timestamp_t ts;
    std::vector<Annotation> annotates;
    std::vector<TypedAnnotation> typed_annotates;

    inline void annotate(const std::string &value, const Endpoint *endpoint = nullptr)
    {
        Annotation annotation = {timestamp(), value, endpoint};

        annotates.push_back(annotation);
    }

    template <typename T>
    void annotate(const std::string &key, T value, const Endpoint *endpoint = nullptr);

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
void Span::annotate(const std::string &key, T value, const Endpoint *endpoint)
{
    auto ptr = reinterpret_cast<uint8_t *>(&value);

    TypedAnnotation annotation = {
        key,                                        // key
        std::vector<uint8_t>(ptr, ptr + sizeof(T)), // value
        __impl::__annotation<T>::type,              // type
        endpoint,                                   // endpoint
    };

    typed_annotates.push_back(annotation);
}

} // namespace zipkin