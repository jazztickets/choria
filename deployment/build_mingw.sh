#!/bin/bash

# get parameters
project=choria
upload_server=$1
upload_path="/srv/http/files/"

# get base project dir
projectdir=`git rev-parse --show-toplevel`
if [ -z "$projectdir" ]; then
	echo "No .git directory found"
	exit 1
fi

# build out dir
outputdir="$projectdir/deployment/out"
mkdir -p "$outputdir"

# get versions
version=`grep 'GAME_VERSION=".*"' -o "$projectdir"/CMakeLists.txt | sed -r "s/GAME_VERSION=\"(.*)\"/\1/"`
gitver=`git rev-parse --short HEAD`

build() {

	bits=$1
	build_4k=$2

	# get mingw prefix
	if [ $bits -eq "32" ]; then
		arch=i686-w64-mingw32
	else
		arch=x86_64-w64-mingw32
	fi

	# run cmake
	builddir="$projectdir/build/mingw$bits"
	mkdir -p "$builddir"
	pushd "$builddir"
	cmake -DDISABLE_EDITOR=${DISABLE_EDITOR} -DCMAKE_TOOLCHAIN_FILE=../../cmake/mingw${bits}.cmake ../../

	# build
	make -j`nproc`

	# check for build errors
	if [ $? -ne 0 ]; then
		echo "failed $builddir"
		exit
	fi

	# go back to deployment dir
	popd

	# create new working dir
	archive_base=${project}-${version}-${gitver}-win${bits}
	archive=${archive_base}.zip
	if [ "${build_4k}" -eq 1 ]; then
		archive=${archive_base}_4k.zip
	fi
	rm -rf "${archive_base}"
	cp -r "${projectdir}/working" "${archive_base}"
	rm "${projectdir}/working/${project}.exe"

	# remove linux only files
	rm -f "${archive_base}"/"${project}"{,_debug}

	# copy dlls
	cp /usr/$arch/bin/{OpenAL32.dll,libbz2-1.dll,libfreetype-6.dll,libssp-0.dll,libgcc_*.dll,libsqlite3-0.dll,libstdc++-6.dll,libwinpthread-1.dll,lua53.dll,libvorbisfile-3.dll,libvorbis-0.dll,libogg-0.dll,SDL2.dll,SDL2_image.dll,libpng16-16.dll,zlib1.dll,libjsoncpp.dll,libtinyxml2.dll} "${archive_base}"/

	# strip exe
	${arch}-strip "${archive_base}"/${project}.exe

	# copy files
	cp "${projectdir}"/{README,CHANGELOG} "${archive_base}"/
	echo "${project}.exe -server" > "${archive_base}"/run_server.bat
	echo "${project}.exe -server -hardcore" > "${archive_base}"/run_hardcore_server.bat
	echo "${project}.exe -hardcore" > "${archive_base}"/run_hardcore.bat
	echo "${project}.exe -test" > "${archive_base}"/run_test.bat
	echo "${project}.exe -connect host port -username myusername -password mypassword" > "${archive_base}"/run_connect.bat
	echo -e "${project}.exe -benchmark\npause" > "${archive_base}"/run_benchmark.bat
	chmod +x "${archive_base}"/*.bat

	if [ "${build_4k}" -eq 1 ]; then
		cp -r "$projectdir/assets/art/4k/textures" "${archive_base}/"
	fi

	# zip
	zip -r "${archive}" "${archive_base}"

	# upload zip to server
	if [ -n "$upload_server" ]; then
		scp $archive $upload_server:"$upload_path"
	fi

	# clean up
	rm -rf "${archive_base}"
	mv $archive "$outputdir"
}

# remove old versions from server
if [ -n "$upload_server" ]; then
	ssh $upload_server rm -f "$upload_path"/"${project}-${version}"*.zip
fi

# remove old zips
rm -f "$outputdir"/"${project}-${version}"*.zip

# build project
build 64 0
build 64 1
