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
			Object.SetBattleTarget(Enemies[Target])

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
			Object.SetBattleTarget(Enemies[Target])

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
					Object.SetBattleTarget(Enemies[i])
				end

				Object.SetAction(1)
				return
			end

			-- Get random target
			Target = Random.GetInt(1, #Enemies)

			-- Set target
			Object.SetBattleTarget(Enemies[Target])

			-- Set skill
			Object.SetAction(0)
		end
	end
end
