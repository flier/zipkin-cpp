#/bin/sh -f
set -e

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    brew update
    brew install ccache curl double-conversion gflags glog google-benchmark gperftools rapidjson folly thrift librdkafka grpc doxygen
else
    wget --no-check-certificate https://cmake.org/files/v3.7/cmake-$CMAKE_VERSION-Linux-x86_64.tar.gz
    tar -xvf cmake-$CMAKE_VERSION-Linux-x86_64.tar.gz
fi