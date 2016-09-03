-- Bleeding --

Buff_Bleeding = {}
Buff_Bleeding.Damage = 1
Buff_Bleeding.DamageType = DamageType["Bleed"]

function Buff_Bleeding.GetInfo(self, Level)

	return "Slowly bleeding for [c red]" .. self.Damage * Level .. " [c white]damage"
end

function Buff_Bleeding.Update(self, Level, Source, Change)
	Damage = self.Damage
	Damage = Damage * (1 - Source.GetResistance(self.DamageType))
	Damage = math.max(Damage, 0)

	Change.Health = -Damage * Level

	return Change
end

-- Healing --

Buff_Healing = { Heal = 3 }

function Buff_Healing.GetInfo(self, Level)

	return "Slowly healing for [c green]" .. self.Heal * Level .. " [c white]HP"
end

function Buff_Healing.Update(self, Level, Source, Change)
	Change.Health = self.Heal * Level

	return Change
end

-- Mana --

Buff_Mana = { Mana = 1 }

function Buff_Mana.GetInfo(self, Level)

	return "Slowly regaining [c light_blue]" .. self.Mana * Level .. " [c white]MP"
end

function Buff_Mana.Update(self, Level, Source, Change)
	Change.Mana = self.Mana * Level

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

	return "Battle speed reduced by [c green]" .. math.floor(self:GetChange(Level) * 100) .. "%"
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
	Change.BattleSpeed = -10000

	return Change
end

-- Hasted --

Buff_Hasted = Base_Buff:New()

function Buff_Hasted.GetInfo(self, Level)

	return "Battle speed increased by [c green]" .. math.floor(self:GetChange(Level) * 100) .. "%"
end

function Buff_Hasted.Stats(self, Level, Source, Change)
	Change.BattleSpeed = self:GetChange(Level)

	return Change
end

-- Fast --

Buff_Fast = Base_Buff:New()

function Buff_Fast.GetInfo(self, Level)

	return "Move speed increased by [c green]" .. math.floor(self:GetChange(Level) * 100) .. "%"
end

function Buff_Fast.Stats(self, Level, Source, Change)
	Change.MoveSpeed = self:GetChange(Level)

	return Change
end

-- Blinded --

Buff_Blinded = Base_Buff:New()

function Buff_Blinded.GetInfo(self, Level)

	return "Hit chance reduced by [c green]" .. math.floor(self:GetChange(Level) * 100) .. "%"
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

	return "Defense increased by [c green]" .. Level
end

function Buff_Hardened.Stats(self, Level, Source, Change)
	Change.MinDefense = Level
	Change.MaxDefense = Level

	return Change
end

-- Parry --

Buff_Parry = Base_Buff:New()
Buff_Parry.DamageReduction = 0.5
Buff_Parry.StaminaGain = 0.2

function Buff_Parry.GetInfo(self, Level)

	return "Blocking attacks"
end

function Buff_Parry.OnHit(self, Level, Change)
	Change.Damage = math.floor(Change.Damage * self.DamageReduction)
	Change.Stamina = self.StaminaGain

	return Change
end

-- Poisoned --

Buff_Poisoned = {}
Buff_Poisoned.HealPower = -0.5
Buff_Poisoned.Damage = 1

function Buff_Poisoned.GetInfo(self, Level)

	return "Healing reduced and taking [c red]" .. self.Damage * Level .. " [c white]damage"
end

function Buff_Poisoned.Update(self, Level, Source, Change)
	Change.Health = -self.Damage * Level

	return Change
end

function Buff_Poisoned.Stats(self, Level, Source, Change)
	Change.HealPower = self.HealPower

	return Change
end

-- Burning --

Buff_Burning = {}
Buff_Burning.Damage = 3

function Buff_Burning.GetInfo(self, Level)

	return "Burning for [c red]" .. self.Damage * Level .. " [c white]damage"
end

function Buff_Burning.Update(self, Level, Source, Change)
	Change.Health = -self.Damage * Level

	return Change
end

-- Bleed Resist --

Buff_BleedResist = Base_Buff:New()
Buff_BleedResist.Increments = 5

function Buff_BleedResist.GetInfo(self, Level)

	return "Bleed resist increased by [c green]" .. self.Increments * Level .. "%"
end

function Buff_BleedResist.Stats(self, Level, Source, Change)
	Change.ResistType = DamageType["Bleed"]
	Change.Resist = self.Increments * Level / 100

	return Change
end
