-- Base Set --

Base_Set = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetAddedInfo = function(self, Source, Item)
		return "[c light_green]Added " .. self.Item.Set.Name .. " Set Bonus\n" .. self:GetSetInfo(Source, Item.SetLevel, Item.MaxSetLevel, Item.MoreInfo)
	end,

	GetInfo = function(self, Source, Item)
		return self:GetAddedInfo(Source, Item)
	end,

	GetSetInfo = function(self, Source, Upgrades, MaxUpgrades, MoreInfo)

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
			MaxValue = self.Attributes[Key][2]

			-- Show range or upgraded value
			Text = Text .. "[c white]" .. Label .. "[c white] "
			if MoreInfo == true then
				Text = Text .. "[c light_green]" .. self:GetDisplayValue(Key, Value, MaxValue, Upgrades, MaxUpgrades, true) .. "[c white] (" .. Value .. " - " .. MaxValue ..  ")\n"
			else
				Text = Text .. self:GetDisplayValue(Key, Value, MaxValue, Upgrades, MaxUpgrades, false) .. "\n"
			end
		end

		return Text
	end,

	GetDisplayValue = function(self, Key, Value, MaxValue, Upgrades, MaxUpgrades, Fractions)
		UpgradedValue = self:GetUpgradedValue(Key, Value, MaxValue, Upgrades, MaxUpgrades, Fractions)

		Sign = "+"
		if UpgradedValue < 0 then
			Sign = ""
		end

		PercentPosition = string.find(Value, "%%")
		Percent = ""
		if PercentPosition ~= nil then
			Percent = "%"
		end

		return Sign .. UpgradedValue .. Percent
	end,

	GetUpgradedValue = function(self, Key, Value, MaxValue, Upgrades, MaxUpgrades, Fractions)
		Value = string.gsub(Value, "%%", "")
		Value = tonumber(Value)
		if MaxUpgrades == 0 then
			return Value
		end

		MaxValue = string.gsub(MaxValue, "%%", "")
		MaxValue = tonumber(MaxValue)
		Range = MaxValue - Value;

		UpgradeProgression = Upgrades / MaxUpgrades
		UpgradedValue = Value + Range * UpgradeProgression
		if not Fractions then
			UpgradedValue = math.floor(UpgradedValue)
		end

		return UpgradedValue
	end,

	SetStats = function(self, Object, Upgrades, MaxUpgrades, Change)
		for Key, Value in pairs(self.Attributes) do
			Change[Key] = self:GetUpgradedValue(Key, Value[1], Value[2], Upgrades, MaxUpgrades)
		end

		return Change
	end,

	Attributes = {}
}

-- Base Bonus

Set_Cloth = Base_Set:New()
Set_Cloth.Attributes = {
	BattleSpeed = { "5%", "20%" },
	BleedPower = { "10%", "50%" },
	BleedResist = { "10%", "25%" },
	ConsumeChance = { "-5%", "-15%" },
	Evasion = { "5%", "15%" },
	Initiative = { "20%", "40%" },
	MaxHealth = { "10", "100" },
}

Set_Black = Base_Set:New()
Set_Black.Attributes = {
	BattleSpeed = { "15%", "35%" },
	BleedPower = { "25%", "100%" },
	BleedResist = { "15%", "45%" },
	ConsumeChance = { "-10%", "-25%" },
	Evasion = { "10%", "30%" },
	Initiative = { "30%", "50%" },
	MaxHealth = { "50", "200" },
}

Set_Elusive = Base_Set:New()
Set_Elusive.Attributes = {
	BattleSpeed = { "25%", "75%" },
	BleedPower = { "50%", "250%" },
	BleedResist = { "25%", "75%" },
	ConsumeChance = { "-15%", "-50%" },
	Evasion = { "20%", "50%" },
	Initiative = { "40%", "75%" },
	MaxHealth = { "125", "500" },
}

Set_Mage = Base_Set:New()
Set_Mage.Attributes = {
	AllSkills = { "0", "1" },
	ElementalResist = { "5%", "25%" },
	ManaRegen = { "2", "4" },
	MaxHealth = { "5", "50" },
	MaxMana = { "50", "150" },
}

Set_Wizard = Base_Set:New()
Set_Wizard.Attributes = {
	AllSkills = { "0.5", "2" },
	ElementalResist = { "15%", "40%" },
	ManaRegen = { "3", "8" },
	MaxHealth = { "25", "100" },
	MaxMana = { "100", "300" },
}

Set_Arcane = Base_Set:New()
Set_Arcane.Attributes = {
	AllSkills = { "1", "5" },
	Cooldowns = { "-5%", "-25%" },
	ElementalResist = { "25%", "60%" },
	ManaRegen = { "5", "20" },
	MaxHealth = { "50", "250" },
	MaxMana = { "200", "750" },
}

Set_Leather = Base_Set:New()
Set_Leather.Attributes = {
	Armor = { "2", "10" },
	ColdResist = { "10%", "25%" },
	HealPower = { "5%", "15%" },
	MaxHealth = { "25", "250" },
	PhysicalPower = { "10%", "50%" },
}

Set_ReinforcedLeather = Base_Set:New()
Set_ReinforcedLeather.Attributes = {
	Armor = { "6", "20" },
	ColdResist = { "15%", "50%" },
	HealPower = { "10%", "25%" },
	MaxHealth = { "125", "500" },
	PhysicalPower = { "25%", "100%" },
}

Set_Warriors = Base_Set:New()
Set_Warriors.Attributes = {
	Armor = { "13", "45" },
	ColdResist = { "25%", "75%" },
	FireResist = { "15%", "35%" },
	HealPower = { "15%", "75%" },
	MaxHealth = { "250", "1250" },
	PhysicalPower = { "50%", "250%" },
}

Set_Bronze = Base_Set:New()
Set_Bronze.Attributes = {
	Armor = { "5", "15" },
	AllResist = { "10%", "20%" },
	DamageBlock = { "5", "25" },
	MaxHealth = { "50", "400" },
	ShieldDamage = { "10%", "50%" },
}

Set_Iron = Base_Set:New()
Set_Iron.Attributes = {
	Armor = { "10", "30" },
	AllResist = { "15%", "35%" },
	DamageBlock = { "15", "50" },
	MaxHealth = { "200", "800" },
	ShieldDamage = { "25%", "100%" },
}

Set_Steel = Base_Set:New()
Set_Steel.Attributes = {
	Armor = { "20", "75" },
	AllResist = { "20%", "50%" },
	DamageBlock = { "25", "125" },
	HitChance = { "10%", "35%" },
	MaxHealth = { "500", "2000" },
	ShieldDamage = { "50%", "250%" },
}

Set_Dark = Base_Set:New()
Set_Dark.Attributes = {
	SummonLimit = { "1", "2" },
}

Set_Health = Base_Set:New()
Set_Health.Attributes = {
	HealPower = { "10%", "50%" },
}

Set_Magic = Base_Set:New()
Set_Magic.Attributes = {
	ManaPower = { "10%", "50%" },
}

Set_Warm = Base_Set:New()
Set_Warm.Attributes = {
	HealPower = { "25%", "100%" },
	HealthBonus = { "0%", "15%" },
}

Set_Glowing = Base_Set:New()
Set_Glowing.Attributes = {
	ManaPower = { "25%", "100%" },
	ManaBonus = { "0%", "15%" },
}

Set_Trolls = Base_Set:New()
Set_Trolls.Attributes = {
	HealPower = { "50%", "100%" },
	HealthBonus = { "5%", "25%" },
}

Set_Jem = Base_Set:New()
Set_Jem.Attributes = {
	AttackPower = { "25%", "50%" },
	BleedPower = { "25%", "50%" },
	PoisonPower = { "25%", "50%" },
}

Set_Pain = Base_Set:New()
Set_Pain.Attributes = {
	BossCooldowns = { "-10%", "-50%" },
	MonsterCount = { "25%", "50%" },
}

Set_Fire = Base_Set:New()
Set_Fire.Attributes = {
	FirePower = { "25%", "75%" },
}

Set_Cold = Base_Set:New()
Set_Cold.Attributes = {
	ColdPower = { "25%", "75%" },
}

Set_Lightning = Base_Set:New()
Set_Lightning.Attributes = {
	LightningPower = { "25%", "75%" },
}

Set_Poison = Base_Set:New()
Set_Poison.Attributes = {
	PoisonPower = { "25%", "75%" },
}

Set_Bleed = Base_Set:New()
Set_Bleed.Attributes = {
	BleedPower = { "25%", "75%" },
}

Set_Mundane = Base_Set:New()
Set_Mundane.Attributes = {
	PhysicalPower = { "15%", "50%" },
}

-- Added Bonus --
-- Names need to start with SetBonus for the tooltip description to work correctly --

-- One-Handed Weapons --

SetBonus_MageWand = Base_Set:New()
SetBonus_MageWand.Attributes = {
	SpellDamage = { "10%", "25%" },
}

SetBonus_WizardWand = Base_Set:New()
SetBonus_WizardWand.Attributes = {
	SpellDamage = { "15%", "50%" },
}

SetBonus_ArcaneWand = Base_Set:New()
SetBonus_ArcaneWand.Attributes = {
	SpellDamage = { "25%", "125%" },
}

SetBonus_BoneWand = Base_Set:New()
SetBonus_BoneWand.Attributes = {
	PoisonPower = { "50%", "200%" },
	SummonBattleSpeed = { "10%", "25%" },
	SummonLimit = { "0.5", "2" },
}

SetBonus_ShortSword = Base_Set:New()
SetBonus_ShortSword.Attributes = {
	AttackPower = { "15%", "30%" },
}

SetBonus_Sword = Base_Set:New()
SetBonus_Sword.Attributes = {
	AttackPower = { "20%", "35%" },
}

SetBonus_SpiderSword = Base_Set:New()
SetBonus_SpiderSword.Attributes = {
	BattleSpeed = { "10%", "25%" },
}

SetBonus_Scimitar = Base_Set:New()
SetBonus_Scimitar.Attributes = {
	AttackPower = { "25%", "45%" },
}

SetBonus_VenomBlade = Base_Set:New()
SetBonus_VenomBlade.Attributes = {
	AttackPower = { "25%", "45%" },
	PoisonPower = { "25%", "50%" },
}

SetBonus_SilverEdge = Base_Set:New()
SetBonus_SilverEdge.Attributes = {
	AttackPower = { "25%", "75%" },
}

SetBonus_LightBlade = Base_Set:New()
SetBonus_LightBlade.Attributes = {
	AttackPower = { "30%", "100%" },
}

SetBonus_Flamuss = Base_Set:New()
SetBonus_Flamuss.Attributes = {
	AttackPower = { "25%", "75%" },
	FirePower = { "25%", "75%" },
}

SetBonus_Mace = Base_Set:New()
SetBonus_Mace.Attributes = {
	AttackPower = { "20%", "50%" },
}

SetBonus_MorningStar = Base_Set:New()
SetBonus_MorningStar.Attributes = {
	AttackPower = { "25%", "65%" },
}

SetBonus_FlangedMace = Base_Set:New()
SetBonus_FlangedMace.Attributes = {
	AttackPower = { "30%", "80%" },
}

SetBonus_ShiningStar = Base_Set:New()
SetBonus_ShiningStar.Attributes = {
	AttackPower = { "50%", "150%" },
}

-- Two-Handed Weapons --

SetBonus_QuarterStaff = Base_Set:New()
SetBonus_QuarterStaff.Attributes = {
	HealPower = { "15%", "50%" },
}

SetBonus_HolyStaff = Base_Set:New()
SetBonus_HolyStaff.Attributes = {
	HealPower = { "25%", "75%" },
}

SetBonus_LightStaff = Base_Set:New()
SetBonus_LightStaff.Attributes = {
	HealPower = { "50%", "200%" },
	HealthBonus = { "5%", "35%" },
}

SetBonus_MysticStaff = Base_Set:New()
SetBonus_MysticStaff.Attributes = {
	AttackPower = { "25%", "50%" },
	Initiative = { "5%", "25%" },
}

SetBonus_DarkStaff = Base_Set:New()
SetBonus_DarkStaff.Attributes = {
	SummonBattleSpeed = { "5%", "15%" },
	SummonLimit = { "0", "1" },
}

SetBonus_DemonStick = Base_Set:New()
SetBonus_DemonStick.Attributes = {
	SummonBattleSpeed = { "10%", "30%" },
	SummonLimit = { "0.5", "2" },
}

SetBonus_DiabolicStaff = Base_Set:New()
SetBonus_DiabolicStaff.Attributes = {
	SummonBattleSpeed = { "15%", "50%" },
	SummonLimit = { "1", "3" },
}

SetBonus_Axe = Base_Set:New()
SetBonus_Axe.Attributes = {
	AttackPower = { "20%", "40%" },
}

SetBonus_BattleAxe = Base_Set:New()
SetBonus_BattleAxe.Attributes = {
	AttackPower = { "25%", "50%" },
}

SetBonus_Claymore = Base_Set:New()
SetBonus_Claymore.Attributes = {
	AttackPower = { "25%", "60%" },
}

SetBonus_Greatsword = Base_Set:New()
SetBonus_Greatsword.Attributes = {
	AttackPower = { "25%", "75%" },
}

SetBonus_Icebrand = Base_Set:New()
SetBonus_Icebrand.Attributes = {
	AttackPower = { "35%", "100%" },
}

-- Off-Hand Weapons --

SetBonus_SmallKnife = Base_Set:New()
SetBonus_SmallKnife.Attributes = {
	BleedPower = { "15%", "30%" },
}

SetBonus_Dagger = Base_Set:New()
SetBonus_Dagger.Attributes = {
	BleedPower = { "20%", "40%" },
}

SetBonus_Stiletto = Base_Set:New()
SetBonus_Stiletto.Attributes = {
	BleedPower = { "25%", "50%" },
}

SetBonus_SwiftKnife = Base_Set:New()
SetBonus_SwiftKnife.Attributes = {
	BleedPower = { "30%", "75%" },
}

SetBonus_MoonBlade = Base_Set:New()
SetBonus_MoonBlade.Attributes = {
	BleedPower = { "50%", "150%" },
	AttackPower = { "15%", "35%" },
}

SetBonus_Bloodletter = Base_Set:New()
SetBonus_Bloodletter.Attributes = {
	BleedPower = { "25%", "50%" },
	HealPower = { "10%", "25%" },
}

-- Shields --

SetBonus_Gauntlet = Base_Set:New()
SetBonus_Gauntlet.Attributes = {
	AttackPower = { "10%", "25%" },
	DamageBlock = { "5", "25" },
}

SetBonus_MageBook = Base_Set:New()
SetBonus_MageBook.Attributes = {
	ManaPower = { "10%", "25%" },
}

SetBonus_WizardBook = Base_Set:New()
SetBonus_WizardBook.Attributes = {
	ManaPower = { "15%", "50%" },
}

SetBonus_ArcaneBook = Base_Set:New()
SetBonus_ArcaneBook.Attributes = {
	ManaPower = { "25%", "125%" },
}

SetBonus_LeatherBuckler = Base_Set:New()
SetBonus_LeatherBuckler.Attributes = {
	DamageBlock = { "5", "25" },
}

SetBonus_ReinforcedBuckler = Base_Set:New()
SetBonus_ReinforcedBuckler.Attributes = {
	DamageBlock = { "15", "50" },
}

SetBonus_WarriorsBuckler = Base_Set:New()
SetBonus_WarriorsBuckler.Attributes = {
	DamageBlock = { "25", "125" },
}

SetBonus_BronzeShield = Base_Set:New()
SetBonus_BronzeShield.Attributes = {
	DamageBlock = { "10", "50" },
}

SetBonus_IronShield = Base_Set:New()
SetBonus_IronShield.Attributes = {
	DamageBlock = { "25", "100" },
}

SetBonus_SteelShield = Base_Set:New()
SetBonus_SteelShield.Attributes = {
	DamageBlock = { "50", "250" },
}

SetBonus_HandOfZog = Base_Set:New()
SetBonus_HandOfZog.Attributes = {
	ManaShield = { "5%", "25%" },
}

-- Helmet --

SetBonus_Cap = Base_Set:New()
SetBonus_Cap.Attributes = {
	HealPower = { "5%", "25%" },
}

-- Boots --

SetBonus_DimensionalSlippers = Base_Set:New()
SetBonus_DimensionalSlippers.Attributes = {
	Evasion = { "1%", "5%" },
}

SetBonus_LavaBoots = Base_Set:New()
SetBonus_LavaBoots.Attributes = {
	MoveSpeed = { "5%", "15%" },
}
