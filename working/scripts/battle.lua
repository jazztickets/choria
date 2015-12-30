
-- Calculate basic weapon damage vs target's armor
function Battle_ResolveDamage(Source, Target, Result)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)

	Hit = false
	if Random.GetInt(1, 100) <= (Source.HitChance - Target.Evasion) * 100 then
		Hit = true
	end

	return Damage, Hit
end
