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
#include <alc.h>
#include <vorbis/vorbisfile.h>
#include <stdexcept>
#include <vector>

_Audio Audio;

// Constructor
_AudioSource::_AudioSource(const _Sound *Sound, float Volume) {

	// Create source
	alGenSources(1, &ID);

	// Assign buffer to source
	alSourcei(ID, AL_BUFFER, Sound->ID);
	alSourcef(ID, AL_GAIN, Volume);
}

// Destructor
_AudioSource::~_AudioSource() {
	alDeleteSources(1, &ID);
}

// Determine if source is actively playing
bool _AudioSource::IsPlaying() {

	// Get state
	ALenum State;
	alGetSourcei(ID, AL_SOURCE_STATE, &State);

	return State == AL_PLAYING;
}

// Play
void _AudioSource::Play() {
	if(ID)
		alSourcePlay(ID);
}

// Stop
void _AudioSource::Stop() {
	if(ID)
		alSourceStop(ID);
}

// Destructor
_Sound::~_Sound() {
	alDeleteBuffers(1, &ID);
}

// Destructor
_Music::~_Music() {
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
	if(!Enabled)
		return;

	// Open device
	ALCdevice *Device = alcOpenDevice(nullptr);
	if(Device == nullptr)
		throw std::runtime_error("alcOpenDevice failed");

	// Create context
	ALCcontext *Context = alcCreateContext(Device, nullptr);

	// Set active context
	alcMakeContextCurrent(Context);

	// Clear code
	alGetError();

	this->Enabled = Enabled;
}

// Close
void _Audio::Close() {
	if(!Enabled)
		return;

	// Delete sources
	for(auto &Iterator : Sources)
		delete Iterator;

	Sources.clear();

	// Get active context
	ALCcontext *Context = alcGetCurrentContext();

	// Get device for active context
	ALCdevice *Device = alcGetContextsDevice(Context);

	// Disable context
	alcMakeContextCurrent(nullptr);

	// Free context
	alcDestroyContext(Context);

	// Close device
	alcCloseDevice(Device);

	Enabled = false;
}

// Update audio
void _Audio::Update(double FrameTime) {
	if(!Enabled)
		return;

	// Update sources
	for(auto Iterator = Sources.begin(); Iterator != Sources.end(); ) {
		_AudioSource *Source = *Iterator;

		// Delete source
		if(!Source->IsPlaying()) {
			delete Source;
			Iterator = Sources.erase(Iterator);
		}
		else {
			++Iterator;
		}
	}
}

// Load sound
_Sound *_Audio::LoadSound(const std::string &Path) {
	if(!Enabled)
		return nullptr;

	// Open vorbis stream
	OggVorbis_File VorbisStream;
	int ReturnCode = ov_fopen(Path.c_str(), &VorbisStream);
	if(ReturnCode != 0)
		throw std::runtime_error("ov_fopen failed: ReturnCode=" + std::to_string(ReturnCode));

	// Get vorbis file info
	vorbis_info *Info = ov_info(&VorbisStream, -1);

	// Create new buffer
	_Sound *Sound = new _Sound();
	switch(Info->channels) {
		case 1:
			Sound->Format = AL_FORMAT_MONO16;
		break;
		case 2:
			Sound->Format = AL_FORMAT_STEREO16;
		break;
		default:
			throw std::runtime_error("Unsupported number of channels: " + std::to_string(Info->channels));
		break;
	}

	// Alloc some memory for the samples
	std::vector<char> Data;

	// Decode vorbis file
	long BytesRead;
	char Buffer[4096];
	int BitStream;
	do {
		BytesRead = ov_read(&VorbisStream, Buffer, 4096, 0, 2, 1, &BitStream);
		Data.insert(Data.end(), Buffer, Buffer + BytesRead);
	} while(BytesRead > 0);

	// Create buffer
	alGenBuffers(1, &Sound->ID);
	alBufferData(Sound->ID, Sound->Format, &Data[0], (ALsizei)Data.size(), (ALsizei)Info->rate);

	// Close vorbis file
	ov_clear(&VorbisStream);

	return Sound;
}

// Load music
_Music *_Audio::LoadMusic(const std::string &Path) {
	if(!Enabled)
		return nullptr;

	_Music *Music = new _Music();

	return Music;
}

// Set sound volume
void _Audio::SetSoundVolume(float Volume) {
	if(!Enabled)
		return;

	SoundVolume = Volume;
}

// Set music volume
void _Audio::SetMusicVolume(float Volume) {
	if(!Enabled)
		return;

	MusicVolume = Volume;
}

// Play a sound
void _Audio::PlaySound(_Sound *Sound) {
	if(!Enabled || !Sound)
		return;

	// Create audio source
	_AudioSource *AudioSource = new _AudioSource(Sound, SoundVolume);
	AudioSource->Play();

	// Add to sources
	Sources.push_back(AudioSource);
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

		SongPlaying = Music;
	}
}

// Stop all music
void _Audio::StopMusic(int FadeTime) {

	SongPlaying = nullptr;
}
