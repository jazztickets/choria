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
#include <random.h>

std::mt19937 RandomGenerator;

int GetRandomInt(int Min, int Max) {
	std::uniform_int_distribution<int> Distribution(Min, Max);
	return Distribution(RandomGenerator);
}

uint32_t GetRandomInt(uint32_t Min, uint32_t Max) {
	std::uniform_int_distribution<uint32_t> Distribution(Min, Max);
	return Distribution(RandomGenerator);
}

uint64_t GetRandomInt(uint64_t Min, uint64_t Max) {
	std::uniform_int_distribution<uint64_t> Distribution(Min, Max);
	return Distribution(RandomGenerator);
}

double GetRandomReal(double Min, double Max) {
	std::uniform_real_distribution<double> Distribution(Min, Max);
	return Distribution(RandomGenerator);
}
