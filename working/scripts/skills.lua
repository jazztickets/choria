Skill_Attack = {}

function Skill_Attack.GetInfo(Level)
	Chance = 4 + Level

	return "Attack with your weapon.\n" .. Chance .. "% chance to deal 200% extra damage."
end

function Skill_Attack.ResolveBattleUse(Level, Source, Target, Result)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)
	if Random.GetInt(1, 100) <= 4 + Level then
		Damage = Damage * 3
	end

	Result.DamageDealt = Damage
	Result.TargetHealthChange = -Damage

	return Result
end

Skill_Heal = { ManaCost = 5 }

function Skill_Heal:GetInfo(Level)

	return "Heal"
end

function Skill_Heal.ResolveBattleUse(Level, Source, Target, Result)

	return Result
end

function Skill_Heal.CanUse(Level, Object)
	if Object.Mana >= Skill_Heal.ManaCost then
		return 1
	end

	return 0
end

function Skill_Heal.ApplyCost(Level, Result)
	Result.SourceManaChange = -Skill_Heal.ManaCost;

	return Result
end
