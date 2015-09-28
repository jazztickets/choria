/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <objects/item.h>
#include <stats.h>

// Returns the range of damage
void ItemClass::GetDamageRange(int &TMin, int &TMax) const {

	TMin = (int)(Damage - DamageRange);
	TMax = (int)(Damage + DamageRange);
}

// Returns the range of defense
void ItemClass::GetDefenseRange(int &TMin, int &TMax) const {

	TMin = (int)(Defense - DefenseRange);
	TMax = (int)(Defense + DefenseRange);
}

// Returns a string of the item type
void ItemClass::GetType(stringc &TString) const {

	switch(Type) {
		case TYPE_HEAD:
			TString = "Helmet";
		break;
		case TYPE_BODY:
			TString = "Armor";
		break;
		case TYPE_LEGS:
			TString = "Boots";
		break;
		case TYPE_WEAPON1HAND:
			TString = "Weapon";
		break;
		case TYPE_WEAPON2HAND:
			TString = "Weapon";
		break;
		case TYPE_SHIELD:
			TString = "Shield";
		break;
		case TYPE_RING:
			TString = "Ring";
		break;
		case TYPE_POTION:
			TString = "Potion";
		break;
		case TYPE_TRADE:
			TString = "Tradable";
		break;
		default:
		break;
	}
}

// Returns the item's price to/from a vendor
int ItemClass::GetPrice(const VendorStruct *TVendor, int TCount, bool TBuy) const {
	if(!TVendor)
		return 0;

	// Calculate
	int Price;
	if(TBuy)
		Price = (int)(Cost * TVendor->BuyPercent) * TCount;
	else
		Price = (int)(Cost * TVendor->SellPercent) * TCount;

	// Cap
	if(Price < 0)
		Price = 0;
	else if(Price > STATS_MAXGOLD)
		Price = STATS_MAXGOLD;

	return Price;
}
