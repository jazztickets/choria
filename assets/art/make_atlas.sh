#!/bin/bash

# check for imagemagick
type convert >/dev/null 2>&1 || {
	echo >&2 "imagemagick is not installed ";
	exit 1;
}

out="main.png"

mkdir -p out
rm -f "$out"

for f in tiles/*.png; do
	convert "$f" -compose copy -write mpr:tile +delete -size 66x66 -tile-offset -1-1 tile:mpr:tile "out/t_`basename \"$f\"`"
done

count=$(ls tiles/*.png | wc -l)
cols=$(echo $count | awk '{ x = sqrt($1); print x%1 ? int(x)+1 : x}')

montage -background transparent -geometry 66x66 -tile ${cols}x out/* "$out"
rm -rf out/

mogrify "$out" -depth 8
mv "$out" ../source/textures/atlas/
