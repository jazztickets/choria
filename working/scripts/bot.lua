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

-- Convert input state to vec2
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

-- Return a list of map ids that get from Current to Target. Return nil otherwise
function FindMap(Current, Target, Path)

	-- Early exit
	if Current == Target then
		return Current
	end

	-- Get list of connected nodes
	local Connected = Paths[Current]
	if Connected == nil then
		return nil
	end

	-- Check connected nodes
	for i = 1, #Connected do

		if Connected[i] == Target then
			return Target
		end

		-- Avoid loops
		local Skip = 0
		for j = 1, #Path do
			if Connected[i] == Path[j] then
				Skip = 1
			end
		end

		-- Recurse
		if Skip == 0 then

			-- Append node to current path
			table.insert(Path, Connected[i])

			local Last = FindMap(Connected[i], Target, Path)
			if Last ~= nil then
				return Last
			end

			-- Dead end
			table.remove(Path)
		end
	end

	-- No path was found
	return nil
end

-- Builds --

Builds = {
	[1] = {
		Items = {
			-- ItemID, VendorID
			[INVENTORY_HAND1] = {
				{ 101, 3 },
				{ 100, 3 },
				{ 119, 3 },
				{ 147, 3 }
			}
		}
	}
}

-- Paths is a structure that stores map connections --

Paths = {
	[1] = { 10, 2, 4, 5, 6, 7 },
	[2] = { 1 },
	[4] = { 1 },
	[5] = { 1 },
	[6] = { 1 },
	[7] = { 1 },
	[10] = { 1, 11, 12, 13, 14, 16, 20, 30 },
	[11] = { 1 },
	[20] = { 10, 21, 22 },
	[21] = { 20 },
	[22] = { 20, 23 },
	[23] = { 22, 24 },
	[24] = { 23, 25 },
	[25] = { 24, 26 },
	[26] = { 21, 25 },
}

-- Bot that runs on the server --

Bot_Server = {}
Bot_Server.GoalState = GOAL_NONE
Bot_Server.MoveState = MOVE_IDLE
Bot_Server.BuyID = 0
Bot_Server.VendorID = 0
Bot_Server.Timer = 0
Bot_Server.MoveCount = 0
Bot_Server.TargetMapID = 0
Bot_Server.MapPath = nil
Bot_Server.Build = Builds[1]

-- Update bot behavior when outside of battle
function Bot_Server.Update(self, FrameTime, Object)
	--print("goal=" .. self.GoalState .. " gold=" .. Object.Gold .. " map=" .. Object.MapID .. " x=" .. Object.X .. " y=" .. Object.Y .. " timer=" .. self.Timer)

	if self.GoalState == GOAL_NONE then
		self:DetermineNextGoal(Object)
	elseif self.GoalState == GOAL_FARMING then
		Arrived = self:TraverseMap(Object)
		if Arrived then
			self.MoveState = MOVE_RANDOM
		end
	elseif self.GoalState == GOAL_HEALING then
		Arrived = self:TraverseMap(Object)
		if Arrived then
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
	elseif self.GoalState == GOAL_BUY then
		if Object.MapID ~= 1 and Object.MapID ~= 4 then
			X, Y = Object.FindEvent(3, 1)
			if X ~= nil then
				Object.FindPath(X, Y)
				self.MoveState = MOVE_PATH
			end
		elseif Object.MapID == 1 then
			X, Y = Object.FindEvent(3, 4)
			if X ~= nil then
				Object.FindPath(X, Y)
				self.MoveState = MOVE_PATH
			end
		elseif Object.MapID == 4 then
			X, Y = Object.FindEvent(4, self.VendorID)
			if X ~= nil then
				Object.FindPath(X, Y)
				self.MoveState = MOVE_PATH
			end
		end
	end

	self.Timer = self.Timer + FrameTime
end

-- Pathfind to next map in MapPath
function Bot_Server.TraverseMap(self, Object)
	if Object.MapID == self.TargetMapID or self.MapPath == nil or next(self.MapPath) == nil then
		return true
	end

	i, NextMapID = next(self.MapPath)
	if Object.MapID == NextMapID then
		table.remove(self.MapPath, 1)
		i, NextMapID = next(self.MapPath)
	end

	X, Y = Object.FindEvent(3, NextMapID)
	if X ~= nil then
		Object.FindPath(X, Y)
		self.MoveState = MOVE_PATH
	end
end

-- Build MapPath list of MapIDs
function Bot_Server.GoToMap(self, Object, MapID)
	self.MapPath = {}
	self.TargetMapID = MapID
	local Last = FindMap(Object.MapID, self.TargetMapID, self.MapPath)
	if Last ~= nil then
		table.insert(self.MapPath, Last)
	end
end

-- Return the next direction the bot will move
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

-- Get next goal
function Bot_Server.DetermineNextGoal(self, Object)

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
		self:GoToMap(Object, 1)
	else

		self.BuyID = 0
		self.VendorID = 0

		-- Check item progression
		CheckType = INVENTORY_HAND1
		Item = Object.GetInventoryItem(BAG_EQUIPMENT, CheckType)
		SellPrice = 0
		if Item ~= nil then
			SellPrice = math.floor(Item.Cost * 0.5)
		end

		-- Get list of item/vendor ids from build
		IDs = self.Build.Items[CheckType]

		-- Find next item to buy
		for i = #IDs, 1, -1 do
			if Object.Gold >= Items[IDs[i][1]].Cost then
				self.BuyID = IDs[i][1]
				self.VendorID = IDs[i][2]
				break
			end
		end

		self.BuyID = 0
		if self.BuyID ~= 0 then
			self.GoalState = GOAL_BUY
		else
			self.GoalState = GOAL_FARMING
			self:GoToMap(Object, 10)
		end
	end

	print("DetermineNextGoal ( goal=" .. self.GoalState .. " gold=" .. Object.Gold .. " map=" .. Object.MapID .. " x=" .. Object.X .. " y=" .. Object.Y .. " )")
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
