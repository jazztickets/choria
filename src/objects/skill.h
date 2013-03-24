/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
**************************************************************************************/
#ifndef SKILL_H
#define SKILL_H

// Libraries
#include <irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class FighterClass;
struct VendorStruct;
struct FighterResultStruct;

// Classes
class SkillClass {
	friend class StatsClass;

	public:
	
		enum SkillType {
			TYPE_ATTACK,
			TYPE_SPELL,
			TYPE_PASSIVE,
			TYPE_USEPOTION,
		};

		int GetID() const { return ID; }
		int GetType() const { return Type; }
		ITexture *GetImage() const { return Image; }
		const stringc &GetName() const { return Name; }
		const stringc &GetInfo() const { return Info; }
		int GetSkillCost() const { return SkillCost; }
		int GetSellCost(int TPlayerLevel) const;

		int GetManaCost(int TLevel) const;
		int GetPower(int TLevel) const;
		float GetPowerAsFloat(int TLevel) const;
		void GetPowerRange(int TLevel, int &TMin, int &TMax) const;
		void GetPowerRangeRound(int TLevel, int &TMin, int &TMax) const;
		void GetPowerRange(int TLevel, float &TMin, float &TMax) const;

		void ResolveSkill(FighterResultStruct *TResult, FighterResultStruct *TTargetResult) const;
		bool CanUse(FighterClass *TFighter) const;

	private:

		int ID;
		int Type;
		stringc Name;
		stringc Info;
		ITexture *Image;
		int SkillCost;
		float ManaCostBase;
		float ManaCost;
		float PowerBase;
		float PowerRangeBase;
		float Power;
		float PowerRange;
};

#endif
