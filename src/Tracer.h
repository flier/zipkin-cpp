#pragma once

#include <cstddef>
#include <string>

#include "Span.h"

typedef uint64_t trace_id_t;

namespace zipkin
{

struct Tracer
{
    trace_id_t id;
    const std::string name;

    inline const Span span(const std::string &name) const { return Span(this, name); }
};

} // namespace zipkin
