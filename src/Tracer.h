#pragma once

#include <cstddef>
#include <string>

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

  public: // Cached Tracer
    static Tracer *create(Collector *collector, const std::string &name, size_t capacity = 0);
};

} // namespace zipkin
