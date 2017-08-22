# Developer use of CI Docker images

Two flavors of Envoy Docker images, based on Ubuntu and CentOS Linux, are built.

## Ubuntu `zipkin-cpp-build:ubuntu` image

## CentOS `zipkin-cpp-build:centos` image

## Build image base and compiler versions

The current build image is based on Ubuntu 16.04 (Xenial) which uses the GCC 5.4 compiler.

## Building and running tests as a developer

## Testing changes to the build image as a developer

While all changes to the build image should eventually be upstreamed, it can be useful to test those changes locally before sending out a pull request. To experiment with a local clone of the upstream build image you can make changes to files such as build_image.sh locally and then run:

```sh
$ cd ci
$ ./build-image.sh  # Wait patiently for quite some time
```

Besides, you could build images base on CentOS 7

```sh
$ cd ci
$ ./build-image.sh centos  # Wait patiently for quite some time
```
