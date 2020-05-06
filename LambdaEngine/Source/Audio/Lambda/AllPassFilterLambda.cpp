#include "Audio/Lambda/AllPassFilterLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	AllPassFilterLambda::AllPassFilterLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	AllPassFilterLambda::~AllPassFilterLambda()
	{
		SAFEDELETE_ARRAY(m_pPreviousSamples);
	}

	bool AllPassFilterLambda::Init(const AllPassFilterDesc* pDesc)
	{
		m_pName = pDesc->pName;

		m_CurrentSampleIndex	= 0;
		m_Delay					= pDesc->Delay;
		m_pPreviousSamples		= new float64[m_Delay];
		memset(m_pPreviousSamples, 0, m_Delay * sizeof(float64));
		m_Multiplier			= pDesc->Multiplier;

		D_LOG_MESSAGE("[AllPassFilterLambda]: Created All Pass Filter \"%s\"", m_pName);

		return true;
	}

	IAudioFilter* AllPassFilterLambda::CreateCopy() const
	{
		AllPassFilterDesc desc = {};
		desc.pName			= m_pName;
		desc.Delay			= m_Delay;
		desc.Multiplier		= m_Multiplier;

		return m_pAudioDevice->CreateAllPassFilter(&desc);
	}

	float64 AllPassFilterLambda::ProcessSample(float64 sample)
	{
		float64 outputSample = 0.0;

		if (m_Enabled)
		{
			outputSample = -m_Multiplier * sample + m_pPreviousSamples[m_CurrentSampleIndex];
		}
		else
		{
			outputSample = sample;
		}

		m_pPreviousSamples[m_CurrentSampleIndex] = sample + m_Multiplier * outputSample;

		if (++m_CurrentSampleIndex == m_Delay) m_CurrentSampleIndex = 0;

		outputSample = glm::clamp<double>(outputSample, -1.0, 1.0);
		return outputSample;
	}

	void AllPassFilterLambda::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}
}