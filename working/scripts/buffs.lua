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

-- Bleeding --

Buff_Bleeding = {}
Buff_Bleeding.DamageType = DamageType["Bleed"]

function Buff_Bleeding.GetInfo(self, Level)
	return "Slowly bleeding for [c red]" .. Level .. " [c white]damage"
end

function Buff_Bleeding.Update(self, Level, Source, Change)
	Damage = Level * Source.GetDamageReduction(self.DamageType)
	Damage = math.max(Damage, 0)

	Change.Health = -Damage

	return Change
end

-- Healing --

Buff_Healing = { Heal = 1 }

function Buff_Healing.GetInfo(self, Level)
	return "Slowly healing for [c green]" .. self.Heal * Level .. " [c white]HP"
end

function Buff_Healing.Update(self, Level, Source, Change)
	Change.Health = math.floor(self.Heal * Level)

	return Change
end

-- Mana --

Buff_Mana = { Mana = 1 }

function Buff_Mana.GetInfo(self, Level)
	return "Slowly regaining [c light_blue]" .. self.Mana * Level .. " [c white]MP"
end

function Buff_Mana.Update(self, Level, Source, Change)
	Change.Mana = math.floor(self.Mana * Level)

	return Change
end

-- Invis --

Buff_Invis = {}

function Buff_Invis.GetInfo(self, Level)
	return "Avoiding battle"
end

function Buff_Invis.Stats(self, Level, Source, Change)
	Change.Invisible = 1

	return Change
end

-- Slowed --

Buff_Slowed = Base_Buff:New()

function Buff_Slowed.GetInfo(self, Level)
	return "Speed reduced by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Slowed.Stats(self, Level, Source, Change)
	Change.BattleSpeed = -self:GetChange(Level)
	Change.MoveSpeed = -self:GetChange(Level) * 2

	return Change
end

-- Stunned --

Buff_Stunned = {}

function Buff_Stunned.GetInfo(self, Level)
	return "Unable to act"
end

function Buff_Stunned.Stats(self, Level, Source, Change)
	Change.Stunned = true

	return Change
end

-- Taunted --

Buff_Taunted = {}

function Buff_Taunted.GetInfo(self, Level)
	return "Can only target taunter"
end

-- Hasted --

Buff_Hasted = Base_Buff:New()

function Buff_Hasted.GetInfo(self, Level)
	return "Battle speed increased by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Hasted.Stats(self, Level, Source, Change)
	Change.BattleSpeed = self:GetChange(Level)

	return Change
end

-- Fast --

Buff_Fast = Base_Buff:New()

function Buff_Fast.GetInfo(self, Level)
	return "Move speed increased by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Fast.Stats(self, Level, Source, Change)
	Change.MoveSpeed = self:GetChange(Level)

	return Change
end

-- Blinded --

Buff_Blinded = Base_Buff:New()

function Buff_Blinded.GetInfo(self, Level)
	return "Hit chance reduced by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Blinded.Stats(self, Level, Source, Change)
	Change.HitChance = -self:GetChange(Level)

	return Change
end

-- Mighty --

Buff_Mighty = Base_Buff:New()

function Buff_Mighty.GetInfo(self, Level)
	return "Weapon damage increased by [c green]" .. Level
end

function Buff_Mighty.Stats(self, Level, Source, Change)
	Change.MinDamage = Level
	Change.MaxDamage = Level

	return Change
end

-- Hardened --

Buff_Hardened = Base_Buff:New()

function Buff_Hardened.GetInfo(self, Level)
	return "Armor increased by [c green]" .. Level
end

function Buff_Hardened.Stats(self, Level, Source, Change)
	Change.Armor = Level

	return Change
end

-- Evasion --

Buff_Evasion = Base_Buff:New()

function Buff_Evasion.GetInfo(self, Level)
	return "Evasion increased by [c green]" .. self:GetChange(Level) .. "%"
end

function Buff_Evasion.Stats(self, Level, Source, Change)
	Change.Evasion = self:GetChange(Level)

	return Change
end

-- Parry --

Buff_Parry = Base_Buff:New()
Buff_Parry.StaminaGain = 0.2
Buff_Parry.StunDuration = 1

function Buff_Parry.GetInfo(self, Level)
	return "Blocking attacks"
end

function Buff_Parry.OnHit(self, Object, Effect, Change, Result)
	Change.BuffSound = self.ID
	Change.Damage = math.max(math.floor(Change.Damage * (1.0 - (Effect.Level / 100.0))), 0)
	Change.Stamina = self.StaminaGain

	Result.Source.Buff = Buff_Stunned.Pointer
	Result.Source.BuffLevel = 1
	Result.Source.BuffDuration = self.StunDuration
end

function Buff_Parry.PlaySound(self, Level)
	Audio.Play("parry0.ogg")
end

-- Poisoned --

Buff_Poisoned = {}
Buff_Poisoned.HealthUpdateMultiplier = -50
Buff_Poisoned.DamageType = DamageType["Poison"]

function Buff_Poisoned.GetInfo(self, Level)
	return "Healing reduced by [c red]" .. math.abs(self.HealthUpdateMultiplier) .. "%[c white] and taking [c red]" .. Level .. " [c white]damage"
end

function Buff_Poisoned.Update(self, Level, Source, Change)
	Damage = Level * Source.GetDamageReduction(self.DamageType)
	Damage = math.max(math.floor(Damage), 0)
	Change.Health = -Damage

	return Change
end

function Buff_Poisoned.Stats(self, Level, Source, Change)
	Change.HealthUpdateMultiplier = self.HealthUpdateMultiplier

	return Change
end

-- Burning --

Buff_Burning = {}
Buff_Burning.DamageType = DamageType["Fire"]

function Buff_Burning.GetInfo(self, Level)
	return "Burning for [c red]" .. Level .. " [c white]damage"
end

function Buff_Burning.Update(self, Level, Source, Change)
	Damage = Level * Source.GetDamageReduction(self.DamageType)
	Damage = math.max(math.floor(Damage), 0)
	Change.Health = -Damage

	return Change
end

-- Freezing --

Buff_Freezing = {}
Buff_Freezing.SlowLevel = 200
Buff_Freezing.DamageType = DamageType["Cold"]

function Buff_Freezing.GetInfo(self, Level)
	return "Freezing for [c red]" .. Level .. " [c white]damage\nSpeed reduced by [c green]" .. self.SlowLevel .. "%"
end

function Buff_Freezing.Update(self, Level, Source, Change)
	Damage = Level * Source.GetDamageReduction(self.DamageType)
	Damage = math.max(math.floor(Damage), 0)
	Change.Health = -Damage

	return Change
end

function Buff_Freezing.Stats(self, Level, Source, Change)
	Change.BattleSpeed = -self.SlowLevel
	Change.MoveSpeed = -self.SlowLevel

	return Change
end

-- Bleed Resist --

Buff_BleedResist = Base_Buff:New()

function Buff_BleedResist.GetInfo(self, Level)
	return "Bleed resist increased by [c green]" .. Level .. "%"
end

function Buff_BleedResist.Stats(self, Level, Source, Change)
	Change.BleedResist = Level

	return Change
end

-- Poison Resist --

Buff_PoisonResist = Base_Buff:New()

function Buff_PoisonResist.GetInfo(self, Level)
	return "Poison resist increased by [c green]" .. Level .. "%"
end

function Buff_PoisonResist.Stats(self, Level, Source, Change)
	Change.PoisonResist = Level

	return Change
end

-- Cold Resist --

Buff_ColdResist = Base_Buff:New()

function Buff_ColdResist.GetInfo(self, Level)
	return "Cold resist increased by [c green]" .. Level .. "%"
end

function Buff_ColdResist.Stats(self, Level, Source, Change)
	Change.ColdResist = Level

	return Change
end

-- Weak  --

Buff_Weak = Base_Buff:New()

function Buff_Weak.GetInfo(self, Level)
	return "Attack damage reduced by [c green]" .. Level .. "%"
end

function Buff_Weak.Stats(self, Level, Source, Change)
	Change.AttackPower = -Level

	return Change
end

-- Flayed  --

Buff_Flayed = Base_Buff:New()

function Buff_Flayed.GetInfo(self, Level)
	return "Resistances reduced by [c green]" .. Level .. "%"
end

function Buff_Flayed.Stats(self, Level, Source, Change)
	Change.AllResist = -Level

	return Change
end

-- Fractured  --

Buff_Fractured = Base_Buff:New()

function Buff_Fractured.GetInfo(self, Level)
	return "Armor reduced by [c green]" .. Level
end

function Buff_Fractured.Stats(self, Level, Source, Change)
	Change.Armor = -Level

	return Change
end

-- Light  --

Buff_Light = Base_Buff:New()

function Buff_Light.GetInfo(self, Level)
	return "Giving off light"
end

function Buff_Light.Stats(self, Level, Source, Change)
	Change.Light = Level

	return Change
end

-- Shielded --

Buff_Shielded = Base_Buff:New()

function Buff_Shielded.GetInfo(self, Level)
	return "Blocking [c green]" .. Level .. "[c white] damage before breaking"
end

function Buff_Shielded.OnHit(self, Object, Effect, Change, Result)

	-- Check for incoming damage
	if Change.Damage > 0 then
		Change.BuffSound = self.ID
	end

	-- Reduce shield
	Level = Effect.Level - Change.Damage
	Duration = Effect.Duration

	-- Check for shield breaking
	if Level <= 0 then
		Change.Damage = math.abs(Level)
		Object.UpdateBuff(self.ID, 0, 0)
	else
		Change.Damage = 0
		Object.UpdateBuff(self.ID, Level, Duration)
	end
end

function Buff_Shielded.PlaySound(self, Level)
	Audio.Play("absorb" .. Random.GetInt(0, 1) .. ".ogg", 0.5)
end

-- Empowered --

Buff_Empowered = Base_Buff:New()

function Buff_Empowered.GetInfo(self, Level)
	return "Attack damage increased by [c green]" .. Level .. "%"
end

function Buff_Empowered.Stats(self, Level, Source, Change)
	Change.AttackPower = Level

	return Change
end

-- Sanctuary --

Buff_Sanctuary = Base_Buff:New()
Buff_Sanctuary.Heal = 2.5
Buff_Sanctuary.Armor = 1
Buff_Sanctuary.DamageBlock = 2

function Buff_Sanctuary.GetInfo(self, Level)
	return "Armor increased by [c green]" .. self.Armor * Level .. "[c white]\nDamage block increased by [c green]" .. self.DamageBlock * Level .. "[c white]\nSlowly healing for [c green]" .. math.floor(self.Heal * Level) .. " [c white]HP"
end

function Buff_Sanctuary.Stats(self, Level, Source, Change)
	Change.Armor = self.Armor * Level
	Change.DamageBlock = self.DamageBlock * Level

	return Change
end

function Buff_Sanctuary.Update(self, Level, Source, Change)
	Change.Health = math.floor(self.Heal * Level)

	return Change
end

-- Demons --

Buff_SummonDemon = Base_Buff:New()

function Buff_SummonDemon.GetInfo(self, Level)
	Plural = ""
	if Level ~= 1 then
		Plural = "s"
	end

	return "Carrying [c green]" .. Level .. "[c white] demon" .. Plural
end

function Buff_SummonDemon.GetSummonSkill(self)
	return Skill_DemonicConjuring.Item.Pointer
end

-- Ice Imp --

Buff_SummonIceImp = Base_Buff:New()

function Buff_SummonIceImp.GetInfo(self, Level)
	Plural = ""
	if Level ~= 1 then
		Plural = "s"
	end

	return "Carrying [c green]" .. Level .. "[c white] ice imp" .. Plural
end

function Buff_SummonIceImp.GetSummonSkill(self)
	return Skill_DemonicConjuring.Item.Pointer
end

-- Skeletons --

Buff_SummonSkeleton = Base_Buff:New()

function Buff_SummonSkeleton.GetInfo(self, Level)
	Plural = ""
	if Level ~= 1 then
		Plural = "s"
	end

	return "Carrying [c green]" .. Level .. "[c white] skeleton" .. Plural
end

function Buff_SummonSkeleton.GetSummonSkill(self)
	return Skill_RaiseDead.Item.Pointer
end

-- Skeleton Priests --

Buff_SummonSkeletonPriest = Base_Buff:New()

function Buff_SummonSkeletonPriest.GetInfo(self, Level)
	Plural = ""
	if Level ~= 1 then
		Plural = "s"
	end

	return "Carrying [c green]" .. Level .. "[c white] skeleton priest" .. Plural
end

function Buff_SummonSkeletonPriest.GetSummonSkill(self)
	return Skill_RaiseDead.Item.Pointer
end

-- Difficult --

Buff_Difficult = Base_Buff:New()

function Buff_Difficult.GetInfo(self, Level)
	return "Difficulty increased by [c green]" .. Level .. "%"
end

function Buff_Difficult.Stats(self, Level, Source, Change)
	Change.Difficulty = Level

	return Change
end

-- Attractant --

Buff_Attractant = Base_Buff:New()

function Buff_Attractant.GetInfo(self, Level)
	return "Getting into battle more often"
end

function Buff_Attractant.Stats(self, Level, Source, Change)
	Change.Attractant = Level

	return Change
end

-- Lava Immune --

Buff_LavaImmune = Base_Buff:New()

function Buff_LavaImmune.GetInfo(self, Level)
	return "Immune to lava"
end

function Buff_LavaImmune.Stats(self, Level, Source, Change)
	Change.LavaProtection = true

	return Change
end

-- Warm --

Buff_Warm = Base_Buff:New()

function Buff_Warm.GetInfo(self, Level)
	return "Immune to freezing"
end

function Buff_Warm.Stats(self, Level, Source, Change)
	Change.FreezeProtection = true

	if Source.Light == 0 then
		Change.Light = 31
	end

	return Change
end
