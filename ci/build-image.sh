#!/bin/bash

if [[ "$IMAGE_TAG" == "centos" ]]; then
    docker build -t zipkin-cpp-build:centos -f Dockerfile-centos-7 .
else
    docker build -t zipkin-cpp-build:ubuntu -f Dockerfile-ubuntu-xenial .
fi
