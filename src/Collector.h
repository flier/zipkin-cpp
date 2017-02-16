#pragma once

#include <string>

#include "Span.h"

namespace zipkin
{

class Collector
{
  virtual ~Collector() = default;

  virtual void submit(const Span *span) = 0;

public: // Kafka Collector
  static const int PARTITION_UA = -1;

  static Collector *create(const std::string &brokers, const std::string &topic, int partition = PARTITION_UA, const char *codec = nullptr);
};

} // namespace zipkin