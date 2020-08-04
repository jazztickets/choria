SET_UPGRADE_SCALE = 0.1

-- Base Set --

Base_Set = {

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
		UpgradedValue = math.floor(UpgradedValue + UpgradedValue * UpgradeScale[Key] * Upgrades * SET_UPGRADE_SCALE)

		return UpgradedValue
	end,

	Stats = function(self, Object, Upgrades, Count, Change)
		for Key, Value in pairs(self.Attributes) do
			Change[Key] = self:GetUpgradedValue(Key, Value[2], Upgrades)
		end

		return Change
	end
}

-- Mage Set --

Set_Mage = Base_Set:New()
Set_Mage.Attributes = {
	AllSkills           = { 1, "1" },
	SpellDamage         = { 2, "25%" },
	ElementalResistance = { 3, "20%" },
	MaxMana             = { 4, "75" },
	ManaRegen           = { 5, "2" },
}
