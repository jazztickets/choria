-- Bleeding debuff --

Buff_Bleeding = {}

function Buff_Bleeding.GetInfo(Level)

	return "Bleeding"
end

function Buff_Bleeding.Update(Level, Source, Result)
	print("Bleeding!")

	Damage = 1
	Result.DamageDealt = Damage
	Result.TargetHealthChange = -Damage

	return Result
end
