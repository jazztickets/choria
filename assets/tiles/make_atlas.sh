#!/bin/bash

mkdir -p out
rm -f atlas.png

for f in textures/*.png; do
	convert "$f" -compose copy -write mpr:tile +delete -size 66x66 -tile-offset -1-1 tile:mpr:tile "out/t_`basename \"$f\"`"
done

count=$(ls textures/*.png | wc -l)
cols=$(echo $count | awk '{ x = sqrt($1); print x%1 ? int(x)+1 : x}')

montage -background transparent -geometry 66x66 -tile ${cols}x out/* atlas0.png

rm -rf out/

mogrify atlas0.png -depth 8

mv atlas0.png ../../working/textures/map/
