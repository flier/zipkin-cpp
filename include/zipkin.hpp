#pragma once

/**
* HTTP headers are used to pass along trace information.
*
* The B3 portion of the header is so named for the original name of Zipkin: BigBrotherBird.
*
* Ids are encoded as hex strings
*
* \sa https://github.com/openzipkin/b3-propagation
*/

#define ZIPKIN_X_TRACE_ID "X-B3-TraceId"            ///< 128 or 64 lower-hex encoded bits (required)
#define ZIPKIN_X_SPAN_ID "X-B3-SpanId"              ///< 64 lower-hex encoded bits (required)
#define ZIPKIN_X_PARENT_SPAN_ID "X-B3-ParentSpanId" ///< 64 lower-hex encoded bits (absent on root span)
#define ZIPKIN_X_SAMPLED "X-B3-Sampled"             ///< Boolean (either “1” or “0”, can be absent)
#define ZIPKIN_X_FLAGS "X-B3-Flags"                 ///< “1” means debug (can be absent)

#include "Version.h"
#include "Span.h"
#include "Tracer.h"
#include "Collector.h"
#include "KafkaCollector.h"
#include "HttpCollector.h"