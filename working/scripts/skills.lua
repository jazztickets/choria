-- Basic attack --

Skill_Attack = {}

function Skill_Attack.GetInfo(Level)
	Chance = 4 + Level

	return "Attack with your weapon.\n" .. Chance .. "% chance to deal 200% extra damage."
end

function Skill_Attack.Use(Level, Source, Target, Result)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)
	if Random.GetInt(1, 100) <= 4 + Level then
		Damage = Damage * 3
	end

	Result.TargetHealthChange = -Damage
	Result.Buff = Buffs["Buff_Bleeding"]

	return Result
end

-- Heal --

Skill_Heal = { ManaCostBase = 2, HealBase = 10 }

function Skill_Heal.GetInfo(Level)

	return "Heal target for " .. (Skill_Heal.HealBase + Level * 5) .. "HP"
end

function Skill_Heal.Use(Level, Source, Target, Result)

	Result.TargetHealthChange = Skill_Heal.HealBase + Level * 5

	return Result
end

function Skill_Heal.CanUse(Level, Object)
	if Object.Mana >= Skill_Heal.ManaCostBase then
		return 1
	end

	return 0
end

function Skill_Heal.ApplyCost(Level, Result)
	Result.SourceManaChange = -Skill_Heal.ManaCostBase;

	return Result
end
