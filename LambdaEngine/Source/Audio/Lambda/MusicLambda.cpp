#include "Audio/Lambda/MusicLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include <WavLib.h>

namespace LambdaEngine
{
	MusicLambda::MusicLambda(AudioDeviceLambda* pDevice)
		: m_pDevice(pDevice),
		m_Desc()
	{
	}

	MusicLambda::~MusicLambda()
	{
		m_pDevice->RemoveMusic(this);
		WavLibFree(m_pWaveForm);
	}

	bool MusicLambda::Init(const MusicDesc* pDesc)
	{
		WaveFile header = { };
		int32	 result = WavLibLoadFileFloat32(pDesc->pFilepath, &m_pWaveForm, &header, WAV_LIB_FLAG_MONO);
		if (result != WAVE_SUCCESS)
		{
			const char* pError = WavLibGetError(result);
			LOG_ERROR("[MusicLambda]: Failed to load file '%s'. Error: %s", pDesc->pFilepath, pError);

			return false;
		}
		else
		{
			m_Desc.SampleCount	= header.SampleCount;
			m_Desc.ChannelCount = header.ChannelCount;
			m_Desc.SampleRate	= header.SampleRate;

			D_LOG_MESSAGE("[MusicLambda]: Loaded file '%s'. SampleCount=%u, ChannelCount=%u, SampleRate=%u", pDesc->pFilepath, m_Desc.SampleCount, m_Desc.ChannelCount, m_Desc.SampleRate);
			return true;
		}
	}

	SoundDesc MusicLambda::GetDesc() const
	{
		return m_Desc;
	}
}