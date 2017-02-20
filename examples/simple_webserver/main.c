#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <string.h>

#include "mongoose.h"

#include "zipkin.h"

#define HTTP_VIA "Via"
#define HTTP_X_FORWARDED_FOR "X-Forwarded-For"
#define HTTP_X_FORWARDED_PORT "X-Forwarded-Port"
#define HTTP_X_FORWARDED_PROTO "X-Forwarded-Proto"
#define HTTP_FORWARDED "Forwarded"

static sig_atomic_t s_signal_received = 0;
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

void signal_handler(int sig_num)
{
  signal(sig_num, signal_handler); // Reinstantiate signal handler
  s_signal_received = sig_num;
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data);

void forward_tcp_connection(struct mg_connection *nc, struct http_message *hm)
{
}

struct mg_connection *forward_http_request(struct mg_connection *nc, struct http_message *hm)
{
  int i;
  struct mg_connect_opts opts = {nc};
  char *uri = strndup(hm->uri.p, hm->uri.len);
  char hostname[MAXHOSTNAMELEN] = {0};
  union socket_address addr;
  socklen_t addr_len = sizeof(addr);
  char *local_addr, *peer_addr;
  char extra_headers[4096] = {0};
  char *p = extra_headers, *end = extra_headers + sizeof(extra_headers) - 1;
  struct mg_str *proxy_conn = NULL, *proxy_auth = NULL;

  for (i = 0; i < MG_MAX_HTTP_HEADERS && hm->header_names[i].len > 0; i++)
  {
    struct mg_str *hn = &hm->header_names[i];
    struct mg_str *hv = &hm->header_values[i];

    if (0 == mg_vcmp(hn, "Proxy-Connection"))
    {
      proxy_conn = hv;
    }
    else if (0 != mg_vcmp(hn, "Proxy-Authorization"))
    {
      proxy_auth = hv;
    }
    else
    {
      p += snprintf(p, end - p, "%.*s: %.*s\n", (int)hn->len, hn->p, (int)hv->len, hv->p);
    }
  }

  gethostname(hostname, sizeof(hostname));
  getsockname(nc->sock, &addr.sa, &addr_len);
  local_addr = inet_ntoa(addr.sin.sin_addr);
  peer_addr = inet_ntoa(nc->sa.sin.sin_addr);

  p += snprintf(p, end - p, HTTP_VIA ": %s\n", hostname);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_FOR ": %s\n", peer_addr);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_PORT ": %d\n", nc->sa.sin.sin_port);
  p += snprintf(p, end - p, HTTP_X_FORWARDED_PROTO ": %s\n", "http");
  p += snprintf(p, end - p, HTTP_FORWARDED ": for=%s;proto=http;by=%s\n", peer_addr, local_addr);

  struct mg_connection *conn = mg_connect_http_opt(nc->mgr, ev_handler, opts, uri, extra_headers, hm->body.p);

  free(uri);

  return conn;
}

void forward_http_response(struct mg_connection *nc, struct http_message *hm)
{
  struct mg_connection *client_conn = (struct mg_connection *)nc->user_data;
  mg_send(client_conn, hm->message.p, hm->message.len);
  client_conn->flags |= MG_F_SEND_AND_CLOSE;
  nc->flags |= MG_F_CLOSE_IMMEDIATELY;
}

void reply_json_response(struct mg_connection *nc, struct http_message *hm)
{
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
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
  struct http_message *hm = (struct http_message *)ev_data;
  struct mg_str *proxy_connection = NULL;

  switch (ev)
  {
  case MG_EV_HTTP_REQUEST:
    if (0 == mg_vcasecmp(&hm->method, "CONNECT"))
    {
      forward_tcp_connection(nc, hm);
    }
    else if (NULL != (proxy_connection = mg_get_http_header(hm, "Proxy-Connection")))
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
      mg_http_send_error(nc->user_data, 502, NULL);
    }
    break;

  case MG_EV_HTTP_REPLY:
    forward_http_response(nc, hm);
    break;

  case MG_EV_CLOSE:
    if (nc->user_data)
    {
      ((struct mg_connection *)nc->user_data)->flags |= MG_F_SEND_AND_CLOSE;
    }
    break;
  }
}

int main(int argc, char **argv)
{
  struct mg_mgr mgr;
  struct mg_connection *nc;

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);

  mg_mgr_init(&mgr, NULL);

  printf("Starting web server on port %s\n", s_http_port);
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  if (nc == NULL)
  {
    printf("Failed to create listener\n");
    return 1;
  }

  mg_set_protocol_http_websocket(nc);

  while (!s_signal_received)
  {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}