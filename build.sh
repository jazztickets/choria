#!/bin/sh
build_type="Release"

mkdir -p build/release
pushd build/release
cmake -DCMAKE_BUILD_TYPE="${build_type}" ../../
make -j`nproc`
popd
