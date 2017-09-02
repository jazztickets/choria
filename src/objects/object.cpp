/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <objects/components/record.h>
#include <objects/statuseffect.h>
#include <objects/map.h>
#include <objects/battle.h>
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
#include <hud.h>
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
	Record(nullptr),
	Stats(nullptr),
	Map(nullptr),
	HUD(nullptr),
	Scripting(nullptr),
	Server(nullptr),
	Peer(nullptr),

	Position(0, 0),
	ServerPosition(0, 0),
	MoveTime(0),
	DirectionMoved(0),
	UseCommand(false),
	WaitForServer(false),

	Battle(nullptr),
	BattleElement(nullptr),
	LastTarget{nullptr, nullptr},
	TurnTimer(0.0),
	NextBattle(0),
	GoldStolen(0),
	JoinedBattle(false),
	BattleSide(0),

	ModelTexture(nullptr),
	StatusTexture(nullptr),
	Portrait(nullptr),
	BattleOffset(0, 0),
	ResultPosition(0, 0),
	StatPosition(0, 0),
	PortraitID(0),
	ModelID(0),
	Status(0),

	Owner(nullptr),
	DatabaseID(0),
	ExperienceGiven(0),
	GoldGiven(0),
	AI(""),

	LoadMapID(0),
	SpawnMapID(1),
	SpawnPoint(0),
	TeleportTime(-1),

	MenuOpen(false),
	InventoryOpen(false),
	SkillsOpen(false),

	Vendor(nullptr),
	Trader(nullptr),
	Blacksmith(nullptr),
	Minigame(nullptr),
	Seed(0),

	TradePlayer(nullptr),
	TradeGold(0),
	WaitingForTrade(false),
	TradeAccepted(false),

	Bot(false) {

	Inventory = new _Inventory();
	Character = new _Character(this);
	Record = new _Record(this);
}

// Destructor
_Object::~_Object() {

	delete Record;
	Record = nullptr;

	delete Character;
	Character = nullptr;

	delete Inventory;
	Inventory = nullptr;

	if(Map) {
		Map->RemoveObject(this);
		Map = nullptr;
	}

	if(Battle) {
		Battle->RemoveFighter(this);
		Battle = nullptr;
	}

	if(HUD) {
		HUD->RemoveStatChanges(this);
		HUD = nullptr;
	}

	if(Bot) {
		delete Peer;
		Peer = nullptr;
	}

	DeleteStatusEffects();
	RemoveBattleElement();
}

// Updates the player
void _Object::Update(double FrameTime) {
	bool CheckEvent = false;

	// Update bots
	if(Server && Bot)
		UpdateBot(FrameTime);

	// Update player position
	DirectionMoved = Move();
	if(DirectionMoved) {
		CheckEvent = true;

		// Remove node from pathfinding
		if(Bot && Path.size())
			Path.erase(Path.begin());
	}

	// Player hit the use button
	if(UseCommand) {
		CheckEvent = true;
		UseCommand = false;
	}

	// Update actions and battle
	if(Character->IsAlive()) {

		// Update monster AI
		if(Server && Battle && IsMonster())
			UpdateMonsterAI(Battle->Fighters, FrameTime);

		// Check turn timer
		if(Battle) {
			if(!Character->Stunned)
				TurnTimer += FrameTime * BATTLE_DEFAULTSPEED * Character->BattleSpeed / 100.0;
		}
		else
			TurnTimer = 1.0;

		// Resolve action
		if(TurnTimer >= 1.0) {
			TurnTimer = 1.0;

			if(Server && Action.IsSet()) {
				ScopeType Scope = ScopeType::WORLD;
				if(Battle)
					Scope = ScopeType::BATTLE;

				_Buffer Packet;
				if(Action.Resolve(Packet, this, Scope)) {
					SendPacket(Packet);
				}
				else {

					// Can't use action so send an action clear packet
					_Buffer FailPacket;
					FailPacket.Write<PacketType>(PacketType::ACTION_CLEAR);
					FailPacket.Write<NetworkIDType>(NetworkID);
					SendPacket(FailPacket);
				}

				Action.Unset();
			}
		}
	}
	else
		TurnTimer = 0.0;

	// Update status effects
	for(auto Iterator = StatusEffects.begin(); Iterator != StatusEffects.end(); ) {
		_StatusEffect *StatusEffect = *Iterator;
		StatusEffect->Time += FrameTime;

		// Call status effect's update every second
		if(StatusEffect->Time >= 1.0) {
			StatusEffect->Time -= 1.0;

			// Resolve effects
			if(Server && Character->IsAlive()) {
				ResolveBuff(StatusEffect, "Update");
			}
		}

		// Reduce count
		StatusEffect->Duration -= FrameTime;
		if(StatusEffect->Duration <= 0 || !Character->IsAlive()) {
			delete StatusEffect;
			Iterator = StatusEffects.erase(Iterator);

			Character->CalculateStats();
		}
		else
			++Iterator;
	}

	// Update battle cooldowns
	for(auto Iterator = BattleCooldown.begin(); Iterator != BattleCooldown.end(); ) {
		Iterator->second -= FrameTime;

		// Remove cooldown
		if(Iterator->second <= 0.0)
			Iterator = BattleCooldown.erase(Iterator);
		else
			++Iterator;
	}

	// Update status effects
	if(Character)
		Character->Update(FrameTime);

	// Update timers
	MoveTime += FrameTime;

	// Update teleport time
	if(TeleportTime > 0.0) {
		Status = STATUS_TELEPORT;
		TeleportTime -= FrameTime;
		if(TeleportTime <= 0.0) {
			CheckEvent = true;
			TeleportTime = 0.0;
		}
	}

	// Update playtime
	if(Record) {
		Record->PlayTime += FrameTime;
		if(Battle)
			Record->BattleTime += FrameTime;
	}

	// Check events
	if(Map && CheckEvent)
		Map->CheckEvents(this);

	// Update status
	Status = STATUS_NONE;
	if(!Character->IsAlive())
		Status = STATUS_DEAD;
	else if(Battle)
		Status = STATUS_BATTLE;
	else if(WaitingForTrade)
		Status = STATUS_TRADE;
	else if(Vendor)
		Status = STATUS_VENDOR;
	else if(Trader)
		Status = STATUS_TRADER;
	else if(Blacksmith)
		Status = STATUS_BLACKSMITH;
	else if(Minigame)
		Status = STATUS_MINIGAME;
	else if(InventoryOpen)
		Status = STATUS_INVENTORY;
	else if(SkillsOpen)
		Status = STATUS_SKILLS;
	else if(MenuOpen)
		Status = STATUS_MENU;

}

// Update bot AI
void _Object::UpdateBot(double FrameTime) {

	// Call ai script
	if(!Battle && Scripting->StartMethodCall("Bot_Server", "Update")) {
		Scripting->PushReal(FrameTime);
		Scripting->PushObject(this);
		Scripting->MethodCall(2, 0);
		Scripting->FinishMethodCall();
	}

	// Set input
	if(AcceptingMoveInput()) {
		int InputState = 0;

		// Call ai input script
		if(Scripting->StartMethodCall("Bot_Server", "GetInputState")) {
			Scripting->PushObject(this);
			Scripting->MethodCall(1, 1);
			InputState = Scripting->GetInt(1);
			Scripting->FinishMethodCall();
		}

		InputStates.clear();
		if(InputState)
			InputStates.push_back(InputState);
	}

	// Update battle
	if(Battle) {
		if(TurnTimer >= 1.0 && !Action.IsSet()) {

			// Set skill used
			size_t ActionBarIndex = 0;
			if(!GetActionFromSkillbar(Action, ActionBarIndex)) {
				return;
			}

			// Check that the action can be used
			_ActionResult ActionResult;
			ActionResult.Source.Object = this;
			ActionResult.Scope = ScopeType::BATTLE;
			ActionResult.ActionUsed = Action;
			if(!Action.Item->CanUse(Scripting, ActionResult))
				Action.Item = nullptr;

			// Separate fighter list
			std::list<_Object *> Allies, Enemies;
			Battle->GetSeparateFighterList(BattleSide, Allies, Enemies);

			// Call lua script
			if(Enemies.size()) {
				if(Scripting->StartMethodCall("AI_Smart", "Update")) {
					Targets.clear();
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
void _Object::UpdateMonsterAI(const std::list<_Object *> &Fighters, double FrameTime) {
	if(!AI.length())
		return;

	// Call AI script to get action
	if(TurnTimer >= 1.0 && !Action.IsSet()) {

		// Separate fighter list
		std::list<_Object *> Allies, Enemies;
		Battle->GetSeparateFighterList(BattleSide, Allies, Enemies);

		// Call lua script
		if(Enemies.size()) {
			if(Scripting->StartMethodCall(AI, "Update")) {
				Targets.clear();
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

		float Alpha = 1.0f;
		if(Character->Invisible > 0)
			Alpha = PLAYER_INVIS_ALPHA;

		Graphics.SetProgram(Assets.Programs["pos_uv"]);
		glUniformMatrix4fv(Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

		Graphics.SetVBO(VBO_QUAD);

		glm::vec3 DrawPosition;
		if(HUD && HUD->ShowDebug) {
			DrawPosition = glm::vec3(ServerPosition, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
			Graphics.SetColor(glm::vec4(1, 0, 0, 1));
			Graphics.DrawSprite(DrawPosition, ModelTexture);
		}

		glm::vec4 Color(1.0f, 1.0f, 1.0f, Alpha);
		DrawPosition = glm::vec3(Position, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
		Graphics.SetColor(Color);
		Graphics.DrawSprite(DrawPosition, ModelTexture);
		if(StatusTexture) {
			Graphics.DrawSprite(DrawPosition, StatusTexture);
		}

		if(ClientPlayer != this) {
			Assets.Fonts["hud_medium"]->DrawText(Name, glm::vec2(DrawPosition) + glm::vec2(0, -0.5f), CENTER_BASELINE, Color, 1.0f / ModelTexture->Size.x);
		}
	}
}

// Renders the fighter during a battle
void _Object::RenderBattle(_Object *ClientPlayer, double Time) {
	glm::vec4 GlobalColor(glm::vec4(1.0f));
	GlobalColor.a = 1.0f;
	if(!Character->IsAlive())
		GlobalColor.a = 0.2f;

	// Draw slot
	BattleElement->Fade = GlobalColor.a;

	// Get slot center
	glm::vec2 SlotPosition = BattleElement->Bounds.Start;

	// Save positions
	ResultPosition = BattleElement->Bounds.Start + BattleElement->Size / 2.0f;
	StatPosition = ResultPosition + glm::vec2(Portrait->Size.x/2 + 10 + BATTLE_HEALTHBAR_WIDTH/2, -Portrait->Size.y/2);

	// Name
	Assets.Fonts["hud_medium"]->DrawText(Name, SlotPosition + glm::vec2(0, -12), LEFT_BASELINE, GlobalColor);
	Graphics.SetColor(GlobalColor);

	// Portrait
	if(Portrait) {
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(SlotPosition + glm::vec2(Portrait->Size/2), Portrait, GlobalColor);
	}

	// Health/mana bars
	glm::vec2 BarSize(BATTLE_HEALTHBAR_WIDTH, BATTLE_HEALTHBAR_HEIGHT);
	glm::vec2 BarOffset(BattleElement->Size.x + 10, 0);
	float BarPaddingY = 6;

	// Get ui size
	_Bounds BarBounds;
	BarBounds.Start = SlotPosition + glm::vec2(0, 0) + BarOffset;
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x, BarSize.y) + BarOffset;
	glm::vec2 BarCenter = (BarBounds.Start + BarBounds.End) / 2.0f;
	float BarEndX = BarBounds.End.x;

	// Draw empty bar
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.DrawImage(BarBounds, Assets.Elements["image_hud_health_bar_empty"]->Texture, true);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * Character->GetHealthPercent(), BarSize.y) + BarOffset;
	Graphics.DrawImage(BarBounds, Assets.Elements["image_hud_health_bar_full"]->Texture, true);

	// Draw health text
	std::stringstream Buffer;
	Buffer << Round(Character->Health) << " / " << Round(Character->MaxHealth);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str(), BarCenter + glm::vec2(0, 5), CENTER_BASELINE, GlobalColor);
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
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.DrawImage(BarBounds, Assets.Elements["image_hud_mana_bar_empty"]->Texture, true);

		// Draw full bar
		BarBounds.End = SlotPosition + glm::vec2(BarSize.x * ManaPercent, BarSize.y) + BarOffset;
		Graphics.DrawImage(BarBounds, Assets.Elements["image_hud_mana_bar_full"]->Texture, true);

		// Draw mana text
		Buffer << Round(Character->Mana) << " / " << Round(Character->MaxMana);
		Assets.Fonts["hud_small"]->DrawText(Buffer.str(), BarCenter + glm::vec2(0, 5), CENTER_BASELINE, GlobalColor);
		Buffer.str("");
	}

	// Draw turn timer
	BarOffset.y += BarSize.y + BarPaddingY;
	BarSize.y = 8;
	BarBounds.Start = SlotPosition + glm::vec2(0, 0) + BarOffset;
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x, BarSize.y) + BarOffset;

	// Draw empty bar
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.DrawImage(BarBounds, Assets.Elements["image_hud_experience_bar_empty"]->Texture, true);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * TurnTimer, BarSize.y) + BarOffset;
	Graphics.DrawImage(BarBounds, Assets.Elements["image_hud_experience_bar_full"]->Texture, true);

	// Get background for items used
	const _Texture *ItemBackTexture = Assets.Textures["textures/hud/item_back.png"];

	// Draw the action used
	if(ClientPlayer->BattleSide == BattleSide) {

		if(Action.Item) {
			glm::vec2 ItemUsingPosition = SlotPosition + glm::vec2(-ItemBackTexture->Size.x/2 - 10, BattleElement->Size.y/2);
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			if(!Action.Item->IsSkill())
				Graphics.DrawCenteredImage(ItemUsingPosition, ItemBackTexture, GlobalColor);
			Graphics.DrawCenteredImage(ItemUsingPosition, Action.Item->Texture, GlobalColor);
		}
	}

	// Draw potential action to use
	for(auto &BattleTarget : ClientPlayer->Targets) {
		if(BattleTarget == this && ClientPlayer->PotentialAction.IsSet()) {

			// Get texture
			const _Texture *Texture = nullptr;
			if(ClientPlayer->PotentialAction.Item) {

				// Skip dead targets
				if(!ClientPlayer->PotentialAction.Item->CanTarget(ClientPlayer, BattleTarget))
					break;

				// Get texture
				Texture = ClientPlayer->PotentialAction.Item->Texture;
			}

			// Make icon flash
			glm::vec4 Color(glm::vec4(1.0f));
			if(Time - (int)Time < 0.5)
				Color.a = 0.5f;

			// Draw background icon
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			glm::vec2 DrawPosition = glm::ivec2(BarEndX + 10, SlotPosition.y + BattleElement->Size.y/2);
			if(ClientPlayer->PotentialAction.Item) {
				DrawPosition.x += ItemBackTexture->Size.x/2;
				Graphics.DrawCenteredImage(DrawPosition, ItemBackTexture, Color);
			}
			else
				DrawPosition.x += Texture->Size.x/2;

			// Draw item
			Graphics.DrawCenteredImage(DrawPosition, Texture, Color);
		}
	}

	// Draw status effects
	glm::vec2 Offset(0, BattleElement->Size.y + 4);
	for(auto &StatusEffect : StatusEffects) {
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
	StatsNode["spawnmap_id"] = SpawnMapID;
	StatsNode["spawnpoint"] = SpawnPoint;
	StatsNode["portrait_id"] = PortraitID;
	StatsNode["model_id"] = ModelID;
	StatsNode["actionbar_size"] = (Json::Value::UInt64)Character->ActionBar.size();
	StatsNode["health"] = Character->Health;
	StatsNode["mana"] = Character->Mana;
	StatsNode["experience"] = Character->Experience;
	StatsNode["gold"] = Character->Gold;
	StatsNode["goldlost"] = Record->GoldLost;
	StatsNode["playtime"] = Record->PlayTime;
	StatsNode["battletime"] = Record->BattleTime;
	StatsNode["deaths"] = Record->Deaths;
	StatsNode["monsterkills"] = Record->MonsterKills;
	StatsNode["playerkills"] = Record->PlayerKills;
	StatsNode["gamesplayed"] = Record->GamesPlayed;
	StatsNode["bounty"] = Record->Bounty;
	StatsNode["nextbattle"] = NextBattle;
	StatsNode["seed"] = Seed;
	Data["stats"] = StatsNode;

	// Write items
	Json::Value ItemsNode;
	for(auto &Bag : Inventory->Bags) {
		if(Bag.ID == _Bag::NONE)
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
		ItemsNode[std::to_string(Bag.ID)] = BagNode;
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
	for(auto &StatusEffect : StatusEffects) {
		Json::Value StatusEffectNode;
		StatusEffectNode["id"] = StatusEffect->Buff->ID;
		StatusEffectNode["level"] = StatusEffect->Level;
		StatusEffectNode["duration"] = StatusEffect->Duration;
		StatusEffectsNode.append(StatusEffectNode);
	}
	Data["statuseffects"] = StatusEffectsNode;

	// Write unlocks
	Json::Value UnlocksNode;
	for(auto &Unlock : Unlocks) {
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
	Json::Value Data;
	Json::Reader Reader;
	if(!Reader.parse(JsonString, Data))
		throw std::runtime_error("_Object::UnserializeSaveData: Error parsing JSON string!");

	// Get stats
	Json::Value StatsNode = Data["stats"];
	LoadMapID = (NetworkIDType)StatsNode["map_id"].asUInt();
	Position.x = StatsNode["map_x"].asInt();
	Position.y = StatsNode["map_y"].asInt();
	SpawnMapID = (NetworkIDType)StatsNode["spawnmap_id"].asUInt();
	SpawnPoint = StatsNode["spawnpoint"].asUInt();
	Character->Hardcore = StatsNode["hardcore"].asBool();
	PortraitID = StatsNode["portrait_id"].asUInt();
	ModelID = StatsNode["model_id"].asUInt();
	Character->Health = StatsNode["health"].asInt();
	Character->Mana = StatsNode["mana"].asInt();
	Character->Experience = StatsNode["experience"].asInt();
	Character->Gold = StatsNode["gold"].asInt();
	Record->GoldLost = StatsNode["goldlost"].asInt();
	Record->PlayTime = StatsNode["playtime"].asDouble();
	Record->BattleTime = StatsNode["battletime"].asDouble();
	Record->Deaths = StatsNode["deaths"].asInt();
	Record->MonsterKills = StatsNode["monsterkills"].asInt();
	Record->PlayerKills = StatsNode["playerkills"].asInt();
	Record->GamesPlayed = StatsNode["gamesplayed"].asInt();
	Record->Bounty = StatsNode["bounty"].asInt();
	NextBattle = StatsNode["nextbattle"].asInt();
	Seed = StatsNode["seed"].asUInt();

	if(!Seed)
		Seed = GetRandomInt((uint32_t)1, std::numeric_limits<uint32_t>::max());

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
			Inventory->Bags[std::stoul(BagNode.name())].Slots[ItemNode["slot"].asUInt64()] = InventorySlot;
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
		StatusEffect->Time = 1.0 - (StatusEffect->Duration - (int)StatusEffect->Duration);
		StatusEffects.push_back(StatusEffect);
	}

	// Set unlocks
	for(const Json::Value &UnlockNode : Data["unlocks"])
		Unlocks[UnlockNode["id"].asUInt()].Level = UnlockNode["level"].asInt();
}

// Generate damage
int _Object::GenerateDamage() {
	return GetRandomInt(Character->MinDamage, Character->MaxDamage);
}

// Create a UI element for battle
void _Object::CreateBattleElement(_Element *Parent) {
	if(BattleElement)
		throw std::runtime_error("_Object::CreateBattleElement: BattleElement already exists!");

	BattleElement = new _Element();
	BattleElement->Name = "battle_element";
	BattleElement->Size = glm::vec2(64, 64);
	BattleElement->Offset = BattleOffset;
	BattleElement->Alignment = CENTER_MIDDLE;
	BattleElement->Active = true;
	BattleElement->Index = _HUD::WINDOW_HUD_EFFECTS;
	BattleElement->UserData = this;
	BattleElement->Parent = Parent;
	BattleElement->Style = (BattleSide == 0) ? Assets.Styles["style_battle_slot_green"] : Assets.Styles["style_battle_slot_red"];
	BattleElement->CalculateBounds();
	Parent->Children.push_back(BattleElement);
}

// Remove battle element
void _Object::RemoveBattleElement() {

	if(BattleElement) {
		if(BattleElement->Parent)
			BattleElement->Parent->RemoveChild(BattleElement);

		for(auto &Child : BattleElement->Children) {
			if(Child->UserData)
				((_StatusEffect *)Child->UserData)->BattleElement = nullptr;
		}

		delete BattleElement;
		BattleElement = nullptr;
	}
}

// Serialize for ObjectCreate
void _Object::SerializeCreate(_Buffer &Data) {
	Data.Write<NetworkIDType>(NetworkID);
	Data.Write<glm::ivec2>(Position);
	Data.WriteString(Name.c_str());
	Data.Write<uint32_t>(PortraitID);
	Data.Write<uint32_t>(ModelID);
	Data.WriteBit(Character->Invisible);
}

// Serialize for ObjectUpdate
void _Object::SerializeUpdate(_Buffer &Data) {
	Data.Write<NetworkIDType>(NetworkID);
	Data.Write<glm::ivec2>(Position);
	Data.Write<uint8_t>(Status);
	Data.WriteBit(Character->Invisible);
}

// Serialize object stats
void _Object::SerializeStats(_Buffer &Data) {
	Data.WriteString(Name.c_str());
	Data.WriteString(PartyName.c_str());
	Data.Write<uint32_t>(PortraitID);
	Data.Write<uint32_t>(ModelID);
	Data.Write<int>(Character->Health);
	Data.Write<int>(Character->MaxHealth);
	Data.Write<int>(Character->Mana);
	Data.Write<int>(Character->MaxMana);
	Data.Write<int>(Character->Experience);
	Data.Write<int>(Character->Gold);
	Data.Write<int>(Record->GoldLost);
	Data.Write<double>(Record->PlayTime);
	Data.Write<double>(Record->BattleTime);
	Data.Write<int>(Record->Deaths);
	Data.Write<int>(Record->MonsterKills);
	Data.Write<int>(Record->PlayerKills);
	Data.Write<int>(Record->GamesPlayed);
	Data.Write<int>(Record->Bounty);
	Data.Write<int>(Character->Invisible);
	Data.Write<int>(Character->Hardcore);

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
	Data.Write<uint32_t>((uint32_t)Unlocks.size());
	for(const auto &Unlock : Unlocks) {
		Data.Write<uint32_t>(Unlock.first);
		Data.Write<int>(Unlock.second.Level);
	}

	// Write status effects
	Data.Write<uint8_t>((uint8_t)StatusEffects.size());
	for(const auto &StatusEffect : StatusEffects) {
		StatusEffect->Serialize(Data);
	}
}

// Serialize object for battle
void _Object::SerializeBattle(_Buffer &Data) {
	Data.Write<NetworkIDType>(NetworkID);
	Data.Write<uint32_t>(DatabaseID);
	Data.Write<glm::ivec2>(Position);
	Data.Write<double>(TurnTimer);
	Data.Write<int>(Character->Health);
	Data.Write<int>(Character->MaxHealth);
	Data.Write<int>(Character->Mana);
	Data.Write<int>(Character->MaxMana);
	Data.Write<uint8_t>(BattleSide);

	Data.Write<uint8_t>((uint8_t)StatusEffects.size());
	for(auto &StatusEffect : StatusEffects) {
		StatusEffect->Serialize(Data);
	}
}

// Unserialize for ObjectCreate
void _Object::UnserializeCreate(_Buffer &Data) {
	Position = Data.Read<glm::ivec2>();
	Name = Data.ReadString();
	PortraitID = Data.Read<uint32_t>();
	ModelID = Data.Read<uint32_t>();
	Character->Invisible = Data.ReadBit();

	Portrait = Stats->GetPortraitImage(PortraitID);
	ModelTexture = Stats->Models.at(ModelID).Texture;
}

// Unserialize object stats
void _Object::UnserializeStats(_Buffer &Data) {
	Name = Data.ReadString();
	PartyName = Data.ReadString();
	PortraitID = Data.Read<uint32_t>();
	ModelID = Data.Read<uint32_t>();
	Character->Health = Data.Read<int>();
	Character->BaseMaxHealth = Character->MaxHealth = Data.Read<int>();
	Character->Mana = Data.Read<int>();
	Character->BaseMaxMana = Character->MaxMana = Data.Read<int>();
	Character->Experience = Data.Read<int>();
	Character->Gold = Data.Read<int>();
	Record->GoldLost = Data.Read<int>();
	Record->PlayTime = Data.Read<double>();
	Record->BattleTime = Data.Read<double>();
	Record->Deaths = Data.Read<int>();
	Record->MonsterKills = Data.Read<int>();
	Record->PlayerKills = Data.Read<int>();
	Record->GamesPlayed = Data.Read<int>();
	Record->Bounty = Data.Read<int>();
	Character->Invisible = Data.Read<int>();
	Character->Hardcore = Data.Read<int>();

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
		Unlocks[UnlockID].Level = Level;
	}

	// Read status effects
	DeleteStatusEffects();
	size_t StatusEffectsSize = Data.Read<uint8_t>();
	for(size_t i = 0; i < StatusEffectsSize; i++) {
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Unserialize(Data, Stats);
		if(HUD)
			StatusEffect->HUDElement = StatusEffect->CreateUIElement(Assets.Elements["element_hud_statuseffects"]);
		StatusEffects.push_back(StatusEffect);
	}

	Character->RefreshActionBarCount();
	Character->CalculateStats();
}

// Unserialize battle stats
void _Object::UnserializeBattle(_Buffer &Data) {
	InputStates.clear();

	// Get fighter type
	Position = ServerPosition = Data.Read<glm::ivec2>();
	TurnTimer = Data.Read<double>();
	Character->Health = Data.Read<int>();
	Character->BaseMaxHealth = Character->MaxHealth = Data.Read<int>();
	Character->Mana = Data.Read<int>();
	Character->BaseMaxMana = Character->MaxMana = Data.Read<int>();
	BattleSide = Data.Read<uint8_t>();

	DeleteStatusEffects();
	int StatusEffectCount = Data.Read<uint8_t>();
	for(int i = 0; i < StatusEffectCount; i++) {
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Unserialize(Data, Stats);
		StatusEffects.push_back(StatusEffect);
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
		StatusEffect->Duration = StatChange.Values[StatType::BUFFDURATION].Float;

		if(AddStatusEffect(StatusEffect)) {
			if(BattleElement)
				StatusEffect->BattleElement = StatusEffect->CreateUIElement(BattleElement);
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
		GoldStolen += Amount;
		if(GoldStolen > PLAYER_MAX_GOLD)
			GoldStolen = PLAYER_MAX_GOLD;

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
		PotentialAction.Unset();
		Action.Unset();
		ResetUIState();

		// If not in battle apply penalty immediately
		if(!Battle) {

			// Apply penalty
			ApplyDeathPenalty(PLAYER_DEATH_GOLD_PENALTY, 0);
		}
	}

	// Mana change
	if(StatChange.HasStat(StatType::MANA))
		Character->UpdateMana(StatChange.Values[StatType::MANA].Integer);

	// Stamina change
	if(StatChange.HasStat(StatType::STAMINA)) {
		TurnTimer += StatChange.Values[StatType::STAMINA].Float;
		TurnTimer = glm::clamp(TurnTimer, 0.0, 1.0);
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
		if(Battle)
			Battle->RemoveFighter(this);
	}

	// Run server only commands
	if(Server) {
		if(!Battle && Character->IsAlive() && StatChange.HasStat(StatType::TELEPORT)) {
			Server->StartTeleport(this, StatChange.Values[StatType::TELEPORT].Float);
		}

		// Start battle
		if(!Battle && StatChange.HasStat(StatType::BATTLE)) {
			Server->QueueBattle(this, (uint32_t)StatChange.Values[StatType::BATTLE].Integer, true, false);
		}

		// Start PVP
		if(!Battle && StatChange.HasStat(StatType::PVP)) {
			Server->QueueBattle(this, 0, false, StatChange.Values[StatType::PVP].Integer);
		}
	}

	return StatusEffect;
}

// Moves the player and returns direction moved
int _Object::Move() {
	if(WaitForServer || Battle || InputStates.size() == 0 || !Character->IsAlive())
		return 0;

	// Check timer
	if(MoveTime < PLAYER_MOVETIME / (Character->MoveSpeed / 100.0))
		return 0;

	MoveTime = 0;
	int InputState = InputStates.front();
	InputStates.pop_front();

	// Get new position
	glm::ivec2 Direction(0, 0);
	GetDirectionFromInput(InputState, Direction);

	// Move player
	if(Map->CanMoveTo(Position + Direction, this)) {
		Position += Direction;
		if(GetTile()->Zone > 0 && Character->Invisible != 1)
			NextBattle--;

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

// Return true if the object has the item unlocked
bool _Object::HasUnlocked(const _Item *Item) const {
	if(!Item)
		return false;

	if(Unlocks.find(Item->UnlockID) != Unlocks.end())
		return true;

	return false;
}

// Gets the tile that the player is currently standing on
const _Tile *_Object::GetTile() const {

	return Map->GetTile(Position);
}

// Get map id, return 0 if none
NetworkIDType _Object::GetMapID() const {
	if(!Map)
		return 0;

	return Map->NetworkID;
}

// Reset ui state variables
void _Object::ResetUIState() {
	InventoryOpen = false;
	SkillsOpen = false;
	MenuOpen = false;
	Vendor = nullptr;
	Trader = nullptr;
	Blacksmith = nullptr;
	Minigame = nullptr;
	TeleportTime = -1.0;
}

// Generates the number of moves until the next battle
void _Object::GenerateNextBattle() {
	NextBattle = GetRandomInt(BATTLE_MINSTEPS, BATTLE_MAXSTEPS);
}

// Stop a battle
void _Object::StopBattle() {
	Battle = nullptr;
	RemoveBattleElement();
}

// Add status effect to object
bool _Object::AddStatusEffect(_StatusEffect *StatusEffect) {
	if(!StatusEffect)
		return false;

	// Find existing buff
	for(auto &ExistingEffect : StatusEffects) {

		// If buff exists, refresh duration
		if(StatusEffect->Buff == ExistingEffect->Buff) {
			if(StatusEffect->Level >= ExistingEffect->Level) {
				ExistingEffect->Duration = StatusEffect->Duration;
				ExistingEffect->Level = StatusEffect->Level;
				ExistingEffect->Time = 0.0;
			}

			return false;
		}
	}

	StatusEffects.push_back(StatusEffect);

	return true;
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
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::STAT_CHANGE);
	StatChange.Serialize(Packet);

	// Send packet to player
	SendPacket(Packet);
}

// Delete memory used by status effects
void _Object::DeleteStatusEffects() {
	for(auto &StatusEffect : StatusEffects)
		delete StatusEffect;

	StatusEffects.clear();
}

// Update death count and gold loss
void _Object::ApplyDeathPenalty(float Penalty, int BountyLoss) {
	int GoldPenalty = BountyLoss + (int)(std::abs(Character->Gold) * Penalty + 0.5f);

	// Update stats
	Character->UpdateGold(-GoldPenalty);
	Record->Deaths++;
	Record->GoldLost += GoldPenalty;
	Record->Bounty -= BountyLoss;
	if(Record->Bounty < 0)
		Record->Bounty = 0;

	// Send message
	if(Server && Peer) {
		Server->SendMessage(Peer, std::string("You lost " + std::to_string(GoldPenalty) + " gold"), "red");
		Server->Log << "Player " << Name << " died and lost " << std::to_string(GoldPenalty) << " gold ( action=death character_id=" << Character->CharacterID << " gold=" << Character->Gold << " deaths=" << Record->Deaths << " )" << std::endl;
	}
}

// Return an action struct from an action bar slot
bool _Object::GetActionFromSkillbar(_Action &ReturnAction, size_t Slot) {
	if(Slot < Character->ActionBar.size()) {
		ReturnAction.Item = Character->ActionBar[Slot].Item;
		if(!ReturnAction.Item)
			return false;

		// Determine if item is a skill, then look at object's skill levels
		if(ReturnAction.Item->IsSkill() && Character->HasLearned(ReturnAction.Item))
			ReturnAction.Level = Character->Skills[ReturnAction.Item->ID];
		else
			ReturnAction.Level = ReturnAction.Item->Level;

		return true;
	}

	return false;
}

// Set action and targets
void _Object::SetActionUsing(_Buffer &Data, _Manager<_Object> *ObjectManager) {

	// Check for needed commands
	if(!Action.IsSet()) {

		uint8_t ActionBarSlot = Data.Read<uint8_t>();
		int TargetCount = Data.Read<uint8_t>();
		if(!TargetCount)
			return;

		// Get skillbar action
		if(!GetActionFromSkillbar(Action, ActionBarSlot))
			return;

		// Get targets
		Targets.clear();
		for(int i = 0; i < TargetCount; i++) {
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();
			_Object *Target = ObjectManager->GetObject(NetworkID);
			if(Target && Action.Item->CanTarget(this, Target))
				Targets.push_back(Target);
		}
	}
}

// Get the percentage to the next level
float _Object::GetNextLevelPercent() const {
	float Percent = 0;

	if(Character->ExperienceNextLevel > 0)
		Percent = 1.0f - (float)Character->ExperienceNeeded / Character->ExperienceNextLevel;

	return Percent;
}

// Accept a trade from a trader
void _Object::AcceptTrader(std::vector<_Slot> &Slots) {
	if(!Trader)
		return;

	// Trade in required items
	for(uint32_t i = 0; i < Trader->Items.size(); i++)
		Inventory->DecrementItemCount(Slots[i], -Trader->Items[i].Count);

	// Give player reward
	Inventory->AddItem(Trader->RewardItem, Trader->Upgrades, Trader->Count);

	// Update player
	Character->CalculateStats();
}

// Generate and send seed to client
void _Object::SendSeed(bool Generate) {
	if(!Server)
		return;

	if(Generate)
		Seed = GetRandomInt((uint32_t)1, std::numeric_limits<uint32_t>::max());

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::MINIGAME_SEED);
	Packet.Write<uint32_t>(Seed);
	Server->Network->SendPacket(Packet, Peer);
}

// Determines if the player can accept movement keys held down
bool _Object::AcceptingMoveInput() {
	if(Battle)
		return false;

	if(WaitForServer)
		return false;

	if(Vendor)
		return false;

	if(Trader)
		return false;

	if(Blacksmith)
		return false;

	if(Minigame)
		return false;

	if(!Character->IsAlive())
		return false;

	return true;
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

// Updates a skill level
void _Object::AdjustSkillLevel(uint32_t SkillID, int Amount) {
	if(SkillID == 0)
		return;

	const _Item *Skill = Stats->Items.at(SkillID);
	if(Skill == nullptr)
		return;

	// Buying
	if(Amount > 0) {

		// Cap points
		int PointsToSpend = std::min(GetSkillPointsAvailable(), Amount);
		PointsToSpend = std::min(PointsToSpend, Skill->MaxLevel - Character->Skills[SkillID]);

		// Update level
		Character->Skills[SkillID] += PointsToSpend;
	}
	else if(Amount < 0) {

		// Update level
		Character->Skills[SkillID] += Amount;
		if(Character->Skills[SkillID] < 0)
			Character->Skills[SkillID] = 0;

		// Update action bar
		if(Character->Skills[SkillID] == 0) {
			for(size_t i = 0; i < Character->ActionBar.size(); i++) {
				if(Character->ActionBar[i].Item == Skill) {
					Character->ActionBar[i].Unset();
					break;
				}
			}
		}
	}
}

// Can enter battle
bool _Object::CanBattle() const {
	return !Battle && Status == STATUS_NONE && Character->Invisible <= 0;
}

// Send packet to player or broadcast during battle
void _Object::SendPacket(_Buffer &Packet) {
	if(Battle)
		Battle->BroadcastPacket(Packet);
	else if(Peer)
		Server->Network->SendPacket(Packet, Peer);
}

// Create list of nodes to destination
bool _Object::Pathfind(const glm::ivec2 &StartPosition, const glm::ivec2 &EndPosition) {
	if(!Map || !Map->Pather)
		return false;

	if(Path.size())
		return true;

	float TotalCost;
	std::vector<void *> PathFound;
	int Result = Map->Pather->Solve(Map->PositionToNode(StartPosition), Map->PositionToNode(EndPosition), &PathFound, &TotalCost);
	if(Result == micropather::MicroPather::SOLVED) {

		// Convert vector to list
		Path.clear();
		for(auto &Node : PathFound)
			Path.push_back(Node);

		return true;
	}

	return false;
}

// Return an input state from the next node in the path list
int _Object::GetInputStateFromPath() {
	int InputState = 0;

	// Find current position in list
	for(auto Iterator = Path.begin(); Iterator != Path.end(); ++Iterator) {
		glm::ivec2 NodePosition;
		Map->NodeToPosition(*Iterator, NodePosition);

		if(Position == NodePosition) {
			auto NextIterator = std::next(Iterator, 1);
			if(NextIterator == Path.end()) {
				Path.clear();
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
