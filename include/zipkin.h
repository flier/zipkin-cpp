#pragma once

#include <stddef.h>
#include <stdint.h>

#include <sys/socket.h>

#ifdef WITH_CURL
#include <curl/curl.h>
#endif

#include "Config.h"
#include "Version.h"

#define ZIPKIN_CLIENT_SEND "cs"
#define ZIPKIN_CLIENT_RECV "cr"
#define ZIPKIN_SERVER_SEND "ss"
#define ZIPKIN_SERVER_RECV "sr"
#define ZIPKIN_WIRE_SEND "ws"
#define ZIPKIN_WIRE_RECV "wr"
#define ZIPKIN_CLIENT_SEND_FRAGMENT "csf"
#define ZIPKIN_CLIENT_RECV_FRAGMENT "crf"
#define ZIPKIN_SERVER_SEND_FRAGMENT "ssf"
#define ZIPKIN_SERVER_RECV_FRAGMENT "srf"

#define ZIPKIN_HTTP_HOST "http.host"
#define ZIPKIN_HTTP_METHOD "http.method"
#define ZIPKIN_HTTP_PATH "http.path"
#define ZIPKIN_HTTP_URL "http.url"
#define ZIPKIN_HTTP_STATUS_CODE "http.status_code"
#define ZIPKIN_HTTP_REQUEST_SIZE "http.request.size"
#define ZIPKIN_HTTP_RESPONSE_SIZE "http.response.size"
#define ZIPKIN_LOCAL_COMPONENT "lc"
#define ZIPKIN_CLIENT_ADDR "ca"
#define ZIPKIN_SERVER_ADDR "sa"

#define ZIPKIN_ERROR "error"

#define ZIPKIN_COMPRESSION_GZIP "gzip"
#define ZIPKIN_COMPRESSION_SNAPPY "snappy"
#define ZIPKIN_COMPRESSION_LZ4 "lz4"
#define ZIPKIN_COMPRESSION_NONE "none"

#define ZIPKIN_ENCODING_BINARY "binary"
#define ZIPKIN_ENCODING_JSON "json"
#define ZIPKIN_ENCODING_PRETTY_JSON "pretty_json"

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

#define ZIPKIN_X_FLAG_DEBUG (1 << 0)

typedef uint64_t zipkin_span_id_t;
typedef uint64_t zipkin_trace_id_t;
typedef void *zipkin_userdata_t;
typedef uint16_t port_t;

typedef void *zipkin_endpoint_t;
typedef void *zipkin_span_t;
typedef void *zipkin_tracer_t;
typedef void *zipkin_kafka_conf_t;
#ifdef WITH_CURL
typedef void *zipkin_http_conf_t;
#endif
typedef void *zipkin_scribe_conf_t;
typedef void *zipkin_xray_conf_t;
typedef void *zipkin_collector_t;

#ifdef __cplusplus
extern "C" {
#endif

enum zipkin_logger_level_t
{
    LOG_WARN = 0,
    LOG_INFO = 1,
    LOG_DEBUG = 2,
    LOG_TRACE = 3,
};

void zipkin_set_logging_level(enum zipkin_logger_level_t level);

zipkin_endpoint_t zipkin_endpoint_new(const char *service, struct sockaddr *addr);
void zipkin_endpoint_free(zipkin_endpoint_t endpoint);

const char *zipkin_endpoint_service_name(zipkin_endpoint_t endpoint);
void zipkin_endpoint_addr(zipkin_endpoint_t endpoint, struct sockaddr *addr, size_t len);

zipkin_span_t zipkin_span_new(zipkin_tracer_t tracer, const char *name, zipkin_userdata_t userdata);
zipkin_span_t zipkin_span_new_child(zipkin_span_t span, const char *name, zipkin_userdata_t userdata);
void zipkin_span_release(zipkin_span_t span);

zipkin_span_id_t zipkin_span_id(zipkin_span_t span);
zipkin_span_t zipkin_span_set_id(zipkin_span_t span, zipkin_span_id_t id);

const char *zipkin_span_name(zipkin_span_t span);

zipkin_span_id_t zipkin_span_parent_id(zipkin_span_t span);
zipkin_span_t zipkin_span_set_parent_id(zipkin_span_t span, zipkin_span_id_t id);

zipkin_tracer_t zipkin_span_tracer(zipkin_span_t span);

zipkin_trace_id_t zipkin_span_trace_id(zipkin_span_t span);
zipkin_span_t zipkin_span_set_trace_id(zipkin_span_t span, zipkin_trace_id_t id);

zipkin_trace_id_t zipkin_span_trace_id_high(zipkin_span_t span);
zipkin_span_t zipkin_span_set_trace_id_high(zipkin_span_t span, zipkin_trace_id_t id);

zipkin_span_t zipkin_span_parse_trace_id(zipkin_span_t span, const char *str, size_t len);

int zipkin_span_debug(zipkin_span_t span);
zipkin_span_t zipkin_span_set_debug(zipkin_span_t span, int debug);

int zipkin_span_sampled(zipkin_span_t span);
zipkin_span_t zipkin_span_set_sampled(zipkin_span_t span, int sampled);

zipkin_userdata_t zipkin_span_userdata(zipkin_span_t span);
zipkin_span_t zipkin_span_set_userdata(zipkin_span_t span, zipkin_userdata_t userdata);

#define ANNOTATE(span, value, endpoint)                  \
    if (span)                                            \
    {                                                    \
        zipkin_span_annotate(span, value, -1, endpoint); \
    }
#define ANNOTATE_IF(expr, span, value, endpoint)         \
    if (span && (expr))                                  \
    {                                                    \
        zipkin_span_annotate(span, value, -1, endpoint); \
    }

#define ANNOTATE_BOOL(span, key, value, endpoint)              \
    if (span)                                                  \
    {                                                          \
        zipkin_span_annotate_bool(span, key, value, endpoint); \
    }
#define ANNOTATE_BOOL_IF(expr, span, key, value, endpoint)     \
    if (span && (expr))                                        \
    {                                                          \
        zipkin_span_annotate_bool(span, key, value, endpoint); \
    }

#define ANNOTATE_BYTES(span, key, value, len, endpoint)              \
    if (span)                                                        \
    {                                                                \
        zipkin_span_annotate_bytes(span, key, value, len, endpoint); \
    }
#define ANNOTATE_BYTES_IF(expr, span, key, value, len, endpoint)     \
    if (span && (expr))                                              \
    {                                                                \
        zipkin_span_annotate_bytes(span, key, value, len, endpoint); \
    }

#define ANNOTATE_INT16(span, key, value, endpoint)              \
    if (span)                                                   \
    {                                                           \
        zipkin_span_annotate_int16(span, key, value, endpoint); \
    }
#define ANNOTATE_INT16_IF(expr, span, key, value, endpoint)     \
    if (span && (expr))                                         \
    {                                                           \
        zipkin_span_annotate_int16(span, key, value, endpoint); \
    }

#define ANNOTATE_INT32(span, key, value, endpoint)              \
    if (span)                                                   \
    {                                                           \
        zipkin_span_annotate_int32(span, key, value, endpoint); \
    }
#define ANNOTATE_INT32_IF(expr, span, key, value, endpoint)     \
    if (span && (expr))                                         \
    {                                                           \
        zipkin_span_annotate_int32(span, key, value, endpoint); \
    }

#define ANNOTATE_INT64(span, key, value, endpoint)              \
    if (span)                                                   \
    {                                                           \
        zipkin_span_annotate_int64(span, key, value, endpoint); \
    }
#define ANNOTATE_INT64_IF(expr, span, key, value, endpoint)     \
    if (span && (expr))                                         \
    {                                                           \
        zipkin_span_annotate_int64(span, key, value, endpoint); \
    }

#define ANNOTATE_DOUBLE(span, key, value, endpoint)              \
    if (span)                                                    \
    {                                                            \
        zipkin_span_annotate_double(span, key, value, endpoint); \
    }
#define ANNOTATE_DOUBLE_IF(expr, span, key, value, endpoint)     \
    if (span && (expr))                                          \
    {                                                            \
        zipkin_span_annotate_double(span, key, value, endpoint); \
    }

#define ANNOTATE_STR(span, key, value, len, endpoint)              \
    if (span)                                                      \
    {                                                              \
        zipkin_span_annotate_str(span, key, value, len, endpoint); \
    }
#define ANNOTATE_STR_IF(expr, span, key, value, len, endpoint)     \
    if (span && (expr))                                            \
    {                                                              \
        zipkin_span_annotate_str(span, key, value, len, endpoint); \
    }

void zipkin_span_annotate(zipkin_span_t span, const char *value, int len, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_bool(zipkin_span_t span, const char *key, int value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_bytes(zipkin_span_t span, const char *key, const char *value, size_t len, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_int16(zipkin_span_t span, const char *key, int16_t value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_int32(zipkin_span_t span, const char *key, int32_t value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_int64(zipkin_span_t span, const char *key, int64_t value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_double(zipkin_span_t span, const char *key, double value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_str(zipkin_span_t span, const char *key, const char *value, int len, zipkin_endpoint_t endpoint);

void zipkin_span_submit(zipkin_span_t span);

zipkin_tracer_t zipkin_tracer_new(zipkin_collector_t collector);
void zipkin_tracer_free(zipkin_tracer_t tracer);

zipkin_collector_t zipkin_tracer_collector(zipkin_tracer_t tracer);

size_t zipkin_tracer_sample_rate(zipkin_tracer_t tracer);
void zipkin_tracer_set_sample_rate(zipkin_tracer_t tracer, size_t sample_rate);

zipkin_userdata_t zipkin_tracer_userdata(zipkin_tracer_t tracer);
void zipkin_tracer_set_userdata(zipkin_tracer_t tracer, zipkin_userdata_t userdata);

zipkin_kafka_conf_t zipkin_kafka_conf_new(const char *brokers, const char *topic);
void zipkin_kafka_conf_free(zipkin_kafka_conf_t conf);
void zipkin_kafka_conf_set_partition(zipkin_kafka_conf_t conf, int partition);
void zipkin_kafka_conf_set_compression_codec(zipkin_kafka_conf_t conf, const char *codec);
void zipkin_kafka_conf_set_message_codec(zipkin_kafka_conf_t conf, const char *codec);
void zipkin_kafka_conf_set_batch_num_messages(zipkin_kafka_conf_t conf, size_t batch_num_messages);
void zipkin_kafka_conf_set_queue_buffering_max_messages(zipkin_kafka_conf_t conf, size_t queue_buffering_max_messages);
void zipkin_kafka_conf_set_queue_buffering_max_kbytes(zipkin_kafka_conf_t conf, size_t queue_buffering_max_kbytes);
void zipkin_kafka_conf_set_queue_buffering_max_ms(zipkin_kafka_conf_t conf, size_t queue_buffering_max_ms);
void zipkin_kafka_conf_set_message_send_max_retries(zipkin_kafka_conf_t conf, size_t message_send_max_retries);

#ifdef WITH_CURL
zipkin_http_conf_t zipkin_http_conf_new(const char *url);
void zipkin_http_conf_free(zipkin_http_conf_t conf);
void zipkin_http_conf_set_proxy(zipkin_http_conf_t conf, const char *proxy, int tunnel);
void zipkin_http_conf_set_message_codec(zipkin_http_conf_t conf, const char *codec);
void zipkin_http_conf_set_batch_size(zipkin_http_conf_t conf, size_t batch_size);
void zipkin_http_conf_set_backlog(zipkin_http_conf_t conf, size_t backlog);
void zipkin_http_conf_set_max_redirect_times(zipkin_http_conf_t conf, size_t max_redirect_times);
void zipkin_http_conf_set_connect_timeout(zipkin_http_conf_t conf, size_t connect_timeout_ms);
void zipkin_http_conf_set_request_timeout(zipkin_http_conf_t conf, size_t request_timeout_ms);
void zipkin_http_conf_set_batch_interval(zipkin_http_conf_t conf, size_t batch_interval_ms);
#endif

zipkin_scribe_conf_t zipkin_scribe_conf_new(const char *url);
void zipkin_scribe_conf_free(zipkin_scribe_conf_t conf);
void zipkin_scribe_conf_set_message_codec(zipkin_scribe_conf_t conf, const char *codec);
void zipkin_scribe_conf_set_batch_size(zipkin_scribe_conf_t conf, size_t batch_size);
void zipkin_scribe_conf_set_backlog(zipkin_scribe_conf_t conf, size_t backlog);
void zipkin_scribe_conf_set_max_retry_times(zipkin_scribe_conf_t conf, size_t max_redirect_times);
void zipkin_scribe_conf_set_batch_interval(zipkin_scribe_conf_t conf, size_t batch_interval_ms);

zipkin_xray_conf_t zipkin_xray_conf_new(const char *host, port_t port);
void zipkin_xray_conf_free(zipkin_xray_conf_t conf);
void zipkin_xray_conf_set_batch_size(zipkin_xray_conf_t conf, size_t batch_size);
void zipkin_xray_conf_set_backlog(zipkin_xray_conf_t conf, size_t backlog);
void zipkin_xray_conf_set_batch_interval(zipkin_xray_conf_t conf, size_t batch_interval_ms);

zipkin_collector_t zipkin_collector_new(const char *uri);
zipkin_collector_t zipkin_kafka_collector_new(zipkin_kafka_conf_t conf);
#ifdef WITH_CURL
zipkin_collector_t zipkin_http_collector_new(zipkin_http_conf_t conf);
#endif
zipkin_collector_t zipkin_scribe_collector_new(zipkin_scribe_conf_t conf);
zipkin_collector_t zipkin_xray_collector_new(zipkin_scribe_conf_t conf);
int zipkin_collector_flush(zipkin_collector_t collector, size_t timeout_ms);
void zipkin_collector_shutdown(zipkin_collector_t collector, size_t timeout_ms);
void zipkin_collector_free(zipkin_collector_t collector);

size_t zipkin_propagation_inject_headers(char *buf, size_t size, zipkin_span_t span);

#ifdef WITH_CURL
struct curl_slist *zipkin_propagation_inject_curl_headers(struct curl_slist *headers, zipkin_span_t span);
#endif

#ifdef __cplusplus
}
#endif