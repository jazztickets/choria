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
#pragma once

// Libraries
#include <ITexture.h>

// Forward Declarations
class _Fighter;
struct VendorStruct;
struct _FighterResult;

// Classes
class _Skill {
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
		irr::video::ITexture *GetImage() const { return Image; }
		const irr::core::stringc &GetName() const { return Name; }
		const irr::core::stringc &GetInfo() const { return Info; }
		int GetSkillCost() const { return SkillCost; }
		int GetSellCost(int TPlayerLevel) const;

		int GetManaCost(int TLevel) const;
		int GetPower(int TLevel) const;
		void GetPowerRange(int TLevel, int &TMin, int &TMax) const;
		void GetPowerRangeRound(int TLevel, int &TMin, int &TMax) const;
		void GetPowerRange(int TLevel, float &TMin, float &TMax) const;

		void ResolveSkill(_FighterResult *TResult, _FighterResult *TTargetResult) const;
		bool CanUse(_Fighter *TFighter) const;

	private:

		int ID;
		int Type;
		irr::core::stringc Name;
		irr::core::stringc Info;
		irr::video::ITexture *Image;
		int SkillCost;
		float ManaCostBase;
		float ManaCost;
		float PowerBase;
		float PowerRangeBase;
		float Power;
		float PowerRange;
};
