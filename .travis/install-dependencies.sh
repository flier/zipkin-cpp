#/bin/sh -f
set -e

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    brew update
    brew install thrift librdkafka
else
    curl -sSL "http://apache.mirrors.spacedump.net/thrift/$THRIFT_VERSION/thrift-$THRIFT_VERSION.tar.gz" -o thrift.tar.gz
    mkdir -p thrift-$THRIFT_VERSION
    tar zxf thrift.tar.gz -C thrift-$THRIFT_VERSION --strip-components=1
    rm thrift.tar.gz
    cd thrift-$THRIFT_VERSION
    ./configure --without-python --without-java --without-ruby --without-php --without-erlang --without-go --without-nodejs --without-qt4
    make
    sudo make install
    cd ..
    rm -rf thrift-$THRIFT_VERSION

    curl -sSL "https://github.com/edenhill/librdkafka/archive/v$RDKAFKA_VERSION.tar.gz" -o librdkafka.tar.gz
    mkdir -p librdkafka-$RDKAFKA_VERSION
    tar zxf librdkafka.tar.gz -C librdkafka-$RDKAFKA_VERSION --strip-components=1
    rm librdkafka.tar.gz
    cd librdkafka-$RDKAFKA_VERSION
    ./configure
    make
    sudo make install
    cd ..
    rm -rf librdkafka-$RDKAFKA_VERSION
fi