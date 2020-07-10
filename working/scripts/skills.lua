-- MONSTER SKILLS --

-- Monster attack --

Skill_MonsterAttack = Base_Attack:New()

function Skill_MonsterAttack.PlaySound(self, Level)
	Audio.Play("bash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crow attack --

Skill_CrowAttack = Base_Attack:New()

function Skill_CrowAttack.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buff_Blinded.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
		return true
	end

	return false
end

function Skill_CrowAttack.PlaySound(self, Level)
	Audio.Play("swoop" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Slime attack --

Skill_SlimeAttack = Base_Attack:New()

function Skill_SlimeAttack.PlaySound(self, Level)
	Audio.Play("slime" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Ant attack --

Skill_AntAttack = Base_Attack:New()

function Skill_AntAttack.PlaySound(self, Level)
	Audio.Play("crunch" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crab attack --

Skill_CrabAttack = Base_Attack:New()

function Skill_CrabAttack.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buff_Bleeding.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 3
		return true
	end

	return false
end

function Skill_CrabAttack.PlaySound(self, Level)
	Audio.Play("crab" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Spider bite --

Skill_SpiderBite = Base_Attack:New()

function Skill_SpiderBite.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buff_Slowed.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
		return true
	end

	return false
end

function Skill_SpiderBite.PlaySound(self, Level)
	Audio.Play("spider0.ogg")
end

-- Fang bite --

Skill_FangBite = Base_Attack:New()

function Skill_FangBite.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buff_Bleeding.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
		return true
	end

	return false
end

function Skill_FangBite.PlaySound(self, Level)
	Audio.Play("bat0.ogg")
end

-- Venom bite --

Skill_VenomBite = Base_Attack:New()

function Skill_VenomBite.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buff_Poisoned.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 10
		return true
	end

	return false
end

function Skill_VenomBite.PlaySound(self, Level)
	Audio.Play("bat0.ogg")
end

-- Sting --

Skill_Sting = Base_Attack:New()

function Skill_Sting.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = 1
		return true
	elseif Roll <= 30 then
		Result.Target.Buff = Buff_Poisoned.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
		return true
	end

	return false
end

function Skill_Sting.PlaySound(self, Level)
	Audio.Play("sting" .. Random.GetInt(0, 2) .. ".ogg", 0.65)
end

-- Ghost attack --

Skill_GhostAttack = Base_Attack:New()

function Skill_GhostAttack.Use(self, Level, Duration, Source, Target, Result)

	-- Ignore target's defense
	Target.DamageBlock = 0

	Hit = Battle_ResolveDamage(self, Level, Source, Target, Result)

	return Result
end

function Skill_GhostAttack.PlaySound(self, Level)
	Audio.Play("ghost" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Swoop attack --

Skill_Swoop = Base_Attack:New()

function Skill_Swoop.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 75 then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = 3
		return true
	end

	return false
end

function Skill_Swoop.PlaySound(self, Level)
	Audio.Play("multiswoop0.ogg")
end

-- Pincer attack --

Skill_PincerAttack = Base_Attack:New()

function Skill_PincerAttack.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 50 then
		Result.Target.Buff = Buff_Bleeding.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
		return true
	end

	return false
end

function Skill_PincerAttack.PlaySound(self, Level)
	Audio.Play("multislash0.ogg")
end

-- Chew attack --

Skill_ChewAttack = Base_Attack:New()

function Skill_ChewAttack.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 75 then
		Result.Source.Health = -Result.Target.Health
		return true
	end

	return false
end

function Skill_ChewAttack.PlaySound(self, Level)
	Audio.Play("grunt" .. Random.GetInt(0, 2) .. ".ogg")
end

-- Swamp attack --

Skill_SwampAttack = Base_Attack:New()

function Skill_SwampAttack.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 30 then
		Result.Target.Buff = Buff_Slowed.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
		return true
	end

	return false
end

function Skill_SwampAttack.PlaySound(self, Level)
	Audio.Play("sludge0.ogg")
end

-- Skeleton attack --

Skill_SkeletonAttack = Base_Attack:New()

function Skill_SkeletonAttack.PlaySound(self, Level)
	Audio.Play("bones" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Demon attack --

Skill_DemonAttack = Base_Attack:New()

function Skill_DemonAttack.PlaySound(self, Level)
	Audio.Play("demon" .. Random.GetInt(0, 1) .. ".ogg")
end

-- PLAYER SKILLS --

-- Attack --

Skill_Attack = Base_Attack:New()
Skill_Attack.BaseChance = 4
Skill_Attack.ChancePerLevel = 1
Skill_Attack.DamageBase = 100
Skill_Attack.DamagePerLevel = 4

function Skill_Attack.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_Attack.GetChance(self, Level)
	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_Attack.GetInfo(self, Source, Item)
	return "Attack for [c green]" .. self:GetDamage(Item.Level) .. "% [c white]weapon damage\n[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to deal [c green]200% [c white]extra damage"
end

function Skill_Attack.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

function Skill_Attack.GenerateDamage(self, Level, Source)
	Damage = math.floor(Source.GenerateDamage() * (self:GetDamage(Level) / 100))

	Crit = false
	if Random.GetInt(1, 100) <= self:GetChance(Level) then
		Damage = Damage * 3
		Crit = true
	end

	return Damage, Crit
end

-- Fury --

Skill_Fury = Base_Attack:New()
Skill_Fury.StaminaPerLevel = 0.04
Skill_Fury.BaseStamina = 0.25
Skill_Fury.SpeedDuration = 3
Skill_Fury.SpeedDurationPerLevel = 0.2

function Skill_Fury.GetStaminaGain(self, Level)
	return math.min(self.BaseStamina + self.StaminaPerLevel * Level, 1.0)
end

function Skill_Fury.GetDuration(self, Level)
	return self.SpeedDuration + self.SpeedDurationPerLevel * (Level - 1)
end

function Skill_Fury.GetInfo(self, Source, Item)
	return "Strike down your enemy, gaining [c green]" .. math.floor(self:GetStaminaGain(Item.Level) * 100) .. "% [c yellow]stamina[c white] and a [c green]" .. self:GetDuration(Item.Level) .. "[c white] second battle speed boost for a killing blow"
end

function Skill_Fury.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

function Skill_Fury.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Target.Health + Result.Target.Health <= 0 then
		Result.Source.Stamina = self:GetStaminaGain(Level)

		Result.Source.Buff = Buff_Hasted.Pointer
		Result.Source.BuffLevel = 30
		Result.Source.BuffDuration = self:GetDuration(Level)
		return true
	end

	return false
end

-- Gash --

Skill_Gash = Base_Attack:New()
Skill_Gash.BaseChance = 34
Skill_Gash.ChancePerLevel = 1
Skill_Gash.Duration = 5
Skill_Gash.IncreasePerLevel = 6
Skill_Gash.BleedingLevel = 10

function Skill_Gash.CanUse(self, Level, Source, Target)
	OffHandCount = 0
	WeaponMain = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if WeaponMain ~= nil and WeaponMain.Type == ITEM_OFFHAND then
		OffHandCount = OffHandCount + 1
	end

	WeaponOff = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if WeaponOff ~= nil and WeaponOff.Type == ITEM_OFFHAND then
		OffHandCount = OffHandCount + 1
	end

	return OffHandCount > 0
end

function Skill_Gash.GetChance(self, Level)
	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_Gash.GetBleedLevel(self, Source, Level)
	return math.floor((self.BleedingLevel + self.IncreasePerLevel * (Level - 1)) * Source.BleedPower)
end

function Skill_Gash.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	return "Slice your enemy\n[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to cause level [c green]" .. self:GetBleedLevel(Source, Item.Level) .. " [c yellow]bleeding\n[c " .. TextColor .. "]Requires at least one off-hand weapon"
end

function Skill_Gash.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buff_Bleeding.Pointer
		Result.Target.BuffLevel = self:GetBleedLevel(Source, Level)
		Result.Target.BuffDuration = self.Duration
		return true
	end

	return false
end

function Skill_Gash.PlaySound(self, Level)
	Audio.Play("gash0.ogg")
end

-- Shield Bash --

Skill_ShieldBash = Base_Attack:New()
Skill_ShieldBash.MaxPercent = 75
Skill_ShieldBash.BaseChance = 23
Skill_ShieldBash.ChancePerLevel = 4
Skill_ShieldBash.Duration = 2.1
Skill_ShieldBash.DurationPerLevel = 0.1
Skill_ShieldBash.DamageBase = 100
Skill_ShieldBash.DamagePerLevel = 10

function Skill_ShieldBash.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_ShieldBash.GetDuration(self, Level)
	return math.floor(self.Duration + self.DurationPerLevel * (Level - 1))
end

function Skill_ShieldBash.GetChance(self, Level)
	return math.min(self.BaseChance + self.ChancePerLevel * Level, self.MaxPercent)
end

function Skill_ShieldBash.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	return "Bash your enemy with a shield for [c green]" .. self:GetDamage(Item.Level) .. "% [c white]shield damage with a [c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to [c yellow]stun [c white]for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\n[c " .. TextColor .. "]Requires a shield"
end

function Skill_ShieldBash.GenerateDamage(self, Level, Source)
	Shield = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if Shield == nil then
		return 0
	end

	return math.floor(Shield.GenerateDamage(Source.Pointer, Shield.Upgrades) * (self:GetDamage(Level) / 100))
end

function Skill_ShieldBash.CanUse(self, Level, Source, Target)
	Shield = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if Shield == nil then
		return false
	end

	return Shield.Type == ITEM_SHIELD
end

function Skill_ShieldBash.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = self:GetDuration(Level)
		return true
	end

	return false
end

function Skill_ShieldBash.PlaySound(self, Level)
	Audio.Play("bash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Whirlwind --

Skill_Whirlwind = Base_Attack:New()
Skill_Whirlwind.DamageBase = 20
Skill_Whirlwind.DamagePerLevel = 3
Skill_Whirlwind.SlowDurationPerLevel = 1.0 / 4.0
Skill_Whirlwind.SlowDuration = 3 - Skill_Whirlwind.SlowDurationPerLevel

function Skill_Whirlwind.CanUse(self, Level, Source, Target)
	Weapon = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if Weapon == nil then
		return false
	end

	return Weapon.Type == ITEM_TWOHANDED_WEAPON
end

function Skill_Whirlwind.GetDuration(self, Level)
	return math.floor(Skill_Whirlwind.SlowDuration + Skill_Whirlwind.SlowDurationPerLevel * Level)
end

function Skill_Whirlwind.GetDamage(self, Level)
	return Skill_Whirlwind.DamageBase + Skill_Whirlwind.DamagePerLevel * Level
end

function Skill_Whirlwind.GenerateDamage(self, Level, Source)
	return math.floor(Source.GenerateDamage() * (self:GetDamage(Level) / 100))
end

function Skill_Whirlwind.ApplyCost(self, Source, Level, Result)
	Result.Source.Buff = Buff_Slowed.Pointer
	Result.Source.BuffLevel = 30
	Result.Source.BuffDuration = self:GetDuration(Level)

	return Result
end

function Skill_Whirlwind.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	return "Slash all enemies with [c green]" .. self:GetDamage(Item.Level) .. "% [c white]weapon damage\nCauses [c yellow]fatigue [c white]for [c green]" .. self:GetDuration(Item.Level) .." [c white]seconds\n[c " .. TextColor .. "]Requires a two-handed weapon"
end

function Skill_Whirlwind.PlaySound(self, Level)
	Audio.Play("multislash0.ogg")
end

-- Rejuvenation --

Skill_Rejuvenation = Base_Spell:New()
Skill_Rejuvenation.Duration = 5
Skill_Rejuvenation.CostPerLevel = 5
Skill_Rejuvenation.ManaCostBase = 5 - Skill_Rejuvenation.CostPerLevel

function Skill_Rejuvenation.GetHeal(self, Source, Level)
	return Buff_Healing.Heal * self:GetLevel(Source, Level) * self:GetDuration(Level)
end

function Skill_Rejuvenation.GetLevel(self, Source, Level)
	return math.floor(10 * Level * Source.HealPower)
end

function Skill_Rejuvenation.GetInfo(self, Source, Item)
	return "Heal [c green]" .. self:GetHeal(Source, Item.Level) .. " [c white]HP [c white]over [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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
	return "Heal target for [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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
Skill_Resurrect.HealBase = -50
Skill_Resurrect.HealPerLevel = 60
Skill_Resurrect.CostPerLevel = 20
Skill_Resurrect.ManaCostBase = 200 - Skill_Resurrect.CostPerLevel

function Skill_Resurrect.GetHeal(self, Source, Level)
	return math.floor((self.HealBase + self.HealPerLevel * Level) * Source.HealPower + 0.001)
end

function Skill_Resurrect.GetInfo(self, Source, Item)
	return "Resurrect an ally and give them [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Resurrect.ApplyCost(self, Source, Level, Result)
	Result.Source.Mana = -self:GetCost(Level)

	-- Hack for using in world
	Result.Source.Resurrect = self:GetHeal(Source, Level)

	return Result
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
Skill_Spark.CostPerLevel = 4
Skill_Spark.ManaCostBase = 10 - Skill_Spark.CostPerLevel

function Skill_Spark.GetDamagePower(self, Source, Level)
	return Source.LightningPower
end

function Skill_Spark.GetInfo(self, Source, Item)
	return "Shock a target for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] lightning damage\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Spark.PlaySound(self, Level)
	Audio.Play("shock0.ogg", 0.35)
end

-- Icicle --

Skill_Icicle = Base_Spell:New()
Skill_Icicle.DamageBase = 25
Skill_Icicle.DamagePerLevel = 25
Skill_Icicle.CostPerLevel = 4
Skill_Icicle.Duration = 5
Skill_Icicle.DurationPerLevel = 0.5
Skill_Icicle.ManaCostBase = 15 - Skill_Icicle.CostPerLevel

function Skill_Icicle.GetDamagePower(self, Source, Level)
	return Source.ColdPower
end

function Skill_Icicle.GetInfo(self, Source, Item)
	return "Pierce a target for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] cold damage\nSlows for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Icicle.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = 20
	Result.Target.BuffDuration = self:GetDuration(Level)

	return true
end

function Skill_Icicle.PlaySound(self, Level)
	Audio.Play("ice" .. Random.GetInt(0, 1) .. ".ogg", 0.65)
end

-- Poison Touch --

Skill_PoisonTouch = Base_Spell:New()
Skill_PoisonTouch.PoisonLevelPerLevel = 6
Skill_PoisonTouch.PoisonLevel = 10 - Skill_PoisonTouch.PoisonLevelPerLevel
Skill_PoisonTouch.CostPerLevel = 5
Skill_PoisonTouch.ManaCostBase = 20 - Skill_PoisonTouch.CostPerLevel
Skill_PoisonTouch.DurationPerLevel = 0
Skill_PoisonTouch.Duration = 10

function Skill_PoisonTouch.GetPoisonLevel(self, Source, Level)
	return math.floor((self.PoisonLevel + self.PoisonLevelPerLevel * Level) * self:GetDamagePower(Source, Level))
end

function Skill_PoisonTouch.GetDamage(self, Source, Level)
	return self:GetPoisonLevel(Source, Level) * self:GetDuration(Level)
end

function Skill_PoisonTouch.GetDamagePower(self, Source, Level)
	return Source.PoisonPower
end

function Skill_PoisonTouch.GetInfo(self, Source, Item)
	return "Infuse venom into your enemy, dealing [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] poison damage over [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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
Skill_FireBlast.DamageBase = 50
Skill_FireBlast.DamagePerLevel = 30
Skill_FireBlast.CostPerLevel = 10
Skill_FireBlast.ManaCostBase = 60 - Skill_FireBlast.CostPerLevel
Skill_FireBlast.BaseTargets = 3
Skill_FireBlast.TargetsPerLevel = 0.2

function Skill_FireBlast.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_FireBlast.GetDamagePower(self, Source, Level)
	return Source.FirePower
end

function Skill_FireBlast.GetInfo(self, Source, Item)
	return "Blast [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] foes for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] fire damage\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_FireBlast.PlaySound(self, Level)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Ignite --

Skill_Ignite = Base_Spell:New()
Skill_Ignite.BurnLevel = 30
Skill_Ignite.BurnLevelPerLevel = 9
Skill_Ignite.CostPerLevel = 10
Skill_Ignite.Duration = 6
Skill_Ignite.ManaCostBase = 30 - Skill_Ignite.CostPerLevel

function Skill_Ignite.GetBurnLevel(self, Source, Level)
	return math.floor((self.BurnLevel + self.BurnLevelPerLevel * Level) * self:GetDamagePower(Source, Level))
end

function Skill_Ignite.GetDamage(self, Source, Level)
	return self:GetBurnLevel(Source, Level) * self:GetDuration(Level)
end

function Skill_Ignite.GetDamagePower(self, Source, Level)
	return Source.FirePower
end

function Skill_Ignite.GetInfo(self, Source, Item)
	return "Ignite an enemy and deal [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] fire damage over [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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

-- Toughness --

Skill_Toughness = {}
Skill_Toughness.HealthPerLevel = 60
Skill_Toughness.Armor = 1

function Skill_Toughness.GetArmor(self, Level)

	return self.Armor * math.floor(Level / 1)
end

function Skill_Toughness.GetInfo(self, Source, Item)
	BonusText = ""
	if self:GetArmor(Item.Level) > 0 then
		BonusText = "\n[c white]Increase armor by [c green]" .. self:GetArmor(Item.Level)
	end

	return "Increase max HP by [c green]" .. Skill_Toughness.HealthPerLevel * Item.Level .. BonusText
end

function Skill_Toughness.Stats(self, Level, Object, Change)
	Change.MaxHealth = self.HealthPerLevel * Level
	Change.Armor = self:GetArmor(Level)

	return Change
end

-- Arcane Mastery --

Skill_ArcaneMastery = {}
Skill_ArcaneMastery.PerLevel = 40
Skill_ArcaneMastery.ManaRegen = 1
Skill_ArcaneMastery.Power = 25
Skill_ArcaneMastery.PowerPerLevel = 5

function Skill_ArcaneMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_ArcaneMastery.GetInfo(self, Source, Item)
	return "Increase max MP by [c light_blue]" .. Skill_ArcaneMastery.PerLevel * Item.Level .. "\n[c white]Increase mana power by [c green]" .. self:GetPower(Item.Level) .. "%"
end

function Skill_ArcaneMastery.Stats(self, Level, Object, Change)
	Change.MaxMana = self.PerLevel * Level
	Change.ManaPower = self:GetPower(Level) / 100.0

	return Change
end

-- Evasion --

Skill_Evasion = {}
Skill_Evasion.ChancePerLevel = 1
Skill_Evasion.BaseChance = 10
Skill_Evasion.BattleSpeed = 1

function Skill_Evasion.GetChance(self, Level)

	return math.min(math.floor(self.BaseChance + self.ChancePerLevel * Level), 100)
end

function Skill_Evasion.GetBattleSpeed(self, Level)

	return math.floor(self.BattleSpeed * math.floor(Level / 1))
end

function Skill_Evasion.GetInfo(self, Source, Item)
	BonusText = ""
	if self:GetBattleSpeed(Item.Level) > 0 then
		BonusText = "\n[c white]Increase battle speed by [c green]" .. self:GetBattleSpeed(Item.Level) .. "%"
	end

	return "Increase evasion by [c green]" .. self:GetChance(Item.Level) .. "%" .. BonusText
end

function Skill_Evasion.Stats(self, Level, Object, Change)
	Change.Evasion = self:GetChance(Level)
	Change.BattleSpeed = self:GetBattleSpeed(Level)

	return Change
end

-- Energy Field --

Skill_EnergyField = {}
Skill_EnergyField.BasePercent = 20
Skill_EnergyField.PercentPerLevel = 5

function Skill_EnergyField.GetReduction(self, Level)
	return math.min(Skill_EnergyField.BasePercent + self.PercentPerLevel * math.floor(Level / 1), 95)
end

function Skill_EnergyField.GetInfo(self, Source, Item)
	return "Convert [c green]" .. self:GetReduction(Item.Level) .. "%[c white] of damage taken to mana drain"
end

function Skill_EnergyField.Stats(self, Level, Object, Change)
	Change.ManaReductionRatio = self:GetReduction(Level) / 100.0

	return Change
end

-- Physical Mastery --

Skill_PhysicalMastery = {}
Skill_PhysicalMastery.Power = 25
Skill_PhysicalMastery.PowerPerLevel = 5

function Skill_PhysicalMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_PhysicalMastery.GetInfo(self, Source, Item)
	return "Increase physical power by [c green]" .. self:GetPower(Item.Level) .. "%[c white]"
end

function Skill_PhysicalMastery.Stats(self, Level, Object, Change)
	Change.PhysicalPower = self:GetPower(Level) / 100.0

	return Change
end

-- Fire Mastery --

Skill_FireMastery = {}
Skill_FireMastery.Power = 25
Skill_FireMastery.PowerPerLevel = 5

function Skill_FireMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_FireMastery.GetInfo(self, Source, Item)
	return "Increase fire power by [c green]" .. self:GetPower(Item.Level) .. "%[c white]"
end

function Skill_FireMastery.Stats(self, Level, Object, Change)
	Change.FirePower = self:GetPower(Level) / 100.0

	return Change
end

-- Cold Mastery --

Skill_ColdMastery = {}
Skill_ColdMastery.Power = 25
Skill_ColdMastery.PowerPerLevel = 5

function Skill_ColdMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_ColdMastery.GetInfo(self, Source, Item)
	return "Increase cold power by [c green]" .. self:GetPower(Item.Level) .. "%[c white]"
end

function Skill_ColdMastery.Stats(self, Level, Object, Change)
	Change.ColdPower = self:GetPower(Level) / 100.0

	return Change
end

-- Lightning Mastery --

Skill_LightningMastery = {}
Skill_LightningMastery.Power = 25
Skill_LightningMastery.PowerPerLevel = 5

function Skill_LightningMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_LightningMastery.GetInfo(self, Source, Item)
	return "Increase lightning power by [c green]" .. self:GetPower(Item.Level) .. "%[c white]"
end

function Skill_LightningMastery.Stats(self, Level, Object, Change)
	Change.LightningPower = self:GetPower(Level) / 100.0

	return Change
end

-- Bleed Mastery --

Skill_BleedMastery = {}
Skill_BleedMastery.Power = 25
Skill_BleedMastery.PowerPerLevel = 5

function Skill_BleedMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_BleedMastery.GetInfo(self, Source, Item)
	return "Increase bleed power by [c green]" .. self:GetPower(Item.Level) .. "%[c white]"
end

function Skill_BleedMastery.Stats(self, Level, Object, Change)
	Change.BleedPower = self:GetPower(Level) / 100.0

	return Change
end

-- Poison Mastery --

Skill_PoisonMastery = {}
Skill_PoisonMastery.Power = 25
Skill_PoisonMastery.PowerPerLevel = 5

function Skill_PoisonMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_PoisonMastery.GetInfo(self, Source, Item)
	return "Increase poison power by [c green]" .. self:GetPower(Item.Level) .. "%[c white]"
end

function Skill_PoisonMastery.Stats(self, Level, Object, Change)
	Change.PoisonPower = self:GetPower(Level) / 100.0

	return Change
end

-- Heal Mastery --

Skill_HealMastery = {}
Skill_HealMastery.Power = 25
Skill_HealMastery.PowerPerLevel = 5

function Skill_HealMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_HealMastery.GetInfo(self, Source, Item)
	return "Increase heal power by [c green]" .. self:GetPower(Item.Level) .. "%[c white]"
end

function Skill_HealMastery.Stats(self, Level, Object, Change)
	Change.HealPower = self:GetPower(Level) / 100.0

	return Change
end

-- Pet Mastery --

Skill_PetMastery = {}
Skill_PetMastery.Power = 50
Skill_PetMastery.PowerPerLevel = 5
Skill_PetMastery.SummonLimit = 1
Skill_PetMastery.SummonLimitPerLevel = 0.25

function Skill_PetMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_PetMastery.GetSummonLimit(self, Level)
	return math.floor(self.SummonLimit + self.SummonLimitPerLevel * (Level - 1))
end

function Skill_PetMastery.GetInfo(self, Source, Item)
	return "Increase pet stats by [c green]" .. self:GetPower(Item.Level) .. "%\nIncrease summon limit by [c green]" .. self:GetSummonLimit(Item.Level)
end

function Skill_PetMastery.Stats(self, Level, Object, Change)
	Change.PetPower = self:GetPower(Level) / 100.0
	Change.SummonLimit = self:GetSummonLimit(Level)

	return Change
end

-- Flee --

Skill_Flee = {}
Skill_Flee.BaseChance = 28
Skill_Flee.ChancePerLevel = 6
Skill_Flee.Duration = 5
Skill_Flee.DurationPerLevel = -0.2

function Skill_Flee.CanUse(self, Level, Source, Target)
	return not Source.BossBattle
end

function Skill_Flee.GetChance(self, Level)
	return math.min(self.BaseChance + self.ChancePerLevel * (Level - 1), 100)
end

function Skill_Flee.GetDuration(self, Level)
	return math.max(self.Duration + self.DurationPerLevel * (Level - 1), 1)
end

function Skill_Flee.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to run away from combat\nCauses [c yellow]fatigue [c white]for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\n[c yellow]Unusable in boss battles"
end

function Skill_Flee.ApplyCost(self, Source, Level, Result)
	Result.Source.Buff = Buff_Slowed.Pointer
	Result.Source.BuffLevel = 30
	Result.Source.BuffDuration = self:GetDuration(Level)

	return Result
end

function Skill_Flee.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Flee = true
		Result.Target.Buff = Buff_Slowed.Pointer
		Result.Target.BuffLevel = 35
		Result.Target.BuffDuration = self.Duration
		return true
	end

	return false
end

function Skill_Flee.Use(self, Level, Duration, Source, Target, Result)
	self:Proc(Random.GetInt(1, 100), Level, Duration, Source, Target, Result)

	return Result
end

function Skill_Flee.PlaySound(self, Level)
	Audio.Play("run0.ogg")
end

-- Pickpocket --

Skill_Pickpocket = {}
Skill_Pickpocket.BaseChance = 23
Skill_Pickpocket.ChancePerLevel = 4

function Skill_Pickpocket.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_Pickpocket.GetInfo(self, Source, Item)

	return "[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to steal [c green]50%][c white] gold from a monster or [c green]10%[c white] from a player"
end

function Skill_Pickpocket.Proc(self, Roll, Level, Duration, Source, Target, Result)

	if Roll <= self:GetChance(Level) then
		GoldAvailable = Target.Gold
		if Target.MonsterID > 0 then
			GoldAmount = math.ceil(GoldAvailable / 2)
		else
			GoldAmount = math.ceil(GoldAvailable / 10)
		end

		if GoldAmount <= Target.Gold then
			Result.Target.Gold = -GoldAmount
			Result.Source.GoldStolen = GoldAmount
		else
			Result.Target.Gold = 0
			Result.Source.GoldStolen = 0
		end

		return true
	else
		Result.Target.Miss = true
	end

	return false
end

function Skill_Pickpocket.Use(self, Level, Duration, Source, Target, Result)
	self:Proc(Random.GetInt(1, 100), Level, Duration, Source, Target, Result)

	return Result
end

-- Parry --

Skill_Parry = {}
Skill_Parry.StaminaGain = Buff_Parry.StaminaGain
Skill_Parry.Duration = 0.5
Skill_Parry.DurationPerLevel = 0.2
Skill_Parry.DamageReduction = 50
Skill_Parry.DamageReductionPerLevel = 2
Skill_Parry.MaxDamageReduction = 95

function Skill_Parry.GetDuration(self, Level)
	return self.Duration + self.DurationPerLevel * Level
end

function Skill_Parry.GetDamageReduction(self, Level)
	return math.min(self.DamageReduction + self.DamageReductionPerLevel * (Level - 1), self.MaxDamageReduction)
end

function Skill_Parry.GetInfo(self, Source, Item)
	return "Block [c green]" .. self:GetDamageReduction(Item.Level) .. "% [c white]attack damage for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nGain [c green]" .. math.floor(self.StaminaGain * 100) .. "% [c yellow]stamina [c white]for each attack blocked"
end

function Skill_Parry.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Parry.Pointer
	Result.Target.BuffLevel = self:GetDamageReduction(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

	return Result
end

-- Taunt --

Skill_Taunt = {}
Skill_Taunt.Armor = 10
Skill_Taunt.ArmorPerLevel = 2
Skill_Taunt.FatigueDuration = 5
Skill_Taunt.Duration = 3.4
Skill_Taunt.DurationPerLevel = 0.2
Skill_Taunt.BaseTargets = 1
Skill_Taunt.TargetsPerLevel = 0.2

function Skill_Taunt.GetDuration(self, Level)
	return self.Duration + self.DurationPerLevel * Level
end

function Skill_Taunt.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_Taunt.GetInfo(self, Source, Item)
	Count = self:GetTargetCount(Item.Level)
	Plural = "enemies"
	if Count == 1 then
		Plural = "enemy"
	end

	return "Taunt [c green]" .. Count .. "[c white] " .. Plural .. " for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds, forcing them to attack you\nCauses [c yellow]fatigue[c white] for [c green]" .. self.FatigueDuration .."[c white] seconds"
end

function Skill_Taunt.ApplyCost(self, Source, Level, Result)
	Result.Source.Buff = Buff_Slowed.Pointer
	Result.Source.BuffLevel = 30
	Result.Source.BuffDuration = self.FatigueDuration

	return Result
end

function Skill_Taunt.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Taunted.Pointer
	Result.Target.BuffLevel = 1
	Result.Target.BuffDuration = self:GetDuration(Level)

	return Result
end

function Skill_Taunt.PlaySound(self, Level)
	Audio.Play("taunt" .. Random.GetInt(0, 2) .. ".ogg")
end

-- Backstab --

Skill_Backstab = Base_Attack:New()
Skill_Backstab.BaseDamage = 0.5
Skill_Backstab.DamagePerLevel = 0.2
Skill_Backstab.DamageMultiplier = 1.6 - Skill_Backstab.DamagePerLevel

function Skill_Backstab.GetDamage(self, Level)
	return self.DamageMultiplier + self.DamagePerLevel * Level
end

function Skill_Backstab.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	return "Attack for [c green]" .. math.floor(self.BaseDamage * 100) .. "% [c white]weapon damage\nDeal [c green]" .. math.floor(self:GetDamage(Item.Level) * 100) .. "% [c white]damage to stunned enemies\n[c " .. TextColor .. "]Can only use off-hand weapons"
end

function Skill_Backstab.CanUse(self, Level, Source, Target)
	WeaponMain = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if WeaponMain == nil then
		WeaponOff = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
		if WeaponOff ~= nil and WeaponOff.Type == ITEM_OFFHAND then
			return true
		end

		return false
	end

	return WeaponMain.Type == ITEM_OFFHAND
end

function Skill_Backstab.Proc(self, Roll, Level, Duration, Source, Target, Result)
	for i = 1, #Target.StatusEffects do
		Effect = Target.StatusEffects[i]
		if Effect.Buff == Buff_Stunned then
			Result.Target.Health = math.floor(Result.Target.Health * self:GetDamage(Level))
			Result.Target.Crit = true
			return true
		end
	end

	Result.Target.Health = math.floor(Result.Target.Health * self.BaseDamage)
	return false
end

function Skill_Backstab.PlaySound(self, Level)
	Audio.Play("gash0.ogg")
end

-- Demonic Conjuring --

Skill_DemonicConjuring = Base_SummonSpell:New()
Skill_DemonicConjuring.CostPerLevel = 10
Skill_DemonicConjuring.ManaCostBase = 25 - Skill_DemonicConjuring.CostPerLevel
Skill_DemonicConjuring.BaseHealth = 100
Skill_DemonicConjuring.BaseMinDamage = 10
Skill_DemonicConjuring.BaseMaxDamage = 20
Skill_DemonicConjuring.BaseArmor = 1
Skill_DemonicConjuring.HealthPerLevel = 40
Skill_DemonicConjuring.DamagePerLevel = 10
Skill_DemonicConjuring.ArmorPerLevel = 1
Skill_DemonicConjuring.Limit = 1
Skill_DemonicConjuring.LimitPerLevel = 0.1
Skill_DemonicConjuring.Duration = 30
Skill_DemonicConjuring.DurationPerLevel = 1
Skill_DemonicConjuring.Monster = Monsters[23]

function Skill_DemonicConjuring.GetInfo(self, Source, Item)
	MinDamage, MaxDamage = self:GetDamage(Source, Item.Level)

	return "Summon a demon to fight for you that has [c green]" .. self:GetHealth(Source, Item.Level) .. "[c white] HP and does [c green]" .. MinDamage .. "-" .. MaxDamage .. "[c white] fire damage\nCan summon a maximum of [c green]" .. self:GetLimit(Source, Item.Level) .. "[c white]\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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
Skill_RaiseDead.ArmorPerLevel = 0.25
Skill_RaiseDead.SpecialChance = 25
Skill_RaiseDead.SpecialPerLevel = 1
Skill_RaiseDead.Limit = 2
Skill_RaiseDead.LimitPerLevel = 0.2
Skill_RaiseDead.SkillLevel = 1
Skill_RaiseDead.SkillLevelPerLevel = 0.5
Skill_RaiseDead.Duration = 30
Skill_RaiseDead.DurationPerLevel = 1
Skill_RaiseDead.SpecialDamage = 0.85
Skill_RaiseDead.Monster = Monsters[20]
Skill_RaiseDead.SpecialMonster = Monsters[21]

function Skill_RaiseDead.GetSpecialChance(self, Source, Level)
	return math.floor(self.SpecialChance + (Level - 1) * self.SpecialPerLevel)
end

function Skill_RaiseDead.CanTarget(self, Source, Target, Alive)
	return Target.Corpse > 0 and Target.Health == 0
end

function Skill_RaiseDead.CanUse(self, Level, Source, Target)
	if Source.Mana < self:GetCost(Level) then
		return false
	end

	return self:CanTarget(Source, Target, false)
end

function Skill_RaiseDead.GetInfo(self, Source, Item)
	MinDamage, MaxDamage = self:GetDamage(Source, Item.Level)

	return "Raise a skeleton from the dead that has [c green]" .. self:GetHealth(Source, Item.Level) .. "[c white] HP and does [c green]" .. MinDamage .. "-" .. MaxDamage .. "[c white] damage\n[c green]" .. self:GetSpecialChance(Source, Item.Level) .. "%[c white] chance to summon a skeleton priest\nCan summon a maximum of [c green]" .. self:GetLimit(Source, Item.Level) .. "[c white]\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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

	if Roll <= self:GetSpecialChance(Source, Level) then
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
Skill_Enfeeble.MaxPercent = 75
Skill_Enfeeble.PercentPerLevel = 4
Skill_Enfeeble.BasePercent = 25 - Skill_Enfeeble.PercentPerLevel
Skill_Enfeeble.DurationPerLevel = 0.4
Skill_Enfeeble.Duration = 5
Skill_Enfeeble.CostPerLevel = 10
Skill_Enfeeble.ManaCostBase = 10 - Skill_Enfeeble.CostPerLevel
Skill_Enfeeble.BaseTargets = 1
Skill_Enfeeble.TargetsPerLevel = 0.2

function Skill_Enfeeble.GetPercent(self, Level)
	return math.min(math.floor(self.BasePercent + self.PercentPerLevel * Level), self.MaxPercent)
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

	return "Cripple [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] foe" .. Plural .. ", reducing their attack damage by [c green]" .. self:GetPercent(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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
Skill_Flay.PercentPerLevel = 4
Skill_Flay.BasePercent = 25 - Skill_Flay.PercentPerLevel
Skill_Flay.Duration = 4
Skill_Flay.DurationPerLevel = 0.2
Skill_Flay.CostPerLevel = 20
Skill_Flay.ManaCostBase = 50 - Skill_Flay.CostPerLevel
Skill_Flay.BaseTargets = 1
Skill_Flay.TargetsPerLevel = 0.2

function Skill_Flay.GetPercent(self, Level)
	return math.floor(self.BasePercent + self.PercentPerLevel * Level)
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

	return "Strip the skin of [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] foe" .. Plural .. ", reducing their resistances by [c green]" .. self:GetPercent(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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

-- Cleave --

Skill_Cleave = Base_Attack:New()
Skill_Cleave.DamageBase = 32
Skill_Cleave.DamagePerLevel = 1
Skill_Cleave.BaseTargets = 3
Skill_Cleave.TargetsPerLevel = 0.2

function Skill_Cleave.CanUse(self, Level, Source, Target)
	WeaponMain = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if WeaponMain == nil then
		return false
	end

	WeaponOff = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if WeaponOff == nil then
		return WeaponMain.Type ~= ITEM_OFFHAND
	end

	return WeaponMain.Type ~= ITEM_OFFHAND and WeaponOff.Type ~= ITEM_OFFHAND
end

function Skill_Cleave.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_Cleave.GenerateDamage(self, Level, Source)
	return math.floor(Source.GenerateDamage() * (self:GetDamage(Level) / 100))
end

function Skill_Cleave.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_Cleave.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	return "Swing your weapon and hit [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] foes with [c green]" .. self:GetDamage(Item.Level) .. "% [c white]weapon damage\n[c " .. TextColor .. "]Cannot use with off-hand weapons"
end

function Skill_Cleave.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Hunt --

Skill_Hunt = Base_Attack:New()
Skill_Hunt.GoldBase = 10
Skill_Hunt.GoldPerLevel = 2

function Skill_Hunt.GetGold(self, Level)
	return math.floor(Skill_Hunt.GoldBase + Skill_Hunt.GoldPerLevel * (Level - 1))
end

function Skill_Hunt.GetInfo(self, Source, Item)
	return "Attack another player and get [c green]" .. self:GetGold(Item.Level) .. "% [c white]of their gold for a kill\n\n[c yellow]Must be in a PVP zone to use"
end

function Skill_Hunt.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Hunt = self:GetGold(Level) * 0.01

	return Result
end

-- Bounty Hunt --

Skill_BountyHunt = Base_Attack:New()

function Skill_BountyHunt.GetInfo(self, Source, Item)
	return "Attack a fugitive to claim their bounty with a kill\n\n[c yellow]Can use anywhere"
end

function Skill_BountyHunt.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.BountyHunt = 1.0

	return Result
end

-- Light --

Skill_Light = Base_Spell:New()
Skill_Light.Duration = 20
Skill_Light.DurationPerLevel = 5
Skill_Light.CostPerLevel = 5
Skill_Light.ManaCostBase = 50 - Skill_Light.CostPerLevel

function Skill_Light.GetInfo(self, Source, Item)
	return "Emit magic light for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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

function Skill_Portal.GetInfo(self, Source, Item)
	return "Teleport home after [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Portal.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Teleport = self:GetDuration(Level)
	WeaponProc(Source, Target, Result, true)

	return Result
end

-- Blade Dance --

Skill_BladeDance = Base_Attack:New()
Skill_BladeDance.BaseChance = 25
Skill_BladeDance.ChancePerLevel = 0
Skill_BladeDance.Duration = 5
Skill_BladeDance.IncreasePerLevel = 6
Skill_BladeDance.BleedingLevel = 10
Skill_BladeDance.BaseTargets = 2
Skill_BladeDance.TargetsPerLevel = 0.2
Skill_BladeDance.DamageBase = 40
Skill_BladeDance.DamagePerLevel = 1

function Skill_BladeDance.CanUse(self, Level, Source, Target)
	WeaponMain = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if WeaponMain == nil then
		return false
	end

	WeaponOff = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if WeaponOff == nil then
		return false
	end

	return WeaponOff.Type == ITEM_OFFHAND
end

function Skill_BladeDance.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_BladeDance.GetChance(self, Level)
	return math.min(self.BaseChance + self.ChancePerLevel * (Level - 1), 100)
end

function Skill_BladeDance.GetBleedLevel(self, Source, Level)
	return math.floor((self.BleedingLevel + self.IncreasePerLevel * (Level - 1)) * Source.BleedPower)
end

function Skill_BladeDance.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_BladeDance.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	return "Whirl in a dance of blades, hitting [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] enemies with [c green]" .. self:GetDamage(Item.Level) .. "%[c white] weapon damage and a [c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to cause level [c green]" .. self:GetBleedLevel(Source, Item.Level) .. " [c yellow]bleeding\n[c " .. TextColor .. "]Requires dual-wielding weapons"
end

function Skill_BladeDance.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buff_Bleeding.Pointer
		Result.Target.BuffLevel = self:GetBleedLevel(Source, Level)
		Result.Target.BuffDuration = self.Duration
		return true
	end

	return false
end

function Skill_BladeDance.PlaySound(self, Level)
	Audio.Play("gash0.ogg")
end

-- Magic Barrier --

Skill_MagicBarrier = Base_Spell:New()
Skill_MagicBarrier.CostPerLevel = 5
Skill_MagicBarrier.ManaCostBase = 25 - Skill_MagicBarrier.CostPerLevel
Skill_MagicBarrier.BaseBlock = 120
Skill_MagicBarrier.BlockPerLevel = 40
Skill_MagicBarrier.Duration = 10
Skill_MagicBarrier.DurationPerLevel = 1

function Skill_MagicBarrier.GetLevel(self, Source, Level)
	return math.floor((self.BaseBlock + self.BlockPerLevel * (Level - 1)) * Source.ManaPower)
end

function Skill_MagicBarrier.GetInfo(self, Source, Item)
	return "Create a magic shield around an ally that blocks [c green]" .. self:GetLevel(Source, Item.Level) .. "[c white] attack damage before breaking\nLasts [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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

-- Ice Nova --

Skill_IceNova = Base_Spell:New()
Skill_IceNova.DamageBase = 50
Skill_IceNova.DamagePerLevel = 25
Skill_IceNova.CostPerLevel = 10
Skill_IceNova.ManaCostBase = 80 - Skill_IceNova.CostPerLevel
Skill_IceNova.BaseTargets = 3
Skill_IceNova.TargetsPerLevel = 0.2
Skill_IceNova.Duration = 3.2
Skill_IceNova.DurationPerLevel = 0.2

function Skill_IceNova.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_IceNova.GetDamagePower(self, Source, Level)
	return Source.ColdPower
end

function Skill_IceNova.GetInfo(self, Source, Item)
	return "Summon an icy explosion, hitting [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] enemies for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] cold damage that slows for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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
Skill_ChainLightning.DamageBase = 100
Skill_ChainLightning.DamagePerLevel = 20
Skill_ChainLightning.CostPerLevel = 8
Skill_ChainLightning.ManaCostBase = 90 - Skill_ChainLightning.CostPerLevel
Skill_ChainLightning.BaseTargets = 3
Skill_ChainLightning.TargetsPerLevel = 0.2
Skill_ChainLightning.Duration = 2
Skill_ChainLightning.DurationPerLevel = 0.1
Skill_ChainLightning.Chance = 35
Skill_ChainLightning.ChancePerLevel = 0

function Skill_ChainLightning.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_ChainLightning.GetDamagePower(self, Source, Level)
	return Source.LightningPower
end

function Skill_ChainLightning.GetDuration(self, Level)
	return self.Duration + self.DurationPerLevel * (Level - 1)
end

function Skill_ChainLightning.GetChance(self, Level)
	return math.floor(self.Chance + self.ChancePerLevel * (Level - 1))
end

function Skill_ChainLightning.GetInfo(self, Source, Item)
	return "Summon a powerful bolt of energy, hitting [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] enemies for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] damage with a [c green]" .. self:GetChance(Item.Level) .. "%[c white] chance to stun for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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
	Audio.Play("shock0.ogg")
end

-- Rupture --

Skill_Rupture = Base_Spell:New()
Skill_Rupture.Level = 20
Skill_Rupture.LevelPerLevel = 10
Skill_Rupture.CostPerLevel = 15
Skill_Rupture.ManaCostBase = 90 - Skill_Rupture.CostPerLevel
Skill_Rupture.BaseTargets = 3
Skill_Rupture.TargetsPerLevel = 0.2
Skill_Rupture.Duration = 10
Skill_Rupture.DurationPerLevel = 0

function Skill_Rupture.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_Rupture.GetDamage(self, Source, Level)
	return self:GetLevel(Source, Level) * self:GetDuration(Level)
end

function Skill_Rupture.GetDamagePower(self, Source, Level)
	return Source.PoisonPower
end

function Skill_Rupture.GetLevel(self, Source, Level)
	return math.floor((self.Level + self.LevelPerLevel * Level) * self:GetDamagePower(Source, Level))
end

function Skill_Rupture.CanTarget(self, Source, Target, Alive)
	if Alive == true then
		return Target.Health > 0
	else
		return Target.Corpse > 0 and Target.Health == 0
	end
end

function Skill_Rupture.CanUse(self, Level, Source, Target)
	if Source.Mana < self:GetCost(Level) then
		return false
	end

	return self:CanTarget(Source, Target, false)
end

function Skill_Rupture.GetInfo(self, Source, Item)
	return "Explode a corpse, releasing noxious gas over [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] enemies for [c green]" .. self:GetDamage(Source, Item.Level) .. "[c white] poison damage over [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Rupture.Use(self, Level, Duration, Source, Target, Result)
	if Target.Health == 0 then
		Result.Target.Corpse = -1
	else
		Result.Target.Buff = Buff_Poisoned.Pointer
		Result.Target.BuffLevel = self:GetLevel(Source, Level)
		Result.Target.BuffDuration = self:GetDuration(Level)
	end
	WeaponProc(Source, Target, Result, true)

	return Result
end

function Skill_Rupture.PlaySound(self, Level)
	Audio.Play("rupture0.ogg")
end

-- Sanctuary --

Skill_Sanctuary = Base_Spell:New()
Skill_Sanctuary.Level = 1
Skill_Sanctuary.LevelPerLevel = 1
Skill_Sanctuary.CostPerLevel = 20
Skill_Sanctuary.ManaCostBase = 100 - Skill_Sanctuary.CostPerLevel
Skill_Sanctuary.BaseTargets = 3
Skill_Sanctuary.TargetsPerLevel = 0.2
Skill_Sanctuary.Duration = 10
Skill_Sanctuary.DurationPerLevel = 0

function Skill_Sanctuary.GetTargetCount(self, Level)
	return math.floor(self.BaseTargets + self.TargetsPerLevel * Level)
end

function Skill_Sanctuary.GetHeal(self, Source, Level)
	return Buff_Sanctuary.Heal * self:GetLevel(Source, Level) * self:GetDuration(Level)
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
	return "Imbue [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] allies with sanctuary for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds, granting [c green]" .. self:GetArmor(Source, Item.Level) .. "[c white] armor, [c green]" .. self:GetDamageBlock(Source, Item.Level) .. "[c white] damage block and [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
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
