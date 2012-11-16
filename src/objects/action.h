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
#ifndef ACTION_H
#define ACTION_H

// Libraries
#include <irrlicht/irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class FighterClass;

// Classes
class ActionClass {

	public:
	
		enum ActionType {
			YPE_SKILL,
			YPE_ITEM,
		};

		ActionClass();

		// Stats
		ActionType GetActionType() const { return ActionType; }
		int GetID() const { return ID; }
		int GetType() const { return Type; }
		const stringc &GetName() const { return Name; }
		ITexture *GetImage() const { return Image; }
			
		void GetDamageRange(int &Min, int &Max) const;
		virtual int GenerateDamage(const FighterClass *Fighter) const;
		virtual bool CanUse(const FighterClass *Fighter) const;

	protected:

		ActionType ActionType;
		int ID;
		int Type;
		stringc Name;
		ITexture *Image;
		float Damage;
		float DamageRange;
};

#endif
