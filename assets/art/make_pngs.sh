#!/bin/bash

file=$1
names=`grep "title[0-9]" "$file" | egrep ">[a-z_]+<" -o | tr -d '<>'`

convert -density 45 -background none $file -crop 32x32 -depth 8 +repage PNG32:export/_out.png

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

