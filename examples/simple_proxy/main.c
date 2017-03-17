#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zf_log.h"
#include "mongoose.h"

#include "zipkin.h"

#define APP_NAME "simple_proxy"
#define APP_VERSION "1.0"

#define CRLF "\r\n"

#define HTTP_HOST "Host"
#define HTTP_VIA "Via"

#define HTTP_PROXY_CONNECTION "Proxy-Connection"
#define HTTP_PROXY_AUTHORIZATION "Proxy-Authorization"
#define HTTP_PROXY_AGENT "Proxy-Agent"

#define HTTP_FORWARDED "Forwarded"
#define HTTP_X_FORWARDED_FOR "X-Forwarded-For"
#define HTTP_X_FORWARDED_PORT "X-Forwarded-Port"
#define HTTP_X_FORWARDED_PROTO "X-Forwarded-Proto"

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

zipkin_tracer_t session_tracer(struct mg_connection *nc)
{
  return (zipkin_tracer_t)nc->mgr->user_data;
}

zipkin_span_t new_session_span(zipkin_tracer_t tracer, struct http_message *hm)
{
  char name[256] = {0};
  char *p = name, *end = p + sizeof(name);

  if (!tracer)
    return NULL;

  snprintf(p, end - p, "%.*s %.*s", (int)hm->method.len, hm->method.p, (int)hm->uri.len, hm->uri.p);

  return zipkin_span_new(tracer, name, NULL);
}

zipkin_span_t create_session_span(struct mg_connection *nc, struct http_message *hm, zipkin_endpoint_t endpoint)
{
  zipkin_span_t span = new_session_span(session_tracer(nc), hm);
  char client[64] = {0};
  char hostname[MAXHOSTNAMELEN] = {0};

  if (!span)
    return NULL;

  mg_conn_addr_to_str(nc, client, sizeof(client), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT | MG_SOCK_STRINGIFY_REMOTE);

  gethostname(hostname, sizeof(hostname));

  ANNOTATE(span, ZIPKIN_SERVER_RECV, endpoint);
  ANNOTATE_STR(span, ZIPKIN_CLIENT_ADDR, client, -1, endpoint);
  ANNOTATE_STR(span, ZIPKIN_SERVER_ADDR, hostname, -1, endpoint);
  ANNOTATE_STR(span, ZIPKIN_HTTP_METHOD, hm->method.p, hm->method.len, endpoint);

  return span;
}

zipkin_endpoint_t create_session_endpoint(struct mg_connection *nc, struct sockaddr *addr)
{
  union socket_address sock_addr = {0};
  socklen_t addr_len = sizeof(sock_addr);
  const char *service;

  if (!session_tracer(nc))
    return NULL;

  if (MG_F_UPSTREAM == (nc->flags & MG_F_UPSTREAM))
  {
    service = "upstream";

    if (!addr && 0 == getpeername(nc->sock, &sock_addr.sa, &addr_len) && sock_addr.sa.sa_family)
    {
      addr = (struct sockaddr *)&sock_addr.sin;
    }
  }
  else if (MG_F_TUNNEL == (nc->flags & MG_F_TUNNEL))
  {
    service = "tunnel";
  }
  else if (MG_F_PROXY == (nc->flags & MG_F_PROXY))
  {
    service = "proxy";
  }
  else
  {
    service = "http";
  }

  if (!addr && 0 == getsockname(nc->sock, &sock_addr.sa, &addr_len) && sock_addr.sa.sa_family)
  {
    addr = (struct sockaddr *)&sock_addr.sin;
  }

  return zipkin_endpoint_new(service, addr);
}

void forward_tcp_connection(struct mg_connection *nc, struct http_message *hm, zipkin_endpoint_t endpoint)
{
  zipkin_span_t span = create_session_span(nc, hm, endpoint);
  struct mg_connection *cc;
  const char *errmsg = NULL;
  struct mg_connect_opts opts = {nc, MG_F_UPSTREAM | MG_F_TUNNEL, &errmsg};
  char addr[1024] = {0};

  snprintf(addr, sizeof(addr), "tcp://%.*s", (int)hm->uri.len, hm->uri.p);

  cc = mg_connect_opt(nc->mgr, addr, ev_handler, opts);

  ZF_LOGI("TCP tunnel to `%.*s` created, conn=%p", (int)hm->uri.len, hm->uri.p, cc);

  ANNOTATE_STR(span, ZIPKIN_HTTP_URL, hm->uri.p, hm->uri.len, endpoint);

  nc->user_data = zipkin_span_set_userdata(span, cc);

  if (cc)
  {
    cc->user_data = span = zipkin_span_new_child(span, addr, nc);

    ANNOTATE(span, ZIPKIN_CLIENT_SEND, endpoint);

    mg_send_head(nc, 200, 0, HTTP_PROXY_AGENT ": " APP_NAME "/" APP_VERSION);
  }
  else
  {
    ANNOTATE_STR(span, ZIPKIN_ERROR, errmsg, -1, endpoint);

    mg_http_send_error(cc, 400, errmsg);
  }
}

void forward_http_request(struct mg_connection *nc, struct http_message *hm, zipkin_endpoint_t endpoint)
{
  zipkin_span_t span = create_session_span(nc, hm, endpoint);
  struct mg_connection *cc;
  int i;
  const char *errmsg = NULL;
  struct mg_connect_opts opts = {nc, MG_F_UPSTREAM | MG_F_PROXY, &errmsg};
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
    else if (0 == mg_vcmp(hn, HTTP_PROXY_AUTHORIZATION))
    {
      proxy_auth = hv;
    }
    else if (0 == mg_vcmp(hn, ZIPKIN_X_TRACE_ID))
    {
      zipkin_span_parse_trace_id(span, hv->p, hv->len);
    }
    else if (0 == mg_vcmp(hn, ZIPKIN_X_SPAN_ID))
    {
      zipkin_span_set_parent_id(span, strtoull(hv->p, NULL, 16));
    }
    else
    {
      ANNOTATE_STR_IF(0 == mg_vcasecmp(hn, HTTP_HOST), span, ZIPKIN_HTTP_HOST, hv->p, hv->len, endpoint);

      p += snprintf(p, end - p, "%.*s: %.*s" CRLF, (int)hn->len, hn->p, (int)hv->len, hv->p);
    }
  }

  gethostname(hostname, sizeof(hostname));
  mg_conn_addr_to_str(nc, local_addr, sizeof(local_addr), MG_SOCK_STRINGIFY_IP);
  mg_conn_addr_to_str(nc, peer_addr, sizeof(peer_addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_REMOTE);

  p += snprintf(p, end - p, HTTP_VIA ": 1.1 %s (%s/%s)" CRLF, hostname, APP_NAME, APP_VERSION);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_FOR ": %s" CRLF, peer_addr);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_PORT ": %d" CRLF, nc->sa.sin.sin_port);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_PROTO ": %s" CRLF, "http");
  p += snprintf(p, end - p, HTTP_FORWARDED ": for=%s;proto=http;by=%s" CRLF, peer_addr, local_addr);

  if (span)
  {
    p += zipkin_propagation_inject_headers(p, end - p, span);
  }

  cc = mg_connect_http_opt(nc->mgr, ev_handler, opts, uri, extra_headers, hm->body.len ? hm->body.p : NULL);

  ZF_LOGI("forward HTTP request to `%.*s`, conn=%p", (int)hm->uri.len, hm->uri.p, cc);
  ZF_LOGD("headers:\n%s", extra_headers);

  ANNOTATE_STR(span, ZIPKIN_HTTP_URL, hm->uri.p, hm->uri.len, endpoint);
  ANNOTATE(span, ZIPKIN_CLIENT_SEND, endpoint);
  ANNOTATE_INT32(span, ZIPKIN_HTTP_REQUEST_SIZE, cc->send_mbuf.size, endpoint);

  nc->user_data = zipkin_span_set_userdata(span, cc);

  if (cc)
  {
    if (span)
    {
      cc->user_data = zipkin_span_new_child(span, uri, nc);
    }
  }
  else
  {
    ANNOTATE_STR(span, ZIPKIN_ERROR, errmsg, -1, endpoint);

    mg_http_send_error(cc, 400, errmsg);
  }

  free(uri);
}

void reply_json_response(struct mg_connection *nc, struct http_message *hm, zipkin_endpoint_t endpoint)
{
  zipkin_span_t span = create_session_span(nc, hm, endpoint);
  char local_addr[64] = {0}, peer_addr[64] = {0};
  int i;

  mg_send_response_line(nc, 200,
                        "Content-Type: text/html" CRLF
                        "Connection: close" CRLF);
  mg_printf(nc,
            "{\"uri\": \"%.*s\", \"method\": \"%.*s\", \"body\": \"%.*s\", "
            "\"headers\": {",
            (int)hm->uri.len, hm->uri.p, (int)hm->method.len,
            hm->method.p, (int)hm->body.len, hm->body.p);

  for (i = 0; i < MG_MAX_HTTP_HEADERS && hm->header_names[i].len > 0; i++)
  {
    struct mg_str *hn = &hm->header_names[i];
    struct mg_str *hv = &hm->header_values[i];

    ANNOTATE_STR_IF(0 == mg_vcasecmp(hn, HTTP_HOST), span, ZIPKIN_HTTP_HOST, hv->p, hv->len, endpoint);

    if (0 == mg_vcmp(hn, ZIPKIN_X_TRACE_ID))
    {
      zipkin_span_parse_trace_id(span, hv->p, hv->len);
    }
    else if (0 == mg_vcmp(hn, ZIPKIN_X_SPAN_ID))
    {
      zipkin_span_set_parent_id(span, strtoull(hv->p, NULL, 16));
    }
    else if (0 == mg_vcmp(hn, ZIPKIN_X_SAMPLED))
    {
      zipkin_span_set_sampled(span, strtoul(hv->p, NULL, 10));
    }
    else if (0 == mg_vcmp(hn, ZIPKIN_X_FLAGS))
    {
      zipkin_span_set_debug(span, strtoul(hv->p, NULL, 10) & ZIPKIN_X_FLAG_DEBUG);
    }

    mg_printf(nc, "%s\"%.*s\": \"%.*s\"", (i != 0 ? "," : ""), (int)hn->len, hn->p, (int)hv->len, hv->p);
  }

  mg_printf(nc, "}}");

  nc->flags |= MG_F_SEND_AND_CLOSE;
  nc->user_data = span;

  ANNOTATE_STR(span, ZIPKIN_HTTP_PATH, hm->uri.p, hm->uri.len, endpoint);
  ANNOTATE_INT16(span, ZIPKIN_HTTP_STATUS_CODE, 200, endpoint);
  ANNOTATE_INT32(span, ZIPKIN_HTTP_REQUEST_SIZE, hm->message.len, endpoint);
  ANNOTATE_INT32(span, ZIPKIN_HTTP_RESPONSE_SIZE, nc->send_mbuf.size, endpoint);
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
  struct http_message *hm = (struct http_message *)ev_data;
  zipkin_tracer_t tracer = session_tracer(nc);
  zipkin_span_t span = tracer ? (zipkin_span_t)nc->user_data : NULL;
  struct mg_connection *cc = tracer ? (struct mg_connection *)zipkin_span_userdata(span) : (zipkin_span_t)nc->user_data;
  zipkin_endpoint_t endpoint = NULL;

  switch (ev)
  {
  case MG_EV_HTTP_REQUEST:
    ZF_LOGI("received HTTP request from %s:%d, method=%.*s, uri=%.*s, conn=%p", inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, (int)hm->method.len, hm->method.p, (int)hm->uri.len, hm->uri.p, nc);

    ZF_LOG_IF(hm->body.len, ZF_LOGD_MEM(hm->body.p, hm->body.len, "received %zu bytes body", hm->body.len));

    if (tracer && !zipkin_tracer_userdata(tracer))
    {
      socklen_t addr_len = sizeof(union socket_address);
      union socket_address *addr = malloc(addr_len);

      if (0 == getsockname(nc->sock, (struct sockaddr *)&addr->sa, &addr_len) && addr->sa.sa_family)
      {
        zipkin_tracer_set_userdata(tracer, addr);
      }
    }

    if (!mg_vcasecmp(&hm->method, "CONNECT"))
    {
      nc->flags |= MG_F_TUNNEL;

      if (tracer)
        endpoint = create_session_endpoint(nc, NULL);

      forward_tcp_connection(nc, hm, endpoint);
    }
    else if (mg_get_http_header(hm, HTTP_PROXY_CONNECTION))
    {
      nc->flags |= MG_F_PROXY;

      if (tracer)
        endpoint = create_session_endpoint(nc, NULL);

      forward_http_request(nc, hm, endpoint);
    }
    else
    {
      if (tracer)
        endpoint = create_session_endpoint(nc, NULL);

      reply_json_response(nc, hm, endpoint);
    }

    break;

  case MG_EV_HTTP_REPLY:
    ZF_LOGI("received HTTP response from %s:%d, conn=%p", inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, nc);

    if (tracer)
      endpoint = create_session_endpoint(nc, (struct sockaddr *)&nc->sa.sin);

    ANNOTATE(span, ZIPKIN_CLIENT_RECV, endpoint);
    ANNOTATE_INT16(span, ZIPKIN_HTTP_STATUS_CODE, hm->resp_code, endpoint);
    ANNOTATE_INT32(span, ZIPKIN_HTTP_RESPONSE_SIZE, hm->message.len, endpoint);

    mg_send(cc, hm->message.p, hm->message.len);

    cc->flags |= MG_F_SEND_AND_CLOSE;
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;

    break;

  case MG_EV_CONNECT:
    ZF_LOGI("connection to %s:%d was %s, conn=%p", inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, *(int *)ev_data ? "refused" : "accepted", nc);

    if (tracer)
      endpoint = create_session_endpoint(nc, (struct sockaddr *)&nc->sa.sin);

    if (*(int *)ev_data != 0)
    {
      ANNOTATE(span, "refused", endpoint);

      mg_http_send_error(cc, 502, NULL);
    }
    else
    {
      ANNOTATE(span, "connected", endpoint);
    }

    break;

  case MG_EV_RECV:
    ZF_LOGD("received TCP data %d bytes from %s:%d, conn=%p", *(int *)ev_data, inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, nc);

    if (tracer)
      endpoint = create_session_endpoint(nc, NULL);

    ANNOTATE_INT32(span, ZIPKIN_WIRE_RECV, *(int *)ev_data, endpoint);

    if (cc)
    {
      mg_send(cc, nc->recv_mbuf.buf, nc->recv_mbuf.len);
      mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
    }
    break;

  case MG_EV_SEND:
    ZF_LOGD("sent TCP data %d bytes to %s:%d, conn=%p", *(int *)ev_data, inet_ntoa(nc->sa.sin.sin_addr), nc->sa.sin.sin_port, nc);

    if (tracer)
      endpoint = create_session_endpoint(nc, NULL);

    ANNOTATE_INT32(span, ZIPKIN_WIRE_SEND, *(int *)ev_data, endpoint);

    break;

  case MG_EV_CLOSE:
    ZF_LOGI("connection closed, conn=%p", nc);

    if (cc)
    {
      cc->flags |= MG_F_SEND_AND_CLOSE;
    }

    if (tracer)
      endpoint = create_session_endpoint(nc, zipkin_tracer_userdata(tracer));

    ANNOTATE_IF(0 == (nc->flags & MG_F_UPSTREAM), span, ZIPKIN_SERVER_SEND, endpoint);

    zipkin_span_submit(span);

    break;
  }

  if (endpoint)
    zipkin_endpoint_free(endpoint);
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
  const char *collector_uri = NULL;
  int binary_encoding = 0;

  zipkin_collector_t collector = NULL;
  zipkin_tracer_t tracer = NULL;

  struct mg_mgr mgr;
  struct mg_connection *nc;

  zipkin_set_logging_level(LOG_WARN);
  cs_log_set_level(LL_WARN);
  zf_log_set_output_level(ZF_LOG_WARN);

  while ((c = getopt(argc, argv, "dvtp:bu:h")) != -1)
  {
    switch (c)
    {
    case 't':
      zipkin_set_logging_level(LOG_TRACE);
      cs_log_set_level(LL_VERBOSE_DEBUG);
      zf_log_set_output_level(ZF_LOG_VERBOSE);
      break;

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

    case 'u':
      collector_uri = optarg;
      break;

    case 'b':
      binary_encoding = 1;
      break;

    case 'h':
    case '?':
      printf("%s [options]\n\n", argv[0]);
      printf("-t\t\tShow trace messages\n");
      printf("-d\t\tShow debug messages\n");
      printf("-v\t\tShow verbose messages\n");
      printf("-p <port>\tListen on port (default: %s)\n", http_port);
      printf("-u <uri>\tCollector URI for tracing\n");

      return 1;

    default:
      printf("unknown argument: %c\n", c);

      return -1;
    }
  }

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  if (collector_uri)
  {
    collector = zipkin_collector_new(collector_uri);

    if (!collector)
    {
      ZF_LOGF("failed to create collector");
      return -2;
    }

    tracer = zipkin_tracer_new(collector);

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

  if (collector)
  {
    zipkin_collector_shutdown(collector, 2000);
    free(zipkin_tracer_userdata(tracer));
    zipkin_tracer_free(tracer);
    zipkin_collector_free(collector);
  }

  return 0;
}