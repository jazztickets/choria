-- Base Attack --

Base_Attack = {

	Targets = 1,
	TargetsPerLevel = 0,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetTargetCount = function(self, Level, Fraction)
		Value = self.Targets + self.TargetsPerLevel * Level
		if Fraction == nil or Fraction == false then
			return math.floor(Value)
		else
			return Value
		end
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

function Skill_AntAttack.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= 15 then
		Result.Target.Buff = Buff_Poisoned.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = 2
		return true
	end

	return false
end

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

-- Ice Imp attack --

Skill_IceImpAttack = Base_Attack:New()

function Skill_IceImpAttack.PlaySound(self, Level)
	Audio.Play("demon" .. Random.GetInt(0, 1) .. ".ogg")
end

-- PLAYER SKILLS --

-- Attack --

Skill_Attack = Base_Attack:New()
Skill_Attack.BaseChance = 8
Skill_Attack.ChancePerLevel = 0.25
Skill_Attack.DamageBase = 100
Skill_Attack.DamagePerLevel = 2
Skill_Attack.CritMultiplier = 2
Skill_Attack.CritMultiplierPerLevel = 0.02

function Skill_Attack.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_Attack.GetCritMultiplier(self, Level)
	return self.CritMultiplier + self.CritMultiplierPerLevel * (Level - 1)
end

function Skill_Attack.GetChance(self, Level)
	return math.floor(math.min(self.BaseChance + self.ChancePerLevel * Level, 100))
end

function Skill_Attack.GetInfo(self, Source, Item)
	DamageValue = self:GetDamage(Item.Level) .. "%"
	CritValue = self:GetCritMultiplier(Item.Level) .. "x"
	if Item.MoreInfo == true then
		DamageValue = Source.GetAverageDamage() * (self:GetDamage(Item.Level) * 0.01)
		CritValue = Round(DamageValue * self:GetCritMultiplier(Item.Level)) .. " avg"
		DamageValue = Round(DamageValue) .. " avg"
	end
	return "Attack for [c green]" .. DamageValue .. "[c white] weapon damage\n[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to deal [c green]" .. CritValue .. "[c white] damage"
end

function Skill_Attack.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

function Skill_Attack.GenerateDamage(self, Level, Source)
	Damage = math.floor(Source.GenerateDamage() * (self:GetDamage(Level) * 0.01))

	Crit = false
	if Random.GetInt(1, 100) <= self:GetChance(Level) then
		Damage = math.floor(Damage * self:GetCritMultiplier(Level))
		Crit = true
	end

	return Damage, Crit
end

-- Fury --

Skill_Fury = Base_Attack:New()
Skill_Fury.StaminaPerLevel = 0.01
Skill_Fury.BaseStamina = 0.28
Skill_Fury.SpeedDuration = 2
Skill_Fury.SpeedDurationPerLevel = 0.1

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
Skill_Gash.DamageBase = 100.5
Skill_Gash.DamagePerLevel = 0.5
Skill_Gash.BaseChance = 36
Skill_Gash.ChancePerLevel = 1
Skill_Gash.Duration = 5
Skill_Gash.BleedingLevel = 10
Skill_Gash.IncreasePerLevel = 3
Skill_Gash.BleedScale = 0.03

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

function Skill_Gash.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_Gash.GenerateDamage(self, Level, Source)
	return math.floor(Source.GenerateDamage() * (self:GetDamage(Level) * 0.01))
end

function Skill_Gash.GetChance(self, Level)
	return math.min(self.BaseChance + self.ChancePerLevel * (Level - 1), 100)
end

function Skill_Gash.GetBleedDamage(self, Source, Level)
	return self.Duration * self:GetBleedLevel(Source, Level)
end

function Skill_Gash.GetBleedLevel(self, Source, Level)
	return math.floor((self.BleedingLevel + self.IncreasePerLevel * (Level - 1) + Level * Level * Level * self.BleedScale) * Source.BleedPower * 0.01)
end

function Skill_Gash.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	if Item.MoreInfo == true then
		DamageValue = Round(Source.GetAverageDamage() * (self:GetDamage(Item.Level) * 0.01)) .. " [c green]avg[c white]"
	else
		DamageValue = self:GetDamage(Item.Level) .. "%"
	end

	if Item.MoreInfo == true then
		BleedDamageValue = self:GetBleedLevel(Source, Item.Level) .. "[c white] bleeding DPS"
	else
		BleedDamageValue = self:GetBleedDamage(Source, Item.Level) .. "[c white] bleeding damage over [c green]" .. self.Duration .. "[c white] seconds"
	end

	return "Slice your enemy, dealing [c green]" .. DamageValue .. "[c white] weapon damage with a [c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to cause [c green]" .. BleedDamageValue .. "\n[c " .. TextColor .. "]Requires at least one off-hand weapon"
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
Skill_ShieldBash.Duration = 2.0
Skill_ShieldBash.DurationPerLevel = 0.05
Skill_ShieldBash.DamageBase = 300
Skill_ShieldBash.DamagePerLevel = 10
Skill_ShieldBash.Constant = 10000
Skill_ShieldBash.BasePercent = 75
Skill_ShieldBash.Multiplier = 2010

function Skill_ShieldBash.GetChance(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_ShieldBash.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_ShieldBash.GetDamageMultiplier(self, Source, Level)
	return (self:GetDamage(Level) * 0.01) * (Source.ShieldDamage * 0.01) * (Source.AttackPower * 0.01)
end

function Skill_ShieldBash.GetDuration(self, Level)
	return math.floor(10 * (self.Duration + self.DurationPerLevel * (Level - 1))) / 10
end

function Skill_ShieldBash.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	DamageValue = self:GetDamage(Item.Level) .. "%"

	Shield = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if Shield == nil then
		AverageDamage = 0
	else
		AverageDamage = Shield.GetAverageDamage(Source.Pointer, Shield.Upgrades)
	end

	if Item.MoreInfo == true then
		DamageValue = Round(AverageDamage * self:GetDamageMultiplier(Source, Item.Level)) .. " [c green]avg[c white]"
	end

	return "Bash your enemy with a shield for [c green]" .. DamageValue .. "[c white] shield damage and a [c green]" .. self:GetChance(Item.Level) .. "%[c white] chance to [c yellow]stun [c white]for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\n[c " .. TextColor .. "]Requires a shield"
end

function Skill_ShieldBash.GenerateDamage(self, Level, Source)
	Shield = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if Shield == nil then
		return 0
	end

	return math.floor(Shield.GenerateDamage(Source.Pointer, Shield.Upgrades)) * self:GetDamageMultiplier(Source, Level)
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

-- Backstab --

Skill_Backstab = Base_Attack:New()
Skill_Backstab.BaseDamage = 50
Skill_Backstab.DamagePerLevel = 20
Skill_Backstab.Stamina = 90
Skill_Backstab.DamageMultiplier = 300 - Skill_Backstab.DamagePerLevel

function Skill_Backstab.GetDamage(self, Level)
	return self.DamageMultiplier + self.DamagePerLevel * Level
end

function Skill_Backstab.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	BaseDamageValue = math.floor(self.BaseDamage) .. "%"
	DamageValue = self:GetDamage(Item.Level) .. "%"
	if Item.MoreInfo == true then
		BaseDamageValue = Round(Source.GetAverageDamage() * (self.BaseDamage * 0.01)) .. " avg"
		DamageValue = Round(Source.GetAverageDamage() * (self:GetDamage(Item.Level) * 0.01)) .. " avg"
	end

	return "Attack for [c green]" .. BaseDamageValue .. "[c white] weapon damage\nDeal [c green]" .. DamageValue .. "[c white] damage to stunned enemies\nGain [c green]" .. self.Stamina .. "%[c white] [c yellow]stamina[c white] for a kill\n[c " .. TextColor .. "]Can only use off-hand weapons"
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
			Result.Target.Health = math.floor(Result.Target.Health * (self:GetDamage(Level) * 0.01))
			if Target.Health + Result.Target.Health < 0 then
				Result.Source.Stamina = self.Stamina * 0.01
			end
			Result.Target.Crit = true
			return true
		end
	end

	Result.Target.Health = math.floor(Result.Target.Health * self.BaseDamage * 0.01)
	return false
end

function Skill_Backstab.PlaySound(self, Level)
	Audio.Play("gash0.ogg")
end

-- Cleave --

Skill_Cleave = Base_Attack:New()
Skill_Cleave.DamageBase = 50
Skill_Cleave.DamagePerLevel = 1.75
Skill_Cleave.Targets = 3
Skill_Cleave.TargetsPerLevel = 0

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

function Skill_Cleave.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	DamageValue = self:GetDamage(Item.Level) .. "%"
	if Item.MoreInfo == true then
		DamageValue = Round(Source.GetAverageDamage() * (self:GetDamage(Item.Level) * 0.01)) .. " avg"
	end

	return "Swing your weapon and hit [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] foes with [c green]" .. DamageValue .. "[c white] weapon damage\n[c " .. TextColor .. "]Cannot use with off-hand weapons"
end

function Skill_Cleave.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Blade Dance --

Skill_BladeDance = Base_Attack:New()
Skill_BladeDance.BaseChance = 75
Skill_BladeDance.ChancePerLevel = 0
Skill_BladeDance.Targets = 4
Skill_BladeDance.TargetsPerLevel = 0.08
Skill_BladeDance.DamageBase = 75
Skill_BladeDance.DamagePerLevel = 2
Skill_BladeDance.BleedingLevel = 20
Skill_BladeDance.IncreasePerLevel = 10
Skill_BladeDance.BleedScale = 2
Skill_BladeDance.Duration = 5

function Skill_BladeDance.CanUse(self, Level, Source, Target)
	WeaponMain = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if WeaponMain == nil then
		return false
	end

	WeaponOff = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if WeaponOff == nil then
		return false
	end

	return WeaponMain.Type == ITEM_OFFHAND and WeaponOff.Type == ITEM_OFFHAND
end

function Skill_BladeDance.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_BladeDance.GetChance(self, Level)
	return math.min(self.BaseChance + self.ChancePerLevel * (Level - 1), 100)
end

function Skill_BladeDance.GenerateDamage(self, Level, Source)
	return math.floor(Source.GenerateDamage() * (self:GetDamage(Level) * 0.01))
end

function Skill_BladeDance.GetBleedDamage(self, Source, Level)
	return self.Duration * self:GetBleedLevel(Source, Level)
end

function Skill_BladeDance.GetBleedLevel(self, Source, Level)
	return math.floor((self.BleedingLevel + self.IncreasePerLevel * (Level - 1) + Level * Level * self.BleedScale) * Source.BleedPower * 0.01)
end

function Skill_BladeDance.GetInfo(self, Source, Item)
	TextColor = "yellow"
	if not self:CanUse(Item.Level, Source, nil) then
		TextColor = "red"
	end

	DamageValue = self:GetDamage(Item.Level) .. "%"
	if Item.MoreInfo == true then
		DamageValue = Round(Source.GetAverageDamage() * (self:GetDamage(Item.Level) * 0.01)) .. " avg"
	end

	if Item.MoreInfo == true then
		BleedDamageValue = self:GetBleedLevel(Source, Item.Level) .. "[c white] bleeding DPS"
	else
		BleedDamageValue = self:GetBleedDamage(Source, Item.Level) .. "[c white] bleeding damage over [c green]" .. self.Duration .. "[c white] seconds"
	end

	return "Whirl in a dance of blades, hitting [c green]" .. self:GetTargetCount(Item.Level, Item.MoreInfo) .. "[c white] enemies with [c green]" .. DamageValue .. "[c white] weapon damage and a [c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to cause [c green]" .. BleedDamageValue .. "\n[c " .. TextColor .. "]Requires two off-hand weapons"
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

-- Whirlwind --

Skill_Whirlwind = Base_Attack:New()
Skill_Whirlwind.DamageBase = 75
Skill_Whirlwind.DamagePerLevel = 1
Skill_Whirlwind.Duration = 3
Skill_Whirlwind.DurationPerLevel = 0

function Skill_Whirlwind.CanUse(self, Level, Source, Target)
	Weapon = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if Weapon == nil then
		return false
	end

	return Weapon.Type == ITEM_TWOHANDED_WEAPON
end

function Skill_Whirlwind.GetDamage(self, Level)
	return math.floor(self.DamageBase + self.DamagePerLevel * (Level - 1))
end

function Skill_Whirlwind.GenerateDamage(self, Level, Source)
	return math.floor(Source.GenerateDamage() * (self:GetDamage(Level) * 0.01))
end

function Skill_Whirlwind.GetDuration(self, Level)
	return math.max(self.Duration + self.DurationPerLevel * (Level - 1), 0.5)
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

	DamageValue = self:GetDamage(Item.Level) .. "%"
	if Item.MoreInfo == true then
		DamageValue = Round(Source.GetAverageDamage() * (self:GetDamage(Item.Level) * 0.01)) .. " avg"
	end

	return "Spin around and slash all enemies with [c green]" .. DamageValue .. "[c white] weapon damage\nCauses [c yellow]fatigue[c white] for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds\n[c " .. TextColor .. "]Requires a two-handed weapon"
end

function Skill_Whirlwind.PlaySound(self, Level)
	Audio.Play("multislash0.ogg")
end

-- Toughness --

Skill_Toughness = {}
Skill_Toughness.HealthPerLevel = 50
Skill_Toughness.Armor = 5
Skill_Toughness.ArmorPerLevel = 0.5

function Skill_Toughness.GetArmor(self, Level)
	return math.floor(self.Armor + self.ArmorPerLevel * math.floor(Level - 1))
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
	Change.ManaPower = self:GetPower(Level)

	return Change
end

-- Evasion --

Skill_Evasion = {}
Skill_Evasion.ChancePerLevel = 1
Skill_Evasion.BaseChance = 10
Skill_Evasion.BattleSpeed = 5
Skill_Evasion.BattleSpeedPerLevel = 0.5

function Skill_Evasion.GetChance(self, Level)

	return math.min(math.floor(self.BaseChance + self.ChancePerLevel * Level), 100)
end

function Skill_Evasion.GetBattleSpeed(self, Level)

	return math.floor(self.BattleSpeed + self.BattleSpeedPerLevel * (Level - 1))
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
Skill_EnergyField.Constant = 28
Skill_EnergyField.BasePercent = 6
Skill_EnergyField.Multiplier = 100

function Skill_EnergyField.GetReduction(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_EnergyField.GetInfo(self, Source, Item)
	return "Convert [c green]" .. self:GetReduction(Item.Level) .. "%[c white] of attack damage taken to mana drain"
end

function Skill_EnergyField.Stats(self, Level, Object, Change)
	Change.EnergyField = self:GetReduction(Level)

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
	Change.PhysicalPower = self:GetPower(Level)

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
	Change.FirePower = self:GetPower(Level)

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
	Change.ColdPower = self:GetPower(Level)

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
	Change.LightningPower = self:GetPower(Level)

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
	Change.BleedPower = self:GetPower(Level)

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
	Change.PoisonPower = self:GetPower(Level)

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
	Change.HealPower = self:GetPower(Level)

	return Change
end

-- Pet Mastery --

Skill_PetMastery = {}
Skill_PetMastery.Power = 25
Skill_PetMastery.PowerPerLevel = 5
Skill_PetMastery.SummonLimit = 1.1
Skill_PetMastery.SummonLimitPerLevel = 0.1

function Skill_PetMastery.GetPower(self, Level)
	return math.floor(self.Power + self.PowerPerLevel * (Level - 1))
end

function Skill_PetMastery.GetSummonLimit(self, Level, Fraction)
	Value = self.SummonLimit + self.SummonLimitPerLevel * (Level - 1)
	if Fraction == true then
		return Value
	else
		return math.floor(Value)
	end
end

function Skill_PetMastery.GetInfo(self, Source, Item)
	return "Increase pet power by [c green]" .. self:GetPower(Item.Level) .. "%\nIncrease summon limit by [c green]" .. self:GetSummonLimit(Item.Level, Item.MoreInfo)
end

function Skill_PetMastery.Stats(self, Level, Object, Change)
	Change.PetPower = self:GetPower(Level)
	Change.SummonLimit = self:GetSummonLimit(Level, false)

	return Change
end

-- Flee --

Skill_Flee = {}
Skill_Flee.Duration = 5
Skill_Flee.DurationPerLevel = -0.1
Skill_Flee.Constant = 30
Skill_Flee.BasePercent = 32
Skill_Flee.Multiplier = 100
Skill_Flee.MissMultiplier = 25

function Skill_Flee.CanUse(self, Level, Source, Target)
	if not Source.BossBattle then
		return true
	end

	return Source.BossBattle and Source.GoldStolen == 0
end

function Skill_Flee.GetAttempts(self, Source)
	if Source.Server == false then
		return 0
	end

	if Source.BattleID ~= nil and Battles[Source.BattleID] ~= nil then
		local Storage = Battles[Source.BattleID]
		if Storage[Source.ID] ~= nil then
			return Storage[Source.ID].FleeAttempts
		end
	end

	return 0
end

function Skill_Flee.GetChance(self, Source, Level)
	ExtraChance = self:GetAttempts(Source) * self.MissMultiplier
	Chance = math.min(math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent) + ExtraChance, 100)
	return Chance
end

function Skill_Flee.GetDuration(self, Level)
	return math.max(self.Duration + self.DurationPerLevel * (Level - 1), 0.5)
end

function Skill_Flee.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Source, Item.Level) .. "% [c white]chance to run away from battle\nChance to flee goes up by [c green]" .. self.MissMultiplier .. "%[c white] after a failed attempt\nCauses [c yellow]fatigue [c white]for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\n\n[c yellow]Unusable in boss battles after [c yellow]pickpocketing"
end

function Skill_Flee.ApplyCost(self, Source, Level, Result)
	Result.Source.Buff = Buff_Slowed.Pointer
	Result.Source.BuffLevel = 30
	Result.Source.BuffDuration = self:GetDuration(Level)

	return Result
end

function Skill_Flee.Proc(self, Roll, Level, Duration, Source, Target, Result)
	local Storage = Battles[Source.BattleID]
	if Storage[Source.ID] == nil then
		Storage[Source.ID] = { FleeAttempts = 0 }
	elseif Storage[Source.ID].FleeAttempts == nil then
		Storage[Source.ID].FleeAttempts = 0
	end

	if Roll <= self:GetChance(Source, Level) then
		Result.Target.Flee = true
		Result.Target.Buff = Buff_Slowed.Pointer
		Result.Target.BuffLevel = 35
		Result.Target.BuffDuration = self.Duration
		return true
	end

	Storage[Source.ID].FleeAttempts = Storage[Source.ID].FleeAttempts + 1

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
Skill_Pickpocket.ChanceConstant = 100
Skill_Pickpocket.ChanceBasePercent = 40
Skill_Pickpocket.ChanceMultiplier = 90
Skill_Pickpocket.GoldConstant = 100
Skill_Pickpocket.GoldBasePercent = 34
Skill_Pickpocket.GoldMultiplier = 125
Skill_Pickpocket.PlayerMultiplier = 0.2

function Skill_Pickpocket.GetChance(self, Level)
	return math.floor(self.ChanceMultiplier * Level / (self.ChanceConstant + Level) + self.ChanceBasePercent)
end

function Skill_Pickpocket.GetGold(self, Level)
	return math.floor(self.GoldMultiplier * Level / (self.GoldConstant + Level) + self.GoldBasePercent)
end

function Skill_Pickpocket.GetGoldPlayer(self, Level)
	return self:GetGold(Level) * self.PlayerMultiplier
end

function Skill_Pickpocket.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item.Level) .. "% [c white]chance to steal [c green]" .. self:GetGold(Item.Level) .. "%][c white] gold from a monster or [c green]" .. self:GetGoldPlayer(Item.Level) .. "%[c white] from a player"
end

function Skill_Pickpocket.Proc(self, Roll, Level, Duration, Source, Target, Result)

	if Roll <= self:GetChance(Level) then
		GoldAvailable = Target.Gold
		if Target.MonsterID > 0 then
			GoldAmount = math.ceil(GoldAvailable * self:GetGold(Level) * 0.01)
		else
			GoldAmount = math.ceil(GoldAvailable * self:GetGoldPlayer(Level) * 0.01)
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
Skill_Parry.DurationPerLevel = 0.1
Skill_Parry.Constant = 30
Skill_Parry.BasePercent = 48
Skill_Parry.Multiplier = 80

function Skill_Parry.GetChance(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Parry.GetDuration(self, Level)
	return self.Duration + self.DurationPerLevel * Level
end

function Skill_Parry.GetDamageReduction(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Parry.GetInfo(self, Source, Item)
	Plural = ""
	if Buff_Parry.StunDuration ~= 1 then
		Plural = "s"
	end

	return "Block [c green]" .. self:GetDamageReduction(Item.Level) .. "% [c white]attack damage for [c green]" .. self:GetDuration(Item.Level) .. " [c white]seconds\nGain [c green]" .. math.floor(self.StaminaGain * 100) .. "% [c yellow]stamina [c white]for each attack blocked\nThe attacker is stunned for [c green]" .. Buff_Parry.StunDuration .. "[c white] second" .. Plural
end

function Skill_Parry.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Parry.Pointer
	Result.Target.BuffLevel = self:GetDamageReduction(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

	return Result
end

-- Taunt --

Skill_Taunt = Base_Attack:New()
Skill_Taunt.Armor = 10
Skill_Taunt.ArmorPerLevel = 0.5
Skill_Taunt.Duration = 3.01
Skill_Taunt.DurationPerLevel = 0.02
Skill_Taunt.Targets = 2
Skill_Taunt.TargetsPerLevel = 0.2

function Skill_Taunt.GetDuration(self, Level)
	return math.floor((self.Duration + self.DurationPerLevel * Level) * 10) / 10.0
end

function Skill_Taunt.GetArmor(self, Level)
	return math.floor(self.Armor + self.ArmorPerLevel * Level)
end

function Skill_Taunt.GetInfo(self, Source, Item)
	Count = self:GetTargetCount(Item.Level, Item.MoreInfo)
	Plural = "enemies"
	if Count == 1 then
		Plural = "enemy"
	end

	return "Taunt [c green]" .. Count .. "[c white] " .. Plural .. " for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds, forcing them to attack you. Increase armor by [c green]" .. self:GetArmor(Item.Level) .. "[c white] for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds"
end

function Skill_Taunt.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Taunted.Pointer
	Result.Target.BuffLevel = 1
	Result.Target.BuffDuration = self:GetDuration(Level)

	Result.Source.Buff = Buff_Hardened.Pointer
	Result.Source.BuffLevel = self:GetArmor(Level)
	Result.Source.BuffDuration = self:GetDuration(Level)

	return Result
end

function Skill_Taunt.PlaySound(self, Level)
	Audio.Play("taunt" .. Random.GetInt(0, 2) .. ".ogg")
end

-- Hunt --

Skill_Hunt = Base_Attack:New()
Skill_Hunt.MaxGold = 100
Skill_Hunt.Constant = 15
Skill_Hunt.BasePercent = 7
Skill_Hunt.Multiplier = 50

function Skill_Hunt.GetGold(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Hunt.GetInfo(self, Source, Item)
	return "Attack another player and get [c green]" .. self:GetGold(Item.Level) .. "% [c white]of their gold for a kill. If you die, you will lose [c green]" .. self:GetGold(Item.Level) .. "%[c white] gold plus your bounty\n[c yellow]Must be in a PVP zone to use"
end

function Skill_Hunt.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Hunt = self:GetGold(Level) * 0.01

	return Result
end

-- Bounty Hunt --

Skill_BountyHunt = Base_Attack:New()

function Skill_BountyHunt.GetInfo(self, Source, Item)
	return "Attack a fugitive and attempt to claim their bounty\n\n[c yellow]Can use anywhere"
end

function Skill_BountyHunt.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.BountyHunt = 1.0

	return Result
end

-- Dodge --

Skill_Dodge = Base_Attack:New()
Skill_Dodge.Duration = 1.0
Skill_Dodge.DurationPerLevel = 0.015
Skill_Dodge.Constant = 15
Skill_Dodge.BasePercent = 47
Skill_Dodge.Multiplier = 56

function Skill_Dodge.GetDuration(self, Level)
	return math.floor((self.Duration + self.DurationPerLevel * (Level - 1)) * 10) / 10
end

function Skill_Dodge.GetEvasion(self, Level)
	return math.floor(self.Multiplier * Level / (self.Constant + Level) + self.BasePercent)
end

function Skill_Dodge.GetInfo(self, Source, Item)
	return "Gain [c green]" .. self:GetEvasion(Item.Level) .. "%[c white] evasion for [c green]" .. self:GetDuration(Item.Level) .. "[c white] seconds"
end

function Skill_Dodge.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Evasion.Pointer
	Result.Target.BuffLevel = self:GetEvasion(Level)
	Result.Target.BuffDuration = self:GetDuration(Level)

	return Result
end
