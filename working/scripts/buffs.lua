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

function Buff_Invis.Begin(self, Level, Source, Change)
	Change.Invisible = 1

	return Change
end

function Buff_Invis.End(self, Level, Source, Change)
	Change.Invisible = 0

	return Change
end
