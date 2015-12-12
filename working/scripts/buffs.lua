-- Bleeding debuff --

Buff_Bleeding = {}

function Buff_Bleeding.GetInfo(Level)

	return "Bleeding"
end

function Buff_Bleeding.Update(Level, Source, Result)
	print("Bleeding!")

	Result.HealthChange = -1

	return Result
end
