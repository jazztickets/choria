AI_Dumb = {}

function AI_Dumb.Update(self, Object, Enemies, Allies)

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target])

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
	Object.AddTarget(Enemies[TargetIndex])

	-- Set skill
	Object.SetAction(0)
end

AI_Boss = {}

function AI_Boss.Update(self, Object, Enemies, Allies)

	-- Chance to do special attack
	if Random.GetInt(1, 10) == 1 then
		for i = 1, #Enemies do
			Object.AddTarget(Enemies[i])
		end

		Object.SetAction(1)
		return
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target])

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
				Object.AddTarget(Enemies[i])
			end

			return
		end
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target])

	-- Set skill
	if Random.GetInt(1, 10) <= 7 then
		Object.SetAction(0)
	else
		Object.SetAction(2)
	end
end

AI_SlimePrince = {}

function AI_SlimePrince.Update(self, Object, Enemies, Allies)

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
			Object.AddTarget(Object)
			return
		end
	end

	-- Chance to resurrect
	if Random.GetInt(1, 2) == 1 then

		CanUse = Object.SetAction(1)
		if CanUse == true then
			for i = 1, #Allies do
				if Allies[i].Health == 0 then
					Object.AddTarget(Allies[i])
					return
				end
			end
		end
	end

	-- Get random target
	Target = Random.GetInt(1, #Enemies)

	-- Set target
	Object.AddTarget(Enemies[Target])

	-- Set skill
	Object.SetAction(0)
end

AI_SkeletonPriest = {}

function AI_SkeletonPriest.Update(self, Object, Enemies, Allies)

	-- Heal
	for i = 1, #Allies do
		if Allies[i].Health > 0 and Allies[i].Health <= Allies[i].MaxHealth * 0.75 then

			-- See if target can be healed
			Object.AddTarget(Allies[i])
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
	Object.AddTarget(Enemies[Target])

	-- Set skill
	Object.SetAction(0)
end

AI_GoblinThief = {}

function AI_GoblinThief.Update(self, Object, Enemies, Allies)

	-- Flee
	if Random.GetInt(1, 5) == 1 then
		Object.AddTarget(Object)
		Object.SetAction(2)
	else
		Target = Random.GetInt(1, #Enemies)
		Object.AddTarget(Enemies[Target])
		Object.SetAction(1)
	end
end
