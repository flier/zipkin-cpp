#include "Tracer.h"

#include <boost/lockfree/stack.hpp>

namespace zipkin
{

class CachedTracer : public Tracer
{
    Collector *m_collector;

    trace_id_t m_id;
    const std::string m_name;

    boost::lockfree::stack<Span *> m_spans;

  public:
    CachedTracer(Collector *collector, const std::string &name, size_t capacity = 0)
        : m_collector(collector), m_id(Span::next_id()), m_name(name)
    {
    }
    virtual ~CachedTracer()
    {
        m_spans.consume_all([](Span *span) -> void { delete span; });
    }

    virtual trace_id_t id(void) const override { return m_id; }
    virtual const std::string &name(void) const override { return m_name; }

    virtual Span *span(const std::string &name, span_id_t parent_id = 0) override
    {
        Span *span = nullptr;

        if (m_spans.pop(span) && span)
        {
            span->reset(name, parent_id);
        }
        else
        {
            span = new Span(this, name, parent_id);
        }

        return span;
    }

    virtual void submit(Span *span) override
    {
        m_collector->submit(span);
        m_spans.push(span);
    }
};

Tracer *Tracer::create(Collector *collector, const std::string &name, size_t capacity)
{
    return new CachedTracer(collector, name, capacity);
}

} // namespace zipkin