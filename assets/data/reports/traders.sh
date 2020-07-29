#!/bin/bash
sqlite3 ../../../working/stats/stats.db 'select r.name || " " || t.count || "x" as reward, group_concat(i.name || " " || ti.count || "x", ", ") as required_items from trader t, item r, traderitem ti, item i where t.item_id = r.id and ti.item_id = i.id and ti.trader_id = t.id group by reward order by reward;'
