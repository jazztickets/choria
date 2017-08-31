#!/bin/bash
mkdir -p out

version=`grep 'GAME_VERSION=".*"' -o ../CMakeLists.txt | sed -r "s/GAME_VERSION=\"(.*)\"/\1/"`
gitver=`git log --oneline | wc -l`
base=choria-${version}r${gitver}
pkg=${base}-src.tar.gz

tar --transform "s,^,${base}/," -czvf ${pkg} -C ../ src/ working/ deployment/{choria,choria.desktop,choria.png} cmake/ CMakeLists.txt README CHANGELOG LICENSE --exclude=*.dll --exclude=*.exe --exclude=*.swp --exclude=*.nsi

echo -e "\nMade ${pkg}"

mv "${pkg}" out/
