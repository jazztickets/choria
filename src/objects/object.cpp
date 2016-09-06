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
#include <ui/image.h>
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

	BaseMaxHealth(0.0f),
	BaseMaxMana(0.0f),
	BaseHealthRegen(0.0f),
	BaseManaRegen(0.0f),
	BaseHealPower(1.0f),
	BaseMinDamage(0),
	BaseMaxDamage(0),
	BaseMinDefense(0),
	BaseMaxDefense(0),
	BaseMoveSpeed(1.0f),
	BaseBattleSpeed(1.0),
	BaseEvasion(0.0f),
	BaseHitChance(1.0f),

	UpdateTimer(0.0),
	CalcLevelStats(true),

	Name(""),
	Level(0),
	Health(1.0f),
	MaxHealth(1.0f),
	Mana(0.0f),
	MaxMana(0.0f),
	HealPower(0.0f),
	MinDamage(0),
	MaxDamage(0),
	MinDefense(0),
	MaxDefense(0),
	MoveSpeed(1.0f),
	BattleSpeed(1.0),
	Evasion(0.0f),
	HitChance(1.0f),

	PlayTime(0.0),
	Deaths(0),
	MonsterKills(0),
	PlayerKills(0),
	Bounty(0),
	Gold(0),
	Experience(0),
	ExperienceNeeded(0),
	ExperienceNextLevel(0),

	Battle(nullptr),
	BattleElement(nullptr),
	TurnTimer(0.0),
	AITimer(1.0),
	BattleSide(0),
	LastTarget(nullptr),
	Portrait(nullptr),
	BattleOffset(0, 0),

	DatabaseID(0),
	ExperienceGiven(0),
	GoldGiven(0),
	AI(""),

	CharacterID(0),

	Status(0),
	PortraitID(0),
	WorldTexture(nullptr),
	StatusTexture(nullptr),
	LoadMapID(0),
	SpawnMapID(1),
	SpawnPoint(0),
	TeleportTime(-1),

	NextBattle(0),
	AttackPlayerTime(0),
	Invisible(0),
	InventoryOpen(false),
	Inventory(nullptr),
	Vendor(nullptr),
	Trader(nullptr),
	SkillsOpen(false),
	SkillPoints(0),
	SkillPointsUsed(0),
	SkillPointsOnActionBar(0),
	TradeGold(0),
	WaitingForTrade(false),
	TradeAccepted(false),
	TradePlayer(nullptr) {

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
		if(Battle)
			TurnTimer += FrameTime * BATTLE_DEFAULTSPEED * BattleSpeed;
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

	// Generic update timer
	UpdateTimer += FrameTime;
	if(UpdateTimer >= 1.0) {
		UpdateTimer -= 1.0;

		// Update stats
		if(Server && IsAlive()) {
			_StatChange StatChange;
			StatChange.Object = this;

			// Update regen
			if(Health < MaxHealth && HealthRegen != 0.0f)
				StatChange.Values[StatType::HEALTH].Float = HealthRegen;
			if(Mana < MaxMana && ManaRegen != 0.0f)
				StatChange.Values[StatType::MANA].Float = ManaRegen;

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

	// Check events
	if(Map && CheckEvent)
		Map->CheckEvents(this);
}

// Update AI during battle
void _Object::UpdateAI(const std::list<_Object *> &Fighters, double FrameTime) {
	if(!AI.length())
		return;

	// Update AI every second
	AITimer += FrameTime;
	if(AITimer >= BATTLE_AI_UPDATE_PERIOD) {
		AITimer = 0.0;

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
	if(Map && WorldTexture) {

		float Alpha = 1.0f;
		if(Invisible > 0)
			Alpha = PLAYER_INVIS_ALPHA;

		Graphics.SetProgram(Assets.Programs["pos_uv"]);
		glUniformMatrix4fv(Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

		Graphics.SetVBO(VBO_QUAD);

		glm::vec3 DrawPosition;
		if(HUD && HUD->ShowStats) {
			DrawPosition = glm::vec3(ServerPosition, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
			Graphics.SetColor(glm::vec4(1, 0, 0, 1));
			Graphics.DrawSprite(DrawPosition, WorldTexture);
		}

		glm::vec4 Color(1.0f, 1.0f, 1.0f, Alpha);
		DrawPosition = glm::vec3(Position, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
		Graphics.SetColor(Color);
		Graphics.DrawSprite(DrawPosition, WorldTexture);
		if(StatusTexture) {
			Graphics.DrawSprite(DrawPosition, StatusTexture);
		}

		if(ClientPlayer != this) {
			Assets.Fonts["hud_medium"]->DrawText(Name, glm::vec2(DrawPosition) + glm::vec2(0, -0.5f), Color, CENTER_BASELINE, 1.0f / WorldTexture->Size.x);
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
	Assets.Fonts["hud_medium"]->DrawText(Name.c_str(), SlotPosition + glm::vec2(0, -12), GlobalColor, LEFT_BASELINE);
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
	Graphics.DrawImage(BarBounds, Assets.Images["image_hud_health_bar_empty"]->Texture, true);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * GetHealthPercent(), BarSize.y) + BarOffset;
	Graphics.DrawImage(BarBounds, Assets.Images["image_hud_health_bar_full"]->Texture, true);

	// Draw health text
	std::stringstream Buffer;
	Buffer << Round(Health) << " / " << Round(MaxHealth);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), BarCenter + glm::vec2(0, 5), GlobalColor, CENTER_BASELINE);
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
		Graphics.DrawImage(BarBounds, Assets.Images["image_hud_mana_bar_empty"]->Texture, true);

		// Draw full bar
		BarBounds.End = SlotPosition + glm::vec2(BarSize.x * ManaPercent, BarSize.y) + BarOffset;
		Graphics.DrawImage(BarBounds, Assets.Images["image_hud_mana_bar_full"]->Texture, true);

		// Draw mana text
		Buffer << Round(Mana) << " / " << Round(MaxMana);
		Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), BarCenter + glm::vec2(0, 5), GlobalColor, CENTER_BASELINE);
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
	Graphics.DrawImage(BarBounds, Assets.Images["image_hud_experience_bar_empty"]->Texture, true);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::vec2(BarSize.x * TurnTimer, BarSize.y) + BarOffset;
	Graphics.DrawImage(BarBounds, Assets.Images["image_hud_experience_bar_full"]->Texture, true);

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
			const _Texture *Texture = nullptr;
			if(ClientPlayer->PotentialAction.Item)
				Texture = ClientPlayer->PotentialAction.Item->Texture;

			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			glm::vec4 Color(COLOR_WHITE);
			if(Time - (int)Time < 0.5f)
				Color.a = 0.5f;

			glm::vec2 DrawPosition = glm::ivec2(BarEndX + 10, SlotPosition.y + BattleElement->Size.y/2);
			if(ClientPlayer->PotentialAction.Item) {
				DrawPosition.x += ItemBackTexture->Size.x/2;
				Graphics.DrawCenteredImage(DrawPosition, ItemBackTexture, Color);
			}
			else
				DrawPosition.x += Texture->Size.x/2;

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

// Generate damage
int _Object::GenerateDamage() {
	return GetRandomInt(MinDamage, MaxDamage);
}

// Generate defense
int _Object::GenerateDefense() {
	return GetRandomInt(MinDefense, MaxDefense);
}

// Create a UI element for battle
void _Object::CreateBattleElement(_Element *Parent) {
	if(BattleElement)
		throw std::runtime_error("_Object::CreateBattleElement: BattleElement already exists!");

	BattleElement = new _Element();
	if(BattleSide == 0)
		BattleElement->Style = Assets.Styles["style_battle_slot_green"];
	else
		BattleElement->Style = Assets.Styles["style_battle_slot_red"];

	BattleElement->Identifier = "battle_element";
	BattleElement->Size = glm::vec2(64, 64);
	BattleElement->Offset = BattleOffset;
	BattleElement->Alignment = CENTER_MIDDLE;
	BattleElement->UserCreated = true;
	BattleElement->Visible = true;
	BattleElement->UserData = (void *)_HUD::WINDOW_BATTLE;
	BattleElement->Parent = Parent;
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
	Data.Write<uint32_t>(PortraitID);
	Data.Write<float>(Health);
	Data.Write<float>(MaxHealth);
	Data.Write<float>(Mana);
	Data.Write<float>(MaxMana);
	Data.Write<int32_t>(Experience);
	Data.Write<int32_t>(Gold);
	Data.Write<double>(PlayTime);
	Data.Write<int32_t>(Deaths);
	Data.Write<int32_t>(MonsterKills);
	Data.Write<int32_t>(PlayerKills);
	Data.Write<int32_t>(Bounty);
	Data.Write<int32_t>(Invisible);

	// Write inventory
	Inventory->Serialize(Data);

	// Write skills
	Data.Write<uint32_t>((uint32_t)Skills.size());
	for(const auto &Skill : Skills) {
		Data.Write<uint32_t>(Skill.first);
		Data.Write<int32_t>(Skill.second);
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
		Data.Write<int32_t>(Unlock.second.Level);
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
	Data.Write<float>(Health);
	Data.Write<float>(MaxHealth);
	Data.Write<float>(Mana);
	Data.Write<float>(MaxMana);
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
	Portrait = Stats->GetPortraitImage(PortraitID);
	Invisible = Data.ReadBit();
	WorldTexture = Assets.Textures["players/basic.png"];
}

// Unserialize object stats
void _Object::UnserializeStats(_Buffer &Data) {
	WorldTexture = Assets.Textures["players/basic.png"];
	Name = Data.ReadString();
	PortraitID = Data.Read<uint32_t>();
	Health = Data.Read<float>();
	BaseMaxHealth = MaxHealth = Data.Read<float>();
	Mana = Data.Read<float>();
	BaseMaxMana = MaxMana = Data.Read<float>();
	Experience = Data.Read<int32_t>();
	Gold = Data.Read<int32_t>();
	PlayTime = Data.Read<double>();
	Deaths = Data.Read<int32_t>();
	MonsterKills = Data.Read<int32_t>();
	PlayerKills = Data.Read<int32_t>();
	Bounty = Data.Read<int32_t>();
	Invisible = Data.Read<int32_t>();

	// Read inventory
	Inventory->Unserialize(Data, Stats);

	// Read skills
	uint32_t SkillCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < SkillCount; i++) {
		uint32_t SkillID = Data.Read<uint32_t>();
		int32_t Points = Data.Read<int32_t>();
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
		int32_t Level = Data.Read<int32_t>();
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
	Health = Data.Read<float>();
	BaseMaxHealth = MaxHealth = Data.Read<float>();
	Mana = Data.Read<float>();
	BaseMaxMana = MaxMana = Data.Read<float>();
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
_StatusEffect * _Object::UpdateStats(_StatChange &StatChange) {
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
		UpdateHealth(StatChange.Values[StatType::HEALTH].Float);

	// Just died
	if(WasAlive && !IsAlive()) {
		PotentialAction.Unset();
		Action.Unset();
		ResetUIState();

		// If not in battle apply penalty immediately
		if(!Battle) {

			// Apply penalty
			ApplyDeathPenalty();
		}
	}

	// Mana change
	if(StatChange.HasStat(StatType::MANA))
		UpdateMana(StatChange.Values[StatType::MANA].Float);

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
			Server->QueueBattle(this, (uint32_t)StatChange.Values[StatType::BATTLE].Integer, true);
		}
	}

	return StatusEffect;
}

// Update health
void _Object::UpdateHealth(float &Value) {
	if(Server && Value > 0)
		Value *= HealPower;

	Health += Value;

	if(Health < 0)
		Health = 0;
	else if(Health > MaxHealth)
		Health = MaxHealth;
}

// Update mana
void _Object::UpdateMana(float Value) {
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
	if(MoveTime < PLAYER_MOVETIME / MoveSpeed)
		return 0;

	MoveTime = 0;
	int InputState = InputStates.front();
	InputStates.pop_front();

	// Get new position
	glm::ivec2 Direction(0, 0);
	GetDirectionFromInput(InputState, Direction);

	// Remove diagonols
	if(Direction.x != 0 && Direction.y != 0)
		Direction.x = 0;

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
	GenerateNextBattle();
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
	if(Gold < 0)
		Gold = 0;
	else if(Gold > PLAYER_MAX_GOLD)
		Gold = PLAYER_MAX_GOLD;
}

// Update experience
void _Object::UpdateExperience(int Value) {

	Experience += Value;
	if(Experience < 0)
		Experience = 0;
}

// Update death count and gold loss
void _Object::ApplyDeathPenalty() {

	// Notify player
	int GoldLoss = (int)(Gold * PLAYER_DEATH_GOLD_PENALTY);
	if(Server && Peer)
		Server->SendMessage(Peer, std::string("You lost " + std::to_string(GoldLoss) + " gold"), COLOR_RED);

	Deaths++;
	UpdateGold(-GoldLoss);
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
			if(Target)
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
void _Object::AcceptTrader(std::vector<size_t> &Slots) {
	if(!Trader)
		return;

	// Trade in required items
	for(uint32_t i = 0; i < Trader->TraderItems.size(); i++) {
		Inventory->DecrementItemCount(Slots[i], -Trader->TraderItems[i].Count);
	}

	// Give player reward
	Inventory->AddItem(Trader->RewardItem, Trader->Count);

	// Update player
	Trader = nullptr;
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

	// Save stat percentages
	float HealthPercent = GetHealthPercent();
	float ManaPercent = GetManaPercent();

	// Get base stats
	CalculateLevelStats();

	MaxHealth = BaseMaxHealth;
	MaxMana = BaseMaxMana;
	HealthRegen = BaseHealthRegen;
	ManaRegen = BaseManaRegen;
	HealPower = BaseHealPower;
	BattleSpeed = 0;
	Evasion = BaseEvasion;
	HitChance = BaseHitChance;
	MinDamage = BaseMinDamage;
	MaxDamage = BaseMaxDamage;
	MinDefense = BaseMinDefense;
	MaxDefense = BaseMaxDefense;
	MoveSpeed = BaseMoveSpeed;
	Resistances.clear();
	Invisible = 0;

	// Get item stats
	int WeaponMinDamage = 0;
	int WeaponMaxDamage = 0;
	int ArmorMinDefense = 0;
	int ArmorMaxDefense = 0;
	float WeaponDamageModifier = 1.0f;
	for(size_t i = 0; i < InventoryType::BAG; i++) {

		// Check each item
		const _Item *Item = Inventory->Slots[i].Item;
		if(Item) {

			// Add damage
			WeaponMinDamage += Item->MinDamage;
			WeaponMaxDamage += Item->MaxDamage;

			// Add defense
			ArmorMinDefense += Item->MinDefense;
			ArmorMaxDefense += Item->MaxDefense;

			// Stat changes
			MaxHealth += Item->MaxHealth;
			MaxMana += Item->MaxMana;
			HealthRegen += Item->HealthRegen;
			ManaRegen += Item->ManaRegen;
			BattleSpeed += Item->BattleSpeed;
			MoveSpeed += Item->MoveSpeed;

			Resistances[Item->ResistanceTypeID] += Item->Resistance;
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
				Skill->Stats(Scripting, ActionResult);
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
	MinDamage += (int)std::roundf(WeaponMinDamage * WeaponDamageModifier);
	MaxDamage += (int)std::roundf(WeaponMaxDamage * WeaponDamageModifier);

	// Get defense
	MinDefense += ArmorMinDefense;
	MaxDefense += ArmorMaxDefense;

	MinDamage = std::max(MinDamage, 0);
	MaxDamage = std::max(MaxDamage, 0);
	MinDefense = std::max(MinDefense, 0);
	MaxDefense = std::max(MaxDefense, 0);

	BattleSpeed = BaseBattleSpeed * BattleSpeed + BaseBattleSpeed;
	if(BattleSpeed < BATTLE_MIN_SPEED)
		BattleSpeed = BATTLE_MIN_SPEED;

	if(MoveSpeed < PLAYER_MIN_MOVESPEED)
		MoveSpeed = PLAYER_MIN_MOVESPEED;

	// Set health/mana
	Health = HealthPercent * MaxHealth;
	Mana = ManaPercent * MaxMana;

	RefreshActionBarCount();
}

// Update an object's stats from a statchange
void _Object::CalculateStatBonuses(_StatChange &StatChange) {
	if(StatChange.HasStat(StatType::MAXHEALTH))
		MaxHealth += StatChange.Values[StatType::MAXHEALTH].Float;
	if(StatChange.HasStat(StatType::MAXMANA))
		MaxMana += StatChange.Values[StatType::MAXMANA].Float;
	if(StatChange.HasStat(StatType::HEALTHREGEN))
		HealthRegen += StatChange.Values[StatType::HEALTHREGEN].Float;
	if(StatChange.HasStat(StatType::MANAREGEN))
		ManaRegen += StatChange.Values[StatType::MANAREGEN].Float;

	if(StatChange.HasStat(StatType::HEALPOWER))
		HealPower += StatChange.Values[StatType::HEALPOWER].Float;

	if(StatChange.HasStat(StatType::BATTLESPEED))
		BattleSpeed += StatChange.Values[StatType::BATTLESPEED].Float;
	if(StatChange.HasStat(StatType::HITCHANCE))
		HitChance += StatChange.Values[StatType::HITCHANCE].Float;
	if(StatChange.HasStat(StatType::EVASION))
		Evasion += StatChange.Values[StatType::EVASION].Float;

	if(StatChange.HasStat(StatType::RESISTTYPE))
		Resistances[(uint32_t)StatChange.Values[StatType::RESISTTYPE].Integer] += StatChange.Values[StatType::RESIST].Float;

	if(StatChange.HasStat(StatType::MINDAMAGE))
		MinDamage += StatChange.Values[StatType::MINDAMAGE].Integer;
	if(StatChange.HasStat(StatType::MAXDAMAGE))
		MaxDamage += StatChange.Values[StatType::MAXDAMAGE].Integer;
	if(StatChange.HasStat(StatType::MINDEFENSE))
		MinDefense += StatChange.Values[StatType::MINDEFENSE].Integer;
	if(StatChange.HasStat(StatType::MAXDEFENSE))
		MaxDefense += StatChange.Values[StatType::MAXDEFENSE].Integer;

	if(StatChange.HasStat(StatType::MOVESPEED))
		MoveSpeed += StatChange.Values[StatType::MOVESPEED].Float;

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
	BaseMinDefense = LevelStat->Defense;
	BaseMaxDefense = LevelStat->Defense;
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
