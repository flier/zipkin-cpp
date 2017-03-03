#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <curl/curl.h>

#include <folly/Uri.h>

#include <boost/lockfree/queue.hpp>

#include "Collector.h"

namespace zipkin
{

class HttpCollector;

struct HttpConf
{
    /**
    * \brief the URL to use in the request
    *
    * \sa https://curl.haxx.se/libcurl/c/CURLOPT_URL.html
    */
    std::string url;

    /**
    * \brief the proxy to use for the upcoming request.
    *
    * \sa https://curl.haxx.se/libcurl/c/CURLOPT_PROXY.html
    */
    std::string proxy;

    /**
    * \brief tunnel through HTTP proxy
    *
    * \sa https://curl.haxx.se/libcurl/c/CURLOPT_HTTPPROXYTUNNEL.html
    */
    bool http_proxy_tunnel = false;

    /**
    * \brief Message codec to use for encoding message sets.
    *
    * default: binary
    */
    std::shared_ptr<MessageCodec> message_codec = MessageCodec::binary;

    /**
    * \brief the maximum batch size, after which a collect will be triggered.
    *
    * The default batch size is 100 traces.
    */
    size_t batch_size = 100;

    /**
    * \brief the maximum backlog size
    *
    * when batch size reaches this threshold spans from the beginning of the batch will be disposed
    *
    * The default maximum backlog size is 1000
    */
    size_t backlog = 1000;

    /**
    * \brief maximum number of redirects allowed
    *
    * The default maximum redirect times is 3
    *
    * \sa https://curl.haxx.se/libcurl/c/CURLOPT_MAXREDIRS.html
    */
    size_t max_redirect_times = 3;

    /**
    * \brief the maximum timeout for TCP connect.
    *
    * The default connect timeout is 5 seconds
    *
    * \sa https://curl.haxx.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
    */
    std::chrono::milliseconds connect_timeout = std::chrono::seconds(5);

    /**
    * \brief the maximum timeout for HTTP request.
    *
    * The default request timeout is 15 seconds
    *
    * \sa https://curl.haxx.se/libcurl/c/CURLOPT_TIMEOUT_MS.html
    */
    std::chrono::milliseconds request_timeout = std::chrono::seconds(15);

    /**
    * \brief the maximum duration we will buffer traces before emitting them to the collector.
    *
    * The default batch interval is 1 second.
    */
    std::chrono::milliseconds batch_interval = std::chrono::seconds(1);

    HttpConf(const std::string u) : url(u)
    {
    }

    HttpConf(folly::Uri &uri);

    /**
    * \brief Create HttpCollector base on the configuration
    */
    HttpCollector *create(void) const;
};

struct CUrlEnv
{
    CUrlEnv() { curl_global_init(CURL_GLOBAL_SSL); }
    ~CUrlEnv() { curl_global_cleanup(); }
};

class HttpCollector : public Collector
{
    static CUrlEnv s_curl_env;

    HttpConf m_conf;
    boost::lockfree::queue<Span *> m_spans;
    std::atomic_size_t m_queued_spans;

    std::thread m_worker;
    std::atomic_bool m_terminated;
    std::mutex m_sending, m_flushing;
    std::condition_variable m_flush, m_sent;

    bool drop_front_span(void);
    void try_send_spans(void);
    void send_spans(void);
    CURLcode upload_messages(const uint8_t *data, size_t size);

    static void run(HttpCollector *collector);

    static int debug_callback(CURL *handle,
                              curl_infotype type,
                              char *data,
                              size_t size,
                              void *userptr);

  public:
    HttpCollector(const HttpConf &conf);
    ~HttpCollector();

    virtual void submit(Span *span) override;

    virtual bool flush(std::chrono::milliseconds timeout_ms) override;
};

} // namespace zipkin