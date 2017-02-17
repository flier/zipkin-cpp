#include <random>
#include <utility>

#include <ios>
#include <cstdlib>
#include <string>
#include <locale>
#include <codecvt>

#include <glog/logging.h>

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

Span::Span(Tracer *tracer, const std::string &name, span_id_t parent_id) : m_tracer(tracer)
{
    if (tracer)
        m_span.__set_trace_id(tracer->id());

    reset(name, parent_id);
}

void Span::reset(const std::string &name, span_id_t parent_id)
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
}

void Span::submit(void)
{
    if (m_span.timestamp)
        m_span.__set_duration(now() - m_span.timestamp);

    if (m_tracer)
        m_tracer->submit(this);
}

void Span::release(void)
{
    if (m_tracer)
    {
        VLOG(2) << "Span @ " << this << " released to tracer @ " << m_tracer << ", id=" << id();

        m_tracer->release(this);
    }
    else
    {
        delete this;
    }
}

size_t Span::cache_size(void) const
{
    return static_cast<CachedTracer *>(m_tracer)->cache().message_size() - cache_offset();
}

void *Span::operator new(size_t size, CachedTracer *tracer) noexcept
{
    size_t sz = tracer ? tracer->cache().message_size() : size;
    void *p = nullptr;

    if (posix_memalign(&p, CachedTracer::cache_line_size, sz))
        return nullptr;

    VLOG(2) << "Span @ " << p << " allocated with " << sz << " bytes";

    return ::operator new(sz, p);
}

void Span::operator delete(void *ptr, std::size_t sz)
{
    Span *span = static_cast<Span *>(ptr);

    VLOG(2) << "Span @ " << ptr << " deleted with " << sz << " bytes, id=" << std::hex << span->id();

    free(ptr);
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

Span *Span::span(const std::string &name) const
{
    if (m_tracer)
    {
        return m_tracer->span(m_span.name, m_span.id);
    }
    else
    {
        return new (nullptr) Span(nullptr, m_span.name, m_span.id);
    }
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

} // namespace zipkin