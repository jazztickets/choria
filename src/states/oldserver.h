/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2015  Alan Witkowski
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
#pragma once

/*
// Classes
class _OldServerState : public _State {

	public:

		void Init();
		void Close();

		void Update(double FrameTime);
		void DeleteObject(_Object *Object);

		void PlayerTeleport(_Object *Player);
		double GetServerTime() const { return ServerTime; }

		void StartCommandThread();
		void StopServer() { StopRequested = true; }
		void SendMessage(_Object *Player, const std::string &Message, const glm::vec4 &Color=glm::vec4(1.0f));

	private:

		void SendHUD(_Object *Player);
		void SendCharacterList(_Object *Player);
		void SendEvent(_Object *Player, int Type, int Data);
		void SendTradeInformation(_Object *Sender, _Object *Receiver);

		void BuildTradeItemsPacket(_Object *Player, _Buffer *Packet, int Gold);

		void RemovePlayerFromBattle(_Object *Player);

};
*/