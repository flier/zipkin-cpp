#pragma once

#include <string>

#include "Span.h"

namespace zipkin
{

class Collector
{
  protected:
    virtual ~Collector() = default;

  public:
    static const int PARTITION_UA = -1;

    static Collector *create(const std::string &brokers, const std::string &topic, int partition = PARTITION_UA, const char *codec = nullptr);

    virtual void Submit(const Span &span) = 0;
};

} // namespace zipkin