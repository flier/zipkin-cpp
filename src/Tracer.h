#pragma once

#include <cstddef>
#include <string>

#include <boost/lockfree/stack.hpp>

#include "Span.h"
#include "Collector.h"

namespace zipkin
{

/**
* \brief A trace is a series of spans (often RPC calls) which form a latency tree.
*/
struct Tracer
{
    virtual ~Tracer() = default;

    /**
     * \brief Unique 8-byte identifier for a trace, set on all spans within it.
     *
     * \sa #id_high
     */
    virtual trace_id_t id(void) const = 0;
    /**
    * \sa #id_high
    */
    virtual void set_id(trace_id_t id) = 0;

    /**
    * When non-zero, the trace containing this span uses 128-bit trace identifiers.
    *
    * orresponds to the high bits in big-endian format and #id corresponds to the low bits.
    */
    virtual trace_id_t id_high(void) const = 0;
    /**
    * \sa #id_high
    */
    virtual void set_id_high(trace_id_t id_high) = 0;

    /**
    * \sa #id and #id_high
    */
    virtual void set_id(const x_trace_id_t &id) = 0;

    /**
    * \brief Tracer name in lowercase.
    */
    virtual const std::string &name(void) const = 0;

    /**
     * \brief Associated Collector
     */
    virtual Collector *collector(void) const = 0;

    /**
     * \brief Create new Span belongs to the Tracer
     *
     * First, CachedTracer will try to get a Span from internal cache instead of allocate it.
     * It is multi-thread safe since the internal cache base on the lock free stack.
     */
    virtual Span *span(const std::string &name, span_id_t parent_id = 0, userdata_t userdata = nullptr) = 0;

    /**
     * \brief Submit a Span to the associated Collector
     *
     * The Tracer will own the submited Span before it be released,
     * Collector will package the Span to internal message and send to the ZipKin server,
     * then the Span will be released to Tracer for reusing.
     */
    virtual void submit(Span *span) = 0;

    /**
     * \brief Release the Span when it ready to reuse.
     *
     * Don't release a Span which has been submited to a Tracer,
     * the Tracer will release it later after it can be reuse.
     */
    virtual void release(Span *span) = 0;

    /**
     * \brief Create a new Tracer.
     *
     * The default Tracer will cache Span for performance.
     */
    static Tracer *create(Collector *collector, const std::string &name);
};

template <size_t CAPACITY>
class SpanCache
{
    size_t m_message_size;

    boost::lockfree::stack<CachedSpan *, boost::lockfree::capacity<CAPACITY>> m_spans;

  public:
    SpanCache(size_t message_size) : m_message_size(message_size)
    {
    }

    ~SpanCache() { purge_all(); }

    size_t message_size(void) const { return m_message_size; }

    static constexpr size_t capacity(void) { return CAPACITY; }

    inline bool empty(void) const { return m_spans.empty(); }

    inline void purge_all(void)
    {
        m_spans.consume_all([](CachedSpan *span) -> void { delete span; });
    }

    inline CachedSpan *get(void)
    {
        CachedSpan *span = nullptr;

        return m_spans.pop(span) ? span : nullptr;
    }

    inline void release(CachedSpan *span)
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

    trace_id_t m_id, m_id_high;
    const std::string m_name;

  public:
    typedef SpanCache<64> span_cache_t;

  private:
    span_cache_t m_cache;

  public:
    CachedTracer(Collector *collector, const std::string &name, size_t cache_message_size = default_cache_message_size)
        : m_collector(collector), m_id(Span::next_id()), m_id_high(0), m_name(name), m_cache(cache_message_size)
    {
    }

    static const size_t cache_line_size;
    static const size_t default_cache_message_size;

    const span_cache_t &cache(void) const { return m_cache; }

    // Implement Tracer

    virtual trace_id_t id(void) const override { return m_id; }
    virtual void set_id(trace_id_t id_high) override { m_id = id_high; }
    virtual trace_id_t id_high(void) const override { return m_id_high; }
    virtual void set_id_high(trace_id_t id_high) override { m_id_high = id_high; }
    virtual void set_id(const x_trace_id_t &id) override
    {
        m_id = id.second;
        m_id_high = id.first;
    }
    virtual const std::string &name(void) const override { return m_name; }
    virtual Collector *collector(void) const override { return m_collector; }

    virtual Span *span(const std::string &name, span_id_t parent_id = 0, void *userdata = nullptr) override;

    virtual void submit(Span *span) override;

    virtual void release(Span *span) override;
};

} // namespace zipkin
