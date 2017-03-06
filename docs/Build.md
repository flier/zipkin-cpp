# Build from Source

## Dependencies

### Mac OSX

You could install dependencies with [Homebrew](https://brew.sh/).

```sh
$ brew install boost curl double-conversion folly thrift librdkafka conan
```

### Linux

`zipkin-cpp` requires gcc 4.8+ and a version of boost compiled with C++11 support.

#### Ubuntu

```sh
$ sudo apt-get install libboost-dev libcurl4-openssl-dev libdouble-conversion-dev thrift-compiler librdkafka-dev
```

#### CentOS

```sh
$ sudo yum install boost-devel libcurl-devel double-conversion-devel thrift-devel
```

### Install from Source

If the OS missed some packages, you could build and install from source.

#### folly

Follow the [build notes](https://github.com/facebook/folly#build-notes) to install the [folly](https://github.com/facebook/folly) library.

#### librdkafka

Follow the [instructions](https://github.com/edenhill/librdkafka/#instructions) to install the [librdkafka](https://github.com/edenhill/librdkafka) library;

#### thrift

Follow the [build guide](https://thrift.apache.org/docs/BuildingFromSource) to install the [thrift](https://thrift.apache.org/).

#### conan

Follow the [installation guide](http://docs.conan.io/en/latest/installation.html#) to install `conan` with [PIP](http://docs.conan.io/en/latest/installation.html#install-with-pip).

## Get source files

Download the latest release from the [releases page](https://github.com/flier/zipkin-cpp/releases) or clone from the [git repo](https://github.com/flier/zipkin-cpp.git).

```sh
$ git clone https://github.com/flier/zipkin-cpp.git
```

Build `zipkin-cpp` with CMake

```sh
$ mkdir build
$ cd build
$ conan install .. --build=missing
$ cmake ..
```

### CMake options
- [CMAKE_BUILD_TYPE](https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html) default is `RELWITHDEBINFO`
- [CMAKE_INSTALL_PREFIX](https://cmake.org/cmake/help/v3.0/variable/CMAKE_INSTALL_PREFIX.html) default is `/usr/local`

### Build options

- **WITH_FPIC**     Build with -fPIC for shared library, default is `OFF`
- **SHARED_LIB**    Build shared library, default is `STATIC`
- **BUILD_DOCS**    Build API documentation (requires Doxygen), default is `AUTO`
