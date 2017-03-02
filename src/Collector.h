#pragma once

#include <chrono>

#include <thrift/transport/TBufferTransports.h>

#include "Span.h"

namespace zipkin
{

/**
 * \brief use for compressing message sets.
 *
 * \sa https://cwiki.apache.org/confluence/display/KAFKA/Compression
 */
enum CompressionCodec
{
  none,   ///< No compression
  gzip,   ///< GZIP compression
  snappy, ///< Snappy compression
  lz4     ///< LZ4 compression
};

CompressionCodec parse_compression_codec(const std::string &codec);
const std::string to_string(CompressionCodec codec);

class BinaryCodec;
class JsonCodec;
class PrettyJsonCodec;

/**
* \brief use for encoding message sets.
*/
class MessageCodec
{
public:
  virtual ~MessageCodec(void) = default;

  virtual const std::string name(void) const = 0;

  virtual const std::string mime_type(void) const = 0;

  virtual size_t encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans) = 0;

  static std::shared_ptr<MessageCodec> parse(const std::string &codec);

  static std::shared_ptr<BinaryCodec> binary;
  static std::shared_ptr<JsonCodec> json;
  static std::shared_ptr<PrettyJsonCodec> pretty_json;
};

/**
* \brief Thrift binary encoding
*/
class BinaryCodec : public MessageCodec
{
public:
  virtual const std::string name(void) const override { return "binary"; }

  virtual const std::string mime_type(void) const override { return "application/x-thrift"; }

  virtual size_t encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans) override;
};

/**
* \brief JSON encoding
*/
class JsonCodec : public MessageCodec
{
public:
  virtual const std::string name(void) const override { return "json"; }

  virtual const std::string mime_type(void) const override { return "application/json"; }

  virtual size_t encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans) override;
};

/**
* \brief Pretty print JSON encoding
*/
class PrettyJsonCodec : public MessageCodec
{
public:
  virtual const std::string name(void) const override { return "pretty_json"; }

  virtual const std::string mime_type(void) const override { return "application/json"; }

  virtual size_t encode(boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf, const std::vector<Span *> &spans) override;
};

/**
* \brief Push Span as messages to transport
*/
struct Collector
{
  virtual ~Collector() = default;

  /**
  * \brief Async submit Span to transport
  *
  * \param span encoded to message and send to the transport,
  *
  * Span#release will be call after the message was sent.
  */
  virtual void submit(Span *span) = 0;

  /**
  * \brief Wait until all outstanding messages, et.al, are sent.
  *
  * \param timeout_ms the maximum amount of time (in milliseconds) that the call will block waiting.
  * \return \c true if all outstanding messages were sent, or \c false if the \p timeout_ms was reached.
  */
  virtual bool flush(std::chrono::milliseconds timeout_ms) = 0;
};

} // namespace zipkin