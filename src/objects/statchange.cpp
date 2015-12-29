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
#include <objects/statchange.h>
#include <objects/object.h>
#include <stats.h>
#include <buffer.h>
#include <constants.h>
#include <font.h>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <sstream>

// Constructor
_StatChange::_StatChange() {
	Reset();
}

// Reset stats
void _StatChange::Reset() {
	Object = nullptr;
	Health = 0;
	MaxHealth = 0;
	Mana = 0;
	MaxMana = 0;
	BattleSpeed = 0;
	Experience = 0;
	Gold = 0;
	Invisible = -1;
	ActionBarSize = 0;
}

// Get bit field of fields changed
int _StatChange::GetChangedFlag() {
	int Flag = 0;

	if(StatusEffect.Buff != nullptr)
		Flag |= StatType::STATUSEFFECT;
	if(Health != 0.0f)
		Flag |= StatType::HEALTH;
	if(MaxHealth != 0.0f)
		Flag |= StatType::MAXHEALTH;
	if(Mana != 0.0f)
		Flag |= StatType::MANA;
	if(MaxMana != 0.0f)
		Flag |= StatType::MAXMANA;
	if(BattleSpeed != 0.0f)
		Flag |= StatType::BATTLESPEED;
	if(Experience != 0)
		Flag |= StatType::EXPERIENCE;
	if(Gold != 0)
		Flag |= StatType::GOLD;
	if(Invisible != -1)
		Flag |= StatType::INVISIBLE;
	if(ActionBarSize != 0)
		Flag |= StatType::ACTIONBARSIZE;

	return Flag;
}

// Serialize network
void _StatChange::Serialize(_Buffer &Data) {
	if(!Object)
		throw std::runtime_error("_StatChange::Serialize: Object is null!");

	int ChangedFlag = GetChangedFlag();
	Data.Write<NetworkIDType>(Object->NetworkID);
	Data.Write<int>(ChangedFlag);

	if(ChangedFlag & StatType::STATUSEFFECT)
		StatusEffect.Serialize(Data);
	if(ChangedFlag & StatType::HEALTH)
		Data.Write<float>(Health);
	if(ChangedFlag & StatType::MAXHEALTH)
		Data.Write<float>(MaxHealth);
	if(ChangedFlag & StatType::MANA)
		Data.Write<float>(Mana);
	if(ChangedFlag & StatType::MAXMANA)
		Data.Write<float>(MaxMana);
	if(ChangedFlag & StatType::BATTLESPEED)
		Data.Write<float>(BattleSpeed);
	if(ChangedFlag & StatType::EXPERIENCE)
		Data.Write<int>(Experience);
	if(ChangedFlag & StatType::GOLD)
		Data.Write<int>(Gold);
	if(ChangedFlag & StatType::INVISIBLE)
		Data.Write<int>(Invisible);
	if(ChangedFlag & StatType::ACTIONBARSIZE)
		Data.Write<int>(ActionBarSize);
}

// Unserialize network
void _StatChange::Unserialize(_Buffer &Data, _Manager<_Object> *Manager) {
	Reset();

	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	Object = Manager->IDMap[NetworkID];

	int ChangedFlag = Data.Read<int>();
	if(ChangedFlag & StatType::STATUSEFFECT)
		StatusEffect.Unserialize(Data, Object->Stats);
	if(ChangedFlag & StatType::HEALTH)
		Health = Data.Read<float>();
	if(ChangedFlag & StatType::MAXHEALTH)
		MaxHealth = Data.Read<float>();
	if(ChangedFlag & StatType::MANA)
		Mana = Data.Read<float>();
	if(ChangedFlag & StatType::MAXMANA)
		MaxMana = Data.Read<float>();
	if(ChangedFlag & StatType::BATTLESPEED)
		BattleSpeed = Data.Read<float>();
	if(ChangedFlag & StatType::EXPERIENCE)
		Experience = Data.Read<int>();
	if(ChangedFlag & StatType::GOLD)
		Gold = Data.Read<int>();
	if(ChangedFlag & StatType::INVISIBLE)
		Invisible = Data.Read<int>();
	if(ChangedFlag & StatType::ACTIONBARSIZE)
		ActionBarSize = Data.Read<int>();
}

// Constructor
_StatChangeUI::_StatChangeUI() :
	Object(nullptr),
	Font(nullptr),
	Color(1.0f),
	LastPosition(0, 0),
	Position(0, 0),
	Direction(-1.0f),
	Time(0.0),
	Timeout(HUD_STATCHANGE_TIMEOUT),
	Change(0) {

}

// Render stat change
void _StatChangeUI::Render(double BlendFactor) {
	if(!Object || Change == 0)
		return;

	// Get alpha
	double TimeLeft = Timeout - Time;
	Color.a = 1.0f;
	if(TimeLeft < HUD_ACTIONRESULT_FADETIME)
		Color.a = (float)(TimeLeft / HUD_ACTIONRESULT_FADETIME);

	// Get final draw position
	glm::vec2 DrawPosition = glm::mix(LastPosition, Position, BlendFactor);

	// Draw text
	Font->DrawText(Text.c_str(), DrawPosition + glm::vec2(0, 7), Color, CENTER_BASELINE);
}

// Set text and color
void _StatChangeUI::SetText(const glm::vec4 &NegativeColor, const glm::vec4 &PositiveColor) {

	// Get text color
	std::stringstream Buffer;
	if(Change > 0) {
		Color = PositiveColor;
		Buffer << "+";
	}
	else if(Change < 0) {
		Color = NegativeColor;
		Buffer << "-";
	}
	else {
		Color = COLOR_WHITE;
		Buffer << " ";
	}

	// Set text
	Buffer << std::abs(Change);
	Text = Buffer.str();
}
