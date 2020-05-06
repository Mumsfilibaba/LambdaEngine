#include "Audio/Lambda/MusicLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"
#include "Audio/Lambda/LambdaAudio.h"

#include "Log/Log.h"

#include <WavLib.h>

namespace LambdaEngine
{
	MusicLambda::MusicLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	MusicLambda::~MusicLambda()
	{
		m_pAudioDevice->DeleteMusicInstance(this);

		SAFEDELETE_ARRAY(m_pSrcWaveForm);

		for (uint32 c = 0; c < m_Header.ChannelCount; c++)
		{
			SAFEDELETE_ARRAY(m_pResampledWaveForms[c]);
		}

		SAFEDELETE_ARRAY(m_pResampledWaveForms);
	}

	bool MusicLambda::Init(const MusicDesc* pDesc)
	{
		VALIDATE(pDesc);

		uint32 waveFlags = WAV_LIB_FLAG_MONO;

		int32 result = WavLibLoadFileFloat64(pDesc->pFilepath, &m_pSrcWaveForm, &m_Header, waveFlags);
		if (result != WAVE_SUCCESS)
		{
			const char* pError = WavLibGetError(result);

			LOG_ERROR("[MusicLambda]: Failed to load file '%s'. Error: %s", pDesc->pFilepath, pError);
			return false;
		}

		m_Volume = glm::clamp(pDesc->Volume, -1.0, 1.0);
		m_CurrentWaveFormIndex	= 0;
		m_Playing = true;

		return true;
	}

	void MusicLambda::Play()
	{
		m_Playing = true;
	}

	void MusicLambda::Pause()
	{
		m_Playing = false;
	}

	void MusicLambda::Toggle()
	{
		if (m_Playing)
		{
			Pause();
		}
		else
		{
			Play();
		}
	}

	void MusicLambda::SetVolume(float64 volume)
	{
		m_Volume = glm::clamp(volume, -1.0, 1.0);
	}

	void MusicLambda::SetPitch(float32 pitch)
	{
		UNREFERENCED_VARIABLE(pitch);
	}

	void MusicLambda::Resample()
	{
		r8b::CDSPResampler* pResampler = m_pAudioDevice->GetResampler(m_Header.SampleRate);

		float64* pChannelWaveForm = DBG_NEW float64[m_Header.SampleCount];

		m_pResampledWaveForms = DBG_NEW float64* [m_Header.ChannelCount];

		m_ResampledSampleCount = (uint32)glm::ceil(m_Header.SampleCount * (float)m_pAudioDevice->GetSampleRate() / (float)m_Header.SampleRate);

		for (uint32 c = 0; c < m_Header.ChannelCount; c++)
		{
			for (uint32 s = 0; s < m_Header.SampleCount; s++)
			{
				pChannelWaveForm[s] = m_pSrcWaveForm[c + s * m_Header.ChannelCount];
			}

			float64* pResampledWaveForm = DBG_NEW float64[m_ResampledSampleCount];
			pResampler->oneshot(pChannelWaveForm, m_Header.SampleCount, pResampledWaveForm, m_ResampledSampleCount);
			m_pResampledWaveForms[c] = pResampledWaveForm;
		}

		SAFEDELETE_ARRAY(pChannelWaveForm);
	}

	void MusicLambda::ProcessBuffer(double** ppOutputChannels, uint32 channelCount, uint32 outputSampleCount)
	{
		if (m_Playing)
		{
			//Todo: What if not mono
			float64* pResampledWaveForm = m_pResampledWaveForms[0];

			for (uint32 s = 0; s < outputSampleCount; s++)
			{
				for (uint32 c = 0; c < channelCount; c++)
				{
					double* pOutputChannel = ppOutputChannels[c];
					pOutputChannel[s] += m_Volume * pResampledWaveForm[m_CurrentWaveFormIndex];
				}

				m_CurrentWaveFormIndex++;
				if (m_CurrentWaveFormIndex == m_ResampledSampleCount)
					m_CurrentWaveFormIndex = 0;
			}
		}
	}
}