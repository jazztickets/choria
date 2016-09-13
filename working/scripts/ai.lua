AI_Dumb = {}

function AI_Dumb.Update(self, Object, Enemies, Allies)
	if Object.TurnTimer >= 1.0 then
		if not Object.BattleActionIsSet then

			-- Chance to do nothing
			if Random.GetInt(1, 2) == 1 then
				return
			end

			-- Get random target
			Target = Random.GetInt(1, #Enemies)

			-- Set target
			Object.AddTarget(Enemies[Target])

			-- Set skill
			Object.SetAction(0)
		end
	end
end

AI_Smart = {}

function AI_Smart.Update(self, Object, Enemies, Allies)
	if Object.TurnTimer >= 0.7 then
		if not Object.BattleActionIsSet then

			-- Get random target
			Target = Random.GetInt(1, #Enemies)

			-- Set target
			Object.AddTarget(Enemies[Target])

			-- Set skill
			Object.SetAction(0)
		end
	end
end

AI_Boss = {}

function AI_Boss.Update(self, Object, Enemies, Allies)
	if Object.TurnTimer >= 0.7 then
		if not Object.BattleActionIsSet then

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
	end
end

AI_DeadQueen = {}

function AI_DeadQueen.Update(self, Object, Enemies, Allies)
	if Object.TurnTimer >= 0.7 then
		if not Object.BattleActionIsSet then

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
	end
end

AI_SlimePrince = {}

function AI_SlimePrince.Update(self, Object, Enemies, Allies)
	if Object.TurnTimer >= 0.7 then
		if not Object.BattleActionIsSet then

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
	end
end

AI_SkeletonPriest = {}

function AI_SkeletonPriest.Update(self, Object, Enemies, Allies)
	if Object.TurnTimer >= 0.9 then
		if not Object.BattleActionIsSet then

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
	end
end
