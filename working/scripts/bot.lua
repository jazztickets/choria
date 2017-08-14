DIRECTION_NONE  = 0
DIRECTION_UP    = 1
DIRECTION_DOWN  = 2
DIRECTION_LEFT  = 4
DIRECTION_RIGHT = 8
MOVE_IDLE       = 0
MOVE_PATH       = 1
MOVE_HEAL       = 2
MOVE_RANDOM     = 3
GOAL_NONE       = 0
GOAL_FARMING    = 1
GOAL_HEALING    = 2
GOAL_BUY        = 3
GOAL_SELL       = 4
GOAL_UPGRADE    = 5

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

-- Bot that runs only on the server --

Bot_Server = {}
Bot_Server.GoalState = GOAL_NONE
Bot_Server.MoveState = MOVE_IDLE
Bot_Server.Timer = 0

function Bot_Server.Update(self, FrameTime, Object)
	--print("goal=" .. self.GoalState .. " gold=" .. Object.Gold .. " map=" .. Object.MapID .. " x=" .. Object.X .. " y=" .. Object.Y .. " timer=" .. self.Timer)

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

	self.Timer = self.Timer + FrameTime
end

function Bot_Server.GetInputState(self, Object)
	InputState = DIRECTION_NONE

	if self.MoveState == MOVE_IDLE then
	elseif self.MoveState == MOVE_PATH then
		InputState = Object.GetInputStateFromPath()
		if InputState == 0 then
			self.MoveState = MOVE_IDLE
		end
	elseif self.MoveState == MOVE_HEAL then
		InputState = DIRECTION_LEFT
	elseif self.MoveState == MOVE_RANDOM then
		InputState = MoveTypes[Random.GetInt(1, 4)]
		Direction = GetDirection(InputState)
		EventType, EventData = Object.GetTileEvent(Object.X + Direction[1], Object.Y + Direction[2])
		if EventType ~= 0 then
			InputState = DIRECTION_NONE
		end
	end

	return InputState
end

function Bot_Server.DetermineNextGoal(self, Object)
	--print(self.Timer .. ": determine goal")

	-- Check skill points
	SkillPointsAvailable = Object.GetSkillPointsAvailable()
	if SkillPointsAvailable > 0 then

		-- Toughness
		SkillPointsAvailable = Object.SpendSkillPoints(5, SkillPointsAvailable)

		-- Attack
		if SkillPointsAvailable > 0 then
			SkillPointsAvailable = Object.SpendSkillPoints(1, SkillPointsAvailable)
		end
	end

	-- Check health
	HealthPercent = Object.Health / Object.MaxHealth
	if HealthPercent <= 0.5 then
		self.GoalState = GOAL_HEALING
	else

		-- Check item progression
		Item = Object.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
		if Item ~= nil then
			--print(Item.ID)
		end

		self.GoalState = GOAL_FARMING
	end

end

-- Bot that simulates a connected client --

Bot_Client = {}
for Key, Value in pairs(Bot_Server) do
	Bot_Client[Key] = Value
end

function Bot_Client.DetermineNextGoal(self, Object)
	HealthPercent = Object.Health / Object.MaxHealth
	if HealthPercent <= 0.5 then
		self.GoalState = GOAL_HEALING
	else
		self.GoalState = GOAL_FARMING
	end
end
