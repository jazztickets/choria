
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
			SortedAttributes[Value[1]] = { Value[2], Value[3] }
		end

		Text = ""
		for Key, Value in ipairs(SortedAttributes) do
			Text = Text .. "[c white]" .. Value[1] .. "[c white] " .. Value[2] .. "\n"
		end

		return Text
	end,

	Stats = function(self, Object, Upgrades, Count, Change)
		for Key, Value in pairs(self.Attributes) do
			Number = string.gsub(Value[3], "%%", "")
			Change[Key] = tonumber(Number)
		end

		return Change
	end
}

-- Mage Set --

Set_Mage = Base_Set:New()
Set_Mage.Attributes = {
	AllSkills           = { 1, "All Skills",               "+1"   },
	SpellDamage         = { 2, "Spell Damage",             "+25%" },
	ElementalResistance = { 3, "Elemental Resistance",     "+20%" },
	MaxMana             = { 4, "Max Mana",                 "+75"  },
	ManaRegen           = { 5, "Mana Regen",               "+2"   },
}
