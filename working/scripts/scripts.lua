Script_Rejuv = { BaseHealth = 5, BaseMana = 5 }

function Script_Rejuv.Activate(self, Level, Cooldown, Object, Change)
	Change.Health = self.BaseHealth * Level
	Change.Mana = self.BaseMana * Level

	return Change
end

Script_Lava = { BaseHealth = -10 }

function Script_Lava.Activate(self, Level, Cooldown, Object, Change)
	Change.Health = self.BaseHealth * Level

	return Change
end

Script_Boss = {}

function Script_Boss.Activate(self, Level, Cooldown, Object, Change)
	Change.Battle = Level

	return Change
end
