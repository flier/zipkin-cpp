#pragma once

#include "Config.h"

#ifdef WITH_CURL
#include <curl/curl.h>
#endif

#ifdef WITH_GRPC
#include <grpc++/grpc++.h>
#endif

#include "Span.h"

#ifdef __APPLE__
#define ZIPKIN_SPAN_ID_FMT "%016llx"
#else
#define ZIPKIN_SPAN_ID_FMT "%016lx"
#endif

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

#define ZIPKIN_X_TRACE_ID_LOWERCASE "x-b3-traceid"            ///< 128 or 64 lower-hex encoded bits (required)
#define ZIPKIN_X_SPAN_ID_LOWERCASE "x-b3-spanid"              ///< 64 lower-hex encoded bits (required)
#define ZIPKIN_X_PARENT_SPAN_ID_LOWERCASE "x-b3-parentspanid" ///< 64 lower-hex encoded bits (absent on root span)
#define ZIPKIN_X_SAMPLED_LOWERCASE "x-b3-sampled"             ///< Boolean (either “1” or “0”, can be absent)
#define ZIPKIN_X_FLAGS_LOWERCASE "x-b3-flags"                 ///< “1” means debug (can be absent)

namespace zipkin
{

class Propagation
{
  public:
    static size_t inject(char *buf, size_t size, const Span &span);

#ifdef WITH_CURL
    static struct curl_slist *inject(struct curl_slist *headers, const Span &span);
#endif

#ifdef WITH_GRPC
    static void inject(::grpc::ClientContext &context, const Span &span);

    static void extract(::grpc::ServerContext &context, Span &span);
#endif
};

#ifdef WITH_GRPC
inline ::grpc::ClientContext &operator<<(::grpc::ClientContext &context, const Span &span)
{
    Propagation::inject(context, span);

    return context;
}
#endif

} // namespace zipkin