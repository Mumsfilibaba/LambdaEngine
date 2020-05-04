#pragma once
#include "Audio/API/ISoundEffect3D.h"

#include <WavLib.h>

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceLambda;

	class SoundEffect3DLambda : public ISoundEffect3D
	{
	public:
		SoundEffect3DLambda(const IAudioDevice* pAudioDevice);
		~SoundEffect3DLambda();

		virtual bool Init(const SoundEffect3DDesc* pDesc) override final;
		virtual void PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float64 volume, float pitch) override final;

		FORCEINLINE virtual float64 GetDuration() override final { return (float64)m_Header.Duration; }

		void Resample();

		FORCEINLINE const float64* GetWaveform()		const { return m_pResampledWaveForm; }

		FORCEINLINE uint32 GetResampledSampleCount()	const { return m_ResampledSampleCount; }

		FORCEINLINE uint32 GetSrcSampleCount()			const { return m_Header.SampleCount; }
		FORCEINLINE uint32 GetSrcSampleRate()			const { return m_Header.SampleRate; }
		FORCEINLINE uint32 GetSrcChannelCount()			const { return m_Header.ChannelCount; }
		
	private:		
		const AudioDeviceLambda* m_pAudioDevice;

		float64*	m_pSrcWaveForm				= nullptr;
		float64*	m_pResampledWaveForm		= nullptr;
		uint32		m_ResampledSampleCount		= 0;
		WaveFile	m_Header;
	};
}