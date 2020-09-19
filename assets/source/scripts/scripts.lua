Script_Rejuv = { BaseHealth = 5, BaseMana = 5 }

function Script_Rejuv.Activate(self, Level, Object, Change)
	Change.Health = self.BaseHealth * Level
	Change.Mana = self.BaseMana * Level
	Change.Buff = Buff_Sanctuary.Pointer
	Change.BuffLevel = 10
	Change.BuffDuration = 10

	return Change
end

Script_Lava = { BaseDamage = 10 }

function Script_Lava.Activate(self, Level, Object, Change)
	if Object.LavaProtection == true then
		Change.ClearBuff = Buff_Invis.Pointer
		return Change
	end

	Damage = self.BaseDamage * Level * Object.GetDamageReduction(DamageType["Fire"])
	Change.Health = -Damage
	Change.Buff = Buff_Burning.Pointer
	Change.BuffLevel = Level * 2
	Change.BuffDuration = 10
	if Object.HasBuff(Buff_Healing.Pointer) then
		Change.ClearBuff = Buff_Healing.Pointer
	else
		Change.ClearBuff = Buff_Invis.Pointer
	end

	return Change
end

function Script_Lava.PlaySound(self, Object)
	if Object.LavaProtection == true then
		return
	end

	Audio.Play("flame0.ogg")
end

Script_Boss = {}

function Script_Boss.Activate(self, Level, Object, Change)
	Change.Battle = Level

	return Change
end

Script_Fall = { }

function Script_Fall.Activate(self, Level, Object, Change)
	Change.Health = -5000
	Change.Buff = Buff_Bleeding.Pointer
	Change.BuffLevel = 1000
	Change.BuffDuration = 10
	Change.MapChange = Level

	return Change
end

function Script_Fall.PlaySound(self, Object)
	Audio.Play("crunch1.ogg")
end

Script_Slow = { }

function Script_Slow.Activate(self, Level, Object, Change)
	Change.Buff = Buff_Slowed.Pointer
	Change.BuffLevel = Level
	Change.BuffDuration = 5
	if Object.HasBuff(Buff_Burning.Pointer) then
		Change.ClearBuff = Buff_Burning.Pointer
	else
		Change.ClearBuff = Buff_Invis.Pointer
	end

	return Change
end

function Script_Slow.PlaySound(self, Object)
	Audio.Play("swamp0.ogg", 0.5)
end

Script_Freezing = { BaseDamage = 5 }

function Script_Freezing.Activate(self, Level, Object, Change)
	if Object.FreezeProtection == true then
		Change.ClearBuff = Buff_Invis.Pointer
		return Change
	end

	Damage = self.BaseDamage * Level * Object.GetDamageReduction(DamageType["Cold"])
	Change.Health = -Damage
	Change.Buff = Buff_Freezing.Pointer
	Change.BuffLevel = Level
	Change.BuffDuration = 10
	if Object.HasBuff(Buff_Healing.Pointer) then
		Change.ClearBuff = Buff_Healing.Pointer
	else
		Change.ClearBuff = Buff_Invis.Pointer
	end

	return Change
end

function Script_Freezing.PlaySound(self, Object)
	if Object.FreezeProtection == true then
		return
	end

	Audio.Play("ice" .. Random.GetInt(0, 1) .. ".ogg")
end