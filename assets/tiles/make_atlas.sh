#!/bin/bash

mkdir -p out
rm -f atlas.png

for f in textures/*.png; do
	convert "$f" -compose copy -write mpr:tile +delete -size 34x34 -tile-offset -1-1 tile:mpr:tile "out/t_`basename \"$f\"`"
done

count=$(ls textures/*.png | wc -l)
cols=$(echo $count | awk '{ x = sqrt($1); print x%1 ? int(x)+1 : x}')

montage -background transparent -geometry 34x34 -tile ${cols}x out/* atlas0.png

rm -rf out/