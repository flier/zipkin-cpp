#pragma once

#include <curl/curl.h>

#include <folly/Uri.h>

#include "Collector.h"

namespace zipkin
{

class HttpCollector;

struct HttpConf : public BaseConf
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

class HttpCollector : public BaseCollector
{
    static CUrlEnv s_curl_env;

    CURLcode upload_messages(const uint8_t *data, size_t size);

    static int debug_callback(CURL *handle,
                              curl_infotype type,
                              char *data,
                              size_t size,
                              void *userptr);

  public:
    HttpCollector(const HttpConf *conf) : BaseCollector(conf)
    {
    }

    const HttpConf *conf(void) const { return static_cast<const HttpConf *>(m_conf.get()); }

    // Implement Collector

    virtual const char *name(void) const override { return "HTTP"; }

    virtual void send_message(const uint8_t *msg, size_t size) override
    {
        upload_messages(msg, size);
    }
};

} // namespace zipkin