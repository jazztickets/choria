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
#ifndef ITEM_H
#define ITEM_H

// Libraries
#include "action.h"
#include <irrlicht/irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
struct VendorStruct;

// Classes
class ItemClass : public ActionClass {
	friend class StatsClass;

	public:
	
		enum ItemType {
			YPE_HEAD,
			YPE_BODY,
			YPE_LEGS,
			YPE_WEAPON1HAND,
			YPE_WEAPON2HAND,
			YPE_SHIELD,
			YPE_RING,
			YPE_USEABLE,
			YPE_TRADE,
		};

		ItemClass() { ActionType = YPE_ITEM; }

		int GetLevel() const { return Level; }
		int GetHealthRestore() const { return HealthRestore; }
		int GetManaRestore() const { return ManaRestore; }
		int GetMaxHealth() const { return MaxHealth; }
		int GetMaxMana() const { return MaxMana; }
		float GetHealthRegen() const { return HealthRegen; }
		float GetManaRegen() const { return ManaRegen; }

		void GetDefenseRange(int &Min, int &Max) const;
		void GetTypeAsString(stringc &TString) const;
		int GetPrice(const VendorStruct *Vendor, int Count, bool DrawY) const;

	private:

		int Level;
		int LevelRequired;
		int Cost;
		float Defense;
		float DefenseRange;
		int DamageType;
		int HealthRestore;
		int ManaRestore;
		int MaxHealth;
		int MaxMana;
		float HealthRegen;
		float ManaRegen;
};

#endif
