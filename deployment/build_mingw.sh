#!/bin/bash

mkdir -p release
cd ../

version=`grep 'GAME_VERSION=".*"' -o CMakeLists.txt | sed -r "s/GAME_VERSION=\"(.*)\"/\1/"`

build() {

	bits=$1

	if [ $bits -eq "32" ]; then
		arch=i686-w64-mingw32
	else
		arch=x86_64-w64-mingw32
	fi

	builddir=build-mingw$bits
	cd $builddir

	make -j4

	if [ $? -ne 0 ]; then
		echo "failed $builddir"
		exit
	fi

	cd ..

	cp /usr/$arch/bin/{libbz2-1.dll,libfreetype-6.dll,libgcc_*.dll,libsqlite3-0.dll,libstdc++-6.dll,libwinpthread-1.dll,lua53.dll,libvorbisfile-3.dll,libvorbis-0.dll,libogg-0.dll,SDL2.dll,zlib1.dll} working/

	gitver=`git rev-list --all --count`
	mv bin/Release/choria.exe working/

	archive=choria${bits}-${version}r${gitver}.zip
	zip -r $archive working

	rm working/choria.exe
	rm working/*.dll

	scp $archive workcomp:web/files/
	mv $archive deployment/release
}

ssh workcomp rm web/files/choria*.zip
rm -f deployment/release/choria*.zip

build 32
build 64
