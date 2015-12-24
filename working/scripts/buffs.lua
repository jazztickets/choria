-- Bleeding debuff --

Buff_Bleeding = { Damage = 1 }

function Buff_Bleeding.GetInfo(Level)

	return "Slowly bleeding for [c red]" .. Buff_Bleeding.Damage * Level .. " [c white]damage"
end

function Buff_Bleeding.Update(Level, Source, Result)

	Result.HealthChange = -Buff_Bleeding.Damage * Level

	return Result
end

-- Healing buff --

Buff_Healing = { Heal = 3 }

function Buff_Healing.GetInfo(Level)

	return "Slowly healing for [c green]" .. Buff_Healing.Heal * Level .. " [c white]HP"
end

function Buff_Healing.Update(Level, Source, Result)

	Result.HealthChange = Buff_Healing.Heal * Level

	return Result
end

-- Mana buff --

Buff_Mana = { Mana = 1 }

function Buff_Mana.GetInfo(Level)

	return "Slowly regaining [c light_blue]" .. Buff_Mana.Mana * Level .. " [c white]MP"
end

function Buff_Mana.Update(Level, Source, Result)

	Result.ManaChange = Buff_Mana.Mana * Level

	return Result
end

-- Invis buff --

Buff_Invis = { }

function Buff_Invis.GetInfo(Level)

	return "Avoiding combat"
end

function Buff_Invis.Update(Level, Source, Result)
	Result.Invisible = 1

	return Result
end

function Buff_Invis.End(Level, Source, Result)
	Result.Invisible = 0

	return Result
end
