#/bin/sh -f
set -e

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    brew update
    brew install curl double-conversion gflags glog google-benchmark gperftools rapidjson folly thrift librdkafka grpc doxygen
fi