
Base_Set = {
	UpgradeRate = 0.1,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetInfo = function(self, Source, Item)

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

			UpgradedValue = self:GetUpgradedValue(Key, Value, Item.Upgrades)

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

	Stats = function(self, Object, Upgrades, Count, Change)
		for Key, Value in pairs(self.Attributes) do
			Change[Key] = self:GetUpgradedValue(Key, Value, Upgrades)
		end

		return Change
	end
}

Set_Mage = Base_Set:New()
Set_Mage.UpgradeRate = 0.1
Set_Mage.Attributes = {
	AllSkills           = "1",
	ElementalResistance = "20%",
	MaxMana             = "75",
	ManaRegen           = "2",
}

Set_Wizard = Base_Set:New()
Set_Wizard.UpgradeRate = 0.05
Set_Wizard.Attributes = {
	AllSkills           = "2",
	ElementalResistance = "35%",
	MaxMana             = "150",
	ManaRegen           = "5",
}
