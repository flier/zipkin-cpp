# Build from Source

## Dependencies

### Mac OSX

You could install dependencies with [Homebrew](https://brew.sh/).

```sh
$ brew update
$ brew install curl double-conversion gflags glog google-benchmark gperftools rapidjson folly thrift librdkafka grpc doxygen
```

### Linux

`zipkin-cpp` requires gcc 4.8+ and a version of boost compiled with C++11 support.

#### Ubuntu

```sh
$ sudo apt-get install build-essential automake autoconf bison flex libevent-dev libboost-all-dev libssl-dev libcurl4-openssl-dev libdouble-conversion-dev thrift-compiler thrift-compiler
```

#### CentOS

```sh
$ sudo yum install autoconf libtool libevent-devel boost-devel libcurl-devel double-conversion-devel thrift-devel gflags-devel glog-devel
```

### Install from Source

`zipkin-cpp` will try to install the missing packages from source, or you could build and install those by manual.

#### folly

Follow the [build notes](https://github.com/facebook/folly#build-notes) to install the [folly](https://github.com/facebook/folly) library.

#### librdkafka

Follow the [instructions](https://github.com/edenhill/librdkafka/#instructions) to install the [librdkafka](https://github.com/edenhill/librdkafka) library;

#### thrift

Follow the [build guide](https://thrift.apache.org/docs/BuildingFromSource) to install the [thrift](https://thrift.apache.org/).

## Get source files

Download the latest release from the [releases page](https://github.com/flier/zipkin-cpp/releases) or clone from the [git repo](https://github.com/flier/zipkin-cpp.git).

```sh
$ git clone https://github.com/flier/zipkin-cpp.git
```

Build `zipkin-cpp` with CMake

```sh
$ mkdir build
$ cd build
$ cmake ..
```

When build with `homebrew` at OSX, we must give the OPENSSL_DIR
```
$ cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl
```

### CMake options
- [CMAKE_BUILD_TYPE](https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html) default is `RELWITHDEBINFO`
- [CMAKE_INSTALL_PREFIX](https://cmake.org/cmake/help/v3.0/variable/CMAKE_INSTALL_PREFIX.html) default is `/usr/local`

### Build options

- **WITH_CURL**     Build with cURL propagation
- **WITH_GRPC**     Build with gRPC propagation
- **WITH_FPIC**     Build with -fPIC for shared library, default is `OFF`
- **WITH_TCMALLOC** Build with tcmalloc library
- **WITH_PROFILER** Build with CPU profiler
- **SHARED_LIB**    Build shared library, default is `STATIC`
- **BUILD_DOCS**    Build API documentation (requires Doxygen), default is `AUTO`
