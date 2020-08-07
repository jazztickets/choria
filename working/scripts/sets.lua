
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

-- One-Handed Weapons --

SetBonus_Stick = Base_Set:New()
SetBonus_Stick.Attributes = {
	SpellDamage = "10%",
}

SetBonus_MagicStick = Base_Set:New()
SetBonus_MagicStick.Attributes = {
	SpellDamage = "20%",
}

SetBonus_ArcaneWand = Base_Set:New()
SetBonus_ArcaneWand.Attributes = {
	SpellDamage = "50%",
}

SetBonus_ShortSword = Base_Set:New()
SetBonus_ShortSword.Attributes = {
	AttackPower = "15%",
}

SetBonus_Sword = Base_Set:New()
SetBonus_Sword.Attributes = {
	AttackPower = "20%",
}

SetBonus_SpiderSword = Base_Set:New()
SetBonus_SpiderSword.Attributes = {
	BattleSpeed = "20%",
}

SetBonus_Scimitar = Base_Set:New()
SetBonus_Scimitar.Attributes = {
	AttackPower = "30%",
}

SetBonus_VenomBlade = Base_Set:New()
SetBonus_VenomBlade.Attributes = {
	AttackPower = "30%",
	PoisonPower = "25%",
}

SetBonus_SilverEdge = Base_Set:New()
SetBonus_SilverEdge.Attributes = {
	AttackPower = "50%",
}

SetBonus_LightBlade = Base_Set:New()
SetBonus_LightBlade.Attributes = {
	AttackPower = "50%",
}

SetBonus_Flamuss = Base_Set:New()
SetBonus_Flamuss.Attributes = {
	AttackPower = "50%",
	FirePower = "50%",
}

SetBonus_Mace = Base_Set:New()
SetBonus_Mace.Attributes = {
	AttackPower = "25%",
}

SetBonus_MorningStar = Base_Set:New()
SetBonus_MorningStar.Attributes = {
	AttackPower = "50%",
}

SetBonus_FlangedMace = Base_Set:New()
SetBonus_FlangedMace.Attributes = {
	AttackPower = "75%",
}

SetBonus_ShiningStar = Base_Set:New()
SetBonus_ShiningStar.Attributes = {
	AttackPower = "100%",
}

-- Two-Handed Weapons --

SetBonus_QuarterStaff = Base_Set:New()
SetBonus_QuarterStaff.Attributes = {
	HealPower = "25%",
}

SetBonus_HolyStaff = Base_Set:New()
SetBonus_HolyStaff.Attributes = {
	HealPower = "50%",
}

SetBonus_LightStaff = Base_Set:New()
SetBonus_LightStaff.Attributes = {
	HealPower = "100%",
}

SetBonus_MysticStaff = Base_Set:New()
SetBonus_MysticStaff.Attributes = {
	HealPower = "50%",
	Evasion = "5%",
}

SetBonus_DarkStaff = Base_Set:New()
SetBonus_DarkStaff.Attributes = {
	PetPower = "25%",
}

SetBonus_DemonStick = Base_Set:New()
SetBonus_DemonStick.Attributes = {
	PetPower = "50%",
}

SetBonus_DiabolicStaff = Base_Set:New()
SetBonus_DiabolicStaff.Attributes = {
	PetPower = "100%",
}

SetBonus_Axe = Base_Set:New()
SetBonus_Axe.Attributes = {
	AttackPower = "25%",
}

SetBonus_BattleAxe = Base_Set:New()
SetBonus_BattleAxe.Attributes = {
	AttackPower = "35%",
}

SetBonus_Claymore = Base_Set:New()
SetBonus_Claymore.Attributes = {
	AttackPower = "50%",
}

SetBonus_Greatsword = Base_Set:New()
SetBonus_Greatsword.Attributes = {
	AttackPower = "75%",
}

SetBonus_Icebrand = Base_Set:New()
SetBonus_Icebrand.Attributes = {
	AttackPower = "100%",
}

-- Off-Hand Weapons --

SetBonus_SmallKnife = Base_Set:New()
SetBonus_SmallKnife.Attributes = {
	BleedPower = "15%",
}

SetBonus_Dagger = Base_Set:New()
SetBonus_Dagger.Attributes = {
	BleedPower = "25%",
}

SetBonus_Stiletto = Base_Set:New()
SetBonus_Stiletto.Attributes = {
	BleedPower = "40%",
}

SetBonus_SwiftKnife = Base_Set:New()
SetBonus_SwiftKnife.Attributes = {
	BleedPower = "75%",
}

SetBonus_MoonBlade = Base_Set:New()
SetBonus_MoonBlade.Attributes = {
	BleedPower = "100%",
}

SetBonus_Bloodletter = Base_Set:New()
SetBonus_Bloodletter.Attributes = {
	BleedPower = "25%",
	HealPower = "25%",
}

-- Shields --

SetBonus_Gauntlet = Base_Set:New()
SetBonus_Gauntlet.Attributes = {
	AttackPower = "10%",
	DamageBlock = "5",
}

SetBonus_MageBook = Base_Set:New()
SetBonus_MageBook.Attributes = {
	ManaPower = "10%",
}

SetBonus_WizardBook = Base_Set:New()
SetBonus_WizardBook.Attributes = {
	ManaPower = "30%",
}

SetBonus_ArcaneBook = Base_Set:New()
SetBonus_ArcaneBook.Attributes = {
	ManaPower = "50%",
}

SetBonus_LeatherBuckler = Base_Set:New()
SetBonus_LeatherBuckler.Attributes = {
	DamageBlock = "10",
}

SetBonus_ReinforcedBuckler = Base_Set:New()
SetBonus_ReinforcedBuckler.Attributes = {
	DamageBlock = "20",
}

SetBonus_BronzeShield = Base_Set:New()
SetBonus_BronzeShield.Attributes = {
	DamageBlock = "20",
}

SetBonus_IronShield = Base_Set:New()
SetBonus_IronShield.Attributes = {
	DamageBlock = "35",
}

SetBonus_SteelShield = Base_Set:New()
SetBonus_SteelShield.Attributes = {
	DamageBlock = "50",
}
