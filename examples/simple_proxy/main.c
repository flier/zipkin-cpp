#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

#include "mongoose.h"

#include "zipkin.h"

#define APP_NAME "simple_proxy"
#define APP_VERSION "1.0"

#define HTTP_PROXY_CONNECTION "Proxy-Connection"
#define HTTP_PROXY_AUTHORIZATION "Proxy-Authorization"
#define HTTP_PROXY_AGENT "Proxy-Agent"
#define HTTP_VIA "Via"
#define HTTP_FORWARDED "Forwarded"
#define HTTP_X_FORWARDED_FOR "X-Forwarded-For"
#define HTTP_X_FORWARDED_PORT "X-Forwarded-Port"
#define HTTP_X_FORWARDED_PROTO "X-Forwarded-Proto"
#define HTTP_X_SPAN_ID "X-Span-Id"

static sig_atomic_t s_signal_received = 0;
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

void signal_handler(int sig_num)
{
  signal(sig_num, signal_handler); // Reinstantiate signal handler
  s_signal_received = sig_num;
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data);

zipkin_span_t create_session_span(struct mg_connection *nc, struct http_message *hm, const char *service, const char *type)
{
  static char hostname[MAXHOSTNAMELEN] = {0};
  union socket_address addr;
  socklen_t addr_len = sizeof(addr);
  zipkin_span_t span = zipkin_span_new((zipkin_tracer_t)nc->mgr->user_data, "session", NULL);

  if (!hostname[0])
    gethostname(hostname, sizeof(hostname));

  getsockname(nc->sock, &addr.sa, &addr_len);

  zipkin_span_annotate_str(span, "hostname", hostname, -1, NULL);
  zipkin_span_annotate_str(span, "type", type, -1, zipkin_endpoint_new(&addr.sin, service, -1));
  zipkin_span_annotate_str(span, "request", hm->uri.p, hm->uri.len, zipkin_endpoint_new(&nc->sa.sin, hm->method.p, hm->method.len));

  return span;
}

void forward_tcp_connection(struct mg_connection *nc, struct http_message *hm)
{
  zipkin_span_t span = create_session_span(nc, hm, "proxy", "stream");
  struct mg_connection *cc;
  const char *errmsg = NULL;
  struct mg_connect_opts opts = {nc, 0, &errmsg};
  char addr[1024] = {0};
  char *p = addr, *end = addr + sizeof(addr) + 1;

  p += snprintf(p, end - p, "tcp://%.*s", (int)hm->uri.len, hm->uri.p);

  cc = mg_connect_opt(nc->mgr, addr, ev_handler, opts);

  zipkin_span_annotate_str(span, "upstream", addr, -1, NULL);
  zipkin_span_set_userdata(span, cc);

  nc->user_data = span;

  if (cc)
  {
    cc->user_data = zipkin_span_new_child(span, "upstream", nc);

    mg_send_head(nc, 200, 0, HTTP_PROXY_AGENT ": " APP_NAME "/" APP_VERSION);
  }
  else
  {
    mg_http_send_error(cc, 400, errmsg);
  }
}

void forward_http_request(struct mg_connection *nc, struct http_message *hm)
{
  zipkin_span_t span = create_session_span(nc, hm, "proxy", "request");
  struct mg_connection *cc;
  int i;
  const char *errmsg = NULL;
  struct mg_connect_opts opts = {nc, 0, &errmsg};
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
      p += snprintf(p, end - p, "%.*s: %.*s\n", (int)hn->len, hn->p, (int)hv->len, hv->p);
    }
  }

  gethostname(hostname, sizeof(hostname));
  mg_conn_addr_to_str(nc, local_addr, sizeof(local_addr), MG_SOCK_STRINGIFY_IP);
  mg_conn_addr_to_str(nc, peer_addr, sizeof(peer_addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_REMOTE);

  p += snprintf(p, end - p, HTTP_VIA ": 1.1 %s (%s/%s)\n", hostname, APP_NAME, APP_VERSION);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_FOR ": %s\n", peer_addr);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_PORT ": %d\n", nc->sa.sin.sin_port);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_PROTO ": %s\n", "http");
  p += snprintf(p, end - p, HTTP_FORWARDED ": for=%s;proto=http;by=%s\n", peer_addr, local_addr);
  p += snprintf(p, end - p, HTTP_X_SPAN_ID ": %llx", zipkin_span_id(span));

  cc = mg_connect_http_opt(nc->mgr, ev_handler, opts, uri, extra_headers, hm->body.p);

  zipkin_span_annotate_str(span, "upstream", hm->uri.p, hm->uri.len, NULL);
  zipkin_span_set_userdata(span, cc);

  nc->user_data = span;

  if (cc)
  {
    cc->user_data = zipkin_span_new_child(span, "upstream", nc);
  }
  else
  {
    mg_http_send_error(cc, 400, errmsg);
  }

  free(uri);
}

void reply_json_response(struct mg_connection *nc, struct http_message *hm)
{
  zipkin_span_t span = create_session_span(nc, hm, "http", "response");
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
    struct mg_str hn = hm->header_names[i];
    struct mg_str hv = hm->header_values[i];
    mg_printf(nc, "%s\"%.*s\": \"%.*s\"", (i != 0 ? "," : ""), (int)hn.len,
              hn.p, (int)hv.len, hv.p);
  }

  mg_printf(nc, "}}");

  nc->flags |= MG_F_SEND_AND_CLOSE;
  nc->user_data = span;
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
  struct http_message *hm = (struct http_message *)ev_data;
  zipkin_span_t span = (zipkin_span_t)nc->user_data;
  struct mg_connection *cc = span ? (struct mg_connection *)zipkin_span_userdata(span) : NULL;

  switch (ev)
  {
  case MG_EV_HTTP_REQUEST:
    if (0 == mg_vcasecmp(&hm->method, "CONNECT"))
    {
      forward_tcp_connection(nc, hm);
    }
    else if (mg_get_http_header(hm, HTTP_PROXY_CONNECTION))
    {
      forward_http_request(nc, hm);
    }
    else
    {
      reply_json_response(nc, hm);
    }

    break;

  case MG_EV_CONNECT:
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

  case MG_EV_HTTP_REPLY:
    zipkin_span_annotate_int16(span, "status", hm->resp_code, NULL);
    zipkin_span_annotate_int32(span, "size", hm->message.len, NULL);

    mg_send(cc, hm->message.p, hm->message.len);

    cc->flags |= MG_F_SEND_AND_CLOSE;
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;

    break;

  case MG_EV_RECV:
    if (cc)
    {
      zipkin_span_annotate_int32(span, "received", nc->recv_mbuf.len, NULL);

      mg_send(cc, nc->recv_mbuf.buf, nc->recv_mbuf.len);
      mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
    }
    break;

  case MG_EV_CLOSE:
    if (cc)
    {
      cc->flags |= MG_F_SEND_AND_CLOSE;
    }

    if (span)
    {
      zipkin_span_annotate(span, "closed", -1, NULL);

      zipkin_span_submit(span);
      zipkin_span_free(span);
    }
    break;
  }
}

zipkin_tracer_t create_tracer(const char *kafka_uri)
{
  struct mg_str uri = mg_mk_str(kafka_uri), host, path;
  unsigned int port = 0;
  char broker[1024] = {0};
  char *topic;
  zipkin_conf_t conf;
  zipkin_collector_t collector;
  zipkin_tracer_t tracer;

  if (mg_parse_uri(uri, NULL, NULL, &host, &port, &path, NULL, NULL))
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

  if (!conf)
    return NULL;

  collector = zipkin_collector_new(conf);

  zipkin_conf_free(conf);

  if (collector)
  {
    tracer = zipkin_tracer_new(collector, APP_NAME);
  }

  return tracer;
}

int main(int argc, char **argv)
{
  int c;
  const char *kafka_uri = NULL;

  struct mg_mgr mgr;
  struct mg_connection *nc;

  while ((c = getopt(argc, argv, "ht:")) != -1)
  {
    switch (c)
    {
    case 't':
      kafka_uri = optarg;
      break;

    case 'h':
    case '?':
      printf("%s [options]\n\n", argv[0]);
      printf("-t <uri>\tKafka for tracing\n");

      return 1;

    default:
      printf("unknown argument: %c", c);

      abort();
    }
  }

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);

  mg_mgr_init(&mgr, NULL);

  printf("Starting proxy on port %s\n", s_http_port);
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  if (nc == NULL)
  {
    printf("Failed to create listener\n");
    return 1;
  }

  if (kafka_uri)
  {
    mgr.user_data = create_tracer(kafka_uri);
  }

  mg_set_protocol_http_websocket(nc);

  while (!s_signal_received)
  {
    mg_mgr_poll(&mgr, 1000);
  }

  if (mgr.user_data)
  {
    zipkin_collector_free(zipkin_tracer_collector((zipkin_tracer_t)mgr.user_data));
  }

  mg_mgr_free(&mgr);

  return 0;
}