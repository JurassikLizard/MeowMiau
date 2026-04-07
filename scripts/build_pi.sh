#!/bin/bash

docker build -t meowmiau-alpine-builder docker/

docker run --rm -v "$(pwd)":/app meowmiau-alpine-builder sh -c "
  cmake -S . -B build-pi \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-alpine.cmake \
    -DPLATFORM=DRM && \
  cmake --build build-pi
"