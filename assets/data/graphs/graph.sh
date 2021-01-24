#!/bin/bash

# check for gnuplot
type gnuplot >/dev/null 2>&1 || {
	echo >&2 "gnuplot is not installed!";
	exit 1;
}

# get path to log file
log="$HOME/.local/share/choria_legacy/log/server.log"
if [ -n "$1" ]; then
	log="$1"
fi


# make temp directory
mkdir -p gold
mkdir -p exp
mkdir -p kills

rm -f gold/*
rm -f exp/*
rm -f kills/*

# get list of players with character id
if [ -n "$2" ]; then
	players=`echo "$2" | tr " " "\n"`
else
	players=$(grep "\[SAVE\] Saving player .* character_id=[0-9]\+ " "$log" -o | sed "s/.*Saving player //" | sed "s/\ ( character_id=/_/" | sort | uniq)
fi

# return data from log file based on player name and column indexes
function get_columns() {
	grep "\[SAVE\] Saving player $name (" "$log" | grep "( .* )" -o | awk -v x=$1 -v y=$2 'BEGIN{FS=" "}{print $x, $y}' | tr -d '[A-Za-z=]' | awk 'BEGIN{FS=" "}{print $1/3600.0, $2}'
}

# iterate through each player and extract save data
echo "$players" | while read player; do

	name=$(echo "${player}" | sed "s/_[0-9]\+$//")

	# create player file
	gold_file="gold/$player"
	exp_file="exp/$player"
	kills_file="kills/$player"

	# get gold
	get_columns 5 4 > "$gold_file"

	# get exp
	get_columns 5 3 > "$exp_file"

	# get kills
	get_columns 5 6 > "$kills_file"

done

# graph data
gnuplot gold.gpi -p &
gnuplot exp.gpi -p &
gnuplot kills.gpi -p &

echo "Waiting for windows to close"
wait
