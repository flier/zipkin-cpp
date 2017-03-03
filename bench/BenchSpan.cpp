#include <benchmark/benchmark_api.h>

#include <utility>

#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include "Span.h"
#include "Tracer.h"

void bench_span_reuse(benchmark::State &state)
{
    std::unique_ptr<zipkin::Tracer> tracer(zipkin::Tracer::create(nullptr));
    std::vector<zipkin::Span *> spans(state.range(0));

    while (state.KeepRunning())
    {
        for (int i = 0; i < spans.size(); i++)
        {
            spans[i] = tracer->span("bench");
        }
        for (int i = 0; i < spans.size(); i++)
        {
            spans[i]->release();
        }
    }

    state.SetItemsProcessed(state.iterations() * spans.size());
}

BENCHMARK(bench_span_reuse)->RangeMultiplier(4)->Range(1, 512)->ThreadPerCpu();

void bench_span_annonate(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");

    while (state.KeepRunning())
    {
        span << zipkin::TraceKeys::CLIENT_SEND;
    }

    state.SetItemsProcessed(span.message().annotations.size());
}

BENCHMARK(bench_span_annonate);

void bench_span_annonate_with_endpoint(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");
    zipkin::Endpoint endpoint("bench");

    while (state.KeepRunning())
    {
        span << zipkin::TraceKeys::CLIENT_SEND << endpoint;
    }

    state.SetItemsProcessed(span.message().annotations.size());
}

BENCHMARK(bench_span_annonate_with_endpoint);

void bench_span_annonate_bool(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, false);
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_bool);

void bench_span_annonate_bool_with_endpoint(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");
    zipkin::Endpoint endpoint("bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, false) << endpoint;
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_bool_with_endpoint);

void bench_span_annonate_int16(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, (int16_t)123);
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_int16);

void bench_span_annonate_int32(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, (int32_t)123);
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_int32);

void bench_span_annonate_int64(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, (int64_t)123);
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_int64);

void bench_span_annonate_double(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, (double)12.3);
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_double);

void bench_span_annonate_cstr(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, "hello world");
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_cstr);

void bench_span_annonate_string(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");
    std::string value("hello");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, value);
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_string);

void bench_span_annonate_string_with_endpoint(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");
    std::string value("hello");
    zipkin::Endpoint endpoint("bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, value) << endpoint;
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_string_with_endpoint);

void bench_span_annonate_wcstr(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, L"hello world");
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_wcstr);

void bench_span_annonate_wstring(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");
    std::wstring value(L"hello world");

    while (state.KeepRunning())
    {
        span << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, value);
    }

    state.SetItemsProcessed(span.message().binary_annotations.size());
}

BENCHMARK(bench_span_annonate_wstring);

void bench_span_serialize_binary(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");
    zipkin::Endpoint endpoint("bench");

    span << zipkin::TraceKeys::CLIENT_SEND
         << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, false) << endpoint
         << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, L"hello world");

    boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buf(new apache::thrift::transport::TMemoryBuffer());
    apache::thrift::protocol::TBinaryProtocol protocol(buf);

    while (state.KeepRunning())
    {
        span.serialize_binary(protocol);
        buf->resetBuffer();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(bench_span_serialize_binary);

void bench_span_serialize_json(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");
    zipkin::Endpoint endpoint("bench");

    span << zipkin::TraceKeys::CLIENT_SEND
         << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, false) << endpoint
         << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, L"hello world");

    rapidjson::StringBuffer buffer;

    while (state.KeepRunning())
    {
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        span.serialize_json(writer);
        buffer.Clear();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(bench_span_serialize_json);

void bench_span_serialize_pretty_json(benchmark::State &state)
{
    zipkin::Span span(nullptr, "bench");
    zipkin::Endpoint endpoint("bench");

    span << zipkin::TraceKeys::CLIENT_SEND
         << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, false) << endpoint
         << std::make_pair(zipkin::TraceKeys::CLIENT_SEND, L"hello world");

    rapidjson::StringBuffer buffer;

    while (state.KeepRunning())
    {
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        span.serialize_json(writer);
        buffer.Clear();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(bench_span_serialize_pretty_json);

BENCHMARK_MAIN();