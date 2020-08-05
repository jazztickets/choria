
Base_Set = {
	UpgradeRate = 0.1,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetInfo = function(self, Source, Item)
		SortedAttributes = {}
		for Key, Value in pairs(self.Attributes) do
			SortedAttributes[Value[1]] = { Key, Value[2] }
		end

		Text = ""
		for Key, Value in ipairs(SortedAttributes) do
			AttributeName = Value[1]:gsub("([a-z])([A-Z])", "%1 %2")
			PercentPosition = string.find(Value[2], "%%")
			if PercentPosition ~= nil then
				Percent = "%"
			else
				Percent = ""
			end

			UpgradedValue = self:GetUpgradedValue(Value[1], Value[2], Item.Upgrades)

			Sign = "+"
			if UpgradedValue < 0 then
				Sign = "-"
			end
			Text = Text .. "[c white]" .. AttributeName .. "[c white] " .. Sign .. UpgradedValue .. Percent .. "\n"
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
			Change[Key] = self:GetUpgradedValue(Key, Value[2], Upgrades)
		end

		return Change
	end
}

Set_Mage = Base_Set:New()
Set_Mage.UpgradeRate = 0.1
Set_Mage.Attributes = {
	AllSkills           = { 1, "1"   },
	SpellDamage         = { 2, "25%" },
	ElementalResistance = { 3, "20%" },
	MaxMana             = { 4, "75"  },
	ManaRegen           = { 5, "2"   },
}

Set_Wizard = Base_Set:New()
Set_Wizard.UpgradeRate = 0.05
Set_Wizard.Attributes = {
	AllSkills           = { 1, "2"   },
	SpellDamage         = { 2, "50%" },
	ElementalResistance = { 3, "35%" },
	MaxMana             = { 4, "150" },
	ManaRegen           = { 5, "5"   },
}
