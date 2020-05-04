#pragma once

#include "PortAudio.h"
#include "Audio/API/IMusic.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceLambda;

	class MusicLambda : public IMusic
	{
	public:
		MusicLambda(const IAudioDevice* pAudioDevice);
		~MusicLambda();

		virtual bool Init(const MusicDesc* pDesc) override final;

		virtual void Play()		override final;
		virtual void Pause()	override final;
		virtual void Toggle()	override final;

		virtual void SetVolume(float volume)				override final;
		virtual void SetPitch(float pitch)					override final;

		FORCEINLINE virtual float	GetVolume()		const override final	{ return m_Volume;	}
		FORCEINLINE virtual float	GetPitch()		const override final	{ return 1.0f;		}	

		void UpdateVolume(float masterVolume);

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
		const AudioDeviceLambda* m_pAudioDevice = nullptr;

		PaStream*	m_pStream					= nullptr;

		float32*	m_pWaveForm					= nullptr;
		uint32		m_SampleCount				= 0;
		uint32		m_CurrentBufferIndex		= 0;
		uint32		m_ChannelCount				= 0;
		uint32		m_TotalSampleCount			= 0;

		bool		m_Playing					= false;

		float		m_Volume					= 1.0f;
		//Pitch

		float		m_OutputVolume				= 1.0f;
	};
}
