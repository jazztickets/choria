-- Functions --

function Proc_AddBuff(Result, Buff, Level, Duration)

	-- Check for existing buff
	if Result.Target.Buff == nil then
		Result.Target.Buff = Buff
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration
		return
	end

	-- Buff exists, but is different
	if Result.Target.Buff ~= Buff then
		return
	end

	-- Take better level or duration
	if Level > Result.Target.BuffLevel then
		Result.Target.BuffLevel = Level
	elseif Level == Result.Target.BuffLevel and Duration > Result.Target.BuffDuration then
		Result.Target.BuffDuration = Duration
	end
end

-- Stun --

Proc_Stun = { }

function Proc_Stun.GetInfo(self, Source, Item)
	return "[c green]" .. Item.Chance .. "%[c white] chance to stun for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Stun.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance then
		Proc_AddBuff(Result, Buff_Stunned.Pointer, 1, Duration)

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
		Proc_AddBuff(Result, Buff_Poisoned.Pointer, Level, Duration)

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
		Proc_AddBuff(Result, Buff_Slowed.Pointer, Level, Duration)

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
		Proc_AddBuff(Result, Buff_Bleeding.Pointer, Level, Duration)

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
		Proc_AddBuff(Result, Buff_Hasted.Pointer, Level, Duration)

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
		Proc_AddBuff(Result, Buff_Hardened.Pointer, Level, Duration)

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
		Proc_AddBuff(Result, Buff_Blinded.Pointer, Level, Duration)

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
		Proc_AddBuff(Result, Buff_Empowered.Pointer, Level, Duration)

		return true
	end

	return false
end