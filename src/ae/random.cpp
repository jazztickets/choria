/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/random.h>

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
