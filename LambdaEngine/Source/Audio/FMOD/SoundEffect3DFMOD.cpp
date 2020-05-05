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

		FMOD_Sound_GetLength(m_pHandle, &m_LengthMS, FMOD_TIMEUNIT_MS);
		return true;
	}

	FMOD_SOUND* SoundEffect3DFMOD::GetHandle() 
	{ 
		return m_pHandle; 
	}

	uint32 SoundEffect3DFMOD::GetLengthMS() 
	{ 
		return m_LengthMS; 
	}
	
	SoundDesc SoundEffect3DFMOD::GetDesc() const
	{
		return SoundDesc();
	}
}
