#include "Audio/Lambda/CombFilterLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	CombFilterLambda::CombFilterLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	CombFilterLambda::~CombFilterLambda()
	{
		SAFEDELETE_ARRAY(m_pPreviousSamples);
	}

	bool CombFilterLambda::Init(const CombFilterDesc* pDesc)
	{
		m_pName = pDesc->pName;

		m_CurrentSampleIndex	= 0;
		m_Delay					= pDesc->Delay;
		m_pPreviousSamples		= new float64[m_Delay];
		memset(m_pPreviousSamples, 0, m_Delay * sizeof(float64));
		m_Multiplier			= pDesc->Multiplier;

		D_LOG_MESSAGE("[CombFilterLambda]: Created Comb Filter \"%s\"", m_pName);

		return true;
	}

	IAudioFilter* CombFilterLambda::CreateCopy() const
	{
		CombFilterDesc desc = {};
		desc.pName			= m_pName;
		desc.Delay			= m_Delay;
		desc.Multiplier		= m_Multiplier;

		return m_pAudioDevice->CreateCombFilter(&desc);
	}

	float64 CombFilterLambda::ProcessSample(float64 sample)
	{
		float64 outputSample = 0.0;

		if (m_Enabled)
		{
			outputSample = m_Multiplier * m_pPreviousSamples[m_CurrentSampleIndex];
		}
		else
		{
			outputSample = sample;
		}

		m_pPreviousSamples[m_CurrentSampleIndex] = sample;

		if (++m_CurrentSampleIndex == m_Delay) m_CurrentSampleIndex = 0;

		outputSample = glm::clamp<double>(outputSample, -1.0, 1.0);
		return outputSample;
	}

	void CombFilterLambda::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}
}