Script_heal = {}

function Script_heal.Activate(self, Health, Mana, Object, Change)
	Change.Health = Health
	Change.Mana = Mana

	return Change
end

Script_boss = {}

function Script_boss.Activate(self, Zone, Object, Change)
	Change.Battle = Zone

	return Change
end
