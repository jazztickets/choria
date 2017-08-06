/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <objects/inventory.h>
#include <objects/statuseffect.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <ui/element.h>
#include <network/servernetwork.h>
#include <buffer.h>
#include <assets.h>
#include <graphics.h>
#include <hud.h>
#include <server.h>
#include <random.h>
#include <stats.h>
#include <font.h>
#include <utils.h>
#include <scripting.h>
#include <program.h>
#include <constants.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <json/reader.h>

// Constructor
_Object::_Object() :
	Stats(nullptr),
	Map(nullptr),
	HUD(nullptr),
	Scripting(nullptr),
	Server(nullptr),
	Peer(nullptr),
	Moved(0),
	WaitForServer(false),
	CheckEvent(false),
	Paused(false),
	MoveTime(0),
	Position(0, 0),
	ServerPosition(0, 0),

	BaseMaxHealth(0),
	BaseMaxMana(0),
	BaseHealthRegen(0),
	BaseManaRegen(0),
	BaseHealPower(1.0f),
	BaseAttackPower(1.0f),
	BaseMinDamage(0),
	BaseMaxDamage(0),
	BaseArmor(0),
	BaseDamageBlock(0),
	BaseMoveSpeed(100),
	BaseBattleSpeed(100),
	BaseEvasion(0),
	BaseHitChance(100),
	BaseDropRate(0),

	UpdateTimer(0.0),
	CalcLevelStats(true),

	Name(""),
	Level(0),
	Health(1),
	MaxHealth(1),
	Mana(0),
	MaxMana(0),
	HealPower(0.0f),
	AttackPower(0.0f),
	MinDamage(0),
	MaxDamage(0),
	Armor(0),
	DamageBlock(0),
	MoveSpeed(100),
	BattleSpeed(100),
	Evasion(0),
	HitChance(100),
	DropRate(0),

	PlayTime(0.0),
	BattleTime(0.0),
	Hardcore(false),
	Deaths(0),
	MonsterKills(0),
	PlayerKills(0),
	Bounty(0),
	Gold(0),
	GoldLost(0),
	Experience(0),
	ExperienceNeeded(0),
	ExperienceNextLevel(0),

	Battle(nullptr),
	BattleElement(nullptr),
	JoinedBattle(false),
	TurnTimer(0.0),
	BattleSide(0),
	Portrait(nullptr),
	Model(nullptr),
	BattleOffset(0, 0),

	Owner(nullptr),
	DatabaseID(0),
	ExperienceGiven(0),
	GoldGiven(0),
	AI(""),

	CharacterID(0),

	Status(0),
	PortraitID(0),
	ModelID(0),
	ModelTexture(nullptr),
	StatusTexture(nullptr),
	LoadMapID(0),
	SpawnMapID(1),
	SpawnPoint(0),
	TeleportTime(-1),

	NextBattle(0),
	AttackPlayerTime(0),
	Invisible(0),
	Stunned(0),
	InventoryOpen(false),
	Inventory(nullptr),
	Vendor(nullptr),
	Trader(nullptr),
	Blacksmith(nullptr),
	SkillsOpen(false),
	SkillPoints(0),
	SkillPointsUsed(0),
	SkillPointsOnActionBar(0),
	TradeGold(0),
	WaitingForTrade(false),
	TradeAccepted(false),
	TradePlayer(nullptr) {

	LastTarget[0] = nullptr;
	LastTarget[1] = nullptr;

	Inventory = new _Inventory();
}

// Destructor
_Object::~_Object() {
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

	DeleteStatusEffects();
	RemoveBattleElement();
}

// Updates the player
void _Object::Update(double FrameTime) {
	if(!Server)
		ClientMessage.resize(0);

	CheckEvent = false;

	// Update player position
	Moved = Move();
	if(Moved) {
		CheckEvent = true;
	}

	// Update status
	Status = STATUS_NONE;
	if(!IsAlive())
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
	else if(InventoryOpen)
		Status = STATUS_INVENTORY;
	else if(SkillsOpen)
		Status = STATUS_SKILLS;
	else if(Paused)
		Status = STATUS_PAUSE;

	// Update actions and battle
	if(IsAlive()) {

		// Update AI
		if(Server && Battle)
			UpdateAI(Battle->Fighters, FrameTime);

		// Check turn timer
		if(Battle) {
			if(!Stunned)
				TurnTimer += FrameTime * BATTLE_DEFAULTSPEED * BattleSpeed / 100.0;
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
			if(Server && IsAlive()) {
				ResolveBuff(StatusEffect, "Update");
			}
		}

		// Reduce count
		StatusEffect->Duration -= FrameTime;
		if(StatusEffect->Duration <= 0 || !IsAlive()) {

			delete StatusEffect;
			Iterator = StatusEffects.erase(Iterator);

			CalculateStats();
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

	// Generic update timer
	UpdateTimer += FrameTime;
	if(UpdateTimer >= 1.0) {
		UpdateTimer -= 1.0;

		// Update stats
		if(Server && IsAlive()) {
			_StatChange StatChange;
			StatChange.Object = this;

			// Update regen
			if(Health < MaxHealth && HealthRegen != 0)
				StatChange.Values[StatType::HEALTH].Integer = HealthRegen;
			if(Mana < MaxMana && ManaRegen != 0)
				StatChange.Values[StatType::MANA].Integer = ManaRegen;

			// Update object
			if(StatChange.GetChangedFlag() != 0) {
				UpdateStats(StatChange);

				// Build packet
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::STAT_CHANGE);
				StatChange.Serialize(Packet);

				// Send packet to player
				SendPacket(Packet);
			}
		}
	}

	// Update timers
	MoveTime += FrameTime;
	AttackPlayerTime += FrameTime;

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
	PlayTime += FrameTime;
	if(Battle)
		BattleTime += FrameTime;

	// Check events
	if(Map && CheckEvent)
		Map->CheckEvents(this);
}

// Update AI during battle
void _Object::UpdateAI(const std::list<_Object *> &Fighters, double FrameTime) {
	if(!AI.length())
		return;

	// Call AI script to get action
	if(TurnTimer >= 1.0 && !Action.IsSet()) {

		// Separate fighter list
		std::list<_Object *> Enemies, Allies;
		for(const auto &Fighter : Fighters) {
			if(Fighter->Deleted)
				continue;

			if(Fighter->BattleSide == BattleSide)
				Allies.push_back(Fighter);
			else if(Fighter->IsAlive())
				Enemies.push_back(Fighter);
		}

		// Call lua script
		if(Enemies.size()) {
			if(Scripting->StartMethodCall(AI, "Update")) {
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
		if(Invisible > 0)
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
			Assets.Fonts["hud_medium"]->DrawText(Name, glm::vec2(DrawPosition) + glm::vec2(0, -0.5f), Color, CENTER_BASELINE, 1.0f / ModelTexture->Size.x);
		}
	}
}

// Renders the fighter during a battle
void _Object::RenderBattle(_Object *ClientPlayer, double Time) {
	glm::vec4 GlobalColor(COLOR_WHITE);
	GlobalColor.a = 1.0f;
	if(!IsAlive())
		GlobalColor.a = 0.2f;

	// Draw slot
	BattleElement->Fade = GlobalColor.a;

	// Get slot center
	glm::vec2 SlotPosition = BattleElement->Bounds.Start;

	// Save positions
	ResultPosition = BattleElement->Bounds.Start + BattleElement->Size / 2.0f;
	StatPosition = ResultPosition + glm::vec2(Portrait->Size.x/2 + 10 + BATTLE_HEALTHBAR_WIDTH/2, -Portrait->Size.y/2);

	// Name
	Assets.Fonts["hud_medium"]->DrawText(Name, SlotPosition + glm::vec2(0, -12), GlobalColor, LEFT_BASELINE);
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
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * GetHealthPercent(), BarSize.y) + BarOffset;
	Graphics.DrawImage(BarBounds, Assets.Elements["image_hud_health_bar_full"]->Texture, true);

	// Draw health text
	std::stringstream Buffer;
	Buffer << Round(Health) << " / " << Round(MaxHealth);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str(), BarCenter + glm::vec2(0, 5), GlobalColor, CENTER_BASELINE);
	Buffer.str("");

	// Draw mana
	if(MaxMana > 0) {
		float ManaPercent = MaxMana > 0 ? Mana / (float)MaxMana : 0;

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
		Buffer << Round(Mana) << " / " << Round(MaxMana);
		Assets.Fonts["hud_small"]->DrawText(Buffer.str(), BarCenter + glm::vec2(0, 5), GlobalColor, CENTER_BASELINE);
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
	const _Texture *ItemBackTexture = Assets.Textures["hud/item_back.png"];

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
			glm::vec4 Color(COLOR_WHITE);
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
	StatsNode["hardcore"] = Hardcore;
	StatsNode["map_x"] = Position.x;
	StatsNode["map_y"] = Position.y;
	StatsNode["spawnmap_id"] = SpawnMapID;
	StatsNode["spawnpoint"] = SpawnPoint;
	StatsNode["portrait_id"] = PortraitID;
	StatsNode["model_id"] = ModelID;
	StatsNode["actionbar_size"] = (Json::Value::UInt64)ActionBar.size();
	StatsNode["health"] = Health;
	StatsNode["mana"] = Mana;
	StatsNode["experience"] = Experience;
	StatsNode["gold"] = Gold;
	StatsNode["goldlost"] = GoldLost;
	StatsNode["playtime"] = PlayTime;
	StatsNode["battletime"] = BattleTime;
	StatsNode["deaths"] = Deaths;
	StatsNode["monsterkills"] = MonsterKills;
	StatsNode["playerkills"] = PlayerKills;
	StatsNode["bounty"] = Bounty;
	StatsNode["nextbattle"] = NextBattle;
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
	for(auto &Skill : Skills) {
		Json::Value SkillNode;
		SkillNode["id"] = Skill.first;
		SkillNode["level"] = Skill.second;
		SkillsNode.append(SkillNode);
	}
	Data["skills"] = SkillsNode;

	// Write action bar
	Json::Value ActionBarNode;
	for(size_t i = 0; i < ActionBar.size(); i++) {
		if(ActionBar[i].IsSet()) {
			Json::Value ActionNode;
			ActionNode["slot"] = (Json::Value::UInt64)i;
			ActionNode["id"] = ActionBar[i].Item ? ActionBar[i].Item->ID : 0;
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
	Hardcore = StatsNode["hardcore"].asBool();
	PortraitID = StatsNode["portrait_id"].asUInt();
	ModelID = StatsNode["model_id"].asUInt();
	Health = StatsNode["health"].asInt();
	Mana = StatsNode["mana"].asInt();
	Experience = StatsNode["experience"].asInt();
	Gold = StatsNode["gold"].asInt();
	GoldLost = StatsNode["goldlost"].asInt();
	PlayTime = StatsNode["playtime"].asDouble();
	BattleTime = StatsNode["battletime"].asDouble();
	Deaths = StatsNode["deaths"].asInt();
	MonsterKills = StatsNode["monsterkills"].asInt();
	PlayerKills = StatsNode["playerkills"].asInt();
	Bounty = StatsNode["bounty"].asInt();
	NextBattle = StatsNode["nextbattle"].asInt();

	size_t ActionBarSize = 0;
	ActionBarSize = StatsNode["actionbar_size"].asUInt64();
	ActionBar.resize(ActionBarSize);

	// Set items
	for(Json::ValueIterator BagNode = Data["items"].begin(); BagNode != Data["items"].end(); BagNode++) {
		for(const Json::Value &ItemNode : *BagNode) {
			_InventorySlot InventorySlot;
			InventorySlot.Item = Stats->Items[ItemNode["id"].asUInt()];
			InventorySlot.Upgrades = ItemNode["upgrades"].asInt();
			InventorySlot.Count = ItemNode["count"].asInt();
			Inventory->Bags[std::stoul(BagNode.name())].Slots[ItemNode["slot"].asUInt64()] = InventorySlot;
		}
	}

	// Set skills
	for(const Json::Value &SkillNode : Data["skills"]) {
		uint32_t ItemID = SkillNode["id"].asUInt();
		Skills[ItemID] = std::min(SkillNode["level"].asInt(), Stats->Items[ItemID]->MaxLevel);
	}

	// Set actionbar
	for(const Json::Value &ActionNode : Data["actionbar"]) {
		uint32_t Slot = ActionNode["slot"].asUInt();
		if(Slot < ActionBar.size())
			ActionBar[Slot].Item = Stats->Items[ActionNode["id"].asUInt()];
	}

	// Set status effects
	for(const Json::Value &StatusEffectNode : Data["statuseffects"]) {
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Buff = Stats->Buffs[StatusEffectNode["id"].asUInt()];
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
	return GetRandomInt(MinDamage, MaxDamage);
}

// Create a UI element for battle
void _Object::CreateBattleElement(_Element *Parent) {
	if(BattleElement)
		throw std::runtime_error("_Object::CreateBattleElement: BattleElement already exists!");

	BattleElement = new _Element();
	BattleElement->Type = _Element::ELEMENT;
	BattleElement->Identifier = "battle_element";
	BattleElement->Size = glm::vec2(64, 64);
	BattleElement->Offset = BattleOffset;
	BattleElement->Alignment = CENTER_MIDDLE;
	BattleElement->Visible = true;
	BattleElement->UserData = (void *)_HUD::WINDOW_HUD_EFFECTS;
	BattleElement->UserDataAlt = this;
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
			Child->Parent = nullptr;
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
	Data.WriteBit(Invisible);
}

// Serialize for ObjectUpdate
void _Object::SerializeUpdate(_Buffer &Data) {
	Data.Write<NetworkIDType>(NetworkID);
	Data.Write<glm::ivec2>(Position);
	Data.Write<uint8_t>(Status);
	Data.WriteBit(Invisible);
}

// Serialize object stats
void _Object::SerializeStats(_Buffer &Data) {
	Data.WriteString(Name.c_str());
	Data.WriteString(PartyName.c_str());
	Data.Write<uint32_t>(PortraitID);
	Data.Write<uint32_t>(ModelID);
	Data.Write<int>(Health);
	Data.Write<int>(MaxHealth);
	Data.Write<int>(Mana);
	Data.Write<int>(MaxMana);
	Data.Write<int>(Experience);
	Data.Write<int>(Gold);
	Data.Write<int>(GoldLost);
	Data.Write<double>(PlayTime);
	Data.Write<double>(BattleTime);
	Data.Write<int>(Deaths);
	Data.Write<int>(MonsterKills);
	Data.Write<int>(PlayerKills);
	Data.Write<int>(Bounty);
	Data.Write<int>(Invisible);
	Data.Write<int>(Hardcore);

	// Write inventory
	Inventory->Serialize(Data);

	// Write skills
	Data.Write<uint32_t>((uint32_t)Skills.size());
	for(const auto &Skill : Skills) {
		Data.Write<uint32_t>(Skill.first);
		Data.Write<int>(Skill.second);
	}

	// Write action bar
	Data.Write<uint8_t>((uint8_t)ActionBar.size());
	for(size_t i = 0; i < ActionBar.size(); i++) {
		ActionBar[i].Serialize(Data);
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
	Data.Write<int>(Health);
	Data.Write<int>(MaxHealth);
	Data.Write<int>(Mana);
	Data.Write<int>(MaxMana);
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
	Invisible = Data.ReadBit();

	Portrait = Stats->GetPortraitImage(PortraitID);
	ModelTexture = Stats->Models[ModelID].Texture;
}

// Unserialize object stats
void _Object::UnserializeStats(_Buffer &Data) {
	Name = Data.ReadString();
	PartyName = Data.ReadString();
	PortraitID = Data.Read<uint32_t>();
	ModelID = Data.Read<uint32_t>();
	Health = Data.Read<int>();
	BaseMaxHealth = MaxHealth = Data.Read<int>();
	Mana = Data.Read<int>();
	BaseMaxMana = MaxMana = Data.Read<int>();
	Experience = Data.Read<int>();
	Gold = Data.Read<int>();
	GoldLost = Data.Read<int>();
	PlayTime = Data.Read<double>();
	BattleTime = Data.Read<double>();
	Deaths = Data.Read<int>();
	MonsterKills = Data.Read<int>();
	PlayerKills = Data.Read<int>();
	Bounty = Data.Read<int>();
	Invisible = Data.Read<int>();
	Hardcore = Data.Read<int>();

	ModelTexture = Stats->Models[ModelID].Texture;

	// Read inventory
	Inventory->Unserialize(Data, Stats);

	// Read skills
	uint32_t SkillCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < SkillCount; i++) {
		uint32_t SkillID = Data.Read<uint32_t>();
		int Points = Data.Read<int>();
		Skills[SkillID] = Points;
	}

	// Read action bar
	size_t ActionBarSize = Data.Read<uint8_t>();
	ActionBar.resize(ActionBarSize);
	for(size_t i = 0; i < ActionBarSize; i++)
		ActionBar[i].Unserialize(Data, Stats);

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

	RefreshActionBarCount();
	CalculateStats();
}

// Unserialize battle stats
void _Object::UnserializeBattle(_Buffer &Data) {
	InputStates.clear();

	// Get fighter type
	Position = ServerPosition = Data.Read<glm::ivec2>();
	TurnTimer = Data.Read<double>();
	Health = Data.Read<int>();
	BaseMaxHealth = MaxHealth = Data.Read<int>();
	Mana = Data.Read<int>();
	BaseMaxMana = MaxMana = Data.Read<int>();
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

		CalculateStats();
	}

	// Update gold
	if(StatChange.HasStat(StatType::GOLD)) {
		UpdateGold(StatChange.Values[StatType::GOLD].Integer);
	}

	// Update experience
	if(StatChange.HasStat(StatType::EXPERIENCE)) {
		UpdateExperience(StatChange.Values[StatType::EXPERIENCE].Integer);
	}

	// Update health
	bool WasAlive = IsAlive();
	if(StatChange.HasStat(StatType::HEALTH))
		UpdateHealth(StatChange.Values[StatType::HEALTH].Integer);

	// Just died
	if(WasAlive && !IsAlive()) {
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
		UpdateMana(StatChange.Values[StatType::MANA].Integer);

	// Stamina change
	if(StatChange.HasStat(StatType::STAMINA)) {
		TurnTimer += StatChange.Values[StatType::STAMINA].Float;
		TurnTimer = glm::clamp(TurnTimer, 0.0, 1.0);
	}

	// Action bar upgrade
	if(StatChange.HasStat(StatType::ACTIONBARSIZE)) {
		size_t NewSize = ActionBar.size() + (size_t)StatChange.Values[StatType::ACTIONBARSIZE].Integer;
		if(NewSize >= ACTIONBAR_MAX_SIZE)
			NewSize = ACTIONBAR_MAX_SIZE;

		ActionBar.resize(NewSize);
	}

	// Flee from battle
	if(StatChange.HasStat(StatType::FLEE)) {
		if(Battle)
			Battle->RemoveFighter(this);
	}

	// Run server only commands
	if(Server) {
		if(!Battle && IsAlive() && StatChange.HasStat(StatType::TELEPORT)) {
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

// Update health
void _Object::UpdateHealth(int &Value) {
	if(Server && Value > 0)
		Value *= HealPower;

	Health += Value;

	if(Health < 0)
		Health = 0;
	else if(Health > MaxHealth)
		Health = MaxHealth;
}

// Update mana
void _Object::UpdateMana(int Value) {
	Mana += Value;

	if(Mana < 0)
		Mana = 0;
	else if(Mana > MaxMana)
		Mana = MaxMana;
}

// Moves the player
int _Object::Move() {
	if(WaitForServer || Battle || InputStates.size() == 0 || !IsAlive())
		return 0;

	// Check timer
	if(MoveTime < PLAYER_MOVETIME / (MoveSpeed / 100.0))
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
		if(GetTile()->Zone > 0 && Invisible != 1)
			NextBattle--;

		return InputState;
	}

	return 0;
}

// Return true if the object has the skill unlocked
bool _Object::HasLearned(const _Item *Skill) const {
	if(!Skill)
		return false;

	if(Skills.find(Skill->ID) != Skills.end())
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
const _Tile *_Object::GetTile() {

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
	Paused = false;
	Vendor = nullptr;
	Trader = nullptr;
	Blacksmith = nullptr;
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

// Update gold amount
void _Object::UpdateGold(int Value) {

	Gold += Value;
	if(Gold > PLAYER_MAX_GOLD)
		Gold = PLAYER_MAX_GOLD;
}

// Update experience
void _Object::UpdateExperience(int Value) {

	Experience += Value;
	if(Experience < 0)
		Experience = 0;
}

// Update death count and gold loss
void _Object::ApplyDeathPenalty(float Penalty, int BountyLoss) {
	int GoldPenalty = BountyLoss + (int)(std::abs(Gold) * Penalty + 0.5f);

	// Update stats
	Deaths++;
	UpdateGold(-GoldPenalty);
	GoldLost += GoldPenalty;
	Bounty -= BountyLoss;
	if(Bounty < 0)
		Bounty = 0;

	// Send message
	if(Server && Peer)
		Server->SendMessage(Peer, std::string("You lost " + std::to_string(GoldPenalty) + " gold"), COLOR_RED);
}

// Update counts on action bar
void _Object::RefreshActionBarCount() {
	SkillPointsOnActionBar = 0;
	for(size_t i = 0; i < ActionBar.size(); i++) {
		const _Item *Item = ActionBar[i].Item;
		if(Item) {
			if(Item->IsSkill() && HasLearned(Item))
				SkillPointsOnActionBar += Skills[Item->ID];
			else
				ActionBar[i].Count = Inventory->CountItem(Item);
		}
		else
			ActionBar[i].Count = 0;
	}
}

// Return an action struct from an action bar slot
bool _Object::GetActionFromSkillbar(_Action &ReturnAction, size_t Slot) {
	if(Slot < ActionBar.size()) {
		ReturnAction.Item = ActionBar[Slot].Item;
		if(!ReturnAction.Item)
			return false;

		// Determine if item is a skill, then look at object's skill levels
		if(ReturnAction.Item->IsSkill() && HasLearned(ReturnAction.Item))
			ReturnAction.Level = Skills[ReturnAction.Item->ID];
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

	if(ExperienceNextLevel > 0)
		Percent = 1.0f - (float)ExperienceNeeded / ExperienceNextLevel;

	return Percent;
}

// Accept a trade from a trader
void _Object::AcceptTrader(std::vector<_Slot> &Slots) {
	if(!Trader)
		return;

	// Trade in required items
	for(uint32_t i = 0; i < Trader->TraderItems.size(); i++)
		Inventory->DecrementItemCount(Slots[i], -Trader->TraderItems[i].Count);

	// Give player reward
	Inventory->AddItem(Trader->RewardItem, Trader->Upgrades, Trader->Count);

	// Update player
	CalculateStats();
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

	if(!IsAlive())
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

	const _Item *Skill = Stats->Items[SkillID];
	if(Skill == nullptr)
		return;

	// Buying
	if(Amount > 0) {

		// Cap points
		int PointsToSpend = std::min(GetSkillPointsRemaining(), Amount);
		PointsToSpend = std::min(PointsToSpend, Skill->MaxLevel - Skills[SkillID]);

		// Update level
		Skills[SkillID] += PointsToSpend;
	}
	else if(Amount < 0) {

		// Update level
		Skills[SkillID] += Amount;
		if(Skills[SkillID] < 0)
			Skills[SkillID] = 0;

		// Update action bar
		if(Skills[SkillID] == 0) {
			for(size_t i = 0; i < ActionBar.size(); i++) {
				if(ActionBar[i].Item == Skill) {
					ActionBar[i].Unset();
					break;
				}
			}
		}
	}
}

// Can enter battle
bool _Object::CanBattle() const {
	return !Battle && Status == STATUS_NONE && Invisible <= 0;
}

// Calculates all of the player stats
void _Object::CalculateStats() {

	// Get base stats
	CalculateLevelStats();

	MaxHealth = BaseMaxHealth;
	MaxMana = BaseMaxMana;
	HealthRegen = BaseHealthRegen;
	ManaRegen = BaseManaRegen;
	HealPower = BaseHealPower;
	AttackPower = BaseAttackPower;
	BattleSpeed = 0;
	Evasion = BaseEvasion;
	HitChance = BaseHitChance;
	MinDamage = BaseMinDamage;
	MaxDamage = BaseMaxDamage;
	Armor = BaseArmor;
	DamageBlock = BaseDamageBlock;
	MoveSpeed = BaseMoveSpeed;
	DropRate = BaseDropRate;
	Resistances.clear();

	Invisible = 0;
	Stunned = 0;

	// Get item stats
	int ItemMinDamage = 0;
	int ItemMaxDamage = 0;
	int ItemArmor = 0;
	int ItemDamageBlock = 0;
	float WeaponDamageModifier = 1.0f;
	_Bag &EquipmentBag = Inventory->Bags[_Bag::EQUIPMENT];
	for(size_t i = 0; i < EquipmentBag.Slots.size(); i++) {

		// Check each item
		const _Item *Item = EquipmentBag.Slots[i].Item;
		int Upgrades = EquipmentBag.Slots[i].Upgrades;
		if(Item) {

			// Add damage
			if(Item->Type != ItemType::SHIELD) {
				ItemMinDamage += Item->GetMinDamage(Upgrades);
				ItemMaxDamage += Item->GetMaxDamage(Upgrades);
			}

			// Add defense
			ItemArmor += Item->GetArmor(Upgrades);
			ItemDamageBlock += Item->GetDamageBlock(Upgrades);

			// Stat changes
			MaxHealth += Item->GetMaxHealth(Upgrades);
			MaxMana += Item->GetMaxMana(Upgrades);
			HealthRegen += Item->GetHealthRegen(Upgrades);
			ManaRegen += Item->GetManaRegen(Upgrades);
			BattleSpeed += Item->GetBattleSpeed(Upgrades);
			MoveSpeed += Item->GetMoveSpeed(Upgrades);
			DropRate += Item->GetDropRate(Upgrades);

			// Add resistances
			Resistances[Item->ResistanceTypeID] += Item->GetResistance(Upgrades);
		}
	}

	SkillPointsUsed = 0;
	for(const auto &SkillLevel : Skills) {
		const _Item *Skill = Stats->Items[SkillLevel.first];
		if(Skill)
			SkillPointsUsed += SkillLevel.second;
	}

	// Get skill bonus
	for(size_t i = 0; i < ActionBar.size(); i++) {
		_ActionResult ActionResult;
		ActionResult.Source.Object = this;
		if(GetActionFromSkillbar(ActionResult.ActionUsed, i)) {
			const _Item *Skill = ActionResult.ActionUsed.Item;
			if(Skill->IsSkill() && Skill->TargetID == TargetType::NONE) {

				// Get passive stat changes
				Skill->GetStats(Scripting, ActionResult);
				CalculateStatBonuses(ActionResult.Source);
			}
		}
	}

	// Get buff stats
	for(const auto &StatusEffect : StatusEffects) {
		_StatChange StatChange;
		StatChange.Object = this;
		StatusEffect->Buff->ExecuteScript(Scripting, "Stats", StatusEffect->Level, StatChange);
		CalculateStatBonuses(StatChange);
	}

	// Get damage
	MinDamage += (int)std::roundf(ItemMinDamage * WeaponDamageModifier);
	MaxDamage += (int)std::roundf(ItemMaxDamage * WeaponDamageModifier);
	MinDamage = std::max(MinDamage, 0);
	MaxDamage = std::max(MaxDamage, 0);

	// Get defense
	Armor += ItemArmor;
	DamageBlock += ItemDamageBlock;
	DamageBlock = std::max(DamageBlock, 0);

	// Cap resistances
	for(auto &Resist : Resistances) {
		Resist.second = std::min(Resist.second, GAME_MAX_RESISTANCE);
		Resist.second = std::max(Resist.second, -GAME_MAX_RESISTANCE);
	}

	// Get physical resistance from armor
	float ArmorResist = Armor / (30.0f + std::abs(Armor));

	// Physical resist comes solely from armor
	Resistances[2] = (int)(ArmorResist * 100);

	BattleSpeed = (int)(BaseBattleSpeed * BattleSpeed / 100.0 + BaseBattleSpeed);
	if(BattleSpeed < BATTLE_MIN_SPEED)
		BattleSpeed = BATTLE_MIN_SPEED;

	if(MoveSpeed < PLAYER_MIN_MOVESPEED)
		MoveSpeed = PLAYER_MIN_MOVESPEED;

	Health = std::min(Health, MaxHealth);
	Mana = std::min(Mana, MaxMana);

	RefreshActionBarCount();
}

// Update an object's stats from a statchange
void _Object::CalculateStatBonuses(_StatChange &StatChange) {
	if(StatChange.HasStat(StatType::MAXHEALTH))
		MaxHealth += StatChange.Values[StatType::MAXHEALTH].Integer;
	if(StatChange.HasStat(StatType::MAXMANA))
		MaxMana += StatChange.Values[StatType::MAXMANA].Integer;
	if(StatChange.HasStat(StatType::HEALTHREGEN))
		HealthRegen += StatChange.Values[StatType::HEALTHREGEN].Integer;
	if(StatChange.HasStat(StatType::MANAREGEN))
		ManaRegen += StatChange.Values[StatType::MANAREGEN].Integer;

	if(StatChange.HasStat(StatType::HEALPOWER))
		HealPower += StatChange.Values[StatType::HEALPOWER].Float;
	if(StatChange.HasStat(StatType::ATTACKPOWER))
		AttackPower += StatChange.Values[StatType::ATTACKPOWER].Float;

	if(StatChange.HasStat(StatType::BATTLESPEED))
		BattleSpeed += StatChange.Values[StatType::BATTLESPEED].Integer;
	if(StatChange.HasStat(StatType::HITCHANCE))
		HitChance += StatChange.Values[StatType::HITCHANCE].Integer;
	if(StatChange.HasStat(StatType::EVASION))
		Evasion += StatChange.Values[StatType::EVASION].Integer;
	if(StatChange.HasStat(StatType::STUNNED))
		Stunned = StatChange.Values[StatType::STUNNED].Integer;

	if(StatChange.HasStat(StatType::RESISTTYPE))
		Resistances[(uint32_t)StatChange.Values[StatType::RESISTTYPE].Integer] += StatChange.Values[StatType::RESIST].Integer;

	if(StatChange.HasStat(StatType::MINDAMAGE))
		MinDamage += StatChange.Values[StatType::MINDAMAGE].Integer;
	if(StatChange.HasStat(StatType::MAXDAMAGE))
		MaxDamage += StatChange.Values[StatType::MAXDAMAGE].Integer;
	if(StatChange.HasStat(StatType::ARMOR))
		Armor += StatChange.Values[StatType::ARMOR].Integer;
	if(StatChange.HasStat(StatType::DAMAGEBLOCK))
		DamageBlock += StatChange.Values[StatType::DAMAGEBLOCK].Integer;

	if(StatChange.HasStat(StatType::MOVESPEED))
		MoveSpeed += StatChange.Values[StatType::MOVESPEED].Integer;

	if(StatChange.HasStat(StatType::DROPRATE))
		DropRate += StatChange.Values[StatType::DROPRATE].Integer;

	if(StatChange.HasStat(StatType::INVISIBLE))
		Invisible = StatChange.Values[StatType::INVISIBLE].Integer;
}

// Calculates the base level stats
void _Object::CalculateLevelStats() {
	if(!Stats || !CalcLevelStats)
		return;

	// Cap min experience
	if(Experience < 0)
		Experience = 0;

	// Cap max experience
	const _Level *MaxLevelStat = Stats->GetLevel(Stats->GetMaxLevel());
	if(Experience > MaxLevelStat->Experience)
		Experience = MaxLevelStat->Experience;

	// Find current level
	const _Level *LevelStat = Stats->FindLevel(Experience);
	Level = LevelStat->Level;
	BaseMaxHealth = LevelStat->Health;
	BaseMaxMana = LevelStat->Mana;
	BaseMinDamage = LevelStat->Damage;
	BaseMaxDamage = LevelStat->Damage+1;
	BaseArmor = LevelStat->Armor;
	BaseDamageBlock = 0;
	SkillPoints = LevelStat->SkillPoints;
	ExperienceNextLevel = LevelStat->NextLevel;
	if(Level == Stats->GetMaxLevel())
		ExperienceNeeded = 0;
	else
		ExperienceNeeded = LevelStat->NextLevel - (Experience - LevelStat->Experience);
}

// Send packet to player or broadcast during battle
void _Object::SendPacket(_Buffer &Packet) {
	if(Battle)
		Battle->BroadcastPacket(Packet);
	else if(Peer)
		Server->Network->SendPacket(Packet, Peer);
}

// Create list of nodes to destination
bool _Object::MoveTo(const glm::ivec2 &StartPosition, const glm::ivec2 &EndPosition) {
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
