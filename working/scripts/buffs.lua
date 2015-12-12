-- Bleeding debuff --

Buff_Bleeding = { Damage = 1 }

function Buff_Bleeding.GetInfo(Level)

	return "Slowly bleeding for " .. Buff_Bleeding.Damage * Level .. " damage"
end

function Buff_Bleeding.Update(Level, Source, Result)

	Result.HealthChange = -Buff_Bleeding.Damage * Level

	return Result
end
