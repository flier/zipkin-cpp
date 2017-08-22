#!/bin/bash

set -e

[[ -z "${ZIPKIN_VERSION}" ]] && ZIPKIN_VERSION="0.3.1"

BASE_DIR=`pwd`/..
SRC_DIR=${BASE_DIR}
DIST_DIR=${BASE_DIR}/dist

IMAGE_TAG=${IMAGE_TAG:-ubuntu}
IMAGE_NAME=zipkin-cpp-build:${IMAGE_TAG}
CONTAINER_NAME=zipkin-cpp-build-${IMAGE_TAG}

# When running docker on a Mac, root user permissions are required.
if [[ "$OSTYPE" == "darwin"* ]]; then
    USER=root
    USER_GROUP=root
else
    USER=$(id -u)
    USER_GROUP=$(id -g)
fi

function container_is_exists {
    echo "docker ps -a -q -f \"name=${CONTAINER_NAME}\" -f \"ancestor=${IMAGE_NAME}\""
    local CONTAINER_ID=`docker ps -a -q -f "name=${CONTAINER_NAME}" -f "ancestor=${IMAGE_NAME}"`

    [[ -n $CONTAINER_ID ]]
}

function container_is_running {
    local CONTAINER_ID=`docker ps -q -f "name=${CONTAINER_NAME}" -f "ancestor=${IMAGE_NAME}"`

    [[ -n $CONTAINER_ID ]]
}

function container_has_outdated {
    local CONTAINER_ID=`docker ps -a -q -f "name=${CONTAINER_NAME}"`

    [[ -n $CONTAINER_ID ]]
}

function run_container {
    echo "run new container: ${CONTAINER_NAME}"

    if [[ $1 == 'local' ]]; then
        docker run --name ${CONTAINER_NAME} -u "${USER}":"${USER_GROUP}" -v ${SRC_DIR}:/source -v ${DIST_DIR}:/dist ${IMAGE_NAME} $1
    else
        docker run --name ${CONTAINER_NAME} -u "${USER}":"${USER_GROUP}" -v ${DIST_DIR}:/dist ${IMAGE_NAME} $1
    fi
}

if container_is_exists; then
    if container_is_running; then
        docker logs -f ${CONTAINER_NAME}
    else
        echo "remove stopped container: ${CONTAINER_NAME}"

        docker rm ${CONTAINER_NAME}

        run_container $@
    fi
else
    if container_has_outdated; then
        echo "remove outdated container ${CONTAINER_NAME}"

        docker rm ${CONTAINER_NAME}
    fi

    run_container $@
fi

if [[ -n $2 ]]; then
    PKG_TYPE=$2

    cd ${DIST_DIR}

    if [[ `gem list -i fpm` != 'true' ]]; then
        gem install fpm -f --no-document
    fi

    fpm -f --prefix /usr/local -s dir -t $PKG_TYPE -n zipkin-cpp -v ${ZIPKIN_VERSION} include lib

    if [ "$PKG_TYPE" == 'osxpkg' ]; then
        pkgutil --payload-files zipkin-cpp-${ZIPKIN_VERSION}.pkg
    fi
fi
