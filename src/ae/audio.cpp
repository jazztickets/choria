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
#include <ae/audio.h>
#include <alc.h>
#include <stdexcept>
#include <vector>

_Audio Audio;

// Function to run the audio thread
static void RunThread(void *Arguments) {

	// Loop until audio system is closed
	while(!Audio.Done) {

		// Update streams
		Audio.UpdateMusic();

		// Sleep thread
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

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
	if(Loaded)
		ov_clear(&Stream);
}

// Constructor
_Audio::_Audio() :
	Done(false),
	Enabled(false),
	SoundVolume(1.0f),
	MusicVolume(1.0f),
	MusicSource(0),
	CurrentSong(nullptr),
	NewSong(nullptr),
	Thread(nullptr) {

	for(int i = 0; i < BUFFER_COUNT; i++)
		MusicBuffers[i] = 0;
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

	// Create music buffers
	alGenBuffers(BUFFER_COUNT, MusicBuffers);
	alGenSources(1, &MusicSource);

	// Clear code
	alGetError();

	CurrentSong = nullptr;
	NewSong = nullptr;
	Done = false;

	// Start thread
	Thread = new std::thread(RunThread, this);

	this->Enabled = Enabled;
}

// Close
void _Audio::Close() {
	if(!Enabled)
		return;

	// Close thread
	Done = true;
	if(Thread)
		Thread->join();

	delete Thread;
	Thread = nullptr;

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

// Stop all sound and music
void _Audio::Stop() {

	StopSounds();
	StopMusic();
}

// Stop all sounds
void _Audio::StopSounds() {
	for(auto &Iterator : Sources) {
		_AudioSource *Source = Iterator;
		Source->Stop();

		delete Source;
	}

	Sources.clear();
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

// Update music stream
void _Audio::UpdateMusic() {

	// Check for stopped song
	if(CurrentSong && CurrentSong->Stop) {
		CurrentSong = nullptr;
	}

	// Check for track change
	if(NewSong) {
		alSourceStop(MusicSource);

		// Clear queued buffers
		ALint Queued;
		alGetSourcei(MusicSource, AL_BUFFERS_QUEUED, &Queued);
		while(Queued--) {
			ALuint CurrentBuffer;
			alSourceUnqueueBuffers(MusicSource, 1, &CurrentBuffer);
		}

		// Seek to beginning
		ov_time_seek(&NewSong->Stream, 0);

		// Load initial buffers
		for(int i = 0; i < BUFFER_COUNT; i++) {

			// Queue buffers
			bool End = QueueBuffers(NewSong, MusicBuffers[i]);
			if(End) {
				NewSong = nullptr;
				break;
			}
		}

		// Play music
		alSourcef(MusicSource, AL_GAIN, MusicVolume);
		alSourcePlay(MusicSource);

		CurrentSong = NewSong;
		NewSong = nullptr;
	}

	// Get number of buffers processed
	ALint Processed;
	alGetSourcei(MusicSource, AL_BUFFERS_PROCESSED, &Processed);

	// Handle processed buffers
	while(Processed-- > 0) {

		// Pop buffer
		ALuint CurrentBuffer;
		alSourceUnqueueBuffers(MusicSource, 1, &CurrentBuffer);

		// Check for song
		if(CurrentSong) {

			// Queue buffers
			bool End = QueueBuffers(CurrentSong, CurrentBuffer);
			if(End)
				CurrentSong = nullptr;
		}
	}
}

// Load sound
_Sound *_Audio::LoadSound(const std::string &Path) {
	if(!Enabled)
		return nullptr;

	// Open file
	OggVorbis_File VorbisStream;
	long Rate;
	int Format;
	OpenVorbis(Path, &VorbisStream, Rate, Format);

	// Alloc some memory for the samples
	std::vector<char> Data;

	// Decode vorbis file
	long BytesRead;
	char Buffer[BUFFER_SIZE];
	int BitStream;
	do {
		BytesRead = ov_read(&VorbisStream, Buffer, BUFFER_SIZE, 0, 2, 1, &BitStream);
		Data.insert(Data.end(), Buffer, Buffer + BytesRead);
	} while(BytesRead > 0);

	// Create buffer
	_Sound *Sound = new _Sound();
	alGenBuffers(1, &Sound->ID);
	alBufferData(Sound->ID, Format, &Data[0], (ALsizei)Data.size(), (ALsizei)Rate);

	// Close vorbis file
	ov_clear(&VorbisStream);

	return Sound;
}

// Load music
_Music *_Audio::LoadMusic(const std::string &Path) {
	if(!Enabled)
		return nullptr;

	_Music *Music = new _Music();

	// Get vorbis file info
	OpenVorbis(Path, &Music->Stream, Music->Frequency, Music->Format);
	Music->Loaded = true;

	return Music;
}

// Play a sound
void _Audio::PlaySound(_Sound *Sound, float Volume) {
	if(!Enabled || !Sound)
		return;

	// Create audio source
	_AudioSource *AudioSource = new _AudioSource(Sound, SoundVolume * Volume);
	AudioSource->Play();

	// Add to sources
	Sources.push_back(AudioSource);
}

// Play music
void _Audio::PlayMusic(_Music *Music, bool Loop) {
	if(!Enabled)
		return;

	// Stop music if playing nothing
	if(!Music) {
		StopMusic();
		return;
	}

	// Change songs only
	if(CurrentSong != Music) {
		NewSong = Music;
		NewSong->Stop = false;
		NewSong->Loop = Loop;
	}
}

// Stop all music
void _Audio::StopMusic() {
	alSourceStop(MusicSource);

	// Request song stop
	if(CurrentSong)
		CurrentSong->Stop = true;

	NewSong = nullptr;

	// Wait for thread to kill current song
	while(CurrentSong) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

// Read data from a vorbis stream
long _Audio::ReadStream(OggVorbis_File *Stream, char *Buffer, int Size) {

	// Decode vorbis file
	int BitStream;
	long BytesRead = ov_read(Stream, Buffer, Size, 0, 2, 1, &BitStream);

	return BytesRead;
}

// Open vorbis file and return format/rate
void _Audio::OpenVorbis(const std::string &Path, OggVorbis_File *Stream, long &Rate, int &Format) {

	// Open file
	int ReturnCode = ov_fopen(Path.c_str(), Stream);
	if(ReturnCode != 0)
		throw std::runtime_error("ov_fopen failed: ReturnCode=" + std::to_string(ReturnCode));

	// Get vorbis file info
	vorbis_info *Info = ov_info(Stream, -1);
	Rate = Info->rate;

	// Get openal format
	switch(Info->channels) {
		case 1:
			Format = AL_FORMAT_MONO16;
		break;
		case 2:
			Format = AL_FORMAT_STEREO16;
		break;
		default:
			throw std::runtime_error("Unsupported number of channels: " + std::to_string(Info->channels));
	}
}

// Queue openal buffers with vorbis stream data, return true on stream end or error
bool _Audio::QueueBuffers(_Music *Music, ALuint Buffer) {

	// Decode vorbis stream
	char Data[BUFFER_SIZE];
	long BytesNeeded = BUFFER_SIZE;
	while(BytesNeeded > 0) {

		// Read some bytes
		int BitStream;
		long BytesRead = ov_read(&Music->Stream, Data + (BUFFER_SIZE - BytesNeeded), (int)BytesNeeded, 0, 2, 1, &BitStream);

		// Check for errors
		if(BytesRead < 0) {
			return true;
		}
		// Handle track end
		else if(BytesRead == 0) {
			if(Music->Loop) {
				ov_time_seek(&Music->Stream, 0);
			}
			else {
				return true;
			}
		}
		// Subtract from total bytes to read
		else {
			BytesNeeded -= BytesRead;
		}
	}

	if(Music) {
		alBufferData(Buffer, Music->Format, Data, (ALsizei)BUFFER_SIZE, (ALsizei)Music->Frequency);
		alSourceQueueBuffers(MusicSource, 1, &Buffer);
	}

	return false;
}

// Set sound volume
void _Audio::SetSoundVolume(float Volume) {
	if(!Enabled)
		return;

	SoundVolume = std::min(std::max(Volume, 0.0f), 1.0f);
}

// Set music volume
void _Audio::SetMusicVolume(float Volume) {
	if(!Enabled)
		return;

	MusicVolume = std::min(std::max(Volume, 0.0f), 1.0f);
	alSourcef(MusicSource, AL_GAIN, MusicVolume);
}