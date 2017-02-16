#include "Tracer.h"

namespace zipkin
{

Tracer *Tracer::create(Collector *collector, const std::string &name)
{
    return new CachedTracer(collector, name);
}

Span *CachedTracer::span(const std::string &name, span_id_t parent_id)
{
    Span *span = nullptr;

    if (m_spans.pop(span) && span)
    {
        span->reset(name, parent_id);
    }
    else
    {
        span = new (this) Span(this, name, parent_id);
    }

    return span;
}

} // namespace zipkin