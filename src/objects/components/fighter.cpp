/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
#include <objects/components/fighter.h>
#include <objects/statuseffect.h>
#include <objects/item.h>
#include <hud/hud.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <stdexcept>

// Constructor
_Fighter::_Fighter(_Object *Object) :
	Object(Object),
	BattleElement(nullptr),
	BattleBaseOffset(0.0f, 0.0f),
	ResultPosition(0.0f, 0.0f),
	StatPosition(0.0f, 0.0f),
	TurnTimer(0.0),
	GoldStolen(0),
	Corpse(1),
	TargetIndex(0),
	JoinedBattle(false),
	FleeBattle(false),
	BattleSide(0) {

	ItemDropsReceived.reserve(10);
}

// Get starting side for potential action based on target type
int _Fighter::GetStartingSide(const _Item *Item) {

	// Default to enemy side
	int StartingSide = !BattleSide;

	// Check for self/ally target
	if(Item->TargetID != TargetType::ANY && Item->CanTargetAlly())
		StartingSide = BattleSide;

	return StartingSide;
}

// Create a UI element for battle
void _Fighter::CreateBattleElement(ae::_Element *Parent) {
	if(BattleElement)
		throw std::runtime_error("_Fighter::CreateBattleElement: BattleElement already exists!");

	BattleElement = new ae::_Element();
	BattleElement->Name = "battle_element";
	BattleElement->BaseSize = glm::vec2(100, 100);
	BattleElement->BaseOffset = BattleBaseOffset;
	BattleElement->Alignment = ae::CENTER_MIDDLE;
	BattleElement->Active = true;
	BattleElement->Index = _HUD::WINDOW_HUD_EFFECTS;
	BattleElement->UserData = Object;
	BattleElement->Parent = Parent;
	BattleElement->Style = (BattleSide == 0) ? ae::Assets.Styles["style_battle_slot_green"] : ae::Assets.Styles["style_battle_slot_red"];
	BattleElement->Clickable = true;
	BattleElement->CalculateBounds();
	Parent->Children.push_back(BattleElement);
}

// Remove battle element
void _Fighter::RemoveBattleElement() {
	if(!BattleElement)
		return;

	if(BattleElement->Parent)
		BattleElement->Parent->RemoveChild(BattleElement);

	for(auto &Child : BattleElement->Children) {
		if(Child->UserData)
			((_StatusEffect *)Child->UserData)->BattleElement = nullptr;
	}

	delete BattleElement;
	BattleElement = nullptr;
}
