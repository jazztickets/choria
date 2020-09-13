#!/bin/bash

# check for imagemagick
type convert >/dev/null 2>&1 || {
	echo >&2 "imagemagick is not installed ";
	exit 1;
}

# check for inkscape
type inkscape >/dev/null 2>&1 || {
	echo >&2 "inkscape is not installed ";
	exit 1;
}

# check for pngcrush
type pngcrush >/dev/null 2>&1 || {
	echo >&2 "pngcrush is not installed ";
	exit 1;
}

# check for xmllint
type xmllint >/dev/null 2>&1 || {
	echo >&2 "xmllint is not installed ";
	exit 1;
}

# check for parallel
type parallel >/dev/null 2>&1 || {
	echo >&2 "parallel is not installed ";
	exit 1;
}

# function to optimize and rename png
worker_img() {
	mv "$1" "$2"
	pngcrush -s -brute "$2" "$3"
}

export -f worker_img

# get parameters
file=$1
scale=$2

# get file
if [ -z "$file" ]; then
	echo "No input .svg file"
	exit
fi

# get scale
if [ -z "$scale" ]; then
	scale=1
fi
echo "$file"
echo "calculating..."

# calculate density
density=$((96 * $scale))

# get size of each image
width=`identify -density 96 -format "%[fx:w/10]" "$file"`
height=`identify -density 96 -format "%[fx:h/10]" "$file"`
width=$(($width * $scale))
height=$(($height * $scale))

# get list of objects
names=`xmllint --xpath "//*[local-name()='svg']/*[local-name()='metadata']//*[local-name()='description']/text()" "$file"`

# create export directory
mkdir -p export
rm -f export/*.png

# export pngs
echo "exporting..."
temp_export="export/export.png"
inkscape "${file}" -d "${density}" -y 0 -o "${temp_export}" 2>/dev/null
convert "${temp_export}" -crop ${width}x${height} -depth 8 +repage PNG32:export/_out.png
rm "${temp_export}"

# name files and optimize in parallel
echo "compressing..."
i=0
for name in $names; do
	oldname="export/_out-${i}.png"
	newname="export/_${name}.png"
	optname="export/${name}.png"

	echo worker_img \"$oldname\" \"$newname\" \"$optname\"
	((i++))
done | parallel

# clean up
rm -f export/_*
rm -f export/none.png
