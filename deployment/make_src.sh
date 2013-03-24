#!/bin/sh
mkdir package
cp -r ../src package
cp ../README package
cp ../FindIrrlicht.cmake package
cp ../CMakeLists.txt package
mkdir package/deployment
cp choria choria.desktop choria.xpm license.txt changelog.txt package/deployment
cp -r ../working/ package

rm -rf choria-$1-src
mv package choria-$1-src
tar -czf choria-$1-src.tar.gz choria-$1-src
