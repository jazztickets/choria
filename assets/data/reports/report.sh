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

query=".headers on \nselect name, cost, mindamage, maxdamage, (mindamage+maxdamage)/2.0 as avg, printf('%.2f', ((mindamage+maxdamage)/2.0)/cost) as dpc from item where itemtype_id in (5, 6)"

echo >> "$report"
echo -e "WEAPONS BY COST\n" >> "$report"
echo -e "$query order by cost" | sqlite3 "$db" | column -t -s '|' >> "$report"

echo >> "$report"
echo -e "WEAPONS BY AVG DAMAGE\n" >> "$report"
echo -e "$query order by avg" | sqlite3 "$db" | column -t -s '|' >> "$report"

echo >> "$report"
echo -e "WEAPONS BY DPC\n" >> "$report"
echo -e "$query order by dpc" | sqlite3 "$db" | column -t -s '|' >> "$report"
