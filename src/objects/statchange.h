/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <unordered_map>

// Forward Declarations
class _Object;

namespace ae {
	template<class T> class _Manager;
	class _Font;
	class _Buffer;
}

enum class StatValueType : int {
	BOOLEAN,
	INTEGER,
	INTEGER64,
	FLOAT,
	PERCENT,
	TIME,
	POINTER,
};

enum class StatUpdateType : int {
	NONE,
	ADD,
	SET,
	MULTIPLICATIVE,
};

struct _Value {
	float Mult() const { return Int * 0.01f; }

	union {
		int Int;
		int64_t	Int64;
		float Float;
		double Double;
		void *Pointer;
	};
};

// Stat changes
class _StatChange {

	public:

		_StatChange();

		void Reset() { Object = nullptr; Values.clear(); }
		bool HasStat(const std::string &Name) const { return Values.find(Name) != Values.end();	}

		void Serialize(ae::_Buffer &Data);
		void Unserialize(ae::_Buffer &Data, ae::_Manager<_Object> *Manager);

		// Owner
		_Object *Object;

		// Data
		std::unordered_map<std::string, _Value> Values;
};

// Graphical stat change
class _StatChangeUI {

	public:

		_StatChangeUI();

		void Render(double BlendFactor);
		void SetText(const glm::vec4 &NegativeColor, const glm::vec4 &PositiveColor);

		const ae::_Font *Font;
		std::string Text;
		glm::vec4 Color;
		glm::vec2 StartPosition;
		glm::vec2 LastPosition;
		glm::vec2 Position;
		float Direction;
		double Time;
		double Timeout;
		float Change;
		bool Battle;

};
