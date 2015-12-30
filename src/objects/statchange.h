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
#pragma once

// Libraries
#include <objects/statuseffect.h>
#include <manager.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>

// Forward Declarations
class _Object;
class _Battle;
class _Buffer;
class _Font;

// Types of stats
enum StatType : int {
	STATUSEFFECT    = (1 << 1),
	HEALTH          = (1 << 2),
	MAXHEALTH       = (1 << 3),
	MANA            = (1 << 4),
	MAXMANA         = (1 << 5),
	BATTLESPEED     = (1 << 6),
	HITCHANCE       = (1 << 7),
	EXPERIENCE      = (1 << 8),
	GOLD            = (1 << 9),
	INVISIBLE       = (1 << 10),
	ACTIONBARSIZE   = (1 << 11),
};

// Stat changes
class _StatChange {

	public:

		_StatChange();

		void Reset();
		void Serialize(_Buffer &Data);
		void Unserialize(_Buffer &Data, _Manager<_Object> *Manager);

		int GetChangedFlag();

		_Object *Object;
		_StatusEffect StatusEffect;
		float Health;
		float MaxHealth;
		float Mana;
		float MaxMana;
		float BattleSpeed;
		float HitChance;
		int Experience;
		int Gold;
		int Invisible;
		int ActionBarSize;

};

// Graphical stat change
class _StatChangeUI {

	public:

		_StatChangeUI();

		void Render(double BlendFactor);
		void SetText(const glm::vec4 &NegativeColor, const glm::vec4 &PositiveColor);

		_Object *Object;
		const _Font *Font;
		std::string Text;
		glm::vec4 Color;
		glm::vec2 StartPosition;
		glm::vec2 LastPosition;
		glm::vec2 Position;
		float Direction;
		double Time;
		double Timeout;
		int Change;

};

