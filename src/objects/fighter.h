/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef FIGHTER_H
#define FIGHTER_H

// Libraries
#include <irrlicht/irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Constants
const int FIGHTER_MAXSKILLS = 8;
const int FIGHTER_MAXACTIONS = 8;

// Forward Declarations
class BattleClass;
class ActionClass;
struct BattleUpdateStruct;

// Classes
class FighterClass {

	public:

		enum FighterType {
			TYPE_PLAYER,
			TYPE_MONSTER,
		};

		FighterClass(int Type);
		~FighterClass();
		
		// Object
		int GetType() const { return Type; }

		// Render
		void RenderBattle(bool Target);
		void SetOffset(const position2di &Position) { Offset = Position; }

		// Stats
		void SetName(const stringc &Name) { this->Name = Name; }
		void SetHealth(int Value) { Health = Value; }
		void SetMaxHealth(int Value) { MaxHealth = Value; }
		void SetMana(int Value) { Mana = Value; }
		void SetMaxMana(int Value) { MaxMana = Value; }

		const stringc &GetName() const { return Name; }
		int GetHealth() const { return Health; }
		int GetMaxHealth() const { return MaxHealth; }
		int GetMana() const { return Mana; }
		int GetMaxMana() const { return MaxMana; }

		float GetHealthRegen() const { return HealthRegen; }
		float GetManaRegen() const { return ManaRegen; }
		float GetHealthAccumulator() const { return HealthAccumulator; }
		float GetManaAccumulator() const { return ManaAccumulator; }

		void UpdateHealth(int Value);
		void UpdateMana(int Value);
		void RestoreHealthMana();

		void UpdateRegen(int &HealthUpdate, int &ManaUpdate);
		void SetRegenAccumulators(float HealthAccumulator, float ManaAccumulator);

		int GetLevel() const { return Level; }

		// Battles
		void SetBattle(BattleClass *Battle);
		BattleClass *GetBattle();
		void SetBattleAction(const ActionClass *Action) { BattleAction = Action; }
		const ActionClass *GetBattleAction() const { return BattleAction; }
		void SetTarget(int Target) { this->Target = Target; }
		int GetTarget() const { return Target; }
		void SetSlot(int Slot) { this->Slot = Slot; }
		int GetSlot() const { return Slot; }
		int GetSide() const { return Slot & 1; }
		void GetOffset(int &X, int &Y) { X = Offset.X, Y = Offset.Y; }
		int GenerateDamage() const;
		int GenerateDefense() const;
		int GetMinDamage() const { return MinDamage; }
		int GetMaxDamage() const { return MaxDamage; }
		int GetMinDefense() const { return MinDefense; }
		int GetMaxDefense() const { return MaxDefense; }

		void SetTurnTimer(int Value) { TurnTimer = Value; }
		int GetTurnTimer() const { return TurnTimer; }
		void SetTurnTimerMax(int Value) { TurnTimerMax = Value; }
		int GetTurnTimerMax() const { return TurnTimerMax; }
		void UpdateTurnTimer(u32 UpdateTime);
		void ResetTurnTimer() { TurnTimer = 0; }
		bool TurnTimerReady() const { return TurnTimer == TurnTimerMax; }

		virtual int GetExperienceGiven() const { return 0; }
		virtual int GetGoldGiven() const { return 0; }
		virtual void UpdateExperience(int Value) { }

		// Actions
		void SetActionBar(int Slot, const ActionClass *Action) { ActionBar[Slot] = Action; }
		const ActionClass *GetActionBar(int Index);

		// AI
		virtual bool UpdateAI(u32 FrameTime) { return false; }

	protected:

		// Objects
		int Type;

		// Action bar
		const ActionClass *ActionBar[FIGHTER_MAXACTIONS];

		// Stats
		stringc Name;
		int Level;
		int Health, MaxHealth;
		int Mana, MaxMana;
		int MinDamage, MaxDamage;
		int MinDefense, MaxDefense;
		u32 TurnTimer, TurnTimerMax;
		float HealthRegen, ManaRegen, HealthAccumulator, ManaAccumulator;

		// Battle
		BattleClass *Battle;
		int Target, Slot;
		const ActionClass *BattleAction;

		// Render
		ITexture *Portrait;
		position2di Offset;
};

#endif
