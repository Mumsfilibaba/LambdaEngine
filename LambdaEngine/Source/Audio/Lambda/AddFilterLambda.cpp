#include "Audio/Lambda/AddFilterLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	AddFilterLambda::AddFilterLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	AddFilterLambda::~AddFilterLambda()
	{
	}

	bool AddFilterLambda::Init(const AddFilterDesc* pDesc)
	{
		m_pName = pDesc->pName;

		D_LOG_MESSAGE("[AddFilterLambda]: Created Add Filter \"%s\"", m_pName);

		return true;
	}

	IAudioFilter* AddFilterLambda::CreateCopy() const
	{
		AddFilterDesc desc = {};
		desc.pName	= m_pName;

		return m_pAudioDevice->CreateAddFilter(&desc);
	}

	float64 AddFilterLambda::ProcessSample(float64 sample)
	{
		float64 outputSample = 0.0;

		if (m_Enabled)
		{
			outputSample = sample;
		}
		else
		{
			outputSample = sample;
		}

		outputSample = glm::clamp<double>(outputSample, -1.0, 1.0);
		return outputSample;
	}

	void AddFilterLambda::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}
}