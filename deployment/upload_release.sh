#!/bin/bash

token=$1

# get version info
version=`grep 'GAME_VERSION=".*"' -o ../CMakeLists.txt | sed -r "s/GAME_VERSION=\"(.*)\"/\1/"`
gitver=`git rev-list --all --count`
build="${version}r${gitver}"
changelog=$(awk '$0 == "" { exit } { print }' ../CHANGELOG)

echo "${build}"
echo "${changelog}"

echo
echo "Hit enter to continue uploading"
read
#curl -H "Authorization: token ${token}" "https://api.github.com/repos/jazztickets/choria/releases/" --data "{\"tag_name\":\"$version\",\"name\":\"$version\",\"body\":\"$changelog\"}"
