#!/bin/bash

upload_server=$1

mkdir -p release
cd ../

version=`grep 'GAME_VERSION=".*"' -o CMakeLists.txt | sed -r "s/GAME_VERSION=\"(.*)\"/\1/"`

build() {

	bits=$1

	# get mingw prefix
	if [ $bits -eq "32" ]; then
		arch=i686-w64-mingw32
	else
		arch=x86_64-w64-mingw32
	fi

	# run cmake
	builddir=build-mingw$bits
	if [ ! -d "$builddir" ]; then
		mkdir "$builddir"
		cd $builddir
		cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw${bits}.cmake ../
	else
		cd $builddir
	fi

	make -j`nproc`

	if [ $? -ne 0 ]; then
		echo "failed $builddir"
		exit
	fi

	cd ..

	cp /usr/$arch/bin/{libbz2-1.dll,libfreetype-6.dll,libgcc_*.dll,libsqlite3-0.dll,libstdc++-6.dll,libwinpthread-1.dll,lua53.dll,libvorbisfile-3.dll,libvorbis-0.dll,libogg-0.dll,SDL2.dll,zlib1.dll,libjsoncpp.dll} working/

	gitver=`git rev-list --all --count`
	mv bin/Release/choria.exe working/
	cp README working/
	echo "choria.exe -server" > working/server.bat
	chmod +x working/server.bat

	archive=choria-${version}r${gitver}-win${bits}.zip
	zip -r $archive working

	rm working/choria.exe
	rm working/*.dll
	rm working/README
	rm working/server.bat

	if [ -n "$upload_server" ]; then
		scp $archive $upload_server:web/files/
	fi

	mv $archive deployment/release
}

if [ -n "$upload_server" ]; then
	ssh $upload_server rm web/files/choria*.zip
fi

rm -f deployment/release/choria*.zip

build 32
build 64
