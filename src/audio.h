/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <al.h>
#include <list>
#include <string>

// Sound class
class _Sound {

	public:

		_Sound() : ID(0), Format(0) { }
		~_Sound();

		ALuint ID;
		ALenum Format;
};

// Music class
class _Music {

	public:

		_Music() { }
		~_Music();
};

// Audio source class
class _AudioSource {

	public:

		_AudioSource(const _Sound *Sound, float Volume=1.0f);
		~_AudioSource();

		void Play();
		void Stop();

		bool IsPlaying();

		ALuint ID;
};

// Classes
class _Audio {

	public:

		_Audio();

		void Init(bool Enabled);
		void Close();

		void Update(double FrameTime);

		_Sound *LoadSound(const std::string &Path);
		_Music *LoadMusic(const std::string &Path);

		void SetSoundVolume(float Volume);
		void SetMusicVolume(float Volume);

		void PlaySound(_Sound *Sound);
		void PlayMusic(_Music *Music, int FadeTime=0);
		void StopMusic(int FadeTime=0);

	private:

		bool Enabled;

		float SoundVolume;
		float MusicVolume;

		const _Music *SongPlaying;

		std::list<_AudioSource *> Sources;
};

extern _Audio Audio;
