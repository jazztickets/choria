#!/bin/bash

file=$1
size=$2
if [ -z "$file" ]; then
	echo "No input .svg file"
	exit
fi

if [ -z "$size" ]; then
	size=64
fi
density=$((45 * $size/32))

# get list of objects
names=`grep id=\"layer $file -C 3 | grep label | grep =.* -o | tr -d '="<>' | tac`

# export pngs
convert -density ${density} -background none ${file} -crop ${size}x${size} -depth 8 +repage PNG32:export/_out.png

# name files
i=0
for name in $names; do
	oldname="export/_out-${i}.png"
	newname="export/_${name}.png"
	optname="export/${name}.png"

	mv "$oldname" "${newname}"
	pngcrush -brute "${newname}" "${optname}"
	((i++))
done

rm export/_*

