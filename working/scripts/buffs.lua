-- Bleeding debuff --

Buff_Bleeding = {}

function Buff_Bleeding.GetInfo(Level)

	return "Bleeding"
end

function Buff_Bleeding.Update(Level, Source, Target, Result)

	Damage = 1
	Result.DamageDealt = Damage
	Result.TargetHealthChange = -Damage

	return Result
end
