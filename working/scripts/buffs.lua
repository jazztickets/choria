-- Bleeding debuff --

Buff_Bleeding = { Damage = 1 }

function Buff_Bleeding.GetInfo(self, Level)

	return "Slowly bleeding for [c red]" .. self.Damage * Level .. " [c white]damage"
end

function Buff_Bleeding.Update(self, Level, Source, Change)
	Change.Health = -self.Damage * Level

	return Change
end

-- Healing buff --

Buff_Healing = { Heal = 3 }

function Buff_Healing.GetInfo(self, Level)

	return "Slowly healing for [c green]" .. self.Heal * Level .. " [c white]HP"
end

function Buff_Healing.Update(self, Level, Source, Change)
	Change.Health = self.Heal * Level

	return Change
end

-- Mana buff --

Buff_Mana = { Mana = 1 }

function Buff_Mana.GetInfo(self, Level)

	return "Slowly regaining [c light_blue]" .. self.Mana * Level .. " [c white]MP"
end

function Buff_Mana.Update(self, Level, Source, Change)
	Change.Mana = self.Mana * Level

	return Change
end

-- Invis buff --

Buff_Invis = { }

function Buff_Invis.GetInfo(self, Level)

	return "Avoiding combat"
end

function Buff_Invis.Stats(self, Level, Source, Change)
	Change.Invisible = 1

	return Change
end

-- Slowed debuff --

Buff_Slowed = Base_Buff:New()

function Buff_Slowed.GetInfo(self, Level)

	return "Battle speed reduced by [c green]" .. math.floor(self:GetChange(Level) * 100) .. "%"
end

function Buff_Slowed.Stats(self, Level, Source, Change)
	Change.BattleSpeed = -self:GetChange(Level)

	return Change
end

-- Stunned debuff --

Buff_Stunned = { }

function Buff_Stunned.GetInfo(self, Level)

	return "Unable to act"
end

function Buff_Stunned.Stats(self, Level, Source, Change)
	Change.BattleSpeed = -10000

	return Change
end

-- Hasted buff --

Buff_Hasted = Base_Buff:New()

function Buff_Hasted.GetInfo(self, Level)

	return "Battle speed increased by [c green]" .. math.floor(self:GetChange(Level) * 100) .. "%"
end

function Buff_Hasted.Stats(self, Level, Source, Change)
	Change.BattleSpeed = self:GetChange(Level)

	return Change
end

-- Blinded debuff --

Buff_Blinded = Base_Buff:New()

function Buff_Blinded.GetInfo(self, Level)

	return "Hit chance reduced by [c green]" .. math.floor(self:GetChange(Level) * 100) .. "%"
end

function Buff_Blinded.Stats(self, Level, Source, Change)
	Change.HitChance = -self:GetChange(Level)

	return Change
end

-- Hardened debuff --

Buff_Hardened = Base_Buff:New()

function Buff_Hardened.GetInfo(self, Level)

	return "Defense increased by [c green]" .. Level
end

function Buff_Hardened.Stats(self, Level, Source, Change)
	Change.MinDefense = Level
	Change.MaxDefense = Level

	return Change
end
