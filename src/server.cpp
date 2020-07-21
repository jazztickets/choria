/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <ae/random.h>
#include <ae/util.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/controller.h>
#include <objects/components/monster.h>
#include <objects/components/fighter.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <objects/minigame.h>
#include <scripting.h>
#include <save.h>
#include <packet.h>
#include <stats.h>
#include <constants.h>
#include <version.h>
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
	StartShutdownTimer(false),
	StartDisconnect(false),
	StartShutdown(false),
	ShutdownTime(0.0),
	TimeSteps(0),
	Time(0.0),
	SaveTime(0.0),
	BotTime(0.0),
	Network(new ae::_ServerNetwork(Config.MaxClients, NetworkPort)),
	Thread(nullptr),
	PingPacket(1024) {

	if(!Network->HasConnection())
		throw std::runtime_error("Unable to start server!");

	Network->CreatePingSocket(DEFAULT_NETWORKPINGPORT);
	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	ObjectManager = new ae::_Manager<_Object>();
	MapManager = new ae::_Manager<_Map>();
	BattleManager = new ae::_Manager<_Battle>();
	Stats = new _Stats(true);
	Save = new _Save();

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_GAME);

	Log.Open((Config.LogPath + "server.log").c_str());
	Log << "[SERVER_START] Listening on port " << NetworkPort << std::endl;
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

	Log << "[SERVER_STOP] Stopping server" << std::endl;
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
void _Server::StopServer(int Seconds) {
	if(Seconds > 0) {
		StartShutdownTimer = true;
		ShutdownTime = Seconds;
	}
	else {
		StartDisconnect = true;
	}
}

// Create a summon object
_Object *_Server::CreateSummon(_Object *Source, const _Summon &Summon) {

	// Create monster
	_Object *Object = ObjectManager->Create();
	Object->Server = this;
	Object->Scripting = Scripting;
	Object->Monster->DatabaseID = Summon.ID;
	Object->Stats = Stats;
	Object->Monster->Owner = Source;
	Object->Monster->SummonBuff = Summon.SummonBuff;
	Object->Monster->SpellID = Summon.SpellID;
	Object->Monster->Duration = Summon.Duration;

	// Get difficulty
	int Difficulty = 0;
	if(Source->Monster->DatabaseID > 0)
		Difficulty += Source->Monster->Difficulty;
	float DifficultyMultiplier = (100 + Difficulty) * 0.01f;

	// Get stats from db and script
	Stats->GetMonsterStats(Object->Monster->DatabaseID, Object, Difficulty);
	Object->Character->Health = Object->Character->BaseMaxHealth = Summon.Health * DifficultyMultiplier;
	Object->Character->Mana = Object->Character->BaseMaxMana = Summon.Mana * DifficultyMultiplier;
	Object->Character->BaseMinDamage = Summon.MinDamage;
	Object->Character->BaseMaxDamage = Summon.MaxDamage;
	Object->Character->BaseArmor = Summon.Armor;

	for(auto &Skill : Object->Character->Skills)
		Skill.second = Summon.SkillLevel;

	Object->Character->CalculateStats();

	return Object;
}

// Update
void _Server::Update(double FrameTime) {
	//if(std::abs(std::fmod(Time, 1.0)) >= 0.99)
	//	std::cout << "Server: O=" << ObjectManager->Objects.size() << " B=" << BattleManager->Objects.size() << std::endl;

	// Handle pings
	ae::_NetworkAddress PingAddress;
	while(Network->CheckPings(PingPacket, PingAddress)) {
		PingType Type = PingPacket.Read<PingType>();

		// Handle ping types
		switch(Type) {
			case PingType::SERVER_INFO: {
				ae::_Buffer PongPacket;
				PongPacket.Write<PingType>(PingType::SERVER_INFO_RESPONSE);
				PongPacket.Write<uint16_t>(Network->GetListenPort());
				PongPacket.Write<uint16_t>(Network->GetPeers().size());
				PongPacket.Write<uint16_t>(Network->GetMaxPeers());
				PongPacket.WriteBit(Hardcore);
				Network->SendPingPacket(PongPacket, ae::_NetworkAddress(PingAddress.Host, PingAddress.Port));
			} break;
			default:
			break;
		}

		// Reset packet
		PingPacket.StartRead();
	}

	// Update network
	Network->Update(FrameTime);

	// Get events
	ae::_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case ae::_NetworkEvent::CONNECT:
				HandleConnect(NetworkEvent);
			break;
			case ae::_NetworkEvent::DISCONNECT:
				HandleDisconnect(NetworkEvent);
			break;
			case ae::_NetworkEvent::PACKET:
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

	// Handle rebirths
	for(auto &RebirthEvent : RebirthEvents)
		StartRebirth(RebirthEvent);

	RebirthEvents.clear();

	// Wait for peers to disconnect
	if(StartShutdownTimer) {
		ShutdownTime -= FrameTime;
		if(std::abs(std::fmod(ShutdownTime, 5.0)) >= 4.99)
			BroadcastMessage(nullptr, "The server will be shutting down in " + std::to_string((int)(ShutdownTime + 0.5)) + " seconds.", "red");

		if(ShutdownTime <= 0) {
			StartDisconnect = true;
			StartShutdownTimer = false;
		}
	}
	else if(StartDisconnect) {
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
	if(1 && IsTesting && BotTime > 1.1) {
		BotTime = -1;
		CreateBot();
	}
}

// Handle client connect
void _Server::HandleConnect(ae::_NetworkEvent &Event) {
	char Buffer[16];
	ENetAddress *Address = &Event.Peer->ENetPeer->address;
	enet_address_get_host_ip(Address, Buffer, 16);
	Log << "[CONNECT] Connect from " << Buffer << ":" << Address->port << std::endl;

	// Send game version
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::VERSION);
	Packet.WriteString(GAME_VERSION);
	Packet.Write<int>(BUILD_NUMBER);
	Network->SendPacket(Packet, Event.Peer);
}

// Handle client disconnect
void _Server::HandleDisconnect(ae::_NetworkEvent &Event) {
	char Buffer[16];
	ENetAddress *Address = &Event.Peer->ENetPeer->address;
	enet_address_get_host_ip(Address, Buffer, 16);
	Log << "[DISCONNECT] Disconnect from " << Buffer << ":" << Address->port << std::endl;

	ae::_Buffer Data;
	HandleExit(Data, Event.Peer);

	// Delete peer from network
	Network->DeletePeer(Event.Peer);
}

// Handle packet data
void _Server::HandlePacket(ae::_Buffer &Data, ae::_Peer *Peer) {
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
		case PacketType::INVENTORY_DELETE:
			HandleInventoryDelete(Data, Peer);
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
		case PacketType::ENCHANTER_BUY:
			HandleEnchanterBuy(Data, Peer);
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
		case PacketType::PLAYER_CLEARBUFF:
			HandleClearBuff(Data, Peer);
		break;
		case PacketType::COMMAND:
			HandleCommand(Data, Peer);
		break;
		default:
		break;
	}
}

// Send an item to the player
void _Server::SendItem(ae::_Peer *Peer, const _Item *Item, int Count) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player || !Item)
		return;

	// Add item
	Player->Inventory->AddItem(Item, 0, Count);

	// Send item
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_ADD);
	Packet.Write<uint8_t>((uint8_t)Count);
	Packet.Write<uint32_t>(Item->ID);
	Player->Inventory->Serialize(Packet);
	Network->SendPacket(Packet, Peer);

	// Update states
	Player->Character->CalculateStats();
	SendHUD(Player->Peer);
}

// Login information
void _Server::HandleLoginInfo(ae::_Buffer &Data, ae::_Peer *Peer) {

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
			ae::_Buffer Packet;
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
	ae::_Buffer Packet;
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
void _Server::HandleCharacterListRequest(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!Peer->AccountID)
		return;

	SendCharacterList(Peer);
}

// Handles the character create request
void _Server::HandleCharacterCreate(ae::_Buffer &Data, ae::_Peer *Peer) {
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
		ae::_Buffer NewPacket;
		NewPacket.Write<PacketType>(PacketType::CREATECHARACTER_INUSE);
		Network->SendPacket(NewPacket, Peer);
		return;
	}

	// Create the character
	Save->CreateCharacter(Stats, Scripting, Peer->AccountID, Slot, IsHardcore, Name, PortraitID, BuildID);

	// Notify the client
	ae::_Buffer NewPacket;
	NewPacket.Write<PacketType>(PacketType::CREATECHARACTER_SUCCESS);
	Network->SendPacket(NewPacket, Peer);
}

// Handle a character delete request
void _Server::HandleCharacterDelete(ae::_Buffer &Data, ae::_Peer *Peer) {
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
void _Server::HandleCharacterPlay(ae::_Buffer &Data, ae::_Peer *Peer) {

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
	SpawnPlayer(Peer->Object, Peer->Object->Character->LoadMapID, _Map::EVENT_NONE);

	// Broadcast message
	std::string Message = Peer->Object->Name + " has joined the server";
	BroadcastMessage(Peer, Message, "gray");

	// Log
	Log << "[JOIN] Player " << Message << " ( character_id=" << Peer->CharacterID << " )" << std::endl;
}

// Handles move commands from a client
void _Server::HandleMoveCommand(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->Character->IsAlive())
		return;

	Player->Controller->InputStates.push_back(Data.Read<char>());
}

// Handles use command from a client
void _Server::HandleUseCommand(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->Character->IsAlive())
		return;

	Player->Controller->UseCommand = true;
}

// Handle respawn command from client
void _Server::HandleRespawn(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	// Check death
	if(!Player->Character->IsAlive()) {
		if(Player->Character->Hardcore)
			return;

		// Wait for battle to finish
		if(Player->Character->Battle)
			return;

		Player->Character->Health = Player->Character->MaxHealth / 2;
		Player->Character->Mana = Player->Character->MaxMana / 2;
		SpawnPlayer(Player, Player->Character->SpawnMapID, _Map::EVENT_SPAWN);
	}
}

// Handle a chat message
void _Server::HandleChatMessage(ae::_Buffer &Data, ae::_Peer *Peer) {
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

	// Send message to other players
	BroadcastMessage(nullptr, Player->Name + ": " + Message, "white");

	// Log
	Log << "[CHAT] " << Player->Name + ": " + Message << std::endl;
}

// Send position to player
void _Server::SendPlayerPosition(ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_POSITION);
	Packet.Write<glm::ivec2>(Player->Position);

	Network->SendPacket(Packet, Player->Peer);
}

// Send player stats to peer
void _Server::SendPlayerInfo(ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Build packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::OBJECT_STATS);
	Player->SerializeStats(Packet);

	Network->SendPacket(Packet, Peer);
}

// Send character list
void _Server::SendCharacterList(ae::_Peer *Peer) {

	// Create packet
	ae::_Buffer Packet;
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
		Packet.Write<uint8_t>(Player.Character->Hardcore);
		Packet.WriteString(Save->Database->GetString("name"));
		Packet.Write<uint32_t>(Player.Character->PortraitID);
		Packet.Write<int>(Player.Character->Health);
		Packet.Write<int>(Player.Character->Experience);
	}
	Save->Database->CloseQuery();

	// Send list
	Network->SendPacket(Packet, Peer);
}

// Spawns a player at a particular spawn point
void _Server::SpawnPlayer(_Object *Player, ae::NetworkIDType MapID, uint32_t EventType) {
	if(!Stats)
		return;

	if(!ValidatePeer(Player->Peer) || !Player->Peer->CharacterID)
	   return;

	// Use spawn point for new characters
	if(MapID == 0) {
		MapID = Player->Character->SpawnMapID;
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
			uint32_t SpawnPoint = Player->Character->SpawnPoint;
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
			ae::_Buffer Packet;
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
			Player->Character->Path.clear();
	}
	else {
		Map->FindEvent(_Event(EventType, Player->Character->SpawnPoint), Player->Position);
		SendPlayerPosition(Player->Peer);
		SendHUD(Player->Peer);
	}
}

// Queue a player for rebirth
void _Server::QueueRebirth(_Object *Object, int Type, int Value) {
	_RebirthEvent RebirthEvent;
	RebirthEvent.Object = Object;
	RebirthEvent.Type = Type;
	RebirthEvent.Value = Value;

	RebirthEvents.push_back(RebirthEvent);
}

// Queue a battle for an object
void _Server::QueueBattle(_Object *Object, uint32_t Zone, bool Scripted, bool PVP, float BountyEarned, float BountyClaimed) {
	_BattleEvent BattleEvent;
	BattleEvent.Object = Object;
	BattleEvent.Zone = Zone;
	BattleEvent.Scripted = Scripted;
	BattleEvent.PVP = PVP;
	BattleEvent.BountyEarned = BountyEarned;
	BattleEvent.BountyClaimed = BountyClaimed;
	BattleEvent.Side = BATTLE_PVP_VICTIM_SIDE;
	if(BountyEarned > 0)
		BattleEvent.Side = BATTLE_PVP_ATTACKER_SIDE;

	BattleEvents.push_back(BattleEvent);
}

// Start teleporting a player
void _Server::StartTeleport(_Object *Object, double Time) {
	if(Object->Character->Battle || !Object->Character->IsAlive())
		return;

	Object->Character->ResetUIState();
	Object->Character->TeleportTime = Time;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_TELEPORTSTART);
	Packet.Write<double>(Time);
	Network->SendPacket(Packet, Object->Peer);
}

// Create player object and load stats from save
_Object *_Server::CreatePlayer(ae::_Peer *Peer) {

	// Create object
	_Object *Player = ObjectManager->Create();
	Player->Scripting = Scripting;
	Player->Server = this;
	Player->Character->CharacterID = Peer->CharacterID;
	Player->Peer = Peer;
	Player->Stats = Stats;
	Peer->Object = Player;

	Save->LoadPlayer(Stats, Player);

	return Player;
}

// Create server side bot
_Object *_Server::CreateBot() {

	// Check for account being used
	ae::_Peer TestPeer(nullptr);
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
	Bot->Character->Bot = true;
	Bot->Scripting = Scripting;
	Bot->Server = this;
	Bot->Character->CharacterID = CharacterID;
	Bot->Stats = Stats;
	Save->LoadPlayer(Stats, Bot);
	Bot->Character->PartyName = "bot";

	// Create fake peer
	Bot->Peer = new ae::_Peer(nullptr);
	Bot->Peer->Object = Bot;
	Bot->Peer->CharacterID = CharacterID;
	Bot->Peer->AccountID = ACCOUNT_BOTS_ID;

	// Simulate packet
	ae::_Buffer Packet;
	Packet.Write<uint8_t>(0);
	HandleCharacterPlay(Packet, Bot->Peer);

	return Bot;
}

// Validate a peer's attributes
bool _Server::ValidatePeer(ae::_Peer *Peer) {
	if(!Peer)
		return false;

	if(!Peer->AccountID)
		return false;

	if(!Peer->Object)
		return false;

	return true;
}

// Check to see if an account is in use
bool _Server::CheckAccountUse(ae::_Peer *Peer) {
	for(auto &CheckPeer : Network->GetPeers()) {
		if(CheckPeer != Peer && CheckPeer->AccountID == Peer->AccountID)
			return true;
	}

	return false;
}

// Handles a player's inventory move
void _Server::HandleInventoryMove(ae::_Buffer &Data, ae::_Peer *Peer) {
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
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::INVENTORY_SWAP);
		if(Player->Inventory->MoveInventory(Packet, OldSlot, NewSlot)) {
			Network->SendPacket(Packet, Peer);
			Player->Character->CalculateStats();
		}
	}

	// Check for trading players
	_Object *TradePlayer = Player->Character->TradePlayer;
	if(Player->Character->WaitingForTrade && TradePlayer && (OldSlot.Type == BagType::TRADE || NewSlot.Type == BagType::TRADE)) {

		// Reset agreement
		Player->Character->TradeAccepted = false;
		TradePlayer->Character->TradeAccepted = false;

		// Build packet
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_ITEM);

		// Write slots
		Player->Inventory->SerializeSlot(Packet, NewSlot);
		Player->Inventory->SerializeSlot(Packet, OldSlot);

		// Send updates
		Network->SendPacket(Packet, TradePlayer->Peer);
	}
}

// Handle a player's inventory use request
void _Server::HandleInventoryUse(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get alternate flag
	bool Alternate = Data.ReadBit();

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
		if(Alternate || (TargetSlot.Index == EquipmentType::RING1 && Player->Inventory->GetSlot(TargetSlot).Item && !Player->Inventory->GetBag(BagType::EQUIPMENT).Slots[EquipmentType::RING2].Item))
			TargetSlot.Index = EquipmentType::RING2;

		// Check for empty main hand when equipping off-hand
		if(Item->Type == ItemType::OFFHAND && !Player->Inventory->GetBag(BagType::EQUIPMENT).Slots[EquipmentType::HAND1].Item)
			TargetSlot.Index = EquipmentType::HAND1;

		// Attempt to move
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::INVENTORY_SWAP);
		if(Player->Inventory->MoveInventory(Packet, Slot, TargetSlot)) {
			Network->SendPacket(Packet, Peer);
			Player->Character->CalculateStats();
		}
	}
	// Handle consumables
	else {

		// Check for existing action
		if(!Player->Character->Action.IsSet()) {
			Player->Character->Action.Item = Item;
			Player->Character->Action.Level = Item->Level;
			Player->Character->Action.Duration = Item->Duration;
			Player->Character->Action.InventorySlot = (int)Slot.Index;
			Player->Character->Targets.clear();

			if(Item->TargetAlive) {
				Player->Character->Targets.push_back(Player);
			}
			else if(Player->Map) {
				_Object *DeadPlayer = Player->Map->FindDeadPlayer(Player, 1.0f);
				if(DeadPlayer)
					Player->Character->Targets.push_back(DeadPlayer);
			}
		}
	}
}

// Handle a player's inventory split stack request
void _Server::HandleInventorySplit(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	_Slot Slot;
	Slot.Unserialize(Data);
	uint8_t Count = Data.Read<uint8_t>();

	// Inventory only
	if(Slot.Type == BagType::TRADE)
		return;

	// Split items
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
	if(Player->Inventory->SplitStack(Packet, Slot, Count))
		Network->SendPacket(Packet, Peer);
}

// Handle deleting an item
void _Server::HandleInventoryDelete(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	// Get player object
	_Object *Player = Peer->Object;

	// Get item slot
	_Slot Slot;
	Slot.Unserialize(Data);
	if(!Player->Inventory->IsValidSlot(Slot))
		return;

	// Delete item
	Player->Inventory->GetBag(Slot.Type).Slots[Slot.Index].Reset();

	// Update client
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
	Packet.Write<uint8_t>(1);
	Player->Inventory->SerializeSlot(Packet, Slot);
	Network->SendPacket(Packet, Peer);

	// Check for trading players
	_Object *TradePlayer = Player->Character->TradePlayer;
	if(Player->Character->WaitingForTrade && TradePlayer && Slot.Type == BagType::TRADE) {

		// Reset agreement
		Player->Character->TradeAccepted = false;
		TradePlayer->Character->TradeAccepted = false;

		// Notify trade player
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_ITEM);
		Player->Inventory->SerializeSlot(Packet, Slot);
		Player->Inventory->SerializeSlot(Packet, Slot);
		Network->SendPacket(Packet, TradePlayer->Peer);
	}
}

// Handles a vendor exchange message
void _Server::HandleVendorExchange(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get vendor
	const _Vendor *Vendor = Player->Character->Vendor;
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
		int Price = Item->GetPrice(Scripting, Player, Vendor, Amount, Buy);

		// Not enough gold
		if(Price > Player->Character->Gold)
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
		Player->Character->UpdateGold(-Price);
		if(Peer) {
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::INVENTORY_GOLD);
			Packet.Write<int>(Player->Character->Gold);
			Network->SendPacket(Packet, Peer);
		}

		// Update items
		if(Peer) {
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
			Packet.Write<uint8_t>(1);
			Player->Inventory->SerializeSlot(Packet, TargetSlot);
			Network->SendPacket(Packet, Peer);
		}

		Player->Character->CalculateStats();

		// Log
		Log << "[PURCHASE] Player " << Player->Name << " buys " << (int)Amount << "x " << Item->Name << " ( character_id=" << Peer->CharacterID << " item_id=" << Item->ID << " gold=" << Player->Character->Gold << " )" << std::endl;
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
			int Price = InventorySlot.Item->GetPrice(Scripting, Player, Vendor, Amount, Buy, InventorySlot.Upgrades);

			// Update gold
			Player->Character->UpdateGold(Price);
			if(Peer) {
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::INVENTORY_GOLD);
				Packet.Write<int>(Player->Character->Gold);
				Network->SendPacket(Packet, Peer);
			}

			// Log
			Log << "[SELL] Player " << Player->Name << " sells " << Amount << "x " << InventorySlot.Item->Name << " ( character_id=" << Peer->CharacterID << " item_id=" << InventorySlot.Item->ID << " gold=" << Player->Character->Gold << " )" << std::endl;

			// Update items
			Player->Inventory->UpdateItemCount(Slot, -Amount);
			if(Peer) {
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
				Packet.Write<uint8_t>(1);
				Player->Inventory->SerializeSlot(Packet, Slot);
				Network->SendPacket(Packet, Peer);
			}
		}
	}
}

// Handles a trader accept
void _Server::HandleTraderAccept(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get trader information
	std::vector<_Slot> RequiredItemSlots(Player->Character->Trader->Items.size());
	_Slot RewardSlot = Player->Inventory->GetRequiredItemSlots(Player->Character->Trader, RequiredItemSlots);
	if(!Player->Inventory->IsValidSlot(RewardSlot))
		return;

	// Update items
	Player->AcceptTrader(RequiredItemSlots);

	// Give buff
	if(Player->Character->Trader->RewardItem == nullptr) {
		_StatChange StatChange;
		StatChange.Object = Player;
		StatChange.Values[StatType::BUFF].Pointer = (void *)Stats->Buffs.at(22);
		StatChange.Values[StatType::BUFFLEVEL].Integer = 10;
		StatChange.Values[StatType::BUFFDURATION].Float = 60;
		Player->UpdateStats(StatChange);

		// Build packet
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::STAT_CHANGE);
		StatChange.Serialize(Packet);
		Network->SendPacket(Packet, Player->Peer);
	}

	// Send new inventory
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY);
	Player->Inventory->Serialize(Packet);
	Network->SendPacket(Packet, Peer);
}

// Handles a skill adjust
void _Server::HandleSkillAdjust(ae::_Buffer &Data, ae::_Peer *Peer) {
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
	Player->Character->AdjustSkillLevel(SkillID, Amount);
	Player->Character->CalculateStats();
}

// Handle purchase from enchanter
void _Server::HandleEnchanterBuy(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Process packet
	uint32_t SkillID = Data.Read<uint32_t>();

	// Check for skill unlocked
	if(Player->Character->Skills.find(SkillID) == Player->Character->Skills.end())
		return;

	// Validate
	if(Player->Character->MaxSkillLevels.find(SkillID) == Player->Character->MaxSkillLevels.end())
		Player->Character->MaxSkillLevels[SkillID] = GAME_DEFAULT_MAX_SKILL_LEVEL;

	// Get upgrade price
	int MaxSkillLevel = Player->Character->MaxSkillLevels.at(SkillID);
	int Price = _Item::GetEnchantCost(MaxSkillLevel);

	// Check gold
	if(Price > Player->Character->Gold)
		return;

	// Update gold
	{
		_StatChange StatChange;
		StatChange.Object = Player;
		StatChange.Values[StatType::GOLD].Integer = -Price;
		Player->UpdateStats(StatChange);

		// Build packet
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::STAT_CHANGE);
		StatChange.Serialize(Packet);
		Network->SendPacket(Packet, Player->Peer);
	}

	// Update values
	Player->Character->AdjustMaxSkillLevel(SkillID, 1);
	Player->Character->CalculateStats();
}

// Handle a trade request
void _Server::HandleTradeRequest(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get map object
	_Map *Map = Player->Map;
	if(!Map)
		return;

	// Check for level requirement
	if(!Player->Character->CanTrade())
		return;

	// Set status
	Player->Character->WaitingForTrade = true;

	// Find the nearest player to trade with
	_Object *TradePlayer = Map->FindTradePlayer(Player, 2.0f * 2.0f);
	if(TradePlayer == nullptr) {

		// Set up trade post
		Player->Character->TradeGold = 0;
		Player->Character->TradeAccepted = false;
		Player->Character->TradePlayer = nullptr;
	}
	else {

		// Set up trade screen for both players
		SendTradeInformation(Player, TradePlayer);
		SendTradeInformation(TradePlayer, Player);

		Player->Character->TradePlayer = TradePlayer;
		Player->Character->TradeAccepted = false;
		TradePlayer->Character->TradePlayer = Player;
		TradePlayer->Character->TradeAccepted = false;
		TradePlayer->Character->WaitingForTrade = true;
	}
}

// Handles a trade cancel
void _Server::HandleTradeCancel(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Notify trading player
	_Object *TradePlayer = Player->Character->TradePlayer;
	if(TradePlayer) {
		TradePlayer->Character->TradePlayer = nullptr;
		TradePlayer->Character->TradeAccepted = false;

		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
		Network->SendPacket(Packet, TradePlayer->Peer);
	}

	// Set state back to normal
	Player->Character->TradePlayer = nullptr;
	Player->Character->WaitingForTrade = false;
}

// Handle a trade gold update
void _Server::HandleTradeGold(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Set gold amount
	int Gold = Data.Read<int>();
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Character->Gold)
		Gold = std::max(0, Player->Character->Gold);
	Player->Character->TradeGold = Gold;
	Player->Character->TradeAccepted = false;

	// Notify player
	_Object *TradePlayer = Player->Character->TradePlayer;
	if(TradePlayer) {
		TradePlayer->Character->TradeAccepted = false;

		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_GOLD);
		Packet.Write<int>(Gold);
		Network->SendPacket(Packet, TradePlayer->Peer);
	}
}

// Handles a trade accept from a player
void _Server::HandleTradeAccept(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get trading player
	_Object *TradePlayer = Player->Character->TradePlayer;
	if(TradePlayer) {

		// Set the player's state
		bool Accepted = !!Data.Read<char>();
		Player->Character->TradeAccepted = Accepted;

		// Check if both player's agree
		if(Accepted && TradePlayer->Character->TradeAccepted) {

			// Exchange items
			_InventorySlot TempItems[INVENTORY_MAX_TRADE_ITEMS];
			_Slot Slot;
			Slot.Type = BagType::TRADE;
			for(size_t i = 0; i < INVENTORY_MAX_TRADE_ITEMS; i++) {
				Slot.Index = i;
				TempItems[i] = Player->Inventory->GetSlot(Slot);
				Player->Inventory->GetSlot(Slot) = TradePlayer->Inventory->GetSlot(Slot);
				TradePlayer->Inventory->GetSlot(Slot) = TempItems[i];
			}

			// Exchange gold
			Player->Character->UpdateGold(TradePlayer->Character->TradeGold - Player->Character->TradeGold);
			TradePlayer->Character->UpdateGold(Player->Character->TradeGold - TradePlayer->Character->TradeGold);

			// Move items to inventory and reset
			Player->Character->WaitingForTrade = false;
			Player->Character->TradePlayer = nullptr;
			Player->Character->TradeGold = 0;
			Player->Inventory->MoveTradeToInventory();
			TradePlayer->Character->WaitingForTrade = false;
			TradePlayer->Character->TradePlayer = nullptr;
			TradePlayer->Character->TradeGold = 0;
			TradePlayer->Inventory->MoveTradeToInventory();

			// Send packet to players
			{
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::TRADE_EXCHANGE);
				Packet.Write<int>(Player->Character->Gold);
				Player->Inventory->Serialize(Packet);
				Network->SendPacket(Packet, Player->Peer);
			}
			{
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::TRADE_EXCHANGE);
				Packet.Write<int>(TradePlayer->Character->Gold);
				TradePlayer->Inventory->Serialize(Packet);
				Network->SendPacket(Packet, TradePlayer->Peer);
			}

		}
		else {

			// Notify trading player
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADE_ACCEPT);
			Packet.Write<char>(Accepted);
			Network->SendPacket(Packet, TradePlayer->Peer);
		}
	}
}

// Handle party info from client
void _Server::HandlePartyInfo(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	// Validate player
	_Object *Player = Peer->Object;
	if(!Player->Character->CanOpenParty())
		return;

	// Get party name
	Player->Character->PartyName = Data.ReadString();

	// Broadcast party to all objects in map
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PARTY_INFO);
	Packet.Write<ae::NetworkIDType>(Player->NetworkID);
	Packet.WriteString(Player->Character->PartyName.c_str());
	if(Player->Map)
		Player->Map->BroadcastPacket(Packet);
}

// Handles player status change
void _Server::HandlePlayerStatus(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;
	if(Player->Character->Battle)
		return;

	// Read packet
	uint8_t Status = Data.Read<uint8_t>();
	switch(Status) {
		case _Character::STATUS_NONE:
			Player->Character->ResetUIState();
		break;
		case _Character::STATUS_MENU:
			Player->Character->MenuOpen = true;
		break;
		case _Character::STATUS_INVENTORY:
			Player->Character->InventoryOpen = true;
		break;
		case _Character::STATUS_SKILLS:
			Player->Character->SkillsOpen = true;
		break;
		case _Character::STATUS_TELEPORT: {
			StartTeleport(Player, PLAYER_TELEPORT_TIME);
		} break;
		default:
		break;
	}

}

// Handle a player dismissing a buff
void _Server::HandleClearBuff(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	// Get data
	uint32_t BuffID = Data.Read<uint32_t>();
	_Object *Player = Peer->Object;
	if(Player->Character->Battle)
		return;

	// Update buff
	for(auto &StatusEffect : Player->Character->StatusEffects) {
		if(!StatusEffect->Buff->Dismiss)
			continue;

		if(StatusEffect->Buff->ID == BuffID) {
			StatusEffect->Duration = 0.0;
			UpdateBuff(Player, StatusEffect);
			break;
		}
	}
}

// Upgrade an item
void _Server::HandleBlacksmithUpgrade(ae::_Buffer &Data, ae::_Peer *Peer) {
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
	int Price = InventorySlot.Item->GetUpgradeCost(InventorySlot.Upgrades+1);

	// Check gold
	if(Price > Player->Character->Gold)
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
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::STAT_CHANGE);
		StatChange.Serialize(Packet);
		Network->SendPacket(Packet, Player->Peer);
	}

	// Update items
	{
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
		Packet.Write<uint8_t>(1);
		Player->Inventory->SerializeSlot(Packet, Slot);
		Network->SendPacket(Packet, Peer);
	}

	// Log
	Log << "[UPGRADE] Player " << Player->Name << " upgrades " << InventorySlot.Item->Name << " to level " << InventorySlot.Upgrades << " ( character_id=" << Peer->CharacterID << " item_id=" << InventorySlot.Item->ID << " gold=" << Player->Character->Gold << " )" << std::endl;

	Player->Character->CalculateStats();
}

// Handle paying to play a minigame
void _Server::HandleMinigamePay(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	// Validate
	_Object *Player = Peer->Object;
	const _MinigameType *Minigame = Player->Character->Minigame;
	if(!Minigame || Player->Inventory->CountItem(Minigame->RequiredItem) < Minigame->Cost)
		return;

	// Trade in required items
	Player->Inventory->SpendItems(Minigame->RequiredItem, Minigame->Cost);

	// Send new inventory
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY);
	Player->Inventory->Serialize(Packet);
	Network->SendPacket(Packet, Peer);

	// Update stats
	Player->Character->GamesPlayed++;
}

// Give player minigame reward
void _Server::HandleMinigameGetPrize(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	// Validate
	_Object *Player = Peer->Object;
	if(!Player->Character->Minigame)
		return;

	float DropX = Data.Read<float>();

	// Simulate game
	_Minigame Minigame(Player->Character->Minigame);
	Minigame.IsServer = true;
	Minigame.StartGame(Player->Character->Seed);
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
void _Server::HandleJoin(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->Character->AcceptingMoveInput())
		return;

	// Find a nearby battle instance
	bool HitPrivateParty = false;
	bool HitFullBattle = false;
	_Battle *Battle = Player->Map->GetCloseBattle(Player, HitPrivateParty, HitFullBattle);
	if(!Battle) {
		if(HitPrivateParty)
			SendMessage(Peer, "Can't join private party", "red");
		else if(HitFullBattle)
			SendMessage(Peer, "Can't join full battle", "red");

		return;
	}

	// Add player to battle
	Battle->AddObject(Player, 0, true);
	AddBattleSummons(Battle, 0, Player, true);
	Battle->BroadcastStatusEffects(Player);

	// Send battle to new player
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_START);
	Battle->Serialize(Packet);
	Network->SendPacket(Packet, Peer);
}

// Handle client exit command
void _Server::HandleExit(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	// Get object
	_Object *Player = Peer->Object;
	if(Player) {
		Player->Peer = nullptr;

		if(Player->Map) {
			BroadcastMessage(Peer, Player->Name + " has left the server", "gray");
			Player->Character->LoadMapID = Player->GetMapID();
		}

		// Penalize player for leaving battle
		if(Player->Character->Battle) {
			Player->ApplyDeathPenalty(true, PLAYER_DEATH_GOLD_PENALTY, 0);
			Player->Character->Health = 0;
			Player->Character->Mana = Player->Character->MaxMana / 2;
			Player->Character->LoadMapID = 0;
			Player->Character->DeleteStatusEffects();
		}

		// Leave trading screen
		_Object *TradePlayer = Player->Character->TradePlayer;
		if(TradePlayer) {
			TradePlayer->Character->TradePlayer = nullptr;

			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
			Network->SendPacket(Packet, TradePlayer->Peer);
		}

		// Save player
		Save->StartTransaction();
		Save->SavePlayer(Player, Player->Character->LoadMapID, &Log);
		Save->EndTransaction();

		Player->Deleted = true;
		Peer->Object = nullptr;
	}
}

// Handle console commands
void _Server::HandleCommand(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!IsTesting)
		return;

	if(!ValidatePeer(Peer))
	   return;

	// Get player
	_Object *Player = Peer->Object;
	if(!Player->Map)
		return;

	// Process command
	std::string Command = Data.ReadString();
	if(Command == "battle") {
		Player->Character->BattleCooldown.clear();

		uint32_t ZoneID = Data.Read<uint32_t>();
		QueueBattle(Player, ZoneID, false, false, 0.0f, 0.0f);
	}
	else if(Command == "bounty") {
		bool Adjust = Data.ReadBit();
		int Change = Data.Read<int>();
		Player->Character->Bounty = std::max(0, Adjust ? Player->Character->Bounty + Change : Change);
		SendHUD(Peer);
	}
	else if(Command == "clearunlocks") {
		Player->Character->ClearUnlocks();
	}
	else if(Command == "clock") {
		double Clock = Data.Read<int>();
		if(Clock < 0)
			Clock = 0;
		else if(Clock >= MAP_DAY_LENGTH)
			Clock = MAP_DAY_LENGTH;

		SetClock(Clock);
	}
	else if(Command == "event") {
		_Event Event;
		Event.Type = Data.Read<uint32_t>();
		Event.Data = Data.Read<uint32_t>();

		Player->Map->StartEvent(Player, Event);
	}
	else if(Command == "experience") {
		bool Adjust = Data.ReadBit();
		int Change = Data.Read<int>();
		Player->Character->Experience = std::max(0, Adjust ? Player->Character->Experience + Change : Change);
		Player->Character->CalculateStats();
		SendHUD(Peer);
	}
	else if(Command == "give") {
		uint32_t ItemID = Data.Read<uint32_t>();
		int Count = Data.Read<int>();

		if(Stats->Items.find(ItemID) == Stats->Items.end())
			return;

		SendItem(Peer, Stats->Items.at(ItemID), Count);
	}
	else if(Command == "gold") {
		bool Adjust = Data.ReadBit();
		int Change = Data.Read<int>();
		Player->Character->Gold = Adjust ? Player->Character->Gold + Change : Change;
		if(Player->Character->Gold > PLAYER_MAX_GOLD)
			Player->Character->Gold = PLAYER_MAX_GOLD;
		Player->Character->GoldLost = 0;
		SendHUD(Peer);
	}
	else if(Command == "map") {
		ae::NetworkIDType MapID = Data.Read<ae::NetworkIDType>();
		SpawnPlayer(Player, MapID, _Map::EVENT_MAPENTRANCE);
	}
	else if(Command == "move") {
		uint8_t X = Data.Read<uint8_t>();
		uint8_t Y = Data.Read<uint8_t>();

		Player->Position = Player->Map->GetValidCoord(glm::ivec2(X, Y));
		SendPlayerPosition(Player->Peer);
	}
	else if(Command == "save") {
		SaveTime = Config.AutoSavePeriod;
	}
}

// Handle action use by player
void _Server::HandleActionUse(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->Character->IsAlive())
		return;

	// Set action used
	Player->SetActionUsing(Data, ObjectManager);

	// Check for battle
	if(Player->Character->Battle) {

		// Notify other players of action
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_ACTION);
		Packet.Write<ae::NetworkIDType>(Player->NetworkID);
		if(Player->Character->Action.Item)
			Packet.Write<uint32_t>(Player->Character->Action.Item->ID);
		else
			Packet.Write<uint32_t>(0);

		Player->Character->Battle->BroadcastPacket(Packet);
	}
}

// Handle an action bar change
void _Server::HandleActionBarChanged(ae::_Buffer &Data, ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Read skills
	for(size_t i = 0; i < Player->Character->ActionBar.size(); i++) {
		Player->Character->ActionBar[i].Unserialize(Data, Stats);
		Player->Character->ActionBar[i].ActionBarSlot = i;
	}

	Player->Character->CalculateStats();
}

// Updates the player's HUD
void _Server::SendHUD(ae::_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_HUD);
	Packet.Write<int>(Player->Character->Health);
	Packet.Write<int>(Player->Character->Mana);
	Packet.Write<int>(Player->Character->MaxHealth);
	Packet.Write<int>(Player->Character->MaxMana);
	Packet.Write<int>(Player->Character->Experience);
	Packet.Write<int>(Player->Character->Gold);
	Packet.Write<int>(Player->Character->Bounty);
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
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::STAT_CHANGE);
				StatChange.Serialize(Packet);

				// Send packet to player
				Network->SendPacket(Packet, Object->Peer);
			}
		}
	}
}

// Set server clock
void _Server::SetClock(double Clock) {
	if(!Save)
		return;

	Save->Clock = Clock;

	// Build packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_CLOCK);
	Packet.Write<float>(Clock);

	// Broadcast packet
	for(auto &Peer : Network->GetPeers())
		Network->SendPacket(Packet, Peer);
}

// Update buff on client
void _Server::UpdateBuff(_Object *Player, _StatusEffect *StatusEffect) {

	// Create packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PLAYER_UPDATEBUFF);
	Packet.Write<ae::NetworkIDType>(Player->NetworkID);
	Packet.Write<uint32_t>(StatusEffect->Buff->ID);
	Packet.Write<int>(StatusEffect->Level);
	Packet.Write<float>(StatusEffect->Duration);

	// Notify players in battle
	if(Player->Character->Battle)
		Player->Character->Battle->BroadcastPacket(Packet);
	else
		Network->SendPacket(Packet, Player->Peer);
}

// Slap a misbehaving player
void _Server::Slap(ae::NetworkIDType PlayerID, int GoldAmount) {
	_Object *Player = ObjectManager->GetObject(PlayerID);
	if(!Player)
		return;

	// Penalty
	_StatChange StatChange;
	StatChange.Object = Player;
	StatChange.Values[StatType::GOLD].Integer = -GoldAmount;
	StatChange.Values[StatType::HEALTH].Integer = -Player->Character->Health / 2;
	Player->UpdateStats(StatChange);

	// Build packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::STAT_CHANGE);
	StatChange.Serialize(Packet);
	Network->SendPacket(Packet, Player->Peer);

	// Shame them
	BroadcastMessage(nullptr, Player->Name + " has been slapped for misbehaving!", "yellow");
}

// Send a message to the player
void _Server::SendMessage(ae::_Peer *Peer, const std::string &Message, const std::string &ColorName) {
	if(!ValidatePeer(Peer))
		return;

	// Build message
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHAT_MESSAGE);
	Packet.WriteString(ColorName.c_str());
	Packet.WriteString(Message.c_str());

	// Send
	Network->SendPacket(Packet, Peer);
}

// Broadcast message to all peers
void _Server::BroadcastMessage(ae::_Peer *IgnorePeer, const std::string &Message, const std::string &ColorName) {
	for(auto &Peer : Network->GetPeers()) {
		if(Peer != IgnorePeer)
			SendMessage(Peer, Message, ColorName);
	}
}

// Sends information to another player about items they're trading
void _Server::SendTradeInformation(_Object *Sender, _Object *Receiver) {
	_Bag &Bag = Sender->Inventory->GetBag(BagType::TRADE);

	// Send items to trader player
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_REQUEST);
	Packet.Write<ae::NetworkIDType>(Sender->NetworkID);
	Packet.Write<int>(Sender->Character->TradeGold);
	for(size_t i = 0; i < Bag.Slots.size(); i++)
		Sender->Inventory->SerializeSlot(Packet, _Slot(BagType::TRADE, i));

	Network->SendPacket(Packet, Receiver->Peer);
}

// Add summons to the battle from summon buffs
void _Server::AddBattleSummons(_Battle *Battle, int Side, _Object *JoinPlayer, bool Join) {

	// Get list of objects on a side
	std::list<_Object *> ObjectList;
	Battle->GetObjectList(Side, ObjectList);

	// Iterate over all players in battle, collecting summons for each player
	std::vector<_SummonCaptain> SummonCaptains;
	for(auto &SummonOwner : ObjectList) {
		if(JoinPlayer && SummonOwner != JoinPlayer)
			continue;

		// Collect all summons
		_SummonCaptain SummonCaptain;
		SummonCaptain.Summons.reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
		SummonCaptain.Owner = SummonOwner;
		SummonOwner->Character->GetSummonsFromBuffs(SummonCaptain.Summons);
		std::shuffle(SummonCaptain.Summons.begin(), SummonCaptain.Summons.end(), ae::RandomGenerator);
		SummonCaptains.push_back(SummonCaptain);
	}

	// Shuffle who goes first
	std::shuffle(SummonCaptains.begin(), SummonCaptains.end(), ae::RandomGenerator);

	// Get summons from summon buffs
	int SlotsLeft = BATTLE_MAX_OBJECTS_PER_SIDE - ObjectList.size();
	while(SlotsLeft > 0) {

		// Add summons round-robin
		int Added = 0;
		for(size_t i = 0; i < SummonCaptains.size(); i++) {
			_SummonCaptain &Captain = SummonCaptains[i];

			// Check for any summons left in captain's pool
			if(!Captain.Summons.size())
				continue;

			// Create object
			_Object *Object = CreateSummon(Captain.Owner, Captain.Summons.back().first);

			// If joining battle, broadcast create
			if(Join) {
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::WORLD_CREATEOBJECT);
				Object->SerializeCreate(Packet);
				Battle->BroadcastPacket(Packet);
			}

			// Add object to battle
			Battle->AddObject(Object, Captain.Owner->Fighter->BattleSide, Join);

			// Remove summon from pool and decrement owner's status effect level
			_StatusEffect *StatusEffect = Captain.Summons.back().second;
			StatusEffect->Level--;
			if(StatusEffect->Level <= 0)
				StatusEffect->Duration = 0.0;
			else
				StatusEffect->Duration = StatusEffect->MaxDuration;

			Captain.Summons.pop_back();
			Added++;
			SlotsLeft--;
			if(SlotsLeft <= 0)
				break;
		}

		// No summons left to add
		if(!Added)
			break;
	}
}

// Start a battle event
void _Server::StartBattle(_BattleEvent &BattleEvent) {

	// Return if object is already in battle or in zone 0
	if(BattleEvent.Object->Character->Battle || (!BattleEvent.PVP && !BattleEvent.Zone))
		return;

	// Handle PVP
	if(BattleEvent.PVP) {

		// Get list of players on current tile
		std::list<_Object *> Players;
		BattleEvent.Object->Map->GetPVPPlayers(BattleEvent.Object, Players, BattleEvent.BountyEarned > 0);
		if(!Players.size())
			return;

		// Create a new battle instance
		_Battle *Battle = BattleManager->Create();
		Battle->Manager = ObjectManager;
		Battle->Stats = Stats;
		Battle->Server = this;
		Battle->Scripting = Scripting;
		Battle->PVP = BattleEvent.PVP;
		Battle->BountyEarned = BattleEvent.BountyEarned;
		Battle->BountyClaimed = BattleEvent.BountyClaimed;

		// Add players to battle
		Battle->AddObject(BattleEvent.Object, BattleEvent.Side);
		for(auto &TargetPlayer : Players) {
			Battle->AddObject(TargetPlayer, !BattleEvent.Side);
		}

		// Add summons
		AddBattleSummons(Battle, 0);
		AddBattleSummons(Battle, 1);

		// Send battle to players
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_START);
		Battle->Serialize(Packet);
		Battle->BroadcastPacket(Packet);
	}
	else {

		// Get a list of players
		std::list<_Object *> Players;
		BattleEvent.Object->Map->GetPotentialBattlePlayers(BattleEvent.Object, BATTLE_COOP_DISTANCE, BATTLE_MAX_OBJECTS_PER_SIDE-1, Players);
		int AdditionalCount = 0;
		if(!BattleEvent.Scripted)
			AdditionalCount = (int)Players.size();

		// Get monsters
		std::list<_Zone> Monsters;
		bool Boss = false;
		double Cooldown = 0.0;
		Stats->GenerateMonsterListFromZone(AdditionalCount, BattleEvent.Zone, Monsters, Boss, Cooldown);

		// Fight if there are monsters
		if(!Monsters.size())
			return;

		// Check for cooldown
		if(BattleEvent.Object->Character->BattleCooldown.find(BattleEvent.Zone) != BattleEvent.Object->Character->BattleCooldown.end())
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
		int Difficulty = 100;
		if(Scripting->StartMethodCall("Game", "GetDifficulty")) {
			Scripting->PushReal(Save->Clock);
			Scripting->MethodCall(1, 1);
			Difficulty = Scripting->GetInt(1);
			Scripting->FinishMethodCall();
		}

		// Get difficulty increase
		int DifficultyAdjust = GAME_DIFFICULTY_PER_PLAYER;
		if(Boss)
			DifficultyAdjust = GAME_DIFFICULTY_PER_PLAYER_BOSS;

		// Add players to battle
		Difficulty -= DifficultyAdjust;
		for(auto &PartyPlayer : Players) {
			Battle->AddObject(PartyPlayer, 0);

			// Increase difficulty for each player
			Difficulty += DifficultyAdjust;

			// Increase by each player's difficulty stat
			Difficulty += PartyPlayer->Character->Difficulty;
		}

		// Add summons
		AddBattleSummons(Battle, 0);

		// Add monsters
		for(auto &Monster : Monsters) {
			_Object *Object = ObjectManager->Create();
			Object->Server = this;
			Object->Scripting = Scripting;
			Object->Monster->DatabaseID = Monster.MonsterID;
			Object->Monster->Difficulty = Difficulty + Monster.Difficulty;
			Object->Stats = Stats;
			Stats->GetMonsterStats(Monster.MonsterID, Object, Object->Monster->Difficulty);
			Object->Character->CalculateStats();
			Battle->AddObject(Object, 1);
		}

		// Send battle to players
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_START);
		Battle->Serialize(Packet);
		Battle->BroadcastPacket(Packet);
	}
}

// Start a rebirth
void _Server::StartRebirth(_RebirthEvent &RebirthEvent) {
	_Object *Player = RebirthEvent.Object;
	_Character *Character = Player->Character;
	if(Character->Battle)
		return;

	// Load build
	const auto &BuildIterator = Stats->Builds.find(Character->BuildID);
	if(BuildIterator == Stats->Builds.end())
		return;

	const _Object *Build = BuildIterator->second;

	// Save old info
	_Bag OldTradeBag = Player->Inventory->GetBag(BagType::TRADE);
	std::unordered_map<uint32_t, int> OldSkills = Character->Skills;
	std::unordered_map<uint32_t, int> OldMaxSkillLevels = Character->MaxSkillLevels;

	// Reset character
	Character->ActionBar = Build->Character->ActionBar;
	Player->Inventory->Bags = Build->Inventory->GetBags();
	Character->Skills = Build->Character->Skills;
	Character->MaxSkillLevels.clear();
	Character->Unlocks.clear();
	Character->Seed = ae::GetRandomInt((uint32_t)1, std::numeric_limits<uint32_t>::max());
	Character->Gold = Character->Gold * Character->RebirthWealth * 0.01f;
	Character->Experience = Stats->GetLevel(Character->RebirthWisdom + 1)->Experience;
	Character->UpdateTimer = 0;
	Character->SkillPointsUnlocked = 0;
	Character->Vendor = nullptr;
	Character->Trader = nullptr;
	Character->Blacksmith = nullptr;
	Character->Enchanter = nullptr;
	Character->Minigame = nullptr;
	Character->TradePlayer = nullptr;
	Character->TradeGold = 0;
	Character->WaitingForTrade = false;
	Character->TradeAccepted = false;
	Character->TeleportTime = -1;
	Character->MenuOpen = false;
	Character->InventoryOpen = false;
	Character->SkillsOpen = false;
	Character->RebirthTime = 0.0;
	Character->BeltSize = ACTIONBAR_DEFAULT_BELTSIZE;
	Character->SkillBarSize = ACTIONBAR_DEFAULT_SKILLBARSIZE;
	Character->Cooldowns.clear();
	Character->BattleCooldown.clear();
	Character->DeleteStatusEffects();

	// Give bonus
	switch(RebirthEvent.Type) {
		case 1:
			Character->EternalStrength += RebirthEvent.Value;
		break;
		case 2:
			Character->EternalGuard += RebirthEvent.Value;
		break;
		case 3:
			Character->EternalFortitude += RebirthEvent.Value;
		break;
		case 4:
			Character->EternalSpirit += RebirthEvent.Value;
		break;
		case 5:
			Character->EternalWisdom += RebirthEvent.Value;
		break;
		case 6:
			Character->EternalWealth += RebirthEvent.Value;
		break;
		case 7:
			Character->EternalAlacrity += RebirthEvent.Value;
		break;
		case 8:
			Character->EternalKnowledge += RebirthEvent.Value;
		break;
		case 9:
			Character->EternalPain += RebirthEvent.Value;
		break;
	}

	// Keep belt unlocks
	Character->BeltSize = std::min(Character->RebirthGirth + ACTIONBAR_DEFAULT_BELTSIZE, ACTIONBAR_MAX_BELTSIZE);
	Character->UnlockBySearch("Belt Slot %", Character->RebirthGirth);

	// Keep skill bar unlocks
	Character->SkillBarSize = std::min(Character->RebirthProficiency + ACTIONBAR_DEFAULT_SKILLBARSIZE, ACTIONBAR_MAX_SKILLBARSIZE);
	Character->UnlockBySearch("Skill Slot %", Character->RebirthProficiency);

	// Keep skill point unlocks
	Character->SkillPointsUnlocked = Character->UnlockBySearch("Skill Point %", Character->RebirthInsight);

	// Keep items from trade bag
	int ItemCount = Character->RebirthPower;
	for(const auto &Slot : OldTradeBag.Slots) {
		if(ItemCount && Slot.Item) {
			Player->Inventory->AddItem(Slot.Item, Slot.Upgrades, Slot.Count);
			ItemCount--;
			if(ItemCount <= 0)
				break;
		}
	}

	// Get highest skills
	int SkillCount = Character->RebirthKnowledge;
	if(SkillCount) {
		std::list<_HighestSkill> Skills;
		for(const auto &Skill : OldSkills) {
			if(Skill.second > 0)
				Skills.push_back(_HighestSkill(Skill.first, Skill.second));
		}
		Skills.sort();

		// Learn old skills and keep max level
		for(const auto &Skill : Skills) {
			Character->Skills[Skill.ID] = 0;
			Character->MaxSkillLevels[Skill.ID] = OldMaxSkillLevels[Skill.ID];

			SkillCount--;
			if(SkillCount <= 0)
				break;
		}
	}

	Character->CalculateStats();

	// Spawn player
	Character->Health = Character->MaxHealth;
	Character->Mana = Character->MaxMana;
	Character->GenerateNextBattle();
	Character->LoadMapID = 0;
	Character->SpawnMapID = 1;
	Character->SpawnPoint = 0;
	Character->Rebirths++;
	SpawnPlayer(Player, Character->LoadMapID, _Map::EVENT_NONE);
	SendPlayerInfo(Player->Peer);
}
