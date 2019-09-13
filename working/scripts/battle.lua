-- Base Attack Skill --
Base_Attack = {
	Stamina = 0,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetInfo = function(self, Item)
		return ""
	end,

	CanUse = function(self, Level, Object)
		return Object.Stamina > self.Stamina
	end,

	ApplyCost = function(self, Level, Result)
		Result.Source.Stamina = -self.Stamina

		return Result
	end,

	GetDamageType = function(self, Object)
		Weapon = Object.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
		if Weapon ~= nil then
			return Weapon.DamageType
		end

		return self.Item.DamageType
	end,

	GetAttackTimes = function(self, Object)
		Weapon = Object.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
		if Weapon ~= nil then
			return Weapon.AttackDelay, Weapon.AttackTime, Weapon.Cooldown
		end

		return self.Item.AttackDelay, self.Item.AttackTime, self.Item.Cooldown
	end,

	GenerateDamage = function(self, Level, Source)
		Damage = Source.GenerateDamage()

		return Damage
	end,

	Proc = function(self, Roll, Level, Duration, Source, Target, Result)

		return Result
	end,

	Use = function(self, Level, Duration, Source, Target, Result)
		Hit = Battle_ResolveDamage(self, Level, Source, Target, Result)

		if Hit then
			self:Proc(Random.GetInt(1, 100), Level, Duration, Source, Target, Result)
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

	Proc = function(self, Roll, Level, Duration, Source, Target, Result)

		return Result
	end,

	Use = function(self, Level, Duration, Source, Target, Result)
		Damage = self:GetDamage(Level)
		Damage = math.floor(Damage * Target.GetDamageReduction(self.Item.DamageType))
		Damage = math.max(Damage, 0)

		Result.Target.Health = -Damage

		self:Proc(Random.GetInt(1, 100), Level, Duration, Source, Target, Result)

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
		return math.floor(self.Increments * Level)
	end,

	Increments = 1
}

-- Calculate basic weapon damage vs target's armor
function Battle_ResolveDamage(Action, Level, Source, Target, Result)

	if Random.GetInt(1, 100) <= (Source.HitChance - Target.Evasion) then

		-- Get damage
		Change = {}
		Change.Damage, Crit = Action:GenerateDamage(Level, Source)
		if Crit == true then
			Result.Target.Crit = true
		end

		-- Reduce damage
		Change.Damage = math.floor(Change.Damage * Source.AttackPower)

		-- Call OnHit methods for buffs
		StaminaChange = 0
		BuffID = 0
		for i = 1, #Target.StatusEffects do
			Effect = Target.StatusEffects[i]
			if Effect.Buff.OnHit ~= nil then
				Effect.Buff:OnHit(Effect.Level, Change)
				StaminaChange = StaminaChange + Change.Stamina
				BuffID = Change.ID
			end
		end

		-- Update stamina
		if StaminaChange ~= 0 then
			Result.Target.Stamina = StaminaChange
			Result.Target.ID = BuffID
		end

		-- Apply pierce
		DamageBlock = math.max(Target.DamageBlock - Source.Pierce, 0);

		-- Apply damage block
		Change.Damage = math.max(Change.Damage - DamageBlock, 0)

		-- Apply resistance
		Change.Damage = Change.Damage * Target.GetDamageReduction(Action:GetDamageType(Source))

		-- Update health
		Change.Damage = math.floor(Change.Damage)
		Result.Target.Health = -Change.Damage
		Hit = true
	else
		Result.Target.Miss = true
		Hit = false
	end

	return Hit
end

-- Storage for battle instance data
Battles = {}
