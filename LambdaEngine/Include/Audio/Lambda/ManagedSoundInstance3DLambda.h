#pragma once

#include "LambdaEngine.h"
#include "LambdaAudio.h"

#include "Audio/API/AudioTypes.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	struct AudioListenerDesc;

	class IAudioDevice;
	class AudioDeviceLambda;
	class ISoundEffect3D;
	class SoundEffect3DLambda;

	struct ManagedSoundInstance3DDesc
	{
		const char*			pName				= "ManagedSoundInstance3D";
		ISoundEffect3D*		pSoundEffect		= nullptr;
		uint32				Flags				= FSoundModeFlags::SOUND_MODE_NONE;
		glm::vec3			Position			= glm::vec3(0.0f);
		glm::vec3			Velocity			= glm::vec3(0.0f);
		float64				Volume				= 1.0;
		float32				Pitch				= 1.0f;
	};

	class ManagedSoundInstance3DLambda
	{
	public:
		ManagedSoundInstance3DLambda(const IAudioDevice* pAudioDevice);
		~ManagedSoundInstance3DLambda();

		bool Init(const ManagedSoundInstance3DDesc* pDesc);

		void UpdateVolume(float64 masterVolume, const AudioListenerDesc* pAudioListeners, uint32 audioListenerCount, ESpeakerSetup speakerSetup);
		void AddToBuffer(double** ppOutputChannels, uint32 channelCount, uint32 outputSampleCount);

	private:
		const AudioDeviceLambda* m_pAudioDevice		= nullptr;
		const SoundEffect3DLambda* m_pSoundEffect	= nullptr;
		
		bool		m_Completed						= false;

		uint32		m_SampleCount					= 0;
		uint32		m_ChannelCount					= 0;
		uint32		m_CurrentWaveFormIndex			= 0;

		glm::vec3	m_Position						= glm::vec3(0.0f);
		//Velocity
		//Pitch			
		float64		m_Volume						= 1.0;

		float64*	m_pOutputVolumes				= nullptr;
	};
}
