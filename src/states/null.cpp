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
#include <states/null.h>
#include <framework.h>
#include <menu.h>

_NullState NullState;

void _NullState::Init() {
	Menu.InitTitle();
}

void _NullState::Close() {
}

// Action handler
bool _NullState::HandleAction(int InputType, int Action, int Value) {

	return false;
}

// Key handler
void _NullState::KeyEvent(const _KeyEvent &KeyEvent) {
	Menu.KeyEvent(KeyEvent);
}

// Text handler
void _NullState::TextEvent(const char *Text) {
	Menu.TextEvent(Text);
}

// Mouse handler
void _NullState::MouseEvent(const _MouseEvent &MouseEvent) {
	Menu.MouseEvent(MouseEvent);
}

// Update
void _NullState::Update(double FrameTime) {
	Menu.Update(FrameTime);
}

// Render the state
void _NullState::Render(double BlendFactor) {
	Menu.Render();
}

// Connect
void _NullState::HandleConnect(ENetEvent *TEvent) {
	Menu.HandleConnect(TEvent);
}

// Disconnect
void _NullState::HandleDisconnect(ENetEvent *TEvent) {
	Menu.HandleDisconnect(TEvent);
}

// Packet
void _NullState::HandlePacket(ENetEvent *TEvent) {
	Menu.HandlePacket(TEvent);
}
