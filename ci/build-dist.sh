#!/bin/bash

set -e

[[ -z "${ZIPKIN_VERSION}" ]] && ZIPKIN_VERSION="0.3.1"

BASE_DIR=`pwd`/..
DIST_DIR=${BASE_DIR}/dist

IMAGE_NAME=zipkin-cpp-build:ubuntu
CONTAINER_NAME=zipkin-cpp-build

function container_has_outdated {
    local image_name=$1
    local container_name=$2
    local image_id=`docker ps -a -q -f "name=${container_name}" --format="{{.Image}}"`

    [[ $image_id != "" && $image_id != $image_name ]]
}

function container_is_exists {
    local image_name=$1
    local container_name=$2
    local container_id=`docker ps -a -q -f "name=${container_name}" -f "ancestor=${image_name}"`

    [[ -n $container_id ]]
}

function container_has_stopped {
    local image_name=$1
    local container_name=$2
    local container_id=`docker ps -q -f "name=${container_name}" -f "ancestor=${image_name}"`

    [[ -z running_container_id ]]
}

# When running docker on a Mac, root user permissions are required.
if [[ "$OSTYPE" == "darwin"* ]]; then
    USER=root
    USER_GROUP=root
else
    USER=$(id -u)
    USER_GROUP=$(id -g)
fi

if container_has_outdated ${IMAGE_NAME} ${CONTAINER_NAME}; then
    echo "remove outdated container ${OLD_CONTAINER_ID}"

    docker rm ${CONTAINER_NAME}
fi

if container_is_exists ${IMAGE_NAME} ${CONTAINER_NAME}; then
    if container_has_stopped ${IMAGE_NAME} ${CONTAINER_NAME}; then
        echo "start the container"

        docker start ${CONTAINER_NAME}
    fi

    echo "attach to the container"

    docker logs -f ${CONTAINER_NAME}
else
    echo "run new container"

    docker run --name ${CONTAINER_NAME} -u "${USER}":"${USER_GROUP}" -v ${DIST_DIR}:/zipkin-cpp/dist ${IMAGE_NAME} $1
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
