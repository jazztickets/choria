#!/bin/bash
source functions.inc

db="../../../working/stats/stats.db"
fields="name, cost, maxlevel, armor, block, res, maxhealth, maxmana, healthregen, manaregen, battlespeed, movespeed, evasion, spell_damage"
pre=".headers on \nSELECT $fields FROM item WHERE itemtype_id IN (2, 3, 4, 7) AND "
post=" ORDER BY cost, name"

print_data "$pre name LIKE '%mage%' $post"
print_data "$pre name LIKE '%wizard%' $post"
print_data "$pre name LIKE '%arcane%' $post"
print_data "$pre name LIKE '%leather%' $post"
print_data "$pre name LIKE '%reinforced%' $post"
print_data "$pre name LIKE '%warrior%' $post"
print_data "$pre name LIKE '%cloth%' $post"
print_data "$pre name LIKE '%black%' $post"
print_data "$pre name LIKE '%elusive%' $post"
print_data "$pre name LIKE '%bronze%' $post"
print_data "$pre name LIKE '%iron%' $post"
print_data "$pre name LIKE '%steel%' $post"
