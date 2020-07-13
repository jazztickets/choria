#!/bin/bash

report="out/report.txt"
db="../../../working/stats/stats.db"
mkdir -p out

rm -f out/report.txt

echo -e "GOLD PER HITPOINT\n" >> "$report"
echo -e ".headers on \nselect name, experience, gold, health, 1.0*experience / health as eph, 1.0*gold/health as gph from monster order by gph" | sqlite3 "$db" | column -t -s '|' >> "$report"

echo >> "$report"
echo -e "EXP PER HITPOINT\n" >> "$report"
echo -e ".headers on \nselect name, experience, gold, health, 1.0*experience / health as eph, 1.0*gold/health as gph from monster order by eph" | sqlite3 "$db" | column -t -s '|' >> "$report"
