#!/bin/sh
set -x verbose
sudo apt-get install build-essential xserver-xorg-dev x11proto-xf86vidmode-dev libxxf86vm-dev mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev libxext-dev libxcursor-dev

wget -nc http://downloads.sourceforge.net/irrlicht/irrlicht-1.8.zip
unzip -o irrlicht-1.8.zip 
cd irrlicht-1.8/source/Irrlicht/
make NDEBUG=1 sharedlib -j`nproc`
sudo make install