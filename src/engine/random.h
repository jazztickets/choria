/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
**************************************************************************************/
/*
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   Any feedback is very welcome.
   http://www.math.keio.ac.jp/matumoto/emt.html
   email: matumoto@math.keio.ac.jp
*/
#ifndef RANDOM_H
#define RANDOM_H

#include "singleton.h"

typedef unsigned int uint;
typedef unsigned long ulong;

class RandomClass {

    public:

        RandomClass();
        RandomClass(ulong TSeed);

        void SetSeed(ulong TSeed);

        float Generate();
        int GenerateMax(int TMax);
        uint GenerateMax(uint TMax);
        int GenerateRange(int TMin, int TMax);
        uint GenerateRange(uint TMin, uint TMax);
        float GenerateRange(float TMin, float TMax);
	
	private:
	
		// Mersenne Twister variables
        static const ulong N = 624;
        static const ulong M = 397;
        static const ulong MATRIX_A = 0x9908b0dfUL;
        static const ulong UPPER_MASK = 0x80000000UL;
        static const ulong LOWER_MASK = 0x7fffffffUL;

        ulong rStateVector[N];
        ulong rStateVectorIndex;
        ulong GenerateRandomInteger();

};

// Constructor
inline RandomClass::RandomClass() {

    SetSeed(0);
}

// Constructor
inline RandomClass::RandomClass(ulong TSeed) {

    SetSeed(TSeed);
}

// Sets the seed for the generator
inline void RandomClass::SetSeed(ulong TSeed) {

    rStateVector[0]= TSeed & 0xffffffffUL;
    for(rStateVectorIndex = 1; rStateVectorIndex < N; ++rStateVectorIndex) {
        rStateVector[rStateVectorIndex] = (1812433253UL * (rStateVector[rStateVectorIndex-1] ^ (rStateVector[rStateVectorIndex-1] >> 30)) + rStateVectorIndex); 
        rStateVector[rStateVectorIndex] &= 0xffffffffUL;
    }
}

// Generates a random integer
inline ulong RandomClass::GenerateRandomInteger() {
    static ulong mag01[2] = {0x0UL, MATRIX_A};
    ulong y;

    if(rStateVectorIndex >= N) {
        uint kk;

        for(kk = 0; kk < N-M; ++kk) {
            y = (rStateVector[kk] & UPPER_MASK) | (rStateVector[kk+1] & LOWER_MASK);
            rStateVector[kk] = rStateVector[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }

        for(;kk < N-1; ++kk) {
            y = (rStateVector[kk] & UPPER_MASK) | (rStateVector[kk+1] & LOWER_MASK);
            rStateVector[kk] = rStateVector[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }

        y = (rStateVector[N-1] & UPPER_MASK) | (rStateVector[0] & LOWER_MASK);
        rStateVector[N-1] = rStateVector[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        rStateVectorIndex = 0;
    }

    y = rStateVector[rStateVectorIndex++];

    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

// Generates a random number [0, 1)
inline float RandomClass::Generate() {

	return GenerateRange(0.0f, 0.999999f);
}
 
// Generates a random number [0, TMax]
inline int RandomClass::GenerateMax(int TMax) {

    return GenerateRandomInteger() % TMax;
}

// Generates a random number [0, TMax]
inline uint RandomClass::GenerateMax(uint TMax) {

    return GenerateRandomInteger() % TMax;
}

// Generates a random number [TMin, TMax]
inline int RandomClass::GenerateRange(int TMin, int TMax) {

    return GenerateRandomInteger() % (TMax - TMin + 1) + TMin;
}

// Generates a random number [TMin, TMax]
inline uint RandomClass::GenerateRange(uint TMin, uint TMax) {

    return GenerateRandomInteger() % (TMax - TMin + 1) + TMin;
}

// Generates a random number [TMin, TMax]
inline float RandomClass::GenerateRange(float TMin, float TMax) {

    return GenerateRandomInteger() * (1.0f / 4294967295.0f) * (TMax - TMin) + TMin;
}

// Singletons
typedef SingletonClass<RandomClass> Random;

#endif
