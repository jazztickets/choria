#!/bin/bash
grep TRADER "$1" | grep "[0-9]\+x\ [^(]*" -o | sort | uniq -c | gawk 'BEGIN{FS=OFS=" "}{item=$0; sub($1 " " $2, "", item); sub("^ *", "", item); add=$1;if($2 == "5x") { add = $1 * 5; }  sum[item]+=add; } END { for(item in sum) { print sum[item], item}}' | sort -n
