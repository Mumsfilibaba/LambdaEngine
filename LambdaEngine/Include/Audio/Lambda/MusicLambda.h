#pragma once

#include "Audio/API/IMusic.h"

#include <WavLib.h>

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

		void Resample();
		void AddToBuffer(double** ppOutputChannels, uint32 channelCount, uint32 sampleCount);

		FORCEINLINE uint32 GetResampledSampleCount()	const { return m_ResampledSampleCount; }

		FORCEINLINE uint32 GetSrcSampleCount()			const { return m_Header.SampleCount; }
		FORCEINLINE uint32 GetSrcSampleRate()			const { return m_Header.SampleRate; }
		FORCEINLINE uint32 GetSrcChannelCount()			const { return m_Header.ChannelCount; }

	private:
		const AudioDeviceLambda* m_pAudioDevice = nullptr;

		float64*	m_pSrcWaveForm				= nullptr;
		float64**	m_pResampledWaveForms		= nullptr;
		uint32		m_CurrentWaveFormIndex		= 0;
		uint32		m_ResampledSampleCount		= 0;
		WaveFile	m_Header;

		bool		m_Playing					= false;

		float		m_Volume					= 1.0f;
		//Pitch

		float		m_OutputVolume				= 1.0f;
	};
}
