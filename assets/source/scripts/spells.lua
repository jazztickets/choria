-- Base Spell --

Base_Spell = {
	DamageType = 0,
	ManaCostBase = 0,
	DamageBase = 0,
	DamagePerLevel = 0,
	DamageScale = 0,
	DamagePower = 2,
	DamageRange = 0,
	Duration = 0,
	Targets = 1,
	TargetsPerLevel = 0,
	DurationPerLevel = 0,
	CostPerLevel = 0,
	CostScale = 0.15,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GenerateDamage = function(self, Source, Level)
		return Random.GetInt(self:GetMinDamage(Source, Level), self:GetMaxDamage(Source, Level))
	end,

	GetDamage = function(self, Source, Level)
		return math.floor((self.DamageBase + (Level - 1) * self.DamagePerLevel + math.pow(Level, self.DamagePower) * self.DamageScale) * self:GetDamagePower(Source, Level))
	end,

	GetMinDamage = function(self, Source, Level)
		Damage = self:GetDamage(Source, Level)

		return math.max(Damage - math.floor(Damage * self.DamageRange), 0)
	end,

	GetMaxDamage = function(self, Source, Level)
		Damage = self:GetDamage(Source, Level)

		return Damage + math.floor(Damage * self.DamageRange)
	end,

	GetDamageText = function(self, Source, Item)
		if Item.MoreInfo == true then
			return "[c green]" .. self:GetDamage(Source, Item.Level) .. " [c green]avg[c white]"
		else
			return "[c green]" .. self:GetMinDamage(Source, Item.Level) .. "-" .. self:GetMaxDamage(Source, Item.Level) .. "[c white]"
		end
	end,

	GetTargetCount = function(self, Level, Fraction)
		Value = self.Targets + self.TargetsPerLevel * Level
		if Fraction == nil or Fraction == false then
			return math.floor(Value)
		else
			return Value
		end
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
		Change.Damage = self:GenerateDamage(Source, Level)
		Change.Damage = math.floor(Change.Damage * Target.GetDamageReduction(self.Item.DamageType))
		Change.Damage = math.max(Change.Damage, 0)

		Result.Target.DamageType = self.Item.DamageType
		Result.Target.Health = -Change.Damage

		self:Proc(Random.GetInt(1, 100), Level, Duration, Source, Target, Result)
		WeaponProc(Source, Target, Result, true)

		return Result
	end
}

-- Base Summon Spell --

Base_SummonSpell = {
	Count = 1,
	CountPerLevel = 0,
	ManaCostBase = 0,
	CostPerLevel = 0,
	DamageScale = 0,
	SkillLevel = 0,
	SkillLevelPerLevel = 0,
	SkillLevelPower = 0.75,
	ResistAllSummonPowerScale = 0.2,
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

	GetCount = function(self, Source, Level, MoreInfo)
		Value = self.Count + (Level - 1) * self.CountPerLevel
		if MoreInfo then
			return Round(Value)
		else
			return math.floor(Value)
		end
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
		return math.floor((self.BaseHealth + (Level - 1) * self.HealthPerLevel) * Source.SummonPower * 0.01)
	end,

	GetMana = function(self, Source, Level)
		return math.floor((self.BaseMana + (Level - 1) * self.ManaPerLevel) * Source.SummonPower * 0.01)
	end,

	GetArmor = function(self, Source, Level, Fraction)
		Value = (self.BaseArmor + (Level - 1) * self.ArmorPerLevel) * Source.SummonPower * 0.01
		if Fraction == nil or Fraction == false then
			return math.floor(Value)
		else
			return Round(Value)
		end
	end,

	GetResistAll = function(self, Source, Level, Fraction)
		Value = (self.BaseResistAll + (Level - 1) * self.ResistAllPerLevel)
		Value = Value + Value * (Source.SummonPower - 100) * self.ResistAllSummonPowerScale * 0.01
		if Fraction == nil or Fraction == false then
			return math.floor(Value)
		else
			return Round(Value)
		end
	end,

	GetLimit = function(self, Source, Level, Fraction)
		Value = self.Limit + (Level) * self.LimitPerLevel + Source.SummonLimit
		if Fraction == nil or Fraction == false then
			return math.floor(Value)
		else
			return Round(Value)
		end
	end,

	GetSkillLevel = function(self, Source, Level)
		return math.min(math.max(math.floor((self.SkillLevel + (Level - 1) * self.SkillLevelPerLevel) * Source.SummonPower * 0.01 * self.SkillLevelPower), 1), MAX_SKILL_LEVEL)
	end,

	GetDuration = function(self, Source, Level)
		return math.floor(self.Duration + (Level) * self.DurationPerLevel)
	end,

	GetDamage = function(self, Source, Level)
		AddedDamage = math.floor((Level - 1) * self.DamagePerLevel) + Level * Level * self.DamageScale
		return math.floor((self.BaseMinDamage + AddedDamage) * Source.SummonPower * 0.01), math.floor((self.BaseMaxDamage + AddedDamage) * Source.SummonPower * 0.01)
	end,

	GetDamageText = function(self, Source, Item)
		MinDamage, MaxDamage = self:GetDamage(Source, Item.Level)
		if Item.MoreInfo == true then
			return "[c green]" .. (MinDamage + MaxDamage) / 2 .. " [c green]avg[c white]"
		else
			return "[c green]" .. MinDamage .. "-" .. MaxDamage  .. "[c white]"
		end
	end,
}

-- Rejuvenation --

Skill_Rejuvenation = Base_Spell:New()
Skill_Rejuvenation.Duration = 5
Skill_Rejuvenation.CostPerLevel = 5
Skill_Rejuvenation.LevelPerLevel = 5
Skill_Rejuvenation.ManaCostBase = 5 - Skill_Rejuvenation.CostPerLevel

function Skill_Rejuvenation.GetHeal(self, Source, Level)
	return math.floor(Buff_Healing.Heal * self:GetLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_Rejuvenation.GetLevel(self, Source, Level)
	return math.floor(self.LevelPerLevel * Level * Source.HealPower * 0.01)
end

function Skill_Rejuvenation.GetInfo(self, Source, Item)
	return "Heal [c green]" .. self:GetHeal(Source, Item.Level) .. " [c white]HP [c white]over [c green]" .. math.floor(self:GetDuration(Item.Level)) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Rejuvenation.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Healing.Pointer
	Result.Target.BuffLevel = self:GetLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Rejuvenation.PlaySound(self)
	Audio.Play("rejuv0.ogg")
end

-- Heal --

Skill_Heal = Base_Spell:New()
Skill_Heal.HealBase = 0
Skill_Heal.HealPerLevel = 50
Skill_Heal.CostPerLevel = 15
Skill_Heal.ManaCostBase = 15 - Skill_Heal.CostPerLevel

function Skill_Heal.GetHeal(self, Source, Level)
	return math.floor((self.HealBase + self.HealPerLevel * Level) * Source.HealPower * 0.01)
end

function Skill_Heal.GetInfo(self, Source, Item)
	return "Heal target for [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Heal.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Health = self:GetHeal(Source, Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Heal.PlaySound(self)
	Audio.Play("heal0.ogg")
end

-- Resurrect --

Skill_Resurrect = Base_Spell:New()
Skill_Resurrect.HealBase = 100
Skill_Resurrect.HealPerLevel = 25
Skill_Resurrect.ManaBase = 100
Skill_Resurrect.ManaPerLevel = 25
Skill_Resurrect.Targets = 1
Skill_Resurrect.TargetsPerLevel = 0.1
Skill_Resurrect.CostPerLevel = 20
Skill_Resurrect.ManaCostBase = 200 - Skill_Resurrect.CostPerLevel

function Skill_Resurrect.GetHeal(self, Source, Level)
	return math.floor((self.HealBase + self.HealPerLevel * Level) * Source.HealPower * 0.01)
end

function Skill_Resurrect.GetMana(self, Source, Level)
	return math.floor((self.ManaBase + self.ManaPerLevel * Level) * Source.ManaPower * 0.01)
end

function Skill_Resurrect.GetInfo(self, Source, Item)
	Count = self:GetTargetCount(Item.Level, Item.MoreInfo)

	Ally = "ally"
	if Count ~= 1 then
		Ally = "allies"
	end

	return "Resurrect [c green]" .. Count .. "[c white] fallen " .. Ally .. ", giving them [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP and [c green]" .. self:GetMana(Source, Item.Level) .. "[c white] MP\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Can be used outside of battle"
end

function Skill_Resurrect.CanTarget(self, Source, Target, Alive)
	if Target == nil then
		return false
	end

	return Target.Corpse > 0 and Target.Health == 0
end

function Skill_Resurrect.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Health = self:GetHeal(Source, Level)
	Result.Target.Mana = self:GetMana(Source, Level)
	Result.Target.Corpse = 1
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Resurrect.PlaySound(self)
	Audio.Play("choir0.ogg")
end

-- Spark --

Skill_Spark = Base_Spell:New()
Skill_Spark.DamageBase = 40
Skill_Spark.DamagePerLevel = 25
Skill_Spark.DamageScale = 0.025
Skill_Spark.DamagePower = 3
Skill_Spark.DamageRange = 0.3
Skill_Spark.CostPerLevel = 4
Skill_Spark.ManaCostBase = 10 - Skill_Spark.CostPerLevel
Skill_Spark.Duration = 1.0
Skill_Spark.DurationPerLevel = 0.075
Skill_Spark.Chance = 25.2
Skill_Spark.ChancePerLevel = 0.2
Skill_Spark.CostScale = 0.08

function Skill_Spark.GetDamagePower(self, Source, Level)
	return Source.LightningPower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_Spark.GetDuration(self, Level)
	return math.floor(10 * (self.Duration + self.DurationPerLevel * (Level - 1))) / 10.0
end

function Skill_Spark.GetChance(self, Level)
	return math.floor(self.Chance + self.ChancePerLevel * (Level - 1))
end

function Skill_Spark.GetInfo(self, Source, Item)
	return "Shock a target for " .. self:GetDamageText(Source, Item) .. " lightning damage with a [c green]" .. self:GetChance(Item.Level) .. "%[c white] chance to stun for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Spark.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = self:GetDuration(Level)
		return true
	end

	return false
end

function Skill_Spark.PlaySound(self)
	Audio.Play("shock0.ogg", 0.35)
end

-- Icicle --

Skill_Icicle = Base_Spell:New()
Skill_Icicle.DamageBase = 35
Skill_Icicle.DamagePerLevel = 20
Skill_Icicle.DamageScale = 0.023
Skill_Icicle.DamagePower = 3
Skill_Icicle.DamageRange = 0.1
Skill_Icicle.CostPerLevel = 4
Skill_Icicle.Slow = 30
Skill_Icicle.SlowPerLevel = 0.25
Skill_Icicle.Duration = 3.0
Skill_Icicle.DurationPerLevel = 0.05
Skill_Icicle.ManaCostBase = 15 - Skill_Icicle.CostPerLevel
Skill_Icicle.CostScale = 0.08

function Skill_Icicle.GetDamagePower(self, Source, Level)
	return Source.ColdPower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_Icicle.GetSlow(self, Level)
	return math.floor(self.Slow + self.SlowPerLevel * Level)
end

function Skill_Icicle.GetInfo(self, Source, Item)
	return "Pierce a target for " .. self:GetDamageText(Source, Item) .. " cold damage\nSlows by [c green]" .. self:GetSlow(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Icicle.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = self:GetSlow(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

	return true
end

function Skill_Icicle.PlaySound(self)
	Audio.Play("ice" .. Random.GetInt(0, 1) .. ".ogg", 0.4)
end

-- Poison Touch --

Skill_PoisonTouch = Base_Spell:New()
Skill_PoisonTouch.PoisonLevelPerLevel = 6
Skill_PoisonTouch.PoisonLevel = 10 - Skill_PoisonTouch.PoisonLevelPerLevel
Skill_PoisonTouch.PoisonLevelScale = 0.5
Skill_PoisonTouch.CostPerLevel = 7
Skill_PoisonTouch.ManaCostBase = 20 - Skill_PoisonTouch.CostPerLevel
Skill_PoisonTouch.DurationPerLevel = 0
Skill_PoisonTouch.Duration = 10
Skill_PoisonTouch.CostScale = 0.08

function Skill_PoisonTouch.GetPoisonLevel(self, Source, Level)
	return math.floor((self.PoisonLevel + self.PoisonLevelPerLevel * Level + Level * Level * self.PoisonLevelScale) * self:GetDamagePower(Source, Level))
end

function Skill_PoisonTouch.GetDamage(self, Source, Level)
	return math.floor(self:GetPoisonLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_PoisonTouch.GetDamagePower(self, Source, Level)
	return Source.PoisonPower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_PoisonTouch.GetInfo(self, Source, Item)
	if Item.MoreInfo == true then
		DamageValue = self:GetPoisonLevel(Source, Item.Level) .. "[c white] poison DPS"
	else
		DamageValue = self:GetDamage(Source, Item.Level) .. "[c white] poison damage over [c green]" .. self.Duration .. "[c white] seconds"
	end

	return "Infuse venom into your enemy, dealing [c green]" .. DamageValue .. "\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Heal power is reduced when poisoned"
end

function Skill_PoisonTouch.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = self:GetPoisonLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_PoisonTouch.PlaySound(self)
	Audio.Play("touch0.ogg")
end


-- Fireball --

Skill_Fireball = Base_Spell:New()
Skill_Fireball.DamageBase = 60
Skill_Fireball.DamagePerLevel = 20
Skill_Fireball.DamageScale = 1.5
Skill_Fireball.DamagePower = 2
Skill_Fireball.DamageRange = 0.2
Skill_Fireball.CostPerLevel = 10
Skill_Fireball.ManaCostBase = 50 - Skill_Fireball.CostPerLevel
Skill_Fireball.Targets = 3
Skill_Fireball.TargetsPerLevel = 0

function Skill_Fireball.GetDamagePower(self, Source, Level)
	return Source.FirePower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_Fireball.GetInfo(self, Source, Item)
	return "Hurl a flaming ball at [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] foes for [c green]" .. self:GetDamageText(Source, Item) .. "[c white] fire damage\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Fireball.PlaySound(self)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Fire Blast --

Skill_FireBlast = Base_Spell:New()
Skill_FireBlast.DamageBase = 150
Skill_FireBlast.DamagePerLevel = 30
Skill_FireBlast.DamageScale = 2
Skill_FireBlast.DamageRange = 0.2
Skill_FireBlast.BurnLevel = 10
Skill_FireBlast.BurnLevelPerLevel = 5
Skill_FireBlast.CostPerLevel = 20
Skill_FireBlast.ManaCostBase = 160 - Skill_FireBlast.CostPerLevel
Skill_FireBlast.Targets = 4
Skill_FireBlast.Duration = 6
Skill_FireBlast.TargetsPerLevel = 0.08

function Skill_FireBlast.GetDamagePower(self, Source, Level)
	return Source.FirePower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_FireBlast.GetBurnLevel(self, Source, Level)
	return math.floor((self.BurnLevel + self.BurnLevelPerLevel * Level) * self:GetDamagePower(Source, Level))
end

function Skill_FireBlast.GetBurnDamage(self, Source, Level)
	return math.floor(self:GetBurnLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_FireBlast.GetInfo(self, Source, Item)
	if Item.MoreInfo == true then
		DamageValue = self:GetBurnLevel(Source, Item.Level) .. "[c white] fire DPS"
	else
		DamageValue = self:GetBurnDamage(Source, Item.Level) .. "[c white] fire damage over [c green]" .. self.Duration .. "[c white] seconds"
	end

	return "Blast [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] foes for [c green]" .. self:GetDamageText(Source, Item) .. "[c white] fire damage, then igniting them for [c green]" .. DamageValue .. "\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_FireBlast.PlaySound(self)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

function Skill_FireBlast.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = self:GetBurnLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

	return true
end

-- Ice Nova --

Skill_IceNova = Base_Spell:New()
Skill_IceNova.DamageBase = 125
Skill_IceNova.DamagePerLevel = 25
Skill_IceNova.DamageScale = 2
Skill_IceNova.DamageRange = 0.15
Skill_IceNova.CostPerLevel = 20
Skill_IceNova.ManaCostBase = 180 - Skill_IceNova.CostPerLevel
Skill_IceNova.Targets = 4
Skill_IceNova.TargetsPerLevel = 0.08
Skill_IceNova.Slow = 20
Skill_IceNova.SlowPerLevel = 0.25
Skill_IceNova.Duration = 2
Skill_IceNova.DurationPerLevel = 0.05

function Skill_IceNova.GetDamagePower(self, Source, Level)
	return Source.ColdPower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_IceNova.GetSlow(self, Level)
	return math.floor(self.Slow + self.SlowPerLevel * Level)
end

function Skill_IceNova.GetInfo(self, Source, Item)
	return "Summon an icy explosion, hitting [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] enemies for [c green]" .. self:GetDamageText(Source, Item) .. "[c white] cold damage that slows by [c green]" .. self:GetSlow(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_IceNova.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = 20
	Result.Target.BuffDuration = self:GetDuration(Level)

	return true
end

function Skill_IceNova.PlaySound(self)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Chain Lightning --

Skill_ChainLightning = Base_Spell:New()
Skill_ChainLightning.DamageBase = 170
Skill_ChainLightning.DamagePerLevel = 20
Skill_ChainLightning.DamageScale = 2.25
Skill_ChainLightning.DamageRange = 0.35
Skill_ChainLightning.CostPerLevel = 18
Skill_ChainLightning.ManaCostBase = 190 - Skill_ChainLightning.CostPerLevel
Skill_ChainLightning.Targets = 4
Skill_ChainLightning.TargetsPerLevel = 0.08
Skill_ChainLightning.Duration = 1
Skill_ChainLightning.DurationPerLevel = 0.05
Skill_ChainLightning.Chance = 15.2
Skill_ChainLightning.ChancePerLevel = 0.2

function Skill_ChainLightning.GetDamagePower(self, Source, Level)
	return Source.LightningPower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_ChainLightning.GetDuration(self, Level)
	return math.floor(10 * (self.Duration + self.DurationPerLevel * (Level - 1))) / 10.0
end

function Skill_ChainLightning.GetChance(self, Level)
	return math.floor(self.Chance + self.ChancePerLevel * (Level - 1))
end

function Skill_ChainLightning.GetInfo(self, Source, Item)
	return "Summon a powerful bolt of energy, hitting [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] enemies for [c green]" .. self:GetDamageText(Source, Item) .. "[c white] lightning damage with a [c green]" .. self:GetChance(Item.Level) .. "%[c white] chance to stun for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. "[c white] MP"
end

function Skill_ChainLightning.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = self:GetDuration(Level)
		return true
	end

	return false
end

function Skill_ChainLightning.PlaySound(self)
	Audio.Play("shock0.ogg", 0.35)
end

-- Rupture --

Skill_Rupture = Base_Spell:New()
Skill_Rupture.DamageBase = 130
Skill_Rupture.DamagePerLevel = 30
Skill_Rupture.DamageScale = 2.5
Skill_Rupture.DamageRange = 0.2
Skill_Rupture.Level = 20
Skill_Rupture.LevelPerLevel = 10
Skill_Rupture.CostPerLevel = 30
Skill_Rupture.ManaCostBase = 190 - Skill_Rupture.CostPerLevel
Skill_Rupture.Targets = 3
Skill_Rupture.TargetsPerLevel = 0.1
Skill_Rupture.Duration = 10
Skill_Rupture.DurationPerLevel = 0

function Skill_Rupture.GetPoisonDamage(self, Source, Level)
	return math.floor(self:GetLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_Rupture.GetDamagePower(self, Source, Level)
	return Source.PhysicalPower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_Rupture.GetLevel(self, Source, Level)
	return math.floor((self.Level + self.LevelPerLevel * Level) * (Source.PoisonPower * 0.01 * Source.SpellDamage * 0.01))
end

function Skill_Rupture.CanTarget(self, Source, Target, Alive)
	if Target == nil then
		return false
	end

	if Alive == true then
		return Target.Health > 0
	else
		return Target.Corpse > 0 and Target.Health == 0
	end
end

function Skill_Rupture.CanUse(self, Level, Source, Target)
	if Source.Mana < self:GetManaCost(Level) then
		return false
	end

	return self:CanTarget(Source, Target, false)
end

function Skill_Rupture.GetInfo(self, Source, Item)
	if Item.MoreInfo == true then
		DamageValue = self:GetLevel(Source, Item.Level) .. "[c white] poison DPS"
	else
		DamageValue = self:GetPoisonDamage(Source, Item.Level) .. "[c white] poison damage over [c green]" .. self.Duration .. "[c white] seconds"
	end

	return "Explode a corpse, dealing [c green]" .. self:GetDamageText(Source, Item) .. "[c white] physical damage and releasing noxious gas, covering [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] enemies that deals [c green]" .. DamageValue .. "\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Rupture.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Target.Health == 0 then
		Result.Target.Corpse = -1
	else
		Result.Target.Buff = Buff_Poisoned.Pointer
		Result.Target.BuffLevel = self:GetLevel(Source, Level)
		Result.Target.BuffDuration = self:GetDuration(Level)
	end

	return true
end

function Skill_Rupture.PlaySound(self)
	Audio.Play("rupture0.ogg")
end

-- Ignite --

Skill_Ignite = Base_Spell:New()
Skill_Ignite.BurnLevel = 30
Skill_Ignite.BurnLevelPerLevel = 9
Skill_Ignite.BurnLevelScale = 0.5
Skill_Ignite.CostPerLevel = 8
Skill_Ignite.ManaCostBase = 30 - Skill_Ignite.CostPerLevel
Skill_Ignite.Duration = 6
Skill_Ignite.CostScale = 0.08

function Skill_Ignite.GetBurnLevel(self, Source, Level)
	return math.floor((self.BurnLevel + self.BurnLevelPerLevel * Level + Level * Level * self.BurnLevelScale) * self:GetDamagePower(Source, Level))
end

function Skill_Ignite.GetDamage(self, Source, Level)
	return math.floor(self:GetBurnLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_Ignite.GetDamagePower(self, Source, Level)
	return Source.FirePower * 0.01 * Source.SpellDamage * 0.01
end

function Skill_Ignite.GetInfo(self, Source, Item)
	if Item.MoreInfo == true then
		DamageValue = self:GetBurnLevel(Source, Item.Level) .. "[c white] fire DPS"
	else
		DamageValue = self:GetDamage(Source, Item.Level) .. "[c white] fire damage over [c green]" .. self.Duration .. "[c white] seconds"
	end

	return "Ignite an enemy and deal [c green]" .. DamageValue .. "\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Ignite.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = self:GetBurnLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Ignite.PlaySound(self)
	Audio.Play("flame0.ogg")
end

-- Demonic Conjuring --

Skill_DemonicConjuring = Base_SummonSpell:New()
Skill_DemonicConjuring.CostPerLevel = 10
Skill_DemonicConjuring.ManaCostBase = 25 - Skill_DemonicConjuring.CostPerLevel
Skill_DemonicConjuring.BaseHealth = 100
Skill_DemonicConjuring.BaseMinDamage = 15
Skill_DemonicConjuring.BaseMaxDamage = 25
Skill_DemonicConjuring.BaseArmor = 1.1
Skill_DemonicConjuring.BaseResistAll = 0.5
Skill_DemonicConjuring.HealthPerLevel = 20
Skill_DemonicConjuring.DamagePerLevel = 10
Skill_DemonicConjuring.ArmorPerLevel = 0.1
Skill_DemonicConjuring.ResistAllPerLevel = 0.5
Skill_DemonicConjuring.Limit = 1
Skill_DemonicConjuring.LimitPerLevel = 0.05
Skill_DemonicConjuring.DamageScale = 0.30
Skill_DemonicConjuring.SpecialChance = 35
Skill_DemonicConjuring.SpecialChancePerLevel = 0
Skill_DemonicConjuring.Monster = Monsters[23]
Skill_DemonicConjuring.SpecialMonster = Monsters[39]

function Skill_DemonicConjuring.GetInfo(self, Source, Item)
	return "Summon a demon that has [c green]" .. self:GetHealth(Source, Item.Level) .. "[c white] HP, [c green]" .. self:GetArmor(Source, Item.Level, Item.MoreInfo) .. "[c white] armor, [c green]+" .. self:GetResistAll(Source, Item.Level, Item.MoreInfo) .. "%[c white] resist all and does [c green]" .. self:GetDamageText(Source, Item) .. "[c white] fire damage\n[c green]" .. self:GetSpecialChance(Item.Level) .. "%[c white] chance to summon an ice imp that deals cold damage\nCan summon a maximum of [c green]" .. self:GetLimit(Source, Item.Level, Item.MoreInfo) .. "[c white]\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Heals lowest health demon at limit"
end

function Skill_DemonicConjuring.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Summons = {}
	Result.Summons[1] = {}
	Result.Summons[1].SpellID = self.Item.ID
	Result.Summons[1].ID = self.Monster.ID
	Result.Summons[1].Health = self:GetHealth(Source, Level)
	Result.Summons[1].MinDamage, Result.Summons[1].MaxDamage = self:GetDamage(Source, Level)
	Result.Summons[1].Armor = self:GetArmor(Source, Level)
	Result.Summons[1].ResistAll = self:GetResistAll(Source, Level)
	Result.Summons[1].SummonBuff = Buff_SummonDemon.Pointer
	Result.Summons[1].Duration = self:GetDuration(Source, Level)
	Result.Summons[1].BattleSpeed = Source.SummonBattleSpeed

	-- Limit monster summons to 1
	if Source.MonsterID == 0 then
		Result.Summons[1].Limit = self:GetLimit(Source, Level)
	else
		Result.Summons[1].Limit = 1
	end

	Roll = Random.GetInt(1, 100)
	if Result.SummonBuff ~= nil then
		Result.Summons[1].SummonBuff = Result.SummonBuff
		if Result.SummonBuff == Buff_SummonIceImp.Pointer then
			Roll = 1
		else
			Roll = 1000
		end
	end

	if Roll <= self:GetSpecialChance(Level) then
		Result.Summons[1].ID = self.SpecialMonster.ID
		Result.Summons[1].SummonBuff = Buff_SummonIceImp.Pointer
	end

	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_DemonicConjuring.PlaySound(self)
	Audio.Play("summon0.ogg")
end

-- Raise Dead --

Skill_RaiseDead = Base_SummonSpell:New()
Skill_RaiseDead.CostPerLevel = 10
Skill_RaiseDead.Count = 1.04
Skill_RaiseDead.CountPerLevel = 0.04
Skill_RaiseDead.ManaCostBase = 15 - Skill_RaiseDead.CostPerLevel
Skill_RaiseDead.BaseHealth = 50
Skill_RaiseDead.BaseMana = 30
Skill_RaiseDead.BaseMinDamage = 10
Skill_RaiseDead.BaseMaxDamage = 15
Skill_RaiseDead.BaseArmor = 1
Skill_RaiseDead.BaseResistAll = 0.5
Skill_RaiseDead.HealthPerLevel = 19
Skill_RaiseDead.ManaPerLevel = 20
Skill_RaiseDead.DamagePerLevel = 9.5
Skill_RaiseDead.ArmorPerLevel = 0.07
Skill_RaiseDead.ResistAllPerLevel = 0.5
Skill_RaiseDead.SpecialChance = 35
Skill_RaiseDead.SpecialChancePerLevel = 0
Skill_RaiseDead.Limit = 2
Skill_RaiseDead.LimitPerLevel = 0.075
Skill_RaiseDead.SkillLevel = 1
Skill_RaiseDead.SkillLevelPerLevel = 0.5
Skill_RaiseDead.SpecialDamage = 0.85
Skill_RaiseDead.DamageScale = 0.25
Skill_RaiseDead.Monster = Monsters[20]
Skill_RaiseDead.SpecialMonster = Monsters[21]

function Skill_RaiseDead.CanTarget(self, Source, Target, Alive)
	if Target == nil then
		return false
	end

	return Target.Corpse > 0 and Target.Health == 0
end

function Skill_RaiseDead.CanUse(self, Level, Source, Target)
	if Source.Mana < self:GetManaCost(Level) then
		return false
	end

	return self:CanTarget(Source, Target, false)
end

function Skill_RaiseDead.GetInfo(self, Source, Item)
	Count = self:GetCount(Source, Item.Level, Item.MoreInfo)

	Plural = ""
	Has = "has"
	if Count ~= 1 then
		Plural = "s"
		Has = "have"
	end

	return "Raise [c green]" .. Count .. "[c white] skeleton" .. Plural .. " from a corpse that ".. Has .. " [c green]" .. self:GetHealth(Source, Item.Level) .. "[c white] HP, [c green]" .. self:GetArmor(Source, Item.Level, Item.MoreInfo) .. "[c white] armor, [c green]+" .. self:GetResistAll(Source, Item.Level, Item.MoreInfo) .. "%[c white] resist all and [c green]" .. self:GetDamageText(Source, Item) .. "[c white] damage\n[c green]" .. self:GetSpecialChance(Item.Level) .. "%[c white] chance to summon a skeleton priest that can heal but only deals [c green]" .. math.floor(self.SpecialDamage * 100) .. "%[c white] damage\nCan summon a maximum of [c green]" .. self:GetLimit(Source, Item.Level, Item.MoreInfo) .. "[c white]\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Heals lowest health skeleton at limit"
end

function Skill_RaiseDead.Use(self, Level, Duration, Source, Target, Result, Priority)

	Count = self:GetCount(Source, Level, false)
	if Result.SummonBuff ~= nil then
		Count = 1
	end

	Result.Summons = {}
	for i = 1, Count do
		Result.Summons[i] = {}
		Result.Summons[i].SpellID = self.Item.ID
		Result.Summons[i].ID = self.Monster.ID
		Result.Summons[i].Health = self:GetHealth(Source, Level)
		Result.Summons[i].MinDamage, Result.Summons[i].MaxDamage = self:GetDamage(Source, Level)
		Result.Summons[i].Armor = self:GetArmor(Source, Level)
		Result.Summons[i].ResistAll = self:GetResistAll(Source, Level)
		Result.Summons[i].Limit = self:GetLimit(Source, Level)
		Result.Summons[i].SkillLevel = self:GetSkillLevel(Source, Level)
		Result.Summons[i].Duration = self:GetDuration(Source, Level)
		Result.Summons[i].SummonBuff = Buff_SummonSkeleton.Pointer
		Result.Summons[i].BattleSpeed = Source.SummonBattleSpeed

		Roll = Random.GetInt(1, 100)
		if Result.SummonBuff ~= nil then
			Result.Summons[i].SummonBuff = Result.SummonBuff
			if Result.SummonBuff == Buff_SummonSkeletonPriest.Pointer then
				Roll = 1
			else
				Roll = 1000
			end
		end

		if Roll <= self:GetSpecialChance(Level) then
			Result.Summons[i].ID = self.SpecialMonster.ID
			Result.Summons[i].Mana = self:GetMana(Source, Level)
			Result.Summons[i].SummonBuff = Buff_SummonSkeletonPriest.Pointer
			Result.Summons[i].MinDamage = math.ceil(Result.Summons[i].MinDamage * self.SpecialDamage)
			Result.Summons[i].MaxDamage = math.ceil(Result.Summons[i].MaxDamage * self.SpecialDamage)
		end
	end

	Result.Target.Corpse = -1
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_RaiseDead.PlaySound(self)
	Audio.Play("summon0.ogg")
end

-- Enfeeble --

Skill_Enfeeble = Base_Spell:New()
Skill_Enfeeble.Constant = 97
Skill_Enfeeble.BasePercent = 24
Skill_Enfeeble.Multiplier = 150
Skill_Enfeeble.DurationPerLevel = 0.1
Skill_Enfeeble.Duration = 5
Skill_Enfeeble.CostPerLevel = 10
Skill_Enfeeble.ManaCostBase = 10 - Skill_Enfeeble.CostPerLevel
Skill_Enfeeble.Targets = 1
Skill_Enfeeble.TargetsPerLevel = 0.1

function Skill_Enfeeble.GetPercent(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Enfeeble.GetInfo(self, Source, Item)
	Count = self:GetTargetCount(Item.Level)
	Plural = ""
	if Count ~= 1 then
		Plural = "s"
	end

	return "Cripple [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] foe" .. Plural .. ", reducing their attack damage by [c green]" .. self:GetPercent(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Enfeeble.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Weak.Pointer
	Result.Target.BuffLevel = self:GetPercent(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Enfeeble.PlaySound(self)
	Audio.Play("enfeeble0.ogg")
end

-- Flay --

Skill_Flay = Base_Spell:New()
Skill_Flay.Duration = 5
Skill_Flay.DurationPerLevel = 0.1
Skill_Flay.CostPerLevel = 10
Skill_Flay.ManaCostBase = 20 - Skill_Flay.CostPerLevel
Skill_Flay.Targets = 1
Skill_Flay.TargetsPerLevel = 0.14
Skill_Flay.Constant = 100
Skill_Flay.BasePercent = 14
Skill_Flay.Multiplier = 200

function Skill_Flay.GetPercent(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Flay.GetInfo(self, Source, Item)
	Count = self:GetTargetCount(Item.Level)
	Plural = ""
	if Count ~= 1 then
		Plural = "s"
	end

	return "Strip the skin of [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] foe" .. Plural .. ", reducing their elemental resistances by [c green]" .. self:GetPercent(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Attracts summons"
end

function Skill_Flay.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Flayed.Pointer
	Result.Target.BuffLevel = self:GetPercent(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	Result.Target.BuffPriority = Priority
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Flay.PlaySound(self)
	Audio.Play("swamp0.ogg")
end

-- Fracture --

Skill_Fracture = Base_Spell:New()
Skill_Fracture.Duration = 5
Skill_Fracture.DurationPerLevel = 0.1
Skill_Fracture.CostPerLevel = 5
Skill_Fracture.ManaCostBase = 40 - Skill_Fracture.CostPerLevel
Skill_Fracture.Targets = 1
Skill_Fracture.TargetsPerLevel = 0.14
Skill_Fracture.Constant = 100
Skill_Fracture.BasePercent = 14
Skill_Fracture.Multiplier = 200

function Skill_Fracture.GetPercent(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Fracture.GetInfo(self, Source, Item)
	TargetCount = self:GetTargetCount(Item.Level)
	Plural = "enemy"
	if TargetCount ~= 1 then
		Plural = "enemies"
	end

	return "Decimate the defenses of [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] " .. Plural .. ", reducing their physical, poison, and bleed resistances by [c green]" .. self:GetPercent(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Attracts summons"
end

function Skill_Fracture.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Fractured.Pointer
	Result.Target.BuffLevel = self:GetPercent(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	Result.Target.BuffPriority = Priority
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Fracture.PlaySound(self)
	Audio.Play("enfeeble0.ogg")
end

-- Light --

Skill_Light = Base_Spell:New()
Skill_Light.Duration = 20
Skill_Light.DurationPerLevel = 5
Skill_Light.CostPerLevel = 5
Skill_Light.ManaCostBase = 50 - Skill_Light.CostPerLevel

function Skill_Light.GetInfo(self, Source, Item)
	return "Emit magic light for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Light.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Light.Pointer
	Result.Target.BuffLevel = 20 + Level
	if Result.Target.BuffLevel > 30 then
		Result.Target.BuffLevel = 30
	end
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

-- Magic Barrier --

Skill_MagicBarrier = Base_Spell:New()
Skill_MagicBarrier.CostPerLevel = 5
Skill_MagicBarrier.ManaCostBase = 25 - Skill_MagicBarrier.CostPerLevel
Skill_MagicBarrier.BaseBlock = 120
Skill_MagicBarrier.BlockPerLevel = 40
Skill_MagicBarrier.Duration = 10
Skill_MagicBarrier.DurationPerLevel = 0.5

function Skill_MagicBarrier.GetLevel(self, Source, Level)
	return math.floor((self.BaseBlock + self.BlockPerLevel * (Level - 1)) * Source.ManaPower * 0.01)
end

function Skill_MagicBarrier.GetInfo(self, Source, Item)
	return "Create a magic shield around an ally that absorbs [c green]" .. self:GetLevel(Source, Item.Level) .. "[c white] attack damage\nLasts [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nShield increased with [c yellow]mana power\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_MagicBarrier.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Shielded.Pointer
	Result.Target.BuffLevel = self:GetLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_MagicBarrier.PlaySound(self)
	Audio.Play("barrier0.ogg")
end

-- Sanctuary --

Skill_Sanctuary = Base_Spell:New()
Skill_Sanctuary.Level = 10
Skill_Sanctuary.LevelPerLevel = 1
Skill_Sanctuary.CostPerLevel = 20
Skill_Sanctuary.ManaCostBase = 100 - Skill_Sanctuary.CostPerLevel
Skill_Sanctuary.Targets = 3
Skill_Sanctuary.TargetsPerLevel = 0.1
Skill_Sanctuary.Duration = 10
Skill_Sanctuary.DurationPerLevel = 0

function Skill_Sanctuary.GetHeal(self, Source, Level)
	return math.floor(Buff_Sanctuary.Heal * self:GetLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_Sanctuary.GetArmor(self, Source, Level)
	return math.floor(Buff_Sanctuary.Armor * self:GetLevel(Source, Level))
end

function Skill_Sanctuary.GetDamageBlock(self, Source, Level)
	return math.floor(Buff_Sanctuary.DamageBlock * self:GetLevel(Source, Level))
end

function Skill_Sanctuary.GetLevel(self, Source, Level)
	return math.floor((self.Level + self.LevelPerLevel * (Level - 1)) * Source.HealPower * 0.01)
end

function Skill_Sanctuary.GetInfo(self, Source, Item)
	return "Imbue [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] allies with sanctuary for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds, granting [c green]" .. self:GetArmor(Source, Item.Level) .. "[c white] armor, [c green]" .. self:GetDamageBlock(Source, Item.Level) .. "[c white] damage block and [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Sanctuary.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Sanctuary.Pointer
	Result.Target.BuffLevel = self:GetLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Sanctuary.PlaySound(self)
	Audio.Play("sanctuary0.ogg")
end

-- Empower --

Skill_Empower = Base_Spell:New()
Skill_Empower.Level = 17
Skill_Empower.LevelPerLevel = 1.7
Skill_Empower.CostPerLevel = 20
Skill_Empower.ManaCostBase = 100 - Skill_Empower.CostPerLevel
Skill_Empower.Targets = 3
Skill_Empower.TargetsPerLevel = 0.1
Skill_Empower.Duration = 10
Skill_Empower.DurationPerLevel = 0

function Skill_Empower.GetLevel(self, Source, Level)
	return math.floor(self.Level + self.LevelPerLevel * (Level - 1))
end

function Skill_Empower.GetInfo(self, Source, Item)
	return "Empower [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] allies with [c green]" .. self:GetLevel(Source, Item.Level) .. "%[c white] attack power for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Empower.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Empowered.Pointer
	Result.Target.BuffLevel = self:GetLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Empower.PlaySound(self)
	Audio.Play("enfeeble0.ogg")
end
