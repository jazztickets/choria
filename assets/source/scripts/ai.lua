
-- Check for taunted debuff and attack target only
function AI_HandleTaunt(Source, ActionBarSlot)

	-- Check for taunt and use first action
	for i = 1, #Source.StatusEffects do
		local Effect = Source.StatusEffects[i]
		if Effect.Buff == Buff_Taunted and Effect.Source ~= nil then
			Source.AddTarget(Effect.Source)
			Source.SetAction(ActionBarSlot)
			return true
		end
	end

	return false
end

-- Return the first enemy index that has an attractant debuff
function AI_FindAttractant(Buff, Enemies)

	-- Find enemy with attractant
	local AttractIndex = 0
	for i = 1, #Enemies do
		for j = 1, #Enemies[i].StatusEffects do
			if Enemies[i].StatusEffects[j].Buff == Buff then
				return i
			end
		end
	end

	return 0
end

-- Attack random target

AI_Dumb = {}

function AI_Dumb.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	Object.SetAction(0)
end

-- Attack first enemy found with attractant debuff, otherwise attack random target

AI_Attract = {}

function AI_Attract.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Find enemy with debuff
	local AttractIndex = AI_FindAttractant(Buff_Fractured, Enemies)
	if AttractIndex == 0 then
		AttractIndex = AI_FindAttractant(Buff_Flayed, Enemies)
	end

	-- Target enemy with debuff
	local Target = 1
	if AttractIndex > 0 then
		Target = AttractIndex
	else
		-- Get random target
		Target = Random.GetInt(1, #Enemies)
	end

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	Object.SetAction(0)
end

-- Attack enemy with lowest health

AI_Smart = {}

function AI_Smart.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Get lowest health target
	local LowestHealth = math.huge
	local TargetIndex = 1
	for i = 1, #Enemies do
		if Enemies[i].Health < LowestHealth then
			TargetIndex = i
			LowestHealth = Enemies[i].Health
		end
	end

	-- Set target
	Object.AddTarget(Enemies[TargetIndex].Pointer)

	-- Set skill
	Object.SetAction(0)
end

-- Generic boss AI that randomly uses special attack against all enemies

AI_Boss = {}

function AI_Boss.Update(self, Object, Enemies, Allies)
	local Storage = Battles[Object.BattleID]
	if Storage[Object.ID] == nil then
		Storage[Object.ID] = { Turns = 0 }
	end

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Chance to do special attack
	if Storage[Object.ID].Turns >= 2 and Random.GetInt(1, 5) == 1 then

		local CanUse = Object.SetAction(1)
		if CanUse then
			for i = 1, #Enemies do
				Object.AddTarget(Enemies[i].Pointer)
			end

			return
		end
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	if Object.SetAction(0) then
		Storage[Object.ID].Turns = Storage[Object.ID].Turns + 1
	end
end

-- Dead Queen

AI_DeadQueen = {}

function AI_DeadQueen.Update(self, Object, Enemies, Allies)
	local Storage = Battles[Object.BattleID]
	if Storage[Object.ID] == nil then
		Storage[Object.ID] = { Turns = 0 }
	end

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Chance to do special attack
	if Storage[Object.ID].Turns >= 2 and Random.GetInt(1, 5) == 1 then

		local CanUse = Object.SetAction(1)
		if CanUse then
			for i = 1, #Enemies do
				Object.AddTarget(Enemies[i].Pointer)
			end

			return
		end
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	local ActionSet = nil
	if Random.GetInt(1, 10) <= 7 then
		ActionSet = Object.SetAction(0)
	else
		ActionSet = Object.SetAction(2)
	end

	if ActionSet then
		Storage[Object.ID].Turns = Storage[Object.ID].Turns + 1
	end
end

-- Slime Prince

AI_SlimePrince = {}

function AI_SlimePrince.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Use potion when mana is low
	local CanUse = Object.SetAction(10)
	if CanUse and Object.Mana < Object.MaxMana * 0.5 then

		-- Check for existing mana buff
		local ShouldUse = true
		for i = 1, #Object.StatusEffects do
			if Object.StatusEffects[i].Buff == Buff_Mana then
				ShouldUse = false
				break
			end
		end

		if ShouldUse then
			Object.AddTarget(Object.Pointer)
			return
		end
	end

	-- Chance to resurrect
	if Random.GetInt(1, 2) == 1 then

		for i = 1, #Allies do
			if Allies[i].Health == 0 then
				Object.AddTarget(Allies[i].Pointer)
				if Object.SetAction(1) then
					return
				end

				Object.ClearTargets()
			end
		end
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	Object.SetAction(0)
end

-- Skeleton Priest

AI_SkeletonPriest = {}
AI_SkeletonPriest.HealThreshold = 0.65

function AI_SkeletonPriest.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Get heal target
	local PlayerHealIndex = 0
	local OtherHealIndex = 0
	local LowestPlayerHealth = math.huge
	local LowestOtherHealth = math.huge
	for i = 1, #Allies do
		if Allies[i].Health > 0 and Allies[i].Health <= Allies[i].MaxHealth * self.HealThreshold then
			if Allies[i].MonsterID == 0 then
				if Allies[i].Health < LowestPlayerHealth then
					PlayerHealIndex = i
					LowestPlayerPercent = math.min(Allies[i].Health, LowestPlayerHealth)
				end
			else
				if Allies[i].Health < LowestOtherHealth then
					OtherHealIndex = i
					LowestOtherHealth = math.min(Allies[i].Health, LowestOtherHealth)
				end
			end
		end
	end

	-- Prioritize players
	local HealIndex = OtherHealIndex
	if PlayerHealIndex ~= 0 then
		HealIndex = PlayerHealIndex
	end

	-- Heal target
	if HealIndex ~= 0 then

		-- See if target can be healed
		Object.AddTarget(Allies[HealIndex].Pointer)
		if Object.SetAction(1) then
			return
		end

		-- Clear targets if heal can't be used
		Object.ClearTargets()
	end

	-- Find enemy with debuff
	local AttractIndex = AI_FindAttractant(Buff_Fractured, Enemies)

	-- Target enemy with debuff
	local Target = 1
	if AttractIndex > 0 then
		Target = AttractIndex
	else
		-- Get random target
		Target = Random.GetInt(1, #Enemies)
	end

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	Object.SetAction(0)
end

-- Goblin Thief

AI_GoblinThief = {}

function AI_GoblinThief.Update(self, Object, Enemies, Allies)

	local Storage = Battles[Object.BattleID]
	if Storage[Object.ID] == nil then
		Storage[Object.ID] = { Steals = 0 }
	elseif Storage[Object.ID].Steals == nil then
		Storage[Object.ID].Steals = 0
	end

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Flee
	if Storage[Object.ID].Steals >= 2 then
		Object.AddTarget(Object.Pointer)
		Object.SetAction(2)
	else
		local Target = Random.GetInt(1, #Enemies)
		Object.AddTarget(Enemies[Target].Pointer)

		if Random.GetInt(1, 10) <= 7 and Object.SetAction(1) then
			Storage[Object.ID].Steals = Storage[Object.ID].Steals + 1
			return
		end

		Object.SetAction(0)
	end
end

-- Lava Man

AI_LavaMan = {}

function AI_LavaMan.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Chance to do special attack
	if Random.GetInt(1, 5) == 1 then
		local CanUse = Object.SetAction(10)
		if CanUse == false then
			Object.SetAction(0)
		end
	else
		Object.SetAction(0)
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)
end

-- Snow Man

AI_Snowman = {}

function AI_Snowman.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Chance to do special attack
	if Random.GetInt(1, 3) == 1 then
		local CanUse = Object.SetAction(1)
		if CanUse == false then
			Object.SetAction(0)
		end
	else
		Object.SetAction(0)
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)
end

-- Lava Spitter

AI_LavaSpitter = {}

function AI_LavaSpitter.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Chance to do special attack
	if Random.GetInt(1, 5) == 1 then
		local CanUse = Object.SetAction(10)
		if CanUse == false then
			Object.SetAction(0)
		end
	else
		Object.SetAction(0)
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)
end

-- Skeleton Mage

AI_SkeletonMage = {}

function AI_SkeletonMage.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Look for existing demon
	local FoundDemon = false
	local AllyCount = 0
	for i = 1, #Allies do
		if (Allies[i].MonsterID == 23 or Allies[i].MonsterID == 39) and Allies[i].Owner == Object.Pointer then
			FoundDemon = true
		end

		AllyCount = AllyCount + 1
	end

	-- Don't summon if one exists
	local SummonDemon = false
	if FoundDemon == false then
		SummonDemon = true
	end

	-- Can't summon due to limit
	if AllyCount == BATTLE_LIMIT then
		SummonDemon = false
	end

	-- Summon demon
	if SummonDemon then
		Object.AddTarget(Object.Pointer)
		if Object.SetAction(1) then
			return
		end

		Object.ClearTargets()
	end

	-- Cast spell
	local Roll = Random.GetInt(1, 10)
	if Roll < 4 then

		-- Ice nova
		local CanCast = Object.SetAction(2)
		if CanCast then
			for i = 1, #Enemies do
				Object.AddTarget(Enemies[i].Pointer)
			end

			return
		end
	else

		-- Enfeeble or Flay
		local CanCast = Object.SetAction(Random.GetInt(3, 4))
		if CanCast then
			local Target = Random.GetInt(1, #Enemies)
			Object.AddTarget(Enemies[Target].Pointer)

			return
		end
	end

	-- Attack
	local Target = Random.GetInt(1, #Enemies)
	Object.AddTarget(Enemies[Target].Pointer)
	Object.SetAction(0)
end

-- Raj

AI_Raj = {}

function AI_Raj.Update(self, Object, Enemies, Allies)

	local Storage = Battles[Object.BattleID]
	if Storage[Object.ID] == nil then
		Storage[Object.ID] = { Turns = 0 }
	end

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Chance to do special attack
	if Storage[Object.ID].Turns >= 2 and Random.GetInt(1, 3) == 1 then
		local CanUse = Object.SetAction(1)
		if CanUse then
			for i = 1, #Enemies do
				Object.AddTarget(Enemies[i].Pointer)
			end

			return
		end
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	if Object.SetAction(0) then
		Storage[Object.ID].Turns = Storage[Object.ID].Turns + 1
	end
end

-- Arach

AI_Arach = {}

function AI_Arach.Update(self, Object, Enemies, Allies)

	local Storage = Battles[Object.BattleID]
	if Storage[Object.ID] == nil then
		Storage[Object.ID] = { Turns = 0 }
	end

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Special attack
	if (Storage[Object.ID].Turns % 4) == 0 then
		for i = 1, #Enemies do
			Object.AddTarget(Enemies[i].Pointer)
		end

		if Object.SetAction(1) then
			Storage[Object.ID].Turns = Storage[Object.ID].Turns + 1
			return
		end

		Object.ClearTargets()
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	if Object.SetAction(0) then
		Storage[Object.ID].Turns = Storage[Object.ID].Turns + 1
	end
end

-- Jem

AI_Jem = {}

function AI_Jem.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Heal
	if Object.Health > 0 and Object.Health <= Object.MaxHealth * 0.75 then

		-- Check if already healing
		local Healing = false
		for i = 1, #Object.StatusEffects do
			Effect = Object.StatusEffects[i]
			if Effect.Buff == Buff_Healing then
				Healing = true
				break
			end
		end

		-- See if target can be healed
		if Healing == false then
			Object.AddTarget(Object.Pointer)
			if Object.SetAction(10) then
				return
			end

			-- Clear targets if heal can't be used
			Object.ClearTargets()
		end
	end

	-- Chance to do special attack
	if Random.GetInt(1, 3) == 1 then
		for i = 1, #Enemies do
			Object.AddTarget(Enemies[i].Pointer)
		end

		if Object.SetAction(11) then
			return
		end
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	Object.SetAction(0)
end

-- Zog

AI_Zog = {}

function AI_Zog.Update(self, Object, Enemies, Allies)

	-- Handle taunt debuff
	if AI_HandleTaunt(Object, 0) then
		return
	end

	-- Chance to do special attack
	if Random.GetInt(1, 3) == 1 then
		for i = 1, #Enemies do
			Object.AddTarget(Enemies[i].Pointer)
		end

		if Object.SetAction(Random.GetInt(1, 6)) then
			return
		end

		Object.ClearTargets()
	end

	-- Get random target
	local Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target].Pointer)

	-- Set skill
	Object.SetAction(0)
end
