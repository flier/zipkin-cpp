#!/bin/bash

set -e

[[ -z "${ZIPKIN_VERSION}" ]] && ZIPKIN_VERSION="0.3.1"

BASE_DIR=`pwd`/..
DIST_DIR=${BASE_DIR}/dist

IMAGE_NAME=zipkin-cpp-build:ubuntu
CONTAINER_NAME=zipkin-cpp-build

# When running docker on a Mac, root user permissions are required.
if [[ "$OSTYPE" == "darwin"* ]]; then
    USER=root
    USER_GROUP=root
else
    USER=$(id -u)
    USER_GROUP=$(id -g)
fi

if [[ -z `docker ps -a -q -f "name=${CONTAINER_NAME}" -f "ancestor=${IMAGE_NAME}"` ]]; then
    docker run --name ${CONTAINER_NAME} -u "${USER}":"${USER_GROUP}" -v ${DIST_DIR}:/zipkin-cpp/dist ${IMAGE_NAME} $1
else
    CONTAINER_ID=`docker ps -q -f "name=${CONTAINER_NAME})" -f "ancestor=${IMAGE_NAME}"`

    if [[ -z ${CONTAINER_ID} ]]; then
        docker start ${CONTAINER_ID}
    fi

    docker logs -f ${CONTAINER_ID}
fi

PKG_TYPE=$2

if [[ -d ${DIST_DIR} ]]; then
    cd ${DIST_DIR}

    if [[ `gem list -i fpm` != 'true' ]]; then
        gem install fpm -f --no-document
    fi

    fpm -f --prefix /usr/local -s dir -t $PKG_TYPE -n zipkin-cpp -v ${ZIPKIN_VERSION} include lib

    if [ "$PKG_TYPE" == 'osxpkg' ]; then
        pkgutil --payload-files zipkin-cpp-${ZIPKIN_VERSION}.pkg
    fi
fi
