#pragma once
#include "Audio/API/IMusicInstance.h"

namespace LambdaEngine
{
	class AudioDeviceLambda;

	class MusicInstanceLambda : public IMusicInstance
	{
	public:
		MusicInstanceLambda(AudioDeviceLambda* pDevice);
		~MusicInstanceLambda();

		bool Init(const MusicInstanceDesc* pDesc);

		// IMusicInstance interface
		virtual void Play()		override final;
		virtual void Pause()	override final;
		virtual void Toggle()	override final;
		virtual void Stop()		override final;

		virtual void SetVolume(float32 volume)	override final;
		virtual void SetPitch(float32 pitch)	override final;

		virtual float GetVolume() const override final;
		virtual float GetPitch()  const override final;

	public:
		float32*	pWaveForm = nullptr;
		bool		IsPlaying = false;

		uint32 TotalSampleCount		= 0;
		uint32 CurrentBufferIndex	= 0;

		float32	Volume	= 1.0f;
		float32	Pitch	= 1.0f;
		
		SoundDesc Desc;

	private:
		AudioDeviceLambda*	m_pDevice	= nullptr;
		class MusicLambda*	m_pMusic	= nullptr;
	};
}