AI_Dumb = {}

function AI_Dumb.Update(Object, Enemies, Allies)
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

function AI_Smart.Update(Object, Enemies, Allies)
	if Object.TurnTimer >= 1.0 then
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
