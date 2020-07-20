/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020  Alan Witkowski
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
#include <objects/components/prop.h>

// Constructor
_Prop::_Prop(_Object *Object) :
	Object(Object),
	Texture(nullptr),
	Color(1.0f),
	Z(0.0f),
	Floor(true),
	Repeat(false) {

}

// Copy attributes from another prop
void _Prop::CopyAttributes(_Prop *Prop) {
	Texture = Prop->Texture;
	Color = Prop->Color;
	Z = Prop->Z;
	Floor = Prop->Floor;
	Repeat = Prop->Repeat;
}
