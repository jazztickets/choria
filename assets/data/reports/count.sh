#!/bin/bash

in="$1"
keyword="$2"

egrep "$keyword" "$in" | grep "[0-9]\+x\ [^(]*" -o | sort | uniq -c | gawk '
BEGIN {
	FS = " "
	OFS = "\t"
}
{
	# get item name from line
	item = $0
	sub($1 " " $2, "", item)
	sub("^ *", "", item)

	# get count
	add = $1
	if($2 == "5x")
		add *= 5;

	# add to total
	sum[item] += add;
}
END {

	# print all totals
	for(item in sum) {
		print sum[item], item }
}
' | sort -n
