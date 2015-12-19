-- Monster attack --

Skill_MonsterAttack = {}

function Skill_MonsterAttack.GetInfo(Level)
	return ""
end

function Skill_MonsterAttack.Use(Level, Source, Target, Result)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)
	Result.TargetHealthChange = -Damage

	return Result
end

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
	--Result.Buff = Buffs["Buff_Bleeding"]
	--Result.BuffLevel = 1
	--Result.BuffDuration = 5

	return Result
end

-- Whirl --

Skill_Whirl = {}

function Skill_Whirl.GetInfo(Level)
	Chance = 9 + Level

	return "Attack all enemies with 30% weapon damage.\n" .. Chance .. "% chance to cause bleeding."
end

function Skill_Whirl.Use(Level, Source, Target, Result)
	Damage = math.floor(Source.GenerateDamage() * 0.3)
	Damage = math.max(Damage - Target.GenerateDefense(), 0)

	Result.TargetHealthChange = -Damage
	if Random.GetInt(1, 100) <= 9 + Level then
		Result.Buff = Buffs["Buff_Bleeding"]
		Result.BuffLevel = 1
		Result.BuffDuration = 10
	end

	return Result
end

-- Heal --

Skill_Heal = { ManaCostBase = 3, HealBase = 10 }

function Skill_Heal.GetInfo(Level)

	return "Heal target for " .. (Skill_Heal.HealBase + Level * 5) .. "HP\nCost " .. math.floor(Skill_Heal.ManaCostBase + Level / 3) .. " mana"
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
	Result.SourceManaChange = -math.floor(Skill_Heal.ManaCostBase + Level / 3)

	return Result
end

-- Flame --

Skill_Flame = {
	ManaCostBase = 5,
	DamageBase = 5,
	Mult = 2,
	CostPerLevel = 0.5,
	GetDamage = function(Level)
		return Skill_Flame.DamageBase + Level * Skill_Flame.Mult
	end,
	GetCost = function(Level)
		return math.floor(Skill_Flame.ManaCostBase + Level * Skill_Flame.CostPerLevel)
	end
}

function Skill_Flame.GetInfo(Level)

	return "Burn all targets for " .. Skill_Flame.GetDamage(Level) .. "HP\nCost " .. Skill_Flame.GetCost(Level) .. " mana"
end

function Skill_Flame.Use(Level, Source, Target, Result)

	Damage = math.max(Skill_Flame.GetDamage(Level) - Target.GenerateDefense(), 0)
	Result.TargetHealthChange = -Damage

	return Result
end

function Skill_Flame.CanUse(Level, Object)
	if Object.Mana >= Skill_Flame.GetCost(Level) then
		return 1
	end

	return 0
end

function Skill_Flame.ApplyCost(Level, Result)
	Result.SourceManaChange = -Skill_Flame.GetCost(Level)

	return Result
end

-- Bolt --

Skill_Bolt = {
	ManaCostBase = 5,
	DamageBase = 15,
	Mult = 6,
	CostPerLevel = 1,
	GetDamage = function(Level)
		return Skill_Bolt.DamageBase + Level * Skill_Bolt.Mult
	end,
	GetCost = function(Level)
		return math.floor(Skill_Bolt.ManaCostBase + Level * Skill_Bolt.CostPerLevel)
	end
}

function Skill_Bolt.GetInfo(Level)

	return "Strike a target for " .. Skill_Bolt.GetDamage(Level) .. "HP\nCost " .. Skill_Bolt.GetCost(Level) .. " mana"
end

function Skill_Bolt.Use(Level, Source, Target, Result)

	Damage = math.max(Skill_Bolt.GetDamage(Level) - Target.GenerateDefense(), 0)
	Result.TargetHealthChange = -Damage

	return Result
end

function Skill_Bolt.CanUse(Level, Object)
	if Object.Mana >= Skill_Bolt.GetCost(Level) then
		return 1
	end

	return 0
end

function Skill_Bolt.ApplyCost(Level, Result)
	Result.SourceManaChange = -Skill_Bolt.GetCost(Level)

	return Result
end
