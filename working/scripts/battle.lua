
-- Calculate basic weapon damage vs target's armor
function Battle_ResolveDamage(Source, Target, Result)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)

	if Random.GetInt(1, 100) <= (Source.HitChance - Target.Evasion) * 100 then
		Result.Target.Health = -Damage
		Hit = true
	else
		Result.Target.Miss = true
		Hit = false
	end

	return Hit
end
