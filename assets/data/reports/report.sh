#!/bin/bash

report="out/report.txt"
db="../../../working/stats/stats.db"
mkdir -p out
rm -f out/report.txt

query=".headers on \nselect name, experience, gold, health, printf('%.2f', 1.0*experience / health) as eph, printf('%.2f', 1.0*gold/health) as gph from monster"

echo -e "GOLD PER HITPOINT\n" >> "$report"
echo -e "$query order by gph" | sqlite3 "$db" | column -t -s '|' >> "$report"

echo >> "$report"
echo -e "EXP PER HITPOINT\n" >> "$report"
echo -e "$query order by eph" | sqlite3 "$db" | column -t -s '|' >> "$report"
