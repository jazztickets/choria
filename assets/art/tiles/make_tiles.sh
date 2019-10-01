#!/bin/bash

# check for imagemagick
type convert >/dev/null 2>&1 || {
	echo >&2 "imagemagick is not installed ";
	exit 1;
}

in=$1

if [ -z "$in" ]; then
	echo "Usage: ./`basename $0` directory"
	exit
fi

mkdir -p out
rm -f out/*.png

for f in $in/*.png; do
	basename=`basename "$f"`
	tempname="out/t_`basename \"$f\"`"

	convert "$f" -compose copy -write mpr:tile +delete -size 66x66 -tile-offset -1-1 tile:mpr:tile PNG32:"$tempname"
	#pngcrush -brute "$tempname" out/"$basename"
	mv "$tempname" "out/$basename"
done

rm -f out/t_*.png
