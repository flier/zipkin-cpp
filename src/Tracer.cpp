#include "Tracer.h"

#include <glog/logging.h>

namespace zipkin
{

const size_t CachedTracer::cache_line_size = 64;
const size_t CachedTracer::default_cache_message_size = 2048;

Tracer *Tracer::create(Collector *collector, const std::string &name)
{
    return new CachedTracer(collector, name);
}

Span *CachedTracer::span(const std::string &name, span_id_t parent_id)
{
    Span *span = m_cache.get();

    if (span)
    {
        span->reset(name, parent_id);

        VLOG(2) << "Span @ " << span << " reused, id=" << std::hex << span->id();
    }
    else
    {
        span = new (this) Span(this, name, parent_id);
    }

    return span;
}

void CachedTracer::submit(Span *span)
{
    if (m_collector)
        m_collector->submit(span);
}

} // namespace zipkin