#!/bin/bash

# check for imagemagick
type convert >/dev/null 2>&1 || {
	echo >&2 "imagemagick is not installed ";
	exit 1;
}

in=$1

mkdir -p out

for f in $in/*.png; do
	convert "$f" -compose copy -write mpr:tile +delete -size 66x66 -tile-offset -1-1 tile:mpr:tile "out/`basename \"$f\"`"
done
