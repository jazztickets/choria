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
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/statchange.h>
#include <objects/components/inventory.h>
#include <objects/components/character.h>
#include <objects/components/fighter.h>
#include <objects/components/controller.h>
#include <objects/components/monster.h>
#include <objects/statuseffect.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <hud/hud.h>
#include <ae/manager.h>
#include <ae/servernetwork.h>
#include <ae/peer.h>
#include <ae/ui.h>
#include <ae/buffer.h>
#include <ae/assets.h>
#include <ae/graphics.h>
#include <ae/random.h>
#include <ae/font.h>
#include <ae/util.h>
#include <ae/program.h>
#include <packet.h>
#include <server.h>
#include <stats.h>
#include <scripting.h>
#include <constants.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/common.hpp>
#include <json/reader.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>

// Constructor
_Object::_Object() :
	Name(""),
	Character(nullptr),
	Inventory(nullptr),
	Fighter(nullptr),
	Controller(nullptr),
	Monster(nullptr),

	Stats(nullptr),
	Map(nullptr),
	Scripting(nullptr),
	Server(nullptr),
	Peer(nullptr),
	QueuedMapChange(0),

	Position(0, 0),
	ServerPosition(0, 0),

	ModelTexture(nullptr),
	ModelID(0),
	Light(0),

	UpdateID(0),
	Changed(false),
	OldPosition(0, 0),
	OldStatus(0),
	OldInvisible(0),
	OldBounty(0),
	OldLight(0) {

	Inventory = new _Inventory();
	Character = new _Character(this);
	Fighter = new _Fighter(this);
	Controller = new _Controller(this);
	Monster = new _Monster(this);
}

// Destructor
_Object::~_Object() {
	Fighter->RemoveBattleElement();

	if(Map) {
		Map->RemoveObject(this);
		Map = nullptr;
	}

	if(Character->Battle) {
		Character->Battle->RemoveObject(this);
		Character->Battle = nullptr;
	}

	if(Character->HUD) {
		Character->HUD->RemoveStatChanges(this);
		Character->HUD = nullptr;
	}

	if(Character->Bot) {
		delete Peer;
		Peer = nullptr;
	}

	delete Monster;
	delete Controller;
	delete Fighter;
	delete Character;
	delete Inventory;
}

// Updates the player
void _Object::Update(double FrameTime) {
	bool CheckEvent = false;

	// Update bots
	if(Server && Character->Bot)
		UpdateBot(FrameTime);

	// Update player position
	Controller->DirectionMoved = Move();
	if(Controller->DirectionMoved) {
		CheckEvent = true;

		// Remove node from pathfinding
		if(Character->Bot && Character->Path.size())
			Character->Path.erase(Character->Path.begin());
	}

	// Player hit the use button
	if(Controller->UseCommand) {
		CheckEvent = true;
		Controller->UseCommand = false;
	}

	// Update actions and battle
	if(Character->IsAlive()) {

		// Update monster AI
		if(Server && Character->Battle && IsMonster())
			UpdateMonsterAI(FrameTime);

		// Check turn timer
		if(Character->Battle) {
			if(Character->Attributes["Stunned"].Int)
				Fighter->TurnTimer += FrameTime * (1.0 / BATTLE_DEFAULTATTACKPERIOD) * BATTLE_STUNNED_BATTLESPEED * 0.01f;
			else
				Fighter->TurnTimer += FrameTime * (1.0 / Character->BaseAttackPeriod) * Character->Attributes["BattleSpeed"].Mult();
		}
		else
			Fighter->TurnTimer = 1.0;

		// Resolve action
		if(!Character->Attributes["Stunned"].Int && Fighter->TurnTimer >= 1.0) {
			Fighter->TurnTimer = 1.0;

			if(Server && Character->Action.IsSet()) {
				ScopeType Scope = ScopeType::WORLD;
				if(Character->Battle)
					Scope = ScopeType::BATTLE;

				ae::_Buffer Packet;
				if(Character->Action.Resolve(Packet, this, Scope)) {
					SendPacket(Packet);
				}
				else {
					SendActionClear();
				}

				Character->Action.Unset();
			}
		}
	}
	else
		Fighter->TurnTimer = 0.0;

	// Update status effects
	if(Character) {
		Character->Update(FrameTime);

		// Update playtime
		Character->IdleTime += FrameTime;
		Character->Attributes["PlayTime"].Double += FrameTime;
		if(Character->Attributes["Rebirths"].Int)
			Character->Attributes["RebirthTime"].Double += FrameTime;
		if(Character->Battle)
			Character->Attributes["BattleTime"].Double += FrameTime;
	}

	// Update teleport time
	if(Character->TeleportTime > 0.0) {
		Character->Status = _Character::STATUS_TELEPORT;
		Character->TeleportTime -= FrameTime;
		if(Character->TeleportTime <= 0.0) {
			CheckEvent = true;
			Character->TeleportTime = 0.0;
		}
	}

	// Update timers
	Controller->MoveTime += FrameTime;

	// Check events
	if(Map && CheckEvent)
		Map->CheckEvents(this, Scripting);

	// Update status
	if(Character && !Monster->DatabaseID)
		Character->UpdateStatus();

	if(Server) {
		if(Position != OldPosition)
			Changed = true;
		if(Character->Status != OldStatus)
			Changed = true;
		if(OldInvisible != Character->Invisible)
			Changed = true;
		if(Light != OldLight)
			Changed = true;
		if(Character->Attributes["Bounty"].Int != OldBounty)
			Changed = true;
	}

	OldPosition = Position;
	OldStatus = Character->Status;
	OldInvisible = Character->Invisible;
	OldBounty = Character->Attributes["Bounty"].Int;
	OldLight = Light;
}

// Update bot AI
void _Object::UpdateBot(double FrameTime) {

	// Call ai script
	if(!Character->Battle && Scripting->StartMethodCall("Bot_Server", "Update")) {
		Scripting->PushReal(FrameTime);
		Scripting->PushObject(this);
		Scripting->MethodCall(2, 0);
		Scripting->FinishMethodCall();
	}

	// Set input
	if(Character->AcceptingMoveInput()) {
		Character->IdleTime = 0.0;
		int InputState = 0;

		// Call ai input script
		if(Scripting->StartMethodCall("Bot_Server", "GetInputState")) {
			Scripting->PushObject(this);
			Scripting->MethodCall(1, 1);
			InputState = Scripting->GetInt(1);
			Scripting->FinishMethodCall();
		}

		Controller->InputStates.clear();
		if(InputState)
			Controller->InputStates.push_back(InputState);
	}

	// Update battle
	if(Character->Battle) {
		if(Fighter->TurnTimer >= 1.0 && !Character->Action.IsSet()) {

			// Set skill used
			size_t ActionBarIndex = 0;
			if(!Character->GetActionFromActionBar(Character->Action, ActionBarIndex)) {
				return;
			}

			// Check that the action can be used
			_ActionResult ActionResult;
			ActionResult.Source.Object = this;
			ActionResult.Scope = ScopeType::BATTLE;
			ActionResult.ActionUsed = Character->Action;
			if(!Character->Action.Item->CanUse(Scripting, ActionResult))
				Character->Action.Item = nullptr;

			// Separate object list
			std::list<_Object *> Allies, Enemies;
			Character->Battle->GetSeparateObjectList(Fighter->BattleSide, Allies, Enemies);

			// Call lua script
			if(Enemies.size()) {
				if(Scripting->StartMethodCall("AI_Smart", "Update")) {
					Character->Targets.clear();
					Scripting->PushObject(this);
					Scripting->PushObjectList(Enemies);
					Scripting->PushObjectList(Allies);
					Scripting->MethodCall(3, 0);
					Scripting->FinishMethodCall();
				}
			}
		}
	}
}

// Update monster AI during battle
void _Object::UpdateMonsterAI(double FrameTime) {
	if(!Monster->AI.length())
		return;

	// Call AI script to get action
	if(Fighter->TurnTimer >= 1.0 && !Character->Action.IsSet()) {

		// Separate object list
		std::list<_Object *> Allies, Enemies;
		Character->Battle->GetSeparateObjectList(Fighter->BattleSide, Allies, Enemies);

		// Call lua script
		if(Enemies.size()) {
			if(Scripting->StartMethodCall(Monster->AI, "Update")) {
				Character->Targets.clear();
				Scripting->PushObject(this);
				Scripting->PushObjectList(Enemies);
				Scripting->PushObjectList(Allies);
				Scripting->MethodCall(3, 0);
				Scripting->FinishMethodCall();
			}
		}
	}
}

// Renders the player while walking around the world
void _Object::Render(glm::vec4 &ViewBounds, const _Object *ClientPlayer) {
	if(!Map)
		return;

	if(!ModelTexture)
		return;

	if(ClientPlayer && ClientPlayer->Character->Offline && ClientPlayer != this)
		return;

	if(Character->Offline && ClientPlayer != this)
		return;

	// Setup shader
	ae::Graphics.SetProgram(ae::Assets.Programs["map"]);
	glUniformMatrix4fv(ae::Assets.Programs["map"]->TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
	glUniformMatrix4fv(ae::Assets.Programs["map"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

	// Draw debug server position
	glm::vec3 DrawPosition;
	if(Character->HUD && Character->HUD->ShowDebug) {
		DrawPosition = glm::vec3(ServerPosition, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
		ae::Graphics.SetColor(glm::vec4(1, 0, 0, 1));
		ae::Graphics.DrawSprite(DrawPosition, ModelTexture);
	}

	// Set invisible alpha
	float Alpha = 1.0f;
	if(Character->Invisible)
		Alpha = PLAYER_INVIS_ALPHA;

	// Draw model
	DrawPosition = glm::vec3(Position, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
	ae::Graphics.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, Alpha));
	ae::Graphics.DrawSprite(DrawPosition, ModelTexture);
	if(Character->StatusTexture) {
		ae::Graphics.DrawSprite(DrawPosition, Character->StatusTexture);
	}

	// Get name text
	bool SameParty = ClientPlayer->Character->PartyName != "" && ClientPlayer->Character->PartyName == Character->PartyName;
	std::string Color = SameParty ? "green" : "white";
	std::string NameText = "[c " + Color + "]" + Name + "[c white]";
	if(Character->Attributes["Bounty"].Int > 0)
		NameText += " ([c cyan]" + std::to_string(Character->Attributes["Bounty"].Int) + "[c white])";

	std::string Prefix;
	if(Character->Attributes["Rebirths"].Int)
		Prefix = "[c gold]" + std::to_string(Character->Attributes["Rebirths"].Int) + "[c white] ";

	// Cap name to screen
	glm::vec2 NamePosition = DrawPosition;
	float OffsetY = -0.5f;
	if(SameParty || Character->Attributes["Bounty"].Int > 0) {
		ae::_TextBounds TextBounds;
		ae::Assets.Fonts["hud_medium"]->GetStringDimensions(NameText, TextBounds, true);
		float HalfWidth = TextBounds.Width * 0.5f / ModelTexture->Size.x;
		float HalfAbove = (TextBounds.AboveBase) * 0.5f / ModelTexture->Size.x;
		float Above = (TextBounds.AboveBase) * 1.0f / ModelTexture->Size.x;
		NamePosition[0] = glm::clamp(NamePosition[0], ViewBounds[0] + HalfWidth, ViewBounds[2] - HalfWidth);
		NamePosition[1] = glm::clamp(NamePosition[1], ViewBounds[1] + Above - OffsetY, ViewBounds[3] - HalfAbove - OffsetY);
	}

	// Draw name
	if(!Character->Invisible || SameParty) {
		float UIScale = 1.0f;
		if(ae::_Element::GetUIScale() > 1)
			UIScale = ae::_Element::GetUIScale();
		ae::Assets.Fonts["hud_medium"]->DrawTextFormatted(Prefix + NameText, NamePosition + glm::vec2(0, OffsetY), ae::CENTER_BASELINE, 1.0f, 1.0f / ModelTexture->Size.x / UIScale);
	}
}

// Renders the object during a battle
void _Object::RenderBattle(_Object *ClientPlayer, double Time, bool ShowLevel) {
	std::stringstream Buffer;

	// Set color
	glm::vec4 GlobalColor(glm::vec4(1.0f));
	GlobalColor.a = 1.0f;
	if(!Character->IsAlive())
		GlobalColor.a = 0.2f;

	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	ae::Graphics.SetColor(GlobalColor);

	// Draw slot
	Fighter->BattleElement->Fade = GlobalColor.a;

	// Get slot center
	glm::vec2 SlotPosition = Fighter->BattleElement->Bounds.Start;

	// Save positions
	Fighter->ResultPosition = Fighter->BattleElement->Bounds.Start + Fighter->BattleElement->Size / 2.0f;
	Fighter->StatPosition = Fighter->ResultPosition + glm::vec2((Character->Portrait->Size.x/2 + 10 + BATTLE_HEALTHBAR_WIDTH/2), 0) * ae::_Element::GetUIScale();

	// Name
	if(!Character->Invisible) {
		Buffer << Name;
		if(ShowLevel && !Monster->DatabaseID)
			Buffer << " (" << Character->Level << ")" << std::endl;
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), SlotPosition + glm::vec2(0, -12), ae::LEFT_BASELINE, GlobalColor);
		Buffer.str("");
	}

	// Portrait
	if(Character->Portrait) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(SlotPosition + glm::vec2(Character->Portrait->Size/2) * ae::_Element::GetUIScale(), Character->Portrait, GlobalColor);
	}

	// Get health/mana bar positions
	glm::vec2 BarSize = glm::vec2(BATTLE_HEALTHBAR_WIDTH, BATTLE_HEALTHBAR_HEIGHT) * ae::_Element::GetUIScale();
	glm::vec2 BarOffset(Fighter->BattleElement->Size.x + 10 * ae::_Element::GetUIScale(), 0);
	float BarPaddingY = 6 * ae::_Element::GetUIScale();

	// Get bar element bounds
	ae::_Bounds BarBounds;
	BarBounds.Start = SlotPosition + glm::vec2(0, 0) + BarOffset;
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x, BarSize.y) + BarOffset;
	glm::vec2 BarCenter = (BarBounds.Start + BarBounds.End) / 2.0f;
	float BarEndX = BarBounds.End.x;

	// Get text size
	ae::_Font *SmallFont = ae::Assets.Fonts["hud_small"];
	ae::_Font *TinyFont = ae::Assets.Fonts["hud_tiny"];

	// Draw empty bar
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_health_bar_empty"]->Texture);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * Character->GetHealthPercent(), BarSize.y) + BarOffset;
	ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_health_bar_full"]->Texture);

	// Draw health text
	ae::_Font *HealthFont = SmallFont;
	if(Character->Attributes["MaxHealth"].Int > 9999)
		HealthFont = TinyFont;
	int TextOffsetY = (HealthFont->MaxAbove - HealthFont->MaxBelow) / 2 + (int)(2.5 * ae::_Element::GetUIScale());

	Buffer << Character->Attributes["Health"].Int << " / " << Character->Attributes["MaxHealth"].Int;
	HealthFont->DrawText(Buffer.str(), glm::ivec2(BarCenter) + glm::ivec2(0, TextOffsetY), ae::CENTER_BASELINE, GlobalColor);
	Buffer.str("");

	// Draw mana
	if(Character->Attributes["MaxMana"].Int > 0) {
		float ManaPercent = Character->Attributes["MaxMana"].Int > 0 ? Character->Attributes["Mana"].Int / (float)Character->Attributes["MaxMana"].Int : 0;

		// Get ui size
		BarOffset.y += BarSize.y + BarPaddingY;
		BarBounds.Start = SlotPosition + glm::vec2(0, 0) + BarOffset;
		BarBounds.End = SlotPosition + glm::vec2(BarSize.x, BarSize.y) + BarOffset;
		BarCenter = (BarBounds.Start + BarBounds.End) / 2.0f;

		// Draw empty bar
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_mana_bar_empty"]->Texture);

		// Draw full bar
		BarBounds.End = SlotPosition + glm::vec2(BarSize.x * ManaPercent, BarSize.y) + BarOffset;
		ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_mana_bar_full"]->Texture);

		// Draw mana text
		ae::_Font *ManaFont = SmallFont;
		if(Character->Attributes["MaxMana"].Int > 9999)
			ManaFont = TinyFont;
		int TextOffsetY = (ManaFont->MaxAbove - ManaFont->MaxBelow) / 2 + (int)(2.5 * ae::_Element::GetUIScale());
		Buffer << Character->Attributes["Mana"].Int << " / " << Character->Attributes["MaxMana"].Int;
		ManaFont->DrawText(Buffer.str(), glm::ivec2(BarCenter) + glm::ivec2(0, TextOffsetY), ae::CENTER_BASELINE, GlobalColor);
		Buffer.str("");
	}

	// Draw turn timer
	BarOffset.y += BarSize.y + BarPaddingY;
	BarSize.y = 8 * ae::_Element::GetUIScale();
	BarBounds.Start = SlotPosition + glm::vec2(0, 0) + BarOffset;
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x, BarSize.y) + BarOffset;

	// Draw empty bar
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	ae::Graphics.DrawImage(BarBounds, ae::Assets.Textures["textures/hud_repeat/stamina_empty.png"]);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * Fighter->TurnTimer, BarSize.y) + BarOffset;
	ae::Graphics.DrawImage(BarBounds, ae::Assets.Textures["textures/hud_repeat/stamina_full.png"]);

	// Get background for items used
	const ae::_Texture *ItemBackTexture = ae::Assets.Textures["textures/hud/item_back.png"];

	// Draw the action used
	if(ClientPlayer->Fighter->BattleSide == Fighter->BattleSide && Character->Action.Item) {
		glm::vec2 ItemUsingPosition = SlotPosition + glm::vec2((-ItemBackTexture->Size.x/2 - 16) * ae::_Element::GetUIScale(), Fighter->BattleElement->Size.y/2);
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		if(!Character->Action.Item->IsSkill())
			ae::Graphics.DrawScaledImage(ItemUsingPosition, ItemBackTexture, GlobalColor);
		ae::Graphics.DrawScaledImage(ItemUsingPosition, Character->Action.Item->Texture, GlobalColor);
	}

	// Draw potential action to use
	for(auto &BattleTarget : ClientPlayer->Character->Targets) {
		if(BattleTarget == this && ClientPlayer->Fighter->PotentialAction.IsSet()) {

			// Get texture
			const ae::_Texture *Texture = nullptr;
			if(ClientPlayer->Fighter->PotentialAction.Item) {

				// Skip dead targets
				if(!ClientPlayer->Fighter->PotentialAction.Item->CanTarget(Scripting, ClientPlayer, BattleTarget))
					break;

				// Get texture
				Texture = ClientPlayer->Fighter->PotentialAction.Item->Texture;
			}

			// Make icon flash
			glm::vec4 Color(glm::vec4(1.0f));
			double FastTime = Time * 2;
			if(FastTime - (int)FastTime < 0.5)
				Color.a = 0.75f;

			// Draw background icon
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			glm::vec2 DrawPosition = glm::ivec2(BarEndX + 12 * ae::_Element::GetUIScale(), SlotPosition.y + Fighter->BattleElement->Size.y/2);
			if(ClientPlayer->Fighter->PotentialAction.Item && !ClientPlayer->Fighter->PotentialAction.Item->IsSkill()) {
				DrawPosition.x += ItemBackTexture->Size.x/2 * ae::_Element::GetUIScale();
				ae::Graphics.DrawScaledImage(DrawPosition, ItemBackTexture, Color);
			}
			else
				DrawPosition.x += Texture->Size.x/2 * ae::_Element::GetUIScale();

			// Draw item
			ae::Graphics.DrawScaledImage(DrawPosition, Texture, Color);
		}
	}

	// Draw status effects
	glm::vec2 Offset(0, Fighter->BattleElement->BaseSize.y + 2);
	for(auto &StatusEffect : Character->StatusEffects) {
		if(StatusEffect->BattleElement) {
			StatusEffect->BattleElement->BaseOffset = Offset;
			StatusEffect->BattleElement->CalculateBounds();
			StatusEffect->Render(StatusEffect->BattleElement, GlobalColor);
			Offset.x += StatusEffect->Buff->Texture->Size.x + 2;
		}
	}
}

// Serialize attributes for saving
void _Object::SerializeSaveData(Json::Value &Data) const {

	// Write stats
	Json::Value StatsNode;
	StatsNode["Hardcore"] = Character->Hardcore;
	StatsNode["MapX"] = Position.x;
	StatsNode["MapY"] = Position.y;
	StatsNode["SpawnMapID"] = Character->SpawnMapID;
	StatsNode["SpawnPoint"] = Character->SpawnPoint;
	StatsNode["BuildID"] = Character->BuildID;
	StatsNode["PortraitID"] = Character->PortraitID;
	StatsNode["ModelID"] = ModelID;
	StatsNode["BeltSize"] = Character->BeltSize;
	StatsNode["SkillBarSize"] = Character->SkillBarSize;
	StatsNode["SkillPointsUnlocked"] = Character->SkillPointsUnlocked;
	StatsNode["NextBattle"] = Character->NextBattle;
	StatsNode["Seed"] = Character->Seed;
	StatsNode["PartyName"] = Character->PartyName;

	// Save attributes
	for(const auto &Attribute : Stats->Attributes) {
		if(!Attribute.second.Save)
			continue;

		const _Value &AttributeStorage = Character->Attributes.at(Attribute.second.Name);
		switch(Attribute.second.Type) {
			case StatValueType::BOOLEAN:
			case StatValueType::INTEGER:
			case StatValueType::PERCENT:
				StatsNode[Attribute.second.Name] = AttributeStorage.Int;
			break;
			case StatValueType::INTEGER64:
				StatsNode[Attribute.second.Name] = (Json::Value::Int64)AttributeStorage.Int64;
			break;
			case StatValueType::FLOAT:
				StatsNode[Attribute.second.Name] = AttributeStorage.Float;
			break;
			case StatValueType::TIME:
				StatsNode[Attribute.second.Name] = AttributeStorage.Double;
			break;
			default:
				throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " Unsupported save type: " + Attribute.second.Name);
			break;
		}
	}
	Data["stats"] = StatsNode;

	// Write items
	Json::Value ItemsNode;
	for(auto &Bag : Inventory->GetBags()) {
		if(Bag.Type == BagType::NONE)
			continue;

		// Write bag contents
		Json::Value BagNode;
		for(size_t i = 0; i < Bag.Slots.size(); i++) {
			const _InventorySlot &InventorySlot = Bag.Slots[i];
			if(InventorySlot.Item) {
				Json::Value ItemNode;
				ItemNode["slot"] = (Json::Value::UInt64)i;
				ItemNode["id"] = InventorySlot.Item->ID;
				ItemNode["upgrades"] = InventorySlot.Upgrades;
				ItemNode["count"] = InventorySlot.Count;
				BagNode.append(ItemNode);
			}
		}
		ItemsNode[std::to_string((int)Bag.Type)] = BagNode;
	}
	Data["items"] = ItemsNode;

	// Write skills
	Json::Value SkillsNode;
	for(auto &Skill : Character->Skills) {
		Json::Value SkillNode;
		SkillNode["id"] = Skill.first;
		SkillNode["level"] = Skill.second;
		SkillsNode.append(SkillNode);
	}
	Data["skills"] = SkillsNode;

	// Write max skill levels
	Json::Value MaxSkillLevelsNode;
	for(auto &MaxSkillLevel : Character->MaxSkillLevels) {
		Json::Value MaxSkillLevelNode;
		MaxSkillLevelNode["id"] = MaxSkillLevel.first;
		MaxSkillLevelNode["level"] = MaxSkillLevel.second;
		MaxSkillLevelsNode.append(MaxSkillLevelNode);
	}
	Data["max_skill_levels"] = MaxSkillLevelsNode;

	// Write action bar
	Json::Value ActionBarNode;
	for(size_t i = 0; i < Character->ActionBar.size(); i++) {
		if(Character->ActionBar[i].IsSet()) {
			Json::Value ActionNode;
			ActionNode["slot"] = (Json::Value::UInt64)i;
			ActionNode["id"] = Character->ActionBar[i].Item ? Character->ActionBar[i].Item->ID : 0;
			ActionBarNode.append(ActionNode);
		}
	}
	Data["actionbar"] = ActionBarNode;

	// Write status effects
	Json::Value StatusEffectsNode;
	for(auto &StatusEffect : Character->StatusEffects) {
		Json::Value StatusEffectNode;
		StatusEffectNode["id"] = StatusEffect->Buff->ID;
		StatusEffectNode["level"] = StatusEffect->Level;
		if(StatusEffect->Infinite) {
			StatusEffectNode["infinite"] = StatusEffect->Infinite;
		}
		else {
			StatusEffectNode["duration"] = StatusEffect->Duration;
			StatusEffectNode["maxduration"] = StatusEffect->MaxDuration;
		}
		StatusEffectsNode.append(StatusEffectNode);
	}
	Data["statuseffects"] = StatusEffectsNode;

	// Write unlocks
	Json::Value UnlocksNode;
	for(auto &Unlock : Character->Unlocks) {
		Json::Value UnlockNode;
		UnlockNode["id"] = Unlock.first;
		UnlockNode["level"] = Unlock.second.Level;
		UnlocksNode.append(UnlockNode);
	}
	Data["unlocks"] = UnlocksNode;

	// Write cooldowns
	Json::Value CooldownsNode;
	for(auto &Cooldown : Character->BattleCooldown) {
		Json::Value CooldownNode;
		CooldownNode["id"] = Cooldown.first;
		CooldownNode["duration"] = Cooldown.second;
		CooldownsNode.append(CooldownNode);
	}
	Data["cooldowns"] = CooldownsNode;

	// Write boss kills
	Json::Value BossKillsNode;
	for(auto &BossKill : Character->BossKills) {
		Json::Value BossKillNode;
		BossKillNode["id"] = BossKill.first;
		BossKillNode["count"] = BossKill.second;
		BossKillsNode.append(BossKillNode);
	}
	Data["bosskills"] = BossKillsNode;
}

// Unserialize attributes from string
void _Object::UnserializeSaveData(const std::string &JsonString) {

	// Parse JSON
	Json::CharReaderBuilder Reader;
	Json::Value Data;
	std::istringstream Stream(JsonString);
	std::string Errors;
	if(!Json::parseFromStream(Reader, Stream, &Data, &Errors))
		throw std::runtime_error("_Object::UnserializeSaveData: " + Errors);

	// Get stats
	Json::Value StatsNode = Data["stats"];
	Character->LoadMapID = (ae::NetworkIDType)StatsNode["MapID"].asUInt();
	Position.x = StatsNode["MapX"].asInt();
	Position.y = StatsNode["MapY"].asInt();
	Character->SpawnMapID = (ae::NetworkIDType)StatsNode["SpawnMapID"].asUInt();
	Character->SpawnPoint = StatsNode["SpawnPoint"].asUInt();
	Character->Hardcore = StatsNode["Hardcore"].asBool();
	Character->BuildID = StatsNode["BuildID"].asUInt();
	Character->PortraitID = StatsNode["PortraitID"].asUInt();
	ModelID = StatsNode["ModelID"].asUInt();
	Character->BeltSize = StatsNode["BeltSize"].asInt();
	Character->SkillBarSize = StatsNode["SkillBarSize"].asInt();
	Character->SkillPointsUnlocked = StatsNode["SkillPointsUnlocked"].asInt();
	Character->NextBattle = StatsNode["NextBattle"].asInt();
	Character->Seed = StatsNode["Seed"].asUInt();
	Character->PartyName = StatsNode["PartyName"].asString();

	// Load attributes
	for(const auto &Attribute : Stats->Attributes) {
		if(!Attribute.second.Save)
			continue;

		switch(Attribute.second.Type) {
			case StatValueType::BOOLEAN:
				Character->Attributes[Attribute.second.Name].Int = StatsNode[Attribute.second.Name].asBool();
			break;
			case StatValueType::INTEGER:
			case StatValueType::PERCENT:
				Character->Attributes[Attribute.second.Name].Int = StatsNode[Attribute.second.Name].asInt();
			break;
			case StatValueType::INTEGER64:
				Character->Attributes[Attribute.second.Name].Int64 = StatsNode[Attribute.second.Name].asInt64();
			break;
			case StatValueType::FLOAT:
				Character->Attributes[Attribute.second.Name].Float = StatsNode[Attribute.second.Name].asFloat();
			break;
			case StatValueType::TIME:
				Character->Attributes[Attribute.second.Name].Double = StatsNode[Attribute.second.Name].asDouble();
			break;
			default:
				throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " Unsupported save type: " + Attribute.second.Name);
			break;
		}
	}

	if(!Character->BeltSize)
		Character->BeltSize = ACTIONBAR_DEFAULT_BELTSIZE;

	if(!Character->SkillBarSize)
		Character->SkillBarSize = ACTIONBAR_DEFAULT_SKILLBARSIZE;

	if(!Character->Seed)
		Character->Seed = ae::GetRandomInt((uint32_t)1, std::numeric_limits<uint32_t>::max());

	if(!Character->BuildID)
		Character->BuildID = 1;

	// Set items
	for(Json::ValueIterator BagNode = Data["items"].begin(); BagNode != Data["items"].end(); BagNode++) {
		for(const Json::Value &ItemNode : *BagNode) {
			_InventorySlot InventorySlot;
			InventorySlot.Item = Stats->Items.at(ItemNode["id"].asUInt());
			InventorySlot.Upgrades = ItemNode["upgrades"].asInt();
			InventorySlot.Count = ItemNode["count"].asInt();
			BagType Bag = (BagType)std::stoul(BagNode.name());
			if(Inventory->GetBag(Bag).StaticSize)
				Inventory->GetBag(Bag).Slots[ItemNode["slot"].asUInt64()] = InventorySlot;
			else
				Inventory->GetBag(Bag).Slots.push_back(InventorySlot);
		}
	}

	// Set skills
	for(const Json::Value &SkillNode : Data["skills"]) {
		uint32_t ItemID = SkillNode["id"].asUInt();
		Character->Skills[ItemID] = std::min(SkillNode["level"].asInt(), Stats->Items.at(ItemID)->MaxLevel);
	}

	// Set max skill levels
	for(const Json::Value &MaxSkillLevelNode : Data["max_skill_levels"]) {
		uint32_t ItemID = MaxSkillLevelNode["id"].asUInt();
		Character->MaxSkillLevels[ItemID] = std::min(MaxSkillLevelNode["level"].asInt(), Stats->Items.at(ItemID)->MaxLevel);
	}

	// Set actionbar
	for(const Json::Value &ActionNode : Data["actionbar"]) {
		uint32_t Slot = ActionNode["slot"].asUInt();
		if(Slot < Character->ActionBar.size()) {
			if(Slot < ACTIONBAR_MAX_SKILLBARSIZE && Slot >= (uint32_t)Character->SkillBarSize)
				continue;

			if(Slot >= ACTIONBAR_BELT_STARTS && Slot >= (uint32_t)(Character->BeltSize + ACTIONBAR_BELT_STARTS))
				continue;

			const _Item *Item = Stats->Items.at(ActionNode["id"].asUInt());
			if(Item->IsSkill() && Slot >= ACTIONBAR_MAX_SKILLBARSIZE)
				continue;

			if(!Item->IsSkill() && Slot < ACTIONBAR_BELT_STARTS)
				continue;

			Character->ActionBar[Slot].Item = Item;
			Character->ActionBar[Slot].ActionBarSlot = Slot;
		}
	}

	// Set status effects
	for(const Json::Value &StatusEffectNode : Data["statuseffects"]) {
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Buff = Stats->Buffs.at(StatusEffectNode["id"].asUInt());
		StatusEffect->Level = StatusEffectNode["level"].asInt();
		StatusEffect->Infinite = StatusEffectNode["infinite"].asBool();
		if(!StatusEffect->Infinite) {
			StatusEffect->Duration = StatusEffectNode["duration"].asDouble();
			StatusEffect->MaxDuration = StatusEffectNode["maxduration"].asDouble();
			StatusEffect->Time = 1.0 - (StatusEffect->Duration - (int)StatusEffect->Duration);
		}
		Character->StatusEffects.push_back(StatusEffect);
	}

	// Set unlocks
	for(const Json::Value &UnlockNode : Data["unlocks"])
		Character->Unlocks[UnlockNode["id"].asUInt()].Level = UnlockNode["level"].asInt();

	// Set cooldowns
	for(const Json::Value &CooldownNode : Data["cooldowns"])
		Character->BattleCooldown[CooldownNode["id"].asUInt()] = CooldownNode["duration"].asDouble();

	// Set boss kills
	for(const Json::Value &BossKillNode: Data["bosskills"])
		Character->BossKills[BossKillNode["id"].asUInt()] = BossKillNode["count"].asInt();
}

// Serialize for ObjectCreate
void _Object::SerializeCreate(ae::_Buffer &Data) {
	Data.Write<ae::NetworkIDType>(NetworkID);
	Data.Write<glm::ivec2>(Position);
	Data.WriteString(Name.c_str());
	Data.WriteString(Character->PartyName.c_str());
	if(Character)
		Data.Write<uint32_t>(Character->PortraitID);
	else
		Data.Write<uint32_t>(0);
	Data.Write<uint32_t>(ModelID);
	Data.Write<int>(Character->Attributes["Rebirths"].Int);
	Data.Write<uint8_t>(Light);
	Data.WriteBit(Character->Invisible);
	Data.WriteBit(Character->Offline);
}

// Unserialize for ObjectCreate
void _Object::UnserializeCreate(ae::_Buffer &Data) {
	Position = Data.Read<glm::ivec2>();
	Name = Data.ReadString();
	Character->PartyName = Data.ReadString();
	uint32_t PortraitID = Data.Read<uint32_t>();
	if(PortraitID && Character)
		Character->PortraitID = PortraitID;
	ModelID = Data.Read<uint32_t>();
	Character->Attributes["Rebirths"].Int = Data.Read<int>();
	Light = Data.Read<uint8_t>();
	Character->Invisible = Data.ReadBit();
	Character->Offline = Data.ReadBit();

	Character->Portrait = Stats->GetPortraitImage(Character->PortraitID);
	ModelTexture = Stats->Models.at(ModelID).Texture;
}

// Serialize for ObjectUpdate
void _Object::SerializeUpdate(ae::_Buffer &Data) {
	Data.Write<ae::NetworkIDType>(NetworkID);
	Data.Write<uint8_t>(Position.x);
	Data.Write<uint8_t>(Position.y);
	Data.Write<uint8_t>(Character->Status);
	Data.WriteBit(Light);
	Data.WriteBit(Character->Invisible);
	Data.WriteBit(Character->Attributes["Bounty"].Int);
	if(Character->Attributes["Bounty"].Int)
		Data.Write<int>(Character->Attributes["Bounty"].Int);
	if(Light)
		Data.Write<uint8_t>(Light);
}

// Serialize object stats
void _Object::SerializeStats(ae::_Buffer &Data) {
	Data.WriteString(Name.c_str());
	Data.Write<uint8_t>(Light);
	Data.Write<uint32_t>(ModelID);
	Data.Write<uint32_t>(Character->PortraitID);
	Data.WriteString(Character->PartyName.c_str());
	Data.Write<uint8_t>(Character->BeltSize);
	Data.Write<uint8_t>(Character->SkillBarSize);
	Data.Write<int>(Character->SkillPointsUnlocked);
	Data.Write<int>(Character->Invisible);
	Data.Write<int>(Character->Hardcore);

	// Serialize attributes
	for(const auto &AttributeName : Stats->AttributeRank) {
		const _Attribute &Attribute = Stats->Attributes.at(AttributeName);
		if(!Attribute.Network)
			continue;

		_Value &AttributeStorage = Character->Attributes[AttributeName];
		switch(Attribute.Type) {
			case StatValueType::BOOLEAN:
			case StatValueType::INTEGER:
			case StatValueType::PERCENT:
				Data.Write<int>(AttributeStorage.Int);
			break;
			case StatValueType::INTEGER64:
				Data.Write<int64_t>(AttributeStorage.Int64);
			break;
			case StatValueType::FLOAT:
				Data.Write<float>(AttributeStorage.Float);
			break;
			case StatValueType::TIME:
				Data.Write<float>(AttributeStorage.Double);
			break;
			default:
				throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " Unsupported network type: " + Attribute.Name);
			break;
		}
	}

	// Write inventory
	Inventory->Serialize(Data);

	// Write skills
	Data.Write<uint32_t>((uint32_t)Character->Skills.size());
	for(const auto &Skill : Character->Skills) {
		Data.Write<uint32_t>(Skill.first);
		Data.Write<int>(Skill.second);
	}

	// Write max skill levels
	Data.Write<uint32_t>((uint32_t)Character->MaxSkillLevels.size());
	for(const auto &Skill : Character->MaxSkillLevels) {
		Data.Write<uint32_t>(Skill.first);
		Data.Write<int>(Skill.second);
	}

	// Write action bar
	for(size_t i = 0; i < ACTIONBAR_MAX_SIZE; i++)
		Character->ActionBar[i].Serialize(Data);

	// Write unlocks
	Data.Write<uint32_t>((uint32_t)Character->Unlocks.size());
	for(const auto &Unlock : Character->Unlocks) {
		Data.Write<uint32_t>(Unlock.first);
		Data.Write<int>(Unlock.second.Level);
	}

	// Write cooldowns
	Data.Write<uint32_t>((uint32_t)Character->Cooldowns.size());
	for(const auto &Cooldown : Character->Cooldowns) {
		Data.Write<uint32_t>(Cooldown.first);
		Data.Write<float>(Cooldown.second.Duration);
		Data.Write<float>(Cooldown.second.MaxDuration);
	}

	// Write status effects
	Data.Write<uint8_t>((uint8_t)Character->StatusEffects.size());
	for(const auto &StatusEffect : Character->StatusEffects) {
		StatusEffect->Serialize(Data);
	}
}

// Unserialize object stats
void _Object::UnserializeStats(ae::_Buffer &Data) {
	Name = Data.ReadString();
	Light = Data.Read<uint8_t>();
	ModelID = Data.Read<uint32_t>();
	Character->PortraitID = Data.Read<uint32_t>();
	Character->PartyName = Data.ReadString();
	Character->BeltSize = Data.Read<uint8_t>();
	Character->SkillBarSize = Data.Read<uint8_t>();
	Character->SkillPointsUnlocked = Data.Read<int>();
	Character->Invisible = Data.Read<int>();
	Character->Hardcore = Data.Read<int>();

	// Serialize attributes
	for(const auto &AttributeName : Stats->AttributeRank) {
		const _Attribute &Attribute = Stats->Attributes.at(AttributeName);
		if(!Attribute.Network)
			continue;

		_Value &AttributeStorage = Character->Attributes[AttributeName];
		switch(Attribute.Type) {
			case StatValueType::BOOLEAN:
			case StatValueType::INTEGER:
			case StatValueType::PERCENT:
				AttributeStorage.Int = Data.Read<int>();
			break;
			case StatValueType::INTEGER64:
				AttributeStorage.Int64 = Data.Read<int64_t>();
			break;
			case StatValueType::FLOAT:
				AttributeStorage.Float = Data.Read<float>();
			break;
			case StatValueType::TIME:
				AttributeStorage.Double = Data.Read<float>();
			break;
			default:
				throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " Unsupported network type: " + Attribute.Name);
			break;
		}
	}

	ModelTexture = Stats->Models.at(ModelID).Texture;

	// Read inventory
	Inventory->Unserialize(Data, Stats);

	// Read skills
	Character->Skills.clear();
	uint32_t SkillCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < SkillCount; i++) {
		uint32_t SkillID = Data.Read<uint32_t>();
		int Points = Data.Read<int>();
		Character->Skills[SkillID] = Points;
	}

	// Read max skill levels
	Character->MaxSkillLevels.clear();
	uint32_t MaxSkillLevelCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < MaxSkillLevelCount; i++) {
		uint32_t SkillID = Data.Read<uint32_t>();
		int Level = Data.Read<int>();
		Character->MaxSkillLevels[SkillID] = Level;
	}

	// Read action bar
	for(size_t i = 0; i < ACTIONBAR_MAX_SIZE; i++)
		Character->ActionBar[i].Unserialize(Data, Stats);

	// Read unlocks
	Character->Unlocks.clear();
	uint32_t UnlockCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < UnlockCount; i++) {
		uint32_t UnlockID = Data.Read<uint32_t>();
		int Level = Data.Read<int>();
		Character->Unlocks[UnlockID].Level = Level;
	}

	// Read cooldowns
	Character->Cooldowns.clear();
	uint32_t CooldownCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < CooldownCount; i++) {
		uint32_t CooldownID = Data.Read<uint32_t>();
		Character->Cooldowns[CooldownID].Duration = Data.Read<float>();
		Character->Cooldowns[CooldownID].MaxDuration = Data.Read<float>();
	}

	// Read status effects
	Character->DeleteStatusEffects();
	size_t StatusEffectsSize = Data.Read<uint8_t>();
	for(size_t i = 0; i < StatusEffectsSize; i++) {
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Unserialize(Data, Stats);
		if(Character->HUD)
			StatusEffect->HUDElement = StatusEffect->CreateUIElement(ae::Assets.Elements["element_hud_statuseffects"]);
		Character->StatusEffects.push_back(StatusEffect);
	}

	Character->RefreshActionBarCount();
	Character->CalculateStats();
}

// Serialize object for battle
void _Object::SerializeBattle(ae::_Buffer &Data) {
	Data.Write<ae::NetworkIDType>(NetworkID);
	Data.Write<uint32_t>(Monster->DatabaseID);
	Data.Write<glm::ivec2>(Position);
	Data.Write<int>(Character->Level);
	Data.Write<int>(Character->Attributes["Health"].Int);
	Data.Write<int>(Character->Attributes["MaxHealth"].Int);
	Data.Write<int>(Character->Attributes["Mana"].Int);
	Data.Write<int>(Character->Attributes["MaxMana"].Int);
	Data.Write<int>(Character->EquipmentBattleSpeed);
	Data.Write<double>(Fighter->TurnTimer);
	Data.Write<uint8_t>(Fighter->BattleSide);

	SerializeStatusEffects(Data);
}

// Unserialize battle stats
void _Object::UnserializeBattle(ae::_Buffer &Data, bool IsClient) {
	Controller->InputStates.clear();

	// Get object type
	Position = ServerPosition = Data.Read<glm::ivec2>();
	Character->Level = Data.Read<int>();
	Character->Attributes["Health"].Int = Data.Read<int>();
	Character->BaseMaxHealth = Character->Attributes["MaxHealth"].Int = Data.Read<int>();
	Character->Attributes["Mana"].Int = Data.Read<int>();
	Character->BaseMaxMana = Character->Attributes["MaxMana"].Int = Data.Read<int>();
	Character->EquipmentBattleSpeed = Data.Read<int>();
	if(!IsClient)
		Character->BaseBattleSpeed = Character->EquipmentBattleSpeed;
	Fighter->TurnTimer = Data.Read<double>();
	Fighter->BattleSide = Data.Read<uint8_t>();

	UnserializeStatusEffects(Data);
}

// Serialize status effects
void _Object::SerializeStatusEffects(ae::_Buffer &Data) {
	Data.Write<uint8_t>((uint8_t)Character->StatusEffects.size());
	for(auto &StatusEffect : Character->StatusEffects)
		StatusEffect->Serialize(Data);
}

// Unserialize status effects
void _Object::UnserializeStatusEffects(ae::_Buffer &Data) {
	if(!Character)
		return;

	Character->DeleteStatusEffects();
	int StatusEffectCount = Data.Read<uint8_t>();
	for(int i = 0; i < StatusEffectCount; i++) {
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Unserialize(Data, Stats);
		Character->StatusEffects.push_back(StatusEffect);
	}
}

// Update stats
_StatusEffect *_Object::UpdateStats(_StatChange &StatChange, _Object *Source) {

	// Rebirth
	if(Server) {
		if(StatChange.HasStat("Rebirth")) {
			if(StatChange.HasStat("MaxDamage"))
				Server->QueueRebirth(this, 1, StatChange.Values["MaxDamage"].Int);
			else if(StatChange.HasStat("Armor"))
				Server->QueueRebirth(this, 2, StatChange.Values["Armor"].Int);
			else if(StatChange.HasStat("Health"))
				Server->QueueRebirth(this, 3, StatChange.Values["Health"].Int);
			else if(StatChange.HasStat("Mana"))
				Server->QueueRebirth(this, 4, StatChange.Values["Mana"].Int);
			else if(StatChange.HasStat("Experience"))
				Server->QueueRebirth(this, 5, StatChange.Values["Experience"].Int);
			else if(StatChange.HasStat("Gold"))
				Server->QueueRebirth(this, 6, StatChange.Values["Gold"].Int);
			else if(StatChange.HasStat("BattleSpeed"))
				Server->QueueRebirth(this, 7, StatChange.Values["BattleSpeed"].Int);
			else if(StatChange.HasStat("SkillPoint"))
				Server->QueueRebirth(this, 8, StatChange.Values["SkillPoint"].Int);
			else if(StatChange.HasStat("Difficulty"))
				Server->QueueRebirth(this, 9, StatChange.Values["Difficulty"].Int);

			return nullptr;
		}
	}

	_StatusEffect *StatusEffect = nullptr;

	// Add buffs
	if(StatChange.HasStat("Buff")) {
		StatusEffect = new _StatusEffect();
		StatusEffect->Buff = (const _Buff *)StatChange.Values["Buff"].Pointer;
		StatusEffect->Level = StatChange.Values["BuffLevel"].Int;
		StatusEffect->MaxDuration = StatusEffect->Duration = StatChange.Values["BuffDuration"].Float;
		StatusEffect->Source = Source;
		if(StatusEffect->Duration < 0.0) {
			StatusEffect->Infinite = true;
			StatusEffect->Duration = 0.0;
		}

		if(Character->AddStatusEffect(StatusEffect)) {
			if(Fighter->BattleElement)
				StatusEffect->BattleElement = StatusEffect->CreateUIElement(Fighter->BattleElement);
		}
		else {
			delete StatusEffect;
			StatusEffect = nullptr;
		}

		Character->CalculateStats();
	}

	// Clear buff
	if(Server && StatChange.HasStat("ClearBuff")) {
		_Buff *ClearBuff = (_Buff *)StatChange.Values["ClearBuff"].Pointer;

		// Find existing buff
		for(auto &ExistingEffect : Character->StatusEffects) {
			if(ExistingEffect->Buff == ClearBuff) {
				ExistingEffect->Deleted = true;
				Server->UpdateBuff(this, ExistingEffect);
			}
		}
	}

	// Update gold
	if(StatChange.HasStat("Gold")) {
		int GoldUpdate = StatChange.Values["Gold"].Int;
		if(Character->Battle && GoldUpdate < 0 && Fighter->GoldStolen) {
			Fighter->GoldStolen += GoldUpdate;
			if(Fighter->GoldStolen < 0)
				Fighter->GoldStolen = 0;
		}

		Character->UpdateGold(GoldUpdate);
	}

	// Update gold stolen
	if(StatChange.HasStat("GoldStolen")) {
		int Amount = StatChange.Values["GoldStolen"].Int;
		Fighter->GoldStolen += Amount;
		if(Fighter->GoldStolen > PLAYER_MAX_GOLD)
			Fighter->GoldStolen = PLAYER_MAX_GOLD;

		Character->UpdateGold(Amount);
	}

	// Update experience
	if(StatChange.HasStat("Experience")) {
		Character->UpdateExperience(StatChange.Values["Experience"].Int);
	}

	// Update health
	bool WasAlive = Character->IsAlive();
	if(StatChange.HasStat("Health"))
		Character->UpdateHealth(StatChange.Values["Health"].Int);

	// Just died
	if(WasAlive && !Character->IsAlive()) {
		Fighter->PotentialAction.Unset();
		Character->Action.Unset();
		Character->ResetUIState();

		// If not in battle apply penalty immediately
		if(!Character->Battle) {

			// Apply penalty
			ApplyDeathPenalty(false, PLAYER_DEATH_GOLD_PENALTY, 0);
		}
	}

	// Just revived
	if(Server && !WasAlive && Character->IsAlive()) {
		Server->SendHUD(Peer);
	}

	// Mana change
	if(StatChange.HasStat("Mana"))
		Character->UpdateMana(StatChange.Values["Mana"].Int);

	// Stamina change
	if(StatChange.HasStat("Stamina")) {
		Fighter->TurnTimer += StatChange.Values["Stamina"].Float;
		Fighter->TurnTimer = glm::clamp(Fighter->TurnTimer, 0.0, 1.0);
	}

	// Skill bar upgrade
	if(StatChange.HasStat("SkillBarSize")) {
		Character->SkillBarSize += StatChange.Values["SkillBarSize"].Int;
		if(Character->SkillBarSize >= ACTIONBAR_MAX_SKILLBARSIZE)
			Character->SkillBarSize = ACTIONBAR_MAX_SKILLBARSIZE;
	}

	// Belt size upgrade
	if(StatChange.HasStat("BeltSize")) {
		Character->BeltSize += StatChange.Values["BeltSize"].Int;
		if(Character->BeltSize >= ACTIONBAR_MAX_BELTSIZE)
			Character->BeltSize = ACTIONBAR_MAX_BELTSIZE;
	}

	// Skill point unlocked
	if(StatChange.HasStat("SkillPoint")) {
		Character->SkillPointsUnlocked += StatChange.Values["SkillPoint"].Int;
		Character->CalculateStats();
	}

	// Boss cooldowns
	if(StatChange.HasStat("BossCooldowns")) {
		for(auto &BattleCooldown : Character->BattleCooldown)
			BattleCooldown.second *= 1.0 - StatChange.Values["BossCooldowns"].Mult();
	}

	// Rebirth bonus
	if(StatChange.HasStat("RebirthWealth"))
		Character->Attributes["RebirthWealth"].Int += StatChange.Values["RebirthWealth"].Int;
	if(StatChange.HasStat("RebirthWisdom"))
		Character->Attributes["RebirthWisdom"].Int += StatChange.Values["RebirthWisdom"].Int;
	if(StatChange.HasStat("RebirthKnowledge"))
		Character->Attributes["RebirthKnowledge"].Int += StatChange.Values["RebirthKnowledge"].Int;
	if(StatChange.HasStat("RebirthGirth"))
		Character->Attributes["RebirthGirth"].Int += StatChange.Values["RebirthGirth"].Int;
	if(StatChange.HasStat("RebirthProficiency"))
		Character->Attributes["RebirthProficiency"].Int += StatChange.Values["RebirthProficiency"].Int;
	if(StatChange.HasStat("RebirthInsight"))
		Character->Attributes["RebirthInsight"].Int += StatChange.Values["RebirthInsight"].Int;
	if(StatChange.HasStat("RebirthPassage"))
		Character->Attributes["RebirthPassage"].Int += StatChange.Values["RebirthPassage"].Int;
	if(StatChange.HasStat("RebirthEnchantment"))
		Character->Attributes["RebirthEnchantment"].Int += StatChange.Values["RebirthEnchantment"].Int;
	if(StatChange.HasStat("RebirthPower")) {
		Character->Attributes["RebirthPower"].Int += StatChange.Values["RebirthPower"].Int;
		Character->CalculateStats();
	}

	// Reset skills
	if(StatChange.HasStat("Respec")) {
		for(const auto &SkillLevel : Character->Skills) {
			const _Item *Skill = Stats->Items.at(SkillLevel.first);
			if(Skill && SkillLevel.second > 0) {

				// Set action bar skill to 1
				bool SetToZero = true;
				for(int i = 0; i < Character->SkillBarSize; i++) {
					if(Character->ActionBar[i].Item == Skill) {
						Character->Skills[SkillLevel.first] = 1;
						SetToZero = false;
						break;
					}
				}

				if(SetToZero)
					Character->Skills[SkillLevel.first] = 0;
			}
		}

		Character->CalculateStats();
	}

	// Flee from battle
	if(StatChange.HasStat("Flee")) {
		if(Fighter)
			Fighter->FleeBattle = true;
	}

	// Use corpse
	if(StatChange.HasStat("Corpse")) {
		Fighter->Corpse += StatChange.Values["Corpse"].Int;
		if(Fighter->Corpse < 0)
			Fighter->Corpse = 0;
		else if(Fighter->Corpse > 1)
			Fighter->Corpse = 1;
	}

	// Run server only commands
	if(Server) {
		if(!Character->Battle) {

			// Start teleport
			if(StatChange.HasStat("Teleport"))
				Server->StartTeleport(this, StatChange.Values["Teleport"].Float);

			// Start battle
			if(StatChange.HasStat("Battle"))
				Server->QueueBattle(this, (uint32_t)StatChange.Values["Battle"].Int, true, false, 0.0f, 0.0f);

			// Start PVP
			if(StatChange.HasStat("Hunt"))
				Server->QueueBattle(this, 0, false, true, StatChange.Values["Hunt"].Float, 0.0f);
			if(StatChange.HasStat("BountyHunt"))
				Server->QueueBattle(this, 0, false, true, 0.0f, StatChange.Values["BountyHunt"].Float);
		}

		// Set clock
		if(StatChange.HasStat("Clock"))
			Server->SetClock(StatChange.Values["Clock"].Float);

		// Map Change
		if(StatChange.HasStat("MapChange"))
			QueuedMapChange = StatChange.Values["MapChange"].Int;
	}

	return StatusEffect;
}

// Moves the player and returns direction moved
int _Object::Move() {
	if(Controller->WaitForServer || Character->Battle || Controller->InputStates.size() == 0 || !Character->IsAlive())
		return 0;

	// Check timer
	if(Controller->MoveTime < PLAYER_MOVETIME / (Character->Attributes["MoveSpeed"].Mult()))
		return 0;

	Controller->MoveTime = 0;
	int InputState = Controller->InputStates.front();
	Controller->InputStates.pop_front();

	// Get new position
	glm::ivec2 Direction(0, 0);
	GetDirectionFromInput(InputState, Direction);

	// Test movement
	bool Moved = false;
	if(!Map->CanMoveTo(Position + Direction, this)) {

		// Check for moving diagonally
		if(Direction.x != 0 && Direction.y != 0) {

			// Try moving horizontally
			glm::ivec2 TestDirection(Direction.x, 0);
			if(Map->CanMoveTo(Position + TestDirection, this)) {
				Direction = TestDirection;
				Moved = true;
			}
			else {

				// Try moving vertically
				TestDirection.x = 0;
				TestDirection.y = Direction.y;
				if(Map->CanMoveTo(Position + TestDirection, this)) {
					Direction = TestDirection;
					Moved = true;
				}
			}
		}
	}
	else
		Moved = true;

	// Move player
	if(Moved) {
		Position += Direction;
		if(GetTile()->Zone > 0 && !Character->Invisible)
			Character->NextBattle--;

		return InputState;
	}

	return 0;
}

// Return true if the object can respec
bool _Object::CanRespec() const {
	if(Map && Map->IsValidPosition(Position) && GetTile()->Event.Type == _Map::EVENT_SPAWN)
		return true;

	return false;
}

// Gets the tile that the player is currently standing on
const _Tile *_Object::GetTile() const {

	return Map->GetTile(Position);
}

// Get map id, return 0 if none
ae::NetworkIDType _Object::GetMapID() const {
	if(!Map)
		return 0;

	return Map->NetworkID;
}

// Stop a battle
void _Object::StopBattle() {
	Character->Battle = nullptr;
	Fighter->RemoveBattleElement();
}

// Call update function for buff
void _Object::ResolveBuff(_StatusEffect *StatusEffect, const std::string &Function) {
	if(!Server)
		return;

	// Call function
	_StatChange StatChange;
	StatChange.Object = this;
	StatusEffect->Buff->ExecuteScript(Scripting, Function, StatusEffect->Level, StatChange);
	StatChange.Object->UpdateStats(StatChange);

	// Build packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::STAT_CHANGE);
	StatChange.Serialize(Packet);

	// Send packet to player
	SendPacket(Packet);
}

// Update death count and gold loss
void _Object::ApplyDeathPenalty(bool InBattle, float Penalty, int BountyLoss) {
	int GoldPenalty = BountyLoss + (int)(std::abs(Character->Attributes["Gold"].Int) * Penalty + 0.5f);
	int OldBounty = Character->Attributes["Bounty"].Int;

	// Update stats
	Character->UpdateGold(-GoldPenalty);
	Character->Attributes["Deaths"].Int++;
	Character->Attributes["GoldLost"].Int += GoldPenalty;
	Character->Attributes["Bounty"].Int -= BountyLoss;
	if(Character->Attributes["Bounty"].Int < 0)
		Character->Attributes["Bounty"].Int = 0;

	// Send message
	if(Server) {
		if(BountyLoss > 0 && Character->Attributes["Bounty"].Int == 0) {
			std::string BountyMessage = "Player " + Name + "'s bounty of " + std::to_string(OldBounty) + " gold has been claimed!";
			Server->BroadcastMessage(nullptr, BountyMessage, "cyan");
			Server->Log << "[BOUNTY] " << BountyMessage << std::endl;
		}

		if(Peer) {
			std::string Text = "You died ";
			if(InBattle)
				Text += "in battle ";

			Server->SendPlayerPosition(Peer);
			Server->SendMessage(Peer, std::string(Text + "and lost " + std::to_string(GoldPenalty) + " gold"), "red");
			Server->Log << "[DEATH] Player " << Name << " died and lost " << std::to_string(GoldPenalty) << " gold ( character_id=" << Character->CharacterID << " gold=" << Character->Attributes["Gold"].Int << " deaths=" << Character->Attributes["Deaths"].Int << " hardcore=" << Character->Hardcore << " )" << std::endl;
		}
	}
}

// Set action and targets, return false if no action set
bool _Object::SetActionUsing(ae::_Buffer &Data, ae::_Manager<_Object> *ObjectManager) {

	// Check for needed commands
	if(!Character->Action.IsSet()) {

		bool WasInBattle = Data.ReadBit();
		uint8_t ActionBarSlot = Data.Read<uint8_t>();
		int TargetCount = Data.Read<uint8_t>();
		if(!TargetCount)
			return false;

		// Check if action use is consistent to player state
		if(WasInBattle != !!Character->Battle)
			return false;

		// Get skillbar action
		if(!Character->GetActionFromActionBar(Character->Action, ActionBarSlot))
			return false;

		// Handle corpse aoe
		const _Item *Item = Character->Action.Item;
		if(Item->TargetID == TargetType::ENEMY_CORPSE_AOE) {
			Character->Targets.clear();
			if(TargetCount != 1)
				return false;

			// Get first target
			ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
			_Object *Target = ObjectManager->GetObject(NetworkID);
			if(!Target)
				return false;

			_Battle *Battle = Target->Character->Battle;
			if(!Battle)
				return false;

			// Set initial target
			if(Item->CanTarget(Scripting, this, Target))
				Character->Targets.push_back(Target);

			// Get alive target count
			int TargetCount = Item->GetTargetCount(Scripting, this, false);

			// Get list of objects on each side
			std::list<_Object *> ObjectList;
			for(auto &Object : Battle->Objects) {
				if(Object->Fighter->BattleSide == Target->Fighter->BattleSide)
					ObjectList.push_back(Object);
			}

			// Get iterator to last target
			_Object *LastTarget = Target;
			auto Iterator = ObjectList.begin();
			if(ObjectList.size())
			   Iterator = std::find(ObjectList.begin(), ObjectList.end(), LastTarget);

			// Set up alive targets
			for(size_t i = 0; i < ObjectList.size(); i++) {
				_Object *CheckObject = *Iterator;
				if(Item->CanTarget(Scripting, this, CheckObject, true)) {
					this->Character->Targets.push_back(CheckObject);

					TargetCount--;
					if(TargetCount <= 0)
						break;
				}
				++Iterator;
				if(Iterator == ObjectList.end())
					Iterator = ObjectList.begin();
			}
		}
		else {

			// Get targets
			Character->Targets.clear();
			for(int i = 0; i < TargetCount; i++) {
				ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
				_Object *Target = ObjectManager->GetObject(NetworkID);
				if(Target && Item->CanTarget(Scripting, this, Target))
					Character->Targets.push_back(Target);

				// Using resurrect
				if(!Item->TargetAlive && !Character->Battle && Map) {
					_Object *DeadPlayer = Map->FindDeadPlayer(this, 1.0f);
					if(DeadPlayer)
						Character->Targets.push_back(DeadPlayer);
				}
			}
		}
	}

	return true;
}

// Accept a trade from a trader
void _Object::AcceptTrader(std::vector<_Slot> &Slots) {
	if(!Character->Trader)
		return;

	// Trade in required items
	for(uint32_t i = 0; i < Character->Trader->Items.size(); i++)
		Inventory->UpdateItemCount(Slots[i], -Character->Trader->Items[i].Count);

	// Give player reward
	Inventory->AddItem(Character->Trader->RewardItem, Character->Trader->Upgrades, Character->Trader->Count);

	// Update player
	Character->CalculateStats();
}

// Generate and send seed to client
void _Object::SendSeed(bool Generate) {
	if(!Server)
		return;

	if(Generate)
		Character->Seed = ae::GetRandomInt((uint32_t)1, std::numeric_limits<uint32_t>::max());

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::MINIGAME_SEED);
	Packet.Write<uint32_t>(Character->Seed);
	Server->Network->SendPacket(Packet, Peer);
}

// Convert input state bitfield to direction
void _Object::GetDirectionFromInput(int InputState, glm::ivec2 &Direction) {

	// Get new position
	if(InputState & MOVE_UP)
		Direction.y += -1;
	if(InputState & MOVE_DOWN)
		Direction.y += 1;
	if(InputState & MOVE_LEFT)
		Direction.x += -1;
	if(InputState & MOVE_RIGHT)
		Direction.x += 1;

	if(Character->Attributes["DiagonalMovement"].Int)
		return;

	// Remove diagonols
	if(Direction.x != 0 && Direction.y != 0)
		Direction.x = 0;
}

// Send packet to player or broadcast during battle
void _Object::SendPacket(ae::_Buffer &Packet) {
	if(Character->Battle)
		Character->Battle->BroadcastPacket(Packet);
	else if(Peer)
		Server->Network->SendPacket(Packet, Peer);
}

// Send action clear packet to client
void _Object::SendActionClear() {
	if(!Server)
		return;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACTION_CLEAR);
	Packet.Write<ae::NetworkIDType>(NetworkID);
	SendPacket(Packet);
}

// Check if object is a monster
bool _Object::IsMonster() const {
	 return Monster->DatabaseID != 0;
}

// Create list of nodes to destination
bool _Object::Pathfind(const glm::ivec2 &StartPosition, const glm::ivec2 &EndPosition) {
	if(!Map || !Map->Pather)
		return false;

	if(Character->Path.size())
		return true;

	float TotalCost;
	std::vector<void *> PathFound;
	int Result = Map->Pather->Solve(Map->PositionToNode(StartPosition), Map->PositionToNode(EndPosition), &PathFound, &TotalCost);
	if(Result == micropather::MicroPather::SOLVED) {

		// Convert vector to list
		Character->Path.clear();
		for(auto &Node : PathFound)
			Character->Path.push_back(Node);

		return true;
	}

	return false;
}

// Return an input state from the next node in the path list
int _Object::GetInputStateFromPath() {
	int InputState = 0;

	// Find current position in list
	for(auto Iterator = Character->Path.begin(); Iterator != Character->Path.end(); ++Iterator) {
		glm::ivec2 NodePosition;
		Map->NodeToPosition(*Iterator, NodePosition);

		if(Position == NodePosition) {
			auto NextIterator = std::next(Iterator, 1);
			if(NextIterator == Character->Path.end()) {
				Character->Path.clear();
				return 0;
			}

			// Get next node position
			Map->NodeToPosition(*NextIterator, NodePosition);

			// Get direction to next node
			glm::ivec2 Direction = NodePosition - Position;
			if(Direction.x < 0)
				InputState = _Object::MOVE_LEFT;
			else if(Direction.x > 0)
				InputState = _Object::MOVE_RIGHT;
			else if(Direction.y < 0)
				InputState = _Object::MOVE_UP;
			else if(Direction.y > 0)
				InputState = _Object::MOVE_DOWN;

			return InputState;
		}
	}

	return 0;
}
