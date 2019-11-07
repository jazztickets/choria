
-- Base Potion Class --
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

-- Healing Salve --

Item_HealingSalve = Base_Potion:New()

function Item_HealingSalve.GetInfo(self, Item)

	return "Restore [c green]" .. math.floor(Item.Level * Buff_Healing.Heal * Item.Duration) .. "[c white] HP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_HealingSalve.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Healing.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Mana Cider --

Item_ManaCider = Base_Potion:New()

function Item_ManaCider.GetInfo(self, Item)

	return "Restore [c light_blue]" .. math.floor(Item.Level * Buff_Mana.Mana * Item.Duration) .. " [c white]MP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_ManaCider.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Mana.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Invis Potion --

Item_InvisPotion = Base_Potion:New()

function Item_InvisPotion.GetInfo(self, Item)

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

function Item_HastePotion.GetInfo(self, Item)

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

function Item_DeathPotion.GetInfo(self, Item)

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

function Item_BattlePotion.GetInfo(self, Item)

	return "Get into a fight"
end

function Item_BattlePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Battle = "test"

	return Result
end

-- Poison Potion --

Item_PoisonPotion = Base_Potion:New()

function Item_PoisonPotion.GetInfo(self, Item)

	return "Poison target for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_PoisonPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Action Slot --

Item_ActionSlot = { }

function Item_ActionSlot.GetInfo(self, Item)

	return "Increase your action bar size"
end

function Item_ActionSlot.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.ActionBarSize = 1

	return Result
end

-- Throwing Knives --

Item_ThrowingKnives = Base_Attack:New()

function Item_ThrowingKnives.GenerateDamage(self, Level, Source)

	return self.Item.GenerateDamage()
end

function Item_ThrowingKnives.GetInfo(self, Item)

	return "Throw a knife at your enemy"
end

function Item_ThrowingKnives.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Poison Knives --

Item_PoisonKnives = Base_Attack:New()

function Item_PoisonKnives.GetInfo(self, Item)

	return "Throw a poison-tipped knife at your enemy"
end

function Item_PoisonKnives.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
end

function Item_PoisonKnives.GenerateDamage(self, Level, Source)

	return self.Item.GenerateDamage()
end

function Item_PoisonKnives.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Slimy Glob --

Item_SlimyGlob = { }

function Item_SlimyGlob.GetInfo(self, Item)

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

function Item_CrowFeather.GetInfo(self, Item)

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

function Item_FireDust.GetInfo(self, Item)

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

function Item_SpiderLeg.GetInfo(self, Item)

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

function Item_Fang.GetInfo(self, Item)

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

function Item_SpectralDust.GetInfo(self, Item)

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

function Item_CrabLegs.GetInfo(self, Item)

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

function Item_TeleportScroll.GetInfo(self, Item)

	return "Teleport home after [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_TeleportScroll.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Teleport = Duration

	return Result
end

-- Swamp Glob --

Item_SwampGlob = { }

function Item_SwampGlob.GetInfo(self, Item)

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

-- Torch --

Item_Torch = { }

function Item_Torch.GetInfo(self, Item)

	return "Give light for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_Torch.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Light.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_Torch.PlaySound(self, Level)
	Audio.Play("flame0.ogg")
end

-- BrightPotion --

Item_BrightPotion = { }

function Item_BrightPotion.GetInfo(self, Item)

	return "Turn the world to daylight"
end

function Item_BrightPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Clock = Level

	return Result
end

-- DarkPotion --

Item_DarkPotion = { }

function Item_DarkPotion.GetInfo(self, Item)

	return "Turn the world to darkness"
end

function Item_DarkPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Clock = Level

	return Result
end
