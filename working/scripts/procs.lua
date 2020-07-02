-- Functions --

function Proc_AddBuff(Change, Buff, Level, Duration)

	-- Check for existing buff
	if Change.Buff == nil then
		Change.Buff = Buff
		Change.BuffLevel = Level
		Change.BuffDuration = Duration
		return
	end

	-- Buff exists, but is different
	if Change.Buff ~= Buff then
		return
	end

	-- Take better level or duration
	if Level > Change.BuffLevel then
		Change.BuffLevel = Level
	elseif Level == Change.BuffLevel and Duration > Change.BuffDuration then
		Change.BuffDuration = Duration
	end
end

-- Stun --

Proc_Stun = { }

function Proc_Stun.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance to stun for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Stun.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance then
		Proc_AddBuff(Result.Target, Buff_Stunned.Pointer, 1, Duration)

		return true
	end

	return false
end

-- Poison --

Proc_Poison = { }

function Proc_Poison.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance for [c green]" .. math.floor(Item.Level * Item.Duration) .. "[c white] poison damage over [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Poison.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance then
		Proc_AddBuff(Result.Target, Buff_Poisoned.Pointer, Level, Duration)

		return true
	end

	return false
end

-- Slow --

Proc_Slow = { }

function Proc_Slow.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance to slow target by [c green]" .. Item.Level .. "%[c white] for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Slow.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance then
		Proc_AddBuff(Result.Target, Buff_Slowed.Pointer, Level, Duration)

		return true
	end

	return false
end

-- Bleed --

Proc_Bleed = { }

function Proc_Bleed.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance for [c green]" .. math.floor(Item.Level * Item.Duration) .. "[c white] bleeding damage over [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Bleed.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance then
		Proc_AddBuff(Result.Target, Buff_Bleeding.Pointer, Level, Duration)

		return true
	end

	return false
end

-- Haste --

Proc_Haste = { }

function Proc_Haste.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance for a [c green]" .. Item.Level .. "%[c white] speed boost for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Haste.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance and Result.Source.Buff == nil then
		Proc_AddBuff(Result.Source, Buff_Hasted.Pointer, Level, Duration)

		return true
	end

	return false
end

-- Harden --

Proc_Harden = { }

function Proc_Harden.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance for a [c green]" .. Item.Level .. "[c white] armor buff for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Harden.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance and Result.Source.Buff == nil then
		Proc_AddBuff(Result.Source, Buff_Hardened.Pointer, Level, Duration)

		return true
	end

	return false
end

-- Blind --

Proc_Blind = { }

function Proc_Blind.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance to inflict [c green]" .. Item.Level .. "%[c white] blindness for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Blind.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance and Result.Source.Buff == nil then
		Proc_AddBuff(Result.Target, Buff_Blinded.Pointer, Level, Duration)

		return true
	end

	return false
end

-- Empowered --

Proc_Empowered = { }

function Proc_Empowered.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance for a [c green]" .. Item.Level .. "%[c white] damage buff for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Empowered.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance and Result.Source.Buff == nil then
		Proc_AddBuff(Result.Source, Buff_Empowered.Pointer, Level, Duration)

		return true
	end

	return false
end

-- Health --

Proc_Health = { }

function Proc_Health.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance to restore [c green]" .. Item.Level .. "[c white] HP"
end

function Proc_Health.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance then
		if Result.Source.Health == nil then
			Result.Source.Health = Level
		else
			Result.Source.Health = Result.Source.Health + Level
		end
	end

	return false
end

-- Mana --

Proc_Mana = { }

function Proc_Mana.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance to restore [c green]" .. Item.Level .. "[c white] MP"
end

function Proc_Mana.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance then
		if Result.Source.Mana == nil then
			Result.Source.Mana = Level
		else
			Result.Source.Mana = Result.Source.Mana + Level
		end
	end

	return false
end