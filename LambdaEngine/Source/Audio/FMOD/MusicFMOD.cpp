#include "Audio/FMOD/MusicFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	MusicFMOD::MusicFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice))
	{
	}

	MusicFMOD::~MusicFMOD()
	{
		FMOD_Channel_Stop(m_pChannel);
		FMOD_Sound_Release(m_pHandle);
		m_pHandle	= nullptr;
		m_pChannel	= nullptr;
	}

	bool MusicFMOD::Init(const MusicDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pFilepath;

		FMOD_MODE mode = FMOD_2D | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM;

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

		if (FMOD_System_CreateSound(m_pAudioDevice->pSystem, pDesc->pFilepath, mode, &soundCreateInfo, &m_pHandle) != FMOD_OK)
		{
			LOG_WARNING("[MusicFMOD]: Music \"%s\" could not be initialized in %s", pDesc->pFilepath, m_pName);
			return false;
		}

		if (FMOD_System_PlaySound(m_pAudioDevice->pSystem, m_pHandle, nullptr, false, &m_pChannel) != FMOD_OK)
		{
			LOG_WARNING("[MusicFMOD]: Music \"%s\" could not be played in %s", pDesc->pFilepath, m_pName);
			return false;
		}

		return true;
	}

	SoundDesc MusicFMOD::GetDesc() const
	{
		return SoundDesc();
	}
}