-- Base Attack Skill --
Base_Attack = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetInfo = function(self, Level)
		return ""
	end,

	Use = function(self, Level, Source, Target, Result)
		Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)
		Result.Target.Health = -Damage

		return Result
	end
}

-- Base Spell Skill --
Base_Spell = {
	ManaCostBase = 0,
	DamageBase = 0,
	Multiplier = 0,
	CostPerLevel = 0,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetDamage = function(self, Level)
		return self.DamageBase + Level * self.Multiplier
	end,

	GetCost = function(self, Level)
		return math.floor(self.ManaCostBase + Level * self.CostPerLevel)
	end,

	ApplyCost = function(self, Level, Result)
		Result.Source.Mana = -self:GetCost(Level)

		return Result
	end,

	CanUse = function(self, Level, Object)
		if math.ceil(Object.Mana) >= self:GetCost(Level) then
			return true
		end

		return false
	end,

	Use = function(self, Level, Source, Target, Result)

		Damage = math.max(self:GetDamage(Level) - Target.GenerateDefense(), 0)
		Result.Target.Health = -Damage

		return Result
	end
}

-- Monster attack --

Skill_MonsterAttack = Base_Attack:New()

-- Basic attack --

Skill_Attack = Base_Attack:New()

function Skill_Attack.GetInfo(self, Level)
	Chance = 4 + Level

	return "Attack with your weapon\n[c green]" .. Chance .. "% [c white]chance to deal [c green]200% [c white]extra damage"
end

function Skill_Attack.Use(self, Level, Source, Target, Result)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)
	if Random.GetInt(1, 100) <= 4 + Level then
		Damage = Damage * 3
	end

	Result.Target.Health = -Damage

	return Result
end

-- Shield Bash --

Skill_ShieldBash = Base_Attack:New()
Skill_ShieldBash.BaseChance = 23
Skill_ShieldBash.ChancePerLevel = 2
Skill_ShieldBash.Duration = 3

function Skill_ShieldBash.GetInfo(self, Level)
	return "Bash with your enemy with a shield\n[c green]" .. self:GetChance(Level) .. "% [c white]chance to stun for [c green]" .. self.Duration .. " [c white]seconds"
end

function Skill_ShieldBash.GetChance(self, Level)
	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_ShieldBash.CanUse(self, Level, Object)
	Shield = Object.GetInventoryItem(INVENTORY_HAND2)

	return Shield ~= nil
end

function Skill_ShieldBash.Use(self, Level, Source, Target, Result)
	Shield = Source.GetInventoryItem(INVENTORY_HAND2)
	if Shield == nil then
		return Result
	end

	ShieldDamage = Shield.GenerateDefense()
	Damage = math.max(ShieldDamage - Target.GenerateDefense(), 0)

	if Random.GetInt(1, 100) <= self:GetChance(Level) then
		Result.Buff = Buffs["Buff_Stunned"]
		Result.BuffLevel = 1
		Result.BuffDuration = self.Duration
	end

	Result.Target.Health = -Damage

	return Result
end

-- Whirl --

Skill_Whirl = Base_Attack:New()

function Skill_Whirl.GetInfo(self, Level)
	Chance = 9 + Level

	return "Slash all enemies with [c green]30% [c white]weapon damage\n[c green]" .. Chance .. "% [c white]chance to cause [c yellow]bleeding"
end

function Skill_Whirl.Use(self, Level, Source, Target, Result)
	Damage = math.floor(Source.GenerateDamage() * 0.3)
	Damage = math.max(Damage - Target.GenerateDefense(), 0)

	Result.Target.Health = -Damage
	if Random.GetInt(1, 100) <= 9 + Level then
		Result.Buff = Buffs["Buff_Bleeding"]
		Result.BuffLevel = 1
		Result.BuffDuration = 10
	end

	return Result
end

-- Heal --

Skill_Heal = Base_Spell:New()
Skill_Heal.ManaCostBase = 3
Skill_Heal.HealBase = 10
Skill_Heal.CostPerLevel = 1.0 / 3.0

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
