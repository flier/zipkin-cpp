#include "HttpCollector.h"

#include <glog/logging.h>

#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include "Version.h"

namespace zipkin
{

HttpCollector *HttpConf::create(void) const
{
    return new HttpCollector(*this);
}

HttpCollector::HttpCollector(const HttpConf &conf)
    : m_conf(conf), m_spans(conf.backlog), m_queued_spans(0),
      m_terminated(false), m_worker(HttpCollector::run, this)
{
}

HttpCollector::~HttpCollector()
{
    flush(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::microseconds(500)));
}

bool HttpCollector::drop_front_span()
{
    CachedSpan *span = nullptr;

    if (m_spans.pop(span) && span)
    {
        LOG(WARNING) << "Drop Span `" << std::hex << span->id() << " exceed backlog";

        span->release();

        return true;
    }

    return false;
}

void HttpCollector::submit(Span *span)
{
    Span *old = nullptr;

    while (m_queued_spans >= m_conf.backlog)
    {
        if (drop_front_span())
            m_queued_spans--;
    }

    if (m_spans.push(span))
        m_queued_spans++;

    if (m_queued_spans >= m_conf.batch_size)
        flush(std::chrono::milliseconds(0));
}

bool HttpCollector::flush(std::chrono::milliseconds timeout_ms)
{
    VLOG(2) << "flush pendding " << m_queued_spans << " spans";

    if (m_spans.empty())
        return true;

    m_flush.notify_one();

    std::unique_lock<std::mutex> lock(m_flushing);

    return m_sent.wait_for(lock, timeout_ms, [this] { return m_spans.empty(); });
}

void HttpCollector::try_send_spans(void)
{
    std::unique_lock<std::mutex> lock(m_sending);

    if (m_flush.wait_for(lock, m_conf.batch_interval, [this] { return !m_spans.empty(); }))
    {
        send_spans();
    }
}

void HttpCollector::send_spans(void)
{
    VLOG(2) << "sending " << m_queued_spans << " spans";

    CachedSpan *span = nullptr;
    std::vector<Span *> spans;

    m_queued_spans -= m_spans.consume_all([&spans](Span *span) {
        spans.push_back(span);
    });

    boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf(new apache::thrift::transport::TMemoryBuffer());

    m_conf.message_codec->encode(buf, spans);

    for (auto span : spans)
    {
        span->release();
    }

    uint8_t *msg = nullptr;
    uint32_t size = 0;

    buf->getBuffer(&msg, &size);

    upload_messages(msg, size);

    m_sent.notify_all();
}

void HttpCollector::run(HttpCollector *collector)
{
    VLOG(1) << "HTTP collector started";

    do
    {
        collector->try_send_spans();

        std::this_thread::yield();
    } while (!collector->m_terminated);

    VLOG(1) << "HTTP collector stopped";
}

CURLcode HttpCollector::upload_messages(const uint8_t *data, size_t size)
{
    CURLcode res;
    struct curl_slist *headers = nullptr;
    char content_type[128] = {0}, err_msg[CURL_ERROR_SIZE] = {0};
    CURL *curl = curl_easy_init();

    if (!curl)
    {
        LOG(ERROR) << "fail to create curl handle";

        return CURLE_FAILED_INIT;
    }

    if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_msg)))
    {
        LOG(WARNING) << "fail to set curl error buffer, " << curl_easy_strerror(res);
    }

    snprintf(content_type, sizeof(content_type), "Content-Type: %s", m_conf.message_codec->mime_type().c_str());

    headers = curl_slist_append(headers, content_type);

    if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_URL, m_conf.url.c_str())))
    {
        LOG(WARNING) << "fail to set url, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers)))
    {
        LOG(WARNING) << "fail to set http header, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_USERAGENT, ZIPKIN_LIBNAME "/" ZIPKIN_VERSION)))
    {
        LOG(WARNING) << "fail to set http user agent, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data)))
    {
        LOG(WARNING) << "fail to set http body data, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, size)))
    {
        LOG(WARNING) << "fail to set http body size, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_conf.connect_timeout.count())))
    {
        LOG(WARNING) << "fail to set connect timeout, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, m_conf.request_timeout.count())))
    {
        LOG(WARNING) << "fail to set request timeout, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1)))
    {
        LOG(WARNING) << "fail to disable signals, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
    }
    else
    {
        if (VLOG_IS_ON(3))
        {
            if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback)))
            {
                LOG(WARNING) << "fail to set debug callback, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }
            else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1)))
            {
                LOG(WARNING) << "fail to set verbose output, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }
        }

        if (!m_conf.proxy.empty())
        {
            if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_PROXY, m_conf.proxy.c_str())))
            {
                LOG(WARNING) << "fail to set proxy, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }

            if (m_conf.http_proxy_tunnel)
            {
                if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP)))
                {
                    LOG(WARNING) << "fail to set HTTP proxy type, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
                }
                else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1)))
                {
                    LOG(WARNING) << "fail to set HTTP proxy tunnel, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
                }
            }
        }

        if (m_conf.max_redirect_times)
        {
            if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1)))
            {
                LOG(WARNING) << "fail to enable follow location, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }
            else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_MAXREDIRS, m_conf.max_redirect_times)))
            {
                LOG(WARNING) << "fail to set max redirect times, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }
        }

        if (CURLE_OK != (res = curl_easy_perform(curl)))
        {
            LOG(WARNING) << "fail to send http request, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
        }
        else
        {
            long status_code = 0;
            double total_time = 0, uploaded_bytes = 0, upload_speed = 0;

            if (CURLE_OK != (res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code)))
            {
                LOG(WARNING) << "fail to get status code, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }
            else if (CURLE_OK != (res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time)))
            {
                LOG(WARNING) << "fail to get total time, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }
            else if (CURLE_OK != (res = curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &uploaded_bytes)))
            {
                LOG(WARNING) << "fail to get uploaded bytes, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }
            else if (CURLE_OK != (res = curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &upload_speed)))
            {
                LOG(WARNING) << "fail to get upload speed, " << strlen(err_msg) ? err_msg : curl_easy_strerror(res);
            }
            else
            {
                LOG(INFO) << "HTTP port succeeded, status " << status_code
                          << ", uploaded " << uploaded_bytes << " bytes in " << total_time << " seconds (" << (upload_speed / 1024) << " KB/s)";
            }
        }
    }

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    return res;
}

int HttpCollector::debug_callback(CURL *handle,
                                  curl_infotype type,
                                  char *data,
                                  size_t size,
                                  void *userptr)
{
    switch (type)
    {
    case CURLINFO_TEXT:
    {
        VLOG(3) << "* " << std::string(data, size);

        break;
    }
    case CURLINFO_HEADER_IN:
    {
        VLOG(3) << "< " << std::string(data, size);

        break;
    }
    case CURLINFO_HEADER_OUT:
    {
        VLOG(3) << "> " << std::string(data, size);

        break;
    }
    case CURLINFO_DATA_IN:
    {
        VLOG(3) << std::string(data, size);

        break;
    }
    case CURLINFO_DATA_OUT:
    {
        VLOG(3) << std::string(data, size);

        break;
    }
    case CURLINFO_SSL_DATA_IN:
        break;
    case CURLINFO_SSL_DATA_OUT:
        break;
    case CURLINFO_END:
        break;
    }

    return 0;
}

} // namespace zipkin