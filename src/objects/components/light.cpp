/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <objects/components/light.h>
#include <objects/object.h>
#include <scripting.h>

// Constructor
_Light::_Light(_Object *Object) :
	Object(Object),
	LightTypeID(0),
	Texture(nullptr),
	Color(1.0f),
	FinalColor(1.0f),
	Time(0.0) {

}

// Update light
void _Light::Update(double FrameTime) {
	FinalColor = Color;
	Time += FrameTime;

	// Check scripting
	if(Script.empty())
		return;

	// Call update
	_Scripting *Scripting = Object->Scripting;
	if(Scripting && Scripting->StartMethodCall("Light_" + Script, "Update")) {
		Scripting->PushLight(this);
		Scripting->PushReal(Time);
		Object->Scripting->MethodCall(2, 1);
		Object->Scripting->GetLight(1, this);
		Object->Scripting->FinishMethodCall();
	}
}
