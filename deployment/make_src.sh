#!/bin/bash

project=choria
version=`grep 'GAME_VERSION=".*"' -o ../CMakeLists.txt | sed -r "s/GAME_VERSION=\"(.*)\"/\1/"`
gitver=`git rev-parse --short HEAD`
base=${project}-${version}-${gitver}
pkg=${base}-src.tar.gz

mkdir -p out

tar --transform "s,^,${base}/," -czvf out/${pkg} -C ../ \
--exclude=${pkg} \
--exclude=*.swp \
--exclude=.git \
--exclude=working/${project}* \
--exclude=deployment/out \
--exclude=deployment/*.sh \
ext/ \
src/ \
working/ \
deployment/{choria,choria.desktop,choria.png} \
cmake/ \
CMakeLists.txt \
build.sh \
README \
CHANGELOG \
LICENSE

echo -e "\nMade ${pkg}"
