#!/bin/bash

function print_data() {
	echo -e "$1" | sqlite3 "$db" | gawk '
		BEGIN {
			FS=OFS="|"
		}
		{
			for(i = 1; i <= NF; i++) {
				data[NR][i] = $i
				if(NR > 1 && $i != 0) {
					sum[i] += $i
					nonzero[i] = 1
				}
			}
		}
		END {
			for(row in data) {
				for(column in nonzero) {
					printf("%s|", data[row][column])
				}
				printf("\n")
			}
			for(column in nonzero) {
				if(column == 1)
					printf("TOTAL|")
				else
					printf("%s|", sum[column])
			}
		}
	' | column -t -s '|' 

	echo
}

db="../../../working/stats/stats.db"
fields="name, cost, maxlevel, armor, block, res, maxhealth, maxmana, healthregen, manaregen, battlespeed, movespeed, evasion, spell_damage"
pre=".headers on \nSELECT $fields FROM item WHERE "
post=" ORDER BY cost, name"

print_data "$pre name LIKE '%mage%' $post"
print_data "$pre name LIKE '%wizard%' $post"
print_data "$pre name LIKE '%leather%' $post"
print_data "$pre name LIKE '%reinforced%' $post"
