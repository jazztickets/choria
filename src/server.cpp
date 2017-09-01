/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <server.h>
#include <ae/servernetwork.h>
#include <ae/peer.h>
#include <ae/manager.h>
#include <ae/database.h>
#include <ae/buffer.h>
#include <ae/util.h>
#include <objects/object.h>
#include <objects/components/inventory.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <objects/minigame.h>
#include <scripting.h>
#include <save.h>
#include <packet.h>
#include <stats.h>
#include <constants.h>
#include <config.h>
#include <SDL_timer.h>
#include <enet/enet.h>
#include <algorithm>
#include <regex>

// Function to run the server thread
static void RunThread(void *Arguments) {

	// Get server object
	_Server *Server = (_Server *)Arguments;

	// Init timer
	Uint64 Timer = SDL_GetPerformanceCounter();
	double TimeStepAccumulator = 0.0;
	double TimeStep = DEFAULT_TIMESTEP;
	while(!Server->Done) {
		double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
		Timer = SDL_GetPerformanceCounter();

		// Run server
		TimeStepAccumulator += FrameTime * Config.TimeScale;
		while(TimeStepAccumulator >= TimeStep) {
			Server->Update(TimeStep);
			TimeStepAccumulator -= TimeStep;
		}

		// Sleep thread
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

// Constructor
_Server::_Server(uint16_t NetworkPort) :
	IsTesting(false),
	Hardcore(false),
	Done(false),
	StartDisconnect(false),
	StartShutdown(false),
	TimeSteps(0),
	Time(0.0),
	SaveTime(0.0),
	BotTime(0.0),
	Network(new _ServerNetwork(Config.MaxClients, NetworkPort)),
	Thread(nullptr) {

	if(!Network->HasConnection())
		throw std::runtime_error("Unable to bind address!");

	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	ObjectManager = new _Manager<_Object>();
	MapManager = new _Manager<_Map>();
	BattleManager = new _Manager<_Battle>();
	Stats = new _Stats(true);
	Save = new _Save();

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_GAME);

	Log.Open((Config.LogPath + "server.log").c_str());
	Log << "Listening on port " << NetworkPort << std::endl;
}

// Destructor
_Server::~_Server() {
	Done = true;
	JoinThread();

	// Save clock
	Save->SaveSettings();

	// Save players
	Save->StartTransaction();
	for(auto &Object : ObjectManager->Objects)
		Save->SavePlayer(Object, Object->GetMapID(), &Log);
	Save->EndTransaction();

	delete MapManager;
	delete BattleManager;
	delete ObjectManager;
	delete Scripting;
	delete Save;
	delete Stats;
	delete Thread;

	Log << "Stopping server" << std::endl;
}

// Start the server thread
void _Server::StartThread() {
	Thread = new std::thread(RunThread, this);
}

// Wait for thread to join
void _Server::JoinThread() {
	if(Thread)
		Thread->join();

	delete Thread;
	Thread = nullptr;
}

// Stop the server
void _Server::StopServer() {
	StartDisconnect = true;
}

// Update
void _Server::Update(double FrameTime) {
	//if(std::abs(std::fmod(Time, 1.0)) >= 0.99)
	//	std::cout << "Server: O=" << ObjectManager->Objects.size() << " B=" << BattleManager->Objects.size() << std::endl;

	// Update network
	Network->Update(FrameTime);

	// Get events
	_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case _NetworkEvent::CONNECT:
				HandleConnect(NetworkEvent);
			break;
			case _NetworkEvent::DISCONNECT:
				HandleDisconnect(NetworkEvent);
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data, NetworkEvent.Peer);
				delete NetworkEvent.Data;
			break;
		}
	}

	// Update objects
	ObjectManager->Update(FrameTime);

	// Spawn battles
	for(auto &BattleEvent : BattleEvents)
		StartBattle(BattleEvent);

	BattleEvents.clear();

	// Update maps
	MapManager->Update(FrameTime);

	// Update battles
	BattleManager->Update(FrameTime);

	// Check if updates should be sent
	if(Network->NeedsUpdate()) {
		Network->ResetUpdateTimer();
		if(Network->GetPeers().size() > 0) {

			// Send object updates
			for(auto &Map : MapManager->Objects) {
				Map->SendObjectUpdates();
			}
		}
	}

	// Wait for peers to disconnect
	if(StartDisconnect) {
		Network->DisconnectAll();
		StartDisconnect = false;
		StartShutdown = true;
	}
	else if(StartShutdown && Network->GetPeers().size() == 0) {
		Done = true;
	}

	TimeSteps++;
	Time += FrameTime;

	// Update scripting environment
	Scripting->InjectTime(Time);

	// Update clock
	Save->Clock += FrameTime * MAP_CLOCK_SPEED;
	if(Save->Clock >= MAP_DAY_LENGTH)
		Save->Clock -= MAP_DAY_LENGTH;

	// Update autosave
	SaveTime += FrameTime;
	if(Config.AutoSavePeriod > 0 && SaveTime >= Config.AutoSavePeriod) {
		SaveTime = 0;

		// Save players
		Save->StartTransaction();
		for(auto &Object : ObjectManager->Objects)
			Save->SavePlayer(Object, Object->GetMapID(), &Log);
		Save->EndTransaction();
	}

	// Update bot timer
	if(BotTime >= 0)
		BotTime += FrameTime;

	// Spawn bot
	if(1 && IsTesting && BotTime > 2.1) {
		BotTime = -1;
		CreateBot();
	}
}

// Handle client connect
void _Server::HandleConnect(_NetworkEvent &Event) {
	char Buffer[16];
	ENetAddress *Address = &Event.Peer->ENetPeer->address;
	enet_address_get_host_ip(Address, Buffer, 16);
	Log << "Connect " << Buffer << ":" << Address->port << std::endl;

	// Send game version
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::VERSION);
	Packet.WriteString(GAME_VERSION);
	Network->SendPacket(Packet, Event.Peer);
}

// Handle client disconnect
void _Server::HandleDisconnect(_NetworkEvent &Event) {
	char Buffer[16];
	ENetAddress *Address = &Event.Peer->ENetPeer->address;
	enet_address_get_host_ip(Address, Buffer, 16);
	Log << "Disconnect " << Buffer << ":" << Address->port << std::endl;

	_Buffer Data;
	HandleExit(Data, Event.Peer);

	// Delete peer from network
	Network->DeletePeer(Event.Peer);
}

// Handle packet data
void _Server::HandlePacket(_Buffer &Data, _Peer *Peer) {
	PacketType Type = Data.Read<PacketType>();

	switch(Type) {
		case PacketType::ACCOUNT_LOGININFO:
			HandleLoginInfo(Data, Peer);
		break;
		case PacketType::CHARACTERS_REQUEST:
			HandleCharacterListRequest(Data, Peer);
		break;
		case PacketType::CHARACTERS_PLAY:
			HandleCharacterPlay(Data, Peer);
		break;
		case PacketType::CREATECHARACTER_INFO:
			HandleCharacterCreate(Data, Peer);
		break;
		case PacketType::CHARACTERS_DELETE:
			HandleCharacterDelete(Data, Peer);
		break;
		case PacketType::WORLD_MOVECOMMAND:
			HandleMoveCommand(Data, Peer);
		break;
		case PacketType::WORLD_USECOMMAND:
			HandleUseCommand(Data, Peer);
		break;
		case PacketType::WORLD_RESPAWN:
			HandleRespawn(Data, Peer);
		break;
		case PacketType::WORLD_JOIN:
			HandleJoin(Data, Peer);
		break;
		case PacketType::WORLD_EXIT:
			HandleExit(Data, Peer);
		break;
		case PacketType::ACTION_USE:
			HandleActionUse(Data, Peer);
		break;
		case PacketType::CHAT_MESSAGE:
			HandleChatMessage(Data, Peer);
		break;
		case PacketType::BLACKSMITH_UPGRADE:
			HandleBlacksmithUpgrade(Data, Peer);
		break;
		case PacketType::MINIGAME_PAY:
			HandleMinigamePay(Data, Peer);
		break;
		case PacketType::MINIGAME_GETPRIZE:
			HandleMinigameGetPrize(Data, Peer);
		break;
		case PacketType::INVENTORY_MOVE:
			HandleInventoryMove(Data, Peer);
		break;
		case PacketType::INVENTORY_USE:
			HandleInventoryUse(Data, Peer);
		break;
		case PacketType::INVENTORY_SPLIT:
			HandleInventorySplit(Data, Peer);
		break;
		case PacketType::VENDOR_EXCHANGE:
			HandleVendorExchange(Data, Peer);
		break;
		case PacketType::TRADER_ACCEPT:
			HandleTraderAccept(Data, Peer);
		break;
		case PacketType::ACTIONBAR_CHANGED:
			HandleActionBarChanged(Data, Peer);
		break;
		case PacketType::SKILLS_SKILLADJUST:
			HandleSkillAdjust(Data, Peer);
		break;
		case PacketType::TRADE_REQUEST:
			HandleTradeRequest(Data, Peer);
		break;
		case PacketType::TRADE_CANCEL:
			HandleTradeCancel(Data, Peer);
		break;
		case PacketType::TRADE_GOLD:
			HandleTradeGold(Data, Peer);
		break;
		case PacketType::TRADE_ACCEPT:
			HandleTradeAccept(Data, Peer);
		break;
		case PacketType::PARTY_INFO:
			HandlePartyInfo(Data, Peer);
		break;
		case PacketType::PLAYER_STATUS:
			HandlePlayerStatus(Data, Peer);
		break;
		default:
		break;
	}
}

// Send an item to the player
void _Server::SendItem(_Peer *Peer, const _Item *Item, int Count) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player || !Item)
		return;

	// Add item
	Player->Inventory->AddItem(Item, 0, Count);

	// Send item
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_ADD);
	Packet.Write<uint8_t>((uint8_t)Count);
	Packet.Write<uint32_t>(Item->ID);
	Player->Inventory->Serialize(Packet);
	Network->SendPacket(Packet, Peer);

	// Update states
	Player->CalculateStats();
	SendHUD(Player->Peer);
}

// Login information
void _Server::HandleLoginInfo(_Buffer &Data, _Peer *Peer) {

	// Read packet
	bool CreateAccount = Data.ReadBit();
	std::string Username(Data.ReadString());
	std::string Password(Data.ReadString());
	uint64_t Secret = Data.Read<uint64_t>();

	Username.resize(std::min(ACCOUNT_MAX_USERNAME_SIZE, Username.length()));
	Password.resize(std::min(ACCOUNT_MAX_PASSWORD_SIZE, Password.length()));

	// Validate singleplayer
	if(!Password.length() && Secret != Save->Secret)
		return;

	// Create account or login
	if(CreateAccount) {

		// Check for existing account
		if(Save->CheckUsername(Username)) {
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::ACCOUNT_EXISTS);
			Network->SendPacket(Packet, Peer);
			return;
		}
		else
			Save->CreateAccount(Username, Password);
	}

	// Get account id
	Peer->AccountID = Save->GetAccountID(Username, Password);

	// Make sure account exists
	_Buffer Packet;
	if(Peer->AccountID == 0) {
		Packet.Write<PacketType>(PacketType::ACCOUNT_NOTFOUND);
	}
	else {

		// Check for account already being used
		bool AccountInUse = CheckAccountUse(Peer);
		if(AccountInUse) {
			Peer->AccountID = 0;
			Packet.Write<PacketType>(PacketType::ACCOUNT_INUSE);
		}
		else
			Packet.Write<PacketType>(PacketType::ACCOUNT_SUCCESS);
	}

	Network->SendPacket(Packet, Peer);
}

// Sends a player his/her character list
void _Server::HandleCharacterListRequest(_Buffer &Data, _Peer *Peer) {
	if(!Peer->AccountID)
		return;

	SendCharacterList(Peer);
}

// Handles the character create request
void _Server::HandleCharacterCreate(_Buffer &Data, _Peer *Peer) {
	if(!Peer->AccountID)
		return;

	// Get character information
	bool IsHardcore = Data.ReadBit();
	std::string Name(Data.ReadString());
	uint32_t PortraitID = Data.Read<uint32_t>();
	uint32_t BuildID = Data.Read<uint32_t>();
	uint32_t Slot = Data.Read<uint8_t>();
	if(Name.size() > PLAYER_NAME_SIZE)
		return;

	// Check number of characters in account
	if(Save->GetCharacterCount(Peer->AccountID) >= ACCOUNT_MAX_CHARACTER_SLOTS)
		return;

	// Found an existing name
	if(Save->GetCharacterIDByName(Name) != 0) {
		_Buffer NewPacket;
		NewPacket.Write<PacketType>(PacketType::CREATECHARACTER_INUSE);
		Network->SendPacket(NewPacket, Peer);
		return;
	}

	// Create the character
	Save->CreateCharacter(Stats, Scripting, Peer->AccountID, Slot, IsHardcore, Name, PortraitID, BuildID);

	// Notify the client
	_Buffer NewPacket;
	NewPacket.Write<PacketType>(PacketType::CREATECHARACTER_SUCCESS);
	Network->SendPacket(NewPacket, Peer);
}

// Handle a character delete request
void _Server::HandleCharacterDelete(_Buffer &Data, _Peer *Peer) {
	if(!Peer->AccountID)
		return;

	// Get delete slot
	uint32_t Slot = Data.Read<uint8_t>();
	uint32_t CharacterID = 0;

	// Get character id
	CharacterID = Save->GetCharacterIDBySlot(Peer->AccountID, Slot);
	if(!CharacterID)
		return;

	// Delete character
	Save->DeleteCharacter(CharacterID);

	// Update the player
	SendCharacterList(Peer);
}

// Loads the player, updates the world, notifies clients
void _Server::HandleCharacterPlay(_Buffer &Data, _Peer *Peer) {

	// Read packet
	uint32_t Slot = Data.Read<uint8_t>();

	// Check for valid character id
	if(!Peer->Object) {
		Peer->CharacterID = Save->GetCharacterID(Peer->AccountID, Slot);
		if(!Peer->CharacterID)
			return;

		// Create player
		Peer->Object = CreatePlayer(Peer);
	}

	// Send map and players to new player
	SpawnPlayer(Peer->Object, Peer->Object->LoadMapID, _Map::EVENT_NONE);

	// Broadcast message
	std::string Message = Peer->Object->Name + " has joined the server";
	BroadcastMessage(Peer, Message, "gray");

	// Log
	Log << "Player " << Message << " ( action=join character_id=" << Peer->CharacterID << " )" << std::endl;
}

// Handles move commands from a client
void _Server::HandleMoveCommand(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->IsAlive())
		return;

	Player->InputStates.push_back(Data.Read<char>());
}

// Handles use command from a client
void _Server::HandleUseCommand(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->IsAlive())
		return;

	Player->UseCommand = true;
}

// Handle respawn command from client
void _Server::HandleRespawn(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	// Check death
	if(!Player->IsAlive()) {
		if(Player->Hardcore)
			return;

		// Wait for battle to finish
		if(Player->Battle)
			return;

		Player->Fighter->Health = Player->Fighter->MaxHealth / 2;
		Player->Fighter->Mana = Player->Fighter->MaxMana / 2;
		SpawnPlayer(Player, Player->SpawnMapID, _Map::EVENT_SPAWN);
	}
}

// Handle a chat message
void _Server::HandleChatMessage(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	// Get message
	std::string Message = Data.ReadString();
	if(Message.length() == 0)
		return;

	// Resize large messages
	if(Message.length() > HUD_CHAT_SIZE)
		Message.resize(HUD_CHAT_SIZE);

	// Check for test commands
	if(IsTesting && Message[0] == '-' && !Player->Battle) {
		_StatChange StatChange;
		StatChange.Object = Player;

		std::smatch Match;
		if(Message.find("-give") == 0) {
			std::regex Regex("-give ([0-9]+) ([0-9]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 2) {
				uint32_t ItemID = (uint32_t)ToNumber<int>(Match.str(1));
				int Count = std::min(ToNumber<int>(Match.str(2)), 255);

				if(Stats->Items.find(ItemID) == Stats->Items.end())
					return;

				SendItem(Peer, Stats->Items.at(ItemID), Count);
			}
		}
		else if(Message.find("-setgold") == 0) {
			std::regex Regex("-setgold ([0-9-]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 1) {
				Player->Gold = ToNumber<int>(Match.str(1));
				SendHUD(Peer);
			}
		}
		else if(Message.find("-gold") == 0) {
			std::regex Regex("-gold ([0-9-]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 1) {
				StatChange.Values[StatType::GOLD].Integer = ToNumber<int>(Match.str(1));
				Player->UpdateStats(StatChange);
			}
		}
		else if(Message.find("-bounty") == 0) {
			std::regex Regex("-bounty ([0-9-]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 1) {
				Player->Bounty = std::max(0, ToNumber<int>(Match.str(1)));
				SendHUD(Peer);
			}
		}
		else if(Message.find("-exp") == 0) {
			std::regex Regex("-exp ([0-9-]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 1) {
				StatChange.Values[StatType::EXPERIENCE].Integer = ToNumber<int>(Match.str(1));
				Player->UpdateStats(StatChange);
			}
		}
		else if(Message.find("-clock") == 0) {
			std::regex Regex("-clock ([0-9.]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 1) {
				Save->Clock = (double)ToNumber<double>(Match.str(1));
				if(Save->Clock < 0)
					Save->Clock = 0;
				else if(Save->Clock >= MAP_DAY_LENGTH)
					Save->Clock = MAP_DAY_LENGTH;

				SendHUD(Player->Peer);
			}
		}
		else if(Message.find("-battle") == 0) {
			std::regex Regex("-battle ([0-9]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 1) {
				uint32_t ZoneID = ToNumber<uint32_t>(Match.str(1));
				QueueBattle(Player, ZoneID, false, false);
			}
		}
		else if(Message.find("-map") == 0) {
			std::regex Regex("-map ([0-9]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 1) {
				NetworkIDType MapID = ToNumber<NetworkIDType>(Match.str(1));
				SpawnPlayer(Player, MapID, _Map::EVENT_MAPENTRANCE);
			}
		}
		else if(Message.find("-pos") == 0) {
			std::regex Regex("-pos ([0-9]+) ([0-9]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 2) {
				if(!Player->Map)
					return;

				Player->Position = Player->Map->GetValidCoord(glm::ivec2(ToNumber<int>(Match.str(1)), ToNumber<int>(Match.str(2))));
				SendPlayerPosition(Player->Peer);
			}
		}
		else if(Message.find("-event") == 0) {
			std::regex Regex("-event ([0-9]+) ([0-9]+)");
			if(std::regex_search(Message, Match, Regex) && Match.size() > 2) {
				if(!Player->Map)
					return;

				Player->Map->StartEvent(Player, _Event(ToNumber<uint32_t>(Match.str(1)), ToNumber<uint32_t>(Match.str(2))));
			}
		}

		// Build packet
		if(StatChange.GetChangedFlag()) {
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::STAT_CHANGE);
			StatChange.Serialize(Packet);
			Network->SendPacket(Packet, Player->Peer);
		}

		// Update client
		Player->CalculateStats();
		SendHUD(Player->Peer);

		return;
	}

	// Send message to other players
	BroadcastMessage(nullptr, Player->Name + ": " + Message, "white");
}

// Send position to player
void _Server::SendPlayerPosition(_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_POSITION);
	Packet.Write<glm::ivec2>(Player->Position);

	Network->SendPacket(Packet, Player->Peer);
}

// Send player stats to peer
void _Server::SendPlayerInfo(_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Build packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::OBJECT_STATS);
	Player->SerializeStats(Packet);

	Network->SendPacket(Packet, Peer);
}

// Send character list
void _Server::SendCharacterList(_Peer *Peer) {

	// Create packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHARACTERS_LIST);
	Packet.Write<uint8_t>(Hardcore);
	Packet.Write<uint8_t>((uint8_t)Save->GetCharacterCount(Peer->AccountID));

	// Generate a list of characters
	Save->Database->PrepareQuery("SELECT * FROM character WHERE account_id = @account_id");
	Save->Database->BindInt(1, Peer->AccountID);
	while(Save->Database->FetchRow()) {
		_Object Player;
		Player.Stats = Stats;
		Player.UnserializeSaveData(Save->Database->GetString("data"));
		Packet.Write<uint8_t>(Save->Database->GetInt<uint8_t>("slot"));
		Packet.Write<uint8_t>(Player.Hardcore);
		Packet.WriteString(Save->Database->GetString("name"));
		Packet.Write<uint32_t>(Player.PortraitID);
		Packet.Write<int>(Player.Fighter->Health);
		Packet.Write<int>(Player.Fighter->Experience);
	}
	Save->Database->CloseQuery();

	// Send list
	Network->SendPacket(Packet, Peer);
}

// Spawns a player at a particular spawn point
void _Server::SpawnPlayer(_Object *Player, NetworkIDType MapID, uint32_t EventType) {
	if(!Stats)
		return;

	if(!ValidatePeer(Player->Peer) || !Player->Peer->CharacterID)
	   return;

	// Use spawn point for new characters
	if(MapID == 0) {
		MapID = Player->SpawnMapID;
		if(MapID == 0)
			MapID = 1;
		EventType = _Map::EVENT_SPAWN;
	}
	// Verify map id
	else if(Stats->Maps.find(MapID) == Stats->Maps.end() || Stats->Maps.at(MapID).File == "maps/")
		return;

	// Get map
	_Map *Map = MapManager->GetObject(MapID);

	// Load map
	if(!Map) {
		Map = MapManager->CreateWithID(MapID);
		Map->Clock = Save->Clock;
		Map->Server = this;
		Map->Load(&Stats->Maps.at(MapID));
	}

	// Get old map
	_Map *OldMap = Player->Map;

	// Place player in new map
	if(Map != OldMap) {
		if(OldMap)
			OldMap->RemoveObject(Player);

		Player->Map = Map;

		// Check for spawning from events
		if(EventType != _Map::EVENT_NONE) {

			// Find spawn point in map
			uint32_t SpawnPoint = Player->SpawnPoint;
			if(EventType == _Map::EVENT_MAPENTRANCE)
				SpawnPoint = OldMap->NetworkID;

			// Default to mapchange event if entrance not found
			if(!Map->FindEvent(_Event(EventType, SpawnPoint), Player->Position))
				Map->FindEvent(_Event(_Map::EVENT_MAPCHANGE, SpawnPoint), Player->Position);
		}

		// Add player to map
		Map->AddObject(Player);

		// Send data to peer
		if(Player->Peer->ENetPeer) {

			// Send new map id
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::WORLD_CHANGEMAPS);
			Packet.Write<uint32_t>(MapID);
			Packet.Write<double>(Save->Clock);
			Network->SendPacket(Packet, Player->Peer);

			// Send player object list
			Map->SendObjectList(Player->Peer);

			// Send full player data to peer
			SendPlayerInfo(Player->Peer);
		}
		else
			Player->Path.clear();
	}
	else {
		Map->FindEvent(_Event(EventType, Player->SpawnPoint), Player->Position);
		SendPlayerPosition(Player->Peer);
		SendHUD(Player->Peer);
	}
}

// Queue a battle for an object
void _Server::QueueBattle(_Object *Object, uint32_t Zone, bool Scripted, int PVP) {
	_BattleEvent BattleEvent;
	BattleEvent.Object = Object;
	BattleEvent.Zone = Zone;
	BattleEvent.Scripted = Scripted;
	BattleEvent.PVP = PVP;

	BattleEvents.push_back(BattleEvent);
}

// Start teleporting a player
void _Server::StartTeleport(_Object *Object, double Time) {
	if(Object->Battle || !Object->IsAlive())
		return;

	Object->ResetUIState();
	Object->TeleportTime = Time;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_TELEPORTSTART);
	Packet.Write<double>(Time);
	Network->SendPacket(Packet, Object->Peer);
}

// Create player object and load stats from save
_Object *_Server::CreatePlayer(_Peer *Peer) {

	// Create object
	_Object *Player = ObjectManager->Create();
	Player->Scripting = Scripting;
	Player->Server = this;
	Player->CharacterID = Peer->CharacterID;
	Player->Peer = Peer;
	Player->Stats = Stats;
	Peer->Object = Player;

	Save->LoadPlayer(Stats, Player);

	return Player;
}

// Create server side bot
_Object *_Server::CreateBot() {

	// Check for account being used
	_Peer TestPeer(nullptr);
	TestPeer.AccountID = ACCOUNT_BOTS_ID;
	if(CheckAccountUse(&TestPeer))
		return nullptr;

	// Choose slot
	uint32_t Slot = 0;

	// Check for valid character id
	uint32_t CharacterID = Save->GetCharacterID(ACCOUNT_BOTS_ID, Slot);
	if(!CharacterID) {
		std::string Name = "bot_test";
		CharacterID = Save->CreateCharacter(Stats, Scripting, ACCOUNT_BOTS_ID, Slot, Hardcore, Name, 1, 1);
	}

	// Create object
	_Object *Bot = ObjectManager->Create();
	Bot->Bot = true;
	Bot->Scripting = Scripting;
	Bot->Server = this;
	Bot->CharacterID = CharacterID;
	Bot->Stats = Stats;
	Save->LoadPlayer(Stats, Bot);

	// Create fake peer
	Bot->Peer = new _Peer(nullptr);
	Bot->Peer->Object = Bot;
	Bot->Peer->CharacterID = CharacterID;
	Bot->Peer->AccountID = ACCOUNT_BOTS_ID;

	// Simulate packet
	_Buffer Packet;
	Packet.Write<uint8_t>(0);
	HandleCharacterPlay(Packet, Bot->Peer);

	return Bot;
}

// Validate a peer's attributes
bool _Server::ValidatePeer(_Peer *Peer) {
	if(!Peer)
		return false;

	if(!Peer->AccountID)
		return false;

	if(!Peer->Object)
		return false;

	return true;
}

// Check to see if an account is in use
bool _Server::CheckAccountUse(_Peer *Peer) {
	for(auto &CheckPeer : Network->GetPeers()) {
		if(CheckPeer != Peer && CheckPeer->AccountID == Peer->AccountID)
			return true;
	}

	return false;
}

// Handles a player's inventory move
void _Server::HandleInventoryMove(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get slots
	_Slot OldSlot;
	_Slot NewSlot;
	OldSlot.Unserialize(Data);
	NewSlot.Unserialize(Data);

	// Move items
	{
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::INVENTORY_SWAP);
		if(Player->Inventory->MoveInventory(Packet, OldSlot, NewSlot)) {
			Network->SendPacket(Packet, Peer);
			Player->CalculateStats();
		}
	}

	// Check for trading players
	_Object *TradePlayer = Player->TradePlayer;
	if(Player->WaitingForTrade && TradePlayer && (OldSlot.BagType == _Bag::BagType::TRADE || NewSlot.BagType == _Bag::BagType::TRADE)) {

		// Reset agreement
		Player->TradeAccepted = false;
		TradePlayer->TradeAccepted = false;

		// Build packet
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_ITEM);

		// Write slots
		Player->Inventory->SerializeSlot(Packet, NewSlot);
		Player->Inventory->SerializeSlot(Packet, OldSlot);

		// Send updates
		Network->SendPacket(Packet, TradePlayer->Peer);
	}
}

// Handle a player's inventory use request
void _Server::HandleInventoryUse(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get item slot index
	_Slot Slot;
	Slot.Unserialize(Data);
	if(!Player->Inventory->IsValidSlot(Slot))
		return;

	// Get item
	const _Item *Item = Player->Inventory->GetSlot(Slot).Item;
	if(!Item)
		return;

	// Check for equipment
	if(Item->IsEquippable()) {
		_Slot TargetSlot;
		Item->GetEquipmentSlot(TargetSlot);

		// Check for empty second ring slot
		if(TargetSlot.Index == EquipmentType::RING1 && Player->Inventory->GetSlot(TargetSlot).Item && !Player->Inventory->Bags[_Bag::BagType::EQUIPMENT].Slots[EquipmentType::RING2].Item)
			TargetSlot.Index = EquipmentType::RING2;

		// Attempt to move
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::INVENTORY_SWAP);
		if(Player->Inventory->MoveInventory(Packet, Slot, TargetSlot)) {
			Network->SendPacket(Packet, Peer);
			Player->CalculateStats();
		}
	}
	// Handle consumables
	else {

		// Check for existing action
		if(!Player->Action.IsSet()) {
			Player->Targets.clear();
			Player->Targets.push_back(Player);
			Player->Action.Item = Item;
			Player->Action.Level = Item->Level;
			Player->Action.InventorySlot = (int)Slot.Index;
		}
	}
}

// Handle a player's inventory split stack request
void _Server::HandleInventorySplit(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	_Slot Slot;
	Slot.Unserialize(Data);
	uint8_t Count = Data.Read<uint8_t>();

	// Inventory only
	if(Slot.BagType == _Bag::BagType::TRADE)
		return;

	// Split items
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
	if(Player->Inventory->SplitStack(Packet, Slot, Count))
		Network->SendPacket(Packet, Peer);
}

// Handles a vendor exchange message
void _Server::HandleVendorExchange(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get vendor
	const _Vendor *Vendor = Player->Vendor;
	if(!Vendor)
		return;

	// Get info
	bool Buy = Data.ReadBit();
	int Amount = (int)Data.Read<uint8_t>();
	_Slot Slot;
	Slot.Unserialize(Data);

	// Buy item
	if(Buy) {
		if(Slot.Index >= Vendor->Items.size())
			return;

		// Get optional inventory slot
		_Slot TargetSlot;
		TargetSlot.Unserialize(Data);

		// Get item info
		const _Item *Item = Vendor->Items[Slot.Index];
		int Price = Item->GetPrice(Vendor, Amount, Buy);

		// Not enough gold
		if(Price > Player->Gold)
			return;

		// Find open slot for new item
		if(!Player->Inventory->IsValidSlot(TargetSlot))
			TargetSlot = Player->Inventory->FindSlotForItem(Item, 0, Amount);

		// No room
		if(!Player->Inventory->IsValidSlot(TargetSlot))
			return;

		// Attempt to add item
		if(!Player->Inventory->AddItem(Item, 0, Amount, TargetSlot))
			return;

		// Update gold
		Player->UpdateGold(-Price);
		if(Peer) {
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::INVENTORY_GOLD);
			Packet.Write<int>(Player->Gold);
			Network->SendPacket(Packet, Peer);
		}

		// Update items
		if(Peer) {
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
			Packet.Write<uint8_t>(1);
			Player->Inventory->SerializeSlot(Packet, TargetSlot);
			Network->SendPacket(Packet, Peer);
		}

		Player->CalculateStats();

		// Log
		Log << "Player " << Player->Name << " buys " << (int)Amount << "x " << Item->Name << " ( action=buy character_id=" << Peer->CharacterID << " item_id=" << Item->ID << " gold=" << Player->Gold << " )" << std::endl;
	}
	// Sell item
	else {
		if(!Player->Inventory->IsValidSlot(Slot))
			return;

		// Get item info
		const _InventorySlot &InventorySlot = Player->Inventory->GetSlot(Slot);
		if(InventorySlot.Item) {

			// Get price of stack
			Amount = std::min((int)Amount, InventorySlot.Count);
			int Price = InventorySlot.Item->GetPrice(Vendor, Amount, Buy);

			// Update gold
			Player->UpdateGold(Price);
			if(Peer) {
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::INVENTORY_GOLD);
				Packet.Write<int>(Player->Gold);
				Network->SendPacket(Packet, Peer);
			}

			// Log
			Log << "Player " << Player->Name << " sells " << Amount << "x " << InventorySlot.Item->Name << " ( action=sell character_id=" << Peer->CharacterID << " item_id=" << InventorySlot.Item->ID << " gold=" << Player->Gold << " )" << std::endl;

			// Update items
			Player->Inventory->DecrementItemCount(Slot, -Amount);
			if(Peer) {
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
				Packet.Write<uint8_t>(1);
				Player->Inventory->SerializeSlot(Packet, Slot);
				Network->SendPacket(Packet, Peer);
			}
		}
	}
}

// Handles a trader accept
void _Server::HandleTraderAccept(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get trader information
	std::vector<_Slot> RequiredItemSlots(Player->Trader->Items.size());
	_Slot RewardSlot = Player->Inventory->GetRequiredItemSlots(Player->Trader, RequiredItemSlots);
	if(!Player->Inventory->IsValidSlot(RewardSlot))
		return;

	// Update items
	Player->AcceptTrader(RequiredItemSlots);

	// Send new inventory
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY);
	Player->Inventory->Serialize(Packet);
	Network->SendPacket(Packet, Peer);
}

// Handles a skill adjust
void _Server::HandleSkillAdjust(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Process packet
	uint32_t SkillID = Data.Read<uint32_t>();
	int Amount = Data.Read<int>();

	// Check respec condition
	if(Amount < 0 && !Player->CanRespec())
		return;

	// Update values
	Player->AdjustSkillLevel(SkillID, Amount);
	Player->CalculateStats();
}

// Handle a trade request
void _Server::HandleTradeRequest(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get map object
	_Map *Map = Player->Map;
	if(!Map)
		return;

	// Set status
	Player->WaitingForTrade = true;

	// Find the nearest player to trade with
	_Object *TradePlayer = Map->FindTradePlayer(Player, 2.0f * 2.0f);
	if(TradePlayer == nullptr) {

		// Set up trade post
		Player->TradeGold = 0;
		Player->TradeAccepted = false;
		Player->TradePlayer = nullptr;
	}
	else {

		// Set up trade screen for both players
		SendTradeInformation(Player, TradePlayer);
		SendTradeInformation(TradePlayer, Player);

		Player->TradePlayer = TradePlayer;
		Player->TradeAccepted = false;
		TradePlayer->TradePlayer = Player;
		TradePlayer->TradeAccepted = false;
		TradePlayer->WaitingForTrade = true;
	}
}

// Handles a trade cancel
void _Server::HandleTradeCancel(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Notify trading player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradePlayer = nullptr;
		TradePlayer->TradeAccepted = false;

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
		Network->SendPacket(Packet, TradePlayer->Peer);
	}

	// Set state back to normal
	Player->TradePlayer = nullptr;
	Player->WaitingForTrade = false;
}

// Handle a trade gold update
void _Server::HandleTradeGold(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Set gold amount
	int Gold = Data.Read<int>();
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Gold)
		Gold = std::max(0, Player->Gold);
	Player->TradeGold = Gold;
	Player->TradeAccepted = false;

	// Notify player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradeAccepted = false;

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_GOLD);
		Packet.Write<int>(Gold);
		Network->SendPacket(Packet, TradePlayer->Peer);
	}
}

// Handles a trade accept from a player
void _Server::HandleTradeAccept(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get trading player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {

		// Set the player's state
		bool Accepted = !!Data.Read<char>();
		Player->TradeAccepted = Accepted;

		// Check if both player's agree
		if(Accepted && TradePlayer->TradeAccepted) {

			// Exchange items
			_InventorySlot TempItems[PLAYER_TRADEITEMS];
			_Slot Slot;
			Slot.BagType = _Bag::BagType::TRADE;
			for(size_t i = 0; i < PLAYER_TRADEITEMS; i++) {
				Slot.Index = i;
				TempItems[i] = Player->Inventory->GetSlot(Slot);
				Player->Inventory->GetSlot(Slot) = TradePlayer->Inventory->GetSlot(Slot);
				TradePlayer->Inventory->GetSlot(Slot) = TempItems[i];
			}

			// Exchange gold
			Player->UpdateGold(TradePlayer->TradeGold - Player->TradeGold);
			TradePlayer->UpdateGold(Player->TradeGold - TradePlayer->TradeGold);

			// Move items to inventory and reset
			Player->WaitingForTrade = false;
			Player->TradePlayer = nullptr;
			Player->TradeGold = 0;
			Player->Inventory->MoveTradeToInventory();
			TradePlayer->WaitingForTrade = false;
			TradePlayer->TradePlayer = nullptr;
			TradePlayer->TradeGold = 0;
			TradePlayer->Inventory->MoveTradeToInventory();

			// Send packet to players
			{
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::TRADE_EXCHANGE);
				Packet.Write<int>(Player->Gold);
				Player->Inventory->Serialize(Packet);
				Network->SendPacket(Packet, Player->Peer);
			}
			{
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::TRADE_EXCHANGE);
				Packet.Write<int>(TradePlayer->Gold);
				TradePlayer->Inventory->Serialize(Packet);
				Network->SendPacket(Packet, TradePlayer->Peer);
			}

		}
		else {

			// Notify trading player
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADE_ACCEPT);
			Packet.Write<char>(Accepted);
			Network->SendPacket(Packet, TradePlayer->Peer);
		}
	}
}

// Handle party info from client
void _Server::HandlePartyInfo(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	// Validate player
	_Object *Player = Peer->Object;
	if(!Player->CanOpenParty())
		return;

	// Get party name
	Player->PartyName = Data.ReadString();

	// Send info
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PARTY_INFO);
	Packet.WriteString(Player->PartyName.c_str());
	Network->SendPacket(Packet, Player->Peer);
}

// Handles player status change
void _Server::HandlePlayerStatus(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;
	if(Player->Battle)
		return;

	// Read packet
	uint8_t Status = Data.Read<uint8_t>();
	switch(Status) {
		case _Object::STATUS_NONE:
			Player->ResetUIState();
		break;
		case _Object::STATUS_PAUSE:
			Player->Paused = true;
		break;
		case _Object::STATUS_INVENTORY:
			Player->InventoryOpen = true;
		break;
		case _Object::STATUS_SKILLS:
			Player->SkillsOpen = true;
		break;
		case _Object::STATUS_TELEPORT: {
			StartTeleport(Player, PLAYER_TELEPORT_TIME);
		} break;
		default:
		break;
	}

}

// Upgrade an item
void _Server::HandleBlacksmithUpgrade(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	// Get slot
	_Slot Slot;
	Slot.Unserialize(Data);
	if(!Player->Inventory->IsValidSlot(Slot))
		return;

	// Get item
	_InventorySlot &InventorySlot = Player->Inventory->GetSlot(Slot);
	if(InventorySlot.Upgrades >= InventorySlot.Item->MaxLevel)
		return;

	// Get upgrade price
	int Price = InventorySlot.Item->GetUpgradePrice(InventorySlot.Upgrades+1);

	// Check gold
	if(Price > Player->Gold)
		return;

	// Upgrade item
	InventorySlot.Upgrades++;

	// Update gold
	{
		_StatChange StatChange;
		StatChange.Object = Player;
		StatChange.Values[StatType::GOLD].Integer = -Price;
		Player->UpdateStats(StatChange);

		// Build packet
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::STAT_CHANGE);
		StatChange.Serialize(Packet);
		Network->SendPacket(Packet, Player->Peer);
	}

	// Update items
	{
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
		Packet.Write<uint8_t>(1);
		Player->Inventory->SerializeSlot(Packet, Slot);
		Network->SendPacket(Packet, Peer);
	}

	// Log
	Log << Player->Name << " upgrades " << InventorySlot.Item->Name << " to level " << InventorySlot.Upgrades << " ( action=upgrade character_id=" << Peer->CharacterID << " item_id=" << InventorySlot.Item->ID << " gold=" << Player->Gold << " )" << std::endl;

	Player->CalculateStats();
}

// Handle paying to play a minigame
void _Server::HandleMinigamePay(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	// Validate
	_Object *Player = Peer->Object;
	if(!Player->Minigame || Player->Gold < Player->Minigame->Cost)
		return;

	// Update gold and stats
	{
		_StatChange StatChange;
		StatChange.Object = Player;
		StatChange.Values[StatType::GOLD].Integer = -Player->Minigame->Cost;
		Player->UpdateStats(StatChange);

		// Build packet
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::STAT_CHANGE);
		StatChange.Serialize(Packet);
		Network->SendPacket(Packet, Player->Peer);

		Player->GamesPlayed++;
	}
}

// Give player minigame reward
void _Server::HandleMinigameGetPrize(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	// Validate
	_Object *Player = Peer->Object;
	if(!Player->Minigame)
		return;

	float DropX = Data.Read<float>();

	// Simulate game
	_Minigame Minigame(Player->Minigame);
	Minigame.IsServer = true;
	Minigame.StartGame(Player->Seed);
	Minigame.Drop(DropX);

	double Time = 0;
	while(Time < 30) {
		Minigame.Update(DEFAULT_TIMESTEP);
		if(Minigame.State == _Minigame::StateType::DONE) {
			break;
		}
		Time += DEFAULT_TIMESTEP;
	}

	// Give reward
	Player->SendSeed(true);
	if(Minigame.Bucket < Minigame.Prizes.size()) {
		const _MinigameItem *MinigameItem = Minigame.Prizes[Minigame.Bucket];
		if(MinigameItem && MinigameItem->Item) {
			SendItem(Peer, Stats->Items.at(MinigameItem->Item->ID), MinigameItem->Count);
		}
	}
}

// Handle join battle request by player
void _Server::HandleJoin(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->AcceptingMoveInput())
		return;

	// Find a nearby battle instance
	bool HitPrivateParty = false;
	_Battle *Battle = Player->Map->GetCloseBattle(Player, HitPrivateParty);
	if(!Battle) {
		if(HitPrivateParty)
			SendMessage(Peer, "Can't join private party", "red");

		return;
	}

	// Add player to battle
	Battle->AddFighter(Player, 0, true);

	// Send battle to new player
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_START);
	Battle->Serialize(Packet);
	Network->SendPacket(Packet, Peer);
}

// Handle client exit command
void _Server::HandleExit(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	// Get object
	_Object *Player = Peer->Object;
	if(Player) {
		Player->Peer = nullptr;

		if(Player->Map) {
			BroadcastMessage(Peer, Player->Name + " has left the server", "gray");
			Player->LoadMapID = Player->GetMapID();
		}

		// Penalize player for leaving battle
		if(Player->Battle) {
			Player->ApplyDeathPenalty(PLAYER_DEATH_GOLD_PENALTY, 0);
			Player->Fighter->Health = 0;
			Player->Fighter->Mana = Player->Fighter->MaxMana / 2;
			Player->LoadMapID = 0;
		}

		// Leave trading screen
		_Object *TradePlayer = Player->TradePlayer;
		if(TradePlayer) {
			TradePlayer->TradePlayer = nullptr;

			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
			Network->SendPacket(Packet, TradePlayer->Peer);
		}

		// Save player
		Save->StartTransaction();
		Save->SavePlayer(Player, Player->LoadMapID, &Log);
		Save->EndTransaction();

		Player->Deleted = true;
		Peer->Object = nullptr;
	}
}

// Handle action use by player
void _Server::HandleActionUse(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->IsAlive())
		return;

	// Set action used
	Player->SetActionUsing(Data, ObjectManager);

	// Check for battle
	if(Player->Battle) {

		// Notify other players of action
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_ACTION);
		Packet.Write<NetworkIDType>(Player->NetworkID);
		if(Player->Action.Item)
			Packet.Write<uint32_t>(Player->Action.Item->ID);
		else
			Packet.Write<uint32_t>(0);

		Player->Battle->BroadcastPacket(Packet);
	}
}

// Handle an action bar change
void _Server::HandleActionBarChanged(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Read skills
	for(size_t i = 0; i < Player->ActionBar.size(); i++)
		Player->ActionBar[i].Unserialize(Data, Stats);

	Player->CalculateStats();
}

// Updates the player's HUD
void _Server::SendHUD(_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_HUD);
	Packet.Write<int>(Player->Fighter->Health);
	Packet.Write<int>(Player->Fighter->Mana);
	Packet.Write<int>(Player->Fighter->MaxHealth);
	Packet.Write<int>(Player->Fighter->MaxMana);
	Packet.Write<int>(Player->Fighter->Experience);
	Packet.Write<int>(Player->Gold);
	Packet.Write<int>(Player->Bounty);
	Packet.Write<double>(Save->Clock);

	Network->SendPacket(Packet, Peer);
}

// Run event script from map
void _Server::RunEventScript(uint32_t ScriptID, _Object *Object) {
	if(!Object)
		return;

	// Find script
	auto Iterator = Stats->Scripts.find(ScriptID);
	if(Iterator != Stats->Scripts.end()) {
		const _Script &Script = Iterator->second;

		_StatChange StatChange;
		StatChange.Object = Object;
		if(Scripting->StartMethodCall(Script.Name, "Activate")) {
			Scripting->PushInt(Script.Level);
			Scripting->PushReal(Script.Cooldown);
			Scripting->PushObject(StatChange.Object);
			Scripting->PushStatChange(&StatChange);
			Scripting->MethodCall(4, 1);
			Scripting->GetStatChange(1, StatChange);
			Scripting->FinishMethodCall();

			StatChange.Object->UpdateStats(StatChange);

			// Notify peer
			if(Object->Peer) {

				// Build packet
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::STAT_CHANGE);
				StatChange.Serialize(Packet);

				// Send packet to player
				Network->SendPacket(Packet, Object->Peer);
			}
		}
	}
}

// Send a message to the player
void _Server::SendMessage(_Peer *Peer, const std::string &Message, const std::string &ColorName) {
	if(!ValidatePeer(Peer))
		return;

	// Build message
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHAT_MESSAGE);
	Packet.WriteString(ColorName.c_str());
	Packet.WriteString(Message.c_str());

	// Send
	Network->SendPacket(Packet, Peer);
}

// Broadcast message to all peers
void _Server::BroadcastMessage(_Peer *IgnorePeer, const std::string &Message, const std::string &ColorName) {
	for(auto &Peer : Network->GetPeers()) {
		if(Peer != IgnorePeer)
			SendMessage(Peer, Message, ColorName);
	}
}

// Sends information to another player about items they're trading
void _Server::SendTradeInformation(_Object *Sender, _Object *Receiver) {
	_Bag &Bag = Sender->Inventory->Bags[_Bag::BagType::TRADE];

	// Send items to trader player
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_REQUEST);
	Packet.Write<NetworkIDType>(Sender->NetworkID);
	Packet.Write<int>(Sender->TradeGold);
	for(size_t i = 0; i < Bag.Slots.size(); i++)
		Sender->Inventory->SerializeSlot(Packet, _Slot(_Bag::BagType::TRADE, i));

	Network->SendPacket(Packet, Receiver->Peer);
}

// Start a battle event
void _Server::StartBattle(_BattleEvent &BattleEvent) {

	// Return if object is already in battle or in zone 0
	if(BattleEvent.Object->Battle || (!BattleEvent.PVP && !BattleEvent.Zone))
		return;

	// Handle PVP
	if(BattleEvent.PVP) {

		// Get list of players on current tile
		std::list<_Object *> Players;
		BattleEvent.Object->Map->GetPVPPlayers(BattleEvent.Object, Players);
		if(!Players.size())
			return;

		// Create a new battle instance
		_Battle *Battle = BattleManager->Create();
		Battle->Manager = ObjectManager;
		Battle->Stats = Stats;
		Battle->Server = this;
		Battle->Scripting = Scripting;
		Battle->PVP = BattleEvent.PVP;
		Battle->Difficulty[0] = 1.0;
		Battle->Difficulty[1] = 1.0;

		// Add players to battle
		Battle->AddFighter(BattleEvent.Object, BATTLE_PVP_ATTACKER_SIDE);
		for(auto &TargetPlayer : Players) {
			Battle->AddFighter(TargetPlayer, !BATTLE_PVP_ATTACKER_SIDE);
		}

		// Send battle to players
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_START);
		Battle->Serialize(Packet);
		Battle->BroadcastPacket(Packet);
	}
	else {

		// Get a list of players
		std::list<_Object *> Players;
		BattleEvent.Object->Map->GetPotentialBattlePlayers(BattleEvent.Object, 7*7, BATTLE_MAXFIGHTERS_SIDE-1, Players);
		int AdditionalCount = 0;
		if(!BattleEvent.Scripted)
			AdditionalCount = (int)Players.size();

		// Get monsters
		std::list<uint32_t> MonsterIDs;
		bool Boss = false;
		double Cooldown = 0.0;
		Stats->GenerateMonsterListFromZone(AdditionalCount, BattleEvent.Zone, MonsterIDs, Boss, Cooldown);

		// Fight if there are monsters
		if(MonsterIDs.size()) {

			// Check for cooldown
			if(BattleEvent.Object->BattleCooldown.find(BattleEvent.Zone) != BattleEvent.Object->BattleCooldown.end())
				return;

			// Create a new battle instance
			_Battle *Battle = BattleManager->Create();
			Battle->Manager = ObjectManager;
			Battle->Stats = Stats;
			Battle->Server = this;
			Battle->Boss = Boss;
			Battle->Cooldown = Cooldown;
			Battle->Zone = BattleEvent.Zone;
			Battle->Scripting = Scripting;
			Scripting->CreateBattle(Battle);

			// Add player
			Players.push_back(BattleEvent.Object);

			// Sort by network id
			Players.sort(CompareObjects);

			// Get difficulty
			double Difficulty = 1.0;
			if(Scripting->StartMethodCall("Game", "GetDifficulty")) {
				Scripting->PushReal(Save->Clock);
				Scripting->MethodCall(1, 1);
				Difficulty = Scripting->GetReal(1);
				Scripting->FinishMethodCall();
			}

			// Add players to battle
			Difficulty -= GAME_DIFFICULTY_PER_PLAYER;
			for(auto &PartyPlayer : Players) {
				Battle->AddFighter(PartyPlayer, 0);

				// Increase difficulty for each player
				Difficulty += GAME_DIFFICULTY_PER_PLAYER;
			}

			// Set difficulty of battle
			Battle->Difficulty[0] = 1.0;
			Battle->Difficulty[1] = Difficulty;

			// Add monsters
			for(auto &MonsterID : MonsterIDs) {
				_Object *Monster = ObjectManager->Create();
				Monster->Server = this;
				Monster->Scripting = Scripting;
				Monster->DatabaseID = MonsterID;
				Monster->Stats = Stats;
				Stats->GetMonsterStats(MonsterID, Monster, Difficulty);
				Monster->CalculateStats();
				Battle->AddFighter(Monster, 1);
			}

			// Send battle to players
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::BATTLE_START);
			Battle->Serialize(Packet);
			Battle->BroadcastPacket(Packet);
		}
	}
}
