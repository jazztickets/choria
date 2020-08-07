
Base_Set = {

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
			Value = self.Attributes[Key][1]
			Scale = self.Attributes[Key][2]
			PercentPosition = string.find(Value, "%%")
			Percent = ""
			if PercentPosition ~= nil then
				Percent = "%"
			end

			UpgradedValue = self:GetUpgradedValue(Key, Value, Scale, Upgrades)

			Sign = "+"
			if UpgradedValue < 0 then
				Sign = "-"
			end
			Text = Text .. "[c white]" .. Label .. "[c white] " .. Sign .. UpgradedValue .. Percent .. "\n"
		end

		return Text
	end,

	GetUpgradedValue = function(self, Key, Value, Scale, Upgrades)
		UpgradedValue = string.gsub(Value, "%%", "")
		UpgradedValue = tonumber(UpgradedValue)
		UpgradedValue = math.floor(UpgradedValue + UpgradedValue * Scale * Upgrades)

		return UpgradedValue
	end,

	SetStats = function(self, Object, Upgrades, Count, Change)
		for Key, Value in pairs(self.Attributes) do
			Change[Key] = self:GetUpgradedValue(Key, Value[1], Value[2], Upgrades)
		end

		return Change
	end
}

-- Base Bonus

Set_Cloth = Base_Set:New()
Set_Cloth.Attributes = {
	BattleSpeed = { "10%", 1 },
	BleedPower = { "25%", 1 },
	BleedResist = { "10%", 1 },
	Evasion = { "5%", 1 },
	PoisonResist = { "20%", 1 },
}

Set_Black = Base_Set:New()
Set_Black.Attributes = {
	BattleSpeed = { "15%", 1 },
	BleedPower = { "50%", 1 },
	BleedResist = { "17%", 1 },
	Evasion = { "10%", 1 },
	PoisonResist = { "35%", 1 },
}

Set_Mage = Base_Set:New()
Set_Mage.Attributes = {
	AllSkills = { "1", 0.1 },
	ElementalResist = { "20%", 0.05 },
	ManaRegen = { "2", 0.1 },
	MaxMana = { "75", 0.1 },
}

Set_Wizard = Base_Set:New()
Set_Wizard.Attributes = {
	AllSkills = { "2", 1 },
	ElementalResist = { "35%", 1 },
	ManaRegen = { "5", 1 },
	MaxMana = { "150", 1 },
}

Set_Leather = Base_Set:New()
Set_Leather.Attributes = {
	Armor = { "5", 1 },
	BleedResist = { "20%", 1 },
	ColdResist = { "20%", 1 },
	MaxHealth = { "75", 1 },
	PhysicalPower = { "35%", 1 },
}

Set_ReinforcedLeather = Base_Set:New()
Set_ReinforcedLeather.Attributes = {
	Armor = { "10", 1 },
	BleedResist = { "35%", 1 },
	ColdResist = { "35%", 1 },
	MaxHealth = { "150", 1 },
	PhysicalPower = { "50%", 1 },
}

Set_Bronze = Base_Set:New()
Set_Bronze.Attributes = {
	Armor = { "10", 1 },
	AllResist = { "10%", 1 },
	DamageBlock = { "20", 1 },
	MaxHealth = { "100", 1 },
}

Set_Iron = Base_Set:New()
Set_Iron.Attributes = {
	Armor = { "20", 1 },
	AllResist = { "15%", 1 },
	DamageBlock = { "40", 1 },
	MaxHealth = { "200", 1 },
}

-- Added Bonus --
-- Names need to start with SetBonus for the tooltip description to work correctly --

-- One-Handed Weapons --

SetBonus_Stick = Base_Set:New()
SetBonus_Stick.Attributes = {
	SpellDamage = { "10%", 0.25 },
}

SetBonus_MagicStick = Base_Set:New()
SetBonus_MagicStick.Attributes = {
	SpellDamage = { "20%", 1 },
}

SetBonus_ArcaneWand = Base_Set:New()
SetBonus_ArcaneWand.Attributes = {
	SpellDamage = { "50%", 1 },
}

SetBonus_ShortSword = Base_Set:New()
SetBonus_ShortSword.Attributes = {
	AttackPower = { "15%", 1 },
}

SetBonus_Sword = Base_Set:New()
SetBonus_Sword.Attributes = {
	AttackPower = { "20%", 1 },
}

SetBonus_SpiderSword = Base_Set:New()
SetBonus_SpiderSword.Attributes = {
	BattleSpeed = { "20%", 1 },
}

SetBonus_Scimitar = Base_Set:New()
SetBonus_Scimitar.Attributes = {
	AttackPower = { "30%", 1 },
}

SetBonus_VenomBlade = Base_Set:New()
SetBonus_VenomBlade.Attributes = {
	AttackPower = { "30%", 1 },
	PoisonPower = { "25%", 1 },
}

SetBonus_SilverEdge = Base_Set:New()
SetBonus_SilverEdge.Attributes = {
	AttackPower = { "50%", 1 },
}

SetBonus_LightBlade = Base_Set:New()
SetBonus_LightBlade.Attributes = {
	AttackPower = { "50%", 1 },
}

SetBonus_Flamuss = Base_Set:New()
SetBonus_Flamuss.Attributes = {
	AttackPower = { "50%", 1 },
	FirePower = { "50%", 1 },
}

SetBonus_Mace = Base_Set:New()
SetBonus_Mace.Attributes = {
	AttackPower = { "25%", 1 },
}

SetBonus_MorningStar = Base_Set:New()
SetBonus_MorningStar.Attributes = {
	AttackPower = { "50%", 1 },
}

SetBonus_FlangedMace = Base_Set:New()
SetBonus_FlangedMace.Attributes = {
	AttackPower = { "75%", 1 },
}

SetBonus_ShiningStar = Base_Set:New()
SetBonus_ShiningStar.Attributes = {
	AttackPower = { "100%", 1 },
}

-- Two-Handed Weapons --

SetBonus_QuarterStaff = Base_Set:New()
SetBonus_QuarterStaff.Attributes = {
	HealPower = { "25%", 1 },
}

SetBonus_HolyStaff = Base_Set:New()
SetBonus_HolyStaff.Attributes = {
	HealPower = { "50%", 1 },
}

SetBonus_LightStaff = Base_Set:New()
SetBonus_LightStaff.Attributes = {
	HealPower = { "100%", 1 },
}

SetBonus_MysticStaff = Base_Set:New()
SetBonus_MysticStaff.Attributes = {
	HealPower = { "50%", 1 },
	Evasion = { "5%", 1 },
}

SetBonus_DarkStaff = Base_Set:New()
SetBonus_DarkStaff.Attributes = {
	PetPower = { "25%", 1 },
}

SetBonus_DemonStick = Base_Set:New()
SetBonus_DemonStick.Attributes = {
	PetPower = { "50%", 1 },
}

SetBonus_DiabolicStaff = Base_Set:New()
SetBonus_DiabolicStaff.Attributes = {
	PetPower = { "100%", 1 },
}

SetBonus_Axe = Base_Set:New()
SetBonus_Axe.Attributes = {
	AttackPower = { "25%", 1 },
}

SetBonus_BattleAxe = Base_Set:New()
SetBonus_BattleAxe.Attributes = {
	AttackPower = { "35%", 1 },
}

SetBonus_Claymore = Base_Set:New()
SetBonus_Claymore.Attributes = {
	AttackPower = { "50%", 1 },
}

SetBonus_Greatsword = Base_Set:New()
SetBonus_Greatsword.Attributes = {
	AttackPower = { "75%", 1 },
}

SetBonus_Icebrand = Base_Set:New()
SetBonus_Icebrand.Attributes = {
	AttackPower = { "100%", 1 },
}

-- Off-Hand Weapons --

SetBonus_SmallKnife = Base_Set:New()
SetBonus_SmallKnife.Attributes = {
	BleedPower = { "15%", 1 },
}

SetBonus_Dagger = Base_Set:New()
SetBonus_Dagger.Attributes = {
	BleedPower = { "25%", 1 },
}

SetBonus_Stiletto = Base_Set:New()
SetBonus_Stiletto.Attributes = {
	BleedPower = { "40%", 1 },
}

SetBonus_SwiftKnife = Base_Set:New()
SetBonus_SwiftKnife.Attributes = {
	BleedPower = { "75%", 1 },
}

SetBonus_MoonBlade = Base_Set:New()
SetBonus_MoonBlade.Attributes = {
	BleedPower = { "100%", 1 },
}

SetBonus_Bloodletter = Base_Set:New()
SetBonus_Bloodletter.Attributes = {
	BleedPower = { "25%", 1 },
	HealPower = { "25%", 1 },
}

-- Shields --

SetBonus_Gauntlet = Base_Set:New()
SetBonus_Gauntlet.Attributes = {
	AttackPower = { "10%", 1 },
	DamageBlock = { "5", 1 },
}

SetBonus_MageBook = Base_Set:New()
SetBonus_MageBook.Attributes = {
	ManaPower = { "10%", 0.25 },
}

SetBonus_WizardBook = Base_Set:New()
SetBonus_WizardBook.Attributes = {
	ManaPower = { "30%", 1 },
}

SetBonus_ArcaneBook = Base_Set:New()
SetBonus_ArcaneBook.Attributes = {
	ManaPower = { "50%", 1 },
}

SetBonus_LeatherBuckler = Base_Set:New()
SetBonus_LeatherBuckler.Attributes = {
	DamageBlock = { "10", 1 },
}

SetBonus_ReinforcedBuckler = Base_Set:New()
SetBonus_ReinforcedBuckler.Attributes = {
	DamageBlock = { "20", 1 },
}

SetBonus_BronzeShield = Base_Set:New()
SetBonus_BronzeShield.Attributes = {
	DamageBlock = { "20", 1 },
}

SetBonus_IronShield = Base_Set:New()
SetBonus_IronShield.Attributes = {
	DamageBlock = { "35", 1 },
}

SetBonus_SteelShield = Base_Set:New()
SetBonus_SteelShield.Attributes = {
	DamageBlock = { "50", 1 },
}
