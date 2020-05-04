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

		SAFEDELETE_ARRAY(m_pSrcWaveForm);
		SAFEDELETE_ARRAY(m_pResampledWaveForm);
	}

	bool SoundEffect3DLambda::Init(const SoundEffect3DDesc* pDesc)
	{
		VALIDATE(pDesc);

		int32 result = WavLibLoadFileFloat64(pDesc->pFilepath, &m_pSrcWaveForm, &m_Header, WAV_LIB_FLAG_MONO);
		if (result != WAVE_SUCCESS)
		{
			const char* pError = WavLibGetError(result);

			LOG_ERROR("[SoundEffect3DLambda]: Failed to load file '%s'. Error: %s", pDesc->pFilepath, pError);
			return false;
		}

		D_LOG_MESSAGE("[SoundEffect3DLambda]: Loaded file '%s'", pDesc->pFilepath);
		return true;
	}

	void SoundEffect3DLambda::PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float64 volume, float pitch)
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

	void SoundEffect3DLambda::Resample()
	{
		r8b::CDSPResampler* pResampler = m_pAudioDevice->GetResampler(m_Header.SampleRate);

		m_ResampledSampleCount = (uint32)glm::ceil(m_Header.SampleCount * (float)m_pAudioDevice->GetSampleRate() / (float)m_Header.SampleRate);
		m_pResampledWaveForm = DBG_NEW float64[m_ResampledSampleCount];

		pResampler->oneshot(m_pSrcWaveForm, m_Header.SampleCount, m_pResampledWaveForm, m_ResampledSampleCount);
	}
}
