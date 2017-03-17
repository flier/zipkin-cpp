#include "Tracer.h"

#include <glog/logging.h>

namespace zipkin
{
constexpr size_t CachedTracer::CACHE_LINE_SIZE;
constexpr size_t CachedTracer::DEFAULT_CACHE_MESSAGE_SIZE;
constexpr size_t CachedTracer::DEFAULT_CACHE_MESSAGE_COUNT;

Tracer *Tracer::create(Collector *collector, size_t sample_rate)
{
    return new CachedTracer(collector, sample_rate);
}

Span *CachedTracer::span(const std::string &name, span_id_t parent_id, void *userdata)
{
    Span *span = m_cache.get();

    if (span)
    {
        span->reset(name, parent_id, userdata);

        VLOG(2) << "Span @ " << span << " reused, id=" << std::hex << span->id();
    }
    else
    {
        span = new (this) CachedSpan(this, name, parent_id, userdata);
    }

    span->with_sampled(m_total_spans++ % m_sample_rate == 0);

    return span;
}

void CachedTracer::submit(Span *span)
{
    if (m_collector && (span->sampled() || span->debug()))
    {
        VLOG(2) << "Span @ " << span << " submited to collector @ " << m_collector << ", id=" << span->id();

        m_collector->submit(span);
    }
}

void CachedTracer::release(Span *span)
{
    VLOG(2) << "Span @ " << span << " released to tracer @ " << this << ", id=" << span->id();

    m_cache.release(static_cast<CachedSpan *>(span));
}

} // namespace zipkin