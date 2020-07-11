#!/bin/bash

# check for gnuplot
type gnuplot >/dev/null 2>&1 || {
	echo >&2 "gnuplot is not installed!";
	exit 1;
}

# check for gawk
type gawk >/dev/null 2>&1 || {
	echo >&2 "gawk is not installed!";
	exit 1;
}

# get path to log file
log="$HOME/.local/share/choria2015/log/server.log"
if [ -n "$1" ]; then
	log="$1"
fi

# make temp directory
mkdir -p gold
mkdir -p exp
mkdir -p kills

rm gold/*
rm exp/*
rm kills/*

# get list of players
players=$(grep " \[SAVE\]" "$log" | grep -P "(?<=Saving player ).*(?= \()" -o | sort | uniq)

# iterate through each player and extract save data
echo "$players" | while read player; do

	# create player file
	gold_file="gold/$player"
	exp_file="exp/$player"
	kills_file="kills/$player"

	# get gold
	grep "\[SAVE\] Saving player $player (" "$log" | grep "( .* )" -o | gawk 'BEGIN{FS=" "}{print $5, $4}' | tr -d '[A-Za-z=]' > "$gold_file"

	# get exp
	grep "\[SAVE\] Saving player $player (" "$log" | grep "( .* )" -o | gawk 'BEGIN{FS=" "}{print $5, $3}' | tr -d '[A-Za-z=]' > "$exp_file"

	# get kills
	grep "\[SAVE\] Saving player $player (" "$log" | grep "( .* )" -o | gawk 'BEGIN{FS=" "}{print $5, $6}' | tr -d '[A-Za-z=]' > "$kills_file"

done

# graph data
gnuplot gold.gpi -p
gnuplot exp.gpi -p
gnuplot kills.gpi -p
