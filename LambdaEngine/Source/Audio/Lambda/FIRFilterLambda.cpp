#include "Audio/Lambda/FIRFilterLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	FIRFilterLambda::FIRFilterLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	FIRFilterLambda::~FIRFilterLambda()
	{
		SAFEDELETE_ARRAY(m_pSampleBuffer);
		SAFEDELETE_ARRAY(m_pWeights);
	}

	bool FIRFilterLambda::Init(const FIRFilterDesc* pDesc)
	{
		m_pName = pDesc->pName;

		m_CurrentSampleIndex	= 0;
		m_WeightsCount			= pDesc->WeightsCount;
		m_pSampleBuffer			= new float64[m_WeightsCount];
		m_pWeights				= new float64[m_WeightsCount];
		memset(m_pSampleBuffer,		0,					m_WeightsCount * sizeof(float64));
		memcpy(m_pWeights,			pDesc->pWeights,	m_WeightsCount * sizeof(float64));

		D_LOG_MESSAGE("[FIRFilterLambda]: Created FIR Filter \"%s\"", m_pName);

		return true;
	}

	IAudioFilter* FIRFilterLambda::CreateCopy() const
	{
		FIRFilterDesc desc = {};
		desc.pName			= m_pName;
		desc.pWeights		= m_pWeights;
		desc.WeightsCount	= m_WeightsCount;

		return m_pAudioDevice->CreateFIRFilter(&desc);
	}

	float64 FIRFilterLambda::ProcessSample(float64 sample)
	{
		m_pSampleBuffer[m_CurrentSampleIndex] = sample;

		float64 outputSample = 0.0;

		if (m_Enabled)
		{
			int32 currentSampleIndex = m_CurrentSampleIndex;

			for (uint32 w = 0; w < m_WeightsCount; w++)
			{
				float64 currentSample = m_pSampleBuffer[currentSampleIndex--];
				float64 currentWeight = m_pWeights[w];

				outputSample += currentSample * currentWeight;

				if (currentSampleIndex == -1)
					currentSampleIndex = m_WeightsCount - 1;
			}
		}
		else
		{
			outputSample = sample;
		}

		if (++m_CurrentSampleIndex == m_WeightsCount) m_CurrentSampleIndex = 0;

		outputSample = glm::clamp<double>(outputSample, -1.0, 1.0);
		return outputSample;
	}

	void FIRFilterLambda::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}
}