#!/bin/sh

# get type
build_type="Release"
if [ -n "$1" ]; then
	build_type="$1"
fi

# get build dir
build_dir=$(echo $build_type | tr '[:upper:]' '[:lower:]')

# make
mkdir -p "build/$build_dir"
pushd "build/$build_dir"
cmake -DCMAKE_BUILD_TYPE="${build_type}" ../../
make -j`nproc`
popd
