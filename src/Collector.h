#pragma once

#include <chrono>

#include "Span.h"

namespace zipkin
{

/**
* \brief Push Span as messages to transport
*/
struct Collector
{
  virtual ~Collector() = default;

  /**
  * \brief Async submit Span to transport
  *
  * \param span encoded to message and send to the transport,
  *
  * Span#release will be call after the message was sent.
  */
  virtual void submit(Span *span) = 0;

  /**
  * \brief Wait until all outstanding messages, et.al, are sent.
  *
  * \param timeout_ms the maximum amount of time (in milliseconds) that the call will block waiting.
  * \return \c true if all outstanding messages were sent, or \c false if the \p timeout_ms was reached.
  */
  virtual bool flush(std::chrono::milliseconds timeout_ms) = 0;
};

} // namespace zipkin