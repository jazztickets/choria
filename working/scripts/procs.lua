-- Stun --

Proc_Stun = { }

function Proc_Stun.GetInfo(self, Source, Item)

	return "[c green]" .. Item.Chance .. "%[c white] chance to stun for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Stun.Proc(self, Roll, Chance, Level, Duration, Source, Target, Result)
	if Roll <= Chance then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = Duration
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
		Result.Target.Buff = Buff_Poisoned.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration

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
		Result.Target.Buff = Buff_Slowed.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration

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
		Result.Target.Buff = Buff_Bleeding.Pointer
		Result.Target.BuffLevel = Level
		Result.Target.BuffDuration = Duration

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
		Result.Source.Buff = Buff_Hasted.Pointer
		Result.Source.BuffLevel = Level
		Result.Source.BuffDuration = Duration

		return true
	end

	return false
end