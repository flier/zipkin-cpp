#include <strings.h>

#include "zipkin.h"

#include "Span.h"
#include "Tracer.h"
#include "Collector.h"

#ifdef __cplusplus
extern "C" {
#endif

zipkin_endpoint_t zipkin_endpoint_new(struct sockaddr_in *addr, const char *service, int len)
{
    return new zipkin::Endpoint{*addr, std::string(service, len < 0 ? strlen(service) : len)};
}
void zipkin_endpoint_free(zipkin_endpoint_t endpoint)
{
    delete static_cast<zipkin::Endpoint *>(endpoint);
}

zipkin_span_t zipkin_span_new(zipkin_tracer_t tracer, const char *name, zipkin_userdata_t userdata)
{
    return static_cast<zipkin::Tracer *>(tracer)->span(name, 0, userdata);
}
zipkin_span_t zipkin_span_new_child(zipkin_span_t span, const char *name, zipkin_userdata_t userdata)
{
    return static_cast<zipkin::Span *>(span)->span(name, userdata);
}
void zipkin_span_free(zipkin_span_t span)
{
    delete static_cast<zipkin::Span *>(span);
}

zipkin_span_id_t zipkin_span_id(zipkin_span_t span)
{
    return static_cast<zipkin::Span *>(span)->id();
}
void zipkin_span_set_id(zipkin_span_t span, zipkin_span_id_t id)
{
    static_cast<zipkin::Span *>(span)->set_id(id);
}
const char *zipkin_span_name(zipkin_span_t span)
{
    return static_cast<zipkin::Span *>(span)->name().c_str();
}
zipkin_span_id_t zipkin_span_parent_id(zipkin_span_t span)
{
    return static_cast<zipkin::Span *>(span)->parent_id();
}
void zipkin_span_set_parent_id(zipkin_span_t span, zipkin_span_id_t id)
{
    static_cast<zipkin::Span *>(span)->set_parent_id(id);
}
zipkin_tracer_t zipkin_span_tracer(zipkin_span_t span)
{
    return static_cast<zipkin::Span *>(span)->tracer();
}
zipkin_userdata_t zipkin_span_userdata(zipkin_span_t span)
{
    return static_cast<zipkin::Span *>(span)->userdata();
}
void zipkin_span_set_userdata(zipkin_span_t span, zipkin_userdata_t userdata)
{
    static_cast<zipkin::Span *>(span)->set_userdata(userdata);
}

void zipkin_span_annotate(zipkin_span_t span, const char *value, int len, zipkin_endpoint_t endpoint)
{
    static_cast<zipkin::Span *>(span)->annotate(std::string(value, len < 0 ? strlen(value) : len),
                                                static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_bool(zipkin_span_t span, const char *key, int value, zipkin_endpoint_t endpoint)
{
    static_cast<zipkin::Span *>(span)->annotate(key, (bool)value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_bytes(zipkin_span_t span, const char *key, const char *value, size_t len, zipkin_endpoint_t endpoint)
{
    static_cast<zipkin::Span *>(span)->annotate(key, std::vector<uint8_t>(value, value + len), static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_int16(zipkin_span_t span, const char *key, int16_t value, zipkin_endpoint_t endpoint)
{
    static_cast<zipkin::Span *>(span)->annotate(key, value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_int32(zipkin_span_t span, const char *key, int32_t value, zipkin_endpoint_t endpoint)
{
    static_cast<zipkin::Span *>(span)->annotate(key, value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_int64(zipkin_span_t span, const char *key, int64_t value, zipkin_endpoint_t endpoint)
{
    static_cast<zipkin::Span *>(span)->annotate(key, value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_double(zipkin_span_t span, const char *key, double value, zipkin_endpoint_t endpoint)
{
    static_cast<zipkin::Span *>(span)->annotate(key, value, static_cast<zipkin::Endpoint *>(endpoint));
}
void zipkin_span_annotate_str(zipkin_span_t span, const char *key, const char *value, int len, zipkin_endpoint_t endpoint)
{
    static_cast<zipkin::Span *>(span)->annotate(key,
                                                std::string(value, len < 0 ? strlen(value) : len),
                                                static_cast<zipkin::Endpoint *>(endpoint));
}

void zipkin_span_submit(zipkin_span_t span)
{
    static_cast<zipkin::Span *>(span)->submit();
}

zipkin_tracer_t zipkin_tracer_new(zipkin_collector_t collector, const char *name)
{
    return new zipkin::CachedTracer(static_cast<zipkin::Collector *>(collector), name);
}
void zipkin_tracer_free(zipkin_tracer_t tracer)
{
    delete static_cast<zipkin::CachedTracer *>(tracer);
}

trace_id_t zipkin_tracer_id(zipkin_tracer_t tracer)
{
    return static_cast<zipkin::Tracer *>(tracer)->id();
}
const char *zipkin_tracer_name(zipkin_tracer_t tracer)
{
    return static_cast<zipkin::Tracer *>(tracer)->name().c_str();
}
zipkin_collector_t zipkin_tracer_collector(zipkin_tracer_t tracer)
{
    return static_cast<zipkin::Tracer *>(tracer)->collector();
}

zipkin_conf_t zipkin_conf_new(const char *brokers, const char *topic)
{
    return new zipkin::KafkaConf(brokers, topic);
}
void zipkin_conf_free(zipkin_conf_t conf)
{
    delete static_cast<zipkin::KafkaConf *>(conf);
}

void zipkin_conf_set_partition(zipkin_conf_t conf, int partition)
{
    static_cast<zipkin::KafkaConf *>(conf)->partition = partition;
}
int zipkin_conf_set_compression_codec(zipkin_conf_t conf, const char *codec)
{
    if (strcasecmp(codec, "gzip") == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::KafkaConf::CompressionCodec::gzip;
    }
    else if (strcasecmp(codec, "snappy") == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::KafkaConf::CompressionCodec::snappy;
    }
    else if (strcasecmp(codec, "lz4") == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::KafkaConf::CompressionCodec::lz4;
    }
    else if (strcasecmp(codec, "none") == 0)
    {
        static_cast<zipkin::KafkaConf *>(conf)->compression_codec = zipkin::KafkaConf::CompressionCodec::none;
    }
    else
    {
        return 0;
    }

    return -1;
}
void zipkin_conf_set_batch_num_messages(zipkin_conf_t conf, size_t batch_num_messages)
{
    static_cast<zipkin::KafkaConf *>(conf)->batch_num_messages = batch_num_messages;
}
void zipkin_conf_set_queue_buffering_max_messages(zipkin_conf_t conf, size_t queue_buffering_max_messages)
{
    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_messages = queue_buffering_max_messages;
}
void zipkin_conf_set_queue_buffering_max_kbytes(zipkin_conf_t conf, size_t queue_buffering_max_kbytes)
{
    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_kbytes = queue_buffering_max_kbytes;
}
void zipkin_conf_set_queue_buffering_max_ms(zipkin_conf_t conf, size_t queue_buffering_max_ms)
{
    static_cast<zipkin::KafkaConf *>(conf)->queue_buffering_max_ms = std::chrono::milliseconds(queue_buffering_max_ms);
}
void zipkin_conf_set_message_send_max_retries(zipkin_conf_t conf, size_t message_send_max_retries)
{
    static_cast<zipkin::KafkaConf *>(conf)->message_send_max_retries = message_send_max_retries;
}

zipkin_collector_t zipkin_collector_new(zipkin_conf_t conf)
{
    return static_cast<zipkin::KafkaConf *>(conf)->create();
}
void zipkin_collector_free(zipkin_collector_t collector)
{
    delete static_cast<zipkin::Collector *>(collector);
}

#ifdef __cplusplus
}
#endif