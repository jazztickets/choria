#!/bin/bash

# check for zip
type zip >/dev/null 2>&1 || {
	echo >&2 "zip is not installed ";
	exit 1;
}

dest="4k/out/"
rm -rf "$dest"

mkdir -p "$dest"
mkdir -p "$dest/buffs"
mkdir -p "$dest/builds"
mkdir -p "$dest/buttonbar"
mkdir -p "$dest/hud"
mkdir -p "$dest/items"
mkdir -p "$dest/menu"
mkdir -p "$dest/minigames"
mkdir -p "$dest/models"
mkdir -p "$dest/monsters"
mkdir -p "$dest/objects"
mkdir -p "$dest/portraits"
mkdir -p "$dest/skills"
mkdir -p "$dest/status"

# make logo
./make_pngs.sh vector/logo.svgz 2
mv export/*.png "$dest/menu/"

# make textures
for f in buffs builds buttonbar minigames models monsters objects portraits skills status; do
	./make_pngs.sh vector/$f.svgz 2
	mv export/*.png "$dest/$f/"
done

# make hud
for f in hud hud24 hud48; do
	./make_pngs.sh vector/$f.svgz 2
	mv export/*.png "$dest/hud/"
done

# make items
for f in armors boots helms shields weapons items rebirth; do
	./make_pngs.sh vector/$f.svgz 2
	mv export/*.png "$dest/items/"
done

# copy hud texture
cp ../working/textures/hud/body.png 4k/textures/hud/

# remove old items
rm "$dest/items/metal_"*

# make zip
pushd 4k

mkdir -p textures
rm -f textures/*
for f in out/*; do
	../../working/pack.py ./ "$f"

	pack=$(basename "$f")
	mv -v "$pack" textures/
done

zip -r "choria_4k_textures.zip" "textures/"

popd
