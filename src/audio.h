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
#include <vorbis/vorbisfile.h>
#include <thread>
#include <list>
#include <string>

// Sound class
class _Sound {

	public:

		_Sound() : ID(0) { }
		~_Sound();

		ALuint ID;
};

// Music class
class _Music {

	public:

		_Music() : Loaded(false), Loop(false), Stop(false), Format(0), Frequency(0) { }
		~_Music();

		bool Loaded;
		bool Loop;
		bool Stop;
		ALenum Format;
		long Frequency;
		OggVorbis_File Stream;
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

		static const int BUFFER_COUNT = 3;
		static const int BUFFER_SIZE = 4096;

		_Audio();

		void Init(bool Enabled);
		void Close();

		void Update(double FrameTime);
		void UpdateMusic();

		_Sound *LoadSound(const std::string &Path);
		_Music *LoadMusic(const std::string &Path);

		void Stop();
		void StopSounds();
		void StopMusic();
		void PlaySound(_Sound *Sound);
		void PlayMusic(_Music *Music, bool Loop=true);

		void SetSoundVolume(float Volume);
		void SetMusicVolume(float Volume);

		bool Done;

	private:

		long ReadStream(OggVorbis_File &Stream, char *Buffer, int Size);
		void OpenVorbis(const std::string &Path, OggVorbis_File *Stream, long &Rate, int &Format);

		bool Enabled;

		float SoundVolume;
		float MusicVolume;

		ALuint MusicSource;
		ALuint MusicBuffers[BUFFER_COUNT];

		_Music *CurrentSong;
		_Music *NewSong;

		std::list<_AudioSource *> Sources;

		std::thread *Thread;
};

extern _Audio Audio;
