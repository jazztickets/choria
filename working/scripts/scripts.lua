Script_Rejuv = { BaseHealth = 5, BaseMana = 5 }

function Script_Rejuv.Activate(self, Level, Cooldown, Object, Change)
	Change.Health = self.BaseHealth * Level
	Change.Mana = self.BaseMana * Level

	return Change
end

Script_Lava = { BaseHealth = -10 }

function Script_Lava.Activate(self, Level, Cooldown, Object, Change)
	Damage = -self.BaseHealth * Level * Object.GetDamageReduction(DamageType["Fire"])

	Update = ResolveManaReductionRatio(Object, Damage)
	Change.Health = Update.Health
	Change.Mana = Update.Mana
	Change.Buff = Buff_Burning.Pointer
	Change.BuffLevel = 20
	Change.BuffDuration = 10

	return Change
end

function Script_Lava.PlaySound(self, Level)
	Audio.Play("flame0.ogg")
end

Script_Boss = {}

function Script_Boss.Activate(self, Level, Cooldown, Object, Change)
	Change.Battle = Level

	return Change
end

Script_Fall = { }

function Script_Fall.Activate(self, Level, Cooldown, Object, Change)
	Change.Health = -1000
	Change.Buff = Buff_Bleeding.Pointer
	Change.BuffLevel = 100
	Change.BuffDuration = 10
	Change.MapChange = Level

	return Change
end

function Script_Fall.PlaySound(self, Level)
	Audio.Play("crunch1.ogg")
end

Script_Slow = { }

function Script_Slow.Activate(self, Level, Cooldown, Object, Change)
	Change.Buff = Buff_Slowed.Pointer
	Change.BuffLevel = Level
	Change.BuffDuration = 5

	return Change
end

function Script_Slow.PlaySound(self, Level)
	Audio.Play("swamp0.ogg")
end