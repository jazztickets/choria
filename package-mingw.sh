#!/bin/bash
git pull
cd build-mingw

make -j4

if [ $? -ne 0 ]; then
	echo "failed"
	exit
fi

cd ..

#ver=`git log --pretty=format:'%h' -n 1`
ver=`git rev-list --all --count`
cp bin/Release/choria.exe working/
rm -f choria*.7z
rm -f choria*.zip

archive=choria-${ver}.zip
#7za a $archive working
zip -r $archive working

rm working/choria.exe

scp $archive workcomp:downloads/
