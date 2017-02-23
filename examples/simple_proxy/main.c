#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "zf_log.h"
#include "mongoose.h"

#include "zipkin.h"

#define APP_NAME "simple_proxy"
#define APP_VERSION "1.0"

#define HTTP_HOST "Host"
#define HTTP_VIA "Via"

#define HTTP_PROXY_CONNECTION "Proxy-Connection"
#define HTTP_PROXY_AUTHORIZATION "Proxy-Authorization"
#define HTTP_PROXY_AGENT "Proxy-Agent"

#define HTTP_FORWARDED "Forwarded"
#define HTTP_X_FORWARDED_FOR "X-Forwarded-For"
#define HTTP_X_FORWARDED_PORT "X-Forwarded-Port"
#define HTTP_X_FORWARDED_PROTO "X-Forwarded-Proto"
#define HTTP_X_SPAN_ID "X-Span-Id"

#define MG_F_PROXY MG_F_USER_1
#define MG_F_UPSTREAM MG_F_USER_2
#define MG_F_TUNNEL MG_F_USER_3

static sig_atomic_t s_signal_received = 0;
static struct mg_serve_http_opts s_http_server_opts;

void signal_handler(int sig_num)
{
  ZF_LOGI("received `%s` signal", strsignal(sig_num));

  signal(sig_num, signal_handler); // Reinstantiate signal handler

  s_signal_received = sig_num;
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data);

zipkin_span_t new_session_span(struct mg_connection *nc, struct http_message *hm)
{
  char name[256] = {0};
  char *p = name, *end = p + sizeof(name);

  snprintf(p, end - p, "%.*s %.*s", (int)hm->method.len, hm->method.p, (int)hm->uri.len, hm->uri.p);

  return zipkin_span_new((zipkin_tracer_t)nc->mgr->user_data, name, NULL);
}

zipkin_span_t create_session_span(struct mg_connection *nc, struct http_message *hm, zipkin_endpoint_t endpoint)
{
  zipkin_span_t span = new_session_span(nc, hm);
  char client[64] = {0};
  char hostname[MAXHOSTNAMELEN] = {0};

  mg_conn_addr_to_str(nc, client, sizeof(client), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT | MG_SOCK_STRINGIFY_REMOTE);

  gethostname(hostname, sizeof(hostname));

  zipkin_span_annotate(span, ZIPKIN_SERVER_RECV, -1, endpoint);
  zipkin_span_annotate_str(span, ZIPKIN_CLIENT_ADDR, client, -1, endpoint);
  zipkin_span_annotate_str(span, ZIPKIN_SERVER_ADDR, hostname, -1, endpoint);
  zipkin_span_annotate_str(span, ZIPKIN_HTTP_METHOD, hm->method.p, hm->method.len, endpoint);

  return span;
}

zipkin_endpoint_t create_session_endpoint(struct mg_connection *nc, const char *service)
{
  union socket_address addr;
  socklen_t addr_len = sizeof(addr);

  if (MG_F_UPSTREAM == (nc->flags & MG_F_UPSTREAM))
  {
    return zipkin_endpoint_new(&nc->sa.sin, service, -1);
  }

  getsockname(nc->sock, &addr.sa, &addr_len);

  return zipkin_endpoint_new(&addr.sin, service, -1);
}

void forward_tcp_connection(struct mg_connection *nc, struct http_message *hm)
{
  zipkin_endpoint_t endpoint = create_session_endpoint(nc, "tunnel");
  zipkin_span_t span = create_session_span(nc, hm, endpoint);
  struct mg_connection *cc;
  const char *errmsg = NULL;
  struct mg_connect_opts opts = {nc, MG_F_UPSTREAM | MG_F_TUNNEL, &errmsg};
  char addr[1024] = {0};
  char *p = addr, *end = addr + sizeof(addr) + 1;

  p += snprintf(p, end - p, "tcp://%.*s", (int)hm->uri.len, hm->uri.p);

  cc = mg_connect_opt(nc->mgr, addr, ev_handler, opts);

  ZF_LOGI("TCP tunnel to `%.*s` created, conn=%p", (int)hm->uri.len, hm->uri.p, cc);

  zipkin_span_annotate_str(span, ZIPKIN_HTTP_URL, hm->uri.p, hm->uri.len, endpoint);
  zipkin_span_set_userdata(span, cc);

  nc->user_data = span;

  if (cc)
  {
    span = zipkin_span_new_child(span, "upstream", nc);

    zipkin_span_annotate(span, ZIPKIN_CLIENT_SEND, -1, NULL);

    cc->user_data = span;

    mg_send_head(nc, 200, 0, HTTP_PROXY_AGENT ": " APP_NAME "/" APP_VERSION);
  }
  else
  {
    zipkin_span_annotate_str(span, ZIPKIN_ERROR, errmsg, -1, endpoint);

    mg_http_send_error(cc, 400, errmsg);
  }

  zipkin_endpoint_free(endpoint);
}

void forward_http_request(struct mg_connection *nc, struct http_message *hm)
{
  zipkin_endpoint_t endpoint = create_session_endpoint(nc, "proxy"), peer_endpoint;
  zipkin_span_t span = create_session_span(nc, hm, endpoint);
  struct mg_connection *cc;
  int i;
  const char *errmsg = NULL;
  struct mg_connect_opts opts = {nc, MG_F_UPSTREAM, &errmsg};
  char *uri = strndup(hm->uri.p, hm->uri.len);
  char hostname[MAXHOSTNAMELEN] = {0};
  char local_addr[64] = {0}, peer_addr[64] = {0};
  char extra_headers[4096] = {0};
  char *p = extra_headers, *end = extra_headers + sizeof(extra_headers) - 1;
  struct mg_str *proxy_conn = NULL, *proxy_auth = NULL;

  for (i = 0; i < MG_MAX_HTTP_HEADERS && hm->header_names[i].len > 0; i++)
  {
    struct mg_str *hn = &hm->header_names[i];
    struct mg_str *hv = &hm->header_values[i];

    if (0 == mg_vcmp(hn, HTTP_PROXY_CONNECTION))
    {
      proxy_conn = hv;
    }
    else if (0 != mg_vcmp(hn, HTTP_PROXY_AUTHORIZATION))
    {
      proxy_auth = hv;
    }
    else if (0 != mg_vcmp(hn, HTTP_X_SPAN_ID))
    {
      zipkin_span_set_parent_id(span, strtoll(hv->p, NULL, 16));
    }
    else
    {
      if (0 == mg_vcasecmp(hn, HTTP_HOST))
      {
        zipkin_span_annotate_str(span, ZIPKIN_HTTP_HOST, hv->p, hv->len, endpoint);
      }

      p += snprintf(p, end - p, "%.*s: %.*s\n", (int)hn->len, hn->p, (int)hv->len, hv->p);
    }
  }

  gethostname(hostname, sizeof(hostname));
  mg_conn_addr_to_str(nc, local_addr, sizeof(local_addr), MG_SOCK_STRINGIFY_IP);
  mg_conn_addr_to_str(nc, peer_addr, sizeof(peer_addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_REMOTE);

  p += snprintf(p, end - p, HTTP_VIA ": 1.1 %s (%s/%s)\r\n", hostname, APP_NAME, APP_VERSION);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_FOR ": %s\r\n", peer_addr);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_PORT ": %d\r\n", nc->sa.sin.sin_port);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_PROTO ": %s\r\n", "http");
  p += snprintf(p, end - p, HTTP_FORWARDED ": for=%s;proto=http;by=%s\r\n", peer_addr, local_addr);
  p += snprintf(p, end - p, HTTP_X_SPAN_ID ": %llx\r\n", zipkin_span_id(span));

  cc = mg_connect_http_opt(nc->mgr, ev_handler, opts, uri, extra_headers, hm->body.len ? hm->body.p : NULL);

  ZF_LOGI("forward HTTP request to `%.*s`, conn=%p", (int)hm->uri.len, hm->uri.p, cc);
  ZF_LOGD("headers:\n%s", extra_headers);

  zipkin_span_annotate_str(span, ZIPKIN_HTTP_URL, hm->uri.p, hm->uri.len, endpoint);
  zipkin_span_set_userdata(span, cc);

  nc->user_data = span;

  if (cc)
  {
    peer_endpoint = create_session_endpoint(cc, "upstream");
    span = zipkin_span_new_child(span, uri, nc);

    zipkin_span_annotate(span, ZIPKIN_CLIENT_SEND, -1, peer_endpoint);
    zipkin_span_annotate_int32(span, ZIPKIN_HTTP_REQUEST_SIZE, cc->send_mbuf.size, peer_endpoint);

    zipkin_endpoint_free(peer_endpoint);

    cc->user_data = span;
  }
  else
  {
    zipkin_span_annotate_str(span, ZIPKIN_ERROR, errmsg, -1, endpoint);

    mg_http_send_error(cc, 400, errmsg);
  }

  free(uri);

  zipkin_endpoint_free(endpoint);
}

void reply_json_response(struct mg_connection *nc, struct http_message *hm)
{
  zipkin_endpoint_t endpoint = create_session_endpoint(nc, "http");
  zipkin_span_t span = create_session_span(nc, hm, endpoint);
  char local_addr[64] = {0}, peer_addr[64] = {0};
  int i;

  mg_send_response_line(nc, 200,
                        "Content-Type: text/html\r\n"
                        "Connection: close\r\n");
  mg_printf(nc,
            "{\"uri\": \"%.*s\", \"method\": \"%.*s\", \"body\": \"%.*s\", "
            "\"headers\": {",
            (int)hm->uri.len, hm->uri.p, (int)hm->method.len,
            hm->method.p, (int)hm->body.len, hm->body.p);

  for (i = 0; i < MG_MAX_HTTP_HEADERS && hm->header_names[i].len > 0; i++)
  {
    struct mg_str *hn = &hm->header_names[i];
    struct mg_str *hv = &hm->header_values[i];

    if (0 == mg_vcasecmp(hn, HTTP_HOST))
    {
      zipkin_span_annotate_str(span, ZIPKIN_HTTP_HOST, hv->p, hv->len, endpoint);
    }

    mg_printf(nc, "%s\"%.*s\": \"%.*s\"", (i != 0 ? "," : ""), (int)hn->len, hn->p, (int)hv->len, hv->p);
  }

  mg_printf(nc, "}}");

  nc->flags |= MG_F_SEND_AND_CLOSE;
  nc->user_data = span;

  zipkin_span_annotate_str(span, ZIPKIN_HTTP_PATH, hm->uri.p, hm->uri.len, endpoint);
  zipkin_span_annotate_int16(span, ZIPKIN_HTTP_STATUS_CODE, 200, endpoint);
  zipkin_span_annotate_int32(span, ZIPKIN_HTTP_REQUEST_SIZE, hm->message.len, endpoint);
  zipkin_span_annotate_int32(span, ZIPKIN_HTTP_RESPONSE_SIZE, nc->send_mbuf.size, endpoint);

  zipkin_endpoint_free(endpoint);
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
  struct http_message *hm = (struct http_message *)ev_data;
  zipkin_span_t span = (zipkin_span_t)nc->user_data;
  struct mg_connection *cc = span ? (struct mg_connection *)zipkin_span_userdata(span) : NULL;

  switch (ev)
  {
  case MG_EV_HTTP_REQUEST:
    ZF_LOGI("received HTTP request from %s:%d, method=%.*s, uri=%.*s, conn=%p", inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, (int)hm->method.len, hm->method.p, (int)hm->uri.len, hm->uri.p, nc);

    ZF_LOG_IF(hm->body.len, ZF_LOGD_MEM(hm->body.p, hm->body.len, "received %zu bytes body", hm->body.len));

    if (!mg_vcasecmp(&hm->method, "CONNECT"))
    {
      forward_tcp_connection(nc, hm);

      nc->flags |= MG_F_PROXY;
    }
    else if (mg_get_http_header(hm, HTTP_PROXY_CONNECTION))
    {
      forward_http_request(nc, hm);

      nc->flags |= MG_F_PROXY;
    }
    else
    {
      reply_json_response(nc, hm);
    }

    break;

  case MG_EV_HTTP_REPLY:
    ZF_LOGI("received HTTP response from %s:%d, conn=%p", inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, nc);

    zipkin_span_annotate(span, ZIPKIN_CLIENT_RECV, -1, NULL);
    zipkin_span_annotate_int16(span, ZIPKIN_HTTP_STATUS_CODE, hm->resp_code, NULL);
    zipkin_span_annotate_int32(span, ZIPKIN_HTTP_RESPONSE_SIZE, hm->message.len, NULL);

    mg_send(cc, hm->message.p, hm->message.len);

    cc->flags |= MG_F_SEND_AND_CLOSE;
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;

    break;

  case MG_EV_CONNECT:
    ZF_LOGI("connection to %s:%d was %s, conn=%p", inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, *(int *)ev_data ? "refused" : "accepted", nc);

    if (*(int *)ev_data != 0)
    {
      zipkin_span_annotate(span, "refused", -1, NULL);

      mg_http_send_error(cc, 502, NULL);
    }
    else
    {
      zipkin_span_annotate(span, "connected", -1, NULL);
    }
    break;

  case MG_EV_RECV:
    ZF_LOGD("received TCP data %d bytes from %s:%d, conn=%p", *(int *)ev_data, inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, nc);

    if (span)
    {
      zipkin_span_annotate_int32(span, ZIPKIN_WIRE_RECV, *(int *)ev_data, NULL);
    }

    if (cc)
    {
      mg_send(cc, nc->recv_mbuf.buf, nc->recv_mbuf.len);
      mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
    }
    break;

  case MG_EV_SEND:
    ZF_LOGD("sent TCP data %d bytes to %s:%d, conn=%p", *(int *)ev_data, inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, nc);

    if (span)
    {
      zipkin_span_annotate_int32(span, ZIPKIN_WIRE_SEND, *(int *)ev_data, NULL);
    }

    break;

  case MG_EV_CLOSE:
    ZF_LOGI("connection closed, conn=%p", nc);

    if (cc)
    {
      cc->flags |= MG_F_SEND_AND_CLOSE;
    }

    if (span)
    {
      if (0 == (nc->flags & MG_F_UPSTREAM))
      {
        zipkin_span_annotate(span, ZIPKIN_SERVER_SEND, -1, NULL);
      }

      zipkin_span_submit(span);
    }
    break;
  }
}

zipkin_tracer_t create_collector(const char *kafka_uri, int binary_encoding)
{
  struct mg_str host, path;
  unsigned int port = 0;
  char broker[1024] = {0};
  char *topic;
  const char *message_codec = binary_encoding ? ZIPKIN_ENCODING_BINARY : ZIPKIN_ENCODING_PRETTY_JSON;
  zipkin_conf_t conf = NULL;
  zipkin_collector_t collector = NULL;

  if (mg_parse_uri(mg_mk_str(kafka_uri), NULL, NULL, &host, &port, &path, NULL, NULL))
    return NULL;

  if (port)
  {
    snprintf(broker, sizeof(broker), "%.*s:%d", (int)host.len, host.p, port);
  }
  else
  {
    snprintf(broker, sizeof(broker), "%.*s", (int)host.len, host.p);
  }

  topic = strtok((char *)path.p, "/");

  conf = zipkin_conf_new(broker, topic);

  if (conf)
  {
    zipkin_conf_set_message_codec(conf, message_codec);

    collector = zipkin_collector_new(conf);

    if (collector)
    {
      ZF_LOGI("collector created, broker=%s, topic=%s, message_codec=%s", broker, topic, message_codec);
    }

    zipkin_conf_free(conf);
  }

  return collector;
}

enum cs_log_level
{
  LL_NONE = -1,
  LL_ERROR = 0,
  LL_WARN = 1,
  LL_INFO = 2,
  LL_DEBUG = 3,
  LL_VERBOSE_DEBUG = 4,

  _LL_MIN = -2,
  _LL_MAX = 5,
};

extern void cs_log_set_level(enum cs_log_level level);

int main(int argc, char **argv)
{
  int c;

  const char *http_port = "8000";
  const char *kafka_uri = NULL;
  int binary_encoding = 0;

  zipkin_collector_t collector = NULL;
  zipkin_tracer_t tracer = NULL;

  struct mg_mgr mgr;
  struct mg_connection *nc;

  zipkin_set_logging_level(LOG_WARN);
  cs_log_set_level(LL_WARN);
  zf_log_set_output_level(ZF_LOG_WARN);

  while ((c = getopt(argc, argv, "dvp:bt:h")) != -1)
  {
    switch (c)
    {
    case 'd':
      zipkin_set_logging_level(LOG_DEBUG);
      cs_log_set_level(LL_DEBUG);
      zf_log_set_output_level(ZF_LOG_DEBUG);
      break;

    case 'v':
      zipkin_set_logging_level(LOG_INFO);
      cs_log_set_level(LL_INFO);
      zf_log_set_output_level(ZF_LOG_INFO);
      break;

    case 'p':
      http_port = optarg;
      break;

    case 't':
      kafka_uri = optarg;
      break;

    case 'b':
      binary_encoding = 1;
      break;

    case 'h':
    case '?':
      printf("%s [options]\n\n", argv[0]);
      printf("-d\t\tShow debug messages\n");
      printf("-v\t\tShow verbose messages\n");
      printf("-p <port>\tListen on port (default: %s)\n", http_port);
      printf("-t <uri>\tKafka URI for tracing\n");
      printf("-b\t\tEncode message in binary\n");

      return 1;

    default:
      printf("unknown argument: %c\n", c);

      return -1;
    }
  }

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  if (kafka_uri)
  {
    collector = create_collector(kafka_uri, binary_encoding);

    if (!collector)
    {
      ZF_LOGF("failed to create collector");
      return -2;
    }

    tracer = zipkin_tracer_new(collector, APP_NAME);

    if (!tracer)
    {
      ZF_LOGF("failed to create tracer");
      return -3;
    }
  }

  mg_mgr_init(&mgr, tracer);

  ZF_LOGI("starting on port %s, pid=%d", http_port, getpid());

  nc = mg_bind(&mgr, http_port, ev_handler);

  if (!nc)
  {
    ZF_LOGF("failed to create listener");
    return -4;
  }

  mg_set_protocol_http_websocket(nc);

  while (!s_signal_received)
  {
    mg_mgr_poll(&mgr, 500);
  }

  mg_mgr_free(&mgr);

  ZF_LOGI("proxy stopped");

  if (tracer)
  {
    zipkin_tracer_free(tracer);
  }

  if (collector)
  {
    zipkin_collector_free(collector);
  }

  return 0;
}