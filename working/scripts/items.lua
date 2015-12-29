-- Healing Salve  --

Item_HealingSalve = { HealBase = 3, Duration = 5 }

function Item_HealingSalve.GetInfo(self, Level)

	return "Restore [c green]" .. Level * self.HealBase * self.Duration .. "[c white] HP over [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_HealingSalve.CanUse(self, Level, Object)

	return true
end

function Item_HealingSalve.Use(self, Level, Source, Target, Result)

	Result.Buff = Buffs["Buff_Healing"]
	Result.BuffLevel = Level
	Result.BuffDuration = self.Duration

	return Result
end

-- Mana Cider --

Item_ManaCider = { ManaBase = 1, Duration = 10 }

function Item_ManaCider.GetInfo(self, Level)

	return "Restore [c light_blue]" .. Level * self.ManaBase * self.Duration .. " [c white]MP over [c green]" .. self.Duration .. " [c white]seconds"
end

function Item_ManaCider.CanUse(self, Level, Object)

	return true
end

function Item_ManaCider.Use(self, Level, Source, Target, Result)

	Result.Buff = Buffs["Buff_Mana"]
	Result.BuffLevel = Level
	Result.BuffDuration = self.Duration

	return Result
end

-- Invis Potion --

Item_InvisPotion = { }

function Item_InvisPotion.GetInfo(self, Level)

	return "Turn invisible and avoid combat for [c_green]" .. Level .. " [c_white]seconds"
end

function Item_InvisPotion.CanUse(self, Level, Object)

	return true
end

function Item_InvisPotion.Use(self, Level, Source, Target, Result)

	Result.Buff = Buffs["Buff_Invis"]
	Result.BuffLevel = 1
	Result.BuffDuration = Level

	return Result
end

-- Haste Potion --

Item_HastePotion = { Duration = 10 }

function Item_HastePotion.GetInfo(self, Level)

	return "Increase battle speed by [c_green]" .. Level * 10 .. "% [c_white] for [c_green]" .. self.Duration .. " [c_white]seconds"
end

function Item_HastePotion.CanUse(self, Level, Object)

	return true
end

function Item_HastePotion.Use(self, Level, Source, Target, Result)

	Result.Buff = Buffs["Buff_Hasted"]
	Result.BuffLevel = Level
	Result.BuffDuration = self.Duration

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
