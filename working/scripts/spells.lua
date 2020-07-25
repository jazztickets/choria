
-- Rejuvenation --

Skill_Rejuvenation = Base_Spell:New()
Skill_Rejuvenation.Duration = 5
Skill_Rejuvenation.CostPerLevel = 5
Skill_Rejuvenation.ManaCostBase = 5 - Skill_Rejuvenation.CostPerLevel

function Skill_Rejuvenation.GetHeal(self, Source, Level)
	return math.floor(Buff_Healing.Heal * self:GetLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_Rejuvenation.GetLevel(self, Source, Level)
	return math.floor(10 * Level * Source.HealPower)
end

function Skill_Rejuvenation.GetInfo(self, Source, Item)
	return "Heal [c green]" .. self:GetHeal(Source, Item.Level) .. " [c white]HP [c white]over [c green]" .. math.floor(self:GetDuration(Item.Level)) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Rejuvenation.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Healing.Pointer
	Result.Target.BuffLevel = self:GetLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Rejuvenation.PlaySound(self, Level)
	Audio.Play("rejuv0.ogg")
end

-- Heal --

Skill_Heal = Base_Spell:New()
Skill_Heal.HealBase = 0
Skill_Heal.HealPerLevel = 100
Skill_Heal.CostPerLevel = 15
Skill_Heal.ManaCostBase = 15 - Skill_Heal.CostPerLevel

function Skill_Heal.GetHeal(self, Source, Level)
	return math.floor((self.HealBase + self.HealPerLevel * Level) * Source.HealPower + 0.001)
end

function Skill_Heal.GetInfo(self, Source, Item)
	return "Heal target for [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Heal.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Health = self:GetHeal(Source, Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Heal.PlaySound(self, Level)
	Audio.Play("heal0.ogg")
end

-- Resurrect --

Skill_Resurrect = Base_Spell:New()
Skill_Resurrect.HealBase = 0
Skill_Resurrect.HealPerLevel = 25
Skill_Resurrect.CostPerLevel = 20
Skill_Resurrect.ManaCostBase = 200 - Skill_Resurrect.CostPerLevel

function Skill_Resurrect.GetHeal(self, Source, Level)
	return math.floor((self.HealBase + self.HealPerLevel * Level) * Source.HealPower + 0.001)
end

function Skill_Resurrect.GetInfo(self, Source, Item)
	return "Resurrect an ally with [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Can be used outside of battle"
end

function Skill_Resurrect.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Health = self:GetHeal(Source, Level)
	Result.Target.Corpse = 1
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Resurrect.PlaySound(self, Level)
	Audio.Play("choir0.ogg")
end

-- Spark --

Skill_Spark = Base_Spell:New()
Skill_Spark.DamageBase = 10
Skill_Spark.DamagePerLevel = 30
Skill_Spark.DamageScale = 0.75
Skill_Spark.CostPerLevel = 4
Skill_Spark.ManaCostBase = 10 - Skill_Spark.CostPerLevel
Skill_Spark.Duration = 1.0
Skill_Spark.DurationPerLevel = 0.075
Skill_Spark.Chance = 25.2
Skill_Spark.ChancePerLevel = 0.2
Skill_Spark.CostScale = 0.08

function Skill_Spark.GetDamagePower(self, Source, Level)
	return Source.LightningPower * Source.SpellDamage * 0.01
end

function Skill_Spark.GetDuration(self, Level)
	return math.floor(10 * (self.Duration + self.DurationPerLevel * (Level - 1))) / 10.0
end

function Skill_Spark.GetChance(self, Level)
	return math.floor(self.Chance + self.ChancePerLevel * (Level - 1))
end

function Skill_Spark.GetInfo(self, Source, Item)
	return "Shock a target for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] lightning damage with a [c green]" .. self:GetChance(Item.Level) .. "%[c white] chance to stun for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
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

function Skill_Spark.PlaySound(self, Level)
	Audio.Play("shock0.ogg", 0.35)
end

-- Icicle --

Skill_Icicle = Base_Spell:New()
Skill_Icicle.DamageBase = 25
Skill_Icicle.DamagePerLevel = 25
Skill_Icicle.DamageScale = 0.70
Skill_Icicle.CostPerLevel = 4
Skill_Icicle.Slow = 30
Skill_Icicle.SlowPerLevel = 0.25
Skill_Icicle.Duration = 3.0
Skill_Icicle.DurationPerLevel = 0.05
Skill_Icicle.ManaCostBase = 15 - Skill_Icicle.CostPerLevel
Skill_Icicle.CostScale = 0.08

function Skill_Icicle.GetDamagePower(self, Source, Level)
	return Source.ColdPower * Source.SpellDamage * 0.01
end

function Skill_Icicle.GetSlow(self, Level)
	return math.floor(self.Slow + self.SlowPerLevel * Level)
end

function Skill_Icicle.GetInfo(self, Source, Item)
	return "Pierce a target for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] cold damage\nSlows by [c green]" .. self:GetSlow(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Icicle.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = self:GetSlow(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

	return true
end

function Skill_Icicle.PlaySound(self, Level)
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
	return Source.PoisonPower * Source.SpellDamage * 0.01
end

function Skill_PoisonTouch.GetInfo(self, Source, Item)
	return "Infuse venom into your enemy, dealing [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] poison damage over [c green]" .. math.floor(self:GetDuration(Item.Level)) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_PoisonTouch.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = self:GetPoisonLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_PoisonTouch.PlaySound(self, Level)
	Audio.Play("touch0.ogg")
end

-- Fire Blast --

Skill_FireBlast = Base_Spell:New()
Skill_FireBlast.DamageBase = 120
Skill_FireBlast.DamagePerLevel = 30
Skill_FireBlast.DamageScale = 2
Skill_FireBlast.BurnLevel = 10
Skill_FireBlast.BurnLevelPerLevel = 5
Skill_FireBlast.CostPerLevel = 20
Skill_FireBlast.ManaCostBase = 160 - Skill_FireBlast.CostPerLevel
Skill_FireBlast.BaseTargets = 4
Skill_FireBlast.Duration = 6
Skill_FireBlast.TargetsPerLevel = 0.08

function Skill_FireBlast.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_FireBlast.GetDamagePower(self, Source, Level)
	return Source.FirePower * Source.SpellDamage * 0.01
end

function Skill_FireBlast.GetBurnLevel(self, Source, Level)
	return math.floor((self.BurnLevel + self.BurnLevelPerLevel * Level) * self:GetDamagePower(Source, Level))
end

function Skill_FireBlast.GetBurnDamage(self, Source, Level)
	return math.floor(self:GetBurnLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_FireBlast.GetInfo(self, Source, Item)
	return "Blast [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] foes for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] fire damage, then igniting them for [c green]" .. self:GetBurnDamage(Source, Item.Level) .. "[c white] damage over [c green]" .. math.floor(self:GetDuration(Item.Level)) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_FireBlast.PlaySound(self, Level)
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
Skill_IceNova.DamageBase = 100
Skill_IceNova.DamagePerLevel = 25
Skill_IceNova.DamageScale = 2
Skill_IceNova.CostPerLevel = 20
Skill_IceNova.ManaCostBase = 180 - Skill_IceNova.CostPerLevel
Skill_IceNova.BaseTargets = 4
Skill_IceNova.TargetsPerLevel = 0.08
Skill_IceNova.Slow = 20
Skill_IceNova.SlowPerLevel = 0.25
Skill_IceNova.Duration = 2
Skill_IceNova.DurationPerLevel = 0.05

function Skill_IceNova.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_IceNova.GetDamagePower(self, Source, Level)
	return Source.ColdPower * Source.SpellDamage * 0.01
end

function Skill_IceNova.GetSlow(self, Level)
	return math.floor(self.Slow + self.SlowPerLevel * Level)
end

function Skill_IceNova.GetInfo(self, Source, Item)
	return "Summon an icy explosion, hitting [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] enemies for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] cold damage that slows by [c green]" .. self:GetSlow(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_IceNova.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = 20
	Result.Target.BuffDuration = self:GetDuration(Level)

	return true
end

function Skill_IceNova.PlaySound(self, Level)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Chain Lightning --

Skill_ChainLightning = Base_Spell:New()
Skill_ChainLightning.DamageBase = 150
Skill_ChainLightning.DamagePerLevel = 20
Skill_ChainLightning.DamageScale = 2.25
Skill_ChainLightning.CostPerLevel = 18
Skill_ChainLightning.ManaCostBase = 190 - Skill_ChainLightning.CostPerLevel
Skill_ChainLightning.BaseTargets = 4
Skill_ChainLightning.TargetsPerLevel = 0.08
Skill_ChainLightning.Duration = 1
Skill_ChainLightning.DurationPerLevel = 0.05
Skill_ChainLightning.Chance = 15.2
Skill_ChainLightning.ChancePerLevel = 0.2

function Skill_ChainLightning.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_ChainLightning.GetDamagePower(self, Source, Level)
	return Source.LightningPower * Source.SpellDamage * 0.01
end

function Skill_ChainLightning.GetDuration(self, Level)
	return math.floor(10 * (self.Duration + self.DurationPerLevel * (Level - 1))) / 10.0
end

function Skill_ChainLightning.GetChance(self, Level)
	return math.floor(self.Chance + self.ChancePerLevel * (Level - 1))
end

function Skill_ChainLightning.GetInfo(self, Source, Item)
	return "Summon a powerful bolt of energy, hitting [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] enemies for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] damage with a [c green]" .. self:GetChance(Item.Level) .. "%[c white] chance to stun for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. "[c white] MP"
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

function Skill_ChainLightning.PlaySound(self, Level)
	Audio.Play("shock0.ogg", 0.35)
end

-- Rupture --

Skill_Rupture = Base_Spell:New()
Skill_Rupture.DamageBase = 100
Skill_Rupture.DamagePerLevel = 30
Skill_Rupture.DamageScale = 2.5
Skill_Rupture.Level = 20
Skill_Rupture.LevelPerLevel = 10
Skill_Rupture.CostPerLevel = 30
Skill_Rupture.ManaCostBase = 190 - Skill_Rupture.CostPerLevel
Skill_Rupture.BaseTargets = 3
Skill_Rupture.TargetsPerLevel = 0.1
Skill_Rupture.Duration = 10
Skill_Rupture.DurationPerLevel = 0

function Skill_Rupture.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_Rupture.GetPoisonDamage(self, Source, Level)
	return math.floor(self:GetLevel(Source, Level) * self:GetDuration(Level))
end

function Skill_Rupture.GetDamagePower(self, Source, Level)
	return Source.PoisonPower * Source.SpellDamage * 0.01
end

function Skill_Rupture.GetLevel(self, Source, Level)
	return math.floor((self.Level + self.LevelPerLevel * Level) * self:GetDamagePower(Source, Level))
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
	return "Explode a corpse, dealing [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] damage and releasing noxious gas, covering [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] enemies that deals [c green]" .. self:GetPoisonDamage(Source, Item.Level) .. "[c white] poison damage over [c green]" .. math.floor(self:GetDuration(Item.Level)) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
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

function Skill_Rupture.PlaySound(self, Level)
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
	return Source.FirePower * Source.SpellDamage * 0.01
end

function Skill_Ignite.GetInfo(self, Source, Item)
	return "Ignite an enemy and deal [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] fire damage over [c green]" .. math.floor(self:GetDuration(Item.Level)) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Ignite.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = self:GetBurnLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Ignite.PlaySound(self, Level)
	Audio.Play("flame0.ogg")
end

-- Demonic Conjuring --

Skill_DemonicConjuring = Base_SummonSpell:New()
Skill_DemonicConjuring.CostPerLevel = 10
Skill_DemonicConjuring.ManaCostBase = 25 - Skill_DemonicConjuring.CostPerLevel
Skill_DemonicConjuring.BaseHealth = 100
Skill_DemonicConjuring.BaseMinDamage = 10
Skill_DemonicConjuring.BaseMaxDamage = 20
Skill_DemonicConjuring.BaseArmor = 1.1
Skill_DemonicConjuring.HealthPerLevel = 40
Skill_DemonicConjuring.DamagePerLevel = 10
Skill_DemonicConjuring.ArmorPerLevel = 0.1
Skill_DemonicConjuring.Limit = 1
Skill_DemonicConjuring.LimitPerLevel = 0.05
Skill_DemonicConjuring.Duration = 15
Skill_DemonicConjuring.DurationPerLevel = 5
Skill_DemonicConjuring.DamageScale = 0.30
Skill_DemonicConjuring.SpecialChance = 35
Skill_DemonicConjuring.SpecialChancePerLevel = 0
Skill_DemonicConjuring.Monster = Monsters[23]
Skill_DemonicConjuring.SpecialMonster = Monsters[39]

function Skill_DemonicConjuring.GetInfo(self, Source, Item)
	MinDamage, MaxDamage = self:GetDamage(Source, Item.Level)

	return "Summon a demon that has [c green]" .. self:GetHealth(Source, Item.Level) .. "[c white] HP, [c green]" .. self:GetArmor(Source, Item.Level) .. "[c white] armor and does [c green]" .. MinDamage .. "-" .. MaxDamage .. "[c white] damage\n[c green]" .. self:GetSpecialChance(Item.Level) .. "%[c white] chance to summon an ice imp\nCan summon a maximum of [c green]" .. self:GetLimit(Source, Item.Level) .. "[c white]\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_DemonicConjuring.Use(self, Level, Duration, Source, Target, Result)
	Result.Summon = {}
	Result.Summon.SpellID = self.Item.ID
	Result.Summon.ID = self.Monster.ID
	Result.Summon.Health = self:GetHealth(Source, Level)
	Result.Summon.MinDamage, Result.Summon.MaxDamage = self:GetDamage(Source, Level)
	Result.Summon.Armor = self:GetArmor(Source, Level)
	Result.Summon.SummonBuff = Buff_SummonDemon.Pointer
	Result.Summon.Duration = self:GetDuration(Source, Level)

	-- Limit monster summons to 1
	if Source.MonsterID == 0 then
		Result.Summon.Limit = self:GetLimit(Source, Level)
	else
		Result.Summon.Limit = 1
	end

	Roll = Random.GetInt(1, 100)
	if Result.SummonBuff ~= nil then
		Result.Summon.SummonBuff = Result.SummonBuff
		if Result.SummonBuff == Buff_SummonIceImp.Pointer then
			Roll = 1
		else
			Roll = 1000
		end
	end

	if Roll <= self:GetSpecialChance(Level) then
		Result.Summon.ID = self.SpecialMonster.ID
		Result.Summon.SummonBuff = Buff_SummonIceImp.Pointer
	end

	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_DemonicConjuring.PlaySound(self, Level)
	Audio.Play("summon0.ogg")
end

-- Raise Dead --

Skill_RaiseDead = Base_SummonSpell:New()
Skill_RaiseDead.CostPerLevel = 10
Skill_RaiseDead.ManaCostBase = 15 - Skill_RaiseDead.CostPerLevel
Skill_RaiseDead.BaseHealth = 50
Skill_RaiseDead.BaseMana = 30
Skill_RaiseDead.BaseMinDamage = 10
Skill_RaiseDead.BaseMaxDamage = 20
Skill_RaiseDead.BaseArmor = 1
Skill_RaiseDead.HealthPerLevel = 30
Skill_RaiseDead.ManaPerLevel = 20
Skill_RaiseDead.DamagePerLevel = 8
Skill_RaiseDead.ArmorPerLevel = 0.05
Skill_RaiseDead.SpecialChance = 35
Skill_RaiseDead.SpecialChancePerLevel = 0
Skill_RaiseDead.Limit = 2
Skill_RaiseDead.LimitPerLevel = 0.1
Skill_RaiseDead.SkillLevel = 1
Skill_RaiseDead.SkillLevelPerLevel = 0.5
Skill_RaiseDead.Duration = 30
Skill_RaiseDead.DurationPerLevel = 5
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
	MinDamage, MaxDamage = self:GetDamage(Source, Item.Level)

	return "Raise a skeleton from a corpse with [c green]" .. self:GetHealth(Source, Item.Level) .. "[c white] HP, [c green]" .. self:GetArmor(Source, Item.Level) .. "[c white] armor and [c green]" .. MinDamage .. "-" .. MaxDamage .. "[c white] damage\n[c green]" .. self:GetSpecialChance(Item.Level) .. "%[c white] chance to summon a skeleton priest\nCan summon a maximum of [c green]" .. self:GetLimit(Source, Item.Level) .. "[c white]\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_RaiseDead.Use(self, Level, Duration, Source, Target, Result)
	Result.Summon = {}
	Result.Summon.SpellID = self.Item.ID
	Result.Summon.ID = self.Monster.ID
	Result.Summon.Health = self:GetHealth(Source, Level)
	Result.Summon.MinDamage, Result.Summon.MaxDamage = self:GetDamage(Source, Level)
	Result.Summon.Armor = self:GetArmor(Source, Level)
	Result.Summon.Limit = self:GetLimit(Source, Level)
	Result.Summon.SkillLevel = self:GetSkillLevel(Source, Level)
	Result.Summon.Duration = self:GetDuration(Source, Level)
	Result.Summon.SummonBuff = Buff_SummonSkeleton.Pointer

	Roll = Random.GetInt(1, 100)
	if Result.SummonBuff ~= nil then
		Result.Summon.SummonBuff = Result.SummonBuff
		if Result.SummonBuff == Buff_SummonSkeletonPriest.Pointer then
			Roll = 1
		else
			Roll = 1000
		end
	end

	if Roll <= self:GetSpecialChance(Level) then
		Result.Summon.ID = self.SpecialMonster.ID
		Result.Summon.Mana = self:GetMana(Source, Level)
		Result.Summon.SummonBuff = Buff_SummonSkeletonPriest.Pointer
		Result.Summon.Duration = Result.Summon.Duration * 2
		Result.Summon.MinDamage = math.ceil(Result.Summon.MinDamage * self.SpecialDamage)
		Result.Summon.MaxDamage = math.ceil(Result.Summon.MaxDamage * self.SpecialDamage)
	end

	Result.Target.Corpse = -1
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_RaiseDead.PlaySound(self, Level)
	Audio.Play("summon0.ogg")
end

-- Enfeeble --

Skill_Enfeeble = Base_Spell:New()
Skill_Enfeeble.Constant = 15
Skill_Enfeeble.BasePercent = 22
Skill_Enfeeble.Multiplier = 50
Skill_Enfeeble.DurationPerLevel = 0.1
Skill_Enfeeble.Duration = 5
Skill_Enfeeble.CostPerLevel = 10
Skill_Enfeeble.ManaCostBase = 10 - Skill_Enfeeble.CostPerLevel
Skill_Enfeeble.BaseTargets = 1
Skill_Enfeeble.TargetsPerLevel = 0.05

function Skill_Enfeeble.GetPercent(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Enfeeble.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_Enfeeble.GetInfo(self, Source, Item)
	Count = self:GetTargetCount(Item.Level)
	Plural = ""
	if Count ~= 1 then
		Plural = "s"
	end

	return "Cripple [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] foe" .. Plural .. ", reducing their attack damage by [c green]" .. self:GetPercent(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Enfeeble.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Weak.Pointer
	Result.Target.BuffLevel = self:GetPercent(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Enfeeble.PlaySound(self, Level)
	Audio.Play("enfeeble0.ogg")
end

-- Flay --

Skill_Flay = Base_Spell:New()
Skill_Flay.Duration = 5
Skill_Flay.DurationPerLevel = 0.1
Skill_Flay.CostPerLevel = 10
Skill_Flay.ManaCostBase = 20 - Skill_Flay.CostPerLevel
Skill_Flay.BaseTargets = 1
Skill_Flay.TargetsPerLevel = 0.1
Skill_Flay.Constant = 15
Skill_Flay.BasePercent = 14
Skill_Flay.Multiplier = 100

function Skill_Flay.GetPercent(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Flay.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_Flay.GetInfo(self, Source, Item)
	Count = self:GetTargetCount(Item.Level)
	Plural = ""
	if Count ~= 1 then
		Plural = "s"
	end

	return "Strip the skin of [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] foe" .. Plural .. ", reducing their resistances by [c green]" .. self:GetPercent(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Attracts summons"
end

function Skill_Flay.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Flayed.Pointer
	Result.Target.BuffLevel = self:GetPercent(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Flay.PlaySound(self, Level)
	Audio.Play("swamp0.ogg")
end

-- Fracture --

Skill_Fracture = Base_Spell:New()
Skill_Fracture.Duration = 5
Skill_Fracture.DurationPerLevel = 0.1
Skill_Fracture.CostPerLevel = 5
Skill_Fracture.ManaCostBase = 40 - Skill_Fracture.CostPerLevel
Skill_Fracture.Armor = 5
Skill_Fracture.ArmorPerLevel = 1

function Skill_Fracture.GetReduction(self, Level)
	return math.floor(self.Armor + self.ArmorPerLevel * (Level - 1))
end

function Skill_Fracture.GetInfo(self, Source, Item)
	return "Decimate target's defenses, reducing their armor by [c green]" .. self:GetReduction(Item.Level) .. "[c white] for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP\n\n[c yellow]Attracts summons"
end

function Skill_Fracture.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Fractured.Pointer
	Result.Target.BuffLevel = self:GetReduction(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Fracture.PlaySound(self, Level)
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

function Skill_Light.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Light.Pointer
	Result.Target.BuffLevel = 20 + Level
	if Result.Target.BuffLevel > 30 then
		Result.Target.BuffLevel = 30
	end
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

-- Portal --

Skill_Portal = Base_Spell:New()
Skill_Portal.Duration = 3
Skill_Portal.CostPerLevel = -10
Skill_Portal.ManaCostBase = 200 - Skill_Portal.CostPerLevel
Skill_Portal.CostScale = 0

function Skill_Portal.GetInfo(self, Source, Item)
	return "Teleport home after [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Portal.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Teleport = self:GetDuration(Level)
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
	return math.floor((self.BaseBlock + self.BlockPerLevel * (Level - 1)) * Source.ManaPower)
end

function Skill_MagicBarrier.GetInfo(self, Source, Item)
	return "Create a magic shield around an ally that absorbs [c green]" .. self:GetLevel(Source, Item.Level) .. "[c white] attack damage\nLasts [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_MagicBarrier.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Shielded.Pointer
	Result.Target.BuffLevel = self:GetLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_MagicBarrier.PlaySound(self, Level)
	Audio.Play("barrier0.ogg")
end

-- Sanctuary --

Skill_Sanctuary = Base_Spell:New()
Skill_Sanctuary.Level = 10
Skill_Sanctuary.LevelPerLevel = 1
Skill_Sanctuary.CostPerLevel = 20
Skill_Sanctuary.ManaCostBase = 100 - Skill_Sanctuary.CostPerLevel
Skill_Sanctuary.BaseTargets = 3
Skill_Sanctuary.TargetsPerLevel = 0.1
Skill_Sanctuary.Duration = 10
Skill_Sanctuary.DurationPerLevel = 0

function Skill_Sanctuary.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

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
	return math.floor((self.Level + self.LevelPerLevel * (Level - 1)) * Source.HealPower)
end

function Skill_Sanctuary.GetInfo(self, Source, Item)
	return "Imbue [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] allies with sanctuary for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds, granting [c green]" .. self:GetArmor(Source, Item.Level) .. "[c white] armor, [c green]" .. self:GetDamageBlock(Source, Item.Level) .. "[c white] damage block and [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetManaCost(Item.Level) .. " [c white]MP"
end

function Skill_Sanctuary.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Sanctuary.Pointer
	Result.Target.BuffLevel = self:GetLevel(Source, Level)
	Result.Target.BuffDuration = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Sanctuary.PlaySound(self, Level)
	Audio.Play("sanctuary0.ogg")
end