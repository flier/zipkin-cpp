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

    virtual void release(Span *span) = 0;

    // Create Cached Tracer
    static Tracer *create(Collector *collector, const std::string &name);
};

class SpanCache
{
    size_t m_message_size;

    boost::lockfree::stack<Span *, boost::lockfree::capacity<64>> m_spans;

  public:
    SpanCache(size_t message_size) : m_message_size(message_size)
    {
    }

    ~SpanCache() { purge_all(); }

    size_t message_size(void) const { return m_message_size; }

    inline bool empty(void) const { return m_spans.empty(); }

    inline void purge_all(void)
    {
        m_spans.consume_all([](Span *span) -> void { delete span; });
    }

    inline Span *get(void)
    {
        Span *span = nullptr;

        return m_spans.pop(span) ? span : nullptr;
    }

    inline void release(Span *span)
    {
        if (!m_spans.bounded_push(span))
        {
            delete span;
        }
    }
};

class CachedTracer : public Tracer
{
    Collector *m_collector;

    trace_id_t m_id;
    const std::string m_name;

    SpanCache m_cache;

  public:
    CachedTracer(Collector *collector, const std::string &name, size_t cache_message_size = default_cache_message_size)
        : m_collector(collector), m_id(Span::next_id()), m_name(name), m_cache(cache_message_size)
    {
    }

    static const size_t cache_line_size;
    static const size_t default_cache_message_size;

    const SpanCache &cache(void) const { return m_cache; }

    // Implement Tracer

    virtual trace_id_t id(void) const override { return m_id; }
    virtual const std::string &name(void) const override { return m_name; }

    virtual Span *span(const std::string &name, span_id_t parent_id = 0) override;

    virtual void submit(Span *span) override;

    virtual void release(Span *span) override { m_cache.release(span); }
};

} // namespace zipkin
