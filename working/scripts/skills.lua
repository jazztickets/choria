-- MONSTER SKILLS --

-- Monster attack --

Skill_MonsterAttack = Base_Attack:New()

-- Crow attack --

Skill_CrowAttack = Base_Attack:New()
Skill_CrowAttack.Stamina = 50

function Skill_CrowAttack.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buff_Blinded.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 5
	end
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
	end
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
	end
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
	end
end

function Skill_VenomBite.PlaySound(self, Level)
	Audio.Play("bat0.ogg")
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
	end
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
	end
end

function Skill_PincerAttack.PlaySound(self, Level)
	Audio.Play("multislash0.ogg")
end

-- Chew attack --

Skill_ChewAttack = Base_Attack:New()

function Skill_ChewAttack.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 75 then
		Result.Source.Health = -Result.Target.Health
	end
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
	end
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

-- Punch --

Skill_Punch = Base_Attack:New()
Skill_Punch.Stamina = 10
Skill_Punch.AttackDelay = 0.2
Skill_Punch.AttackTime = 0.3
Skill_Punch.Cooldown = 0.1

function Skill_Punch.GetInfo(self, Item)

	return "Punch an enemy with your fists"
end

function Skill_Punch.PlaySound(self, Level)
	Audio.Play("thud0.ogg")
end

function Skill_Punch.GetAttackTimes(self, Object)

	return self.AttackDelay, self.AttackTime, self.Cooldown
end

-- Slash --

Skill_Slash = Base_Attack:New()
Skill_Slash.BaseChance = 4
Skill_Slash.ChancePerLevel = 2
Skill_Slash.Stamina = 20
Skill_Slash.AttackDelay = 0.5
Skill_Slash.AttackTime = 0.35
Skill_Slash.Cooldown = 0.2

function Skill_Slash.GetInfo(self, Item)

	return "Slash your enemies for [c green]100% [c white] weapon damage"
end

function Skill_Slash.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

function Skill_Slash.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_Slash.GenerateDamage(self, Level, Source)
	Damage = Source.GenerateDamage()

	Crit = false
	--[[
	if Random.GetInt(1, 100) <= self:GetChance(Level) then
		Damage = Damage * 3
		Crit = true
	end
	--]]

	return Damage, Crit
end

-- Cleave --

Skill_Cleave = Base_Attack:New()
Skill_Cleave.DamageBase = 70
Skill_Cleave.DamagePerLevel = 2
Skill_Cleave.Stamina = 55
Skill_Cleave.AttackDelay = 0.7
Skill_Cleave.AttackTime = 0.4
Skill_Cleave.Cooldown = 0.3

function Skill_Cleave.GetDamage(self, Level)
	return math.floor(Skill_Cleave.DamageBase + Skill_Cleave.DamagePerLevel * (Level - 1))
end

function Skill_Cleave.GenerateDamage(self, Level, Source)
	return math.floor(Source.GenerateDamage() * (self:GetDamage(Level) / 100))
end

function Skill_Cleave.GetTargetCount(self, Level)
	return 2
end

function Skill_Cleave.GetInfo(self, Item)
	return "Swing your weapon and hit [c green]" .. self.GetTargetCount(Item.Level) .. " [c white]enemies with [c green]" .. self:GetDamage(Item.Level) .. "% [c white]weapon damage"
end

function Skill_Cleave.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Fury --

Skill_Fury = Base_Attack:New()
Skill_Fury.StaminaPerLevel = 0.04
Skill_Fury.BaseStamina = 0.25

function Skill_Fury.GetStaminaGain(self, Level)

	return math.min(self.BaseStamina + self.StaminaPerLevel * Level, 1.0)
end

function Skill_Fury.GetInfo(self, Item)

	return "Attack with your weapon and gain [c green]" .. math.floor(self:GetStaminaGain(Item.Level) * 100) .. "% [c yellow]stamina [c white]for a killing blow"
end

function Skill_Fury.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

function Skill_Fury.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Target.Health + Result.Target.Health <= 0 then
		Result.Source.Stamina = self:GetStaminaGain(Level)
	end
end

-- Gash --

Skill_Gash = Base_Attack:New()
Skill_Gash.BaseChance = 35
Skill_Gash.ChancePerLevel = 0
Skill_Gash.Duration = 5
Skill_Gash.IncreasePerLevel = 6
Skill_Gash.BleedingLevel = 10 - Skill_Gash.IncreasePerLevel

function Skill_Gash.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_Gash.GetBleedLevel(self, Level)

	return math.floor(self.BleedingLevel + self.IncreasePerLevel * Level)
end

function Skill_Gash.GetInfo(self, Item)

	return "Slice your enemy\n[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to cause level [c green]" .. self:GetBleedLevel(Item.Level) .. " [c yellow]bleeding"
end

function Skill_Gash.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buff_Bleeding.Pointer
		Result.Target.BuffLevel = self:GetBleedLevel(Level)
		Result.Target.BuffDuration = self.Duration
	end
end

function Skill_Gash.PlaySound(self, Level)
	Audio.Play("gash0.ogg")
end

-- Shield Bash --

Skill_ShieldBash = Base_Attack:New()
Skill_ShieldBash.BaseChance = 23
Skill_ShieldBash.ChancePerLevel = 4
Skill_ShieldBash.Duration = 2

function Skill_ShieldBash.GetInfo(self, Item)

	return "Bash your enemy with a shield\n[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to [c yellow]stun [c white]for [c green]" .. self.Duration .. " [c white]seconds"
end

function Skill_ShieldBash.GenerateDamage(self, Level, Source)
	Shield = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if Shield == nil then
		return 0
	end

	return Shield.GenerateDamage(Shield.Upgrades)
end

function Skill_ShieldBash.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_ShieldBash.CanUse(self, Level, Object)
	Shield = Object.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)

	return Shield ~= nil
end

function Skill_ShieldBash.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = self.Duration
	end
end

function Skill_ShieldBash.PlaySound(self, Level)
	Audio.Play("bash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Whirlwind --

Skill_Whirlwind = Base_Attack:New()
Skill_Whirlwind.DamageBase = 20
Skill_Whirlwind.DamagePerLevel = 2
Skill_Whirlwind.SlowDurationPerLevel = 2.0 / 3.0
Skill_Whirlwind.SlowDuration = 3 - Skill_Whirlwind.SlowDurationPerLevel

function Skill_Whirlwind.CanUse(self, Level, Object)
	Weapon = Object.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
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

function Skill_Whirlwind.ApplyCost(self, Level, Result)
	Result.Source.Buff = Buff_Slowed.Pointer
	Result.Source.BuffLevel = 30
	Result.Source.BuffDuration = self:GetDuration(Level)

	return Result
end

function Skill_Whirlwind.GetInfo(self, Item)
	return "Slash all enemies with [c green]" .. self:GetDamage(Item.Level) .. "% [c white]weapon damage\nCauses [c yellow]fatigue [c white]for [c green]" .. self:GetDuration(Item.Level) .." [c white]seconds\nRequires a two-handed weapon"
end

function Skill_Whirlwind.PlaySound(self, Level)
	Audio.Play("multislash0.ogg")
end

-- Rejuvenation --

Skill_Rejuvenation = Base_Spell:New()
Skill_Rejuvenation.Duration = 5
Skill_Rejuvenation.CostPerLevel = 5
Skill_Rejuvenation.ManaCostBase = 5 - Skill_Rejuvenation.CostPerLevel

function Skill_Rejuvenation.GetLevel(self, Level)
	return 10 * (Level)
end

function Skill_Rejuvenation.GetDuration(self, Level)
	return self.Duration
end

function Skill_Rejuvenation.GetInfo(self, Item)

	return "Heal [c green]" .. Buff_Healing.Heal * self:GetLevel(Item.Level) * self:GetDuration(Item.Level) .. " [c white]HP [c white]over [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"

end

function Skill_Rejuvenation.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Healing.Pointer
	Result.Target.BuffLevel = self:GetLevel(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

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

function Skill_Heal.GetInfo(self, Item)

	return "Heal target for [c green]" .. (self.HealBase + self.HealPerLevel * Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Heal.Use(self, Level, Duration, Source, Target, Result)

	Result.Target.Health = self.HealBase + self.HealPerLevel * Level

	return Result
end

function Skill_Heal.PlaySound(self, Level)
	Audio.Play("heal0.ogg")
end

-- Resurrect --

Skill_Resurrect = Base_Spell:New()
Skill_Resurrect.HealBase = -50
Skill_Resurrect.HealPerLevel = 60
Skill_Resurrect.CostPerLevel = 40
Skill_Resurrect.ManaCostBase = 200 - Skill_Resurrect.CostPerLevel

function Skill_Resurrect.GetInfo(self, Item)

	return "Resurrect target and give [c green]" .. (self.HealBase + self.HealPerLevel * Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Resurrect.Use(self, Level, Duration, Source, Target, Result)

	Result.Target.Health = self.HealBase + self.HealPerLevel * Level

	return Result
end

function Skill_Resurrect.PlaySound(self, Level)
	Audio.Play("choir0.ogg")
end

-- Spark --

Skill_Spark = Base_Spell:New()
Skill_Spark.DamageBase = 10
Skill_Spark.Multiplier = 30
Skill_Spark.CostPerLevel = 4
Skill_Spark.ManaCostBase = 10 - Skill_Spark.CostPerLevel

function Skill_Spark.GetInfo(self, Item)
	return "Shock a target for [c green]" .. self:GetDamage(Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Spark.PlaySound(self, Level)
	Audio.Play("shock0.ogg", 0.4)
end

-- Icicle --

Skill_Icicle = Base_Spell:New()
Skill_Icicle.DamageBase = 25
Skill_Icicle.Multiplier = 25
Skill_Icicle.CostPerLevel = 4
Skill_Icicle.Duration = 5
Skill_Icicle.ManaCostBase = 15 - Skill_Icicle.CostPerLevel

function Skill_Icicle.GetInfo(self, Item)
	return "Pierce a target for [c green]" .. self:GetDamage(Item.Level) .. "[c white] HP\nSlows for [c green]" .. self.Duration .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Icicle.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = 20
	Result.Target.BuffDuration = 3

	return Result
end

function Skill_Icicle.PlaySound(self, Level)
	Audio.Play("ice" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Fire Blast --

Skill_FireBlast = Base_Spell:New()
Skill_FireBlast.DamageBase = 50
Skill_FireBlast.Multiplier = 30
Skill_FireBlast.CostPerLevel = 10
Skill_FireBlast.ManaCostBase = 60 - Skill_FireBlast.CostPerLevel

function Skill_FireBlast.GetInfo(self, Item)
	return "Blast multiple foes with fire for [c green]" .. self:GetDamage(Item.Level) .. "[c white] HP\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_FireBlast.PlaySound(self, Level)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Ignite --

Skill_Ignite = Base_Spell:New()
Skill_Ignite.BurnLevel = 10
Skill_Ignite.BurnLevelPerLevel = 3
Skill_Ignite.CostPerLevel = 10
Skill_Ignite.DurationPerLevel = 0
Skill_Ignite.Duration = 6 - Skill_Ignite.DurationPerLevel
Skill_Ignite.ManaCostBase = 30 - Skill_Ignite.CostPerLevel

function Skill_Ignite.GetBurnLevel(self, Level)
	return math.floor(self.BurnLevel + self.BurnLevelPerLevel * Level)
end

function Skill_Ignite.GetDamage(self, Level)
	return Buff_Burning.Damage * self:GetBurnLevel(Level) * self:GetDuration(Level)
end

function Skill_Ignite.GetDuration(self, Level)
	return math.floor(self.Duration + self.DurationPerLevel * Level)
end

function Skill_Ignite.GetInfo(self, Item)
	return "Ignite an enemy and deal [c green]" .. self:GetDamage(Item.Level) .. "[c white] damage over [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Ignite.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = self:GetBurnLevel(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

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

function Skill_Toughness.GetInfo(self, Item)
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

function Skill_ArcaneMastery.GetManaRegen(self, Level)

	return self.ManaRegen * math.floor(Level / 1)
end

function Skill_ArcaneMastery.GetInfo(self, Item)
	BonusText = ""
	if self:GetManaRegen(Item.Level) > 0 then
		BonusText = "\n[c white]Increase mana regen by [c green]" .. self:GetManaRegen(Item.Level)
	end

	return "Increase max MP by [c light_blue]" .. Skill_ArcaneMastery.PerLevel * Item.Level .. BonusText
end

function Skill_ArcaneMastery.Stats(self, Level, Object, Change)
	Change.MaxMana = self.PerLevel * Level
	Change.ManaRegen = self:GetManaRegen(Level)

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

function Skill_Evasion.GetInfo(self, Item)
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

-- Flee --

Skill_Flee = {}
Skill_Flee.BaseChance = 22
Skill_Flee.ChancePerLevel = 6
Skill_Flee.Duration = 10

function Skill_Flee.GetChance(self, Level)

	return math.min(self.BaseChance + self.ChancePerLevel * Level, 100)
end

function Skill_Flee.GetInfo(self, Item)

	return "[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to run away from combat\nCauses [c yellow]fatigue [c white]for [c green]" .. self.Duration .. " [c white]seconds"
end

function Skill_Flee.ApplyCost(self, Level, Result)
	Result.Source.Buff = Buff_Slowed.Pointer
	Result.Source.BuffLevel = 30
	Result.Source.BuffDuration = 5

	return Result
end

function Skill_Flee.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= self:GetChance(Level) then
		Result.Target.Flee = true
		Result.Target.Buff = Buff_Slowed.Pointer
		Result.Target.BuffLevel = 70
		Result.Target.BuffDuration = self.Duration
	end
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

function Skill_Pickpocket.GetInfo(self, Item)

	return "[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to steal gold from an enemy"
end

function Skill_Pickpocket.Proc(self, Roll, Level, Duration, Source, Target, Result)

	if Roll <= self:GetChance(Level) then
		HalfGold = math.ceil(Target.Gold / 2)
		if HalfGold <= Target.Gold then
			Result.Target.Gold = -HalfGold
			Result.Source.GoldStolen = HalfGold
		end
	else
		Result.Target.Miss = true
	end
end

function Skill_Pickpocket.Use(self, Level, Duration, Source, Target, Result)
	self:Proc(Random.GetInt(1, 100), Level, Duration, Source, Target, Result)

	return Result
end

function Skill_Pickpocket.PlaySound(self, Level)
	--Audio.Play("coin" .. Random.GetInt(0, 2) .. ".ogg")
end

-- Parry --

Skill_Parry = {}
Skill_Parry.StaminaGain = Buff_Parry.StaminaGain
Skill_Parry.DamageReduction = Buff_Parry.DamageReduction
Skill_Parry.Duration = 0.5
Skill_Parry.DurationPerLevel = 0.2

function Skill_Parry.GetDuration(self, Level)

	return self.Duration + self.DurationPerLevel * Level
end

function Skill_Parry.GetInfo(self, Item)

	return "Block [c green]" .. math.floor(self.DamageReduction * 100) .. "% [c white]damage for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\n" ..
	"Gain [c green]" .. math.floor(self.StaminaGain * 100) .. "% [c yellow]stamina [c white]for each attack blocked"
end

function Skill_Parry.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Parry.Pointer
	Result.Target.BuffLevel = 1
	Result.Target.BuffDuration = self:GetDuration(Level)

	return Result
end

-- Defend --

Skill_Defend = {}
Skill_Defend.Armor = 10
Skill_Defend.ArmorPerLevel = 2
Skill_Defend.Duration = 3.4
Skill_Defend.DurationPerLevel = 0.2

function Skill_Defend.GetArmor(self, Level)

	return math.floor(self.Armor + self.ArmorPerLevel * Level)
end

function Skill_Defend.GetDuration(self, Level)

	return self.Duration + self.DurationPerLevel * Level
end

function Skill_Defend.GetInfo(self, Item)

	return "Gain [c green]" .. self:GetArmor(Item.Level) .. " [c white]armor [c white]for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nRequires a shield"
end

function Skill_Defend.CanUse(self, Level, Object)
	Shield = Object.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)

	return Shield ~= nil
end

function Skill_Defend.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Hardened.Pointer
	Result.Target.BuffLevel = self:GetArmor(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

	return Result
end

-- Backstab --

Skill_Backstab = Base_Attack:New()
Skill_Backstab.BaseDamage = 0.5
Skill_Backstab.DamagePerLevel = 0.2
Skill_Backstab.DamageMultiplier = 1.6 - Skill_Backstab.DamagePerLevel

function Skill_Backstab.GetDamage(self, Level)

	return self.DamageMultiplier + self.DamagePerLevel * Level
end

function Skill_Backstab.GetInfo(self, Item)

	return "Attack for [c green]" .. math.floor(self.BaseDamage * 100) .. "% [c white]weapon damage\nDeal [c green]" .. math.floor(self:GetDamage(Item.Level) * 100) .. "% [c white]damage to stunned enemies\nRequires a one-handed weapon"
end

function Skill_Backstab.CanUse(self, Level, Object)
	Weapon = Object.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if Weapon == nil then
		return false
	end

	return Weapon.Type == ITEM_ONEHANDED_WEAPON
end

function Skill_Backstab.Proc(self, Roll, Level, Duration, Source, Target, Result)
	for i = 1, #Target.StatusEffects do
		Effect = Target.StatusEffects[i]
		if Effect.Buff == Buff_Stunned then
			Result.Target.Health = math.floor(Result.Target.Health * self:GetDamage(Level))
			Result.Target.Crit = true
			return
		end
	end

	Result.Target.Health = math.floor(Result.Target.Health * self.BaseDamage)
end

function Skill_Backstab.PlaySound(self, Level)
	Audio.Play("gash0.ogg")
end

-- Demonic Conjuring --

Skill_DemonicConjuring = Base_Spell:New()
Skill_DemonicConjuring.CostPerLevel = 20
Skill_DemonicConjuring.ManaCostBase = 20 - Skill_DemonicConjuring.CostPerLevel
Skill_DemonicConjuring.BaseHealth = 100
Skill_DemonicConjuring.BaseMinDamage = 10
Skill_DemonicConjuring.BaseMaxDamage = 20
Skill_DemonicConjuring.BaseArmor = 1
Skill_DemonicConjuring.HealthPerLevel = 40
Skill_DemonicConjuring.DamagePerLevel = 10
Skill_DemonicConjuring.ArmorPerLevel = 1
Skill_DemonicConjuring.Monster = Monsters["demon"]

function Skill_DemonicConjuring.GetHealth(self, Level)
	return math.floor(self.BaseHealth + (Level - 1) * self.HealthPerLevel)
end

function Skill_DemonicConjuring.GetArmor(self, Level)
	return math.floor(self.BaseArmor + (Level - 1) * self.ArmorPerLevel)
end

function Skill_DemonicConjuring.GetDamage(self, Level)
	AddedDamage = math.floor((Level - 1) * self.DamagePerLevel)
	return self.BaseMinDamage + AddedDamage, self.BaseMaxDamage + AddedDamage
end

function Skill_DemonicConjuring.GetInfo(self, Item)
	MinDamage, MaxDamage = self:GetDamage(Item.Level)

	return "Summon a demon to fight for you that has [c green]" .. self:GetHealth(Item.Level) .. " HP [c white]and does [c green]" .. MinDamage .. "-" .. MaxDamage .. " [c white]damage\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_DemonicConjuring.Use(self, Level, Duration, Source, Target, Result)
	Result.Summon = {}
	Result.Summon.ID = self.Monster.ID
	Result.Summon.Health = self:GetHealth(Level)
	Result.Summon.MinDamage, Result.Summon.MaxDamage = self:GetDamage(Level)
	Result.Summon.Armor = self:GetArmor(Level)

	return Result
end

function Skill_DemonicConjuring.PlaySound(self, Level)
	Audio.Play("summon0.ogg")
end

-- Enfeeble --

Skill_Enfeeble = Base_Spell:New()
Skill_Enfeeble.PercentPerLevel = 4
Skill_Enfeeble.BasePercent = 25 - Skill_Enfeeble.PercentPerLevel
Skill_Enfeeble.DurationPerLevel = 0.4
Skill_Enfeeble.Duration = 5 - Skill_Enfeeble.DurationPerLevel
Skill_Enfeeble.CostPerLevel = 10
Skill_Enfeeble.ManaCostBase = 10 - Skill_Enfeeble.CostPerLevel

function Skill_Enfeeble.GetPercent(self, Level)
	return math.floor(self.BasePercent + self.PercentPerLevel * Level)
end

function Skill_Enfeeble.GetDuration(self, Level)
	return math.floor(self.Duration + self.DurationPerLevel * Level)
end

function Skill_Enfeeble.GetInfo(self, Item)
	return "Cripple your foe and reduce attack damage by [c green]" .. self:GetPercent(Item.Level) .. "%[c white] for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nCosts [c light_blue]" .. self:GetCost(Item.Level) .. " [c white]MP"
end

function Skill_Enfeeble.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Weak.Pointer
	Result.Target.BuffLevel = self:GetPercent(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

	return Result
end

function Skill_Enfeeble.PlaySound(self, Level)
	Audio.Play("enfeeble0.ogg")
end

-- Hunt --

Skill_Hunt = Base_Attack:New()
Skill_Hunt.GoldBase = 10
Skill_Hunt.GoldPerLevel = 2

function Skill_Hunt.GetGold(self, Level)
	return math.floor(Skill_Hunt.GoldBase + Skill_Hunt.GoldPerLevel * (Level - 1))
end

function Skill_Hunt.GetInfo(self, Item)
	return "Attack another player and get [c green]" .. self:GetGold(Item.Level) .. "% [c white]of their gold for a kill\n\n[c gray]Must be in a PVP zone to use"
end

function Skill_Hunt.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Hunt = self:GetGold(Level) * 0.01

	return Result
end

-- Bounty Hunt --

Skill_BountyHunt = Base_Attack:New()

function Skill_BountyHunt.GetInfo(self, Item)
	return "Attack a fugitive to claim their bounty with a kill\n\n[c gray]Can use anywhere"
end

function Skill_BountyHunt.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.BountyHunt = 1.0

	return Result
end
