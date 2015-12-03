AI_Dumb = {}

function AI_Dumb.Update(Object, Enemies, Allies)
	if not BattleActionIsSet then

		-- Chance to do nothing
		if Random.GetInt(1, 10) <= 5 then
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

