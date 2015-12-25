-- Bleeding debuff --

Buff_Bleeding = { Damage = 1 }

function Buff_Bleeding.GetInfo(Level)

	return "Slowly bleeding for [c red]" .. Buff_Bleeding.Damage * Level .. " [c white]damage"
end

function Buff_Bleeding.Update(Level, Source, Change)
	Change.Health = -Buff_Bleeding.Damage * Level

	return Change
end

-- Healing buff --

Buff_Healing = { Heal = 3 }

function Buff_Healing.GetInfo(Level)

	return "Slowly healing for [c green]" .. Buff_Healing.Heal * Level .. " [c white]HP"
end

function Buff_Healing.Update(Level, Source, Change)
	Change.Health = Buff_Healing.Heal * Level

	return Change
end

-- Mana buff --

Buff_Mana = { Mana = 1 }

function Buff_Mana.GetInfo(Level)

	return "Slowly regaining [c light_blue]" .. Buff_Mana.Mana * Level .. " [c white]MP"
end

function Buff_Mana.Update(Level, Source, Change)
	Change.Mana = Buff_Mana.Mana * Level

	return Change
end

-- Invis buff --

Buff_Invis = { }

function Buff_Invis.GetInfo(Level)

	return "Avoiding combat"
end

function Buff_Invis.Begin(Level, Source, Change)
	Change.Invisible = 1

	return Change
end

function Buff_Invis.End(Level, Source, Change)
	Change.Invisible = 0

	return Change
end
