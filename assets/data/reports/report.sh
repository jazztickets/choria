#!/bin/bash

db="../../../working/stats/stats.db"

query=".headers on \nselect name, experience, gold, health, printf('%.2f', 1.0*experience / health) as eph, printf('%.2f', 1.0*gold/health) as gph from monster"

echo -e "GOLD PER HITPOINT\n"
echo -e "$query order by gph" | sqlite3 "$db" | column -t -s '|'

echo
echo -e "EXP PER HITPOINT\n"
echo -e "$query order by eph" | sqlite3 "$db" | column -t -s '|'

query=".headers on \nselect name, cost, mindamage, maxdamage, pierce, (mindamage+maxdamage)/2.0+pierce as avg, printf('%.2f', ((mindamage+maxdamage)/2.0+pierce)/cost) as dpc from item where itemtype_id in (5, 6, 14)"

echo
echo -e "WEAPONS BY COST\n"
echo -e "$query order by cost" | sqlite3 "$db" | column -t -s '|'

echo
echo -e "WEAPONS BY AVG DAMAGE\n"
echo -e "$query order by avg" | sqlite3 "$db" | column -t -s '|'

echo
echo -e "WEAPONS BY DPC\n"
echo -e "$query order by dpc" | sqlite3 "$db" | column -t -s '|'
