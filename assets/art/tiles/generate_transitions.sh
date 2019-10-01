#!/bin/bash

in="$1"

if [ -z $in ]; then
	echo "No input path!"
	exit 1
fi

# generate one set of images
generate() {

	start=$1
	out=$2
	mkdir -p $out

	# generate directions
	cp      $start                                         $out/01.png
	convert $out/01.png -rotate 90                         $out/02.png
	convert $out/01.png -rotate 180                        $out/04.png
	convert $out/01.png -rotate 270                        $out/08.png

	# generate L shapes
	convert $out/01.png $out/02.png -evaluate-sequence max $out/03.png
	convert $out/03.png -rotate 90                         $out/06.png
	convert $out/03.png -rotate 180                        $out/12.png
	convert $out/03.png -rotate 270                        $out/09.png

	# generate U shapes
	convert $out/03.png $out/04.png -evaluate-sequence max $out/07.png
	convert $out/07.png -rotate 90                         $out/14.png
	convert $out/07.png -rotate 180                        $out/13.png
	convert $out/07.png -rotate 270                        $out/11.png

	# generate pair shapes
	convert $out/01.png $out/04.png -evaluate-sequence max $out/05.png
	convert $out/05.png -rotate 90                         $out/10.png

	# generate blank
	convert $in/01.png -fill transparent -draw 'color 0,0 reset' $out/00.png

	# generate all sides
	convert $out/01.png $out/02.png $out/04.png $out/08.png -evaluate-sequence max $out/15.png
}

rm -rf out/
generate "$in/01.png" ./out/a
generate "$in/17.png" ./out/b

# add 16 to filenames in batch b
for f in out/b/*.png; do
	b=`basename ${f%%.*}`
	n=$((10#$b + 16 ))
	mv $f out/b/$n.png
done

mv out/a/*.png out/
mv out/b/*.png out/

rmdir out/a out/b
