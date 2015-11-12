/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#pragma once

// Libraries
#include <SDL_timer.h>

class _FrameLimit {

	public:

		// Constructor
		_FrameLimit(double FrameRate, bool Vsync) : FrameRate(FrameRate), Vsync(Vsync) { Reset(); }

		// Set the frame timer = now()
		void Reset() {
			Timer = SDL_GetPerformanceCounter();
		}

		// Limit frame rate
		void Update() {
			if(Vsync)
				return;

			// Get frame time
			double LastFrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();

			// Limit frame rate
			if(FrameRate > 0.0) {
				double ExtraTime = 1.0 / FrameRate - LastFrameTime;
				if(ExtraTime > 0.0) {
					SDL_Delay((Uint32)(ExtraTime * 1000));
				}
			}

			// Reset timer after delay
			Reset();
		}

		// Attributes
		double FrameRate;
		bool Vsync;

	private:

		// Time
		Uint64 Timer;
};
