#include "Audio/Lambda/MusicLambda.h"

#include <WavLib.h>

namespace LambdaEngine
{
	MusicLambda::MusicLambda(AudioDeviceLambda* pDevice)
		: m_pDevice(pDevice)
	{
	}

	MusicLambda::~MusicLambda()
	{
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

	void MusicLambda::Play()
	{
	}

	void MusicLambda::Pause()
	{
	}

	void MusicLambda::Toggle()
	{
	}

	void MusicLambda::SetVolume(float volume)
	{
	}

	void MusicLambda::SetPitch(float pitch)
	{
	}

	float MusicLambda::GetVolume() const
	{
		return 0.0f;
	}

	float MusicLambda::GetPitch() const
	{
		return 0.0f;
	}

	SoundDesc MusicLambda::GetDesc() const
	{
		return m_Desc;
	}
}