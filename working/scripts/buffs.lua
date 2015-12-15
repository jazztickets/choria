-- Bleeding debuff --

Buff_Bleeding = { Damage = 1 }

function Buff_Bleeding.GetInfo(Level)

	return "Slowly bleeding for " .. Buff_Bleeding.Damage * Level .. " damage"
end

function Buff_Bleeding.Update(Level, Source, Result)

	Result.HealthChange = -Buff_Bleeding.Damage * Level

	return Result
end

-- Healing buff --

Buff_Healing = { Heal = 3 }

function Buff_Healing.GetInfo(Level)

	return "Slowly healing for " .. Buff_Healing.Heal * Level .. " HP"
end

function Buff_Healing.Update(Level, Source, Result)

	Result.HealthChange = Buff_Healing.Heal * Level

	return Result
end
