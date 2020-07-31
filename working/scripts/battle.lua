-- Base Attack Skill --
Base_Attack = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetPierce = function(self, Source)
		return Source.Pierce
	end,

	GetInfo = function(self, Item)
		return ""
	end,

	GetDamageType = function(self, Object)
		Weapon = Object.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
		if Weapon ~= nil then
			return Weapon.DamageType
		end

		return self.Item.DamageType
	end,

	GenerateDamage = function(self, Level, Source)
		Damage = Source.GenerateDamage()

		return Damage
	end,

	Proc = function(self, Roll, Level, Duration, Source, Target, Result)

		return false
	end,

	Use = function(self, Level, Duration, Source, Target, Result)
		Hit = Battle_ResolveDamage(self, Level, Source, Target, Result)

		if Hit then
			self:Proc(Random.GetInt(1, 100), Level, Duration, Source, Target, Result)
			WeaponProc(Source, Target, Result, false)
		end

		return Result
	end
}

-- Base Spell Skill --
Base_Spell = {
	DamageType = 0,
	ManaCostBase = 0,
	DamageBase = 0,
	DamagePerLevel = 0,
	DamageScale = 0,
	Duration = 0,
	DurationPerLevel = 0,
	CostPerLevel = 0,
	CostScale = 0.15,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetDamage = function(self, Source, Level)
		return math.floor((self.DamageBase + Level * self.DamagePerLevel + Level * Level * self.DamageScale) * self:GetDamagePower(Source, Level))
	end,

	GetDuration = function(self, Level)
		return math.floor(10 * (self.Duration + self.DurationPerLevel * (Level - 1))) / 10.0
	end,

	GetDamagePower = function(self, Source, Level)
		return Source.SpellDamage * 0.01
	end,

	GetManaCost = function(self, Level)
		return math.floor(math.max(self.ManaCostBase + Level * self.CostPerLevel + Level * Level * self.CostScale, 0))
	end,

	ApplyCost = function(self, Source, Level, Result)
		Result.Source.Mana = -self:GetManaCost(Level)

		return Result
	end,

	CanUse = function(self, Level, Source, Target)
		if Source.Mana >= self:GetManaCost(Level) then
			return true
		end

		return false
	end,

	Proc = function(self, Roll, Level, Duration, Source, Target, Result)

		return false
	end,

	Use = function(self, Level, Duration, Source, Target, Result)
		Change = {}
		Change.Damage = self:GetDamage(Source, Level)
		Change.Damage = math.floor(Change.Damage * Target.GetDamageReduction(self.Item.DamageType))
		Change.Damage = math.max(Change.Damage, 0)

		Result.Target.DamageType = self.Item.DamageType

		Update = ResolveManaReductionRatio(Target, Change.Damage)
		Result.Target.Health = Update.Health
		Result.Target.Mana = Update.Mana

		self:Proc(Random.GetInt(1, 100), Level, Duration, Source, Target, Result)
		WeaponProc(Source, Target, Result, true)

		return Result
	end
}

-- Base Summon Spell Skill --
Base_SummonSpell = {
	ManaCostBase = 0,
	CostPerLevel = 0,
	DamageScale = 0,
	SkillLevel = 0,
	SkillLevelPerLevel = 0,
	SkillLevelPower = 0.75,
	SpecialChance = 0,
	SpecialChancePerLevel = 0,
	Duration = -1,
	DurationPerLevel = 0,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetSpecialChance = function(self, Level)
		return math.floor(self.SpecialChance + (Level - 1) * self.SpecialChancePerLevel)
	end,

	GetManaCost = function(self, Level)
		return math.max(self.ManaCostBase + Level * self.CostPerLevel, 0)
	end,

	ApplyCost = function(self, Source, Level, Result)
		Result.Source.Mana = -self:GetManaCost(Level)

		return Result
	end,

	CanUse = function(self, Level, Source, Target)
		if Source.Mana >= self:GetManaCost(Level) then
			return true
		end

		return false
	end,

	GetHealth = function(self, Source, Level)
		return math.floor((self.BaseHealth + (Level - 1) * self.HealthPerLevel) * Source.PetPower)
	end,

	GetMana = function(self, Source, Level)
		return math.floor((self.BaseMana + (Level - 1) * self.ManaPerLevel) * Source.PetPower)
	end,

	GetArmor = function(self, Source, Level)
		return math.floor((self.BaseArmor + (Level - 1) * self.ArmorPerLevel) * Source.PetPower)
	end,

	GetLimit = function(self, Source, Level)
		return math.floor(self.Limit + (Level) * self.LimitPerLevel + Source.SummonLimit)
	end,

	GetSkillLevel = function(self, Source, Level)
		return math.max(math.floor((self.SkillLevel + (Level - 1) * self.SkillLevelPerLevel) * Source.PetPower * self.SkillLevelPower), 1)
	end,

	GetDuration = function(self, Source, Level)
		return math.floor(self.Duration + (Level) * self.DurationPerLevel)
	end,

	GetDamage = function(self, Source, Level)
		AddedDamage = math.floor((Level - 1) * self.DamagePerLevel) + Level * Level * self.DamageScale
		return math.floor((self.BaseMinDamage + AddedDamage) * Source.PetPower), math.floor((self.BaseMaxDamage + AddedDamage) * Source.PetPower)
	end
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

		-- Call OnHit methods for buffs
		StaminaChange = 0
		BuffSound = 0
		for i = 1, #Target.StatusEffects do
			Effect = Target.StatusEffects[i]
			if Effect.Buff.OnHit ~= nil then
				Effect.Buff:OnHit(Target, Effect.Level, Effect.Duration, Change)
				if Change.Stamina ~= nil then
					StaminaChange = StaminaChange + Change.Stamina
				end
				if Change.BuffSound ~= nil then
					BuffSound = Change.BuffSound
				end
			end
		end

		-- Update stamina
		if StaminaChange ~= 0 then
			Result.Target.Stamina = StaminaChange
		end

		-- Set sound from buff
		if BuffSound ~= 0 then
			Result.Target.BuffSound = BuffSound
		end

		-- Apply damage block
		Change.Damage = math.max(Change.Damage - Target.DamageBlock, 0)

		-- Apply resistance
		Result.Target.DamageType = Action:GetDamageType(Source)
		Change.Damage = Change.Damage * Target.GetDamageReduction(Action:GetDamageType(Source)) + Action:GetPierce(Source)

		-- Update health
		Change.Damage = math.floor(Change.Damage)

		-- Handle mana damage reduction
		Update = ResolveManaReductionRatio(Target, Change.Damage)
		Result.Target.Health = Update.Health
		Result.Target.Mana = Update.Mana

		Hit = true
	else
		Result.Target.Miss = true
		Hit = false
	end

	return Hit
end

-- Convert damage into health/mana reduction based on target's mana reduction ratio
function ResolveManaReductionRatio(Target, Damage)
	Update = {}
	Update.Health = 0
	Update.Mana = 0
	if Target.ManaReductionRatio > 0 then
		Update.Health = -Damage * (1.0 - Target.ManaReductionRatio)
		Update.Mana = -Damage * Target.ManaReductionRatio

		-- Remove fractions from damage
		Fraction = math.abs(Update.Health) - math.floor(math.abs(Update.Health))
		Update.Health = Update.Health + Fraction
		Update.Mana = Update.Mana - Fraction

		-- Carry over extra mana damage to health
		ResultingMana = Target.Mana + Update.Mana
		if ResultingMana < 0 then
			Update.Health = Update.Health + ResultingMana
			Update.Mana = Update.Mana - ResultingMana
		end
	else
		Update.Health = -Damage
	end

	return Update
end

-- Roll for proccing weapons
function WeaponProc(Source, Target, Result, IsSpell)
	if Target == nil then
		return
	end

	-- Check for main weapon
	Weapon = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if Weapon == nil or Weapon.Script == nil or (IsSpell == true and Weapon.SpellProc == 0) then
		return
	end

	-- Proc main weapon
	if (Weapon.Script.SpellOnly == true and IsSpell == true) or (Weapon.Script.SpellOnly == false and IsSpell == false) then
		Weapon.Script:Proc(Random.GetInt(1, 100), Weapon, Source, Target, Result)
	end

	-- Check for off-hand
	WeaponOffHand = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if WeaponOffHand == nil or WeaponOffHand.Script == nil or (IsSpell == true and WeaponOffHand.SpellProc == 0) then
		return
	end

	-- Proc off-hand weapon
	if (WeaponOffHand.Script.SpellOnly == true and IsSpell == true) or (WeaponOffHand.Script.SpellOnly == false and IsSpell == false) then
		WeaponOffHand.Script:Proc(Random.GetInt(1, 100), WeaponOffHand, Source, Target, Result)
	end
end

-- Storage for battle instance data
Battles = {}
