#include "Audio/Lambda/IIRFilterLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	IIRFilterLambda::IIRFilterLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	IIRFilterLambda::~IIRFilterLambda()
	{
		SAFEDELETE_ARRAY(m_pFeedForwardSampleBuffer);
		SAFEDELETE_ARRAY(m_pFeedBackSampleBuffer);
		SAFEDELETE_ARRAY(m_pFeedForwardWeights);
		SAFEDELETE_ARRAY(m_pFeedBackWeights);
	}

	bool IIRFilterLambda::Init(const IIRFilterDesc* pDesc)
	{
		m_pName = pDesc->pName;

		m_CurrentFeedForwardSampleIndex		= 0;
		m_FeedForwardWeightsCount			= pDesc->FeedForwardWeightsCount;
		m_pFeedForwardSampleBuffer			= new float64[m_FeedForwardWeightsCount];
		m_pFeedForwardWeights				= new float64[m_FeedForwardWeightsCount];
		memset(m_pFeedForwardSampleBuffer,	0,							m_FeedForwardWeightsCount * sizeof(float64));
		memcpy(m_pFeedForwardWeights,		pDesc->pFeedForwardWeights,	m_FeedForwardWeightsCount * sizeof(float64));

		m_CurrentFeedBackSampleIndex	= 0;
		m_FeedBackWeightsCount			= pDesc->FeedBackWeightsCount;
		m_pFeedBackSampleBuffer			= new float64[m_FeedBackWeightsCount];
		m_pFeedBackWeights				= new float64[m_FeedBackWeightsCount];
		memset(m_pFeedBackSampleBuffer,	0,							m_FeedBackWeightsCount * sizeof(float64));
		memcpy(m_pFeedBackWeights,		pDesc->pFeedBackWeights,	m_FeedBackWeightsCount * sizeof(float64));

		D_LOG_MESSAGE("[IIRFilterLambda]: Created IIR Filter \"%s\"", m_pName);

		return true;
	}

	IAudioFilter* IIRFilterLambda::CreateCopy() const
	{
		IIRFilterDesc desc = {};
		desc.pName						= m_pName;
		desc.pFeedForwardWeights		= m_pFeedForwardWeights;
		desc.FeedForwardWeightsCount	= m_FeedForwardWeightsCount;
		desc.pFeedBackWeights			= m_pFeedBackWeights;
		desc.FeedBackWeightsCount		= m_FeedBackWeightsCount;

		return m_pAudioDevice->CreateIIRFilter(&desc);
	}

	float64 IIRFilterLambda::ProcessSample(float64 sample)
	{
		m_pFeedForwardSampleBuffer[m_CurrentFeedForwardSampleIndex] = sample;

		float64 outputSample = 0.0;

		if (m_Enabled)
		{
			//Feed-Forward Samples
			{
				int32 currentFeedForwardSampleIndex = m_CurrentFeedForwardSampleIndex;

				for (uint32 w = 0; w < m_FeedForwardWeightsCount; w++)
				{
					float64 currentSample = m_pFeedForwardSampleBuffer[currentFeedForwardSampleIndex--];
					float64 currentWeight = m_pFeedForwardWeights[w];

					outputSample += currentSample * currentWeight;

					if (currentFeedForwardSampleIndex == -1)
						currentFeedForwardSampleIndex = m_FeedForwardWeightsCount - 1;
				}
			}

			//Feedback Samples
			{
				int32 currentFeedBackSampleIndex = m_CurrentFeedBackSampleIndex - 1;

				if (currentFeedBackSampleIndex == -1)
					currentFeedBackSampleIndex = m_FeedBackWeightsCount - 1;

				for (uint32 w = 0; w < m_FeedBackWeightsCount; w++)
				{
					float64 currentSample = m_pFeedBackSampleBuffer[currentFeedBackSampleIndex--];
					float64 currentWeight = m_pFeedBackWeights[w];

					outputSample -= currentSample * currentWeight;

					if (currentFeedBackSampleIndex == -1)
						currentFeedBackSampleIndex = m_FeedBackWeightsCount - 1;
				}
			}
		}
		else
		{
			outputSample = sample;
		}

		//LOG_WARNING("Sample: %f", outputSample);

		outputSample = glm::clamp<double>(outputSample, -1.0, 1.0);
		m_pFeedBackSampleBuffer[m_CurrentFeedBackSampleIndex] = outputSample;

		if (++m_CurrentFeedForwardSampleIndex == m_FeedForwardWeightsCount) m_CurrentFeedForwardSampleIndex = 0;
		if (++m_CurrentFeedBackSampleIndex == m_FeedBackWeightsCount) m_CurrentFeedBackSampleIndex = 0;

		return outputSample;
	}

	void IIRFilterLambda::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}
}