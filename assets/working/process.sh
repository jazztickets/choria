#!/bin/bash

mkdir -p ../../working/textures

for f in textures/*; do
	./pack.py ./ "$f"

	pack=$(basename "$f")
	mv -v "${pack}.bin" "../../working/textures/${pack}"
done
