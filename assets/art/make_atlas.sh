#!/bin/bash

# check for imagemagick
type convert >/dev/null 2>&1 || {
	echo >&2 "imagemagick is not installed ";
	exit 1;
}

#for f in tiles/*.png; do
#	convert "$f" -compose copy -write mpr:tile +delete -size 66x66 -tile-offset -1-1 tile:mpr:tile "out/t_`basename \"$f\"`"
#done

# count pngs
count=$(ls tiles/*.png | wc -l)

# determine number of columns for new texture
cols=$(echo $count | awk '{ x = sqrt($1); print x%1 ? int(x)+1 : x}')

# build atlas
montage -background transparent -geometry 66x66 -tile ${cols}x tiles/* atlas0.png

# change bit depth
mogrify atlas0.png -depth 8

# move to working directory
mv atlas0.png ../../working/textures/map/
