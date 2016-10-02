
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

-- Healing Salve  --

Item_HealingSalve = Base_Potion:New()
Item_HealingSalve.Duration = 5

function Item_HealingSalve.GetInfo(self, Level)

	return "Restore [c green]" .. Level * Buff_Healing.Heal * self.Duration .. "[c white] HP over [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_HealingSalve.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Healing.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Mana Cider --

Item_ManaCider = Base_Potion:New()
Item_ManaCider.Duration = 10

function Item_ManaCider.GetInfo(self, Level)

	return "Restore [c light_blue]" .. Level * Buff_Mana.Mana * self.Duration .. " [c white]MP over [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_ManaCider.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Mana.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Invis Potion --

Item_InvisPotion = Base_Potion:New()

function Item_InvisPotion.GetInfo(self, Level)

	return "Turn invisible and avoid combat for [c_green]" .. Level .. " [c_white]seconds"
end

function Item_InvisPotion.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Invis.Pointer
	Result.Target.BuffLevel = 1
	Result.Target.BuffDuration = Level

	return Result
end

-- Haste Potion --

Item_HastePotion = Base_Potion:New()
Item_HastePotion.Duration = 10

function Item_HastePotion.GetInfo(self, Level)

	return "Increase battle speed by [c_green]" .. Level .. "% [c_white]for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_HastePotion.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Hasted.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Death Potion --

Item_DeathPotion = Base_Potion:New()
Item_DeathPotion.Duration = 10

function Item_DeathPotion.GetInfo(self, Level)

	return "Do not drink"
end

function Item_DeathPotion.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Bleeding.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Battle Potion --

Item_BattlePotion = Base_Potion:New()

function Item_BattlePotion.GetInfo(self, Level)

	return "Get into a fight"
end

function Item_BattlePotion.Use(self, Level, Source, Target, Result)
	Result.Target.Battle = Level

	return Result
end

-- Poison Potion --

Item_PoisonPotion = Base_Potion:New()
Item_PoisonPotion.Duration = 10

function Item_PoisonPotion.GetInfo(self, Level)

	return "Poison target for [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_PoisonPotion.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Action Slot --

Item_ActionSlot = { }

function Item_ActionSlot.GetInfo(self, Level)

	return "Increase your action bar size"
end

function Item_ActionSlot.Use(self, Level, Source, Target, Result)

	Result.Target.ActionBarSize = 1

	return Result
end

-- Throwing Knives --

Item_ThrowingKnives = Base_Attack:New()

function Item_ThrowingKnives.GenerateDamage(self, Level, Source)

	return self.Item.GenerateDamage()
end

function Item_ThrowingKnives.GetInfo(self, Level)

	return "Throw a knife at your enemy"
end

function Item_ThrowingKnives.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Poison Knives --

Item_PoisonKnives = Base_Attack:New()
Item_PoisonKnives.Duration = 10

function Item_PoisonKnives.GetInfo(self, Level)

	return "Throw a poison-tipped knife at your enemy"
end

function Item_PoisonKnives.Proc(self, Roll, Level, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration
end

function Item_PoisonKnives.GenerateDamage(self, Level, Source)

	return self.Item.GenerateDamage()
end

function Item_PoisonKnives.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Slimy Glob --

Item_SlimyGlob = { Duration = 30 }

function Item_SlimyGlob.GetInfo(self, Level)

	return "Gain [c green]" .. Level .. "% [c yellow]bleed [c white]resist for [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_SlimyGlob.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_BleedResist.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

function Item_SlimyGlob.PlaySound(self, Level)
	Audio.Play("slime" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crow Feather --

Item_CrowFeather = { Duration = 30 }

function Item_CrowFeather.GetInfo(self, Level)

	return "Increase move speed by [c_green]" .. Level .. "% [c_white]for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_CrowFeather.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Fast.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

function Item_CrowFeather.PlaySound(self, Level)
	Audio.Play("swoop" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Spider Leg --

Item_SpiderLeg = { Duration = 30 }

function Item_SpiderLeg.GetInfo(self, Level)

	return "Increase battle speed by [c_green]" .. Level .. "% [c_white]for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_SpiderLeg.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Hasted.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

function Item_SpiderLeg.PlaySound(self, Level)
	Audio.Play("spider0.ogg")
end

-- Fang --

Item_Fang = { Duration = 30 }

function Item_Fang.GetInfo(self, Level)

	return "Increase damage by [c_green]" .. Level .. " [c_white]for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_Fang.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Mighty.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

function Item_Fang.PlaySound(self, Level)
	Audio.Play("bat0.ogg")
end

-- Spectral Dust --

Item_SpectralDust = { Duration = 30 }

function Item_SpectralDust.GetInfo(self, Level)

	return "Increase evasion by [c green]" .. Level .. "% [c white] for [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_SpectralDust.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Evasion.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

function Item_SpectralDust.PlaySound(self, Level)
	Audio.Play("ghost" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crab Legs --

Item_CrabLegs = { Duration = 30 }

function Item_CrabLegs.GetInfo(self, Level)

	return "Gain [c green]" .. Level .. " [c white]armor for [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_CrabLegs.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Hardened.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

function Item_CrabLegs.PlaySound(self, Level)
	Audio.Play("crab" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Teleport Scroll --

Item_TeleportScroll = { Duration = 3 }

function Item_TeleportScroll.GetInfo(self, Level)

	return "Teleport home after [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_TeleportScroll.Use(self, Level, Source, Target, Result)
	Result.Target.Teleport = self.Duration

	return Result
end

-- Swamp Glob --

Item_SwampGlob = { Duration = 10 }

function Item_SwampGlob.GetInfo(self, Level)

	return "Slow target by [c green]" .. Level .. "% [c white]for [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_SwampGlob.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

function Item_SwampGlob.PlaySound(self, Level)
	Audio.Play("sludge0.ogg")
end
