-- Calculate basic weapon damage vs target's armor
function Battle_ResolveDamage(Source, Target, Result)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)

	print("here")
	return Damage
end

