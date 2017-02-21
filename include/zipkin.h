#pragma once

#include <stddef.h>
#include <stdint.h>

#include <sys/socket.h>

typedef uint64_t zipkin_span_id_t;
typedef uint64_t zipkin_trace_id_t;
typedef void *zipkin_userdata_t;

typedef void *zipkin_endpoint_t;
typedef void *zipkin_span_t;
typedef void *zipkin_tracer_t;
typedef void *zipkin_conf_t;
typedef void *zipkin_collector_t;

#ifdef __cplusplus
extern "C" {
#endif

zipkin_endpoint_t zipkin_endpoint_new(struct sockaddr_in *addr, const char *service, int len);
void zipkin_endpoint_free(zipkin_endpoint_t endpoint);

zipkin_span_t zipkin_span_new(zipkin_tracer_t tracer, const char *name, zipkin_userdata_t userdata);
zipkin_span_t zipkin_span_new_child(zipkin_span_t span, const char *name, zipkin_userdata_t userdata);
void zipkin_span_free(zipkin_span_t span);

zipkin_span_id_t zipkin_span_id(zipkin_span_t span);
void zipkin_span_set_id(zipkin_span_t span, zipkin_span_id_t id);

const char *zipkin_span_name(zipkin_span_t span);

zipkin_span_id_t zipkin_span_parent_id(zipkin_span_t span);
void zipkin_span_set_parent_id(zipkin_span_t span, zipkin_span_id_t id);

zipkin_tracer_t zipkin_span_tracer(zipkin_span_t span);

zipkin_userdata_t zipkin_span_userdata(zipkin_span_t span);
void zipkin_span_set_userdata(zipkin_span_t span, zipkin_userdata_t userdata);

void zipkin_span_annotate(zipkin_span_t span, const char *value, int len, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_bool(zipkin_span_t span, const char *key, int value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_bytes(zipkin_span_t span, const char *key, const char *value, size_t len, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_int16(zipkin_span_t span, const char *key, int16_t value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_int32(zipkin_span_t span, const char *key, int32_t value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_int64(zipkin_span_t span, const char *key, int64_t value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_double(zipkin_span_t span, const char *key, double value, zipkin_endpoint_t endpoint);
void zipkin_span_annotate_str(zipkin_span_t span, const char *key, const char *value, int len, zipkin_endpoint_t endpoint);

void zipkin_span_submit(zipkin_span_t span);

zipkin_tracer_t zipkin_tracer_new(zipkin_collector_t collector, const char *name);
void zipkin_tracer_free(zipkin_tracer_t tracer);

zipkin_trace_id_t zipkin_tracer_id(zipkin_tracer_t tracer);
const char *zipkin_tracer_name(zipkin_tracer_t tracer);
zipkin_collector_t zipkin_tracer_collector(zipkin_tracer_t tracer);

zipkin_conf_t zipkin_conf_new(const char *brokers, const char *topic);
void zipkin_conf_free(zipkin_conf_t conf);

void zipkin_conf_set_partition(zipkin_conf_t conf, int partition);
int zipkin_conf_set_compression_codec(zipkin_conf_t conf, const char *codec);
void zipkin_conf_set_batch_num_messages(zipkin_conf_t conf, size_t batch_num_messages);
void zipkin_conf_set_queue_buffering_max_messages(zipkin_conf_t conf, size_t queue_buffering_max_messages);
void zipkin_conf_set_queue_buffering_max_kbytes(zipkin_conf_t conf, size_t queue_buffering_max_kbytes);
void zipkin_conf_set_queue_buffering_max_ms(zipkin_conf_t conf, size_t queue_buffering_max_ms);
void zipkin_conf_set_message_send_max_retries(zipkin_conf_t conf, size_t message_send_max_retries);

zipkin_collector_t zipkin_collector_new(zipkin_conf_t conf);
void zipkin_collector_free(zipkin_collector_t collector);

#ifdef __cplusplus
}
#endif