#!/bin/bash

upload_server=$1

projectdir=`git rev-parse --show-toplevel`
if [ -z "$projectdir" ]; then
	echo "No .git directory found"
	exit 1
fi

outputdir="$projectdir/deployment/out"
cd "$projectdir"
mkdir -p "$outputdir"

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
	builddir="$projectdir/build/mingw$bits"
	mkdir -p "$builddir"
	cd "$builddir"
	cmake -DCMAKE_TOOLCHAIN_FILE=../../cmake/mingw${bits}.cmake ../../

	# build
	make -j`nproc`

	if [ $? -ne 0 ]; then
		echo "failed $builddir"
		exit
	fi

	cd "$projectdir"

	cp /usr/$arch/bin/{libbz2-1.dll,libfreetype-6.dll,libgcc_*.dll,libsqlite3-0.dll,libstdc++-6.dll,libwinpthread-1.dll,lua53.dll,libvorbisfile-3.dll,libvorbis-0.dll,libogg-0.dll,SDL2.dll,zlib1.dll,libjsoncpp.dll,libtinyxml2.dll} working/

	gitver=`git log --oneline | wc -l`
	mv bin/Release/choria.exe working/
	cp README working/
	echo "choria.exe -server" > working/run_server.bat
	echo "choria.exe -server -hardcore" > working/run_hardcore_server.bat
	echo "choria.exe -hardcore" > working/run_hardcore.bat
	echo "choria.exe -test" > working/run_test.bat
	chmod +x working/*.bat

	archive=choria-${version}r${gitver}-win${bits}.zip
	zip -r $archive working

	rm working/choria.exe
	rm working/*.dll
	rm working/README
	rm working/*.bat

	if [ -n "$upload_server" ]; then
		scp $archive $upload_server:web/files/
	fi

	mv $archive "$outputdir"
}

if [ -n "$upload_server" ]; then
	ssh $upload_server rm -f web/files/choria*.zip
fi

rm -f "$outputdir"/choria*.zip

build 32
build 64
