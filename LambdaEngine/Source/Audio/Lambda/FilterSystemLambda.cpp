#include "Audio/Lambda/FilterSystemLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	FilterSystemLambda::FilterSystemLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	FilterSystemLambda::~FilterSystemLambda()
	{
		for (uint32 i = 0; i < m_AudioFilterCount; i++)
		{
			SAFEDELETE(m_ppAudioFilters[i]);
		}

		SAFEDELETE_ARRAY(m_pFilterOutputs);
		SAFEDELETE_ARRAY(m_ppAudioFilters);
		SAFEDELETE_ARRAY(m_pFilterConnections);
	}

	bool FilterSystemLambda::Init(const FilterSystemDesc* pDesc)
	{
		m_pName = pDesc->pName;

		m_AudioFilterCount				= pDesc->AudioFilterCount;
		m_pFilterOutputs				= DBG_NEW float64[m_AudioFilterCount];
		m_ppAudioFilters				= DBG_NEW IAudioFilter*[m_AudioFilterCount];
		memset(m_pFilterOutputs,		0,						m_AudioFilterCount * sizeof(float64));
		
		for (uint32 i = 0; i < m_AudioFilterCount; i++)
		{
			m_ppAudioFilters[i] = pDesc->ppAudioFilters[i]->CreateCopy();
		}
		
		m_FilterConnectionsCount		= pDesc->FilterConnectionsCount;
		m_pFilterConnections			= DBG_NEW FilterSystemConnection[m_FilterConnectionsCount];
		memcpy(m_pFilterConnections,	pDesc->pFilterConnections, m_FilterConnectionsCount * sizeof(FilterSystemConnection));

		DebugPrintExecutionOrder();

		D_LOG_MESSAGE("[FilterSystemLambda]: Created Filter System \"%s\"", m_pName);

		return true;
	}

	IAudioFilter* FilterSystemLambda::CreateCopy() const
	{
		FilterSystemDesc desc = {};
		desc.pName						= m_pName;
		desc.ppAudioFilters				= m_ppAudioFilters;
		desc.AudioFilterCount			= m_AudioFilterCount;
		desc.pFilterConnections			= m_pFilterConnections;
		desc.FilterConnectionsCount		= m_FilterConnectionsCount;

		return m_pAudioDevice->CreateFilterSystem(&desc);
	}

	float64 FilterSystemLambda::ProcessSample(float64 sample)
	{
		float64 outputSample = 0.0;

		if (m_Enabled)
		{
			//Loop through filter connections
			for (uint32 fc = 0; fc < m_FilterConnectionsCount; fc++)
			{
				const FilterSystemConnection* pConnection = &m_pFilterConnections[fc];

				float64 inputSample		= 0.0;
				for (uint32 p = 0; p < pConnection->PreviousFiltersCount; p++)
				{
					int32 previousFilterIndex = pConnection->pPreviousFilters[p];
					inputSample += previousFilterIndex >= 0 ? m_pFilterOutputs[previousFilterIndex] : sample;
				}

				if (pConnection->NextFilter >= 0)
				{
					float64* pOutputSample	= &m_pFilterOutputs[pConnection->NextFilter];
					IAudioFilter* pFilter	= m_ppAudioFilters[pConnection->NextFilter];


					(*pOutputSample) =  pFilter->ProcessSample(inputSample);
					//D_LOG_MESSAGE("%s: %f", pFilter->GetName(), (*pOutputSample));
				}
				else
				{
					outputSample = inputSample;
					//D_LOG_MESSAGE("%s: %f", "FINAL", outputSample);
				}
			}
		}
		else
		{
			outputSample = sample;
		}

		outputSample = glm::clamp<double>(outputSample, -1.0, 1.0);
		return outputSample;
	}

	void FilterSystemLambda::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}

	void FilterSystemLambda::DebugPrintExecutionOrder()
	{
		std::set<std::string> executedFilters;

		for (uint32 fc = 0; fc < m_FilterConnectionsCount; fc++)
		{
			const FilterSystemConnection* pConnection = &m_pFilterConnections[fc];

			std::string outputString = "";

			for (uint32 p = 0; p < pConnection->PreviousFiltersCount; p++)
			{
				int32 prevFilterIndex = pConnection->pPreviousFilters[p];

				if (prevFilterIndex >= 0)
				{
					const IAudioFilter* pPrevFilter = m_ppAudioFilters[prevFilterIndex];

					if (executedFilters.count(pPrevFilter->GetName()) == 0)
						outputString += "OLD-";

					outputString += pPrevFilter->GetName();
				}
				else
				{
					outputString += "INPUT SAMPLE";
				}

				if (p < pConnection->PreviousFiltersCount - 1)
					outputString += ", ";
			}

			outputString += " -> ";

			if (pConnection->NextFilter >= 0)
			{
				const IAudioFilter* pExecutedFilter = m_ppAudioFilters[pConnection->NextFilter];

				outputString += pExecutedFilter->GetName();

				executedFilters.insert(pExecutedFilter->GetName());
			}
			else
			{
				outputString += "OUTPUT SAMPLE";
			}

			LOG_INFO("%s", outputString.c_str());
		}
	}
}