#!/bin/bash

set -e

if [[ "$1" =~ ^release: ]]; then
    if [ "${1:8}" != "" ]; then
        ZIPKIN_VERSION=${1:8}
    fi

    echo "zipkin-cpp release build with v${ZIPKIN_VERSION} package."

    update-ca-certificates
    wget https://github.com/flier/zipkin-cpp/archive/v${ZIPKIN_VERSION}.tar.gz
    mkdir -p ${SRC_DIR}
    tar xzvf v${ZIPKIN_VERSION}.tar.gz -C ${SRC_DIR} --strip-components=1
    rm v${ZIPKIN_VERSION}.tar.gz
elif [[ "$1" =~ ^git: ]]; then
    echo "zipkin-cpp development build with GIT source."

    git clone https://github.com/flier/zipkin-cpp.git ${SRC_DIR}

    GIT_BRANCH=${1:4-master}

    echo "checkout GIT branch: ${GIT_BRANCH}"

    cd ${SRC_DIR} && git checkout ${GIT_BRANCH}

    GIT_REVISION=`git describe --always`
    ZIPKIN_VERSION="git-${GIT_REVISION}"
elif [ "$1" == 'local' ]; then
    echo "zipkin-cpp development build with with local volume mapping to ${SRC_DIR}."

    cd ${SRC_DIR}

    GIT_REVISION=`git describe --always`
    ZIPKIN_VERSION="git-${GIT_REVISION}"
else
    exec "$@"
fi

echo "build zipkin-cpp version ${ZIPKIN_VERSION} @ ${SRC_DIR}"
echo "install dist files to @ ${DIST_DIR}"
echo "use prebuilt projects @ ${EXT_DIR}"

#tree ${EXT_DIR}

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
${CMAKE_DIR}/bin/cmake \
    -DCMAKE_INSTALL_PREFIX=${DIST_DIR} \
    -DCMAKE_PREFIX_PATH=${EXT_DIR} \
    -DCMAKE_PROGRAM_PATH=${EXT_DIR}/bin \
    -DCMAKE_INCLUDE_PATH=${EXT_DIR}/include \
    -DCMAKE_LIBRARY_PATH=${EXT_DIR}/lib \
    -DBUILD_DOCS=OFF \
    -DBUILD_BENCH=OFF \
    ${CMAKE_OPTS} \
    ${SRC_DIR}

make
make test
make install
make clean

PACKAGE_FILE="zipkin-cpp-$ZIPKIN_VERSION-Linux-x86_64.tar.gz"

tar czvf ${DIST_DIR}/${PACKAGE_FILE} --exclude *.tar.gz ${DIST_DIR}/

echo "dist files packaged to: dist/${PACKAGE_FILE}"
