#include <random>
#include <utility>

#include <string>
#include <locale>
#include <codecvt>

#include "Span.h"
#include "Tracer.h"

namespace zipkin
{

const ::Endpoint Endpoint::host() const
{
    ::Endpoint host;

    host.__set_ipv4(addr);
    host.__set_port(port);
    host.__set_service_name(service);

    return host;
}

Span::Span(const Tracer *tracer, const std::string &name, span_id_t parent_id) : m_message(new ::Span()), m_tracer(tracer)
{
    m_message->__set_trace_id(tracer->id);
    m_message->__set_name(name);
    m_message->__set_id(next_id());
    m_message->__set_timestamp(now());

    if (parent_id)
    {
        m_message->__set_parent_id(parent_id);
    }
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

const std::string &Span::name(void) const { return m_message->name; }

span_id_t Span::id(void) const { return m_message->id; }

span_id_t Span::parent_id(void) const { return m_message->parent_id; }

std::chrono::time_point<std::chrono::system_clock> Span::timestamp(void) const
{
    std::chrono::microseconds ts(m_message->timestamp);

    return std::chrono::system_clock::from_time_t(std::chrono::duration_cast<std::chrono::seconds>(ts).count());
}

std::chrono::microseconds Span::duration(void) const
{
    return std::chrono::microseconds(m_message->duration);
}

const Span Span::span(const std::string &name) const { return Span(m_tracer, name, m_message->id); }

void Span::annotate(const std::string &value, const Endpoint *endpoint)
{
    ::Annotation annotation;

    annotation.__set_timestamp(now());
    annotation.__set_value(value);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_message->annotations.push_back(annotation);
}

void Span::annotate(const std::string &key, const std::vector<uint8_t> &value, const Endpoint *endpoint)
{
    ::BinaryAnnotation annotation;

    annotation.__set_key(key);
    annotation.__set_value(std::string(value.begin(), value.end()));
    annotation.__set_annotation_type(AnnotationType::type::BYTES);

    if (endpoint)
    {
        annotation.__set_host(endpoint->host());
    }

    m_message->binary_annotations.push_back(annotation);
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

    m_message->binary_annotations.push_back(annotation);
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

    m_message->binary_annotations.push_back(annotation);
}

} // namespace zipkin