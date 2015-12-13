-- Health Potion --

Item_HealthPotion = { HealBase = 3, Duration = 5 }

function Item_HealthPotion.GetInfo(Level)

	return "Restore " .. Item_HealthPotion.HealBase * Item_HealthPotion.Duration .. " health over time"
end

function Item_HealthPotion.CanUse(Level, Object)
	if Object.Health < Object.MaxHealth then
		return 1
	end

	return 0
end

function Item_HealthPotion.Use(Level, Source, Target, Result)

	Result.TargetHealthChange = Item_HealthPotion.HealBase * 5

	return Result
end
