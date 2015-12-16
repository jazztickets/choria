-- Health Potion --

Item_HealthPotion = { HealBase = 3, Duration = 5 }

function Item_HealthPotion.GetInfo(Level)

	return "Restore " .. Level * Item_HealthPotion.HealBase * Item_HealthPotion.Duration .. " health over " .. Item_HealthPotion.Duration .. " seconds"
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
