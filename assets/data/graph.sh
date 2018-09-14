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
log="$HOME/.local/share/choria/log/server.log"

# make temp directory
mkdir -p graphs

# get list of players
players=$(grep " \[SAVE\]" "$log" | grep -P "(?<=Saving player ).*(?= \()" -o | sort | uniq)

# iterate through each player and extract save data
echo "$players" | while read player; do

	# create player file
	player_file="graphs/$player"
	#echo "$player" > "$player_file"

	# get playtime and gold
	grep "\[SAVE\] Saving player $player" "$log" | grep "( .* )" -o | gawk 'BEGIN{FS=" "}{print $5, $4}' | tr -d '[A-Za-z=]' > "$player_file"

done

# graph data
gnuplot gold.gpi -p
