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
fi