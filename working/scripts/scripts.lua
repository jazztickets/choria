Script_Rejuv = { BaseHealth = 3, BaseMana = 1 }

function Script_Rejuv.Activate(self, Level, Source, Change)
	Change.Health = self.BaseHealth * Level
	Change.Mana = self.BaseMana * Level

	return Change
end
