Proc_Stun = { }

function Proc_Stun.GetInfo(self, Source, Item)

	return "[c green]" .. Item.Level .. "%[c white] chance to stun for [c green]" .. Item.Duration .. "[c white] seconds"
end

function Proc_Stun.Proc(self, Roll, Level, Duration, Source, Target, Result)
	if Roll <= Level then
		Result.Target.Buff = Buff_Stunned.Pointer
		Result.Target.BuffLevel = 1
		Result.Target.BuffDuration = Duration
		return true
	end

	return false
end
