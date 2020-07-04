
-- Base Potion class --

Base_Potion = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	PlaySound = function(self, Level)
		Audio.Play("open" .. Random.GetInt(0, 2) .. ".ogg")
	end
}

-- Healing Salve  --

Item_HealingSalve = Base_Potion:New()

function Item_HealingSalve.GetInfo(self, Source, Item)
	return "Restore [c green]" .. math.floor(math.floor(Buff_Healing.Heal * Item.Level * Source.HealPower) * Item.Duration) .. "[c white] HP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_HealingSalve.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Healing.Pointer
	Result.Target.BuffLevel = Level * Source.HealPower
	Result.Target.BuffDuration = Duration

	return Result
end

-- Mana Cider --

Item_ManaCider = Base_Potion:New()

function Item_ManaCider.GetInfo(self, Source, Item)
	return "Restore [c light_blue]" .. math.floor(math.floor(Buff_Mana.Mana * Item.Level * Source.ManaPower) * Item.Duration) .. " [c white]MP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_ManaCider.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Mana.Pointer
	Result.Target.BuffLevel = Level * Source.ManaPower
	Result.Target.BuffDuration = Duration

	return Result
end

-- Invis Potion --

Item_InvisPotion = Base_Potion:New()

function Item_InvisPotion.GetInfo(self, Source, Item)
	return "Turn invisible and avoid combat for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_InvisPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Invis.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Haste Potion --

Item_HastePotion = Base_Potion:New()

function Item_HastePotion.GetInfo(self, Source, Item)
	return "Increase battle speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_HastePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Hasted.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Death Potion --

Item_DeathPotion = Base_Potion:New()

function Item_DeathPotion.GetInfo(self, Source, Item)
	return "Do not drink"
end

function Item_DeathPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Bleeding.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Battle Potion --

Item_BattlePotion = Base_Potion:New()

function Item_BattlePotion.GetInfo(self, Source, Item)
	return "Get into a fight"
end

function Item_BattlePotion.CanUse(self, Level, Source, Target)
	Zone = Source.GetTileZone(Source.X, Source.Y)
	if Zone > 0 then
		return true
	end

	return false
end

function Item_BattlePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Battle = Source.GetTileZone(Source.X, Source.Y)

	return Result
end

-- Poison Potion --

Item_PoisonPotion = Base_Potion:New()

function Item_PoisonPotion.GetInfo(self, Source, Item)
	return "Poison a target for [c green]" .. math.floor(math.floor(Item.Level * Source.PoisonPower) * Item.Duration) .. "[c white] damage over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_PoisonPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level * Source.PoisonPower
	Result.Target.BuffDuration = Duration

	return Result
end

-- Action Slot --

Item_ActionSlot = { }

function Item_ActionSlot.GetInfo(self, Source, Item)
	return "Increase your action bar size\n[c yellow]Can only be used once"
end

function Item_ActionSlot.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.ActionBarSize = 1

	return Result
end

-- Skill Point --

Item_SkillPoint = { }

function Item_SkillPoint.GetInfo(self, Source, Item)

	Plural = ""
	if Item.Level ~= 1 then
		Plural = "s"
	end

	return "Grants [c_green]" .. Item.Level .. "[c_white] extra skill point" .. Plural .. "\n[c yellow]Can only be used once"
end

function Item_SkillPoint.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.SkillPoint = Level

	return Result
end

-- Throwing Knives --

Item_ThrowingKnives = Base_Attack:New()

function Item_ThrowingKnives.GenerateDamage(self, Level, Source)
	return self.Item.GenerateDamage(Source.Pointer, 0)
end

function Item_ThrowingKnives.GetPierce(self, Source)
	return self.Item.Pierce
end

function Item_ThrowingKnives.GetInfo(self, Source, Item)
	return "Throw a knife at your enemy"
end

function Item_ThrowingKnives.Use(self, Level, Duration, Source, Target, Result)
	Battle_ResolveDamage(self, Level, Source, Target, Result)

	return Result
end

function Item_ThrowingKnives.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Poison Knives --

Item_PoisonKnives = Base_Attack:New()

function Item_PoisonKnives.GetInfo(self, Source, Item)
	return "Throw a poison-tipped knife at your enemy, causing [c green]" .. math.floor(math.floor(Item.Level * Source.PoisonPower) * Item.Duration) .. "[c white] poison damage over [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_PoisonKnives.GetPierce(self, Source)
	return self.Item.Pierce
end

function Item_PoisonKnives.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level * Source.PoisonPower
	Result.Target.BuffDuration = Duration

	return true
end

function Item_PoisonKnives.GenerateDamage(self, Level, Source)
	return self.Item.GenerateDamage(Source.Pointer, 0)
end

function Item_PoisonKnives.Use(self, Level, Duration, Source, Target, Result)
	Battle_ResolveDamage(self, Level, Source, Target, Result)
	self:Proc(1, Level, Duration, Source, Target, Result)

	return Result
end

function Item_PoisonKnives.PlaySound(self, Level)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Slimy Glob --

Item_SlimyGlob = { }

function Item_SlimyGlob.GetInfo(self, Source, Item)
	return "Purge bleeding and gain [c green]" .. Item.Level .. "% [c yellow]bleed [c white]resist for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_SlimyGlob.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_BleedResist.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Bleeding.Pointer

	return Result
end

function Item_SlimyGlob.PlaySound(self, Level)
	Audio.Play("slime" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crow Feather --

Item_CrowFeather = { }

function Item_CrowFeather.GetInfo(self, Source, Item)
	return "Purge slowness and increase move speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_CrowFeather.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Fast.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Slowed.Pointer

	return Result
end

function Item_CrowFeather.PlaySound(self, Level)
	Audio.Play("swoop" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Fire Dust --

Item_FireDust = { }

function Item_FireDust.GetInfo(self, Source, Item)
	return "Reduce target's hit chance by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_FireDust.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Blinded.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_FireDust.PlaySound(self, Level)
end

-- Spider Leg --

Item_SpiderLeg = { }

function Item_SpiderLeg.GetInfo(self, Source, Item)
	return "Increase battle speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_SpiderLeg.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Hasted.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_SpiderLeg.PlaySound(self, Level)
	Audio.Play("spider0.ogg")
end

-- Fang --

Item_Fang = { }

function Item_Fang.GetInfo(self, Source, Item)
	return "Increase attack damage by [c_green]" .. Item.Level .. " [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_Fang.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Mighty.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_Fang.PlaySound(self, Level)
	Audio.Play("bat0.ogg")
end

-- Spectral Dust --

Item_SpectralDust = { }

function Item_SpectralDust.GetInfo(self, Source, Item)
	return "Increase evasion by [c green]" .. Item.Level .. "% [c white] for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_SpectralDust.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Evasion.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_SpectralDust.PlaySound(self, Level)
	Audio.Play("ghost" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crab Legs --

Item_CrabLegs = { }

function Item_CrabLegs.GetInfo(self, Source, Item)
	return "Gain [c green]" .. Item.Level .. " [c white]armor for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_CrabLegs.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Hardened.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_CrabLegs.PlaySound(self, Level)
	Audio.Play("crab" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Teleport Scroll --

Item_TeleportScroll = { }

function Item_TeleportScroll.GetInfo(self, Source, Item)
	return "Teleport home after [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_TeleportScroll.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Teleport = Duration

	return Result
end

-- Swamp Glob --

Item_SwampGlob = { }

function Item_SwampGlob.GetInfo(self, Source, Item)
	return "Purge burning and slow target by [c green]" .. Item.Level .. "% [c white]for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_SwampGlob.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Burning.Pointer

	return Result
end

function Item_SwampGlob.PlaySound(self, Level)
	Audio.Play("sludge0.ogg")
end

-- Lava Sludge --

Item_LavaSludge = { }

function Item_LavaSludge.GetInfo(self, Source, Item)
	return "Purge weakness and ignite a target for [c green]" .. math.floor(math.floor(Item.Level * Source.FirePower) * Item.Duration) .. "[c white] damage over [c green]" .. Item.Duration .. " [c white]seconds\n\n[c red]Damages yourself when used"
end

function Item_LavaSludge.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = Level * Source.FirePower
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Weak.Pointer

	Result.Source.Buff = Buff_Burning.Pointer
	Result.Source.BuffLevel = math.max(1, math.floor(Level / 2))
	Result.Source.BuffDuration = Duration
	Result.Source.ClearBuff = Buff_Weak.Pointer

	return Result
end

function Item_LavaSludge.PlaySound(self, Level)
	Audio.Play("flame0.ogg")
end

-- Bone --

Item_Bone = { }

function Item_Bone.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Level .. "%[c white] chance to stun for [c green]" .. Item.Duration .. "[c white] seconds when thrown at your enemy"
end

function Item_Bone.Use(self, Level, Duration, Source, Target, Result)
	if Random.GetInt(1, 100) <= Level then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = Duration
		return Result
	end

	return Result
end

function Item_Bone.PlaySound(self, Level)
	Audio.Play("bash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Stinger --

Item_Stinger = { }

function Item_Stinger.GetInfo(self, Source, Item)
	return "Increase attack power by [c_green]" .. Item.Level .. "%[c_white] for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_Stinger.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Empowered.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Torch --

Item_Torch = { }

function Item_Torch.GetDuration(self, Source, Duration)
	return math.floor(Duration * Source.FirePower)
end

function Item_Torch.GetInfo(self, Source, Item)
	return "Give light for [c green]" .. self:GetDuration(Source, Item.Duration) .. " [c white]seconds"
end

function Item_Torch.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Light.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self:GetDuration(Source, Duration)

	return Result
end

function Item_Torch.PlaySound(self, Level)
	Audio.Play("flame0.ogg")
end

-- Bright Potion --

Item_BrightPotion = { }

function Item_BrightPotion.GetInfo(self, Source, Item)
	return "Turn the world to daylight"
end

function Item_BrightPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Clock = Level

	return Result
end

-- Dark Potion --

Item_DarkPotion = { }

function Item_DarkPotion.GetInfo(self, Source, Item)
	return "Turn the world to darkness"
end

function Item_DarkPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Clock = Level

	return Result
end

-- Respec Potion --

Item_RespecPotion = { }

function Item_RespecPotion.GetInfo(self, Source, Item)
	return "Reset your spent skill points\n[c yellow]Action bar skills are set to level 1"
end

function Item_RespecPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Respec = 1

	return Result
end

-- Diagonal --

Item_DiagonalMovement = { }

function Item_DiagonalMovement.GetInfo(self, Source, Item)
	return "[c yellow]Allows for diagonal movement"
end

function Item_DiagonalMovement.Stats(self, Level, Object, Change)
	Change.DiagonalMovement = 1

	return Change
end

-- Rebirth --
function RebirthText(UpgradeText, Skills, Items)
	return "[c gray]Sacrifice everything to rebirth anew\n\nLose all items, equipment, keys, gold, experience and skills for:\n\nPermanent " .. UpgradeText .. "\n\nYou will keep your starting skills, plus [c green]" .. Skills .. "[c white] of your highest skills and [c green]" .. Items .. "[c white] of your items in your trade stash\n[c yellow]All unlocks are kept"
end

Item_RebirthStrength = { Value = 10 }

function Item_RebirthStrength.GetInfo(self, Source, Item)
	return RebirthText("[c green]" .. self.Value .. "%[c white] damage bonus", Source.RebirthSaveCount, Source.RebirthSaveCount)
end

function Item_RebirthStrength.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.MaxDamage = self.Value

	return Result
end

Item_RebirthGuard = { Value = 5 }

function Item_RebirthGuard.GetInfo(self, Source, Item)
	return RebirthText("[c green]" .. self.Value .. "[c white] armor, damage block, and resistance bonus", Source.RebirthSaveCount, Source.RebirthSaveCount)
end

function Item_RebirthGuard.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Armor = self.Value

	return Result
end

Item_RebirthFortitude = { Value = 10 }

function Item_RebirthFortitude.GetInfo(self, Source, Item)
	return RebirthText("[c green]" .. self.Value .. "%[c white] max health and heal power bonus", Source.RebirthSaveCount, Source.RebirthSaveCount)
end

function Item_RebirthFortitude.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Health = self.Value

	return Result
end

Item_RebirthSpirit = { Value = 10 }

function Item_RebirthSpirit.GetInfo(self, Source, Item)
	return RebirthText("[c green]" .. self.Value .. "%[c white] max mana and mana power bonus", Source.RebirthSaveCount, Source.RebirthSaveCount)
end

function Item_RebirthSpirit.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Mana = self.Value

	return Result
end

Item_RebirthWisdom = { Value = 10 }

function Item_RebirthWisdom.GetInfo(self, Source, Item)
	return RebirthText("[c green]" .. self.Value .. "%[c white] experience bonus", Source.RebirthSaveCount, Source.RebirthSaveCount)
end

function Item_RebirthWisdom.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Experience = self.Value

	return Result
end

Item_RebirthWealth = { Value = 10 }

function Item_RebirthWealth.GetInfo(self, Source, Item)
	return RebirthText("[c green]" .. self.Value .. "%[c white] gold bonus", Source.RebirthSaveCount, Source.RebirthSaveCount)
end

function Item_RebirthWealth.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Gold = self.Value

	return Result
end

Item_RebirthAlacrity = { Value = 5 }

function Item_RebirthAlacrity.GetInfo(self, Source, Item)
	return RebirthText("[c green]" .. self.Value .. "%[c white] battle speed bonus", Source.RebirthSaveCount, Source.RebirthSaveCount)
end

function Item_RebirthAlacrity.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.BattleSpeed = self.Value

	return Result
end

Item_RebirthKnowledge = { Value = 1 }

function Item_RebirthKnowledge.GetInfo(self, Source, Item)
	return RebirthText("[c green]" .. self.Value .. "[c white] extra skill point", Source.RebirthSaveCount, Source.RebirthSaveCount)
end

function Item_RebirthKnowledge.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.SkillPoint = self.Value

	return Result
end