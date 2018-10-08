/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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

	Position(0, 0),
	ServerPosition(0, 0),

	ModelTexture(nullptr),
	ModelID(0),
	Light(0) {

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
			if(!Character->Stunned)
				Fighter->TurnTimer += FrameTime * (1.0 / Character->BaseAttackPeriod) * Character->BattleSpeed / 100.0;
		}
		else
			Fighter->TurnTimer = 1.0;

		// Resolve action
		if(Fighter->TurnTimer >= 1.0) {
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

					// Can't use action so send an action clear packet
					ae::_Buffer FailPacket;
					FailPacket.Write<PacketType>(PacketType::ACTION_CLEAR);
					FailPacket.Write<ae::NetworkIDType>(NetworkID);
					SendPacket(FailPacket);
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
		Character->PlayTime += FrameTime;
		if(Character->Battle)
			Character->BattleTime += FrameTime;
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
		Map->CheckEvents(this);

	// Update status
	if(Character)
		Character->UpdateStatus();
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
void _Object::Render(const _Object *ClientPlayer) {
	if(Map && ModelTexture) {

		// Setup shader
		ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
		glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

		// Draw debug server position
		glm::vec3 DrawPosition;
		if(Character->HUD && Character->HUD->ShowDebug) {
			DrawPosition = glm::vec3(ServerPosition, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
			ae::Graphics.SetColor(glm::vec4(1, 0, 0, 1));
			ae::Graphics.DrawSprite(DrawPosition, ModelTexture);
		}

		// Set invisible alpha
		float Alpha = 1.0f;
		if(Character->Invisible > 0)
			Alpha = PLAYER_INVIS_ALPHA;

		// Draw model
		DrawPosition = glm::vec3(Position, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
		ae::Graphics.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, Alpha));
		ae::Graphics.DrawSprite(DrawPosition, ModelTexture);
		if(Character->StatusTexture) {
			ae::Graphics.DrawSprite(DrawPosition, Character->StatusTexture);
		}

		// Draw name
		if(ClientPlayer != this && Character->Invisible != 1) {
			std::string NameText = Name;
			if(Character->Bounty > 0)
				NameText += " ([c cyan]" + std::to_string(Character->Bounty) + "[c white])";

			ae::Assets.Fonts["hud_medium"]->DrawTextFormatted(NameText, glm::vec2(DrawPosition) + glm::vec2(0, -0.5f), ae::CENTER_BASELINE, 1.0f / ModelTexture->Size.x);
		}
	}
}

// Renders the object during a battle
void _Object::RenderBattle(_Object *ClientPlayer, double Time) {

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
	Fighter->StatPosition = Fighter->ResultPosition + glm::vec2(Character->Portrait->Size.x/2 + 10 + BATTLE_HEALTHBAR_WIDTH/2, -Character->Portrait->Size.y/2);

	// Name
	ae::Assets.Fonts["hud_medium"]->DrawText(Name, SlotPosition + glm::vec2(0, -12), ae::LEFT_BASELINE, GlobalColor);

	// Portrait
	if(Character->Portrait) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawCenteredImage(SlotPosition + glm::vec2(Character->Portrait->Size/2), Character->Portrait, GlobalColor);
	}

	// Health/mana bars
	glm::vec2 BarSize(BATTLE_HEALTHBAR_WIDTH, BATTLE_HEALTHBAR_HEIGHT);
	glm::vec2 BarOffset(Fighter->BattleElement->Size.x + 10, 0);
	float BarPaddingY = 6;

	// Get ui size
	ae::_Bounds BarBounds;
	BarBounds.Start = SlotPosition + glm::vec2(0, 0) + BarOffset;
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x, BarSize.y) + BarOffset;
	glm::vec2 BarCenter = (BarBounds.Start + BarBounds.End) / 2.0f;
	float BarEndX = BarBounds.End.x;

	// Draw empty bar
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_health_bar_empty"]->Texture, true);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * Character->GetHealthPercent(), BarSize.y) + BarOffset;
	ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_health_bar_full"]->Texture, true);

	// Draw health text
	std::stringstream Buffer;
	Buffer << ae::Round(Character->Health) << " / " << ae::Round(Character->MaxHealth);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), BarCenter + glm::vec2(0, 5), ae::CENTER_BASELINE, GlobalColor);
	Buffer.str("");

	// Draw mana
	if(Character->MaxMana > 0) {
		float ManaPercent = Character->MaxMana > 0 ? Character->Mana / (float)Character->MaxMana : 0;

		// Get ui size
		BarOffset.y += BarSize.y + BarPaddingY;
		BarBounds.Start = SlotPosition + glm::vec2(0, 0) + BarOffset;
		BarBounds.End = SlotPosition + glm::vec2(BarSize.x, BarSize.y) + BarOffset;
		BarCenter = (BarBounds.Start + BarBounds.End) / 2.0f;

		// Draw empty bar
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_mana_bar_empty"]->Texture, true);

		// Draw full bar
		BarBounds.End = SlotPosition + glm::vec2(BarSize.x * ManaPercent, BarSize.y) + BarOffset;
		ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_mana_bar_full"]->Texture, true);

		// Draw mana text
		Buffer << ae::Round(Character->Mana) << " / " << ae::Round(Character->MaxMana);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), BarCenter + glm::vec2(0, 5), ae::CENTER_BASELINE, GlobalColor);
		Buffer.str("");
	}

	// Draw turn timer
	BarOffset.y += BarSize.y + BarPaddingY;
	BarSize.y = 8;
	BarBounds.Start = SlotPosition + glm::vec2(0, 0) + BarOffset;
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x, BarSize.y) + BarOffset;

	// Draw empty bar
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_experience_bar_empty"]->Texture, true);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * Fighter->TurnTimer, BarSize.y) + BarOffset;
	ae::Graphics.DrawImage(BarBounds, ae::Assets.Elements["image_hud_experience_bar_full"]->Texture, true);

	// Get background for items used
	const ae::_Texture *ItemBackTexture = ae::Assets.Textures["textures/hud/item_back.png"];

	// Draw the action used
	if(ClientPlayer->Fighter->BattleSide == Fighter->BattleSide) {

		if(Character->Action.Item) {
			glm::vec2 ItemUsingPosition = SlotPosition + glm::vec2(-ItemBackTexture->Size.x/2 - 10, Fighter->BattleElement->Size.y/2);
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			if(!Character->Action.Item->IsSkill())
				ae::Graphics.DrawCenteredImage(ItemUsingPosition, ItemBackTexture, GlobalColor);
			ae::Graphics.DrawCenteredImage(ItemUsingPosition, Character->Action.Item->Texture, GlobalColor);
		}
	}

	// Draw potential action to use
	for(auto &BattleTarget : ClientPlayer->Character->Targets) {
		if(BattleTarget == this && ClientPlayer->Fighter->PotentialAction.IsSet()) {

			// Get texture
			const ae::_Texture *Texture = nullptr;
			if(ClientPlayer->Fighter->PotentialAction.Item) {

				// Skip dead targets
				if(!ClientPlayer->Fighter->PotentialAction.Item->CanTarget(ClientPlayer, BattleTarget))
					break;

				// Get texture
				Texture = ClientPlayer->Fighter->PotentialAction.Item->Texture;
			}

			// Make icon flash
			glm::vec4 Color(glm::vec4(1.0f));
			if(Time - (int)Time < 0.5)
				Color.a = 0.5f;

			// Draw background icon
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			glm::vec2 DrawPosition = glm::ivec2(BarEndX + 10, SlotPosition.y + Fighter->BattleElement->Size.y/2);
			if(ClientPlayer->Fighter->PotentialAction.Item) {
				DrawPosition.x += ItemBackTexture->Size.x/2;
				ae::Graphics.DrawCenteredImage(DrawPosition, ItemBackTexture, Color);
			}
			else
				DrawPosition.x += Texture->Size.x/2;

			// Draw item
			ae::Graphics.DrawCenteredImage(DrawPosition, Texture, Color);
		}
	}

	// Draw status effects
	glm::vec2 Offset(0, Fighter->BattleElement->Size.y + 4);
	for(auto &StatusEffect : Character->StatusEffects) {
		if(StatusEffect->BattleElement) {
			StatusEffect->BattleElement->Offset = Offset;
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
	StatsNode["hardcore"] = Character->Hardcore;
	StatsNode["map_x"] = Position.x;
	StatsNode["map_y"] = Position.y;
	StatsNode["spawnmap_id"] = Character->SpawnMapID;
	StatsNode["spawnpoint"] = Character->SpawnPoint;
	StatsNode["portrait_id"] = Character->PortraitID;
	StatsNode["model_id"] = ModelID;
	StatsNode["actionbar_size"] = (Json::Value::UInt64)Character->ActionBar.size();
	StatsNode["health"] = Character->Health;
	StatsNode["mana"] = Character->Mana;
	StatsNode["experience"] = Character->Experience;
	StatsNode["gold"] = Character->Gold;
	StatsNode["goldlost"] = Character->GoldLost;
	StatsNode["playtime"] = Character->PlayTime;
	StatsNode["battletime"] = Character->BattleTime;
	StatsNode["deaths"] = Character->Deaths;
	StatsNode["monsterkills"] = Character->MonsterKills;
	StatsNode["playerkills"] = Character->PlayerKills;
	StatsNode["gamesplayed"] = Character->GamesPlayed;
	StatsNode["bounty"] = Character->Bounty;
	StatsNode["nextbattle"] = Character->NextBattle;
	StatsNode["seed"] = Character->Seed;
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
		StatusEffectNode["duration"] = StatusEffect->Duration;
		StatusEffectNode["maxduration"] = StatusEffect->MaxDuration;
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
	Character->LoadMapID = (ae::NetworkIDType)StatsNode["map_id"].asUInt();
	Position.x = StatsNode["map_x"].asInt();
	Position.y = StatsNode["map_y"].asInt();
	Character->SpawnMapID = (ae::NetworkIDType)StatsNode["spawnmap_id"].asUInt();
	Character->SpawnPoint = StatsNode["spawnpoint"].asUInt();
	Character->Hardcore = StatsNode["hardcore"].asBool();
	Character->PortraitID = StatsNode["portrait_id"].asUInt();
	ModelID = StatsNode["model_id"].asUInt();
	Character->Health = StatsNode["health"].asInt();
	Character->Mana = StatsNode["mana"].asInt();
	Character->Experience = StatsNode["experience"].asInt();
	Character->Gold = StatsNode["gold"].asInt();
	Character->GoldLost = StatsNode["goldlost"].asInt();
	Character->PlayTime = StatsNode["playtime"].asDouble();
	Character->BattleTime = StatsNode["battletime"].asDouble();
	Character->Deaths = StatsNode["deaths"].asInt();
	Character->MonsterKills = StatsNode["monsterkills"].asInt();
	Character->PlayerKills = StatsNode["playerkills"].asInt();
	Character->GamesPlayed = StatsNode["gamesplayed"].asInt();
	Character->Bounty = StatsNode["bounty"].asInt();
	Character->NextBattle = StatsNode["nextbattle"].asInt();
	Character->Seed = StatsNode["seed"].asUInt();

	if(!Character->Seed)
		Character->Seed = ae::GetRandomInt((uint32_t)1, std::numeric_limits<uint32_t>::max());

	size_t ActionBarSize = 0;
	ActionBarSize = StatsNode["actionbar_size"].asUInt64();
	Character->ActionBar.resize(ActionBarSize);

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

	// Set actionbar
	for(const Json::Value &ActionNode : Data["actionbar"]) {
		uint32_t Slot = ActionNode["slot"].asUInt();
		if(Slot < Character->ActionBar.size())
			Character->ActionBar[Slot].Item = Stats->Items.at(ActionNode["id"].asUInt());
	}

	// Set status effects
	for(const Json::Value &StatusEffectNode : Data["statuseffects"]) {
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Buff = Stats->Buffs.at(StatusEffectNode["id"].asUInt());
		StatusEffect->Level = StatusEffectNode["level"].asInt();
		StatusEffect->Duration = StatusEffectNode["duration"].asDouble();
		StatusEffect->MaxDuration = StatusEffectNode["maxduration"].asDouble();
		StatusEffect->Time = 1.0 - (StatusEffect->Duration - (int)StatusEffect->Duration);
		Character->StatusEffects.push_back(StatusEffect);
	}

	// Set unlocks
	for(const Json::Value &UnlockNode : Data["unlocks"])
		Character->Unlocks[UnlockNode["id"].asUInt()].Level = UnlockNode["level"].asInt();
}

// Serialize for ObjectCreate
void _Object::SerializeCreate(ae::_Buffer &Data) {
	Data.Write<ae::NetworkIDType>(NetworkID);
	Data.Write<glm::ivec2>(Position);
	Data.WriteString(Name.c_str());
	if(Character)
		Data.Write<uint32_t>(Character->PortraitID);
	else
		Data.Write<uint32_t>(0);
	Data.Write<uint32_t>(ModelID);
	Data.Write<uint8_t>(Light);
	Data.WriteBit(Character->Invisible);
}

// Serialize for ObjectUpdate
void _Object::SerializeUpdate(ae::_Buffer &Data) {
	Data.Write<ae::NetworkIDType>(NetworkID);
	Data.Write<glm::ivec2>(Position);
	Data.Write<uint8_t>(Character->Status);
	Data.WriteBit(Light);
	Data.WriteBit(Character->Invisible);
	Data.WriteBit(Character->Bounty);
	if(Character->Bounty)
		Data.Write<int>(Character->Bounty);
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
	Data.Write<int>(Character->Health);
	Data.Write<int>(Character->MaxHealth);
	Data.Write<int>(Character->Mana);
	Data.Write<int>(Character->MaxMana);
	Data.Write<int>(Character->Experience);
	Data.Write<int>(Character->Gold);
	Data.Write<int>(Character->Invisible);
	Data.Write<int>(Character->Hardcore);
	Data.Write<int>(Character->GoldLost);
	Data.Write<double>(Character->PlayTime);
	Data.Write<double>(Character->BattleTime);
	Data.Write<int>(Character->Deaths);
	Data.Write<int>(Character->MonsterKills);
	Data.Write<int>(Character->PlayerKills);
	Data.Write<int>(Character->GamesPlayed);
	Data.Write<int>(Character->Bounty);

	// Write inventory
	Inventory->Serialize(Data);

	// Write skills
	Data.Write<uint32_t>((uint32_t)Character->Skills.size());
	for(const auto &Skill : Character->Skills) {
		Data.Write<uint32_t>(Skill.first);
		Data.Write<int>(Skill.second);
	}

	// Write action bar
	Data.Write<uint8_t>((uint8_t)Character->ActionBar.size());
	for(size_t i = 0; i < Character->ActionBar.size(); i++) {
		Character->ActionBar[i].Serialize(Data);
	}

	// Write unlocks
	Data.Write<uint32_t>((uint32_t)Character->Unlocks.size());
	for(const auto &Unlock : Character->Unlocks) {
		Data.Write<uint32_t>(Unlock.first);
		Data.Write<int>(Unlock.second.Level);
	}

	// Write status effects
	Data.Write<uint8_t>((uint8_t)Character->StatusEffects.size());
	for(const auto &StatusEffect : Character->StatusEffects) {
		StatusEffect->Serialize(Data);
	}
}

// Serialize object for battle
void _Object::SerializeBattle(ae::_Buffer &Data) {
	Data.Write<ae::NetworkIDType>(NetworkID);
	Data.Write<uint32_t>(Monster->DatabaseID);
	Data.Write<glm::ivec2>(Position);
	Data.Write<int>(Character->Health);
	Data.Write<int>(Character->MaxHealth);
	Data.Write<int>(Character->Mana);
	Data.Write<int>(Character->MaxMana);
	Data.Write<int>(Character->EquipmentBattleSpeed);
	Data.Write<double>(Fighter->TurnTimer);
	Data.Write<uint8_t>(Fighter->BattleSide);

	Data.Write<uint8_t>((uint8_t)Character->StatusEffects.size());
	for(auto &StatusEffect : Character->StatusEffects) {
		StatusEffect->Serialize(Data);
	}
}

// Unserialize for ObjectCreate
void _Object::UnserializeCreate(ae::_Buffer &Data) {
	Position = Data.Read<glm::ivec2>();
	Name = Data.ReadString();
	uint32_t PortraitID = Data.Read<uint32_t>();
	if(PortraitID && Character)
		Character->PortraitID = PortraitID;
	ModelID = Data.Read<uint32_t>();
	Light = Data.Read<uint8_t>();
	Character->Invisible = Data.ReadBit();

	Character->Portrait = Stats->GetPortraitImage(Character->PortraitID);
	ModelTexture = Stats->Models.at(ModelID).Texture;
}

// Unserialize object stats
void _Object::UnserializeStats(ae::_Buffer &Data) {
	Name = Data.ReadString();
	Light = Data.Read<uint8_t>();
	ModelID = Data.Read<uint32_t>();
	Character->PortraitID = Data.Read<uint32_t>();
	Character->PartyName = Data.ReadString();
	Character->Health = Data.Read<int>();
	Character->BaseMaxHealth = Character->MaxHealth = Data.Read<int>();
	Character->Mana = Data.Read<int>();
	Character->BaseMaxMana = Character->MaxMana = Data.Read<int>();
	Character->Experience = Data.Read<int>();
	Character->Gold = Data.Read<int>();
	Character->Invisible = Data.Read<int>();
	Character->Hardcore = Data.Read<int>();
	Character->GoldLost = Data.Read<int>();
	Character->PlayTime = Data.Read<double>();
	Character->BattleTime = Data.Read<double>();
	Character->Deaths = Data.Read<int>();
	Character->MonsterKills = Data.Read<int>();
	Character->PlayerKills = Data.Read<int>();
	Character->GamesPlayed = Data.Read<int>();
	Character->Bounty = Data.Read<int>();

	ModelTexture = Stats->Models.at(ModelID).Texture;

	// Read inventory
	Inventory->Unserialize(Data, Stats);

	// Read skills
	uint32_t SkillCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < SkillCount; i++) {
		uint32_t SkillID = Data.Read<uint32_t>();
		int Points = Data.Read<int>();
		Character->Skills[SkillID] = Points;
	}

	// Read action bar
	size_t ActionBarSize = Data.Read<uint8_t>();
	Character->ActionBar.resize(ActionBarSize);
	for(size_t i = 0; i < ActionBarSize; i++)
		Character->ActionBar[i].Unserialize(Data, Stats);

	// Read unlocks
	uint32_t UnlockCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < UnlockCount; i++) {
		uint32_t UnlockID = Data.Read<uint32_t>();
		int Level = Data.Read<int>();
		Character->Unlocks[UnlockID].Level = Level;
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

// Unserialize battle stats
void _Object::UnserializeBattle(ae::_Buffer &Data, bool IsClient) {
	Controller->InputStates.clear();

	// Get object type
	Position = ServerPosition = Data.Read<glm::ivec2>();
	Character->Health = Data.Read<int>();
	Character->BaseMaxHealth = Character->MaxHealth = Data.Read<int>();
	Character->Mana = Data.Read<int>();
	Character->BaseMaxMana = Character->MaxMana = Data.Read<int>();
	Character->EquipmentBattleSpeed = Data.Read<int>();
	if(!IsClient)
		Character->BaseBattleSpeed = Character->EquipmentBattleSpeed;
	Fighter->TurnTimer = Data.Read<double>();
	Fighter->BattleSide = Data.Read<uint8_t>();

	Character->DeleteStatusEffects();
	int StatusEffectCount = Data.Read<uint8_t>();
	for(int i = 0; i < StatusEffectCount; i++) {
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Unserialize(Data, Stats);
		Character->StatusEffects.push_back(StatusEffect);
	}
}

// Update stats
_StatusEffect *_Object::UpdateStats(_StatChange &StatChange) {
	_StatusEffect *StatusEffect = nullptr;

	// Add buffs
	if(StatChange.HasStat(StatType::BUFF)) {
		StatusEffect = new _StatusEffect();
		StatusEffect->Buff = (_Buff *)StatChange.Values[StatType::BUFF].Pointer;
		StatusEffect->Level = StatChange.Values[StatType::BUFFLEVEL].Integer;
		StatusEffect->MaxDuration = StatusEffect->Duration = StatChange.Values[StatType::BUFFDURATION].Float;

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

	// Update gold
	if(StatChange.HasStat(StatType::GOLD)) {
		Character->UpdateGold(StatChange.Values[StatType::GOLD].Integer);
	}

	// Update gold stolen
	if(StatChange.HasStat(StatType::GOLDSTOLEN)) {
		int Amount = StatChange.Values[StatType::GOLDSTOLEN].Integer;
		Fighter->GoldStolen += Amount;
		if(Fighter->GoldStolen > PLAYER_MAX_GOLD)
			Fighter->GoldStolen = PLAYER_MAX_GOLD;

		Character->UpdateGold(Amount);
	}

	// Update experience
	if(StatChange.HasStat(StatType::EXPERIENCE)) {
		Character->UpdateExperience(StatChange.Values[StatType::EXPERIENCE].Integer);
	}

	// Update health
	bool WasAlive = Character->IsAlive();
	if(StatChange.HasStat(StatType::HEALTH))
		Character->UpdateHealth(StatChange.Values[StatType::HEALTH].Integer);

	// Just died
	if(WasAlive && !Character->IsAlive()) {
		Fighter->PotentialAction.Unset();
		Character->Action.Unset();
		Character->ResetUIState();

		// If not in battle apply penalty immediately
		if(!Character->Battle) {

			// Apply penalty
			ApplyDeathPenalty(PLAYER_DEATH_GOLD_PENALTY, 0);
		}
	}

	// Mana change
	if(StatChange.HasStat(StatType::MANA))
		Character->UpdateMana(StatChange.Values[StatType::MANA].Integer);

	// Stamina change
	if(StatChange.HasStat(StatType::STAMINA)) {
		Fighter->TurnTimer += StatChange.Values[StatType::STAMINA].Float;
		Fighter->TurnTimer = glm::clamp(Fighter->TurnTimer, 0.0, 1.0);
	}

	// Action bar upgrade
	if(StatChange.HasStat(StatType::ACTIONBARSIZE)) {
		size_t NewSize = Character->ActionBar.size() + (size_t)StatChange.Values[StatType::ACTIONBARSIZE].Integer;
		if(NewSize >= ACTIONBAR_MAX_SIZE)
			NewSize = ACTIONBAR_MAX_SIZE;

		Character->ActionBar.resize(NewSize);
	}

	// Flee from battle
	if(StatChange.HasStat(StatType::FLEE)) {
		if(Character->Battle)
			Character->Battle->RemoveObject(this);
	}

	// Run server only commands
	if(Server) {
		if(!Character->Battle) {

			// Start teleport
			if(StatChange.HasStat(StatType::TELEPORT))
				Server->StartTeleport(this, StatChange.Values[StatType::TELEPORT].Float);

			// Start battle
			if(StatChange.HasStat(StatType::BATTLE))
				Server->QueueBattle(this, (uint32_t)StatChange.Values[StatType::BATTLE].Integer, true, false, 0.0f, 0.0f);

			// Start PVP
			if(StatChange.HasStat(StatType::HUNT))
				Server->QueueBattle(this, 0, false, true, StatChange.Values[StatType::HUNT].Float, 0.0f);
			if(StatChange.HasStat(StatType::BOUNTYHUNT))
				Server->QueueBattle(this, 0, false, true, 0.0f, StatChange.Values[StatType::BOUNTYHUNT].Float);
		}

		// Set clock
		if(StatChange.HasStat(StatType::CLOCK))
			Server->SetClock(StatChange.Values[StatType::CLOCK].Float);
	}

	return StatusEffect;
}

// Moves the player and returns direction moved
int _Object::Move() {
	if(Controller->WaitForServer || Character->Battle || Controller->InputStates.size() == 0 || !Character->IsAlive())
		return 0;

	// Check timer
	if(Controller->MoveTime < PLAYER_MOVETIME / (Character->MoveSpeed / 100.0))
		return 0;

	Controller->MoveTime = 0;
	int InputState = Controller->InputStates.front();
	Controller->InputStates.pop_front();

	// Get new position
	glm::ivec2 Direction(0, 0);
	GetDirectionFromInput(InputState, Direction);

	// Move player
	if(Map->CanMoveTo(Position + Direction, this)) {
		Position += Direction;
		if(GetTile()->Zone > 0 && Character->Invisible != 1)
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
void _Object::ApplyDeathPenalty(float Penalty, int BountyLoss) {
	int GoldPenalty = BountyLoss + (int)(std::abs(Character->Gold) * Penalty + 0.5f);
	int OldBounty = Character->Bounty;

	// Update stats
	Character->UpdateGold(-GoldPenalty);
	Character->Deaths++;
	Character->GoldLost += GoldPenalty;
	Character->Bounty -= BountyLoss;
	if(Character->Bounty < 0)
		Character->Bounty = 0;

	// Send message
	if(Server) {
		if(BountyLoss > 0 && Character->Bounty == 0) {
			std::string BountyMessage = "Player " + Name + "'s bounty of " + std::to_string(OldBounty) + " gold has been claimed!";
			Server->BroadcastMessage(nullptr, BountyMessage, "cyan");
			Server->Log << "[BOUNTY] " << BountyMessage << std::endl;
		}

		if(Peer) {
			Server->SendMessage(Peer, std::string("You lost " + std::to_string(GoldPenalty) + " gold"), "red");
			Server->Log << "[DEATH] Player " << Name << " died and lost " << std::to_string(GoldPenalty) << " gold ( character_id=" << Character->CharacterID << " gold=" << Character->Gold << " deaths=" << Character->Deaths << " )" << std::endl;
		}
	}
}

// Set action and targets
void _Object::SetActionUsing(ae::_Buffer &Data, ae::_Manager<_Object> *ObjectManager) {

	// Check for needed commands
	if(!Character->Action.IsSet()) {

		uint8_t ActionBarSlot = Data.Read<uint8_t>();
		int TargetCount = Data.Read<uint8_t>();
		if(!TargetCount)
			return;

		// Get skillbar action
		if(!Character->GetActionFromActionBar(Character->Action, ActionBarSlot))
			return;

		// Get targets
		Character->Targets.clear();
		for(int i = 0; i < TargetCount; i++) {
			ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
			_Object *Target = ObjectManager->GetObject(NetworkID);
			if(Target && Character->Action.Item->CanTarget(this, Target))
				Character->Targets.push_back(Target);
		}
	}
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
