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
#include <ae/random.h>
#include <ae/util.h>
#include <scripting.h>

// Constructor
_Light::_Light(_Object *Object) :
	Object(Object),
	LightTypeID(0),
	Texture(nullptr),
	Color(1.0f),
	FinalColor(1.0f),
	Seed(0),
	Time(0.0) {

	Seed = ae::GetRandomInt((uint32_t)0, std::numeric_limits<uint32_t>::max());
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
	if(Scripting && Scripting->StartMethodCall("Light_" + Function, "Update")) {
		Scripting->PushLight(this);
		Scripting->PushReal(Time);
		for(size_t i = 0; i < Parameters.size(); i++)
			Scripting->PushReal(Parameters[i]);
		Object->Scripting->MethodCall(2 + (int)Parameters.size(), 1);
		Object->Scripting->GetLight(1, this);
		Object->Scripting->FinishMethodCall();
	}
}

// Set script function and parameters
void _Light::SetScript(const std::string &Line) {
	Script = Line;

	// Convert to string tokens
	std::vector<std::string> StringParameters;
	ae::TokenizeString(Line, StringParameters);
	if(StringParameters.empty())
	   return;

	// Set function
	Function = StringParameters[0];

	// Set number parameters
	Parameters.clear();
	for(size_t i = 1; i < StringParameters.size(); i++)
		Parameters.push_back(std::stod(StringParameters[i]));
}
