/******************************************************************************
* godmode - http://godmode.googlecode.com/
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

// Libraries
#include <list>
#include <cstdint>

// Forward Declarations
class _Object;
class _Map;

// Classes
class _ObjectManager {

	public:

		_ObjectManager();
		~_ObjectManager();

		void Update(double FrameTime);
		void Render(const _Map *TMap, _Object *TClientPlayer=nullptr);
		void SetObjectDeletedCallback(void (* Callback)(_Object *TObject)) { ObjectDeletedCallback = Callback; }

		void ClearObjects();
		void DeletesObjectsExcept(_Object *TObject);
		_Object *CreateObjectFromTemplate(int TTemplateID);
		_Object *AddObject(_Object *TObject);
		_Object *AddObjectWithNetworkID(_Object *TObject, int TNetworkID);
		void DeleteObject(_Object *TObject);

		int GetObjectCount() const { return Objects.size(); }
		const std::list<_Object *> &GetObjects() const { return Objects; }
		_Object *GetObjectFromNetworkID(int TID);

	private:

		int GetNextNetworkID();
		std::list<_Object *> Objects;

		// Networking
		_Object **ObjectArray;
		void (* ObjectDeletedCallback)(_Object *TObject);

		int NextNetworkID;
};
