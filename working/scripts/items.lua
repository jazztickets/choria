-- Health Potion --

Item_HealthPotion = { HealBase = 3, Duration = 5 }

function Item_HealthPotion.GetInfo(Level)

	return "Restore [c green]" .. Level * Item_HealthPotion.HealBase * Item_HealthPotion.Duration .. "[c white] HP over [c green]" .. Item_HealthPotion.Duration .. " [c white]seconds"
end

function Item_HealthPotion.CanUse(Level, Object)
	--if Object.Health < Object.MaxHealth then
	--	return 1
	--end

	return 1
end

function Item_HealthPotion.Use(Level, Source, Target, Result)

	Result.Buff = Buffs["Buff_Healing"]
	Result.BuffLevel = Level
	Result.BuffDuration = Item_HealthPotion.Duration

	return Result
end

-- Mana Potion --

Item_ManaPotion = { ManaBase = 1, Duration = 10 }

function Item_ManaPotion.GetInfo(Level)

	return "Restore [c light_blue]" .. Level * Item_ManaPotion.ManaBase * Item_ManaPotion.Duration .. " [c white]MP over [c green]" .. Item_ManaPotion.Duration .. " [c white]seconds"
end

function Item_ManaPotion.CanUse(Level, Object)
	--if Object.Mana < Object.MaxMana then
	--	return 1
	--end

	return 1
end

function Item_ManaPotion.Use(Level, Source, Target, Result)

	Result.Buff = Buffs["Buff_Mana"]
	Result.BuffLevel = Level
	Result.BuffDuration = Item_ManaPotion.Duration

	return Result
end

-- Invis Potion --

Item_InvisPotion = { }

function Item_InvisPotion.GetInfo(Level)

	return "Turn invisible and avoid combat for [c_green]" .. Level .. " [c_white]seconds"
end

function Item_InvisPotion.CanUse(Level, Object)

	return 1
end

function Item_InvisPotion.Use(Level, Source, Target, Result)

	Result.Buff = Buffs["Buff_Invis"]
	Result.BuffLevel = 1
	Result.BuffDuration = Level

	return Result
end
