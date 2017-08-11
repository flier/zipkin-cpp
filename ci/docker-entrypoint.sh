#!/bin/bash

set -e

if [ "$1" == 'release' ]; then
    echo "zip-cpp release build with v${ZIPKIN_VERSION} package."

    update-ca-certificates
    wget https://github.com/flier/zipkin-cpp/archive/v${ZIPKIN_VERSION}.tar.gz
    mkdir -p ${SRC_DIR}
    tar xzvf v${ZIPKIN_VERSION}.tar.gz -C ${SRC_DIR} --strip-components=1
    rm v${ZIPKIN_VERSION}.tar.gz
elif [ "$1" == 'head' ]; then
    echo "zip-cpp development build with GIT source."

    git clone https://github.com/flier/zipkin-cpp.git ${SRC_DIR}
elif [ "$1" == 'latest' ]; then
    echo "zip-cpp development build with with local volume mapping to ${SRC_DIR}."
else
    exec "$@"
fi

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
${CMAKE_DIR}/bin/cmake \
    -DCMAKE_INSTALL_PREFIX=${DIST_DIR} \
    -DCMAKE_PREFIX_PATH=${EXT_DIR} \
    ${CMAKE_OPTS} \
    ${SRC_DIR}

make
make test
make install
make clean

tree -h ${DIST_DIR}
tar czvf ${DIST_DIR}/zipkin-cpp-$ZIPKIN_VERSION-Linux-x86_64.tar.gz ${DIST_DIR}/
