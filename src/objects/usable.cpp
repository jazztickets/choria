/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <objects/usable.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/fighter.h>
#include <ae/assets.h>
#include <ae/graphics.h>
#include <ae/font.h>
#include <constants.h>
#include <stats.h>
#include <scripting.h>
#include <sstream>

// Constants
const int TOOLTIP_LARGE_SPACING = 56;
const int TOOLTIP_TARGET_OFFSET = -28;

// Constructor
_Usable::_Usable() :
	Texture(nullptr),
	NetworkID(0),
	Level(0),
	MaxLevel(0),
	Duration(0.0),
	AttackDelay(0.0),
	AttackTime(0.0),
	Cooldown(0.0),
	Stamina(0.0f),
	Target(TargetType::NONE),
	Scope(ScopeType::NONE),
	TargetAlive(true) {
}

// Destructor
_Usable::~_Usable() {
}

// Draw tooltip window and header
void _Usable::DrawTooltipBase(const glm::vec2 &Position, const _Object *Player, const std::string &TypeText, glm::vec2 &DrawPosition, glm::vec2 &Size) const {

	// Get elements
	ae::_Element *TooltipElement = ae::Assets.Elements["element_item_tooltip"];
	ae::_Element *TooltipName = ae::Assets.Elements["label_item_tooltip_name"];
	ae::_Element *TooltipType = ae::Assets.Elements["label_item_tooltip_type"];

	// Set up spacing
	float SidePadding = TOOLTIP_SIDE_PADDING * ae::_Element::GetUIScale();
	float LargeSpacingY = TOOLTIP_LARGE_SPACING * ae::_Element::GetUIScale();

	// Set label values
	TooltipName->Text = Name;
	TooltipType->Text = TypeText;

	// Get size of name
	ae::_TextBounds TextBounds;
	ae::Assets.Fonts["hud_medium"]->GetStringDimensions(TooltipName->Text, TextBounds);

	// Set window size
	Size.x = Size.x * ae::_Element::GetUIScale();
	Size.x = std::max(Size.x, (float)TextBounds.Width / ae::_Element::GetUIScale()) + SidePadding * 2;
	Size.y = INVENTORY_TOOLTIP_HEIGHT * ae::_Element::GetUIScale();
	if(Player->Character->Vendor)
		Size.y += LargeSpacingY;

	// Position window
	glm::vec2 WindowOffset = Position;

	// Center vertically
	if(Position.y < 0) {
		WindowOffset.y = (ae::Graphics.CurrentSize.y - Size.y) / 2;
	}
	else {
		WindowOffset.x += INVENTORY_TOOLTIP_OFFSET * ae::_Element::GetUIScale();
		WindowOffset.y += -(TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;
	}

	// Reposition window if out of bounds
	if(WindowOffset.x + Size.x > ae::Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.x -= Size.x + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.y + Size.y > ae::Graphics.Element->Bounds.End.y - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y -= Size.y + INVENTORY_TOOLTIP_OFFSET - (TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

	// Set element bounds
	TooltipElement->Offset = glm::ivec2(WindowOffset);
	TooltipElement->Size = glm::ivec2(Size);
	TooltipElement->CalculateBounds(false);

	// Render
	TooltipElement->SetActive(true);
	TooltipElement->Render();
	TooltipElement->SetActive(false);

	// Set draw position to center of window
	DrawPosition = glm::vec2(TooltipElement->Size.x / 2 + WindowOffset.x, TooltipType->Bounds.End.y);
	DrawPosition.y += LargeSpacingY;

	// Draw target text
	if(RequiresTarget() && Target != TargetType::NONE) {
		DrawPosition.y += TOOLTIP_TARGET_OFFSET * ae::_Element::GetUIScale();
		std::string InfoText = "Target " + Player->Stats->TargetTypes.at(Target).second;
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, glm::ivec2(DrawPosition), ae::CENTER_BASELINE, glm::vec4(1.0f));
		DrawPosition.y += LargeSpacingY;
	}
}

// Draw description
void _Usable::DrawDescription(_Scripting *Scripting, glm::vec2 &DrawPosition, int DrawLevel, bool ShowLevel, float Width, float SpacingY) const {

	// Draw level text
	if(ShowLevel) {
		ae::Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(DrawLevel), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
		DrawPosition.y += SpacingY;
	}

	// Check for scripting function
	std::string Info = "";
	if(Scripting->StartMethodCall(Script, "GetInfo")) {

		// Get description from script
		Scripting->PushItemParameters(DrawLevel, Duration);
		Scripting->MethodCall(1, 1);
		Info = Scripting->GetString(1);
		Scripting->FinishMethodCall();

		std::stringstream Buffer(Info);
		std::string Token;

		// Draw description
		float TextSpacingY = 26 * ae::_Element::GetUIScale();
		while(std::getline(Buffer, Token, '\n')) {
			std::list<std::string> Strings;
			ae::Assets.Fonts["hud_small"]->BreakupString(Token, Width, Strings, true);
			for(const auto &LineToken : Strings) {
				ae::Assets.Fonts["hud_small"]->DrawTextFormatted(LineToken, glm::ivec2(DrawPosition), ae::CENTER_BASELINE);
				DrawPosition.y += TextSpacingY;
			}
		}

		DrawPosition.y += SpacingY;
	}
}

// Apply the cost
void _Usable::CallApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "ApplyCost")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(2, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Get passive stats
void _Usable::CallStats(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "Stats")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushStatChange(&ActionResult.Source);
		Scripting->MethodCall(3, 1);
		Scripting->GetStatChange(1, ActionResult.Source);
		Scripting->FinishMethodCall();
	}
}

// Return true if the item can be used
bool _Usable::CallCanUse(_Scripting *Scripting, _ActionResult &ActionResult) const {
	_Object *Object = ActionResult.Source.Object;
	if(!Object)
		return false;

	// See if the action can be used
	if(!CheckRequirements(Scripting, ActionResult))
		return false;

	// Check scope
	if(!CheckScope(ActionResult.Scope))
		return false;

	// Check if target is alive
	if(Object->Character->Targets.size() == 1) {
		_Object *Target = *Object->Character->Targets.begin();
		if(!CanTarget(Object, Target))
			return false;
	}

	// Check script's function
	if(Scripting->StartMethodCall(Script, "CanUse")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->MethodCall(2, 1);
		int Value = Scripting->GetBoolean(1);
		Scripting->FinishMethodCall();

		return Value;
	}

	return true;
}

// Return attack time adjustments from skill script. Return false if function doesn't exist.
bool _Usable::CallGetAttackTimesAdjust(_Scripting *Scripting, _Object *Object, double &AttackDelayAdjust, double &AttackTimeAdjust, double &CooldownAdjust) const {

	// Check script's function
	if(Scripting->StartMethodCall(Script, "GetAttackTimesAdjust")) {
		Scripting->PushObject(Object);
		Scripting->MethodCall(1, 3);
		AttackDelayAdjust = Scripting->GetReal(1);
		AttackTimeAdjust = Scripting->GetReal(2);
		CooldownAdjust = Scripting->GetReal(3);
		Scripting->FinishMethodCall();

		return true;
	}

	return false;
}

// Use an item
void _Usable::CallUse(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "Use")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushInt(Duration);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushObject(ActionResult.Target.Object);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(5, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Play audio through scripting
void _Usable::CallPlaySound(_Scripting *Scripting) const {
	if(Scripting->StartMethodCall(Script, "PlaySound")) {
		Scripting->MethodCall(0, 0);
		Scripting->FinishMethodCall();
	}
}

// Check if a skill can be equipped to the action bar
bool _Usable::CanEquip(_Scripting *Scripting, _Object *Object) const {
	if(Scripting->StartMethodCall(Script, "CanEquip")) {
		Scripting->PushObject(Object);
		Scripting->MethodCall(1, 1);
		int Value = Scripting->GetBoolean(1);
		Scripting->FinishMethodCall();

		return Value;
	}

	return true;
}

// Check if the item can be used in the given scope
bool _Usable::CheckScope(ScopeType CheckScope) const {
	if(Scope == ScopeType::NONE || (Scope != ScopeType::ALL && Scope != CheckScope))
		return false;

	return true;
}

// Check if an item can target an object
bool _Usable::CanTarget(_Object *SourceObject, _Object *TargetObject) const {
	if(!RequiresTarget())
		return true;

	if(Target == TargetType::NONE)
		return false;

	if(Target == TargetType::SELF && SourceObject != TargetObject)
		return false;

	if(TargetAlive && !TargetObject->Character->IsAlive())
		return false;

	if(!TargetAlive && TargetObject->Character->IsAlive())
		return false;

	if(SourceObject->Character->Battle) {

		if(SourceObject->Fighter->BattleSide == TargetObject->Fighter->BattleSide && !CanTargetAlly())
			return false;

		if(SourceObject->Fighter->BattleSide != TargetObject->Fighter->BattleSide && !CanTargetEnemy())
			return false;
	}

	return true;
}

// Get target count
int _Usable::GetTargetCount(_Scripting *Scripting, _Object *Object) const {
	if(Scripting->StartMethodCall(Script, "GetTargetCount")) {
		Scripting->PushInt(1);
		Scripting->MethodCall(1, 1);
		int Value = Scripting->GetInt(1);
		Scripting->FinishMethodCall();

		return Value;
	}

	return 1;
}
