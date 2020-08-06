
Base_Set = {
	UpgradeRate = 0.05,

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetInfo = function(self, Source, Item)
		return "[c light_green]Added " .. self.Item.Set.Name .. " Set Bonus\n" .. self:GetSetInfo(Source, Item.SetLevel)
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

Set_Cloth = Base_Set:New()
Set_Cloth.Attributes = {
	BattleSpeed = "10%",
	BleedPower = "25%",
	BleedResist = "10%",
	Evasion = "5%",
	PoisonResist = "20%",
}

Set_Black = Base_Set:New()
Set_Black.Attributes = {
	BattleSpeed = "15%",
	BleedPower = "50%",
	BleedResist = "17%",
	Evasion = "10%",
	PoisonResist = "35%",
}

Set_Mage = Base_Set:New()
Set_Mage.Attributes = {
	AllSkills = "1",
	ElementalResist = "20%",
	ManaRegen = "2",
	MaxMana = "75",
}

Set_Wizard = Base_Set:New()
Set_Wizard.Attributes = {
	AllSkills = "2",
	ElementalResist = "35%",
	ManaRegen = "5",
	MaxMana = "150",
}

Set_Leather = Base_Set:New()
Set_Leather.Attributes = {
	Armor = "5",
	BleedResist = "20%",
	ColdResist = "20%",
	MaxHealth = "75",
	PhysicalPower = "35%",
}

Set_ReinforcedLeather = Base_Set:New()
Set_ReinforcedLeather.Attributes = {
	Armor = "10",
	BleedResist = "35%",
	ColdResist = "35%",
	MaxHealth = "150",
	PhysicalPower = "50%",
}

Set_Bronze = Base_Set:New()
Set_Bronze.Attributes = {
	Armor = "10",
	AllResist = "10%",
	DamageBlock = "20",
	MaxHealth = "100",
}

Set_Iron = Base_Set:New()
Set_Iron.Attributes = {
	Armor = "20",
	AllResist = "15%",
	DamageBlock = "40",
	MaxHealth = "200",
}

-- Added Bonus --
-- Names need to start with SetBonus for the tooltip description to work correctly --

SetBonus_Stick = Base_Set:New()
SetBonus_Stick.Attributes = {
	SpellDamage = "10%",
}

SetBonus_MageBook = Base_Set:New()
SetBonus_MageBook.Attributes = {
	ManaPower = "10%",
}

SetBonus_LeatherBuckler = Base_Set:New()
SetBonus_LeatherBuckler.Attributes = {
	DamageBlock = "10",
}

SetBonus_ShortSword = Base_Set:New()
SetBonus_ShortSword.Attributes = {
	AttackPower = "15%",
}

SetBonus_Sword = Base_Set:New()
SetBonus_Sword.Attributes = {
	AttackPower = "20%",
}
