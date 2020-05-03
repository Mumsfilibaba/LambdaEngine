#pragma once

#include "LambdaEngine.h"
#include "PortAudio.h"

#include "Audio/API/AudioTypes.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	struct AudioListenerDesc;

	class IAudioDevice;
	class AudioDeviceLambda;
	class ISoundEffect3D;

	struct ManagedSoundInstance3DDesc
	{
		const char*			pName				= "ManagedSoundInstance3D";
		ISoundEffect3D*		pSoundEffect		= nullptr;
		uint32				Flags				= FSoundModeFlags::SOUND_MODE_NONE;
		glm::vec3			Position			= glm::vec3(0.0f);
		glm::vec3			Velocity			= glm::vec3(0.0f);
		float32				Volume				= 1.0f;
		float32				Pitch				= 1.0f;
	};

	class ManagedSoundInstance3DLambda
	{
	public:
		ManagedSoundInstance3DLambda(const IAudioDevice* pAudioDevice);
		~ManagedSoundInstance3DLambda();

		bool Init(const ManagedSoundInstance3DDesc* pDesc);

		void UpdateVolume(float masterVolume, const AudioListenerDesc* pAudioListeners, uint32 count);

	private:
		int32 LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer);

	private:
		static int32 PortAudioCallback(
			const void* pInputBuffer,
			void* pOutputBuffer,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* pTimeInfo,
			PaStreamCallbackFlags statusFlags,
			void* pUserData);

	private:
		const AudioDeviceLambda* m_pAudioDevice		= nullptr;

		PaStream*	m_pStream						= nullptr;
		
		float32*	m_pWaveForm						= nullptr;
		uint32		m_SampleCount					= 0;
		uint32		m_CurrentBufferIndex			= 0;
		uint32		m_ChannelCount					= 0;
		uint32		m_TotalSampleCount				= 0;

		glm::vec3	m_Position						= glm::vec3(0.0f);
		//Velocity
		//Pitch			
		float		m_Volume						= 1.0f;

		float		m_OutputVolume					= 1.0f;
	};
}
