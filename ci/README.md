# Developer use of CI Docker images

Two flavors of `zipkin-cpp-build` Docker images, based on Ubuntu and CentOS Linux, are built.

## Ubuntu image: `zipkin-cpp-build:ubuntu`

The current build image is based on Ubuntu 16.04 (Xenial) which uses the GCC 5.4 compiler.

## CentOS image: `zipkin-cpp-build:centos`

The current build image is based on CentOS 7 with [devtoolset-4](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-4/) which uses the GCC 5.3 compiler.

## Building and running tests as a developer

An example basic invocation to build a developer version of the `zipkin-cpp` library.

```sh
$ cd ci && ./build-dist.sh git:develop
```

For a release version of the `zipkin-cpp` library you can run:

```sh
$ cd ci && ./build-dist.sh release:0.3.1
```

For a local version of the `zipkin-cpp` library you can run:

```sh
$ cd ci && ./build-dist.sh local
```

The build script will map the local source code to `/source` folder.

To build with the CentOS image, we need `IMAGE_TAG=centos` with previous commands.

```sh
$ cd ci && IMAGE_TAG=centos ./build-dist.sh git:develop
```

The built library can be found in `dist/zipkin-cpp-0.3.1-Linux-x86_64.tar.gz`.

## Testing changes to the build image as a developer

While all changes to the build image should eventually be upstreamed, it can be useful to test those changes locally before sending out a pull request. To experiment with a local clone of the upstream build image you can make changes to files such as `build_image.sh` locally and then run:

```sh
$ cd ci && ./build-image.sh  # Wait patiently for quite some time

Sending build context to Docker daemon  22.53kB
Step 1/25 : FROM ubuntu:xenial
 ...
Successfully built 9f80a02eb4ef
Successfully tagged zipkin-cpp-build:ubuntu
```

Besides, you could build images base on CentOS 7

```sh
$ cd ci && IMAGE_TAG=centos ./build-image.sh  # Wait patiently for quite some time
Sending build context to Docker daemon  22.53kB
Step 1/28 : FROM centos/devtoolset-4-toolchain-centos7
...
Successfully built 419fd83e43c8
Successfully tagged zipkin-cpp-build:centos
```

After the build was finished, it will be tagged with `IMAGE_TAG`.
