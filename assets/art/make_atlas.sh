#!/bin/bash

# check for imagemagick
type convert >/dev/null 2>&1 || {
	echo >&2 "imagemagick is not installed ";
	exit 1;
}

# get parameters
path=$1

# get file
if [ -z "$path" ]; then
	echo "No path specified"
	exit
fi

outfile=$(basename "$path")

#for f in tiles/*.png; do
#	convert "$f" -compose copy -write mpr:tile +delete -size 66x66 -tile-offset -1-1 tile:mpr:tile "out/t_`basename \"$f\"`"
#done

# count pngs
count=$(ls "$path"/*.png | wc -l)

# determine number of columns for new texture
cols=$(echo $count | awk '{ x = sqrt($1); print x%1 ? int(x)+1 : x}')

# build atlas
montage -background transparent -geometry 66x66 -tile ${cols}x "$path"/* "$outfile".png

# change bit depth
mogrify "$outfile".png -depth 8

# move to working directory
mv "$outfile".png ../../working/textures/map/
