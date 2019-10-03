#!/bin/bash

# check for imagemagick
type convert >/dev/null 2>&1 || {
	echo >&2 "imagemagick is not installed ";
	exit 1;
}

# get parameters
in="$1"
out="out"

if [ -z $in ]; then
	echo "No input path!"
	exit 1
fi

# clean out dir
mkdir -p out/
rm -f out/*.png

# build basic pieces
cp      $in/t.png               $out/t.png
convert $in/t.png  -rotate 90   $out/r.png
convert $in/t.png  -rotate 180  $out/b.png
convert $in/t.png  -rotate 270  $out/l.png
cp      $in/tl.png              $out/tl.png
convert $in/tl.png -rotate 90   $out/tr.png
convert $in/tl.png -rotate 180  $out/br.png
convert $in/tl.png -rotate 270  $out/bl.png

# build transitions
convert $out/t.png -fill transparent -draw 'color 0,0 reset' $out/00.png
cp $out/tl.png $out/01.png
cp $out/t.png $out/02.png
cp $out/tr.png $out/03.png
convert $out/tl.png $out/tr.png -evaluate-sequence max $out/04.png
cp $out/l.png $out/05.png
convert $out/t.png $out/l.png -evaluate-sequence max $out/06.png
convert $out/l.png $out/tr.png -evaluate-sequence max $out/07.png
cp $out/r.png $out/08.png
convert $out/r.png $out/tl.png -evaluate-sequence max $out/09.png
convert $out/t.png $out/r.png -evaluate-sequence max $out/10.png
convert $out/l.png $out/r.png -evaluate-sequence max $out/11.png
convert $out/t.png $out/l.png $out/r.png -evaluate-sequence max $out/12.png
cp $out/bl.png $out/13.png
convert $out/tl.png $out/bl.png -evaluate-sequence max $out/14.png
convert $out/t.png $out/bl.png -evaluate-sequence max $out/15.png
convert $out/tr.png $out/bl.png -evaluate-sequence max $out/16.png
convert $out/tl.png $out/tr.png $out/bl.png -evaluate-sequence max $out/17.png
convert $out/r.png $out/bl.png -evaluate-sequence max $out/18.png
convert $out/r.png $out/tl.png $out/bl.png -evaluate-sequence max $out/19.png
convert $out/t.png $out/r.png $out/bl.png -evaluate-sequence max $out/20.png
cp $out/b.png $out/21.png
convert $out/b.png $out/tl.png -evaluate-sequence max $out/22.png
convert $out/t.png $out/b.png -evaluate-sequence max $out/23.png
convert $out/b.png $out/tr.png -evaluate-sequence max $out/24.png
convert $out/b.png $out/tl.png $out/tr.png -evaluate-sequence max $out/25.png
convert $out/l.png $out/b.png -evaluate-sequence max $out/26.png
convert $out/t.png $out/l.png $out/b.png -evaluate-sequence max $out/27.png
convert $out/l.png $out/b.png $out/tr.png -evaluate-sequence max $out/28.png
convert $out/r.png $out/b.png -evaluate-sequence max $out/29.png
convert $out/r.png $out/b.png $out/tl.png -evaluate-sequence max $out/30.png
convert $out/t.png $out/r.png $out/b.png -evaluate-sequence max $out/31.png
convert $out/l.png $out/r.png $out/b.png -evaluate-sequence max $out/32.png
convert $out/t.png $out/l.png $out/r.png $out/b.png -evaluate-sequence max $out/33.png
cp $out/br.png $out/34.png
convert $out/tl.png $out/br.png -evaluate-sequence max $out/35.png
convert $out/t.png $out/br.png -evaluate-sequence max $out/36.png
convert $out/tr.png $out/br.png -evaluate-sequence max $out/37.png
convert $out/tl.png $out/tr.png $out/br.png -evaluate-sequence max $out/38.png
convert $out/l.png $out/br.png -evaluate-sequence max $out/39.png
convert $out/t.png $out/l.png $out/br.png -evaluate-sequence max $out/40.png
convert $out/l.png $out/tr.png $out/br.png -evaluate-sequence max $out/41.png
convert $out/bl.png $out/br.png -evaluate-sequence max $out/42.png
convert $out/tl.png $out/bl.png $out/br.png -evaluate-sequence max $out/43.png
convert $out/t.png $out/bl.png $out/br.png -evaluate-sequence max $out/44.png
convert $out/tr.png $out/bl.png $out/br.png -evaluate-sequence max $out/45.png
convert $out/tl.png $out/tr.png $out/bl.png $out/br.png -evaluate-sequence max $out/46.png

rm out/t*.png
rm out/r*.png
rm out/b*.png
rm out/l*.png
