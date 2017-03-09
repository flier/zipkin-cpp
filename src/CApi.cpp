#include <strings.h>
#include <assert.h>

#include <glog/logging.h>

#include "zipkin.h"

#include "Span.h"
#include "Tracer.h"
#include "Propagation.h"
#include "Collector.h"
#include "KafkaCollector.h"
#ifdef WITH_CURL
#include "HttpCollector.h"
#endif
#include "ScribeCollector.h"
#include "XRayCollector.h"

#ifdef __cplusplus
extern "C" {
#endif

void zipkin_set_logging_level(enum zipkin_logger_level_t level)
{
    FLAGS_v = (int)level;
}

zipkin_endpoint_t zipkin_endpoint_new(const char *service, struct sockaddr *addr)
{
    return new zipkin::Endpoint(service ? std::string(service) : std::string(), addr);
}
void zipkin_endpoint_free(zipkin_endpoint_t endpoint)
{
    assert(endpoint);

    delete static_cast<zipkin::Endpoint *>(endpoint);
}
const char *zipkin_endpoint_service_name(zipkin_endpoint_t endpoint)
{
    assert(endpoint);

    return static_cast<zipkin::Endpoint *>(endpoint)->service_name().c_str();
}
void zipkin_endpoint_addr(zipkin_endpoint_t endpoint, struct sockaddr *addr, size_t len)
{
    assert(endpoint);

    if (addr)
        memcpy(addr, static_cast<zipkin::Endpoint *>(endpoint)->sockaddr().get(), len);
}

zipkin_span_t zipkin_span_new(zipkin_tracer_t tracer, const char *name, zipkin_userdata_t userdata)
{
    assert(tracer);
    assert(name);

    return static_cast<zipkin::Tracer *>(tracer)->span(name, 0, userdata);
}
zipkin_span_t zipkin_span_new_child(zipkin_span_t span, const char *name, zipkin_userdata_t userdata)
{
    assert(name);

    if (!span)
        return NULL;

    return static_cast<zipkin::Span *>(span)->span(name, userdata);
}
void zipkin_span_release(zipkin_span_t span)
{
    assert(span);

    static_cast<zipkin::CachedSpan *>(span)->release();
}

zipkin_span_id_t zipkin_span_id(zipkin_span_t span)
{
    assert(span);

    return static_cast<zipkin::Span *>(span)->id();
}
zipkin_span_t zipkin_span_set_id(zipkin_span_t span, zipkin_span_id_t id)
{
    if (span)
        static_cast<zipkin::Span *>(span)->with_id(id);

    return span;
}
const char *zipkin_span_name(zipkin_span_t span)
{
    assert(span);

    return static_cast<zipkin::Span *>(span)->name().c_str();
}
zipkin_span_id_t zipkin_span_parent_id(zipkin_span_t span)
{
    assert(span);

    return static_cast<zipkin::Span *>(span)->parent_id();
}
zipkin_span_t zipkin_span_set_parent_id(zipkin_span_t span, zipkin_span_id_t id)
{
    if (span)
        static_cast<zipkin::Span *>(span)->with_parent_id(id);

    return span;
}
zipkin_tracer_t zipkin_span_tracer(zipkin_span_t span)
{
    assert(span);

    return static_cast<zipkin::Span *>(span)->tracer();
}
zipkin_trace_id_t zipkin_span_trace_id(zipkin_span_t span)
{
    assert(span);

    return static_cast<zipkin::Span *>(span)->trace_id();
}
zipkin_span_t zipkin_span_set_trace_id(zipkin_span_t span, zipkin_trace_id_t id)
{
    if (span)
        static_cast<zipkin::Span *>(span)->with_trace_id(id);

    return span;
}
zipkin_trace_id_t zipkin_span_trace_id_high(zipkin_span_t span)
{
    assert(span);

    return static_cast<zipkin::Span *>(span)->trace_id_high();
}
zipkin_span_t zipkin_span_set_trace_id_high(zipkin_span_t span, zipkin_trace_id_t id)
{
    if (span)
        static_cast<zipkin::Span *>(span)->with_trace_id_high(id);

    return span;
}
zipkin_span_t zipkin_span_parse_trace_id(zipkin_span_t span, const char *str, size_t len)
{
    if (span)
        static_cast<zipkin::Span *>(span)->with_trace_id(std::string(str, len));

    return span;
}
int zipkin_span_debug(zipkin_span_t span)
{
    return span ? static_cast<zipkin::Span *>(span)->debug() : false;
}
zipkin_span_t zipkin_span_set_debug(zipkin_span_t span, int debug)
{
    if (span)
        static_cast<zipkin::Span *>(span)->with_debug(debug);

    return span;
}
int zipkin_span_sampled(zipkin_span_t span)
{
    return span ? static_cast<zipkin::Span *>(span)->sampled() : false;
}
zipkin_span_t zipkin_span_set_sampled(zipkin_span_t span, int sampled)
{
    if (span)
        static_cast<zipkin::Span *>(span)->with_sampled(sampled);

    return span;
}
zipkin_userdata_t zipkin_span_userdata(zipkin_span_t span)
{
    if (span)
        return static_cast<zipkin::Span *>(span)->userdata();

    return NULL;
}
zipkin_span_t zipkin_span_set_userdata(zipkin_span_t span, zipkin_userdata_t userdata)
{
    if (span)
        static_cast<zipkin::Span *>(span)->with_userdata(userdata);

    return span;
}

void zipkin_span_annotate(zipkin_span_t span, const char *value, int len, zipkin_endpoint_t endpoint)
{
    assert(span);
    assert(value);

    static_cast<zipkin::Span *>(span)->annotate(std::string(value, len < 0 ? strlen(value) : len),
                                                static_cast<const zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_bool(zipkin_span_t span, const char *key, int value, zipkin_endpoint_t endpoint)
{
    assert(span);
    assert(key);

    static_cast<zipkin::Span *>(span)->annotate(key, (bool)value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_bytes(zipkin_span_t span, const char *key, const char *value, size_t len, zipkin_endpoint_t endpoint)
{
    assert(span);
    assert(key);
    assert(value);
    assert(len);

    static_cast<zipkin::Span *>(span)->annotate(key, std::vector<uint8_t>(value, value + len), static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_int16(zipkin_span_t span, const char *key, int16_t value, zipkin_endpoint_t endpoint)
{
    assert(span);
    assert(key);

    static_cast<zipkin::Span *>(span)->annotate(key, value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_int32(zipkin_span_t span, const char *key, int32_t value, zipkin_endpoint_t endpoint)
{
    assert(span);
    assert(key);

    static_cast<zipkin::Span *>(span)->annotate(key, value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_int64(zipkin_span_t span, const char *key, int64_t value, zipkin_endpoint_t endpoint)
{
    assert(span);
    assert(key);

    static_cast<zipkin::Span *>(span)->annotate(key, value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_double(zipkin_span_t span, const char *key, double value, zipkin_endpoint_t endpoint)
{
    assert(span);
    assert(key);

    static_cast<zipkin::Span *>(span)->annotate(key, value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_str(zipkin_span_t span, const char *key, const char *value, int len, zipkin_endpoint_t endpoint)
{
    assert(span);
    assert(key);
    assert(value);
    assert(len);

    static_cast<zipkin::Span *>(span)->annotate(key,
                                                std::string(value, len < 0 ? strlen(value) : len),
                                                static_cast<zipkin::Endpoint *>(endpoint));
}

void zipkin_span_submit(zipkin_span_t span)
{
    if (span)
        static_cast<zipkin::Span *>(span)->submit();
}

zipkin_tracer_t zipkin_tracer_new(zipkin_collector_t collector)
{
    assert(collector);

    return new zipkin::CachedTracer(static_cast<zipkin::Collector *>(collector));
}
void zipkin_tracer_free(zipkin_tracer_t tracer)
{
    assert(tracer);

    delete static_cast<zipkin::CachedTracer *>(tracer);
}
zipkin_collector_t zipkin_tracer_collector(zipkin_tracer_t tracer)
{
    assert(tracer);

    return static_cast<zipkin::Tracer *>(tracer)->collector();
}
size_t zipkin_tracer_sample_rate(zipkin_tracer_t tracer)
{
    assert(tracer);

    return static_cast<zipkin::Tracer *>(tracer)->sample_rate();
}
void zipkin_tracer_set_sample_rate(zipkin_tracer_t tracer, size_t sample_rate)
{
    assert(tracer);

    static_cast<zipkin::Tracer *>(tracer)->set_sample_rate(sample_rate);
}
zipkin_userdata_t zipkin_tracer_userdata(zipkin_tracer_t tracer)
{
    return tracer ? static_cast<zipkin::Tracer *>(tracer)->userdata() : nullptr;
}
void zipkin_tracer_set_userdata(zipkin_tracer_t tracer, zipkin_userdata_t userdata)
{
    assert(tracer);

    static_cast<zipkin::Tracer *>(tracer)->set_userdata(userdata);
}

zipkin_kafka_conf_t zipkin_kafka_conf_new(const char *brokers, const char *topic)
{
    assert(brokers);
    assert(topic);

    return new zipkin::KafkaConf(brokers, topic);
}
void zipkin_kafka_conf_free(zipkin_kafka_conf_t conf)
{
    assert(conf);

    delete static_cast<zipkin::KafkaConf *>(conf);
}

void zipkin_kafka_conf_set_partition(zipkin_kafka_conf_t conf, int partition)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->topic_partition = partition;
}
void zipkin_kafka_conf_set_compression_codec(zipkin_kafka_conf_t conf, const char *codec)
{
    assert(conf);
    assert(codec);

    static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::parse_compression_codec(codec);
}
void zipkin_kafka_conf_set_message_codec(zipkin_kafka_conf_t conf, const char *codec)
{
    assert(conf);
    assert(codec);

    static_cast<zipkin::KafkaConf *>(conf)->message_codec = zipkin::MessageCodec::parse(codec);
}
void zipkin_kafka_conf_set_batch_num_messages(zipkin_kafka_conf_t conf, size_t batch_num_messages)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->batch_num_messages = batch_num_messages;
}
void zipkin_kafka_conf_set_queue_buffering_max_messages(zipkin_kafka_conf_t conf, size_t queue_buffering_max_messages)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_messages = queue_buffering_max_messages;
}
void zipkin_kafka_conf_set_queue_buffering_max_kbytes(zipkin_kafka_conf_t conf, size_t queue_buffering_max_kbytes)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_kbytes = queue_buffering_max_kbytes;
}
void zipkin_kafka_conf_set_queue_buffering_max_ms(zipkin_kafka_conf_t conf, size_t queue_buffering_max_ms)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_ms = std::chrono::milliseconds(queue_buffering_max_ms);
}
void zipkin_kafka_conf_set_message_send_max_retries(zipkin_kafka_conf_t conf, size_t message_send_max_retries)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->message_send_max_retries = message_send_max_retries;
}

#ifdef WITH_CURL

zipkin_http_conf_t zipkin_http_conf_new(const char *url)
{
    assert(url);

    return new zipkin::HttpConf(url);
}
void zipkin_http_conf_free(zipkin_http_conf_t conf)
{
    assert(conf);

    delete static_cast<zipkin::HttpConf *>(conf);
}
void zipkin_http_conf_set_proxy(zipkin_http_conf_t conf, const char *proxy, int tunnel)
{
    assert(conf);
    assert(proxy);

    static_cast<zipkin::HttpConf *>(conf)->proxy = proxy;
    static_cast<zipkin::HttpConf *>(conf)->http_proxy_tunnel = tunnel;
}
void zipkin_http_conf_set_message_codec(zipkin_http_conf_t conf, const char *codec)
{
    assert(conf);
    assert(codec);

    static_cast<zipkin::HttpConf *>(conf)->message_codec = zipkin::MessageCodec::parse(codec);
}
void zipkin_http_conf_set_batch_size(zipkin_http_conf_t conf, size_t batch_size)
{
    assert(conf);

    static_cast<zipkin::HttpConf *>(conf)->batch_size = batch_size;
}
void zipkin_http_conf_set_backlog(zipkin_http_conf_t conf, size_t backlog)
{
    assert(conf);

    static_cast<zipkin::HttpConf *>(conf)->backlog = backlog;
}
void zipkin_http_conf_set_max_redirect_times(zipkin_http_conf_t conf, size_t max_redirect_times)
{
    assert(conf);

    static_cast<zipkin::HttpConf *>(conf)->max_redirect_times = max_redirect_times;
}
void zipkin_http_conf_set_connect_timeout(zipkin_http_conf_t conf, size_t connect_timeout_ms)
{
    assert(conf);

    static_cast<zipkin::HttpConf *>(conf)->connect_timeout = std::chrono::milliseconds(connect_timeout_ms);
}
void zipkin_http_conf_set_request_timeout(zipkin_http_conf_t conf, size_t request_timeout_ms)
{
    assert(conf);

    static_cast<zipkin::HttpConf *>(conf)->request_timeout = std::chrono::milliseconds(request_timeout_ms);
}
void zipkin_http_conf_set_batch_interval(zipkin_http_conf_t conf, size_t batch_interval_ms)
{
    assert(conf);

    static_cast<zipkin::HttpConf *>(conf)->batch_interval = std::chrono::milliseconds(batch_interval_ms);
}

#endif // WITH_CURL

zipkin_scribe_conf_t zipkin_scribe_conf_new(const char *url)
{
    assert(url);

    return new zipkin::ScribeConf(url);
}
void zipkin_scribe_conf_free(zipkin_scribe_conf_t conf)
{
    assert(conf);

    delete static_cast<zipkin::ScribeConf *>(conf);
}
void zipkin_scribe_conf_set_message_codec(zipkin_scribe_conf_t conf, const char *codec)
{
    assert(conf);
    assert(codec);

    static_cast<zipkin::ScribeConf *>(conf)->message_codec = zipkin::MessageCodec::parse(codec);
}
void zipkin_scribe_conf_set_batch_size(zipkin_scribe_conf_t conf, size_t batch_size)
{
    assert(conf);

    static_cast<zipkin::ScribeConf *>(conf)->batch_size = batch_size;
}
void zipkin_scribe_conf_set_backlog(zipkin_scribe_conf_t conf, size_t backlog)
{
    assert(conf);

    static_cast<zipkin::ScribeConf *>(conf)->backlog = backlog;
}
void zipkin_scribe_conf_set_max_retry_times(zipkin_scribe_conf_t conf, size_t max_retry_times)
{
    assert(conf);

    static_cast<zipkin::ScribeConf *>(conf)->max_retry_times = max_retry_times;
}
void zipkin_scribe_conf_set_batch_interval(zipkin_scribe_conf_t conf, size_t batch_interval_ms)
{
    assert(conf);

    static_cast<zipkin::ScribeConf *>(conf)->batch_interval = std::chrono::milliseconds(batch_interval_ms);
}

zipkin_xray_conf_t zipkin_xray_conf_new(const char *host, port_t port)
{
    assert(host);
    assert(port);

    return new zipkin::XRayConf(host, port);
}
void zipkin_xray_conf_free(zipkin_xray_conf_t conf)
{
    delete static_cast<zipkin::XRayConf *>(conf);
}
void zipkin_xray_conf_set_batch_size(zipkin_xray_conf_t conf, size_t batch_size)
{
    assert(conf);

    static_cast<zipkin::XRayConf *>(conf)->batch_size = batch_size;
}
void zipkin_xray_conf_set_backlog(zipkin_xray_conf_t conf, size_t backlog)
{
    assert(conf);

    static_cast<zipkin::XRayConf *>(conf)->backlog = backlog;
}
void zipkin_xray_conf_set_batch_interval(zipkin_xray_conf_t conf, size_t batch_interval_ms)
{
    assert(conf);

    static_cast<zipkin::XRayConf *>(conf)->batch_interval = std::chrono::milliseconds(batch_interval_ms);
}

zipkin_collector_t zipkin_collector_new(const char *uri)
{
    assert(uri);

    return zipkin::Collector::create(uri);
}
zipkin_collector_t zipkin_kafka_collector_new(zipkin_kafka_conf_t conf)
{
    assert(conf);

    return static_cast<zipkin::KafkaConf *>(conf)->create();
}
#ifdef WITH_CURL
zipkin_collector_t zipkin_http_collector_new(zipkin_http_conf_t conf)
{
    assert(conf);

    return static_cast<zipkin::HttpConf *>(conf)->create();
}
#endif
zipkin_collector_t zipkin_scribe_collector_new(zipkin_scribe_conf_t conf)
{
    assert(conf);

    return static_cast<zipkin::ScribeConf *>(conf)->create();
}
zipkin_collector_t zipkin_xray_collector_new(zipkin_scribe_conf_t conf)
{
    assert(conf);
    return static_cast<zipkin::XRayConf *>(conf)->create();
}
int zipkin_collector_flush(zipkin_collector_t collector, size_t timeout_ms)
{
    assert(collector);

    return static_cast<zipkin::Collector *>(collector)->flush(std::chrono::milliseconds(timeout_ms));
}
void zipkin_collector_shutdown(zipkin_collector_t collector, size_t timeout_ms)
{
    assert(collector);

    static_cast<zipkin::Collector *>(collector)->shutdown(std::chrono::milliseconds(timeout_ms));
}
void zipkin_collector_free(zipkin_collector_t collector)
{
    assert(collector);

    delete static_cast<zipkin::Collector *>(collector);
}
size_t zipkin_propagation_inject_headers(char *buf, size_t size, zipkin_span_t span)
{
    assert(buf);
    assert(size);
    assert(span);

    return zipkin::Propagation::inject(buf, size, *static_cast<zipkin::Span *>(span));
}

#ifdef WITH_CURL
struct curl_slist *zipkin_propagation_inject_curl_headers(struct curl_slist *headers, zipkin_span_t span)
{
    assert(headers);
    assert(span);

    return zipkin::Propagation::inject(headers, *static_cast<zipkin::Span *>(span));
}
#endif

#ifdef __cplusplus
}
#endif