FROM ubuntu:xenial

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    binutils-dev \
    bison \
    build-essential \
    ca-certificates \
    doxygen \
    flex \
    git \
    libboost-all-dev \
    libc++-dev \
    libcurl4-openssl-dev \
    libdouble-conversion-dev \
    libdwarf-dev \
    libelf-dev \
    libevent-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    libiberty-dev \
    libjemalloc-dev \
    liblz4-dev \
    liblzma-dev \
    libsnappy-dev \
    libssl-dev \
    libtool \
    libunwind8-dev \
    make \
    pkg-config \
    thrift-compiler \
    tree \
    wget \
    zlib1g-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV CMAKE_DIR   /cmake
ENV BASE_DIR    /zipkin-cpp
ENV SRC_DIR     $BASE_DIR
ENV BUILD_DIR   $BASE_DIR/build
ENV DIST_DIR    $BASE_DIR/dist
ENV EXT_DIR     $BUILD_DIR/externals

ENV CMAKE_VERSION   3.8.2
RUN wget https://cmake.org/files/v3.8/cmake-$CMAKE_VERSION-Linux-x86_64.tar.gz && \
    mkdir -p $CMAKE_DIR && \
    tar xzvf cmake-$CMAKE_VERSION-Linux-x86_64.tar.gz -C $CMAKE_DIR --strip-components=1 && \
    rm cmake-$CMAKE_VERSION-Linux-x86_64.tar.gz

ENV FOLLY_VERSION   2017.06.26.01
RUN wget https://github.com/facebook/folly/archive/v$FOLLY_VERSION.tar.gz -O folly-$FOLLY_VERSION.tar.gz && \
    mkdir -p $EXT_DIR/src/Folly && \
    tar xzvf folly-$FOLLY_VERSION.tar.gz -C $EXT_DIR/src/Folly --strip-components=1 && \
    rm folly-$FOLLY_VERSION.tar.gz && \
    cd $EXT_DIR/src/Folly/folly && \
    autoreconf -vi && \
    ./configure \
        --prefix=$EXT_DIR \
        --with-pic \
        --with-jemalloc \
        LD_LIBRARY_PATH=$EXT_DIR/lib \
        LD_RUN_PATH=$EXT_DIR/lib \
        LIBRARY_PATH=$EXT_DIR/lib \
        LDFLAGS=-L$EXT_DIR/lib \
        PKG_CONFIG_PATH=$EXT_DIR/lib/pkgconfig \
        CFLAGS=-I$EXT_DIR/include \
        CXXFLAGS=-I$EXT_DIR/include \
        CPPFLAGS=-I$EXT_DIR/include && \
    make && \
    make install && \
    make clean && \
    rm -rf $EXT_DIR/src/Folly/folly

ENV THRIFT_VERSION_STRING   0.10.0
RUN wget http://www-us.apache.org/dist/thrift/$THRIFT_VERSION_STRING/thrift-$THRIFT_VERSION_STRING.tar.gz -O thrift-$$THRIFT_VERSION_STRING.tar.gz && \
    mkdir -p $EXT_DIR/src/Thrift && \
    tar xzvf thrift-$$THRIFT_VERSION_STRING.tar.gz -C $EXT_DIR/src/Thrift --strip-components=1 && \
    rm thrift-$$THRIFT_VERSION_STRING.tar.gz && \
    cd $EXT_DIR/src/Thrift && \
    ./configure \
        --prefix=$EXT_DIR \
        --without-c_glib \
        --without-csharp \
        --without-python \
        --without-java \
        --without-nodejs \
        --without-lua \
        --without-ruby \
        --without-php \
        --without-erlang \
        --without-go \
        --without-nodejs \
        --without-qt4 \
        --without-qt5 \
        --disable-plugin && \
    make && \
    make install && \
    make clean && \
    rm -rf $EXT_DIR/src/Thrift

ENV ZIPKIN_VERSION  0.3.1

COPY ./docker-entrypoint.sh  /

ENTRYPOINT ["/docker-entrypoint.sh"]

CMD ["release"]
