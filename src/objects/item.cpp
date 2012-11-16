/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#include "item.h"
#include "../engine/stats.h"

// Returns the range of defense
void ItemClass::GetDefenseRange(int &Min, int &Max) const {

	Min = (int)(Defense - DefenseRange);
	Max = (int)(Defense + DefenseRange);
}

// Returns a string of the item type
void ItemClass::GetTypeAsString(stringc &TString) const {

	switch(Type) {
		case YPE_HEAD:
			TString = "Helmet";
		break;
		case YPE_BODY:
			TString = "Armor";
		break;
		case YPE_LEGS:
			TString = "Boots";
		break;
		case YPE_WEAPON1HAND:
			TString = "Weapon";
		break;
		case YPE_WEAPON2HAND:
			TString = "Weapon";
		break;
		case YPE_SHIELD:
			TString = "Shield";
		break;
		case YPE_RING:
			TString = "Ring";
		break;
		case YPE_USEABLE:
			TString = "Potion";
		break;
		case YPE_TRADE:
			TString = "Tradable";
		break;
		default:
		break;
	}
}

// Returns the item's price to/from a vendor
int ItemClass::GetPrice(const VendorStruct *Vendor, int Count, bool DrawY) const {
	if(!Vendor)
		return 0;

	// Calculate
	int Price;
	if(DrawY)
		Price = (int)(Cost * Vendor->BuyPercent) * Count;
	else
		Price = (int)(Cost * Vendor->SellPercent) * Count;

	// Cap
	if(Price < 0)
		Price = 0;
	else if(Price > STATS_MAXGOLD)
		Price = STATS_MAXGOLD;

	return Price;
}
