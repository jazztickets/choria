GOAL_NONE    = 0
GOAL_FARMING = 1
GOAL_HEALING = 2

Bot_Basic = {}
Bot_Basic.Goal = 0

function Bot_Basic.Update(self, Object)
	print("goal=" .. self.Goal .. " map=" .. Object.MapID .. " x=" .. Object.X .. " y=" .. Object.Y)

	if self.Goal == GOAL_NONE then
		self:DetermineNextGoal(Object)
	elseif self.Goal == GOAL_FARMING then
		if Object.MapID ~= 10 then
			X, Y = Object.FindEvent(3, 10)
			if X ~= nil then
				print("MoveTo x=" .. X .. " y=" .. Y)
				Object.FindPath(X, Y)
			end
		end
	elseif self.Goal == GOAL_HEALING then
	end
end

function Bot_Basic.GetInputState(self, Object)

	return 1
end

function Bot_Basic.DetermineNextGoal(self, Object)

	HealthPercent = Object.Health / Object.MaxHealth
	if HealthPercent <= 0.5 then
		self.Goal = GOAL_HEALING
	else
		self.Goal = GOAL_FARMING
	end
	self.Goal = GOAL_FARMING
end
