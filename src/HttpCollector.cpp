#include "HttpCollector.h"

#include <sstream>

#include <glog/logging.h>

#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <zlib.h>

#include "Version.h"

namespace zipkin
{

HttpConf::HttpConf(folly::Uri &uri)
{
    std::ostringstream oss;

    oss << uri.scheme() << "://";

    if (!uri.username().empty())
    {
        oss << uri.username();

        if (!uri.password().empty())
        {
            oss << ":" << uri.password();
        }

        oss << "@";
    }

    oss << uri.host();

    if (uri.port())
    {
        oss << ":" << uri.port();
    }

    oss << uri.path();

    url = oss.str();

    for (auto &param : uri.getQueryParams())
    {
        if (param.first == "format")
        {
            message_codec = MessageCodec::parse(folly::toStdString(param.second));
        }
        else if (param.first == "batch_size")
        {
            batch_size = folly::to<size_t>(param.second);
        }
        else if (param.first == "backlog")
        {
            backlog = folly::to<size_t>(param.second);
        }
        else if (param.first == "max_redirect_times")
        {
            max_redirect_times = folly::to<size_t>(param.second);
        }
        else if (param.first == "connect_timeout")
        {
            connect_timeout = std::chrono::milliseconds(folly::to<size_t>(param.second));
        }
        else if (param.first == "request_timeout")
        {
            request_timeout = std::chrono::milliseconds(folly::to<size_t>(param.second));
        }
        else if (param.first == "batch_interval")
        {
            batch_interval = std::chrono::milliseconds(folly::to<size_t>(param.second));
        }
    }
}

HttpCollector *HttpConf::create(void) const
{
    return new HttpCollector(this);
}

#define GZIP_WINDOW_BITS 15
#define GZIP_ENCODING 16

CURLcode HttpCollector::upload_messages(const uint8_t *data, size_t size)
{
    CURLcode res;
    struct curl_slist *headers = nullptr;
    char content_type[128] = {0}, err_msg[CURL_ERROR_SIZE] = {0};
    std::vector<uint8_t> compressed(size);
    CURL *curl = curl_easy_init();

    if (!curl)
    {
        LOG(ERROR) << "fail to create curl handle";

        return CURLE_FAILED_INIT;
    }

    int ret = Z_OK;
    z_stream defstream = {0};

    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;

    defstream.next_in = const_cast<uint8_t *>(data);
    defstream.avail_in = size;
    defstream.next_out = compressed.data();
    defstream.avail_out = compressed.size();

    if (Z_OK != (ret = deflateInit2(&defstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, GZIP_WINDOW_BITS | GZIP_ENCODING, 9, Z_DEFAULT_STRATEGY)))
    {
        LOG(WARNING) << "fail to initialize zlib stream, err=" << ret << ", msg=" << defstream.msg;
    }
    else if (Z_STREAM_END != (ret = deflate(&defstream, Z_FINISH)))
    {
        LOG(WARNING) << "fail to compress data, err=" << ret << ", msg=" << defstream.msg;
    }
    else if (Z_OK != (ret = deflateEnd(&defstream)))
    {
        LOG(WARNING) << "fail to flush compressed stream, err=" << ret << ", msg=" << defstream.msg;
    }
    else
    {
        VLOG(2) << size << " bytes " << conf()->message_codec->name()
                << " message was compressed to " << defstream.total_out
                << " bytes (%" << ((double)defstream.total_out * 100 / size) << ")";

        compressed.resize(defstream.total_out);
        headers = curl_slist_append(headers, "Content-Encoding: gzip");
        data = compressed.data();
        size = compressed.size();
    }

    const std::string mime_type = conf()->message_codec->mime_type();
    snprintf(content_type, sizeof(content_type), "Content-Type: %s", mime_type.c_str());

    headers = curl_slist_append(headers, content_type);
    headers = curl_slist_append(headers, "Expect:");

    if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_msg)))
    {
        LOG(WARNING) << "fail to set curl error buffer, " << curl_easy_strerror(res);
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_URL, conf()->url.c_str())))
    {
        LOG(WARNING) << "fail to set url, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers)))
    {
        LOG(WARNING) << "fail to set http header, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_USERAGENT, ZIPKIN_LIBNAME "/" ZIPKIN_VERSION)))
    {
        LOG(WARNING) << "fail to set http user agent, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, size)))
    {
        LOG(WARNING) << "fail to set http body size, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data)))
    {
        LOG(WARNING) << "fail to set http body data, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, conf()->connect_timeout.count())))
    {
        LOG(WARNING) << "fail to set connect timeout, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, conf()->request_timeout.count())))
    {
        LOG(WARNING) << "fail to set request timeout, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
    }
    else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1)))
    {
        LOG(WARNING) << "fail to disable signals, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
    }
    else
    {
        if (VLOG_IS_ON(3))
        {
            if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback)))
            {
                LOG(WARNING) << "fail to set debug callback, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }
            else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1)))
            {
                LOG(WARNING) << "fail to set verbose output, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }
        }

        if (!conf()->proxy.empty())
        {
            if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_PROXY, conf()->proxy.c_str())))
            {
                LOG(WARNING) << "fail to set proxy, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }

            if (conf()->http_proxy_tunnel)
            {
                if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP)))
                {
                    LOG(WARNING) << "fail to set HTTP proxy type, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
                }
                else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1)))
                {
                    LOG(WARNING) << "fail to set HTTP proxy tunnel, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
                }
            }
        }

        if (conf()->max_redirect_times)
        {
            if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1)))
            {
                LOG(WARNING) << "fail to enable follow location, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }
            else if (CURLE_OK != (res = curl_easy_setopt(curl, CURLOPT_MAXREDIRS, conf()->max_redirect_times)))
            {
                LOG(WARNING) << "fail to set max redirect times, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }
        }

        LOG(INFO) << "sending HTTP request to " << conf()->url;

        if (CURLE_OK != (res = curl_easy_perform(curl)))
        {
            LOG(WARNING) << "fail to send http request, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
        }
        else
        {
            long status_code = 0;
            double total_time = 0, uploaded_bytes = 0, upload_speed = 0;

            if (CURLE_OK != (res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code)))
            {
                LOG(WARNING) << "fail to get status code, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }
            else if (CURLE_OK != (res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time)))
            {
                LOG(WARNING) << "fail to get total time, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }
            else if (CURLE_OK != (res = curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &uploaded_bytes)))
            {
                LOG(WARNING) << "fail to get uploaded bytes, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }
            else if (CURLE_OK != (res = curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &upload_speed)))
            {
                LOG(WARNING) << "fail to get upload speed, " << (strlen(err_msg) ? err_msg : curl_easy_strerror(res));
            }
            else
            {
                LOG(INFO) << "HTTP request finished, status " << status_code
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
        VLOG(3) << "< " << size << " bytes";

        break;
    }
    case CURLINFO_DATA_OUT:
    {
        VLOG(3) << "> " << size << " bytes";

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
