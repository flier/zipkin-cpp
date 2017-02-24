#include <random>
#include <utility>

#include <ios>
#include <cstdlib>
#include <string>
#include <locale>
#include <codecvt>

#include <arpa/inet.h>

#include <glog/logging.h>

#include "Span.h"
#include "Tracer.h"

namespace zipkin
{

Span::Span(Tracer *tracer, const std::string &name, span_id_t parent_id, userdata_t userdata) : m_tracer(tracer)
{
    if (tracer)
        m_span.__set_trace_id(tracer->id());

    reset(name, parent_id, userdata);
}

void Span::reset(const std::string &name, span_id_t parent_id, userdata_t userdata)
{
    m_span.__isset = _Span__isset();
    m_span.__set_name(name);
    m_span.__set_id(next_id());
    m_span.__set_timestamp(now().count());
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
        m_span.__set_duration(now().count() - m_span.timestamp);

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
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
}

void Span::annotate(const std::string &value, const Endpoint *endpoint)
{
    ::Annotation annotation;

    annotation.__set_timestamp(now().count());
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

    return p;
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