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
#include <states/oldclient.h>
#include <globals.h>
#include <framework.h>
#include <graphics.h>
#include <input.h>
#include <actions.h>
#include <stats.h>
#include <hud.h>
#include <buffer.h>
#include <assets.h>
#include <camera.h>
#include <program.h>
#include <menu.h>
#include <packet.h>
#include <ui/element.h>
#include <network/oldnetwork.h>
#include <instances/map.h>
#include <instances/clientbattle.h>
#include <objects/object.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

_OldClientState OldClientState;
