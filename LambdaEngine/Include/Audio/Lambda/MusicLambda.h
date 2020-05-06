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

		FORCEINLINE float32* GetWaveForm() const
		{
			return m_pWaveForm;
		}

		// IMusic Interface
		virtual SoundDesc GetDesc() const override final;

	private:
		AudioDeviceLambda*	m_pDevice	= nullptr;
		float32*			m_pWaveForm = nullptr;

		SoundDesc m_Desc;
	};
}