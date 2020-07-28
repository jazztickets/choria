#!/bin/bash
pushd ../../../working/maps/

# get list of map change events
zgrep "^e 3" *.map.gz | sed 's/:e 3 /:/g' | sort | uniq | sort -V > mapchanges.txt

# get list of vendors by map
zgrep "^e 4" *.map.gz | sed 's/:e 4 /:/g' | sort | uniq | sort -V > vendors.txt

# print list of map files to ids
sqlite3 ../stats/stats.db 'select replace(file, "maps/", ""), id from map where file != ""' | tr '|' : > maptoid.txt

# print list of map connections by id
echo "Paths = {"
gawk 'BEGIN{FS=":"} NR == FNR { maptoid[$1] = $2 } NR != FNR { maps[$1] = maps[$1] ", " $2 } END { for(map in maps) { print "\t[" maptoid[map] "] = " maps[map] }}' maptoid.txt mapchanges.txt | sed 's/,/{/'| sed 's/$/ },/' | sort -V
echo "}"

# print list of vendors and map connections
echo "Vendors = {"
gawk 'BEGIN{FS=":"} NR == FNR { maptoid[$1] = $2 } NR != FNR { vendors[$2] = $1 } END { for(vendor in vendors) { print "\t[" vendor "] = " maptoid[vendors[vendor]] "," }}' maptoid.txt vendors.txt | sort -V
echo "}"

# clean up
rm maptoid.txt vendors.txt mapchanges.txt
popd
