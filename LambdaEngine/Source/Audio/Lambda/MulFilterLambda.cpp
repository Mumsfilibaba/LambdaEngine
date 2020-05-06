#include "Audio/Lambda/MulFilterLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	MulFilterLambda::MulFilterLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	MulFilterLambda::~MulFilterLambda()
	{
	}

	bool MulFilterLambda::Init(const MulFilterDesc* pDesc)
	{
		m_pName			= pDesc->pName;
		m_Multiplier	= pDesc->Multiplier;

		D_LOG_MESSAGE("[MulFilterLambda]: Created Add Filter \"%s\"", m_pName);

		return true;
	}

	IAudioFilter* MulFilterLambda::CreateCopy() const
	{
		MulFilterDesc desc = {};
		desc.pName		= m_pName;
		desc.Multiplier	= m_Multiplier;

		return m_pAudioDevice->CreateMulFilter(&desc);
	}

	float64 MulFilterLambda::ProcessSample(float64 sample)
	{
		float64 outputSample = 0.0;

		if (m_Enabled)
		{
			outputSample = m_Multiplier * sample;
		}
		else
		{
			outputSample = sample;
		}

		outputSample = glm::clamp<double>(outputSample, -1.0, 1.0);
		return outputSample;
	}

	void MulFilterLambda::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}
}