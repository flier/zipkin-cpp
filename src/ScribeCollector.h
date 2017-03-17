#pragma once

#include <folly/Uri.h>

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>

#include "Collector.h"
#include "Scribe.h"

namespace zipkin
{

class ScribeCollector;

struct ScribeConf : public BaseConf
{
    /**
    * \breif the Scribe server address
    */
    std::string host = "localhost";

    /**
    * \breif the Scribe server port
    */
    port_t port = 1456;

    /**
    * \brief the Scribe category used to transmit the spans.
    *
    * The default category is zipkin
    */
    std::string category = "zipkin";

    /**
    * \brief the maximum retry times
    *
    * The default maximum retry times is 3
    */
    size_t max_retry_times = 3;

    ScribeConf(const std::string &h, port_t p = 1456) : host(h), port(p)
    {
    }

    ScribeConf(folly::Uri &uri);

    ScribeCollector *create(void) const;
};

class ScribeCollector : public BaseCollector
{
    boost::shared_ptr<apache::thrift::transport::TSocket> m_socket;
    boost::shared_ptr<apache::thrift::transport::TFramedTransport> m_transport;
    boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> m_protocol;
    boost::shared_ptr<ScribeClient> m_client;

    bool connected(void) const { return m_socket->isOpen(); }

    bool reconnect(void);

  public:
    ScribeCollector(const ScribeConf *conf)
        : BaseCollector(conf),
          m_socket(new apache::thrift::transport::TSocket(conf->host, conf->port)),
          m_transport(new apache::thrift::transport::TFramedTransport(m_socket)),
          m_protocol(new apache::thrift::protocol::TBinaryProtocol(m_transport)),
          m_client(new ScribeClient(m_protocol))
    {
    }

    const ScribeConf *conf(void) const { return static_cast<const ScribeConf *>(m_conf.get()); }

    // Implement Collector

    virtual const char *name(void) const override { return "Scribe"; }

    virtual void send_message(const uint8_t *msg, size_t size) override;
};

} // namespace zipkin