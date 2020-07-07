

-- Wrapper function for Object.AddTarget that handles taunted state
function AI_AddTarget(Source, Target, AffectedByTaunt)

	-- Check if affected by taunt
	if AffectedByTaunt == false then
		Source.AddTarget(Target.Pointer)
		return
	end

	-- Check for taunt
	for i = 1, #Source.StatusEffects do
		Effect = Source.StatusEffects[i]
		if Effect.Buff == Buff_Taunted and Effect.Source ~= nil then
			Source.AddTarget(Effect.Source)
			return
		end
	end

	-- Continue with normal target
	Source.AddTarget(Target.Pointer)
end

AI_Dumb = {}

function AI_Dumb.Update(self, Object, Enemies, Allies)

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)

	-- Set skill
	Object.SetAction(0)
end

AI_Smart = {}

function AI_Smart.Update(self, Object, Enemies, Allies)

	-- Get lowest health target
	LowestHealth = math.huge
	TargetIndex = 1
	for i = 1, #Enemies do
		if Enemies[i].Health < LowestHealth then
			TargetIndex = i
			LowestHealth = Enemies[i].Health
		end
	end

	-- Set target
	AI_AddTarget(Object, Enemies[TargetIndex], true)

	-- Set skill
	Object.SetAction(0)
end

AI_Boss = {}

function AI_Boss.Update(self, Object, Enemies, Allies)

	-- Chance to do special attack
	if Random.GetInt(1, 10) == 1 then
		for i = 1, #Enemies do
			Object.AddTarget(Enemies[i].Pointer)
		end

		Object.SetAction(1)
		return
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)

	-- Set skill
	Object.SetAction(0)
end

AI_DeadQueen = {}

function AI_DeadQueen.Update(self, Object, Enemies, Allies)

	-- Chance to do special attack
	if Random.GetInt(1, 5) == 1 then

		CanUse = Object.SetAction(1)
		if CanUse == true then
			for i = 1, #Enemies do
				Object.AddTarget(Enemies[i].Pointer)
			end

			return
		end
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)

	-- Set skill
	if Random.GetInt(1, 10) <= 7 then
		Object.SetAction(0)
	else
		Object.SetAction(2)
	end
end

AI_SlimePrince = {}

function AI_SlimePrince.Update(self, Object, Enemies, Allies)

	-- Use potion when mana is low
	CanUse = Object.SetAction(2)
	if CanUse and Object.Mana < Object.MaxMana * 0.5 then

		-- Check for existing mana buff
		ShouldUse = true
		for i = 1, #Object.StatusEffects do
			if Object.StatusEffects[i].Buff == Buff_Mana then
				ShouldUse = false
				break
			end
		end

		if ShouldUse == true then
			Object.AddTarget(Object.Pointer)
			return
		end
	end

	-- Chance to resurrect
	if Random.GetInt(1, 2) == 1 then

		CanUse = Object.SetAction(1)
		if CanUse == true then
			for i = 1, #Allies do
				if Allies[i].Health == 0 then
					Object.AddTarget(Allies[i].Pointer)
					return
				end
			end
		end
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)

	-- Set skill
	Object.SetAction(0)
end

AI_SkeletonPriest = {}

function AI_SkeletonPriest.Update(self, Object, Enemies, Allies)

	-- Heal
	for i = 1, #Allies do
		if Allies[i].Health > 0 and Allies[i].Health <= Allies[i].MaxHealth * 0.75 then

			-- See if target can be healed
			Object.AddTarget(Allies[i].Pointer)
			if Object.SetAction(1) == true then
				return
			end

			-- Clear targets if heal can't be used
			Object.ClearTargets()
		end
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)

	-- Set skill
	Object.SetAction(0)
end

AI_GoblinThief = {}

function AI_GoblinThief.Update(self, Object, Enemies, Allies)

	local Storage = Battles[Object.BattleID]
	if Storage[Object.ID] == nil then
		Storage[Object.ID] = { Steals = 0 }
	end

	-- Flee
	if Storage[Object.ID].Steals >= 2 then
		Object.AddTarget(Object.Pointer)
		Object.SetAction(2)
	else
		Target = Random.GetInt(1, #Enemies)
		AI_AddTarget(Object, Enemies[Target], true)

		Action = Random.GetInt(0, 1)
		if Action == 1 then
			Storage[Object.ID].Steals = Storage[Object.ID].Steals + 1
		end

		Object.SetAction(Action)
	end
end

AI_LavaMan = {}

function AI_LavaMan.Update(self, Object, Enemies, Allies)

	-- Chance to do special attack
	if Random.GetInt(1, 5) == 1 then
		CanUse = Object.SetAction(1)
		if CanUse == false then
			Object.SetAction(0)
		end
	else
		Object.SetAction(0)
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)
end

AI_LavaSpitter = {}

function AI_LavaSpitter.Update(self, Object, Enemies, Allies)

	-- Chance to do special attack
	if Random.GetInt(1, 5) == 1 then
		CanUse = Object.SetAction(1)
		if CanUse == false then
			Object.SetAction(0)
		end
	else
		Object.SetAction(0)
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)
end

AI_SkeletonMage = {}

function AI_SkeletonMage.Update(self, Object, Enemies, Allies)

	-- Look for existing demon
	FoundDemon = false
	AllyCount = 0
	for i = 1, #Allies do
		if Allies[i].MonsterID == 23 and Allies[i].Owner == Object.Pointer then
			FoundDemon = true
		end

		AllyCount = AllyCount + 1
	end

	-- Don't summon if one exists
	SummonDemon = false
	if FoundDemon == false then
		SummonDemon = true
	end

	-- Can't summon due to limit
	if AllyCount == BATTLE_LIMIT then
		SummonDemon = false
	end

	-- Summon demon
	if SummonDemon == true then
		Object.AddTarget(Object.Pointer)
		if Object.SetAction(1) == true then
			return
		end

		Object.ClearTargets()
	end

	-- Cast spell
	Roll = Random.GetInt(1, 10)
	if Roll < 3 then

		-- Ice nova
		CanCast = Object.SetAction(3)
		if CanCast == true then
			for i = 1, #Enemies do
				Object.AddTarget(Enemies[i].Pointer)
			end

			return
		end
	else

		-- Enfeeble
		CanCast = Object.SetAction(2)
		if CanCast == true then
			Target = Random.GetInt(1, #Enemies)
			AI_AddTarget(Object, Enemies[Target], false)

			return
		end

	end

	-- Attack
	AI_AddTarget(Object, Enemies[Target], true)
	Object.SetAction(0)
end

AI_Raj = {}

function AI_Raj.Update(self, Object, Enemies, Allies)

	-- Chance to do special attack
	if Random.GetInt(1, 5) == 1 then
		for i = 1, #Enemies do
			Object.AddTarget(Enemies[i].Pointer)
		end

		Object.SetAction(1)
		return
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)

	-- Set skill
	Object.SetAction(0)
end

AI_Jem = {}

function AI_Jem.Update(self, Object, Enemies, Allies)

	-- Heal
	if Object.Health > 0 and Object.Health <= Object.MaxHealth * 0.75 then

		-- Check if already healing
		Healing = false
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
			if Object.SetAction(1) == true then
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

		if Object.SetAction(2) == true then
			return
		end
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	AI_AddTarget(Object, Enemies[Target], true)

	-- Set skill
	Object.SetAction(0)
end