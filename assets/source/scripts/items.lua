-- Base Potion class --

Base_Potion = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	PlaySound = function(self)
		Audio.Play("open" .. Random.GetInt(0, 2) .. ".ogg")
	end
}

-- Healing Salve  --

Item_HealingSalve = Base_Potion:New()

function Item_HealingSalve.GetInfo(self, Source, Item)
	return "Restore [c green]" .. math.floor(math.floor(Buff_Healing.Heal * Item.Level * Source.HealPower * 0.01) * Item.Duration) .. "[c white] HP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_HealingSalve.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Healing.Pointer
	Result.Target.BuffLevel = Level * Source.HealPower * 0.01
	Result.Target.BuffDuration = Duration

	return Result
end

-- Mana Cider --

Item_ManaCider = Base_Potion:New()

function Item_ManaCider.GetInfo(self, Source, Item)
	return "Restore [c light_blue]" .. math.floor(math.floor(Buff_Mana.Mana * Item.Level * Source.ManaPower * 0.01) * Item.Duration) .. " [c white]MP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_ManaCider.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Mana.Pointer
	Result.Target.BuffLevel = Level * Source.ManaPower * 0.01
	Result.Target.BuffDuration = Duration

	return Result
end

-- Invis Potion --

Item_InvisPotion = Base_Potion:New()

function Item_InvisPotion.GetInfo(self, Source, Item)
	return "Turn invisible and avoid battle for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_InvisPotion.Use(self, Level, Duration, Source, Target, Result, Priority)
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

function Item_HastePotion.Use(self, Level, Duration, Source, Target, Result, Priority)
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

function Item_DeathPotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Bleeding.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Poison Potion --

Item_PoisonPotion = Base_Potion:New()

function Item_PoisonPotion.GetInfo(self, Source, Item)
	return "Poison a target for [c green]" .. math.floor(math.floor(Item.Level * Source.PoisonPower * 0.01) * Item.Duration) .. "[c white] damage over [c green]" .. Item.Duration .. " [c white]seconds\n\n[c yellow]Heal power is reduced when poisoned"
end

function Item_PoisonPotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level * Source.PoisonPower * 0.01
	Result.Target.BuffDuration = Duration

	return Result
end

-- Bright Potion --

Item_BrightPotion = Base_Potion:New()

function Item_BrightPotion.GetInfo(self, Source, Item)
	return "Turn the world to daylight"
end

function Item_BrightPotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Clock = Level

	return Result
end

-- Dark Potion --

Item_DarkPotion = Base_Potion:New()

function Item_DarkPotion.GetInfo(self, Source, Item)
	return "Turn the world to darkness"
end

function Item_DarkPotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Clock = Level

	return Result
end

-- Respec Potion --

Item_RespecPotion = Base_Potion:New()

function Item_RespecPotion.GetInfo(self, Source, Item)
	return "Reset your spent skill points\n[c yellow]Equipped skills are set to level 1"
end

function Item_RespecPotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Respec = 1

	return Result
end

-- Bottle of Tears --

Item_Tears = Base_Potion:New()

function Item_Tears.GetInfo(self, Source, Item)
	return "Drink your tears to convert [c gold]gold lost[c white] into [c silver]experience points[c white]"
end

function Item_Tears.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Experience = Source.GoldLost
	Result.Target.GoldLost = -Source.GoldLost

	return Result
end

-- Skill Slot --

Item_SkillSlot = { }

function Item_SkillSlot.GetInfo(self, Source, Item)
	return "Increase your skill bar size\n[c yellow]Can only be used once"
end

function Item_SkillSlot.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.SkillBarSize = 1

	return Result
end

function Item_SkillSlot.PlaySound(self)
	Audio.Play("unlock" .. Random.GetInt(0, 1) .. ".ogg", 0.85)
end

-- Belt Slot --

Item_BeltSlot = { }

function Item_BeltSlot.GetInfo(self, Source, Item)
	return "Increase your belt size\n[c yellow]Can only be used once"
end

function Item_BeltSlot.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.BeltSize = 1

	return Result
end

function Item_BeltSlot.PlaySound(self)
	Audio.Play("unlock" .. Random.GetInt(0, 1) .. ".ogg", 0.85)
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

function Item_SkillPoint.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.SkillPoint = Level

	return Result
end

function Item_SkillPoint.PlaySound(self)
	Audio.Play("unlock" .. Random.GetInt(0, 1) .. ".ogg", 0.85)
end

-- Throwing Knives --

Item_ThrowingKnives = Base_Attack:New()

function Item_ThrowingKnives.GenerateDamage(self, Level, Source)
	return self.Item.GenerateDamage(Source.Pointer, 0)
end

function Item_ThrowingKnives.GetPierce(self, Source)
	return self.Item.Pierce
end

function Item_ThrowingKnives.GetDamageType(self, Source)
	return self.Item.DamageType
end

function Item_ThrowingKnives.GetInfo(self, Source, Item)
	return "Throw a knife at your enemy, causing [c green]" .. math.floor(math.floor(Item.Level * Source.BleedPower * 0.01) * Item.Duration) .. "[c white] bleed damage over [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_ThrowingKnives.Use(self, Level, Duration, Source, Target, Result, Priority)
	Battle_ResolveDamage(self, Level, Source, Target, Result)
	self:Proc(1, Level, Duration, Source, Target, Result)

	return Result
end

function Item_ThrowingKnives.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Bleeding.Pointer
	Result.Target.BuffLevel = Level * Source.BleedPower * 0.01
	Result.Target.BuffDuration = Duration

	return true
end

function Item_ThrowingKnives.PlaySound(self)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Poison Knives --

Item_PoisonKnives = Base_Attack:New()

function Item_PoisonKnives.GetInfo(self, Source, Item)
	return "Throw a poison-tipped knife at your enemy, causing [c green]" .. math.floor(math.floor(Item.Level * Source.PoisonPower * 0.01) * Item.Duration) .. "[c white] poison damage over [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_PoisonKnives.GetDamageType(self, Source)
	return self.Item.DamageType
end

function Item_PoisonKnives.GetPierce(self, Source)
	return self.Item.Pierce
end

function Item_PoisonKnives.Proc(self, Roll, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Poisoned.Pointer
	Result.Target.BuffLevel = Level * Source.PoisonPower * 0.01
	Result.Target.BuffDuration = Duration

	return true
end

function Item_PoisonKnives.GenerateDamage(self, Level, Source)
	return self.Item.GenerateDamage(Source.Pointer, 0)
end

function Item_PoisonKnives.Use(self, Level, Duration, Source, Target, Result, Priority)
	Battle_ResolveDamage(self, Level, Source, Target, Result)
	self:Proc(1, Level, Duration, Source, Target, Result)

	return Result
end

function Item_PoisonKnives.PlaySound(self)
	Audio.Play("slash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Slimy Glob --

Item_SlimyGlob = { }

function Item_SlimyGlob.GetInfo(self, Source, Item)
	return "If bleeding, gain [c green]" .. Item.Level .. "% [c yellow]bleed [c white]resist for [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]bleeding"
end

function Item_SlimyGlob.CanUse(self, Level, Source, Target)
	if Target.HasBuff(Buff_Bleeding.Pointer) then
		return true
	end

	return false
end

function Item_SlimyGlob.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Source.HasBuff(Buff_Bleeding.Pointer) then
		Result.Target.Buff = Buff_BleedResist.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration
		Result.Target.ClearBuff = Buff_Bleeding.Pointer
	end

	return Result
end

function Item_SlimyGlob.PlaySound(self)
	Audio.Play("slime" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crab Legs --

Item_CrabLegs = { }

function Item_CrabLegs.GetInfo(self, Source, Item)
	return "Gain [c green]" .. Item.Level .. " [c white]armor for [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]fractured"
end

function Item_CrabLegs.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Hardened.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Fractured.Pointer

	return Result
end

function Item_CrabLegs.PlaySound(self)
	Audio.Play("crab" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Crow Feather --

Item_CrowFeather = { }

function Item_CrowFeather.GetInfo(self, Source, Item)
	return "Increase move speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds\n\nPurges [c yellow]slowness"
end

function Item_CrowFeather.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Fast.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Slowed.Pointer

	return Result
end

function Item_CrowFeather.PlaySound(self)
	Audio.Play("swoop" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Fire Dust --

Item_FireDust = { }

function Item_FireDust.GetInfo(self, Source, Item)
	return "Reduce target's hit chance by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds"
end

function Item_FireDust.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Blinded.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_FireDust.PlaySound(self)
	Audio.Play("dust" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Cold Dust --

Item_ColdDust = { }

function Item_ColdDust.GetInfo(self, Source, Item)
	return "Slow target by [c green]" .. Item.Level .. "% [c white]for [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]burning"
end

function Item_ColdDust.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Source.ClearBuff = Buff_Burning.Pointer

	return Result
end

function Item_ColdDust.PlaySound(self)
	Audio.Play("dust" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Spider Leg --

Item_SpiderLeg = { }

function Item_SpiderLeg.GetInfo(self, Source, Item)
	return "If slowed, increase battle speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds\n\nPurges [c yellow]slowness"
end

function Item_SpiderLeg.CanUse(self, Level, Source, Target)
	if Target.HasBuff(Buff_Slowed.Pointer) then
		return true
	end

	return false
end

function Item_SpiderLeg.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Source.HasBuff(Buff_Slowed.Pointer) then
		Result.Target.Buff = Buff_Hasted.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration
		Result.Target.ClearBuff = Buff_Slowed.Pointer
	end

	return Result
end

function Item_SpiderLeg.PlaySound(self)
	Audio.Play("spider0.ogg")
end

-- Fang --

Item_Fang = { }

function Item_Fang.GetInfo(self, Source, Item)
	return "If poisoned, gain [c green]" .. Item.Level .. "% [c yellow]poison [c white]resist for [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]poisoned"
end

function Item_Fang.CanUse(self, Level, Source, Target)
	if Target.HasBuff(Buff_Poisoned.Pointer) then
		return true
	end

	return false
end

function Item_Fang.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Source.HasBuff(Buff_Poisoned.Pointer) then
		Result.Target.Buff = Buff_PoisonResist.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration
		Result.Target.ClearBuff = Buff_Poisoned.Pointer
	end

	return Result
end

function Item_Fang.PlaySound(self)
	Audio.Play("bat0.ogg")
end

-- Spectral Dust --

Item_SpectralDust = { }

function Item_SpectralDust.GetInfo(self, Source, Item)
	return "Increase cold resist by [c green]" .. Item.Level .. "%[c white] for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_SpectralDust.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_ColdResist.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_SpectralDust.PlaySound(self)
	Audio.Play("ghost" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Teleport Scroll --

Item_TeleportScroll = { }

function Item_TeleportScroll.GetInfo(self, Source, Item)
	return "Teleport home after [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_TeleportScroll.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Teleport = Duration

	return Result
end

-- Ankh --

Item_Ankh = { }

function Item_Ankh.GetHeal(self, Source, Level)
	return math.floor(Level * Source.HealPower * 0.01)
end

function Item_Ankh.GetInfo(self, Source, Item)
	return "Throw an ankh at an ally's corpse to resurrect them with [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\n\n[c yellow]Can be used outside of battle"
end

function Item_Ankh.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Health = self:GetHeal(Source, Level)
	Result.Target.Corpse = 1

	return Result
end

function Item_Ankh.PlaySound(self)
	Audio.Play("choir0.ogg")
end

-- Swamp Glob --

Item_SwampGlob = { }

function Item_SwampGlob.GetInfo(self, Source, Item)
	return "Slow target by [c green]" .. Item.Level .. "% [c white]for [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]burning"
end

function Item_SwampGlob.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Slowed.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Source.ClearBuff = Buff_Burning.Pointer

	return Result
end

function Item_SwampGlob.PlaySound(self)
	Audio.Play("sludge0.ogg")
end

-- Lava Sludge --

Item_LavaSludge = { }

function Item_LavaSludge.GetInfo(self, Source, Item)
	return "Ignite a target for [c green]" .. math.floor(math.floor(Item.Level * Source.FirePower * 0.01) * Item.Duration) .. "[c white] damage over [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]flayed[c white]\n\n[c red]Damages yourself when used"
end

function Item_LavaSludge.GetDamageType(self, Source)
	return self.Item.DamageType
end

function Item_LavaSludge.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = Level * Source.FirePower * 0.01
	Result.Target.BuffDuration = Duration

	if Source.MonsterID == 0 then
		Result.Source.Buff = Buff_Burning.Pointer
		Result.Source.BuffLevel = math.max(1, math.floor(Level / 2))
		Result.Source.BuffDuration = Duration
	end
	Result.Source.ClearBuff = Buff_Flayed.Pointer

	return Result
end

function Item_LavaSludge.PlaySound(self)
	Audio.Play("flame0.ogg")
end

-- Firebomb --

Item_Firebomb = Base_Attack:New()

function Item_Firebomb.GenerateDamage(self, Level, Source)
	return self.Item.GenerateDamage(Source.Pointer, 0)
end

function Item_Firebomb.GetDamageType(self, Source)
	return self.Item.DamageType
end

function Item_Firebomb.GetTargetCount(self, Level)
	return 3
end

function Item_Firebomb.GetInfo(self, Source, Item)
	return "Toss an exploding potion at your enemies, dealing [c green]" .. math.floor(math.floor(Item.Level * Source.FirePower * 0.01) * Item.Duration) .. "[c white] burning damage over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_Firebomb.Use(self, Level, Duration, Source, Target, Result, Priority)
	Battle_ResolveDamage(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = Level * Source.FirePower * 0.01
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_Firebomb.PlaySound(self)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Unstable Potion --

Item_UnstablePotion = Base_Attack:New()

function Item_UnstablePotion.GetTargetCount(self, Level)
	return 3
end

function Item_UnstablePotion.GetInfo(self, Source, Item)
	return "Toss an unstable potion at your enemies, blinding them by [c green]" .. Item.Level .. "%[c white] for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_UnstablePotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Blinded.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_UnstablePotion.PlaySound(self)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Shrapnel Bomb --

Item_ShrapnelBomb = Base_Attack:New()

function Item_ShrapnelBomb.GenerateDamage(self, Level, Source)
	return self.Item.GenerateDamage(Source.Pointer, 0)
end

function Item_ShrapnelBomb.GetTargetCount(self, Level)
	return 3
end

function Item_ShrapnelBomb.GetPierce(self, Source)
	return self.Item.Pierce
end

function Item_ShrapnelBomb.GetInfo(self, Source, Item)
	return "Toss an exploding contraption at your enemies, dealing pierce damage and reducing resistances by [c green]" .. Item.Level .. "%[c white] for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_ShrapnelBomb.Use(self, Level, Duration, Source, Target, Result, Priority)
	Battle_ResolveDamage(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Flayed.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_ShrapnelBomb.PlaySound(self)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Bone --

Item_Bone = { }

function Item_Bone.GetInfo(self, Source, Item)
	return "Throw at your enemy for a [c green]" .. Item.Level .. "%[c white] chance to stun for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_Bone.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Random.GetInt(1, 100) <= Level then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = Duration
		return Result
	end

	return Result
end

function Item_Bone.PlaySound(self)
	Audio.Play("bash" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Stinger --

Item_Stinger = { }

function Item_Stinger.GetInfo(self, Source, Item)
	return "Increase attack power by [c_green]" .. Item.Level .. "%[c_white] for [c_green]" .. Item.Duration .. " [c_white]seconds\n\nPurges [c yellow]weakness"
end

function Item_Stinger.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Empowered.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Weak.Pointer

	return Result
end

function Item_Stinger.PlaySound(self)
	Audio.Play("thud0.ogg")
end

-- Elusive Potion --

Item_ElusivePotion = Base_Potion:New()

function Item_ElusivePotion.GetInfo(self, Source, Item)
	return "Throw down smoke cover for [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] allies, increasing evasion by [c green]" .. Item.Level .. "%[c white] for [c green]" .. math.floor(Item.Duration) .. " [c white]seconds"
end

function Item_ElusivePotion.GetTargetCount(self, Level)
	return 3
end

function Item_ElusivePotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Evasion.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_ElusivePotion.PlaySound(self)
	Audio.Play("ghost" .. Random.GetInt(0, 1) .. ".ogg")
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

function Item_BattlePotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Difficult.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration
	Result.Target.Battle = Source.GetTileZone(Source.X, Source.Y)

	return Result
end

-- Greater Battle Potion --

Item_GreaterBattlePotion = Base_Potion:New()

function Item_GreaterBattlePotion.GetInfo(self, Source, Item)
	return "Increase difficulty by [c green]" .. Item.Level .. "%[c white] for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_GreaterBattlePotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Difficult.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Ultimate Battle Potion --

Item_UltimateBattlePotion = Base_Potion:New()
Item_UltimateBattlePotion.Difficulty = 200

function Item_UltimateBattlePotion.GetInfo(self, Source, Item)
	return "Reduce current boss cooldowns by [c green]" .. Item.Level .. "%[c white]\nIncrease difficulty by [c green]" .. self.Difficulty .. "%[c white] for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_UltimateBattlePotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Difficult.Pointer
	Result.Target.BuffLevel = self.Difficulty
	Result.Target.BuffDuration = Duration
	Result.Target.CurrentBossCooldowns = Level

	return Result
end

-- Belligerent Potion --

Item_BelligerentPotion = Base_Potion:New()

function Item_BelligerentPotion.GetInfo(self, Source, Item)
	return "Get into fights more often for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_BelligerentPotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Attractant.Pointer
	Result.Target.BuffLevel = 2
	Result.Target.BuffDuration = Duration
	Result.Target.Battle = Source.GetTileZone(Source.X, Source.Y)

	return Result
end

-- Lava Potion --

Item_LavaPotion = Base_Potion:New()

function Item_LavaPotion.GetInfo(self, Source, Item)
	return "Grants lava immunity for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_LavaPotion.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_LavaImmune.Pointer
	Result.Target.BuffLevel = 0
	Result.Target.BuffDuration = Duration

	return Result
end

-- Warming Torch --

Item_WarmingTorch = { }

function Item_WarmingTorch.GetInfo(self, Source, Item)
	return "Grants freeze immunity for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_WarmingTorch.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Warm.Pointer
	Result.Target.BuffLevel = 0
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Freezing.Pointer

	return Result
end

function Item_WarmingTorch.PlaySound(self)
	Audio.Play("flame0.ogg")
end

-- Torch --

Item_Torch = { }

function Item_Torch.GetDuration(self, Source, Duration)
	return math.floor(Duration * Source.FirePower * 0.01)
end

function Item_Torch.GetInfo(self, Source, Item)
	return "Give light for [c green]" .. self:GetDuration(Source, Item.Duration) .. " [c white]seconds"
end

function Item_Torch.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Buff = Buff_Light.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = self:GetDuration(Source, Duration)

	return Result
end

function Item_Torch.PlaySound(self)
	Audio.Play("flame0.ogg")
end

-- Flamuss

function SetBonus_Flamuss.GetInfo(self, Source, Item)
	return "[c yellow]Grants freeze immunity\n\n" .. self:GetAddedInfo(Source, Item)
end

function SetBonus_Flamuss.Stats(self, Item, Object, Change)
	Change.FreezeProtection = true

	return Change
end

-- Icebrand
function SetBonus_Icebrand.GetInfo(self, Source, Item)
	return "[c yellow]Grants lava immunity\n\n" .. self:GetAddedInfo(Source, Item)
end

function SetBonus_Icebrand.Stats(self, Item, Object, Change)
	Change.LavaProtection = true

	return Change
end

-- Dimensional Slippers --

function SetBonus_DimensionalSlippers.GetInfo(self, Source, Item)
	return "[c yellow]Grants diagonal movement\n\n" .. self:GetAddedInfo(Source, Item)
end

function SetBonus_DimensionalSlippers.Stats(self, Item, Object, Change)
	Change.DiagonalMovement = true

	return Change
end

-- Lava Boots --

function SetBonus_LavaBoots.GetInfo(self, Source, Item)
	return "[c yellow]Grants lava immunity\n\n" .. self:GetAddedInfo(Source, Item)
end

function SetBonus_LavaBoots.Stats(self, Item, Object, Change)
	Change.LavaProtection = true

	return Change
end

-- Diagonal --

Item_Diagonal = { }

function Item_Diagonal.GetInfo(self, Source, Item)
	return "[c yellow]Grants diagonal movement"
end

function Item_Diagonal.Stats(self, Item, Object, Change)
	Change.DiagonalMovement = true

	return Change
end

-- Lava Protection --

Item_LavaProtection = { }

function Item_LavaProtection.GetInfo(self, Source, Item)
	return "[c yellow]Grants lava immunity"
end

function Item_LavaProtection.Stats(self, Item, Object, Change)
	Change.LavaProtection = true

	return Change
end

-- Freeze Protection --

Item_FreezeProtection = { }

function Item_FreezeProtection.GetInfo(self, Source, Item)
	return "[c yellow]Grants freeze immunity"
end

function Item_FreezeProtection.Stats(self, Item, Object, Change)
	Change.FreezeProtection = true

	return Change
end

-- Set Limit --

Item_SetLimit = { }

function Item_SetLimit.GetInfo(self, Source, Item)
	return "Reduce set requirements by [c green]" .. Item.Level
end

function Item_SetLimit.Stats(self, Item, Object, Change)
	Change.SetLimit = -Item.Level

	return Change
end

-- Pain Ring --

Item_PainRing = { }
Item_PainRing.DifficultyPerLevel = 5

function Item_PainRing.GetDifficulty(self, Source, Item)
	return Item.Level + Item.Upgrades * self.DifficultyPerLevel
end

function Item_PainRing.GetInfo(self, Source, Item)
	return "Increase difficulty by [c green]" .. self:GetDifficulty(Source, Item) .. "%"
end

function Item_PainRing.Stats(self, Item, Object, Change)
	Change.Difficulty = self:GetDifficulty(Source, Item)

	return Change
end

-- Lucky Amulet --

Item_LuckyAmulet = { }

function Item_LuckyAmulet.GetInfo(self, Source, Item)
	return "[c yellow]Increases gambling speed"
end

function Item_LuckyAmulet.Stats(self, Item, Object, Change)
	Change.MinigameSpeed = 2

	return Change
end

-- Mana Shield--

Item_ManaShield = { }
Item_ManaShield.ReductionPerUpgrade = 1

function Item_ManaShield.GetReduction(self, Item)
	return Item.Level + Item.Upgrades * self.ReductionPerUpgrade
end

function Item_ManaShield.GetInfo(self, Source, Item)
	return "Convert [c green]" .. self:GetReduction(Item) .. "%[c white] of attack damage taken to mana drain"
end

function Item_ManaShield.Stats(self, Item, Object, Change)
	Change.ManaShield = self:GetReduction(Item)

	return Change
end

-- Consume Chance --

Item_ConsumeChance = { }
Item_ConsumeChance.ChancePerUpgrade = 2

function Item_ConsumeChance.GetInfo(self, Source, Item)
	return "Decrease chance to consume items in battle by [c green]" .. self:GetChance(Item) .. "%[c white]"
end

function Item_ConsumeChance.GetChance(self, Item)
	return Item.Level + Item.Upgrades * self.ChancePerUpgrade
end

function Item_ConsumeChance.Stats(self, Item, Object, Change)
	Change.ConsumeChance = -self:GetChance(Item)

	return Change
end

-- Drop Rate --

Item_DropRate = { }
Item_DropRate.ChancePerUpgrade = 4

function Item_DropRate.GetInfo(self, Source, Item)
	return "Increase item drop rate from monsters by [c green]" .. self:GetChance(Item) .. "%[c white]"
end

function Item_DropRate.GetChance(self, Item)
	return Item.Level + Item.Upgrades * self.ChancePerUpgrade
end

function Item_DropRate.Stats(self, Item, Object, Change)
	Change.DropRate = self:GetChance(Item)

	return Change
end

-- Constricting Amulet --

Item_ConstrictingAmulet = { }
Item_ConstrictingAmulet.LevelPerUpgrade = 3.75

function Item_ConstrictingAmulet.GetInfo(self, Source, Item)
	return "Increase monster count in battle by [c green]" .. self:GetAmount(Item) .. "%[c white]"
end

function Item_ConstrictingAmulet.GetAmount(self, Item)
	return math.floor(Item.Level + Item.Upgrades * self.LevelPerUpgrade)
end

function Item_ConstrictingAmulet.Stats(self, Item, Object, Change)
	Change.MonsterCount = self:GetAmount(Item)

	return Change
end

-- Dark Note --

Item_DarkNote = { }

function Item_DarkNote.GetInfo(self, Source, Item)
	return "[c gray]Great power lies within the tower..."
end

-- Credits --

Item_Credits = { }

function Item_Credits.GetInfo(self, Source, Item)
	return "Thank you for playing!\n\n[c gray]Programming\nAlan Witkowski\n\n[c gray]Game Design\nAlan Witkowski\n\n[c gray]Artwork\nAlan Witkowski\n\n[c gray]Sound Design\nAlan Witkowski\n\n[c gray]Music\nAlan Witkowski"
end

-- Dark Ring --

Item_DarkRing = Base_Set:New()
Item_DarkRing.PowerPerUpgrade = 1

function Item_DarkRing.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_DarkRing.GetInfo(self, Source, Item)
	return "Increases summon power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_DarkRing.Stats(self, Item, Object, Change)
	Change.SummonPower = self:GetPower(Item)

	return Change
end

-- Attack Power --

Item_AttackPower = { }
Item_AttackPower.PowerPerUpgrade = 1

function Item_AttackPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_AttackPower.GetInfo(self, Source, Item)
	return "Increases attack power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_AttackPower.Stats(self, Item, Object, Change)
	Change.AttackPower = self:GetPower(Item)

	return Change
end

-- Physical Power --

Item_PhysicalPower = { }
Item_PhysicalPower.PowerPerUpgrade = 1

function Item_PhysicalPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_PhysicalPower.GetInfo(self, Source, Item)
	return "Increases physical power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_PhysicalPower.Stats(self, Item, Object, Change)
	Change.PhysicalPower = self:GetPower(Item)

	return Change
end

-- Fire Power --

Item_FirePower = { }
Item_FirePower.PowerPerUpgrade = 1

function Item_FirePower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_FirePower.GetInfo(self, Source, Item)
	return "Increases fire power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_FirePower.Stats(self, Item, Object, Change)
	Change.FirePower = self:GetPower(Item)

	return Change
end

-- Cold Power --

Item_ColdPower = { }
Item_ColdPower.PowerPerUpgrade = 1

function Item_ColdPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_ColdPower.GetInfo(self, Source, Item)
	return "Increases cold power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_ColdPower.Stats(self, Item, Object, Change)
	Change.ColdPower = self:GetPower(Item)

	return Change
end

-- Lightning Power --

Item_LightningPower = { }
Item_LightningPower.PowerPerUpgrade = 1

function Item_LightningPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_LightningPower.GetInfo(self, Source, Item)
	return "Increases lightning power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_LightningPower.Stats(self, Item, Object, Change)
	Change.LightningPower = self:GetPower(Item)

	return Change
end

-- Bleed Power --

Item_BleedPower = { }
Item_BleedPower.PowerPerUpgrade = 1

function Item_BleedPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_BleedPower.GetInfo(self, Source, Item)
	return "Increases bleed power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_BleedPower.Stats(self, Item, Object, Change)
	Change.BleedPower = self:GetPower(Item)

	return Change
end

-- Poison Power --

Item_PoisonPower = { }
Item_PoisonPower.PowerPerUpgrade = 1

function Item_PoisonPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_PoisonPower.GetInfo(self, Source, Item)
	return "Increases poison power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_PoisonPower.Stats(self, Item, Object, Change)
	Change.PoisonPower = self:GetPower(Item)

	return Change
end

-- Heal Power --

Item_HealPower = { }
Item_HealPower.PowerPerUpgrade = 5

function Item_HealPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_HealPower.GetInfo(self, Source, Item)
	return "Increases heal power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_HealPower.Stats(self, Item, Object, Change)
	Change.HealPower = self:GetPower(Item)

	return Change
end

-- Mana Power --

Item_ManaPower = { }
Item_ManaPower.PowerPerUpgrade = 1

function Item_ManaPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_ManaPower.GetInfo(self, Source, Item)
	return "Increases mana power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_ManaPower.Stats(self, Item, Object, Change)
	Change.ManaPower = self:GetPower(Item)

	return Change
end

-- Summon Power --

Item_SummonPower = { }
Item_SummonPower.PowerPerUpgrade = 5

function Item_SummonPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_SummonPower.GetInfo(self, Source, Item)
	return "Increases summon power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_SummonPower.Stats(self, Item, Object, Change)
	Change.SummonPower = self:GetPower(Item)

	return Change
end

-- Rebirth Token--

Item_RebirthToken = { }

function Item_RebirthToken.GetInfo(self, Source, Item)
	Levels = 5
	Tiers = 1
	Difficulty = 1

	return "You are in rebirth tier [c green]" .. Source.RebirthTier .. "[c white]\n\nEvery [c green]" .. Levels .. "[c white] character levels gives [c green]" .. Tiers .. "[c white] tier, which increases your rebirth stat bonus\n\nEvery rebirth adds [c green]" .. Difficulty .. "%[c white] difficulty"
end

-- Evolve Token--

Item_EvolveToken = { }

function Item_EvolveToken.GetInfo(self, Source, Item)
	Levels = 10
	Tiers = 1

	return "You are in evolve tier [c green]" .. Source.EvolveTier .. "[c white]\n\nEvery [c green]" .. Levels .. "[c white] rebirths gives [c green]" .. Tiers .. "[c white] tier, which increases your evolve stat bonus\n\nEvolving increases your rebirth tier by [c green]1\n\n[c yellow]Evolving resets your rebirths and bonuses\n[c yellow]You will keep all unlocked rites"
end

-- Rebirth --

Base_Rebirth = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetBonus = function(self, Source, MoreInfo)
		if Source.RebirthTier == 0 then
			return 0
		end

		if self.Type == 1 then
			return math.floor(Source.RebirthTier) + 4
		elseif self.Type == 2 then
			local Value = Source.RebirthTier / 2 + 2
			if MoreInfo then
				return Round(Value)
			else
				return math.floor(Value)
			end
		elseif self.Type == 3 then
			local Value = (Source.RebirthTier - 1) / 5 + 1
			if MoreInfo then
				return Round(Value)
			else
				return math.floor(Value)
			end
		end
	end,

	CanUse = function(self, Level, Source, Target)
		if self:GetBonus(Source) > 0 then
			return true
		end

		return false
	end,

	Type = 1
}

Base_Evolve = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetBonus = function(self, Source)
		if Source.EvolveTier == 0 then
			return 0
		end

		if self.Type == 1 then
			return math.floor(Source.EvolveTier * 10)
		elseif self.Type == 2 then
			return math.floor(Source.EvolveTier * 2)
		end
	end,

	CanUse = function(self, Level, Source, Target)
		if self:GetBonus(Source) > 0 then
			return true
		end

		return false
	end,

	Type = 1
}

function RebirthStartText(Item, Source, Mode)
	AddedDifficulty = 0
	if Item == Item_EternalPain then
		AddedDifficulty = Item:GetBonus(Source)
	end

	Gold = math.min(math.floor(Source.RebirthWealth * 0.01 * Source.Experience * Item_RiteWealth.Multiplier), MAX_GOLD)
	PrivilegePlural = ""
	WisdomPlural = ""
	PassagePlural = ""
	if Source.RebirthPrivilege ~= 1 then
		PrivilegePlural = "s"
	end
	if Source.RebirthWisdom + 1 ~= 1 then
		WisdomPlural = "s"
	end
	if Source.RebirthPassage ~= 1 then
		PassagePlural = "s"
	end

	KeepText = "\n\n[c yellow]You will keep\n"
	KeepText = KeepText .. "[c green]" .. Source.RebirthKnowledge .. "[c white] of your highest level skills\n"
	if Source.RebirthPrivilege > 0 then
		KeepText = KeepText .. "[c green]" .. Source.RebirthPrivilege .. "[c white] item" .. PrivilegePlural .." in your trade bag\n"
	end
	KeepText = KeepText .. "\n[c yellow]You will start with\n"
	KeepText = KeepText .. "[c green]" .. Source.RebirthEnchantment .. "[c white] extra max skill levels\n"
	if Mode == 0 then
		KeepText = KeepText .. "[c green]" .. Source.Rebirths + Source.EternalPain + AddedDifficulty + 1 .. "%[c white] difficulty\n"
	end
	KeepText = KeepText .. "[c green]" .. Source.RebirthWisdom + 1 .. "[c white] character level" .. WisdomPlural .. "\n"
	KeepText = KeepText .. "[c green]" .. Source.RebirthPassage .. "[c white] key" .. PassagePlural .. "\n"
	KeepText = KeepText .. "[c green]" .. Gold .. "[c white] gold\n"

	return KeepText
end

function RebirthText(Item, UpgradeText, Source)
	KeepText = RebirthStartText(Item, Source, 0)

	return "[c gray]Sacrifice everything to rebirth anew\n\nLose all items, unlocks, keys, gold, experience and learned skills for:\n\nPermanent " .. UpgradeText .. KeepText .. "\n[c yellow]Warning\nYou will only be able to interact with players that have the same number of rebirths"
end

function EvolveText(Item, UpgradeText, Source)
	KeepText = RebirthStartText(Item, Source, 1)

	return "[c gray]Evolve into a higher form\n\nLose all rebirths, items, unlocks, keys, gold, experience and learned skills for:\n\nPermanent " .. UpgradeText .. KeepText .. "\n[c yellow]Warning\nYou will only be able to interact with players that have the same number of evolves and rebirths"
end

Item_EternalStrength = Base_Rebirth:New()

function Item_EternalStrength.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. self:GetBonus(Source) .. "%[c white] damage power bonus", Source)
end

function Item_EternalStrength.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Rebirth = 1
	Result.Target.MaxDamage = self:GetBonus(Source)

	return Result
end

function Item_EternalStrength.PlaySound(self)
	Audio.Play("rebirth.ogg")
end

Item_EternalGuard = Base_Rebirth:New()

function Item_EternalGuard.GetInfo(self, Source, Item)
	local DamageBlock = self:GetBonus(Source)
	local Armor = math.floor(self:GetBonus(Source) / 3)
	local Resist = math.floor(self:GetBonus(Source) / 4)

	if Item.MoreInfo then
		Armor = Round(self:GetBonus(Source) / 3)
		Resist = Round(self:GetBonus(Source) / 4)
	end

	return RebirthText(self, "[c green]" .. DamageBlock .. "[c white] damage block, [c green]" .. Armor .. "[c white] armor, and [c green]" .. Resist .. "[c white] resistance bonus", Source)
end

function Item_EternalGuard.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Rebirth = 1
	Result.Target.Armor = self:GetBonus(Source)

	return Result
end

function Item_EternalGuard.PlaySound(self)
	Audio.Play("rebirth.ogg")
end

Item_EternalFortitude = Base_Rebirth:New()

function Item_EternalFortitude.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. self:GetBonus(Source) .. "%[c white] max health and heal power bonus", Source)
end

function Item_EternalFortitude.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Rebirth = 1
	Result.Target.Health = self:GetBonus(Source)

	return Result
end

function Item_EternalFortitude.PlaySound(self)
	Audio.Play("rebirth.ogg")
end

Item_EternalSpirit = Base_Rebirth:New()

function Item_EternalSpirit.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. self:GetBonus(Source) .. "%[c white] max mana and mana power bonus", Source)
end

function Item_EternalSpirit.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Rebirth = 1
	Result.Target.Mana = self:GetBonus(Source)

	return Result
end

function Item_EternalSpirit.PlaySound(self)
	Audio.Play("rebirth.ogg")
end

Item_EternalWisdom = Base_Rebirth:New()

function Item_EternalWisdom.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. self:GetBonus(Source) .. "%[c white] experience bonus", Source)
end

function Item_EternalWisdom.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Rebirth = 1
	Result.Target.Experience = self:GetBonus(Source)

	return Result
end

function Item_EternalWisdom.PlaySound(self)
	Audio.Play("rebirth.ogg")
end

Item_EternalWealth = Base_Rebirth:New()

function Item_EternalWealth.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. self:GetBonus(Source) .. "%[c white] gold bonus", Source)
end

function Item_EternalWealth.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Rebirth = 1
	Result.Target.Gold = self:GetBonus(Source)

	return Result
end

function Item_EternalWealth.PlaySound(self)
	Audio.Play("rebirth.ogg")
end

Item_EternalKnowledge = Base_Rebirth:New()
Item_EternalKnowledge.Type = 3

function Item_EternalKnowledge.GetInfo(self, Source, Item)
	Bonus = self:GetBonus(Source)
	Plural = ""
	if Bonus ~= 1 then
		Plural = "s"
	end

	return RebirthText(self, "[c green]" .. self:GetBonus(Source, Item.MoreInfo) .. "[c white] extra skill point" .. Plural, Source)
end

function Item_EternalKnowledge.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Rebirth = 1
	Result.Target.SkillPoint = self:GetBonus(Source)

	return Result
end

function Item_EternalKnowledge.PlaySound(self)
	Audio.Play("rebirth.ogg")
end

Item_EternalPain = Base_Rebirth:New()
Item_EternalPain.Type = 2

function Item_EternalPain.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. self:GetBonus(Source, Item.MoreInfo) .. "%[c white] difficulty increase", Source)
end

function Item_EternalPain.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Rebirth = 1
	Result.Target.Difficulty = self:GetBonus(Source)

	return Result
end

function Item_EternalPain.PlaySound(self)
	Audio.Play("rebirth.ogg")
end

Item_EternalAlacrity = Base_Evolve:New()

function Item_EternalAlacrity.GetInfo(self, Source, Item)
	return EvolveText(self, "[c green]" .. self:GetBonus(Source) .. "%[c white] battle speed bonus", Source)
end

function Item_EternalAlacrity.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Evolve = 1
	Result.Target.BattleSpeed = self:GetBonus(Source)

	return Result
end

function Item_EternalAlacrity.PlaySound(self)
	Audio.Play("sparkle0.ogg")
end

Item_EternalCommand = Base_Evolve:New()

function Item_EternalCommand.GetInfo(self, Source, Item)
	return EvolveText(self, "[c green]" .. self:GetBonus(Source) .. "%[c white] summon battle speed bonus", Source)
end

function Item_EternalCommand.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Evolve = 1
	Result.Target.SummonBattleSpeed = self:GetBonus(Source)

	return Result
end

function Item_EternalCommand.PlaySound(self)
	Audio.Play("sparkle0.ogg")
end

Item_EternalImpatience = Base_Evolve:New()
Item_EternalImpatience.Type = 2

function Item_EternalImpatience.GetInfo(self, Source, Item)
	return EvolveText(self, "[c green]" .. self:GetBonus(Source) .. "%[c white] cooldown reduction bonus", Source)
end

function Item_EternalImpatience.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.Evolve = 1
	Result.Target.Cooldowns = self:GetBonus(Source)

	return Result
end

function Item_EternalImpatience.PlaySound(self)
	Audio.Play("sparkle0.ogg")
end

-- Rites --
Base_Rite = {

	New = function(self, Object)
		Object = Object or {}
		setmetatable(Object, self)
		self.__index = self
		return Object
	end,

	GetRiteText = function(self, UpgradeText)
		return "Permanently " .. UpgradeText
	end,

	PlaySound = function(self)
		Audio.Play("unlock" .. Random.GetInt(0, 1) .. ".ogg", 0.85)
	end,

	GetUpgradedPrice = function(self, Source, Upgrades)
		Index = Source.GetInventoryItemCount(self.Item.Pointer) + Upgrades

		return math.floor(self.Item.Cost + self.Item.Cost * (self.CostA * Index ^ self.Exponent + self.CostB * Index))
	end,

	Exponent = 2.0,
	CostA = 0.0,
	CostB = 1.0
}

Item_RiteWealth = Base_Rite:New()
Item_RiteWealth.CostA = 0.1
Item_RiteWealth.Multiplier = REBIRTH_WEALTH_MULTIPLIER

function Item_RiteWealth.GetInfo(self, Source, Item)
	return self:GetRiteText("increase the amount of experience converted into gold after rebirth by [c green]" .. self:GetPercent(Item.Level) .. "%[c white]")
end

function Item_RiteWealth.GetPercent(self, Level)
	return Level * self.Multiplier
end

function Item_RiteWealth.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthWealth)
end

function Item_RiteWealth.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.RebirthWealth = Level

	return Result
end

Item_RiteWisdom = Base_Rite:New()
Item_RiteWisdom.CostA = 0.1

function Item_RiteWisdom.GetInfo(self, Source, Item)
	return self:GetRiteText("increase the starting level after rebirth by [c green]" .. Item.Level .. "[c white]")
end

function Item_RiteWisdom.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthWisdom)
end

function Item_RiteWisdom.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.RebirthWisdom = Level

	return Result
end

Item_RiteKnowledge = Base_Rite:New()
Item_RiteKnowledge.CostA = 0.75
Item_RiteKnowledge.CostB = 0.75

function Item_RiteKnowledge.GetInfo(self, Source, Item)
	return self:GetRiteText("increase the number of skills carried over after rebirth by [c green]" .. Item.Level .. "[c white]\n\nSkills will have a max level of [c green]" .. Source.RebirthEnchantment + DEFAULT_MAX_SKILL_LEVEL)
end

function Item_RiteKnowledge.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthKnowledge)
end

function Item_RiteKnowledge.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.RebirthKnowledge = Level

	return Result
end

Item_RitePower = Base_Rite:New()
Item_RitePower.CostA = 1.0

function Item_RitePower.GetInfo(self, Source, Item)
	return self:GetRiteText("increase your rebirth tier bonus by [c green]" .. Item.Level .. "[c white]")
end

function Item_RitePower.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthPower)
end

function Item_RitePower.Use(self, Level, Duration, Source, Target, Result, Priority)
	Result.Target.RebirthPower = Level

	return Result
end

Item_RiteGirth = Base_Rite:New()
Item_RiteGirth.CostA = 5

function Item_RiteGirth.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthGirth >= MAX_BELT_SIZE - DEFAULT_BELTSIZE then
		AddedText = "\n\n[c red]Max girth attained"
	end

	return self:GetRiteText("increase the belt size after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText)
end

function Item_RiteGirth.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthGirth)
end

function Item_RiteGirth.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Target.RebirthGirth < MAX_BELT_SIZE - DEFAULT_BELTSIZE then
		Result.Target.RebirthGirth = Level
	end

	return Result
end

Item_RiteProficiency = Base_Rite:New()
Item_RiteProficiency.CostA = 1

function Item_RiteProficiency.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthProficiency >= MAX_SKILLBAR_SIZE - DEFAULT_SKILLBARSIZE then
		AddedText = "\n\n[c red]Max proficiency attained"
	end

	return self:GetRiteText("increase the skill bar size after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText)
end

function Item_RiteProficiency.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthProficiency)
end

function Item_RiteProficiency.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Target.RebirthProficiency < MAX_SKILLBAR_SIZE - DEFAULT_SKILLBARSIZE then
		Result.Target.RebirthProficiency = Level
	end

	return Result
end

Item_RiteInsight = Base_Rite:New()
Item_RiteInsight.CostA = 0.25

function Item_RiteInsight.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthInsight >= MAX_SKILL_UNLOCKS then
		AddedText = "\n\n[c red]Max insight attained"
	end

	return self:GetRiteText("increase the starting skill point unlocks after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText)
end

function Item_RiteInsight.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthInsight)
end

function Item_RiteInsight.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Target.RebirthInsight < MAX_SKILL_UNLOCKS then
		Result.Target.RebirthInsight = Level
	end

	return Result
end

Item_RitePassage = Base_Rite:New()
Item_RitePassage.Keys = {
	{ "Graveyard Key",    500000 },
	{ "Library Key",      1000000 },
	{ "Cellar Key",       2500000 },
	{ "Tower Key",        5000000 },
	{ "City Key",         10000000 },
	{ "Bridge Key",       25000000 },
	{ "Lost Key",         50000000 },
	{ "Green Runestone",  100000000 },
	{ "Blue Runestone",   250000000 },
	{ "Red Runestone",    500000000 },
	{ "Black Runestone",  750000000 },
	{ "Garden Key",       1000000000 },
	{ "Icy Key",          5000000000 },
	{ "Timeless Key",     10000000000 },
	{ "Dark Chamber Key", 100000000000 },
}

function Item_RitePassage.GetInfo(self, Source, Item)
	if Source.RebirthPassage >= #self.Keys then
		AddedText = "\n\n[c red]Max passage attained"
	else
		AddedText = "\n\n[c yellow]Start with the " .. self.Keys[Source.RebirthPassage + 1][1]
	end

	return self:GetRiteText("increase the number of keys unlocked after rebirth by [c green]" .. Item.Level .. AddedText)
end

function Item_RitePassage.GetCost(self, Source)
	Count = Source.GetInventoryItemCount(self.Item.Pointer)
	KeyIndex = math.max(1, math.min(Source.RebirthPassage + Count + 1, #self.Keys))
	return self.Keys[KeyIndex][2]
end

function Item_RitePassage.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Target.RebirthPassage < #self.Keys then
		Result.Target.RebirthPassage = Level
	end

	return Result
end

Item_RiteEnchantment = Base_Rite:New()
Item_RiteEnchantment.CostA = 0.25

function Item_RiteEnchantment.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthEnchantment >= MAX_SKILL_LEVEL - DEFAULT_MAX_SKILL_LEVEL then
		AddedText = "\n\n[c red]Max enchantment attained"
	end

	return self:GetRiteText("increase the max level for starting skills after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText)
end

function Item_RiteEnchantment.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthEnchantment)
end

function Item_RiteEnchantment.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Target.RebirthEnchantment < MAX_SKILL_LEVEL - DEFAULT_MAX_SKILL_LEVEL then
		Result.Target.RebirthEnchantment = Level
	end

	return Result
end

Item_RitePrivilege = Base_Rite:New()
Item_RitePrivilege.CostA = 2
Item_RitePrivilege.Max = 8

function Item_RitePrivilege.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthPrivilege >= self.Max then
		AddedText = "\n\n[c red]Max privilege attained"
	end

	return self:GetRiteText("increase the number of items carried over after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText .. "\n\n[c yellow]Items must be placed in your trade bag")
end

function Item_RitePrivilege.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthPrivilege)
end

function Item_RitePrivilege.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Source.RebirthPrivilege < self.Max then
		Result.Target.RebirthPrivilege = Level
	end

	return Result
end

Item_RiteSoul = Base_Rite:New()
Item_RiteSoul.CostA = 2
Item_RiteSoul.Max = 100

function Item_RiteSoul.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthSoul >= self.Max then
		AddedText = "\n\n[c red]Max soul attained"
	end

	return self:GetRiteText("decrease boss cooldowns by [c green]" .. Item.Level .. "%[c white]" .. AddedText)
end

function Item_RiteSoul.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthSoul)
end

function Item_RiteSoul.Use(self, Level, Duration, Source, Target, Result, Priority)
	if Source.RebirthSoul < self.Max then
		Result.Target.RebirthSoul = Level
	end

	return Result
end
