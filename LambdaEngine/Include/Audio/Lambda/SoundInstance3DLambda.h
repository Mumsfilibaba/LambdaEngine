#pragma once

#include "LambdaAudio.h"
#include "Audio/API/ISoundInstance3D.h"

namespace LambdaEngine
{
	struct AudioListenerDesc;

	class IAudioDevice;
	class AudioDeviceLambda;
	class ISoundEffect3D;
	class SoundEffect3DLambda;
	
	class SoundInstance3DLambda : public ISoundInstance3D
	{
	public:
		SoundInstance3DLambda(const IAudioDevice* pAudioDevice);
		~SoundInstance3DLambda();

		virtual bool Init(const SoundInstance3DDesc* pDesc) override final;

		virtual void Play()		override final;
		virtual void Pause()	override final;
		virtual void Stop()		override final;
		virtual void Toggle()	override final;

		virtual void SetPosition(const glm::vec3& position) override final;
		virtual void SetVolume(float32 volume)				override final;
		virtual void SetPitch(float32 pitch)					override final;

		FORCEINLINE virtual const glm::vec3&	GetPosition()	const override final { return m_Position;	}
		FORCEINLINE virtual float32				GetVolume()		const override final { return m_Volume;		}
		FORCEINLINE virtual float32				GetPitch()		const override final { return 1.0f;			}

		void UpdateVolume(float masterVolume, const AudioListenerDesc* pAudioListeners, uint32 audioListenerCount, ESpeakerSetup speakerSetup);
		void AddToBuffer(double** ppOutputChannels, uint32 channelCount, uint32 outputSampleCount);

	private:
		const AudioDeviceLambda* m_pAudioDevice		= nullptr;
		const SoundEffect3DLambda* m_pSoundEffect	= nullptr;

		uint32		m_SampleCount					= 0;
		uint32		m_ChannelCount					= 0;
		uint32		m_CurrentWaveFormIndex			= 0;

		bool		m_Looping						= false;
		bool		m_Playing						= false;

		glm::vec3	m_Position						= glm::vec3(0.0f);
		//Pitch			
		float32		m_Volume						= 1.0f;

		float64*	m_pOutputVolumes				= nullptr;
	};
}
