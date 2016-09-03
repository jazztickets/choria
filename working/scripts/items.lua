-- Healing Salve  --

Item_HealingSalve = { HealBase = 3, Duration = 5 }

function Item_HealingSalve.GetInfo(self, Level)

	return "Restore [c green]" .. Level * self.HealBase * self.Duration .. "[c white] HP over [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_HealingSalve.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Healing"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Mana Cider --

Item_ManaCider = { ManaBase = 1, Duration = 10 }

function Item_ManaCider.GetInfo(self, Level)

	return "Restore [c light_blue]" .. Level * self.ManaBase * self.Duration .. " [c white]MP over [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_ManaCider.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Mana"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Invis Potion --

Item_InvisPotion = { }

function Item_InvisPotion.GetInfo(self, Level)

	return "Turn invisible and avoid combat for [c_green]" .. Level .. " [c_white]seconds"
end

function Item_InvisPotion.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Invis"]
	Result.Target.BuffLevel = 1
	Result.Target.BuffDuration = Level

	return Result
end

-- Haste Potion --

Item_HastePotion = { Duration = 10 }

function Item_HastePotion.GetInfo(self, Level)

	return "Increase battle speed by [c_green]" .. Level * 10 .. "% [c_white]for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_HastePotion.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Hasted"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Death Potion --

Item_DeathPotion = { Duration = 10 }

function Item_DeathPotion.GetInfo(self, Level)

	return "Do not drink"
end

function Item_DeathPotion.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Bleeding"]
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

-- Glob  --

Item_Glob = { Duration = 30 }

function Item_Glob.GetInfo(self, Level)

	return "Gain [c green]" .. Buff_BleedResist.Increments * Level .. "% [c yellow]bleed [c white]resist for [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_Glob.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_BleedResist"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Crow Feather --

Item_CrowFeather = { Duration = 30 }

function Item_CrowFeather.GetInfo(self, Level)

	return "Increase move speed by [c_green]" .. Level * 10 .. "% [c_white]for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_CrowFeather.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Fast"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Spider Leg --

Item_SpiderLeg = { Duration = 30 }

function Item_SpiderLeg.GetInfo(self, Level)

	return "Increase battle speed by [c_green]" .. Level * 10 .. "% [c_white]for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_SpiderLeg.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Hasted"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Fang --

Item_Fang = { Duration = 30 }

function Item_Fang.GetInfo(self, Level)

	return "Increase damage by [c_green]" .. Level .. " [c_white]for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_Fang.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Mighty"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Crab Legs --

Item_CrabLegs = { Duration = 30 }

function Item_CrabLegs.GetInfo(self, Level)

	return "Gain [c green]1 [c white]defense for [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_CrabLegs.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Hardened"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end

-- Battle Potion --

Item_BattlePotion = { }

function Item_BattlePotion.GetInfo(self, Level)

	return "Get into a fight"
end

function Item_BattlePotion.Use(self, Level, Source, Target, Result)
	Result.Target.Battle = Level

	return Result
end

-- Poison Potion --

Item_PoisonPotion = { Duration = 30 }

function Item_PoisonPotion.GetInfo(self, Level)

	return "Do not drink"
end

function Item_PoisonPotion.Use(self, Level, Source, Target, Result)

	Result.Target.Buff = Buffs["Buff_Poisoned"]
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self.Duration

	return Result
end
