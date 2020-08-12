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
	return "Restore [c green]" .. math.floor(math.floor(Buff_Healing.Heal * Item.Level * Source.HealPower * 0.01) * Item.Duration) .. "[c white] HP over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_HealingSalve.Use(self, Level, Duration, Source, Target, Result)
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

function Item_ManaCider.Use(self, Level, Duration, Source, Target, Result)
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
	return "Poison a target for [c green]" .. math.floor(math.floor(Item.Level * Source.PoisonPower * 0.01) * Item.Duration) .. "[c white] damage over [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_PoisonPotion.Use(self, Level, Duration, Source, Target, Result)
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

function Item_BrightPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Clock = Level

	return Result
end

-- Dark Potion --

Item_DarkPotion = Base_Potion:New()

function Item_DarkPotion.GetInfo(self, Source, Item)
	return "Turn the world to darkness"
end

function Item_DarkPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Clock = Level

	return Result
end

-- Respec Potion --

Item_RespecPotion = Base_Potion:New()

function Item_RespecPotion.GetInfo(self, Source, Item)
	return "Reset your spent skill points\n[c yellow]Action bar skills are set to level 1"
end

function Item_RespecPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Respec = 1

	return Result
end

-- Skill Slot --

Item_SkillSlot = { }

function Item_SkillSlot.GetInfo(self, Source, Item)
	return "Increase your skill bar size\n[c yellow]Can only be used once"
end

function Item_SkillSlot.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.SkillBarSize = 1

	return Result
end

function Item_SkillSlot.PlaySound(self, Level)
	Audio.Play("unlock" .. Random.GetInt(0, 1) .. ".ogg", 0.85)
end

-- Belt Slot --

Item_BeltSlot = { }

function Item_BeltSlot.GetInfo(self, Source, Item)
	return "Increase your belt size\n[c yellow]Can only be used once"
end

function Item_BeltSlot.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.BeltSize = 1

	return Result
end

function Item_BeltSlot.PlaySound(self, Level)
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

function Item_SkillPoint.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.SkillPoint = Level

	return Result
end

function Item_SkillPoint.PlaySound(self, Level)
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

function Item_ThrowingKnives.Use(self, Level, Duration, Source, Target, Result)
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

function Item_ThrowingKnives.PlaySound(self, Level)
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
	return "If bleeding, gain [c green]" .. Item.Level .. "% [c yellow]bleed [c white]resist for [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]bleeding"
end

function Item_SlimyGlob.CanUse(self, Level, Source, Target)
	if Target.HasBuff(Buff_Bleeding.Pointer) then
		return true
	end

	return false
end

function Item_SlimyGlob.Use(self, Level, Duration, Source, Target, Result)
	if Source.HasBuff(Buff_Bleeding.Pointer) then
		Result.Target.Buff = Buff_BleedResist.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration
		Result.Target.ClearBuff = Buff_Bleeding.Pointer
	end

	return Result
end

function Item_SlimyGlob.PlaySound(self, Level)
	Audio.Play("slime" .. Random.GetInt(0, 1) .. ".ogg")
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

-- Crow Feather --

Item_CrowFeather = { }

function Item_CrowFeather.GetInfo(self, Source, Item)
	return "Increase move speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds\n\nPurges [c yellow]slowness"
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
	return "If slowed, increase battle speed by [c_green]" .. Item.Level .. "% [c_white]for [c_green]" .. Item.Duration .. " [c_white]seconds\n\nPurges [c yellow]slowness"
end

function Item_SpiderLeg.CanUse(self, Level, Source, Target)
	if Target.HasBuff(Buff_Slowed.Pointer) then
		return true
	end

	return false
end

function Item_SpiderLeg.Use(self, Level, Duration, Source, Target, Result)
	if Source.HasBuff(Buff_Slowed.Pointer) then
		Result.Target.Buff = Buff_Hasted.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration
		Result.Target.ClearBuff = Buff_Slowed.Pointer
	end

	return Result
end

function Item_SpiderLeg.PlaySound(self, Level)
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

function Item_Fang.Use(self, Level, Duration, Source, Target, Result)
	if Source.HasBuff(Buff_Poisoned.Pointer) then
		Result.Target.Buff = Buff_PoisonResist.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration
		Result.Target.ClearBuff = Buff_Poisoned.Pointer
	end

	return Result
end

function Item_Fang.PlaySound(self, Level)
	Audio.Play("bat0.ogg")
end

-- Spectral Dust --

Item_SpectralDust = { }

function Item_SpectralDust.GetInfo(self, Source, Item)
	return "Increase cold resist by [c green]" .. Item.Level .. "%[c white] for [c green]" .. Item.Duration .. " [c white]seconds"
end

function Item_SpectralDust.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_ColdResist.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_SpectralDust.PlaySound(self, Level)
	Audio.Play("ghost" .. Random.GetInt(0, 1) .. ".ogg")
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

-- Ankh --

Item_Ankh = { }

function Item_Ankh.GetHeal(self, Source, Level)
	return math.floor(Level * Source.HealPower * 0.01)
end

function Item_Ankh.GetInfo(self, Source, Item)
	return "Throw an ankh at an ally's corpse to resurrect them with [c green]" .. self:GetHeal(Source, Item.Level) .. "[c white] HP\n\n[c yellow]Can be used outside of battle"
end

function Item_Ankh.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Health = self:GetHeal(Source, Level)
	Result.Target.Corpse = 1

	return Result
end

function Item_Ankh.PlaySound(self, Level)
	Audio.Play("choir0.ogg")
end

-- Swamp Glob --

Item_SwampGlob = { }

function Item_SwampGlob.GetInfo(self, Source, Item)
	return "Slow target by [c green]" .. Item.Level .. "% [c white]for [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]burning"
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
	return "Ignite a target for [c green]" .. math.floor(math.floor(Item.Level * Source.FirePower * 0.01) * Item.Duration) .. "[c white] damage over [c green]" .. Item.Duration .. " [c white]seconds\n\nPurges [c yellow]weakness[c white]\n\n[c red]Damages yourself when used"
end

function Item_LavaSludge.GetDamageType(self, Source)
	return self.Item.DamageType
end

function Item_LavaSludge.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = Level * Source.FirePower * 0.01
	Result.Target.BuffDuration = Duration
	Result.Target.ClearBuff = Buff_Weak.Pointer

	if Source.MonsterID == 0 then
		Result.Source.Buff = Buff_Burning.Pointer
		Result.Source.BuffLevel = math.max(1, math.floor(Level / 2))
		Result.Source.BuffDuration = Duration
	end
	Result.Source.ClearBuff = Buff_Weak.Pointer

	return Result
end

function Item_LavaSludge.PlaySound(self, Level)
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

function Item_Firebomb.Use(self, Level, Duration, Source, Target, Result)
	Battle_ResolveDamage(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Burning.Pointer
	Result.Target.BuffLevel = Level * Source.FirePower * 0.01
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_Firebomb.PlaySound(self, Level)
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

function Item_UnstablePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Blinded.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_UnstablePotion.PlaySound(self, Level)
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

function Item_ShrapnelBomb.Use(self, Level, Duration, Source, Target, Result)
	Battle_ResolveDamage(self, Level, Source, Target, Result)

	Result.Target.Buff = Buff_Flayed.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_ShrapnelBomb.PlaySound(self, Level)
	Audio.Play("blast" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Bone --

Item_Bone = { }

function Item_Bone.GetInfo(self, Source, Item)
	return "Throw at your enemy for a [c green]" .. Item.Level .. "%[c white] chance to stun for [c green]" .. Item.Duration .. "[c white] seconds"
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

-- Elusive Potion --

Item_ElusivePotion = Base_Potion:New()

function Item_ElusivePotion.GetInfo(self, Source, Item)
	return "Throw down smoke cover for [c green]" .. self:GetTargetCount(Item.Level) .. "[c white] allies and increase evasion by [c green]" .. Item.Level .. "% [c white] for [c green]" .. math.floor(Item.Duration) .. " [c white]seconds"
end

function Item_ElusivePotion.GetTargetCount(self, Level)
	return 3
end

function Item_ElusivePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Evasion.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

function Item_ElusivePotion.PlaySound(self, Level)
	Audio.Play("ghost" .. Random.GetInt(0, 1) .. ".ogg")
end

-- Greater Battle Potion --

Item_GreaterBattlePotion = Base_Potion:New()

function Item_GreaterBattlePotion.GetInfo(self, Source, Item)
	return "Increase difficulty by [c green]" .. Item.Level .. "%[c white] for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_GreaterBattlePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Difficult.Pointer
	Result.Target.BuffLevel = Level
	Result.Target.BuffDuration = Duration

	return Result
end

-- Ultimate Battle Potion --

Item_UltimateBattlePotion = Base_Potion:New()

function Item_UltimateBattlePotion.GetInfo(self, Source, Item)
	return "Reduce current boss cooldowns by [c green]" .. Item.Level .. "%[c white]"
end

function Item_UltimateBattlePotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.BossCooldowns = Level

	return Result
end

-- Belligerent Potion --

Item_BelligerentPotion = Base_Potion:New()

function Item_BelligerentPotion.GetInfo(self, Source, Item)
	return "Get into fights more often for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Item_BelligerentPotion.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Buff = Buff_Attractant.Pointer
	Result.Target.BuffLevel = 2
	Result.Target.BuffDuration = Duration
	Result.Target.Battle = Source.GetTileZone(Source.X, Source.Y)

	return Result
end

-- Torch --

Item_Torch = { }

function Item_Torch.GetDuration(self, Source, Duration)
	return math.floor(Duration * Source.FirePower * 0.01)
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

-- Diagonal --

Item_Diagonal = { }

function Item_Diagonal.GetInfo(self, Source, Item)
	return "[c yellow]Allows for diagonal movement"
end

function Item_Diagonal.Stats(self, Item, Object, Change)
	Change.DiagonalMovement = true

	return Change
end

-- Lava Protection --

Item_LavaProtection = { }

function Item_LavaProtection.GetInfo(self, Source, Item)
	return "[c yellow]Grants immunity to lava"
end

function Item_LavaProtection.Stats(self, Item, Object, Change)
	Change.LavaProtection = true

	return Change
end

-- Pain Ring --

Item_PainRing = { }
Item_PainRing.Difficulty = 50
Item_PainRing.DifficultyPerLevel = 5

function Item_PainRing.GetDifficulty(self, Source, Item)
	return self.Difficulty + Item.Upgrades * self.DifficultyPerLevel
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

-- Energy Field --

Item_EnergyField = { }
Item_EnergyField.ReductionPerUpgrade = 1

function Item_EnergyField.GetReduction(self, Item)
	return Item.Level + Item.Upgrades * self.ReductionPerUpgrade
end

function Item_EnergyField.GetInfo(self, Source, Item)
	return "Convert [c green]" .. self:GetReduction(Item) .. "%[c white] of attack damage taken to mana drain"
end

function Item_EnergyField.Stats(self, Item, Object, Change)
	Change.EnergyField = self:GetReduction(Item)

	return Change
end

-- Consume Chance --

Item_ConsumeChance = { }
Item_ConsumeChance.ChancePerUpgrade = 1

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

-- Dark Note --

Item_DarkNote = { }

function Item_DarkNote.GetInfo(self, Source, Item)
	return "[c gray]Great power lies within the tower..."
end

-- Health Ring --

Item_HealthRing = { }
Item_HealthRing.PowerPerUpgrade = 1

function Item_HealthRing.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_HealthRing.GetInfo(self, Source, Item)
	return "Increases heal power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_HealthRing.Stats(self, Item, Object, Change)
	Change.HealPower = self:GetPower(Item)

	return Change
end

-- Dark Ring --

Item_DarkRing = { }
Item_DarkRing.PowerPerUpgrade = 1
Item_DarkRing.SummonLimit = 1

function Item_DarkRing.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_DarkRing.GetInfo(self, Source, Item)
	return "Increases pet power by [c green]" .. self:GetPower(Item) .. "%\nIncreases summon limit by [c green]" .. self.SummonLimit
end

function Item_DarkRing.Stats(self, Item, Object, Change)
	Change.PetPower = self:GetPower(Item)
	Change.SummonLimit = 1

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

-- Pet Power --

Item_PetPower = { }
Item_PetPower.PowerPerUpgrade = 5

function Item_PetPower.GetPower(self, Item)
	return Item.Level + Item.Upgrades * self.PowerPerUpgrade
end

function Item_PetPower.GetInfo(self, Source, Item)
	return "Increases pet power by [c green]" .. self:GetPower(Item) .. "%"
end

function Item_PetPower.Stats(self, Item, Object, Change)
	Change.PetPower = self:GetPower(Item)

	return Change
end

-- Rebirth Token--

Item_RebirthToken = { }

function Item_RebirthToken.GetInfo(self, Source, Item)
	Levels = 5
	Tiers = 1
	Difficulty = 1

	return "You are in rebirth tier [c green]" .. Source.RebirthTier .. "[c white]\n\nEvery [c green]" .. Levels .. "[c white] character levels gives [c green]" .. Tiers .. "[c white] tier\nEach tier increases your rebirth stat bonus\n\nEvery rebirth adds [c green]" .. Difficulty .. "%[c white] difficulty"
end

-- Rebirth --

function GetRebirthBonus(Source, Type)
	if Source.RebirthTier == 0 then
		return 0
	end

	if Type == 1 then
		return math.floor(Source.RebirthTier) + 4
	elseif Type == 2 then
		return math.floor(Source.RebirthTier / 2) + 2
	elseif Type == 3 then
		return math.ceil(Source.RebirthTier / 5)
	end
end

function RebirthText(Item, UpgradeText, Source)
	AddedDifficulty = 0
	if Item == Item_EternalPain then
		AddedDifficulty = Item_EternalPain.Value
	end

	Gold = math.min(math.floor(Source.RebirthWealth * 0.01 * Source.Experience * Item_RiteWealth.Multiplier), MAX_GOLD)
	KeepText = "\n\n[c yellow]You will keep\n"
	KeepText = KeepText .. "[c green]" .. Source.RebirthKnowledge .. "[c white] of your highest level skills\n"
	Plural = ""
	if Source.RebirthPower ~= 1 then
		Plural = "s"
	end

	KeepText = KeepText .. "\n[c yellow]You will start with\n"
	KeepText = KeepText .. "[c green]" .. Source.RebirthEnchantment .. "[c white] extra max skill levels\n"

	KeepText = KeepText .. "[c green]" .. Source.Rebirths + Source.EternalPain + AddedDifficulty + 1 .. "%[c white] difficulty\n"
	Plural = ""
	if Source.RebirthWisdom + 1 ~= 1 then
		Plural = "s"
	end
	KeepText = KeepText .. "[c green]" .. Source.RebirthWisdom + 1 .. "[c white] character level" .. Plural .. "\n"

	Plural = ""
	if Source.RebirthPassage ~= 1 then
		Plural = "s"
	end
	KeepText = KeepText .. "[c green]" .. Source.RebirthPassage .. "[c white] key" .. Plural .. "\n"
	KeepText = KeepText .. "[c green]" .. Gold .. "[c white] gold\n"

	return "[c gray]Sacrifice everything to rebirth anew\n\nLose all items, unlocks, keys, gold, experience and skills for:\n\nPermanent " .. UpgradeText .. KeepText .. "\n[c yellow]Warning\nYou will only be able to interact with players that have the same number of rebirths"
end

Item_EternalStrength = { Type = 1 }

function Item_EternalStrength.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. GetRebirthBonus(Source, self.Type) .. "%[c white] damage bonus", Source)
end

function Item_EternalStrength.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.MaxDamage = GetRebirthBonus(Source, self.Type)

	return Result
end

function Item_EternalStrength.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
end

Item_EternalGuard = { Type = 1 }

function Item_EternalGuard.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. GetRebirthBonus(Source, self.Type) .. "[c white] damage block, [c green]" .. math.floor(GetRebirthBonus(Source, self.Type) / 3) .. "[c white] armor, and [c green]" .. math.floor(GetRebirthBonus(Source, self.Type) / 4) .. "[c white] resistance bonus", Source)
end

function Item_EternalGuard.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Armor = GetRebirthBonus(Source, self.Type)

	return Result
end

function Item_EternalGuard.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
end

Item_EternalFortitude = { Type = 1 }

function Item_EternalFortitude.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. GetRebirthBonus(Source, self.Type) .. "%[c white] max health and heal power bonus", Source)
end

function Item_EternalFortitude.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Health = GetRebirthBonus(Source, self.Type)

	return Result
end

function Item_EternalFortitude.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
end

Item_EternalSpirit = { Type = 1 }

function Item_EternalSpirit.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. GetRebirthBonus(Source, self.Type) .. "%[c white] max mana and mana power bonus", Source)
end

function Item_EternalSpirit.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Mana = GetRebirthBonus(Source, self.Type)

	return Result
end

function Item_EternalSpirit.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
end

Item_EternalWisdom = { Type = 1 }

function Item_EternalWisdom.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. GetRebirthBonus(Source, self.Type) .. "%[c white] experience bonus", Source)
end

function Item_EternalWisdom.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Experience = GetRebirthBonus(Source, self.Type)

	return Result
end

function Item_EternalWisdom.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
end

Item_EternalWealth = { Type = 1 }

function Item_EternalWealth.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. GetRebirthBonus(Source, self.Type) .. "%[c white] gold bonus", Source)
end

function Item_EternalWealth.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Gold = GetRebirthBonus(Source, self.Type)

	return Result
end

function Item_EternalWealth.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
end

Item_EternalAlacrity = { Type = 2 }

function Item_EternalAlacrity.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. GetRebirthBonus(Source, self.Type) .. "%[c white] battle speed bonus", Source)
end

function Item_EternalAlacrity.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.BattleSpeed = GetRebirthBonus(Source, self.Type)

	return Result
end

function Item_EternalAlacrity.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
end

Item_EternalKnowledge = { Type = 3 }

function Item_EternalKnowledge.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. GetRebirthBonus(Source, self.Type) .. "[c white] extra skill point", Source)
end

function Item_EternalKnowledge.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.SkillPoint = GetRebirthBonus(Source, self.Type)

	return Result
end

function Item_EternalKnowledge.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
end

Item_EternalPain = { Value = 10 }

function Item_EternalPain.GetInfo(self, Source, Item)
	return RebirthText(self, "[c green]" .. self.Value .. "%[c white] difficulty increase", Source)
end

function Item_EternalPain.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.Rebirth = 1
	Result.Target.Difficulty = self.Value

	return Result
end

function Item_EternalPain.PlaySound(self, Level)
	Audio.Play("rebirth.ogg")
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
		return "Permanently increase " .. UpgradeText
	end,

	PlaySound = function(self, Level)
		Audio.Play("unlock" .. Random.GetInt(0, 1) .. ".ogg", 0.85)
	end,

	GetUpgradedPrice = function(self, Source, Upgrades)
		Count = Source.GetInventoryItemCount(self.Item.Pointer)

		return math.floor(self.Item.Cost + self.Item.Cost * (Count + Upgrades) ^ self.Exponent)
	end,

	Exponent = 1.0
}

Item_RiteWealth = Base_Rite:New()
Item_RiteWealth.Exponent = 1.25
Item_RiteWealth.Multiplier = REBIRTH_WEALTH_MULTIPLIER

function Item_RiteWealth.GetInfo(self, Source, Item)
	return self:GetRiteText("the amount of experience converted into gold after rebirth by [c green]" .. self:GetPercent(Item.Level) .. "%[c white]")
end

function Item_RiteWealth.GetPercent(self, Level)
	return Level * self.Multiplier
end

function Item_RiteWealth.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthWealth)
end

function Item_RiteWealth.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.RebirthWealth = Level

	return Result
end

Item_RiteWisdom = Base_Rite:New()
Item_RiteWisdom.Exponent = 1.25

function Item_RiteWisdom.GetInfo(self, Source, Item)
	return self:GetRiteText("the starting level after rebirth by [c green]" .. Item.Level .. "[c white]")
end

function Item_RiteWisdom.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthWisdom)
end

function Item_RiteWisdom.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.RebirthWisdom = Level

	return Result
end

Item_RiteKnowledge = Base_Rite:New()
Item_RiteKnowledge.Exponent = 2

function Item_RiteKnowledge.GetInfo(self, Source, Item)
	return self:GetRiteText("the number of learned skills carried over after rebirth by [c green]" .. Item.Level .. "[c white]\n\nSkills will have a max level of [c green]" .. Source.RebirthEnchantment + DEFAULT_MAX_SKILL_LEVEL)
end

function Item_RiteKnowledge.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthKnowledge)
end

function Item_RiteKnowledge.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.RebirthKnowledge = Level

	return Result
end

Item_RitePower = Base_Rite:New()
Item_RitePower.Exponent = 2.0

function Item_RitePower.GetInfo(self, Source, Item)
	return self:GetRiteText("your rebirth tier bonus by [c green]" .. Item.Level .. "[c white]")
end

function Item_RitePower.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthPower)
end

function Item_RitePower.Use(self, Level, Duration, Source, Target, Result)
	Result.Target.RebirthPower = Level

	return Result
end

Item_RiteGirth = Base_Rite:New()
Item_RiteGirth.Exponent = 2

function Item_RiteGirth.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthGirth >= MAX_BELT_SIZE - DEFAULT_BELTSIZE then
		AddedText = "\n\n[c red]Max girth attained"
	end

	return self:GetRiteText("the belt size after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText)
end

function Item_RiteGirth.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthGirth)
end

function Item_RiteGirth.Use(self, Level, Duration, Source, Target, Result)
	if Target.RebirthGirth < MAX_BELT_SIZE - DEFAULT_BELTSIZE then
		Result.Target.RebirthGirth = Level
	end

	return Result
end

Item_RiteProficiency = Base_Rite:New()
Item_RiteProficiency.Exponent = 1.25

function Item_RiteProficiency.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthProficiency >= MAX_SKILLBAR_SIZE - DEFAULT_SKILLBARSIZE then
		AddedText = "\n\n[c red]Max proficiency attained"
	end

	return self:GetRiteText("the skill bar size after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText)
end

function Item_RiteProficiency.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthProficiency)
end

function Item_RiteProficiency.Use(self, Level, Duration, Source, Target, Result)
	if Target.RebirthProficiency < MAX_SKILLBAR_SIZE - DEFAULT_SKILLBARSIZE then
		Result.Target.RebirthProficiency = Level
	end

	return Result
end

Item_RiteInsight = Base_Rite:New()
Item_RiteInsight.Exponent = 1.25

function Item_RiteInsight.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthInsight >= MAX_SKILL_UNLOCKS then
		AddedText = "\n\n[c red]Max insight attained"
	end

	return self:GetRiteText("the starting skill point unlocks after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText)
end

function Item_RiteInsight.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthInsight)
end

function Item_RiteInsight.Use(self, Level, Duration, Source, Target, Result)
	if Target.RebirthInsight < MAX_SKILL_UNLOCKS then
		Result.Target.RebirthInsight = Level
	end

	return Result
end

Item_RitePassage = Base_Rite:New()
Item_RitePassage.Keys = {
	{ "Graveyard Key", 500000 },
	{ "Library Key", 1000000 },
	{ "Cellar Key", 2500000 },
	{ "Tower Key", 5000000 },
	{ "City Key", 10000000 },
	{ "Bridge Key", 25000000 },
	{ "Lost Key", 50000000 },
}

function Item_RitePassage.GetInfo(self, Source, Item)
	if Source.RebirthPassage >= #self.Keys then
		AddedText = "\n\n[c red]Max passage attained"
	else
		AddedText = "\n\n[c yellow]Start with the " .. self.Keys[Source.RebirthPassage + 1][1]
	end

	return self:GetRiteText("the number of keys unlocked after rebirth by [c green]" .. Item.Level .. AddedText)
end

function Item_RitePassage.GetCost(self, Source)
	Count = Source.GetInventoryItemCount(self.Item.Pointer)
	KeyIndex = math.max(1, math.min(Source.RebirthPassage + Count + 1, #self.Keys))
	return self.Keys[KeyIndex][2]
end

function Item_RitePassage.Use(self, Level, Duration, Source, Target, Result)
	if Target.RebirthPassage < #self.Keys then
		Result.Target.RebirthPassage = Level
	end

	return Result
end

Item_RiteEnchantment = Base_Rite:New()
Item_RiteEnchantment.Exponent = 1.25

function Item_RiteEnchantment.GetInfo(self, Source, Item)
	AddedText = ""
	if Source.RebirthEnchantment >= MAX_SKILL_LEVEL - DEFAULT_MAX_SKILL_LEVEL then
		AddedText = "\n\n[c red]Max enchantment attained"
	end

	return self:GetRiteText("the max level for starting skills after rebirth by [c green]" .. Item.Level .. "[c white]" .. AddedText)
end

function Item_RiteEnchantment.GetCost(self, Source)
	return self:GetUpgradedPrice(Source, Source.RebirthEnchantment)
end

function Item_RiteEnchantment.Use(self, Level, Duration, Source, Target, Result)
	if Target.RebirthEnchantment < MAX_SKILL_LEVEL - DEFAULT_MAX_SKILL_LEVEL then
		Result.Target.RebirthEnchantment = Level
	end

	return Result
end
