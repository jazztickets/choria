/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <bot.h>
#include <network/peer.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/battle.h>
#include <objects/map.h>
#include <objects/inventory.h>
#include <stats.h>
#include <constants.h>
#include <actions.h>
#include <random.h>
#include <scripting.h>
#include <buffer.h>
#include <iostream>
#include <iomanip>

// Constructor
_Bot::_Bot(_Stats *Stats, const std::string &HostAddress, uint16_t Port) :
	Network(new _ClientNetwork()),
	Map(nullptr),
	Battle(nullptr),
	Player(nullptr),
	Stats(Stats),
	Pather(nullptr),
	GoalState(GoalStateType::NONE),
	BotState(BotStateType::IDLE) {

	ObjectManager = new _Manager<_Object>();

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_PATH + SCRIPTS_GAME);

	Username = "a";
	Password = "a";
	Network->Connect(HostAddress, Port);
}

// Destructor
_Bot::~_Bot() {

	delete ObjectManager;
	delete Battle;
	delete Map;
	delete Pather;
	delete Scripting;
}

// Update
void _Bot::Update(double FrameTime) {

	// Update network
	Network->Update(FrameTime);

	// Get events
	_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case _NetworkEvent::CONNECT: {
				std::cout << Username << " connected" << std::endl;

				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
				Packet.WriteBit(0);
				Packet.WriteString(Username.c_str());
				Packet.WriteString(Password.c_str());
				Packet.Write<uint64_t>(0);
				Network->SendPacket(Packet);
			} break;
			case _NetworkEvent::DISCONNECT:
				std::cout << Username << " disconnected" << std::endl;
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
	}

	if(!Player || !Map)
		return;

	// Update goals
	EvaluateGoal();

	// Set input
	if(Player->AcceptingMoveInput()) {
		int InputState = GetNextInputState();

		Player->InputStates.clear();
		if(InputState)
			Player->InputStates.push_back(InputState);
	}

	// Update objects
	ObjectManager->Update(FrameTime);

	// Update map
	Map->Update(FrameTime);

	// Send input to server
	if(Player->Moved) {
		if(Path.size())
			Path.erase(Path.begin());
		//std::cout << Path.size() << std::endl;

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_MOVECOMMAND);
		Packet.Write<char>((char)Player->Moved);
		Network->SendPacket(Packet);
	}

	// Update battle system
	if(Battle) {
		if(!Player->PotentialAction.IsSet()) {
			Battle->ClientHandleInput(_Actions::SKILL1);
			Battle->ClientHandleInput(_Actions::SKILL1);
		}

		if(!Player->Battle) {
			delete Battle;
			Battle = nullptr;
		} else
			Battle->Update(FrameTime);
	}
}

// Handle packet
void _Bot::HandlePacket(_Buffer &Data) {
	PacketType Type = Data.Read<PacketType>();

	//std::cout << (int)Type << std::endl;

	switch(Type) {
		case PacketType::ACCOUNT_SUCCESS: {

			// Request character list
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHARACTERS_REQUEST);
			Network->SendPacket(Packet);
		} break;
		case PacketType::CHARACTERS_LIST: {

			// Get count
			uint8_t CharacterCount = Data.Read<uint8_t>();
			std::cout << "character count: " << (int)CharacterCount << std::endl;

			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHARACTERS_PLAY);
			Packet.Write<uint8_t>(0);
			Network->SendPacket(Packet);
		} break;
		case PacketType::OBJECT_STATS:
			if(Player)
				Player->UnserializeStats(Data);
		break;
		case PacketType::WORLD_CHANGEMAPS: {

			// Load map
			NetworkIDType MapID = (NetworkIDType)Data.Read<uint32_t>();
			double Clock = Data.Read<double>();

			std::cout << "WORLD_CHANGEMAPS " << MapID << std::endl;

			// Delete old map and create new
			if(!Map || Map->NetworkID != MapID) {
				if(Map) {
					delete Pather;
					delete Map;
					Pather = nullptr;
					Map = nullptr;
				}

				Map = new _Map();
				Map->Clock = Clock;
				Map->NetworkID = MapID;
				Map->Load(Stats->GetMap(MapID)->File);
				Player = nullptr;

				Pather = new micropather::MicroPather(Map, (unsigned)(Map->Size.x * Map->Size.y), 4);
			}
		} break;
		case PacketType::WORLD_OBJECTLIST: {

			// Read header
			NetworkIDType ClientNetworkID = Data.Read<NetworkIDType>();
			NetworkIDType ObjectCount = Data.Read<NetworkIDType>();

			// Create objects
			for(NetworkIDType i = 0; i < ObjectCount; i++) {
				NetworkIDType NetworkID = Data.Read<NetworkIDType>();

				// Create object
				_Object *Object = CreateObject(Data, NetworkID);

				// Set player pointer
				if(Object->NetworkID == ClientNetworkID)
					AssignPlayer(Object);
				else
					Object->CalcLevelStats = false;
			}

			if(Player) {
			}
			else {
				// Error
			}
		} break;
		case PacketType::WORLD_CREATEOBJECT: {
			if(!Map || !Player)
				break;

			// Read packet
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();

			// Check id
			if(NetworkID != Player->NetworkID) {

				// Create object
				CreateObject(Data, NetworkID);
			}
		} break;
		case PacketType::WORLD_DELETEOBJECT: {
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();

			// Get object
			_Object *Object = ObjectManager->IDMap[NetworkID];
			if(Object && Object != Player) {
				Object->Deleted = true;
			}
		} break;
		case PacketType::WORLD_OBJECTUPDATES:
			//HandleObjectUpdates(Data);
		break;
		case PacketType::WORLD_POSITION: {
			if(!Player)
				break;

			Player->Position = Data.Read<glm::ivec2>();
			Player->WaitForServer = false;
			Player->TeleportTime = -1;
		} break;
		case PacketType::WORLD_TELEPORTSTART:
			Player->TeleportTime = Data.Read<double>();
		break;
		case PacketType::EVENT_START:
			//HandleEventStart(Data);
		break;
		case PacketType::CHAT_MESSAGE:
			//HandleChatMessage(Data);
		break;
		case PacketType::INVENTORY:
			//HandleInventory(Data);
		break;
		case PacketType::INVENTORY_USE:
			//HandleInventoryUse(Data);
		break;
		case PacketType::INVENTORY_SWAP:
			//HandleInventorySwap(Data);
		break;
		case PacketType::INVENTORY_UPDATE:
			//HandleInventoryUpdate(Data);
		break;
		case PacketType::INVENTORY_GOLD:
			//HandleInventoryGold(Data);
		break;
		case PacketType::TRADE_REQUEST:
			//HandleTradeRequest(Data);
		break;
		case PacketType::TRADE_CANCEL:
			//HandleTradeCancel(Data);
		break;
		case PacketType::TRADE_ITEM:
			//HandleTradeItem(Data);
		break;
		case PacketType::TRADE_GOLD:
			//HandleTradeGold(Data);
		break;
		case PacketType::TRADE_ACCEPT:
			//HandleTradeAccept(Data);
		break;
		case PacketType::TRADE_EXCHANGE:
			//HandleTradeExchange(Data);
		break;
		case PacketType::BATTLE_START: {
			std::cout << "BATTLE_START" << std::endl;

			// Already in a battle
			if(Battle)
				break;

			// Allow player to hit menu buttons
			Player->WaitForServer = false;

			// Create a new battle instance
			Battle = new _Battle();
			Battle->Manager = ObjectManager;
			Battle->Stats = Stats;
			Battle->Scripting = Scripting;
			Battle->ClientPlayer = Player;
			Battle->ClientNetwork = Network.get();

			Battle->Unserialize(Data, nullptr);
		} break;
		case PacketType::BATTLE_ACTION:
			//HandleBattleAction(Data);
		break;
		case PacketType::BATTLE_LEAVE:
			//HandleBattleLeave(Data);
		break;
		case PacketType::BATTLE_END: {
			if(!Player || !Battle)
				return;

			std::cout << "BATTLE_END" << std::endl;

			Player->WaitForServer = false;

			_StatChange StatChange;
			StatChange.Object = Player;

			// Get ending stats
			bool SideDead[2];
			SideDead[0] = Data.ReadBit();
			SideDead[1] = Data.ReadBit();
			int PlayerKills = Data.Read<uint8_t>();
			int MonsterKills = Data.Read<uint8_t>();
			StatChange.Values[StatType::EXPERIENCE].Integer = Data.Read<int32_t>();
			StatChange.Values[StatType::GOLD].Integer = Data.Read<int32_t>();
			uint8_t ItemCount = Data.Read<uint8_t>();
			for(uint8_t i = 0; i < ItemCount; i++) {

				uint32_t ItemID = Data.Read<uint32_t>();
				const _Item *Item = Stats->Items[ItemID];
				int Count = (int)Data.Read<uint8_t>();

				// Add items
				Player->Inventory->AddItem(Item, Count);
			}

			// Check win or death
			int PlayerSide = Player->BattleSide;
			int OtherSide = !PlayerSide;
			if(!SideDead[PlayerSide] && SideDead[OtherSide]) {
				Player->PlayerKills += PlayerKills;
				Player->MonsterKills += MonsterKills;
			}

			Player->Battle = nullptr;
			delete Battle;
			Battle = nullptr;

			DetermineNextGoal();
		} break;
		case PacketType::ACTION_RESULTS: {
			// Create result
			_ActionResult ActionResult;
			bool DecrementItem = Data.ReadBit();
			bool SkillUnlocked = Data.ReadBit();
			bool ItemUnlocked = Data.ReadBit();
			uint32_t ItemID = Data.Read<uint32_t>();
			int InventorySlot = (int)Data.Read<char>();
			ActionResult.ActionUsed.Item = Stats->Items[ItemID];

			// Set texture
			if(ActionResult.ActionUsed.Item)
				ActionResult.Texture = ActionResult.ActionUsed.Item->Texture;

			// Get source change
			HandleStatChange(Data, ActionResult.Source);

			// Update source fighter
			if(ActionResult.Source.Object) {
				ActionResult.Source.Object->TurnTimer = 0.0;
				ActionResult.Source.Object->Action.Unset();
				ActionResult.Source.Object->Targets.clear();

				// Use item on client
				if(Player == ActionResult.Source.Object) {
					if(ActionResult.ActionUsed.Item) {

						if(DecrementItem) {
							size_t Index;
							if(Player->Inventory->FindItem(ActionResult.ActionUsed.Item, Index, (size_t)InventorySlot)) {
								Player->Inventory->DecrementItemCount(Index, -1);
								Player->RefreshActionBarCount();
							}
						}

						if(SkillUnlocked) {
							Player->Skills[ActionResult.ActionUsed.Item->ID] = 0;
						}

						if(ItemUnlocked) {
							Player->Unlocks[ActionResult.ActionUsed.Item->UnlockID].Level = 1;
						}
					}
				}
			}

			// Update targets
			uint8_t TargetCount = Data.Read<uint8_t>();
			for(uint8_t i = 0; i < TargetCount; i++) {
				HandleStatChange(Data, ActionResult.Source);
				HandleStatChange(Data, ActionResult.Target);

				if(Battle) {

					// No damage dealt
					if((ActionResult.ActionUsed.GetTargetType() == TargetType::ENEMY || ActionResult.ActionUsed.GetTargetType() == TargetType::ENEMY_ALL)
					   && ((ActionResult.Target.HasStat(StatType::HEALTH) && ActionResult.Target.Values[StatType::HEALTH].Float == 0.0f) || ActionResult.Target.HasStat(StatType::MISS))) {
						ActionResult.Timeout = HUD_ACTIONRESULT_TIMEOUT_SHORT;
						ActionResult.Speed = HUD_ACTIONRESULT_SPEED_SHORT;
					}
					else {
						ActionResult.Timeout = HUD_ACTIONRESULT_TIMEOUT;
						ActionResult.Speed = HUD_ACTIONRESULT_SPEED;
					}

					Battle->ActionResults.push_back(ActionResult);
				}
				else if(ActionResult.Target.Object == Player) {
				}
			}
		} break;
		case PacketType::STAT_CHANGE: {
			_StatChange StatChange;
			HandleStatChange(Data, StatChange);
		} break;
		case PacketType::WORLD_HUD:
			Player->Health = Data.Read<float>();
			Player->Mana = Data.Read<float>();
			Player->MaxHealth = Data.Read<float>();
			Player->MaxMana = Data.Read<float>();
			Player->Experience = Data.Read<int32_t>();
			Player->Gold = Data.Read<int32_t>();
			Player->CalculateStats();
		break;
		default:
		break;
	}
}

// Handles a stat change
void _Bot::HandleStatChange(_Buffer &Data, _StatChange &StatChange) {
	if(!Player)
		return;

	// Get stats
	StatChange.Unserialize(Data, ObjectManager);
	if(StatChange.Object) {

		// Update object
		StatChange.Object->UpdateStats(StatChange);
	}
}

// Assigns the client player pointer
void _Bot::AssignPlayer(_Object *Object) {
	Player = Object;
	if(Player)
		Player->CalcLevelStats = true;

	if(Battle)
		Battle->ClientPlayer = Player;
}

// Creates an object from a buffer
_Object *_Bot::CreateObject(_Buffer &Data, NetworkIDType NetworkID) {

	// Create object
	_Object *Object = ObjectManager->CreateWithID(NetworkID);
	Object->Scripting = Scripting;
	Object->Stats = Stats;
	Object->Map = Map;
	Object->CalcLevelStats = false;
	Object->UnserializeCreate(Data);

	// Add to map
	Map->AddObject(Object);

	return Object;
}

// Update bot based on goals
void _Bot::EvaluateGoal() {

	// Respawn
	if(!Player->WaitForServer && !Player->IsAlive()) {
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_RESPAWN);
		Network->SendPacket(Packet);

		Player->WaitForServer = true;
		return;
	}

	// Evaluate goal
	switch(GoalState) {
		case GoalStateType::NONE:
			DetermineNextGoal();
		break;
		case GoalStateType::FARMING:
			if(Map->NetworkID != 10) {
				glm::ivec2 Position;
				if(FindEvent(_Event(_Map::EVENT_MAPCHANGE, 10), Position))
					MoveTo(Player->Position, Position);
			}
			else {
				BotState = BotStateType::MOVE_RANDOM;
			}
		break;
		case GoalStateType::HEALING:
			if(Map->NetworkID != 1) {
				if(!Player->WaitForServer && Player->CanTeleport()) {
				   _Buffer Packet;
				   Packet.Write<PacketType>(PacketType::PLAYER_STATUS);
				   Packet.Write<uint8_t>(_Object::STATUS_TELEPORT);
				   Network->SendPacket(Packet);

				   Player->WaitForServer = true;
				}
			}
			else {
				if(Player->GetHealthPercent() < 1.0f) {
					glm::ivec2 Position;
					if(FindEvent(_Event(_Map::EVENT_SCRIPT, 1), Position)) {
						if(Player->Position == Position)
							BotState = BotStateType::MOVE_HEAL;
						else
							MoveTo(Player->Position, Position);
					}
				}
				else
					DetermineNextGoal();
			}
		break;
	}

}

// Set next goal
void _Bot::DetermineNextGoal() {
	if(!Player)
		return;

	if(Player->GetHealthPercent() < 0.5f)
		SetGoal(GoalStateType::HEALING);
	else
		SetGoal(GoalStateType::FARMING);
}

// Find an event in the map
bool _Bot::FindEvent(const _Event &Event, glm::ivec2 &Position) {
	auto Iterator = Map->IndexedEvents.find(Event);
	if(Iterator == Map->IndexedEvents.end())
		return false;

	Position = Iterator->second;

	return true;
}

// Create list of nodes to destination
void _Bot::MoveTo(const glm::ivec2 &StartPosition, const glm::ivec2 &EndPosition) {
	if(!Pather)
		return;

	float TotalCost;
	std::vector<void *> PathFound;
	int Result = Pather->Solve(Map->PositionToNode(StartPosition), Map->PositionToNode(EndPosition), &PathFound, &TotalCost);
	if(Result == micropather::MicroPather::SOLVED) {

		// Convert vector to list
		Path.clear();
		for(auto &Node : PathFound)
			Path.push_back(Node);

		BotState = BotStateType::MOVE_PATH;
	}
}

// Set the player input state based on pathfound
int _Bot::GetNextInputState() {
	int InputState = 0;
	if(!Map || !Player)
		return InputState;

	switch(BotState) {
		case BotStateType::IDLE:
		break;
		case BotStateType::MOVE_HEAL: {
			return _Object::MOVE_LEFT;
		}
		case BotStateType::MOVE_PATH: {
			if(Path.size() == 0) {
				BotState = BotStateType::IDLE;
				return InputState;
			}

			// Find current position in list
			for(auto Iterator = Path.begin(); Iterator != Path.end(); ++Iterator) {
				glm::ivec2 NodePosition;
				Map->NodeToPosition(*Iterator, NodePosition);

				if(Player->Position == NodePosition) {
					auto NextIterator = std::next(Iterator, 1);
					if(NextIterator == Path.end()) {
						Path.clear();
						return 0;
					}

					// Get next node position
					Map->NodeToPosition(*NextIterator, NodePosition);

					// Get direction to next node
					glm::ivec2 Direction = NodePosition - Player->Position;
					if(Direction.x < 0)
						InputState = _Object::MOVE_LEFT;
					else if(Direction.x > 0)
						InputState = _Object::MOVE_RIGHT;
					else if(Direction.y < 0)
						InputState = _Object::MOVE_UP;
					else if(Direction.y > 0)
						InputState = _Object::MOVE_DOWN;
					break;
				}
			}

		} break;
		case BotStateType::MOVE_RANDOM: {
			InputState = 1 << GetRandomInt(0, 3);
			glm::ivec2 Direction;
			Player->GetDirectionFromInput(InputState, Direction);
			if(Map->GetTile(Player->Position + Direction)->Event.Type != _Map::EVENT_NONE)
				InputState = 0;
		} break;
	}

	return InputState;
}

// Set goal of bot
void _Bot::SetGoal(GoalStateType NewGoal) {
	std::cout << "SetGoal=" << (int)NewGoal << std::endl;
	GoalState = NewGoal;
}
