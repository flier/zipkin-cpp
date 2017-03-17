#pragma once

#include <folly/Uri.h>

#include <boost/asio.hpp>
using boost::asio::ip::udp;

#include "Collector.h"

namespace zipkin
{

class XRayCollector;

/**
* \brief X-Ray encoding
*
* \sa http://docs.aws.amazon.com/xray/latest/devguide/xray-api.html
*/
class XRayCodec : public MessageCodec
{
  public:
    virtual const std::string name(void) const override { return "xray"; }

    virtual const std::string mime_type(void) const override { return "application/json"; }

    virtual size_t encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans) override;
};

struct XRayConf : public BaseConf
{
    static std::shared_ptr<XRayCodec> xray;

    /**
    * \breif the X-Ray daemon hostname
    */
    std::string host = "localhost";

    /**
    * \breif the X-Ray daemon port
    */
    port_t port = 2000;

    XRayConf(const std::string &h, port_t p = 2000) : host(h), port(p)
    {
        message_codec = XRayConf::xray;
    }

    XRayConf(folly::Uri &uri);

    XRayCollector *create(void) const;
};

class XRayCollector : public BaseCollector
{
    boost::asio::io_service m_io_service;
    udp::socket m_socket;
    udp::endpoint m_receiver;

  public:
    XRayCollector(const XRayConf *conf)
        : BaseCollector(conf), m_socket(m_io_service)
    {
        udp::resolver resolver(m_io_service);
        udp::resolver::query query(udp::v4(), conf->host, "");
        m_receiver = *resolver.resolve(query);
        m_receiver.port(conf->port);

        m_socket.open(udp::v4());
    }

    // Implement Collector

    virtual const char *name(void) const override { return "X-Ray"; }

    virtual void send_message(const uint8_t *msg, size_t size) override
    {
        m_socket.send_to(boost::asio::buffer(msg, size), m_receiver);
    }
};

} // namespace zipkin