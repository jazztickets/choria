/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2015  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <objects/item.h>
#include <stats.h>
#include <constants.h>

// Returns the range of damage
void _Item::GetDamageRange(int &Min, int &Max) const {

	Min = (int)(Damage - DamageRange);
	Max = (int)(Damage + DamageRange);
}

// Returns the range of defense
void _Item::GetDefenseRange(int &Min, int &Max) const {

	Min = (int)(Defense - DefenseRange);
	Max = (int)(Defense + DefenseRange);
}

// Returns a string of the item type
void _Item::GetType(std::string &String) const {

	switch(Type) {
		case TYPE_HEAD:
			String = "Helmet";
		break;
		case TYPE_BODY:
			String = "Armor";
		break;
		case TYPE_LEGS:
			String = "Boots";
		break;
		case TYPE_WEAPON1HAND:
			String = "Weapon";
		break;
		case TYPE_WEAPON2HAND:
			String = "Weapon";
		break;
		case TYPE_SHIELD:
			String = "Shield";
		break;
		case TYPE_RING:
			String = "Ring";
		break;
		case TYPE_POTION:
			String = "Potion";
		break;
		case TYPE_TRADE:
			String = "Tradable";
		break;
		default:
		break;
	}
}

// Returns the item's price to/from a vendor
int _Item::GetPrice(const _Vendor *Vendor, int TCount, bool TBuy) const {
	if(!Vendor)
		return 0;

	// Calculate
	int Price;
	if(TBuy)
		Price = (int)(Cost * Vendor->BuyPercent) * TCount;
	else
		Price = (int)(Cost * Vendor->SellPercent) * TCount;

	// Cap
	if(Price < 0)
		Price = 0;
	else if(Price > STATS_MAXGOLD)
		Price = STATS_MAXGOLD;

	return Price;
}
