#include <random>
#include <utility>

#include <chrono>
#include <string>
#include <locale>
#include <codecvt>

#include "Span.h"

namespace zipkin
{

uint64_t next_id()
{
    thread_local std::mt19937_64 rand_gen((std::chrono::system_clock::now().time_since_epoch().count() << 32) + std::random_device()());

    return rand_gen();
}

timestamp_t timestamp()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void Span::annotate(const std::string &key, const std::vector<uint8_t> &value, const Endpoint *endpoint)
{
    TypedAnnotation annotation = {key, value, AnnotationType::BYTES, endpoint};

    typed_annotates.push_back(annotation);
}

void Span::annotate(const std::string &key, const std::string &value, const Endpoint *endpoint)
{
    TypedAnnotation annotation = {key, std::vector<uint8_t>(value.cbegin(), value.cend()), AnnotationType::STRING, endpoint};

    typed_annotates.push_back(annotation);
}

void Span::annotate(const std::string &key, const std::wstring &value, const Endpoint *endpoint)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    const std::string utf8 = converter.to_bytes(value);
    TypedAnnotation annotation = {key, std::vector<uint8_t>(utf8.cbegin(), utf8.cend()), AnnotationType::STRING, endpoint};

    typed_annotates.push_back(annotation);
}

} // namespace zipkin