#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

#include <WavLib.h>

namespace LambdaEngine
{
	SoundEffect3DLambda::SoundEffect3DLambda(AudioDeviceLambda* pDevice) 
		: m_pDevice(pDevice),
		m_Desc()
	{
	}

	SoundEffect3DLambda::~SoundEffect3DLambda()
	{
		m_pDevice->RemoveSoundEffect3D(this);
		WavLibFree(m_pWaveForm);
	}

	bool SoundEffect3DLambda::Init(const SoundEffect3DDesc* pDesc)
	{
        VALIDATE(pDesc);

		WaveFile header = { };
		int32	 result = WavLibLoadFileFloat32(pDesc->pFilepath, &m_pWaveForm, &header, WAV_LIB_FLAG_MONO);
		if (result != WAVE_SUCCESS)
		{
			const char* pError = WavLibGetError(result);
			LOG_ERROR("[SoundEffect3DLambda]: Failed to load file '%s'. Error: %s", pDesc->pFilepath, pError);

			return false;
		}
		else
		{
			m_Desc.SampleCount	= header.SampleCount;
			m_Desc.ChannelCount = header.ChannelCount;
			m_Desc.SampleRate	= header.SampleRate;

			D_LOG_MESSAGE("[SoundEffect3DLambda]: Loaded file '%s'. BitsPerSample=%u, SampleCount=%u, ChannelCount=%u, SampleRate=%u", pDesc->pFilepath, header.OriginalBitsPerSample, m_Desc.SampleCount, m_Desc.ChannelCount, m_Desc.SampleRate);
			return true;
		}
	}
	
	SoundDesc SoundEffect3DLambda::GetDesc() const
	{
		return m_Desc;
	}
	
	void SoundEffect3DLambda::PlayOnce(const SoundInstance3DDesc* pDesc)
	{
		m_pDevice->PlayOnce(pDesc);
	}
}
