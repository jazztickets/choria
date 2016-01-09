Script_Rejuv = { BaseHealth = 3, BaseMana = 1 }

function Script_Rejuv.Activate(self, Level, Object, Change)
	Change.Health = self.BaseHealth * Level
	Change.Mana = self.BaseMana * Level

	return Change
end

Script_Lava = { BaseHealth = -10 }

function Script_Lava.Activate(self, Level, Object, Change)
	Change.Health = self.BaseHealth * Level

	return Change
end

Script_Boss = { SpawnTime = {}, Cooldown = 300 }

function Script_Boss.Activate(self, Level, Object, Change)
	LastFightTime = self.SpawnTime[Object.CharacterID]
	SpawnFight = false

	-- Check last fight time
	if LastFightTime == nil then
		SpawnFight = true
	elseif ServerTime - LastFightTime > self.Cooldown then
		SpawnFight = true
	end

	if SpawnFight == true then
		Change.Battle = Level
		self.SpawnTime[Object.CharacterID] = ServerTime
	end

	return Change
end
