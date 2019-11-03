Script_heal = {}

function Script_heal.Activate(self, Object, Change, Health, Mana)
	Amount = Health
	Change.Health = Amount
	if Mana ~= nil then
		Amount = Mana
	end

	Change.Mana = Amount

	return Change
end

Script_boss = {}

function Script_boss.Activate(self, Object, Change, Zone)
	Change.Battle = Zone

	return Change
end
