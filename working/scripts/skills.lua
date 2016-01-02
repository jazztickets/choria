-- Monster attack --

Skill_MonsterAttack = Base_Attack:New()

-- Spider bite --

Skill_SpiderBite = Base_Attack:New()

function Skill_SpiderBite.Proc(self, Roll, Level, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buffs["Buff_Slowed"]
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
	end
end

-- Fang bite --

Skill_FangBite = Base_Attack:New()

function Skill_FangBite.Proc(self, Roll, Level, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buffs["Buff_Bleeding"]
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
	end
end

-- Swoop attack --

Skill_Swoop = Base_Attack:New()

function Skill_Swoop.Proc(self, Roll, Level, Source, Target, Result)
	if Roll <= 75 then
		Result.Target.Buff = Buffs["Buff_Stunned"]
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = 3
	end
end

-- Basic attack --

Skill_Attack = Base_Attack:New()
Skill_Attack.BaseChance = 4
Skill_Attack.ChancePerLevel = 1

function Skill_Attack.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_Attack.GetInfo(self, Level)

	return "Attack with your weapon\n[c green]" .. self:GetChance(Level) .. "% [c white]chance to deal [c green]200% [c white]extra damage"
end

function Skill_Attack.GenerateDamage(self, Level, Source)
	Damage = Source.GenerateDamage()

	Crit = false
	if Random.GetInt(1, 100) <= self:GetChance(Level) then
		Damage = Damage * 3
		Crit = true
	end

	return Damage, Crit
end

-- Gash --

Skill_Gash = Base_Attack:New()
Skill_Gash.BaseChance = 14
Skill_Gash.ChancePerLevel = 1
Skill_Gash.Duration = 5
Skill_Gash.BleedingLevel = 1

function Skill_Gash.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_Gash.GetInfo(self, Level)

	return "Slice your enemy\n[c green]" .. self:GetChance(Level) .. "% [c white]chance to cause [c yellow]bleeding"
end

function Skill_Gash.Proc(self, Roll, Level, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buffs["Buff_Bleeding"]
		Result.Target.BuffLevel = self.BleedingLevel
		Result.Target.BuffDuration = self.Duration
	end
end

-- Shield Bash --

Skill_ShieldBash = Base_Attack:New()
Skill_ShieldBash.BaseChance = 23
Skill_ShieldBash.ChancePerLevel = 2
Skill_ShieldBash.Duration = 3

function Skill_ShieldBash.GetInfo(self, Level)

	return "Bash with your enemy with a shield\n[c green]" .. self:GetChance(Level) .. "% [c white]chance to stun for [c green]" .. self.Duration .. " [c white]seconds"
end

function Skill_ShieldBash.GenerateDamage(self, Level, Source)
	Shield = Source.GetInventoryItem(INVENTORY_HAND2)
	if Shield == nil then
		return 0
	end

	return Shield.GenerateDefense()
end

function Skill_ShieldBash.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_ShieldBash.CanUse(self, Level, Object)
	Shield = Object.GetInventoryItem(INVENTORY_HAND2)

	return Shield ~= nil
end

function Skill_ShieldBash.Proc(self, Roll, Level, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buffs["Buff_Stunned"]
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = self.Duration
	end
end

-- Whirl --

Skill_Whirlwind = Base_Attack:New()
Skill_Whirlwind.DamageBase = 29
Skill_Whirlwind.DamagePerLevel = 1
Skill_Whirlwind.SlowDurationPerLevel = 1.0 / 3.0
Skill_Whirlwind.SlowDuration = 3 - Skill_Whirlwind.SlowDurationPerLevel

function Skill_Whirlwind.GetDuration(self, Level)
	return math.floor(Skill_Whirlwind.SlowDuration + Skill_Whirlwind.SlowDurationPerLevel * Level)
end

function Skill_Whirlwind.GetDamage(self, Level)
	return Skill_Whirlwind.DamageBase + Skill_Whirlwind.DamagePerLevel * Level
end

function Skill_Whirlwind.GenerateDamage(self, Level, Source)
	return math.floor(Source.GenerateDamage() * (self:GetDamage(Level) / 100))
end

function Skill_Whirlwind.ApplyCost(self, Level, Result)
	Result.Source.Buff = Buffs["Buff_Slowed"]
	Result.Source.BuffLevel = 3
	Result.Source.BuffDuration = self:GetDuration(Level)

	return Result
end

function Skill_Whirlwind.GetInfo(self, Level)
	return "Slash all enemies with [c green]" .. self:GetDamage(Level) .. "% [c white]weapon damage\nCauses fatigue for [c green]" .. self:GetDuration(Level) .." [c white]seconds"
end

-- Heal --

Skill_Heal = Base_Spell:New()
Skill_Heal.HealBase = 10
Skill_Heal.CostPerLevel = 1.0 / 3.0
Skill_Heal.ManaCostBase = 3 - Skill_Heal.CostPerLevel

function Skill_Heal.GetInfo(self, Level)

	return "Heal target for [c green]" .. (self.HealBase + Level * 5) .. "[c white] HP\nCost [c light_blue]" .. self:GetCost(Level) .. " [c white]MP"
end

function Skill_Heal.Use(self, Level, Source, Target, Result)

	Result.Target.Health = self.HealBase + Level * 5

	return Result
end

-- Spark --
Skill_Spark = Base_Spell:New()
Skill_Spark.ManaCostBase = 1
Skill_Spark.DamageBase = 1
Skill_Spark.Multiplier = 2
Skill_Spark.CostPerLevel = 1 / 3

function Skill_Spark.GetInfo(self, Level)
	return "Shock a target for [c green]" .. self:GetDamage(Level) .. "[c white] HP\nCost [c light_blue]" .. self:GetCost(Level) .. " [c white]MP"
end

-- Fire Blast --
Skill_FireBlast = Base_Spell:New()
Skill_FireBlast.ManaCostBase = 9
Skill_FireBlast.DamageBase = 5
Skill_FireBlast.Multiplier = 2
Skill_FireBlast.CostPerLevel = 1

function Skill_FireBlast.GetInfo(self, Level)
	return "Blast all targets with fire for [c green]" .. self:GetDamage(Level) .. "[c white] HP\nCost [c light_blue]" .. self:GetCost(Level) .. " [c white]MP"
end

-- Toughness --

Skill_Toughness = { PerLevel = 4 }

function Skill_Toughness.GetInfo(self, Level)

	return "Increase max HP by [c green]" .. Skill_Toughness.PerLevel * Level
end

function Skill_Toughness.Stats(self, Level, Object, Change)
	Change.MaxHealth = self.PerLevel * Level

	return Change
end

-- Arcane Mastery --

Skill_ArcaneMastery = { PerLevel = 2 }

function Skill_ArcaneMastery.GetInfo(self, Level)

	return "Increase max MP by [c light_blue]" .. Skill_ArcaneMastery.PerLevel * Level
end

function Skill_ArcaneMastery.Stats(self, Level, Object, Change)
	Change.MaxMana = self.PerLevel * Level

	return Change
end

-- Evasion --

Skill_Evasion = { ChancePerLevel = 0.01, BaseChance = 0.09 }

function Skill_Evasion.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 1)
end

function Skill_Evasion.GetInfo(self, Level)

	return "Increase evasion by [c green]" .. math.floor(self:GetChance(Level) * 100) .. "%"
end

function Skill_Evasion.Stats(self, Level, Object, Change)
	Change.Evasion = self:GetChance(Level)

	return Change
end
