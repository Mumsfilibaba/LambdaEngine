#include "Audio/AudioSystem.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	IAudioDevice*	AudioSystem::s_pAudioDevice = nullptr;

	bool AudioSystem::Init()
	{

		AudioDeviceDesc audioDeviceDesc = {};
		audioDeviceDesc.pName					= "Main AudioDevice";
		audioDeviceDesc.Debug					= true;
		audioDeviceDesc.SpeakerSetup			= ESpeakerSetup::STEREO_HEADPHONES;
		audioDeviceDesc.MasterVolume			= 0.5f;
		audioDeviceDesc.MaxNumAudioListeners	= 1;
		audioDeviceDesc.MaxWorldSize			= 200;

		s_pAudioDevice = CreateAudioDevice(EAudioAPI::LAMBDA, audioDeviceDesc);

		return true;
	}

	bool AudioSystem::Release()
	{
		SAFEDELETE(s_pAudioDevice)
		return true;
	}

	void AudioSystem::FixedTick(Timestamp delta)
	{
		s_pAudioDevice->FixedTick(delta);
	}
}
