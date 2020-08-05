
Base_Set = {
	UpgradeRate = 0.1,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetInfo = function(self, Source, Item)
		return "[c light_green]Added Set Bonus\n" .. self:GetSetInfo(Source, Item.SetLevel)
	end,

	GetSetInfo = function(self, Source, Upgrades)

		-- Sort attributes by name
		SortedAttributes = {}
		for Key in pairs(self.Attributes) do
			table.insert(SortedAttributes, Key)
		end
		table.sort(SortedAttributes)

		Text = ""
		for i, Key in pairs(SortedAttributes) do
			Label = Key:gsub("([a-z])([A-Z])", "%1 %2")
			Value = self.Attributes[Key]
			PercentPosition = string.find(Value, "%%")
			Percent = ""
			if PercentPosition ~= nil then
				Percent = "%"
			end

			UpgradedValue = self:GetUpgradedValue(Key, Value, Upgrades)

			Sign = "+"
			if UpgradedValue < 0 then
				Sign = "-"
			end
			Text = Text .. "[c white]" .. Label .. "[c white] " .. Sign .. UpgradedValue .. Percent .. "\n"
		end

		return Text
	end,

	GetUpgradedValue = function(self, Key, Value, Upgrades)
		UpgradedValue = string.gsub(Value, "%%", "")
		UpgradedValue = tonumber(UpgradedValue)
		UpgradedValue = math.floor(UpgradedValue + UpgradedValue * UpgradeScale[Key] * Upgrades * self.UpgradeRate)

		return UpgradedValue
	end,

	SetStats = function(self, Object, Upgrades, Count, Change)
		for Key, Value in pairs(self.Attributes) do
			Change[Key] = self:GetUpgradedValue(Key, Value, Upgrades)
		end

		return Change
	end
}

-- Base Bonus

Set_Mage = Base_Set:New()
Set_Mage.UpgradeRate = 0.1
Set_Mage.Attributes = {
	AllSkills = "1",
	ElementalResist = "20%",
	ManaRegen = "2",
	MaxMana = "75",
}

Set_Wizard = Base_Set:New()
Set_Wizard.UpgradeRate = 0.05
Set_Wizard.Attributes = {
	AllSkills = "2",
	ElementalResist = "35%",
	ManaRegen = "5",
	MaxMana = "150",
}

Set_Leather = Base_Set:New()
Set_Leather.UpgradeRate = 0.05
Set_Leather.Attributes = {
	Armor = "5",
	BleedResist = "20%",
	MaxHealth = "75",
	PhysicalPower = "25%",
}

-- Added Bonus --

SetBonus_Stick = Base_Set:New()
SetBonus_Stick.Attributes = {
	SpellDamage = "10%",
}

SetBonus_MageBook = Base_Set:New()
SetBonus_MageBook.Attributes = {
	SpellDamage = "10%",
}
