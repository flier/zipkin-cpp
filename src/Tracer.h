#pragma once

#include <cstddef>
#include <string>

#include <boost/lockfree/stack.hpp>

#include "Span.h"
#include "Collector.h"

typedef uint64_t trace_id_t;

namespace zipkin
{

struct Tracer
{
    virtual ~Tracer() = default;

    virtual trace_id_t id(void) const = 0;

    virtual const std::string &name(void) const = 0;

    virtual Span *span(const std::string &name, span_id_t parent_id = 0) = 0;

    virtual void submit(Span *span) = 0;

    // Create Cached Tracer
    static Tracer *create(Collector *collector, const std::string &name);
};

class CachedTracer : public Tracer
{
    Collector *m_collector;

    trace_id_t m_id;
    const std::string m_name;
    size_t m_cache_message_size;

    boost::lockfree::stack<Span *> m_spans;

  public:
    CachedTracer(Collector *collector, const std::string &name, size_t cache_message_size = 2048)
        : m_collector(collector), m_id(Span::next_id()), m_name(name), m_cache_message_size(cache_message_size)
    {
    }
    virtual ~CachedTracer()
    {
        m_spans.consume_all([](Span *span) -> void { delete span; });
    }

    static size_t cache_line_size(void) { return 64; }
    size_t cache_message_size(void) const { return m_cache_message_size; }

    virtual trace_id_t id(void) const override { return m_id; }
    virtual const std::string &name(void) const override { return m_name; }

    virtual Span *span(const std::string &name, span_id_t parent_id = 0) override;

    virtual void submit(Span *span) override { m_collector->submit(span); }

    void release(Span *span) { m_spans.push(span); }
};

} // namespace zipkin
