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

	Proc = function(self, Roll, Level, Source, Target, Result)

		return Result
	end,

	Use = function(self, Level, Source, Target, Result)
		Hit = Battle_ResolveDamage(self, Level, Source, Target, Result)

		if Hit then
			self:Proc(Random.GetInt(1, 100), Level, Source, Target, Result)
		end

		return Result
	end
}

-- Base Spell Skill --
Base_Spell = {
	DamageType = 0,
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
		return self.ManaCostBase + Level * self.CostPerLevel
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
		Damage = self:GetDamage(Level)
		Damage = math.floor(Damage * (1 - Target.GetResistance(self.DamageType)))
		Damage = math.max(Damage, 0)

		Result.Target.Health = -Damage

		return Result
	end
}

-- Base Buff --
Base_Buff = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetInfo = function(self, Level)
		return ""
	end,

	GetChange = function(self, Level)
		return self.Increments * Level / 100
	end,

	Increments = 10
}

-- Calculate basic weapon damage vs target's armor
function Battle_ResolveDamage(Action, Level, Source, Target, Result)

	if Random.GetInt(1, 100) <= (Source.HitChance - Target.Evasion) * 100 then

		-- Get damage
		Change = {}
		Change.Damage, Crit = Action:GenerateDamage(Level, Source)
		if Crit == true then
			Result.Target.Crit = true
		end

		-- Call OnHit methods for buffs
		Result.Target.Stamina = 0
		for i = 1, #Target.StatusEffects do
			Effect = Target.StatusEffects[i]
			if Effect.Buff.OnHit ~= nil then
				Effect.Buff:OnHit(Effect.Level, Change)
				Result.Target.Stamina = Result.Target.Stamina + Change.Stamina
			end
		end

		-- Apply defense
		Defense = Target.GenerateDefense()
		Change.Damage = math.max(Change.Damage - Defense, 0)

		-- Update health
		Result.Target.Health = -Change.Damage
		Hit = true
	else
		Result.Target.Miss = true
		Hit = false
	end

	return Hit
end
