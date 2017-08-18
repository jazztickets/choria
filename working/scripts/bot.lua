DIRECTION_NONE  = 0
DIRECTION_UP    = 1
DIRECTION_DOWN  = 2
DIRECTION_LEFT  = 4
DIRECTION_RIGHT = 8
MOVE_IDLE       = 0
MOVE_PATH       = 1
MOVE_RANDOM     = 2
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

-- Return a list of map ids in Path that get from Current to Target. Return true on found
function FindMap(Current, Target, Path)

	local Set = {}
	local Parents = {}
	local Queue = {}

	-- Add root node
	Set[Current] = 1
	table.insert(Queue, 1, Current)

	-- Iterate over queue
	while next(Queue) ~= nil do
		Next = table.remove(Queue, 1)

		-- Found target
		if Next == Target then

			-- Build path
			Parent = Parents[Next]
			while Parent ~= nil do
				table.insert(Path, 1, Parent)
				Parent = Parents[Parent]
			end
			table.insert(Path, Target)

			return true
		end

		-- Add adjacent nodes
		local Connected = Paths[Next]
		if Connected ~= nil then
			for i = 1, #Connected do
				Adjacent = Connected[i]
				if Set[Adjacent] == nil then
					Set[Adjacent] = 1
					Parents[Adjacent] = Next
					table.insert(Queue, 1, Adjacent)
				end
			end
		end

	end

	return false
end

-- Paths is a structure that stores map connections --

Paths = {
	[1] = { 2, 4, 5, 6, 7, 10 },
	[2] = { 1 },
	[4] = { 1 },
	[5] = { 1 },
	[6] = { 1 },
	[7] = { 1 },
	[10] = { 1, 11, 12, 13, 14, 16, 20, 30 },
	[11] = { 10 },
	[20] = { 10, 21, 22 },
	[21] = { 20 },
	[22] = { 20, 23 },
	[23] = { 22, 24 },
	[24] = { 23, 25 },
	[25] = { 24, 26 },
	[26] = { 21, 25 },
}

-- Builds --

Builds = {
	[1] = {
		Items = {
			-- ItemID, VendorID
			[INVENTORY_HAND1] = {
				{ 101, 3 },
				{ 100, 3 },
				{ 119, 3 },
				{ 147, 3 },
			},
			[INVENTORY_HAND2] = {
				{ 106, 4 },
				{ 107, 4 },
			},
			[INVENTORY_BODY] = {
				{ 107, 4 },
				{ 113, 4 },
			},
			[INVENTORY_LEGS] = {
				{ 120, 4 },
				{ 122, 4 },
			},
			[INVENTORY_HEAD] = {
				{ 121, 4 },
				{ 201, 4 },
			},
		}
	}
}

-- Vendor to MapID --

Vendors = {
	[2] = 6,
	[3] = 4,
	[4] = 5,
	[5] = 3,
}

-- Bot that runs on the server --

Bot_Server = {}
Bot_Server.GoalState = GOAL_NONE
Bot_Server.MoveState = MOVE_IDLE
Bot_Server.SellSlot = -1
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

	if Object.Health <= 0 then
		Object.Respawn()
	elseif self.GoalState == GOAL_NONE then
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
						Object.UseCommand()
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
		Arrived = self:TraverseMap(Object)
		if Arrived then
			X, Y = Object.FindEvent(4, self.VendorID)
			if X ~= nil then
				if Object.X == X and Object.Y == Y then
					if Object.Status ~= 3 then
						Object.UseCommand()
					else
						Object.VendorExchange(false, 1, self.SellSlot, 1)
						Object.VendorExchange(true, self.BuyID, 1)
						Object.CloseWindows()
						self:DetermineNextGoal(Object)
					end
				else
					Object.FindPath(X, Y)
					self.MoveState = MOVE_PATH
				end
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
		if Object.X == X and Object.Y == Y then
			Object.UseCommand()
		else
			Object.FindPath(X, Y)
			self.MoveState = MOVE_PATH
		end
	end
end

-- Build MapPath list of MapIDs
function Bot_Server.GoToMap(self, Object, MapID)
	self.MapPath = {}
	self.TargetMapID = MapID
	local Last = FindMap(Object.MapID, self.TargetMapID, self.MapPath)
	if Last ~= nil then
		table.insert(self.MapPath, Last)
		--for i = 1, #self.MapPath do print(self.MapPath[i]) end
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
		self.SellSlot = -1

		-- Check item progression
		for CheckType, IDs in pairs(self.Build.Items) do

			-- Get equipped item
			Item = Object.GetInventoryItem(BAG_EQUIPMENT, CheckType)
			SellPrice = 0
			if Item ~= nil then
				SellPrice = math.floor(Item.Cost * 0.5)
			end

			-- Find next item to buy
			for i = #IDs, 1, -1 do

				-- Exit if we already have the item
				if Item ~= nil and Item.ID == IDs[i][1] then
					break
				end

				-- Check price
				if Object.Gold >= Items[IDs[i][1]].Cost then
					self.BuyID = IDs[i][1]
					self.VendorID = IDs[i][2]
					self.SellSlot = CheckType
					break
				end
			end
		end

		if self.BuyID ~= 0 then
			self.GoalState = GOAL_BUY
			self:GoToMap(Object, Vendors[self.VendorID])
		else
			self.GoalState = GOAL_FARMING
			self:GoToMap(Object, 10)
		end
	end

	print("DetermineNextGoal ( goal=" .. self.GoalState .. " gold=" .. Object.Gold .. " buyid=" .. self.BuyID .. " map=" .. Object.MapID .. " x=" .. Object.X .. " y=" .. Object.Y .. " )")
end

-- Bot that simulates a connected client --

Bot_Client = {}
for Key, Value in pairs(Bot_Server) do
	Bot_Client[Key] = Value
end

function Bot_Client.DetermineNextGoal(self, Object)
	self.GoalState = GOAL_FARMING
	self:GoToMap(Object, 10)
end
