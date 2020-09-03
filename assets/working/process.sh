#!/bin/bash

for f in textures/*; do
	./pack.py ./ "$f"

	pack=$(basename "$f")
	mv -v "$pack" ../../working/textures
done
