#pragma once

#include "FMOD.h"
#include "Audio/API/ISoundEffect3D.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class SoundEffect3DFMOD : public ISoundEffect3D
	{
	public:
		SoundEffect3DFMOD(const IAudioDevice* pAudioDevice);
		~SoundEffect3DFMOD();

		virtual bool Init(const SoundEffect3DDesc* pDesc) override final;
		virtual void PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float64 volume, float pitch) override final;
		
		FORCEINLINE virtual float64 GetDuration() override final { return m_Duration; }
		FMOD_SOUND* GetHandle();

	private:
		//Engine
		const AudioDeviceFMOD*	m_pAudioDevice;

		//FMOD
		FMOD_SOUND*			m_pHandle;

		//Local
		const char*			m_pName;
		float64				m_Duration;
	};
}
