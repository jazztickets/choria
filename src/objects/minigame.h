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
#pragma once

// Libraries
#include <ae/manager.h>
#include <ae/ui.h>
#include <random>

// Forward Declarations
class _Sprite;
class _Camera;
struct _MinigameItem;
struct _MinigameType;
struct _MouseEvent;

// Base minigame class
class _Minigame {

	public:

		enum class StateType : int {
			NEEDSEED,
			CANDROP,
			DROPPED,
			DONE,
		};

		_Minigame(const _MinigameType *Minigame);
		~_Minigame();

		// Update
		void Update(double FrameTime);
		void Render(double BlendFactor);
		void HandleMouseButton(const _MouseEvent &MouseEvent);
		void StartGame(uint64_t Seed);
		void Drop(float X);
		void RefreshPrizes();
		void GetUIBoundary(_Bounds &Bounds);

		// Attributes
		std::mt19937 Random;
		_Bounds Boundary;
		_Manager<_Sprite> *Sprites;
		_Sprite *Ball;
		_Camera *Camera;
		const _MinigameType *Minigame;
		std::vector<const _MinigameItem *> Prizes;

		// State
		StateType State;
		double Time;
		float DropX;
		size_t Bucket;

	private:

};