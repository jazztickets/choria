#!/bin/bash
source functions.inc

db="../../../working/stats/stats.db"
fields="name, cost, maxlevel, mindamage, maxdamage, chance, level, duration, maxhealth, maxmana, healthregen, manaregen, battlespeed, movespeed, evasion, spell_damage"
pre=".headers on \nSELECT $fields FROM item WHERE itemtype_id IN (5, 6, 14)"
post=" ORDER BY cost, name"

print_data "$pre $post"
