
-- Base Potion class --

Base_Potion = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	PlaySound = function(self, Level)
		Audio.Play("open" .. Random.GetInt(0, 2) .. ".ogg")
	end
}

-- Healing Salve  --

Item_HealingSalve = Base_Potion:New()

function Item_HealingSalve.GetInfo(self, Source, Item)
	return "Restore [c green]" .. math.floor(Item.Level * Buff_Healing.Heal * Item.Duration * Source.HealPower + 0.001) .. "[c white] HP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_HealingSalve.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Healing.Pointer
	Result.Target.BuffLevel = Level * Source.HealPower
	Result.Target.BuffDuration = Duration

	return Result
end

-- Mana Cider --

Item_ManaCider = Base_Potion:New()

function Item_ManaCider.GetInfo(self, Source, Item)
	return "Restore [c light_blue]" .. math.floor(Item.Level * Buff_Mana.Mana * Item.Duration * Source.ManaPower + 0.001) .. " [c white]MP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_ManaCider.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Mana.Pointer
	Result.Target.BuffLevel = Level * Source.ManaPower
	Result.Target.BuffDuration = Duration

	return Result
end

-- Invis Potion --

Item_InvisPotion = Base_Potion:New()

function Item_InvisPotion.GetInfo(self, Source, Item)
	return "Turn invisible and avoid combat for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_InvisPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Invis.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Haste Potion --

Item_HastePotion = Base_Potion:New()

function Item_HastePotion.GetInfo(self, Source, Item)
	return "Increase battle speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_HastePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Hasted.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Death Potion --

Item_DeathPotion = Base_Potion:New()

function Item_DeathPotion.GetInfo(self, Source, Item)
	return "Do not drink"
end

function Item_DeathPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Bleeding.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Battle Potion --

Item_BattlePotion = Base_Potion:New()

function Item_BattlePotion.GetInfo(self, Source, Item)
	return "Get into a fight"
end

function Item_BattlePotion.CanUse(self, Level, Source, Target)
	Zone = Source.GetTileZone(Source.X, Source.Y)
	if Zone > 0 then
		return true
	end

	return false
end

function Item_BattlePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Battle = Source.GetTileZone(Source.X, Source.Y)

	return Result
end

-- Poison Potion --

Item_PoisonPotion = Base_Potion:New()

function Item_PoisonPotion.GetInfo(self, Source, Item)
	return "Poison a target for [c green]" .. math.floor(Buff_Poisoned.Damage * Item.Level * Item.Duration * Source.PoisonPower) .. "[c white] damage over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_PoisonPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level * Source.PoisonPower
	Result.Target.BuffDuration = Duration

	return Result
end

-- Action Slot --

Item_ActionSlot = { }

function Item_ActionSlot.GetInfo(self, Source, Item)
	return "Increase your action bar size\n[c yellow]Can only be used once"
end

function Item_ActionSlot.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.ActionBarSize = 1

	return Result
end

-- Skill Point --

Item_SkillPoint = { }

function Item_SkillPoint.GetInfo(self, Source, Item)

	Plural = ""
	if Item.Level ~= 1 then
		Plural = "s"
	end

	return "Grants [c_green]" .. Item.Level .. "[c_white] extra skill point" .. Plural .. "\n[c yellow]Can only be used once"
end

function Item_SkillPoint.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.SkillPoint = Level

	return Result
end

-- Throwing Knives --

Item_ThrowingKnives = Base_Attack:New()

function Item_ThrowingKnives.GenerateDamage(self, Level, Source)
	return self.Item.GenerateDamage(Source.Pointer, 0)
end

function Item_ThrowingKnives.GetPierce(self, Source)
	return self.Item.Pierce
end

function Item_ThrowingKnives.GetInfo(self, Source, Item)
	return "Throw a knife at your enemy"
end

function Item_ThrowingKnives.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Poison Knives --

Item_PoisonKnives = Base_Attack:New()

function Item_PoisonKnives.GetInfo(self, Source, Item)
	return "Throw a poison-tipped knife at your enemy"
end

function Item_PoisonKnives.GetPierce(self, Source)
	return self.Item.Pierce
end

function Item_PoisonKnives.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level * Source.PoisonPower
	Result.Target.BuffDuration = Duration

	return true
end

function Item_PoisonKnives.GenerateDamage(self, Level, Source)
	return self.Item.GenerateDamage(Source.Pointer, 0)
end

function Item_PoisonKnives.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Slimy Glob --

Item_SlimyGlob = { }

function Item_SlimyGlob.GetInfo(self, Source, Item)

	return "Gain [c green]" .. Item.Level .. "% [c yellow]bleed [c white]resist for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_SlimyGlob.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_BleedResist.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_SlimyGlob.PlaySound(self, Level)
	Audio.Play("slime" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crow Feather --

Item_CrowFeather = { }

function Item_CrowFeather.GetInfo(self, Source, Item)

	return "Increase move speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_CrowFeather.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Fast.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_CrowFeather.PlaySound(self, Level)
	Audio.Play("swoop" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Fire Dust --

Item_FireDust = { }

function Item_FireDust.GetInfo(self, Source, Item)

	return "Reduce target's hit chance by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_FireDust.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Blinded.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_FireDust.PlaySound(self, Level)
end

-- Spider Leg --

Item_SpiderLeg = { }

function Item_SpiderLeg.GetInfo(self, Source, Item)

	return "Increase battle speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_SpiderLeg.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Hasted.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_SpiderLeg.PlaySound(self, Level)
	Audio.Play("spider0.ogg")
end

-- Fang --

Item_Fang = { }

function Item_Fang.GetInfo(self, Source, Item)

	return "Increase damage by [c_green]" .. Item.Level .. " [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_Fang.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Mighty.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_Fang.PlaySound(self, Level)
	Audio.Play("bat0.ogg")
end

-- Spectral Dust --

Item_SpectralDust = { }

function Item_SpectralDust.GetInfo(self, Source, Item)

	return "Increase evasion by [c green]" .. Item.Level .. "% [c white] for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_SpectralDust.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Evasion.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_SpectralDust.PlaySound(self, Level)
	Audio.Play("ghost" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crab Legs --

Item_CrabLegs = { }

function Item_CrabLegs.GetInfo(self, Source, Item)

	return "Gain [c green]" .. Item.Level .. " [c white]armor for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_CrabLegs.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Hardened.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_CrabLegs.PlaySound(self, Level)
	Audio.Play("crab" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Teleport Scroll --

Item_TeleportScroll = { }

function Item_TeleportScroll.GetInfo(self, Source, Item)

	return "Teleport home after [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_TeleportScroll.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Teleport = Duration

	return Result
end

-- Swamp Glob --

Item_SwampGlob = { }

function Item_SwampGlob.GetInfo(self, Source, Item)

	return "Slow target by [c green]" .. Item.Level .. "% [c white]for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_SwampGlob.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_SwampGlob.PlaySound(self, Level)
	Audio.Play("sludge0.ogg")
end

-- Lava Sludge --

Item_LavaSludge = { }

function Item_LavaSludge.GetInfo(self, Source, Item)
	return "Ignite a target for [c green]" .. math.floor(Buff_Burning.Damage * Item.Level * Item.Duration * Source.FirePower) .. "[c white] damage over [c green]" .. Item.Duration .. " [c white]seconds\n\n[c red]Damages yourself when used"
end

function Item_LavaSludge.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = Level * Source.FirePower
	Result.Target.BuffDuration = Duration

	Result.Source.Buff = Buff_Burning.Pointer
	Result.Source.BuffLevel = math.max(1, math.floor(Level / 2))
	Result.Source.BuffDuration = Duration

	return Result
end

function Item_LavaSludge.PlaySound(self, Level)
	Audio.Play("flame0.ogg")
end

-- Torch --

Item_Torch = { }

function Item_Torch.GetDuration(self, Source, Duration)
	return math.floor(Duration * Source.FirePower)
end

function Item_Torch.GetInfo(self, Source, Item)
	return "Give light for [c green]" .. self:GetDuration(Source, Item.Duration) .. " [c white]seconds"
end

function Item_Torch.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Light.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self:GetDuration(Source, Duration)

	return Result
end

function Item_Torch.PlaySound(self, Level)
	Audio.Play("flame0.ogg")
end

-- BrightPotion --

Item_BrightPotion = { }

function Item_BrightPotion.GetInfo(self, Source, Item)

	return "Turn the world to daylight"
end

function Item_BrightPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Clock = Level

	return Result
end

-- DarkPotion --

Item_DarkPotion = { }

function Item_DarkPotion.GetInfo(self, Source, Item)

	return "Turn the world to darkness"
end

function Item_DarkPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Clock = Level

	return Result
end

-- RespecPotion --

Item_RespecPotion = { }

function Item_RespecPotion.GetInfo(self, Source, Item)

	return "Reset your spent skill points\n[c yellow]Action bar skills are set to level 1"
end

function Item_RespecPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Respec = 1

	return Result
end
