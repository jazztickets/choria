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
		void PlaySound(_Sound *Sound, float Volume=1.0f);
		void PlayMusic(_Music *Music, bool Loop=true);

		void SetSoundVolume(float Volume);
		void SetMusicVolume(float Volume);

		bool Done;

	private:

		long ReadStream(OggVorbis_File *Stream, char *Buffer, int Size);
		void OpenVorbis(const std::string &Path, OggVorbis_File *Stream, long &Rate, int &Format);
		bool QueueBuffers(_Music *Music, ALuint Buffer);

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
