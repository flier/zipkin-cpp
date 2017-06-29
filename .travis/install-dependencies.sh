#/bin/sh -f
set -e

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    brew update
    brew install ccache curl double-conversion gflags glog google-benchmark gperftools rapidjson folly thrift librdkafka grpc doxygen tree jemalloc
else
    if [[ ! -f "cmake-$CMAKE_VERSION-Linux-x86_64/bin/cmake" ]]
    then
        wget --no-check-certificate https://cmake.org/files/v3.8/cmake-$CMAKE_VERSION-Linux-x86_64.tar.gz
        tar -xvf cmake-$CMAKE_VERSION-Linux-x86_64.tar.gz
    fi
fi
