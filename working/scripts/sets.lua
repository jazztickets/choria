
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
			Text = Text .. "[c white]" .. AttributeName .. "[c white] " .. Value[2] .. "\n"
		end

		return Text
	end,

	Stats = function(self, Object, Upgrades, Count, Change)
		for Key, Value in pairs(self.Attributes) do
			Number = string.gsub(Value[2], "%%", "")
			Change[Key] = tonumber(Number)
		end

		return Change
	end
}

-- Mage Set --

Set_Mage = Base_Set:New()
Set_Mage.Attributes = {
	AllSkills           = { 1, "+1" },
	SpellDamage         = { 2, "+25%" },
	ElementalResistance = { 3, "+20%" },
	MaxMana             = { 4, "+75" },
	ManaRegen           = { 5, "+2" },
}
