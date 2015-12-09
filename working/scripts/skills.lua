Skill_Attack = {}

function Skill_Attack.GetInfo(Level)
	Chance = 4 + Level

	return "Attack with your weapon.\n" .. Chance .. "% chance to deal 200% extra damage."
end

function Skill_Attack.ResolveBattleUse(SkillLevel, Source, Target, Result)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)
	if Random.GetInt(1, 100) <= 4 + SkillLevel then
		Damage = Damage * 3
	end

	Result.DamageDealt = Damage
	Result.TargetHealthChange = -Damage

	return Result
end

Skill_Heal = {}

function Skill_Heal.ResolveBattleUse(SkillLevel, Source, Target, Result)

	return Result
end

function Skill_Heal.GetInfo(Level)

	return "Heal"
end
