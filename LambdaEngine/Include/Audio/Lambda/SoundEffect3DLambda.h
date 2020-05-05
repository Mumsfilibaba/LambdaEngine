#pragma once
#include "Audio/API/ISoundEffect3D.h"

namespace LambdaEngine
{
	class AudioDeviceLambda;

	class SoundEffect3DLambda : public ISoundEffect3D
	{
	public:
		SoundEffect3DLambda(AudioDeviceLambda* pDevice);
		~SoundEffect3DLambda();

		bool Init(const SoundEffect3DDesc* pDesc);
		
		FORCEINLINE const float32* GetWaveform() const
		{ 
			return m_pWaveForm; 
		}

		// ISoundEffect3D interface
		virtual SoundDesc GetDesc() const override final;
		
	private:		
		AudioDeviceLambda*	m_pAudioDevice	= nullptr;
		float32*			m_pWaveForm		= nullptr;

		SoundDesc m_Desc;
	};
}