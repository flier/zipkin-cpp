# Copyright 2012 Twitter Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
namespace java com.twitter.zipkin.thriftjava
#@namespace scala com.twitter.zipkin.thriftscala
namespace rb Zipkin

#************** Annotation.value **************
const string CLIENT_SEND = "cs"
const string CLIENT_RECV = "cr"
const string SERVER_SEND = "ss"
const string SERVER_RECV = "sr"
const string WIRE_SEND = "ws"
const string WIRE_RECV = "wr"
const string CLIENT_SEND_FRAGMENT = "csf"
const string CLIENT_RECV_FRAGMENT = "crf"
const string SERVER_SEND_FRAGMENT = "ssf"
const string SERVER_RECV_FRAGMENT = "srf"

#***** BinaryAnnotation.key ******
const string HTTP_HOST = "http.host"
const string HTTP_METHOD = "http.method"
const string HTTP_PATH = "http.path"
const string HTTP_URL = "http.url"
const string HTTP_STATUS_CODE = "http.status_code"
const string HTTP_REQUEST_SIZE = "http.request.size"
const string HTTP_RESPONSE_SIZE = "http.response.size"
const string LOCAL_COMPONENT = "lc"
const string CLIENT_ADDR = "ca"
const string SERVER_ADDR = "sa"

struct Endpoint {
  1: i32 ipv4
  2: i16 port
  3: string service_name
}

struct Annotation {
  1: i64 timestamp
  2: string value
  3: optional Endpoint host
}

enum AnnotationType {
  BOOL,
  BYTES,
  I16,
  I32,
  I64,
  DOUBLE,
  STRING
}

struct BinaryAnnotation {
  1: string key,
  2: binary value,
  3: AnnotationType annotation_type,
  4: optional Endpoint host
}

struct Span {
  1: i64 trace_id
  3: string name,
  4: i64 id,
  5: optional i64 parent_id,
  6: list<Annotation> annotations,
  8: list<BinaryAnnotation> binary_annotations
  9: optional bool debug = 0
  10: optional i64 timestamp,
  11: optional i64 duration
}
