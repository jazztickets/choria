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

	GenerateDamage = function(self, Level, Source)
		Damage = Source.GenerateDamage()

		return Damage
	end,

	Use = function(self, Level, Source, Target, Result)
		Hit = Battle_ResolveDamage(self, Level, Source, Target, Result)

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
		if Object.Mana >= self:GetCost(Level) then
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

-- Calculate basic weapon damage vs target's armor
function Battle_ResolveDamage(Action, Level, Source, Target, Result)
	Damage = math.max(Action:GenerateDamage(Level, Source) - Target.GenerateDefense(), 0)

	if Random.GetInt(1, 100) <= (Source.HitChance - Target.Evasion) * 100 then
		Result.Target.Health = -Damage
		Hit = true
	else
		Result.Target.Miss = true
		Hit = false
	end

	return Hit
end
