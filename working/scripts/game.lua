require("scripts/battle")
require("scripts/procs")
require("scripts/buffs")
require("scripts/skills")
require("scripts/spells")
require("scripts/items")
require("scripts/sets")
require("scripts/ai")
require("scripts/bot")
require("scripts/scripts")

Game = {}

-- Set difficulty based on game time
function Game.GetDifficulty(self, Clock)

	-- Make nighttime more difficult
	if Clock < 6 * 60 or Clock >= 22 * 60 then
		return 50
	end

	return 0
end

function Round(Value)
	return math.floor(Value * 10) / 10.0
end