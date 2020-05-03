#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/ManagedSoundInstance3DLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundEffect3DLambda::SoundEffect3DLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
		UNREFERENCED_VARIABLE(pAudioDevice);
	}

	SoundEffect3DLambda::~SoundEffect3DLambda()
	{
		m_pAudioDevice->DeleteSoundEffect(this);

		SAFEDELETE_ARRAY(m_pWaveForm);
	}

	bool SoundEffect3DLambda::Init(const SoundEffect3DDesc* pDesc)
	{
		VALIDATE(pDesc);

		if (LoadWavFileFloat(pDesc->pFilepath, &m_pWaveForm, &m_Header) != WAVE_SUCCESS)
		{
			LOG_ERROR("[SoundEffect3DLambda]: Could not load sound effect \"%s\"", pDesc->pFilepath);
			return false;
		}

		return true;
	}

	void SoundEffect3DLambda::PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float volume, float pitch)
	{
		UNREFERENCED_VARIABLE(velocity);

		ManagedSoundInstance3DDesc managedSoundInstanceDesc = {};
		managedSoundInstanceDesc.pName				= "Managed Sound Instance";
		managedSoundInstanceDesc.pSoundEffect		= this;
		managedSoundInstanceDesc.Flags				= FSoundModeFlags::SOUND_MODE_NONE;
		managedSoundInstanceDesc.Position			= position;
		managedSoundInstanceDesc.Velocity			= velocity;
		managedSoundInstanceDesc.Volume				= volume;
		managedSoundInstanceDesc.Pitch				= pitch;

		m_pAudioDevice->AddManagedSoundInstance(&managedSoundInstanceDesc);
	}
}
