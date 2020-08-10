-- Base Proc --

Base_Proc = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetChance = function(self, Item)
		return math.floor(Item.Chance + Item.Upgrades * self.ChancePerLevel)
	end,

	GetLevel = function(self, Source, Item)
		Level = math.floor(Item.Level + Item.Upgrades * self.LevelPerLevel)
		if self.MaxLevel ~= nil then
			Level = math.min(Level, self.MaxLevel)
		end

		return Level
	end,

	GetDuration = function(self, Item)
		return math.floor(10 * (Item.Duration + Item.Upgrades * self.DurationPerLevel)) / 10.0
	end,

	GetTotal = function(self, Source, Item)
		return math.floor(self:GetLevel(Source, Item) * self:GetDuration(Item))
	end,

	GetDamageText = function(self, Source, Item)
		if Item.MoreInfo == true then
			Text = "[c green]" .. self:GetLevel(Source, Item) .. "[c white] " .. self.DamageTypeName .. " DPS"
		else
			Text = "[c green]" .. self:GetTotal(Source, Item) .. "[c white] " .. self.DamageTypeName .. " damage over [c green]" .. self:GetDuration(Item) .. "[c white] seconds"
		end

		return Text
	end,

	AddBuff = function(self, Change, Buff, Source, Item, Level)

		-- Get stats from item
		Duration = self:GetDuration(Item)

		-- Check for existing buff
		if Change.Buff == nil then
			Change.Buff = Buff
			Change.BuffLevel = Level
			Change.BuffDuration = Duration
			return
		end

		-- Buff exists, but is different
		if Change.Buff ~= Buff then
			return
		end

		if self.Additive == true then

			-- Add to buff level
			Change.BuffLevel = Change.BuffLevel + Level
			if Duration >= Change.BuffDuration then
				Change.BuffDuration = Duration
			end
		else

			-- Take better level or duration
			if Level > Change.BuffLevel then
				Change.BuffLevel = Level
			elseif Level == Change.BuffLevel and Duration > Change.BuffDuration then
				Change.BuffDuration = Duration
			end
		end
	end,

	Proc = function(self, Roll, Item, Source, Target, Result)
		if Roll <= self:GetChance(Item) then
			if self.OnSelf == true then
				self:AddBuff(Result.Source, self.Buff.Pointer, Source, Item, self:GetLevel(Source, Item))
			else
				self:AddBuff(Result.Target, self.Buff.Pointer, Source, Item, self:GetLevel(Source, Item))
			end

			return true
		end

		return false
	end,

	Buff = nil,
	OnSelf = false,
	SpellOnly = false,
	MaxLevel = nil,
	Additive = false,
	ChancePerLevel = 0,
	LevelPerLevel = 0,
	DurationPerLevel = 0,
	DamageTypeName = "",
}

-- Stun --

Proc_Stun = Base_Proc:New()
Proc_Stun.Buff = Buff_Stunned
Proc_Stun.MaxChance = 75
Proc_Stun.ChancePerLevel = 1
Proc_Stun.LevelPerLevel = 0
Proc_Stun.DurationPerLevel = 0.05

function Proc_Stun.GetChance(self, Item)
	return math.min(math.floor(Item.Chance + Item.Upgrades * self.ChancePerLevel), self.MaxChance)
end

function Proc_Stun.GetDuration(self, Item)
	return Item.Duration + math.floor(10 * Item.Upgrades * self.DurationPerLevel) / 10
end

function Proc_Stun.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance to stun for [c green]" .. self:GetDuration(Item) .. "[c white] seconds"
end

-- Poison --

Proc_Poison = Base_Proc:New()
Proc_Poison.Buff = Buff_Poisoned
Proc_Poison.ChancePerLevel = 1
Proc_Poison.PercentPerLevel = 10
Proc_Poison.DurationPerLevel = 0
Proc_Poison.DamageTypeName = "poison"

function Proc_Poison.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance for " .. self:GetDamageText(Source, Item)
end

function Proc_Poison.GetLevel(self, Source, Item)
	return math.floor((Item.Level + Item.Level * Item.Upgrades * self.PercentPerLevel * 0.01) * Source.PoisonPower * 0.01)
end

-- Slow --

Proc_Slow = Base_Proc:New()
Proc_Slow.Buff = Buff_Slowed
Proc_Slow.ChancePerLevel = 1
Proc_Slow.LevelPerLevel = 1
Proc_Slow.DurationPerLevel = 0.1
Proc_Slow.MaxLevel = 75

function Proc_Slow.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance to slow target by [c green]" .. self:GetLevel(Source, Item) .. "%[c white] for [c green]" .. self:GetDuration(Item) .. "[c white] seconds"
end

-- Ignite --

Proc_Ignite = Base_Proc:New()
Proc_Ignite.Buff = Buff_Burning
Proc_Ignite.ChancePerLevel = 1
Proc_Ignite.PercentPerLevel = 10
Proc_Ignite.DurationPerLevel = 0
Proc_Ignite.DamageTypeName = "fire"

function Proc_Ignite.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance for " .. self:GetDamageText(Source, Item)
end

function Proc_Ignite.GetLevel(self, Source, Item)
	return math.floor((Item.Level + Item.Level * Item.Upgrades * self.PercentPerLevel * 0.01) * Source.FirePower * 0.01)
end

-- Bleed --

Proc_Bleed = Base_Proc:New()
Proc_Bleed.Buff = Buff_Bleeding
Proc_Bleed.Additive = true
Proc_Bleed.ChancePerLevel = 1
Proc_Bleed.PercentPerLevel = 10
Proc_Bleed.DurationPerLevel = 0
Proc_Bleed.DamageTypeName = "bleeding"

function Proc_Bleed.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance for " .. self:GetDamageText(Source, Item)
end

function Proc_Bleed.GetLevel(self, Source, Item)
	return math.floor((Item.Level + Item.Level * Item.Upgrades * self.PercentPerLevel * 0.01) * Source.BleedPower * 0.01)
end

-- Bloodlet --

Proc_Bloodlet = Base_Proc:New()
Proc_Bloodlet.Additive = true
Proc_Bloodlet.ChancePerLevel = 1
Proc_Bloodlet.PercentPerLevel = 10
Proc_Bloodlet.DurationPerLevel = 0
Proc_Bloodlet.HealMultiplier = 0.25
Proc_Bloodlet.DamageTypeName = "bleeding"

function Proc_Bloodlet.GetInfo(self, Source, Item)
	Chance = self:GetChance(Item)

	return "[c green]" .. Chance .. "%[c white] chance for " .. self:GetDamageText(Source, Item) .. " and " .. self:GetHealingText(Source, Item)
end

function Proc_Bloodlet.GetHealingText(self, Source, Item)
	if Item.MoreInfo == true then
		Text = "[c green]" .. self:GetHealingLevel(Source, Item) .. "[c white] HPS"
	else
		Healing = self:GetHealingTotal(Source, Item)
		Duration = self:GetDuration(Item)
		Text = "[c green]" .. Healing .. "[c white] healing over [c green]" .. Duration .. "[c white] seconds"
	end

	return Text
end

function Proc_Bloodlet.GetHealingTotal(self, Source, Item)
	return math.floor(self:GetHealingLevel(Source, Item) * self:GetDuration(Item))
end

function Proc_Bloodlet.GetHealingLevel(self, Source, Item)
	return math.floor((Item.Level + Item.Level * Item.Upgrades * self.PercentPerLevel * 0.01) * Source.HealPower * 0.01 * self.HealMultiplier)
end

function Proc_Bloodlet.GetLevel(self, Source, Item)
	return math.floor((Item.Level + Item.Level * Item.Upgrades * self.PercentPerLevel * 0.01) * Source.BleedPower * 0.01)
end

function Proc_Bloodlet.Proc(self, Roll, Item, Source, Target, Result)
	if Roll <= self:GetChance(Item) then
		self:AddBuff(Result.Target, Buff_Bleeding.Pointer, Source, Item, self:GetLevel(Source, Item))
		self:AddBuff(Result.Source, Buff_Healing.Pointer, Source, Item, self:GetHealingLevel(Source, Item))

		return true
	end

	return false
end

-- Haste --

Proc_Haste = Base_Proc:New()
Proc_Haste.Buff = Buff_Hasted
Proc_Haste.OnSelf = true
Proc_Haste.ChancePerLevel = 1
Proc_Haste.LevelPerLevel = 0.2
Proc_Haste.DurationPerLevel = 0.05

function Proc_Haste.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance for a [c green]" .. self:GetLevel(Source, Item) .. "%[c white] speed boost for [c green]" .. self:GetDuration(Item) .. "[c white] seconds"
end

-- Harden --

Proc_Harden = Base_Proc:New()
Proc_Harden.Buff = Buff_Hardened
Proc_Harden.OnSelf = true
Proc_Harden.ChancePerLevel = 1
Proc_Harden.LevelPerLevel = 1
Proc_Harden.DurationPerLevel = 0.1

function Proc_Harden.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance for a [c green]" .. self:GetLevel(Source, Item) .. "[c white] armor buff for [c green]" .. self:GetDuration(Item) .. "[c white] seconds"
end

-- Blind --

Proc_Blind = Base_Proc:New()
Proc_Blind.Buff = Buff_Blinded
Proc_Blind.ChancePerLevel = 1
Proc_Blind.LevelPerLevel = 1
Proc_Blind.DurationPerLevel = 0.1

function Proc_Blind.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance to inflict [c green]" .. self:GetLevel(Source, Item) .. "%[c white] blindness for [c green]" .. self:GetDuration(Item) .. "[c white] seconds"
end

-- Empowered --

Proc_Empowered = Base_Proc:New()
Proc_Empowered.Buff = Buff_Empowered
Proc_Empowered.OnSelf = true
Proc_Empowered.ChancePerLevel = 0.5
Proc_Empowered.LevelPerLevel = 0.2
Proc_Empowered.DurationPerLevel = 0.1

function Proc_Empowered.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance for a [c green]" .. self:GetLevel(Source, Item) .. "%[c white] attack damage buff for [c green]" .. self:GetDuration(Item) .. "[c white] seconds"
end

-- Sanctuary --

Proc_Sanctuary = Base_Proc:New()
Proc_Sanctuary.Buff = Buff_Sanctuary
Proc_Sanctuary.OnSelf = true
Proc_Sanctuary.ChancePerLevel = 1
Proc_Sanctuary.LevelPerLevel = 0.5
Proc_Sanctuary.DurationPerLevel = 0

function Proc_Sanctuary.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance for a level [c green]" .. self:GetLevel(Source, Item) .. "[c white] [c yellow]sanctuary[c white] buff for [c green]" .. self:GetDuration(Item) .. "[c white] seconds"
end

-- Health --

Proc_Health = Base_Proc:New()
Proc_Health.ChancePerLevel = 1
Proc_Health.LevelPerLevel = 5
Proc_Health.DurationPerLevel = 0

function Proc_Health.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance to restore [c green]" .. self:GetLevel(Source, Item) .. "[c white] HP"
end

function Proc_Health.GetLevel(self, Source, Item)
	return math.floor((Item.Level + math.floor(Item.Upgrades * self.LevelPerLevel)) * Source.HealPower * 0.01)
end

function Proc_Health.Proc(self, Roll, Item, Source, Target, Result)
	if Roll <= self:GetChance(Item) then
		if Result.Source.Health == nil then
			Result.Source.Health = self:GetLevel(Source, Item)
		else
			Result.Source.Health = Result.Source.Health + self:GetLevel(Source, Item)
		end
	end

	return false
end

-- Mana --

Proc_Mana = Base_Proc:New()
Proc_Mana.ChancePerLevel = 1
Proc_Mana.LevelPerLevel = 5
Proc_Mana.DurationPerLevel = 0
Proc_Mana.SpellOnly = true

function Proc_Mana.GetInfo(self, Source, Item)
	return "[c green]" .. self:GetChance(Item) .. "%[c white] chance to restore [c blue]" .. self:GetLevel(Source, Item) .. "[c white] MP on spell use"
end

function Proc_Mana.GetLevel(self, Source, Item)
	return math.floor((Item.Level + math.floor(Item.Upgrades * self.LevelPerLevel)) * Source.ManaPower * 0.01)
end

function Proc_Mana.Proc(self, Roll, Item, Source, Target, Result)
	if Roll <= self:GetChance(Item) then
		if Result.Source.Mana == nil then
			Result.Source.Mana = self:GetLevel(Source, Item)
		else
			Result.Source.Mana = Result.Source.Mana + self:GetLevel(Source, Item)
		end
	end

	return false
end
