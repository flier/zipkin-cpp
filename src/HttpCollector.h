#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <curl/curl.h>

#include <boost/lockfree/queue.hpp>

#include "Collector.h"

namespace zipkin
{

class HttpCollector;

struct HttpConf
{
    std::string url;

    /**
    * \brief Message codec to use for encoding message sets.
    *
    * default: binary
    */
    std::shared_ptr<MessageCodec> message_codec;

    /**
    * \brief the maximum batch size, after which a collect will be triggered.
    *
    * The default batch size is 100 traces.
    */
    size_t batch_size;

    /**
    * \brief the maximum backlog size
    *
    * when batch size reaches this threshold spans from the beginning of the batch will be disposed
    */
    size_t backlog;

    /**
    * \brief the maximum timeout for http request.
    *
    * The default request timeout is 5 seconds
    */
    std::chrono::microseconds request_timeout;

    /**
    * \brief the maximum duration we will buffer traces before emitting them to the collector.
    *
    * The default batch interval is 1 second.
    */
    std::chrono::microseconds batch_interval;

    HttpConf(const std::string u)
        : url(u), message_codec(MessageCodec::binary), batch_size(100), backlog(1000),
          request_timeout(std::chrono::seconds(5)), batch_interval(std::chrono::seconds(1))
    {
    }

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

    static void run(HttpCollector *collector);

  public:
    HttpCollector(const HttpConf &conf);
    ~HttpCollector();

    virtual void submit(Span *span) override;

    virtual bool flush(std::chrono::milliseconds timeout_ms) override;
};

} // namespace zipkin