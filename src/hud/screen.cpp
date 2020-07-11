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
#include <hud/character_screen.h>
#include <ae/ui.h>

// Constructor
_Screen::_Screen(_HUD *HUD, ae::_Element *Element) :
	HUD(HUD),
	Element(Element) {

	Element->SetActive(false);
}

// Toggle screen
void _Screen::Toggle() {
	Element->SetActive(!Element->Active);
}
