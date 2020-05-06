#pragma once
#include "Audio/API/ISoundEffect3D.h"

#include "FMOD.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class SoundEffect3DFMOD : public ISoundEffect3D
	{
	public:
		SoundEffect3DFMOD(const IAudioDevice* pAudioDevice);
		~SoundEffect3DFMOD();

		bool Init(const SoundEffect3DDesc* pDesc);
				
		FMOD_SOUND* GetHandle();
		uint32 GetLengthMS();

		// ISoundEffect3D interface
		virtual SoundDesc GetDesc() const override final;

		virtual void PlayOnce(const SoundInstance3DDesc* pDesc) override final;

	private:
		//Engine
		const AudioDeviceFMOD*	m_pAudioDevice;

		//FMOD
		FMOD_SOUND*			m_pHandle;

		//Local
		const char*			m_pName;
		uint32				m_LengthMS;

		SoundDesc m_Desc;
	};
}
