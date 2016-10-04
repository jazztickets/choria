GOAL_NONE    = 0
GOAL_FARMING = 1
GOAL_HEALING = 2
MOVE_IDLE    = 0
MOVE_PATH    = 1
MOVE_HEAL    = 2
MOVE_RANDOM  = 3

MoveTypes = { 1, 2, 4, 8 }

function GetDirection(InputState)

	if InputState == 1 then
		return {  0, -1 }
	elseif InputState == 2 then
		return {  0,  1 }
	elseif InputState == 4 then
		return { -1,  0 }
	elseif InputState == 8 then
		return {  1,  0 }
	end

	return { 0, 0 }
end

-- Basic bot AI --

Bot_Basic = {}
Bot_Basic.GoalState = GOAL_NONE
Bot_Basic.MoveState = MOVE_IDLE

function Bot_Basic.Update(self, Object)
	--print("goal=" .. self.GoalState .. " map=" .. Object.MapID .. " x=" .. Object.X .. " y=" .. Object.Y)

	if self.GoalState == GOAL_NONE then
		self:DetermineNextGoal(Object)
	elseif self.GoalState == GOAL_FARMING then
		if Object.MapID ~= 10 then
			X, Y = Object.FindEvent(3, 10)
			if X ~= nil then
				Object.FindPath(X, Y)
				self.MoveState = MOVE_PATH
			end
		else
			self.MoveState = MOVE_RANDOM
		end
	elseif self.GoalState == GOAL_HEALING then
		if Object.MapID ~= 1 then
			X, Y = Object.FindEvent(3, 1)
			if X ~= nil then
				Object.FindPath(X, Y)
				self.MoveState = MOVE_PATH
			end
		elseif Object.MapID == 1 then
			HealthPercent = Object.Health / Object.MaxHealth
			if HealthPercent < 1.0 then
				X, Y = Object.FindEvent(7, 1)
				if X ~= nil then
					if Object.X == X and Object.Y == Y then
						self.MoveState = MOVE_HEAL
					else
						Object.FindPath(X, Y)
						self.MoveState = MOVE_PATH
					end
				end
			else
				self:DetermineNextGoal(Object)
			end
		end
	end
end

function Bot_Basic.GetInputState(self, Object)
	InputState = 0

	if self.MoveState == MOVE_IDLE then
	elseif self.MoveState == MOVE_PATH then
		InputState = Object.GetInputStateFromPath()
		if InputState == 0 then
			self.MoveState = MOVE_IDLE
		end
	elseif self.MoveState == MOVE_HEAL then
		InputState = 4
	elseif self.MoveState == MOVE_RANDOM then
		InputState = MoveTypes[Random.GetInt(1, 4)]
		Direction = GetDirection(InputState)
		EventType, EventData = Object.GetTileEvent(Object.X + Direction[1], Object.Y + Direction[2])
		if EventType ~= 0 then
			InputState = 0
		end
	end

	return InputState
end

function Bot_Basic.DetermineNextGoal(self, Object)

	HealthPercent = Object.Health / Object.MaxHealth
	if HealthPercent <= 0.5 then
		self.GoalState = GOAL_HEALING
	else
		self.GoalState = GOAL_FARMING
	end
end

