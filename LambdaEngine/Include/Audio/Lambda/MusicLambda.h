#pragma once
#include "Audio/API/IMusic.h"

#include "PortAudio.h"

namespace LambdaEngine
{
	class AudioDeviceLambda;

	class MusicLambda : public IMusic
	{
	public:
		MusicLambda(AudioDeviceLambda* pDevice);
		~MusicLambda();

		bool Init(const MusicDesc* pDesc);

		// IMusic Interface
		virtual void Play()		override final;
		virtual void Pause()	override final;
		virtual void Toggle()	override final;

		virtual void SetVolume(float volume)	override final;
		virtual void SetPitch(float pitch)		override final;

		virtual float		GetVolume() const override final;
		virtual float		GetPitch()  const override final;
		virtual SoundDesc	GetDesc()	const override final;

	private:
		AudioDeviceLambda*	m_pDevice	= nullptr;
		float*				m_pWaveForm = nullptr;

		SoundDesc m_Desc;
	};
}