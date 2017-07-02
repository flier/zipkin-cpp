#pragma once

#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/lockfree/queue.hpp>

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

  virtual const char *name(void) const = 0;

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

  /**
  * \brief Shutdown the collector
  */
  virtual void shutdown(std::chrono::milliseconds timeout_ms) = 0;

  static Collector *create(const std::string &uri);
};

struct BaseConf
{
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
  * \brief the maximum duration we will buffer traces before emitting them to the collector.
  *
  * The default batch interval is 1 second.
  */
  std::chrono::milliseconds batch_interval = std::chrono::seconds(1);
};

class BaseCollector : public Collector
{
  boost::lockfree::queue<Span *> m_spans;
  std::atomic_size_t m_queued_spans = ATOMIC_VAR_INIT(0);

  std::thread m_worker;
  std::atomic_bool m_terminated = ATOMIC_VAR_INIT(false);
  std::mutex m_sending;
  std::condition_variable m_flush, m_sent;

  bool drop_front_span(void);

  void try_send_spans(void);

  void send_spans(void);

  static void run(BaseCollector *collector);

protected:
  std::unique_ptr<const BaseConf> m_conf;

protected:
  BaseCollector(const BaseConf *conf)
      : m_conf(conf), m_spans(conf->backlog), m_worker(BaseCollector::run, this)
  {
  }

  virtual ~BaseCollector()
  {
    if (!m_terminated.exchange(true)) {
      m_worker.detach();
    }
  }

  virtual void send_message(const uint8_t *msg, size_t size) = 0;

public:
  // Implement Collector

  virtual void submit(Span *span) override;

  virtual bool flush(std::chrono::milliseconds timeout_ms) override;

  virtual void shutdown(std::chrono::milliseconds timeout_ms) override;
};

} // namespace zipkin
