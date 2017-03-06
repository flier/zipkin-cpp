#include "Propagation.h"

#include <stdio.h>

#include <folly/Conv.h>

#define CRLF "\r\n"

namespace zipkin
{

size_t Propagation::inject(char *buf, size_t size, const Span &span)
{
    char *p = buf, *end = buf + size;
    size_t wrote = 0;

    if (span.trace_id_high())
    {
        p += snprintf(p, end - p, ZIPKIN_X_TRACE_ID ": " ZIPKIN_SPAN_ID_FMT ZIPKIN_SPAN_ID_FMT CRLF, span.trace_id_high(), span.trace_id());
    }
    else
    {
        p += snprintf(p, end - p, ZIPKIN_X_TRACE_ID ": " ZIPKIN_SPAN_ID_FMT CRLF, span.trace_id());
    }

    p += snprintf(p, end - p, ZIPKIN_X_SPAN_ID ": " ZIPKIN_SPAN_ID_FMT CRLF, span.id());

    if (span.parent_id())
    {
        p += snprintf(p, end - p, ZIPKIN_X_PARENT_SPAN_ID ": " ZIPKIN_SPAN_ID_FMT CRLF, span.parent_id());
    }

    if (span.sampled())
    {
        p += snprintf(p, end - p, ZIPKIN_X_SAMPLED ": 1" CRLF);
    }

    if (span.debug())
    {
        p += snprintf(p, end - p, ZIPKIN_X_FLAGS ": 1" CRLF);
    }

    return p - buf;
}

#ifdef WITH_CURL
struct curl_slist *Propagation::inject(struct curl_slist *headers, const Span &span)
{
    char buf[128];

    if (span.trace_id_high())
    {
        snprintf(buf, sizeof(buf), ZIPKIN_X_TRACE_ID ": " ZIPKIN_SPAN_ID_FMT ZIPKIN_SPAN_ID_FMT, span.trace_id_high(), span.trace_id());
        headers = curl_slist_append(headers, buf);
    }
    else
    {
        snprintf(buf, sizeof(buf), ZIPKIN_X_TRACE_ID ": " ZIPKIN_SPAN_ID_FMT, span.trace_id());
        headers = curl_slist_append(headers, buf);
    }

    snprintf(buf, sizeof(buf), ZIPKIN_X_SPAN_ID ": " ZIPKIN_SPAN_ID_FMT, span.id());
    headers = curl_slist_append(headers, buf);

    if (span.parent_id())
    {
        snprintf(buf, sizeof(buf), ZIPKIN_X_PARENT_SPAN_ID ": " ZIPKIN_SPAN_ID_FMT, span.parent_id());
        headers = curl_slist_append(headers, buf);
    }

    if (span.sampled())
    {
        snprintf(buf, sizeof(buf), ZIPKIN_X_SAMPLED ": 1");
        headers = curl_slist_append(headers, buf);
    }

    if (span.debug())
    {
        snprintf(buf, sizeof(buf), ZIPKIN_X_FLAGS ": 1");
        headers = curl_slist_append(headers, buf);
    }

    return headers;
}

#endif // WITH_CURL

#ifdef WITH_GRPC

void Propagation::inject(grpc::ClientContext &context, const Span &span)
{
    context.AddMetadata(ZIPKIN_X_TRACE_ID_LOWERCASE, folly::to<std::string>(span.trace_id()));
    context.AddMetadata(ZIPKIN_X_SPAN_ID_LOWERCASE, folly::to<std::string>(span.id()));

    if (span.parent_id())
        context.AddMetadata(ZIPKIN_X_PARENT_SPAN_ID_LOWERCASE, folly::to<std::string>(span.parent_id()));
    if (span.sampled())
        context.AddMetadata(ZIPKIN_X_SAMPLED_LOWERCASE, "1");
    if (span.debug())
        context.AddMetadata(ZIPKIN_X_FLAGS_LOWERCASE, "1");
}

void Propagation::extract(grpc::ServerContext &context, Span &span)
{
    for (auto &item : context.client_metadata())
    {
        const std::string value(item.second.begin(), item.second.end());

        if (item.first == ZIPKIN_X_TRACE_ID_LOWERCASE)
        {
            span.with_trace_id(folly::to<uint64_t>(value));
        }
        else if (item.first == ZIPKIN_X_SPAN_ID_LOWERCASE)
        {
            span.with_parent_id(folly::to<uint64_t>(value));
        }
        else if (item.first == ZIPKIN_X_SAMPLED_LOWERCASE)
        {
            span.with_sampled(folly::to<int>(value));
        }
        else if (item.first == ZIPKIN_X_FLAGS_LOWERCASE)
        {
            span.with_debug(folly::to<int>(value));
        }
    }
}

#endif // WITH_GRPC

} // namespace zipkin