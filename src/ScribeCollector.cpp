#include "ScribeCollector.h"

#include <glog/logging.h>

#include <folly/Format.h>

#include "Base64.h"

namespace zipkin
{
ScribeConf::ScribeConf(folly::Uri &uri)
{
    std::vector<folly::StringPiece> parts;

    host = folly::toStdString(uri.host());

    if (uri.port())
        port = uri.port();

    folly::split("/", uri.path(), parts);

    if (parts.size() > 1)
        category = parts[1].str();

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
        else if (param.first == "max_retry_times")
        {
            max_retry_times = folly::to<size_t>(param.second);
        }
        else if (param.first == "batch_interval")
        {
            batch_interval = std::chrono::milliseconds(folly::to<size_t>(param.second));
        }
    }
}

ScribeCollector *ScribeConf::create(void) const
{
    return new ScribeCollector(this);
}

void ScribeCollector::send_message(const uint8_t *msg, size_t size)
{
    LogEntry entry;

    entry.__set_category(conf()->category);
    entry.__set_message(base64::encode(msg, size));

    std::vector<LogEntry> entries;

    entries.push_back(entry);

    ResultCode::type res = ResultCode::type::TRY_LATER;
    size_t retry_times = 0;

    do
    {
        if (connected() || reconnect())
        {
            res = m_client->Log(entries);
        }
    } while (res == ResultCode::type::TRY_LATER && retry_times++ < conf()->max_retry_times);

    VLOG(1) << entries.size() << " message was sent after retry " << retry_times << " times";
}

bool ScribeCollector::reconnect(void)
{
    try
    {
        m_socket->open();

        VLOG(1) << "conntected to scribe server @ " << conf()->host << ":" << conf()->port;
    }
    catch (apache::thrift::transport::TTransportException &ex)
    {
        LOG(WARNING) << "fail to connect scribe server @ " << conf()->host << ":" << conf()->port
                     << ", drop the sending message, " << ex.what();

        m_socket->close();
    }

    return m_socket->isOpen();
}

} // namespace zipkin
