#include "Audio/FMOD/SoundEffect3DFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundEffect3DFMOD::SoundEffect3DFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice)),
		m_pHandle(nullptr)
	{
	}

	SoundEffect3DFMOD::~SoundEffect3DFMOD()
	{
		if (m_pHandle != nullptr)
		{
			FMOD_Sound_Release(m_pHandle);
			m_pHandle = nullptr;
		}
	}

	bool SoundEffect3DFMOD::Init(const SoundEffect3DDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pFilepath;

		FMOD_MODE mode = FMOD_3D | FMOD_CREATESAMPLE | FMOD_LOOP_OFF;

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

		if (FMOD_System_CreateSound(m_pAudioDevice->pSystem, pDesc->pFilepath, mode, &soundCreateInfo, &m_pHandle) != FMOD_OK)
		{
			LOG_ERROR("[Sound]: Sound \"%s\" could not be initialized", m_pName);
			return false;
		}

		uint32 durationMS = 0;
		FMOD_Sound_GetLength(m_pHandle, &durationMS, FMOD_TIMEUNIT_MS);

		m_Duration = (float64)durationMS / 1000.0;

		return true;
	}

	void SoundEffect3DFMOD::PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float64 volume, float pitch)
	{
		FMOD_CHANNEL* pChannel = nullptr;
		FMOD_VECTOR fmodPosition = { position.x, position.y, position.z };
		FMOD_VECTOR fmodVelocity = { velocity.x, velocity.y, velocity.z };

		FMOD_System_PlaySound(m_pAudioDevice->pSystem, m_pHandle, nullptr, true, &pChannel);

		FMOD_Channel_Set3DAttributes(pChannel, &fmodPosition, &fmodVelocity);
		FMOD_Channel_SetVolume(pChannel, volume);
		FMOD_Channel_SetPitch(pChannel, pitch);

		FMOD_Channel_SetPaused(pChannel, 0);
	}

	FMOD_SOUND* SoundEffect3DFMOD::GetHandle() 
	{ 
		return m_pHandle; 
	}
}
