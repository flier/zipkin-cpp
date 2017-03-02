#include "Tracer.h"

#include <glog/logging.h>

namespace zipkin
{

const size_t CachedTracer::cache_line_size = 64;
const size_t CachedTracer::default_cache_message_size = 4096;

Tracer *Tracer::create(Collector *collector, const std::string &name)
{
    return new CachedTracer(collector, name);
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
    if (m_collector && span->sampled())
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