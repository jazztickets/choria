-- Bleeding --

Buff_Bleeding = {}
Buff_Bleeding.Damage = 1
Buff_Bleeding.DamageType = DamageType["Bleed"]

function Buff_Bleeding.GetInfo(self, Level)

	return "Slowly bleeding for [c red]" .. self.Damage * Level .. " [c white]damage"
end

function Buff_Bleeding.Update(self, Level, Source, Change)
	Damage = self.Damage * Level
	Damage = Damage * Source.GetDamageReduction(self.DamageType)
	Damage = math.max(Damage, 0)

	Update = ResolveManaReductionRatio(Source, Damage)
	Change.Health = Update.Health
	Change.Mana = Update.Mana

	return Change
end

-- Healing --

Buff_Healing = { Heal = 1 }

function Buff_Healing.GetInfo(self, Level)

	return "Slowly healing for [c green]" .. self.Heal * Level .. " [c white]HP"
end

function Buff_Healing.Update(self, Level, Source, Change)
	Change.Health = math.floor(self.Heal * Level)

	return Change
end

-- Mana --

Buff_Mana = { Mana = 1 }

function Buff_Mana.GetInfo(self, Level)

	return "Slowly regaining [c light_blue]" .. self.Mana * Level .. " [c white]MP"
end

function Buff_Mana.Update(self, Level, Source, Change)
	Change.Mana = math.floor(self.Mana * Level)

	return Change
end

-- Invis --

Buff_Invis = {}

function Buff_Invis.GetInfo(self, Level)

	return "Avoiding combat"
end

function Buff_Invis.Stats(self, Level, Source, Change)
	Change.Invisible = 1

	return Change
end

-- Slowed --

Buff_Slowed = Base_Buff:New()

function Buff_Slowed.GetInfo(self, Level)

	return "Battle speed reduced by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Slowed.Stats(self, Level, Source, Change)
	Change.BattleSpeed = -self:GetChange(Level)

	return Change
end

-- Stunned --

Buff_Stunned = {}

function Buff_Stunned.GetInfo(self, Level)

	return "Unable to act"
end

function Buff_Stunned.Stats(self, Level, Source, Change)
	Change.Stunned = 1

	return Change
end

-- Taunted --

Buff_Taunted = {}

function Buff_Taunted.GetInfo(self, Level)
	return "Can only target taunter"
end

-- Hasted --

Buff_Hasted = Base_Buff:New()

function Buff_Hasted.GetInfo(self, Level)

	return "Battle speed increased by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Hasted.Stats(self, Level, Source, Change)
	Change.BattleSpeed = self:GetChange(Level)

	return Change
end

-- Fast --

Buff_Fast = Base_Buff:New()

function Buff_Fast.GetInfo(self, Level)

	return "Move speed increased by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Fast.Stats(self, Level, Source, Change)
	Change.MoveSpeed = self:GetChange(Level)

	return Change
end

-- Blinded --

Buff_Blinded = Base_Buff:New()

function Buff_Blinded.GetInfo(self, Level)

	return "Hit chance reduced by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Blinded.Stats(self, Level, Source, Change)
	Change.HitChance = -self:GetChange(Level)

	return Change
end

-- Mighty --

Buff_Mighty = Base_Buff:New()

function Buff_Mighty.GetInfo(self, Level)

	return "Damage increased by [c green]" .. Level
end

function Buff_Mighty.Stats(self, Level, Source, Change)
	Change.MinDamage = Level
	Change.MaxDamage = Level

	return Change
end

-- Hardened --

Buff_Hardened = Base_Buff:New()

function Buff_Hardened.GetInfo(self, Level)

	return "Armor increased by [c green]" .. Level
end

function Buff_Hardened.Stats(self, Level, Source, Change)
	Change.Armor = Level

	return Change
end

-- Evasion --

Buff_Evasion = Base_Buff:New()

function Buff_Evasion.GetInfo(self, Level)

	return "Evasion increased by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Evasion.Stats(self, Level, Source, Change)
	Change.Evasion = self:GetChange(Level)

	return Change
end

-- Parry --

Buff_Parry = Base_Buff:New()
Buff_Parry.DamageReduction = 0.5
Buff_Parry.StaminaGain = 0.2

function Buff_Parry.GetInfo(self, Level)

	return "Blocking attacks"
end

function Buff_Parry.OnHit(self, Object, Level, Duration, Change)
	Change.BuffSound = self.ID
	Change.Damage = math.floor(Change.Damage * self.DamageReduction)
	Change.Stamina = self.StaminaGain
end

function Buff_Parry.PlaySound(self, Level)
	Audio.Play("parry0.ogg")
end

-- Poisoned --

Buff_Poisoned = {}
Buff_Poisoned.HealPower = -0.5
Buff_Poisoned.Damage = 1
Buff_Poisoned.DamageType = DamageType["Poison"]

function Buff_Poisoned.GetInfo(self, Level)

	return "Healing reduced and taking [c red]" .. self.Damage * Level .. " [c white]damage"
end

function Buff_Poisoned.Update(self, Level, Source, Change)
	Damage = self.Damage * Level
	Damage = Damage * Source.GetDamageReduction(self.DamageType)
	Damage = math.max(math.floor(Damage), 0)

	Update = ResolveManaReductionRatio(Source, Damage)
	Change.Health = Update.Health
	Change.Mana = Update.Mana

	return Change
end

function Buff_Poisoned.Stats(self, Level, Source, Change)
	Change.HealPower = self.HealPower

	return Change
end

-- Burning --

Buff_Burning = {}
Buff_Burning.Damage = 3
Buff_Burning.DamageType = DamageType["Fire"]

function Buff_Burning.GetInfo(self, Level)

	return "Burning for [c red]" .. self.Damage * Level .. " [c white]damage"
end

function Buff_Burning.Update(self, Level, Source, Change)
	Damage = self.Damage * Level
	Damage = Damage * Source.GetDamageReduction(self.DamageType)
	Damage = math.max(math.floor(Damage), 0)

	Update = ResolveManaReductionRatio(Source, Damage)
	Change.Health = Update.Health
	Change.Mana = Update.Mana

	return Change
end

-- Bleed Resist --

Buff_BleedResist = Base_Buff:New()

function Buff_BleedResist.GetInfo(self, Level)

	return "Bleed resist increased by [c green]" .. Level .. "%"
end

function Buff_BleedResist.Stats(self, Level, Source, Change)
	Change.ResistType = DamageType["Bleed"]
	Change.Resist = Level

	return Change
end

-- Weak  --

Buff_Weak = Base_Buff:New()

function Buff_Weak.GetInfo(self, Level)

	return "Attack damage reduced by [c green]" .. Level .. "%"
end

function Buff_Weak.Stats(self, Level, Source, Change)
	Change.AttackPower = -Level / 100.0

	return Change
end

-- Light  --

Buff_Light = Base_Buff:New()

function Buff_Light.GetInfo(self, Level)

	return "Giving off light"
end

function Buff_Light.Stats(self, Level, Source, Change)
	Change.Light = Level

	return Change
end

-- Shielded --

Buff_Shielded = Base_Buff:New()

function Buff_Shielded.GetInfo(self, Level)
	return "Blocking [c green]" .. Level .. "[c white] damage before breaking"
end

function Buff_Shielded.OnHit(self, Object, Level, Duration, Change)

	-- Reduce shield
	Level = Level - Change.Damage

	-- Check for shield breaking
	if Level <= 0 then
		Change.Damage = math.abs(Level)
		Object.UpdateBuff(self.ID, 0, 0)
	else
		Change.Damage = 0
		Object.UpdateBuff(self.ID, Level, Duration)
	end

	Change.BuffSound = self.ID
end

function Buff_Shielded.PlaySound(self, Level)
	Audio.Play("absorb" .. Random.GetInt(0, 1) .. ".ogg", 0.5)
end