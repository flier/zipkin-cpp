#pragma once

#include <cstddef>
#include <string>
#include <atomic>

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
    * \brief Tracer sample rate
    */
    virtual size_t sample_rate(void) const = 0;

    /** \sa Tracer#sample_rate */
    virtual void set_sample_rate(size_t sample_rate) = 0;

    /**
     * \brief Associated user data
     */
    virtual userdata_t userdata(void) const = 0;

    /** \sa Span#userdata */
    virtual void set_userdata(userdata_t userdata) = 0;

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
    static Tracer *create(Collector *collector, size_t sample_rate = 1);
};

class SpanCache
{
    size_t m_message_size, m_message_capacity;

    boost::lockfree::stack<CachedSpan *> m_spans;

  public:
    SpanCache(size_t message_size, size_t message_capacity)
        : m_message_size(message_size), m_message_capacity(message_capacity), m_spans(message_capacity)
    {
    }

    ~SpanCache() { purge_all(); }

    size_t message_size(void) const { return m_message_size; }

    size_t message_capacity(void) { return m_message_capacity; }

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

    size_t m_sample_rate = 1;
    std::atomic_size_t m_total_spans = ATOMIC_VAR_INIT(0);

    userdata_t m_userdata = nullptr;

  private:
    SpanCache m_cache;

  public:
    CachedTracer(Collector *collector,
                 size_t sample_rate = 1,
                 size_t cache_message_size = DEFAULT_CACHE_MESSAGE_SIZE,
                 size_t cache_message_count = DEFAULT_CACHE_MESSAGE_COUNT)
        : m_collector(collector), m_sample_rate(sample_rate),
          m_cache(cache_message_size, cache_message_count)
    {
    }

    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t DEFAULT_CACHE_MESSAGE_SIZE = 4096;
    static constexpr size_t DEFAULT_CACHE_MESSAGE_COUNT = 64;

    const SpanCache &cache(void) const { return m_cache; }

    // Implement Tracer

    virtual size_t sample_rate(void) const override { return m_sample_rate; }
    virtual void set_sample_rate(size_t sample_rate) override { m_sample_rate = sample_rate; }

    virtual userdata_t userdata(void) const override { return m_userdata; }
    virtual void set_userdata(userdata_t userdata) override { m_userdata = userdata; }

    virtual Collector *collector(void) const override { return m_collector; }

    virtual Span *span(const std::string &name, span_id_t parent_id = 0, void *userdata = nullptr) override;

    virtual void submit(Span *span) override;

    virtual void release(Span *span) override;
};

} // namespace zipkin
