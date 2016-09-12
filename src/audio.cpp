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
#include <audio.h>
#include <stdexcept>
#include <SDL_mixer.h>

_Audio Audio;

// Destructor
_Sound::~_Sound() {
	Mix_FreeChunk(Chunk);
}

// Destructor
_Music::~_Music() {
	Mix_FreeMusic(Music);
}

// Constructor
_Audio::_Audio() :
	Enabled(false),
	SoundVolume(1.0f),
	MusicVolume(1.0f),

	SongPlaying(nullptr) {
}

// Initialize
void _Audio::Init(bool Enabled) {
	if(Enabled) {
		int MixFlags = MIX_INIT_OGG;
		int MixInit = Mix_Init(MixFlags);
		if((MixInit & MixFlags) != MixFlags)
			throw std::runtime_error("Mix_Init failed: " + std::string(Mix_GetError()));

		if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
			throw std::runtime_error("Mix_OpenAudio failed: " + std::string(Mix_GetError()));

		this->Enabled = Enabled;
	}
}

// Close
void _Audio::Close() {
	if(Enabled) {
		Mix_CloseAudio();
		Mix_Quit();
	}
}

// Load sound
_Sound *_Audio::LoadSound(const std::string &Path) {
	if(!Enabled)
		return nullptr;

	_Sound *Sound = new _Sound();
	Sound->Chunk = Mix_LoadWAV(Path.c_str());

	return Sound;
}


// Load music
_Music *_Audio::LoadMusic(const std::string &Path) {
	if(!Enabled)
		return nullptr;

	_Music *Music = new _Music();
	Music->Music = Mix_LoadMUS(Path.c_str());

	return Music;
}

// Set sound volume
void _Audio::SetSoundVolume(float Volume) {
	if(!Enabled)
		return;

	SoundVolume = Volume;

	Mix_Volume(-1, (int)(SoundVolume * MIX_MAX_VOLUME));
}

// Set music volume
void _Audio::SetMusicVolume(float Volume) {
	if(!Enabled)
		return;

	MusicVolume = Volume;

	Mix_VolumeMusic((int)(MusicVolume * MIX_MAX_VOLUME));
}

// Play a sound
void _Audio::PlaySound(_Sound *Sound) {
	if(!Enabled || !Sound)
		return;

	Mix_PlayChannel(-1, Sound->Chunk, 0);
}

// Play music
void _Audio::PlayMusic(_Music *Music, int FadeTime) {
	if(!Enabled)
		return;

	if(!Music) {
		StopMusic();
		return;
	}

	if(SongPlaying != Music) {
		if(FadeTime)
			Mix_FadeInMusic(Music->Music, -1, FadeTime);
		else
			Mix_PlayMusic(Music->Music, -1);

		SongPlaying = Music;
	}
}

// Stop all music
void _Audio::StopMusic(int FadeTime) {
	if(FadeTime)
		Mix_FadeOutMusic(FadeTime);
	else
		Mix_HaltMusic();

	SongPlaying = nullptr;
}
