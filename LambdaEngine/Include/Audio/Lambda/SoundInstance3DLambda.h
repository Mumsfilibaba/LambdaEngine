#pragma once

#include "PortAudio.h"
#include "Audio/API/ISoundInstance3D.h"

namespace LambdaEngine
{
	struct AudioListenerDesc;

	class IAudioDevice;
	class AudioDeviceLambda;
	
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

		void UpdateVolume(float masterVolume, const AudioListenerDesc* pAudioListeners, uint32 count, ESpeakerSetup speakerSetup);

	private:
		int32 LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer);
		
	private:
		/*
		* This routine will be called by the PortAudio engine when audio is needed.
		* It may called at interrupt level on some machines so don't do anything
		*  that could mess up the system like calling malloc() or free().
		*/ 
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
		uint32		m_ChannelCount					= 0;
		uint32		m_CurrentBufferIndex			= 0;

		bool		m_Looping						= false;
		bool		m_Playing						= false;

		glm::vec3	m_Position						= glm::vec3(0.0f);
		//Pitch			
		float32		m_Volume						= 1.0f;

		float32*	m_pOutputVolumes				= nullptr;
	};
}
