#include <strings.h>
#include <assert.h>

#include <glog/logging.h>

#include "zipkin.h"

#include "Span.h"
#include "Tracer.h"
#include "Collector.h"

#ifdef __cplusplus
extern "C" {
#endif

void zipkin_set_logging_level(enum zipkin_logger_level_t level)
{
    FLAGS_v = (int)level;
}

zipkin_endpoint_t zipkin_endpoint_new(const char *service, struct sockaddr *addr)
{
    assert(addr);

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

zipkin_tracer_t zipkin_tracer_new(zipkin_collector_t collector, const char *name)
{
    assert(collector);
    assert(name);

    return new zipkin::CachedTracer(static_cast<zipkin::Collector *>(collector), name);
}
void zipkin_tracer_free(zipkin_tracer_t tracer)
{
    assert(tracer);

    delete static_cast<zipkin::CachedTracer *>(tracer);
}

trace_id_t zipkin_tracer_id(zipkin_tracer_t tracer)
{
    assert(tracer);

    return static_cast<zipkin::Tracer *>(tracer)->id();
}
const char *zipkin_tracer_name(zipkin_tracer_t tracer)
{
    assert(tracer);

    return static_cast<zipkin::Tracer *>(tracer)->name().c_str();
}
zipkin_collector_t zipkin_tracer_collector(zipkin_tracer_t tracer)
{
    assert(tracer);

    return static_cast<zipkin::Tracer *>(tracer)->collector();
}

zipkin_conf_t zipkin_conf_new(const char *brokers, const char *topic)
{
    assert(brokers);
    assert(topic);

    return new zipkin::KafkaConf(brokers, topic);
}
void zipkin_conf_free(zipkin_conf_t conf)
{
    assert(conf);

    delete static_cast<zipkin::KafkaConf *>(conf);
}

void zipkin_conf_set_partition(zipkin_conf_t conf, int partition)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->topic_partition = partition;
}
int zipkin_conf_set_compression_codec(zipkin_conf_t conf, const char *codec)
{
    assert(conf);
    assert(codec);

    if (strcasecmp(codec, ZIPKIN_COMPRESSION_GZIP) == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::CompressionCodec::gzip;
    }
    else if (strcasecmp(codec, ZIPKIN_COMPRESSION_SNAPPY) == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::CompressionCodec::snappy;
    }
    else if (strcasecmp(codec, ZIPKIN_COMPRESSION_LZ4) == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::CompressionCodec::lz4;
    }
    else if (strcasecmp(codec, ZIPKIN_COMPRESSION_NONE) == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::CompressionCodec::none;
    }
    else
    {
        return 0;
    }

    return -1;
}
int zipkin_conf_set_message_codec(zipkin_conf_t conf, const char *codec)
{
    assert(conf);
    assert(codec);

    if (strcasecmp(codec, ZIPKIN_ENCODING_BINARY) == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->message_codec = zipkin::MessageCodec::binary;
    }
    else if (strcasecmp(codec, ZIPKIN_ENCODING_JSON) == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->message_codec = zipkin::MessageCodec::json;
    }
    else if (strcasecmp(codec, ZIPKIN_ENCODING_PRETTY_JSON) == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->message_codec = zipkin::MessageCodec::pretty_json;
    }
    else
    {
        return 0;
    }

    return -1;
}
void zipkin_conf_set_batch_num_messages(zipkin_conf_t conf, size_t batch_num_messages)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->batch_num_messages = batch_num_messages;
}
void zipkin_conf_set_queue_buffering_max_messages(zipkin_conf_t conf, size_t queue_buffering_max_messages)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_messages = queue_buffering_max_messages;
}
void zipkin_conf_set_queue_buffering_max_kbytes(zipkin_conf_t conf, size_t queue_buffering_max_kbytes)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_kbytes = queue_buffering_max_kbytes;
}
void zipkin_conf_set_queue_buffering_max_ms(zipkin_conf_t conf, size_t queue_buffering_max_ms)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_ms = std::chrono::milliseconds(queue_buffering_max_ms);
}
void zipkin_conf_set_message_send_max_retries(zipkin_conf_t conf, size_t message_send_max_retries)
{
    assert(conf);

    static_cast<zipkin::KafkaConf *>(conf)->message_send_max_retries = message_send_max_retries;
}

zipkin_collector_t zipkin_collector_new(zipkin_conf_t conf)
{
    assert(conf);

    return static_cast<zipkin::KafkaConf *>(conf)->create();
}
void zipkin_collector_free(zipkin_collector_t collector)
{
    assert(collector);

    delete static_cast<zipkin::Collector *>(collector);
}
int zipkin_collector_flush(zipkin_collector_t collector, size_t timeout_ms)
{
    assert(collector);

    return static_cast<zipkin::Collector *>(collector)->flush(std::chrono::milliseconds(timeout_ms));
}

#ifdef __cplusplus
}
#endif