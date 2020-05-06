#include "Audio/Lambda/AudioDeviceLambda.h"
#include "Audio/Lambda/MusicLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/SoundInstance3DLambda.h"
#include "Audio/Lambda/ManagedSoundInstance3DLambda.h"
#include "Audio/Lambda/FIRFilterLambda.h"
#include "Audio/Lambda/IIRFilterLambda.h"
#include "Audio/Lambda/AddFilterLambda.h"
#include "Audio/Lambda/MulFilterLambda.h"
#include "Audio/Lambda/CombFilterLambda.h"
#include "Audio/Lambda/AllPassFilterLambda.h"
#include "Audio/Lambda/FilterSystemLambda.h"

#include "Audio/API/IAudioFilter.h"
#include "Audio/API/AudioHelpers.h"

#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "portaudio.h"

#include <Butterworth.h>

#define FIXED_TICK_RATE 60
#define SAMPLES_PER_TICK 1024

namespace LambdaEngine
{	
	AudioDeviceLambda::AudioDeviceLambda()
	{
	}

	AudioDeviceLambda::~AudioDeviceLambda()
	{
		PaError paResult;
		
		for (auto it = m_ManagedInstances.begin(); it != m_ManagedInstances.end(); it++)
		{
			ManagedSoundInstance3DLambda* pManagedSoundInstance = it->second;
			SAFEDELETE(pManagedSoundInstance);
		}
		m_ManagedInstances.clear();

		m_MusicInstances.clear();
		m_SoundEffects.clear();
		m_SoundInstances.clear();

		paResult = Pa_Terminate();
		if (paResult != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not terminate PortAudio, error: \"%s\"", Pa_GetErrorText(paResult));
		}

		for (auto it = m_Resamplers.begin(); it != m_Resamplers.end(); it++)
		{
			r8b::CDSPResampler* pResampler = it->second;
			pResampler->clear();
			delete pResampler;
		}
		m_Resamplers.clear();

		for (uint32 i = 0; i < m_OutputChannelCount; i++)
		{
			SAFEDELETE(m_ppWriteBuffers[i]);
			SAFEDELETE(m_ppWaveForms[i]);
		}

		SAFEDELETE_ARRAY(m_ppWriteBuffers);
		SAFEDELETE_ARRAY(m_ppWaveForms);
	}

	bool AudioDeviceLambda::Init(const AudioDeviceDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;
		m_MasterVolume = glm::clamp(pDesc->MasterVolume, -1.0, 1.0);
		m_MaxNumAudioListeners = pDesc->MaxNumAudioListeners;

		if (pDesc->MaxNumAudioListeners > 1)
		{
			LOG_ERROR("[AudioDeviceLambda]: MaxNumAudioListeners can not be greater than 1 in the current version of the Audio Engine");
			return false;
		}

		PaError paResult;

		paResult = Pa_Initialize();
		if (paResult != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not initialize PortAudio, error: \"%s\"", Pa_GetErrorText(paResult));
			return false;
		}

		PaDeviceIndex deviceIndex		= Pa_GetDefaultOutputDevice();
		const PaDeviceInfo* pDeviceInfo = Pa_GetDeviceInfo(deviceIndex);

		m_SpeakerSetup					= pDesc->SpeakerSetup;
		int32 outputChannelCount		= ConvertSpeakerSetupToChannelCount(m_SpeakerSetup);

		if (outputChannelCount > pDeviceInfo->maxOutputChannels)
		{
			LOG_WARNING("[AudioDeviceLambda]: Speaker Setup requires a higher channel count %u than the default device can handle %u", outputChannelCount, pDeviceInfo->maxOutputChannels);
			outputChannelCount	= 2;
			m_SpeakerSetup		= ESpeakerSetup::STEREO_SOUND_SYSTEM;
		}

		m_DeviceIndex				= deviceIndex;
		m_OutputChannelCount		= outputChannelCount;
		m_SampleRate				= pDeviceInfo->defaultSampleRate;
		m_DefaultOutputLatency		= pDeviceInfo->defaultLowOutputLatency;

		if (!InitStream())
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not initialize Stream!");
			return false;
		}

		return true;
	}

	void AudioDeviceLambda::FixedTick(Timestamp delta)
	{
		float64 elapsedSinceStart = EngineLoop::GetTimeSinceStart().AsSeconds();

		for (auto it = m_MusicInstancesToDelete.begin(); it != m_MusicInstancesToDelete.end(); it++)
		{
			m_MusicInstances.erase(*it);
		}
		m_MusicInstancesToDelete.clear();

		for (auto it = m_SoundEffectsToDelete.begin(); it != m_SoundEffectsToDelete.end(); it++)
		{
			m_SoundEffects.erase(*it);
		}
		m_SoundEffectsToDelete.clear();

		for (auto it = m_SoundInstancesToDelete.begin(); it != m_SoundInstancesToDelete.end(); it++)
		{
			m_SoundInstances.erase(*it);
		}
		m_SoundInstancesToDelete.clear();

		for (auto it = m_ManagedInstances.begin(); it != m_ManagedInstances.end();)
		{
			ManagedSoundInstance3DLambda* pManagedSoundInstance = it->second;

			if (elapsedSinceStart >= it->first)
			{
				SAFEDELETE(pManagedSoundInstance);
				it = m_ManagedInstances.erase(it);
			}
			else
			{
				it++;
			}
		}

		UpdateWaveForm();
	}

	void AudioDeviceLambda::UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc)
	{
		auto it = m_AudioListenerMap.find(index);

		if (it == m_AudioListenerMap.end())
		{
			LOG_WARNING("[AudioDeviceLambda]: Audio Listener with index %u could not be found in device %s!", index, m_pName);
			return;
		}

		uint32 arrayIndex = it->second;
		m_AudioListeners[arrayIndex] = *pDesc;
	}

	uint32 AudioDeviceLambda::CreateAudioListener()
	{
		if (m_NumAudioListeners >= m_MaxNumAudioListeners)
		{
			LOG_WARNING("[AudioDeviceLambda]: Audio Listener could not be created, max amount reached for %s!", m_pName);
			return UINT32_MAX;
		}

		uint32 index = m_NumAudioListeners++;

		AudioListenerDesc audioListener = {};
		m_AudioListeners.push_back(audioListener);

		m_AudioListenerMap[index] = m_AudioListeners.size() - 1;

		return index;
	}

	IMusic* AudioDeviceLambda::CreateMusic(const MusicDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		MusicLambda* pMusic = DBG_NEW MusicLambda(this);

		if (!pMusic->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			uint32 srcSampleRate = pMusic->GetSrcSampleRate();
			auto it = m_Resamplers.find(srcSampleRate);

			if (it == m_Resamplers.end())
			{
				r8b::CDSPResampler* pResampler = new r8b::CDSPResampler
				(
					(double)srcSampleRate,
					(double)m_SampleRate,
					SAMPLES_PER_TICK
				);

				m_Resamplers.insert({srcSampleRate, pResampler });
			}

			pMusic->Resample();

			m_MusicInstances.insert(pMusic);
			return pMusic;
		}
	}

	ISoundEffect3D* AudioDeviceLambda::CreateSoundEffect(const SoundEffect3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundEffect3DLambda* pSoundEffect = DBG_NEW SoundEffect3DLambda(this);

		if (!pSoundEffect->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			uint32 srcSampleRate = pSoundEffect->GetSrcSampleRate();
			auto it = m_Resamplers.find(srcSampleRate);

			if (it == m_Resamplers.end())
			{
				r8b::CDSPResampler* pResampler = new r8b::CDSPResampler
				(
					(double)srcSampleRate, 
					(double)m_SampleRate, 
					SAMPLES_PER_TICK
				);

				m_Resamplers.insert({ srcSampleRate, pResampler });
			}

			pSoundEffect->Resample();

			m_SoundEffects.insert(pSoundEffect);
			return pSoundEffect;
		}
	}

	ISoundInstance3D* AudioDeviceLambda::CreateSoundInstance(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundInstance3DLambda* pSoundInstance = DBG_NEW SoundInstance3DLambda(this);

		if (!pSoundInstance->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			m_SoundInstances.insert(pSoundInstance);
			return pSoundInstance;
		}
	}

	IAudioGeometry* AudioDeviceLambda::CreateAudioGeometry(const AudioGeometryDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		return nullptr;
	}

	IReverbSphere* AudioDeviceLambda::CreateReverbSphere(const ReverbSphereDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		return nullptr;
	}

	IAudioFilter* AudioDeviceLambda::CreateFIRFilter(const FIRFilterDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		FIRFilterLambda* pFIRFilter = DBG_NEW FIRFilterLambda(this);

		if (!pFIRFilter->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pFIRFilter;
		}
	}

	IAudioFilter* AudioDeviceLambda::CreateIIRFilter(const IIRFilterDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		IIRFilterLambda* pIIRFilter = DBG_NEW IIRFilterLambda(this);

		if (!pIIRFilter->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pIIRFilter;
		}
	}

	IAudioFilter* AudioDeviceLambda::CreateAddFilter(const AddFilterDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		AddFilterLambda* pAddFilterLambda = DBG_NEW AddFilterLambda(this);

		if (!pAddFilterLambda->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pAddFilterLambda;
		}
	}

	IAudioFilter* AudioDeviceLambda::CreateMulFilter(const MulFilterDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		MulFilterLambda* pMulFilterLambda = DBG_NEW MulFilterLambda(this);

		if (!pMulFilterLambda->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pMulFilterLambda;
		}
	}

	IAudioFilter* AudioDeviceLambda::CreateCombFilter(const CombFilterDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		CombFilterLambda* pCombFilterLambda = DBG_NEW CombFilterLambda(this);

		if (!pCombFilterLambda->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pCombFilterLambda;
		}
	}

	IAudioFilter* AudioDeviceLambda::CreateAllPassFilter(const AllPassFilterDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		AllPassFilterLambda* pAllPassFilterLambda = DBG_NEW AllPassFilterLambda(this);

		if (!pAllPassFilterLambda->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pAllPassFilterLambda;
		}
	}

	IAudioFilter* AudioDeviceLambda::CreateFilterSystem(const FilterSystemDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		FilterSystemLambda* pFilterSystem = DBG_NEW FilterSystemLambda(this);

		if (!pFilterSystem->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pFilterSystem;
		}
	}

	IAudioFilter* AudioDeviceLambda::CreateLowpassFIRFilter(float64 cutoffFreq, uint32 order) const
	{
		//Normalize
		int32 oddOrder				= order % 2 == 0 ? order + 1: order;
		float64 normalizedCutoff	= cutoffFreq / m_SampleRate;
		float64 angularCutoff		= glm::two_pi<double>() * normalizedCutoff;
		int32 middle				= oddOrder / 2;

		float64* pWeights = DBG_NEW float64[oddOrder];

		for (int32 n = -middle; n <= middle; n++)
		{
			float64 n_f = (float64)n;
			float64 weight;

			if (n == 0)
			{
				weight = 2.0 * normalizedCutoff;
			}
			else
			{
				weight = glm::sin(angularCutoff * n_f) / (glm::pi<double>() * n_f);
			}

			//Hamming Window
			float64 a0 = 0.53836;
			float64 a1 = 0.46164;
			float64 hammingWeight = a0 + a1 * glm::cos(glm::two_pi<double>() * n_f / ((float64)oddOrder));

			pWeights[middle + n] = hammingWeight * weight;
		}

		FIRFilterDesc filterDesc = {};
		filterDesc.pName			= "Lowpass FIR Filter";
		filterDesc.pWeights			= pWeights;
		filterDesc.WeightsCount		= oddOrder;

		IAudioFilter* pFIRFilter = CreateFIRFilter(&filterDesc);

		SAFEDELETE_ARRAY(pWeights);
		
		return pFIRFilter;
	}

	IAudioFilter* AudioDeviceLambda::CreateHighpassFIRFilter(float64 cutoffFreq, uint32 order) const
	{
		//Normalize
		int32 oddOrder				= order % 2 == 0 ? order + 1: order;
		float64 normalizedCutoff	= cutoffFreq / m_SampleRate;
		float64 angularCutoff		= glm::two_pi<double>() * normalizedCutoff;
		int32 middle				= oddOrder / 2;

		float64* pWeights = DBG_NEW float64[oddOrder];

		for (int32 n = -middle; n <= middle; n++)
		{
			float64 n_f = (float64)n;
			float64 weight;

			if (n == 0)
			{
				weight = 1.0 - 2.0 * normalizedCutoff;
			}
			else
			{
				weight = -glm::sin(angularCutoff * n_f) / (glm::pi<double>() * n_f);
			}

			//Hamming Window
			float64 a0 = 0.53836;
			float64 a1 = 0.46164;
			float64 hammingWeight = a0 + a1 * glm::cos(glm::two_pi<double>() * n_f / ((float64)oddOrder));

			pWeights[middle + n] = hammingWeight * weight;
		}

		FIRFilterDesc filterDesc = {};
		filterDesc.pName			= "Highpass FIR Filter";
		filterDesc.pWeights			= pWeights;
		filterDesc.WeightsCount		= oddOrder;

		IAudioFilter* pFIRFilter = CreateFIRFilter(&filterDesc);

		SAFEDELETE_ARRAY(pWeights);
		
		return pFIRFilter;
	}

	IAudioFilter* AudioDeviceLambda::CreateBandpassFIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order) const
	{
		//Normalize
		int32 oddOrder					= order % 2 == 0 ? order + 1: order;
		float64 normalizedLowCutoff		= lowCutoffFreq / m_SampleRate;
		float64 normalizedHighCutoff	= highCutoffFreq / m_SampleRate;
		float64 angularLowCutoff		= glm::two_pi<double>() * normalizedLowCutoff;
		float64 angularHighCutoff		= glm::two_pi<double>() * normalizedHighCutoff;
		int32 middle					= oddOrder / 2;

		float64* pWeights = DBG_NEW float64[oddOrder];

		for (int32 n = -middle; n <= middle; n++)
		{
			float64 n_f = (float64)n;
			float64 weight;

			if (n == 0)
			{
				weight = 2.0 * (normalizedHighCutoff - normalizedLowCutoff);
			}
			else
			{
				weight = (glm::sin(angularHighCutoff * n_f) - glm::sin(angularLowCutoff * n_f))/ (glm::pi<double>() * n_f);
			}

			//Hamming Window
			float64 a0 = 0.53836;
			float64 a1 = 0.46164;
			float64 hammingWeight = a0 + a1 * glm::cos(glm::two_pi<double>() * n_f / ((float64)oddOrder));

			pWeights[middle + n] = hammingWeight * weight;
		}

		FIRFilterDesc filterDesc = {};
		filterDesc.pName			= "Bandpass FIR Filter";
		filterDesc.pWeights			= pWeights;
		filterDesc.WeightsCount		= oddOrder;

		IAudioFilter* pFIRFilter = CreateFIRFilter(&filterDesc);

		SAFEDELETE_ARRAY(pWeights);
		
		return pFIRFilter;
	}

	IAudioFilter* AudioDeviceLambda::CreateBandstopFIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order) const
	{
		//Normalize
		int32 oddOrder					= order % 2 == 0 ? order + 1: order;
		float64 normalizedLowCutoff		= lowCutoffFreq / m_SampleRate;
		float64 normalizedHighCutoff	= highCutoffFreq / m_SampleRate;
		float64 angularLowCutoff		= glm::two_pi<double>() * normalizedLowCutoff;
		float64 angularHighCutoff		= glm::two_pi<double>() * normalizedHighCutoff;
		int32 middle					= oddOrder / 2;

		float64* pWeights = DBG_NEW float64[oddOrder];

		for (int32 n = -middle; n <= middle; n++)
		{
			float64 n_f = (float64)n;
			float64 weight;

			if (n == 0)
			{
				weight = 1.0 - 2.0 * (normalizedHighCutoff - normalizedLowCutoff);
			}
			else
			{
				weight = (glm::sin(angularLowCutoff * n_f) - glm::sin(angularHighCutoff * n_f))/ (glm::pi<double>() * n_f);
			}

			//Hamming Window
			float64 a0 = 0.53836;
			float64 a1 = 0.46164;
			float64 hammingWeight = a0 + a1 * glm::cos(glm::two_pi<double>() * n_f / ((float64)oddOrder));

			pWeights[middle + n] = hammingWeight * weight;
		}

		FIRFilterDesc filterDesc = {};
		filterDesc.pName			= "Bandstop FIR Filter";
		filterDesc.pWeights			= pWeights;
		filterDesc.WeightsCount		= oddOrder;

		IAudioFilter* pFIRFilter = CreateFIRFilter(&filterDesc);

		SAFEDELETE_ARRAY(pWeights);
		
		return pFIRFilter;
	}

	IAudioFilter* AudioDeviceLambda::CreateLowpassIIRFilter(float64 cutoffFreq, uint32 order) const
	{
		Butterworth butterworth;
		std::vector<Biquad> biquads;
		double overallGain;

		if (!butterworth.loPass((float64)m_SampleRate, cutoffFreq, 0.0, order, biquads, overallGain))
		{
			LOG_ERROR("[AudioDeviceLambda] butterworth::loPass failed!");
			return nullptr;
		}

		IAudioFilter* pFinalFilter;

		if (biquads.size() > 1)
		{
			uint32 filterCount = biquads.size();
			std::vector<IAudioFilter*> filters;
			std::vector<FilterSystemConnection> filterConnections;
			filters.reserve(filterCount);
			filterConnections.reserve(filterCount + 1);

			for (int32 f = 0; f < filterCount; f++)
			{
				const Biquad* pBiquad = &biquads[f];

				float64 gain = f > 0 ? 1.0f : overallGain;

				float64 pFeedForwardWeights[3];
				pFeedForwardWeights[0]			= gain * pBiquad->b0;
				pFeedForwardWeights[1]			= gain * pBiquad->b1;
				pFeedForwardWeights[2]			= gain * pBiquad->b2;

				float64 pFeedBackWeights[2];
				pFeedBackWeights[0] = -pBiquad->a1;
				pFeedBackWeights[1] = -pBiquad->a2;

				IIRFilterDesc filterDesc = {};
				filterDesc.pName					= "Lowpass IIR Filter Biquad";
				filterDesc.pFeedForwardWeights		= pFeedForwardWeights;
				filterDesc.FeedForwardWeightsCount	= 3;
				filterDesc.pFeedBackWeights			= pFeedBackWeights;
				filterDesc.FeedBackWeightsCount		= 2;
				
				filters.push_back(CreateIIRFilter(&filterDesc));
				
				FilterSystemConnection connection = {};
				connection.pPreviousFilters[0]		= f - 1;
				connection.PreviousFiltersCount		= 1;
				connection.NextFilter				= f;

				filterConnections.push_back(connection);
			}

			FilterSystemConnection outputConnection = {};
			outputConnection.pPreviousFilters[0]		= filterCount - 1;
			outputConnection.PreviousFiltersCount		= 1;
			outputConnection.NextFilter					= -1;

			filterConnections.push_back(outputConnection);

			FilterSystemDesc filterSystemDesc = {};
			filterSystemDesc.pName					= "Lowpass IIR Filter System";
			filterSystemDesc.ppAudioFilters			= filters.data();
			filterSystemDesc.AudioFilterCount		= filters.size();
			filterSystemDesc.pFilterConnections		= filterConnections.data();
			filterSystemDesc.FilterConnectionsCount = filterConnections.size();

			pFinalFilter = CreateFilterSystem(&filterSystemDesc);

			for (uint32 f = 0; f < filters.size(); f++)
			{
				SAFEDELETE(filters[f]);
			}
		}
		else
		{
			const Biquad* pBiquad = &biquads[0];

			float64 pFeedForwardWeights[3];
			pFeedForwardWeights[0]			= overallGain * pBiquad->b0;
			pFeedForwardWeights[1]			= overallGain * pBiquad->b1;
			pFeedForwardWeights[2]			= overallGain * pBiquad->b2;

			float64 pFeedBackWeights[2];
			pFeedBackWeights[0] = -pBiquad->a1;
			pFeedBackWeights[1] = -pBiquad->a2;

			IIRFilterDesc filterDesc = {};
			filterDesc.pName					= "Lowpass IIR Filter";
			filterDesc.pFeedForwardWeights		= pFeedForwardWeights;
			filterDesc.FeedForwardWeightsCount	= 3;
			filterDesc.pFeedBackWeights			= pFeedBackWeights;
			filterDesc.FeedBackWeightsCount		= 2;

			pFinalFilter = CreateIIRFilter(&filterDesc);
		}

		return pFinalFilter;
	}

	IAudioFilter* AudioDeviceLambda::CreateHighpassIIRFilter(float64 cutoffFreq, uint32 order) const
	{
		Butterworth butterworth;
		std::vector<Biquad> biquads;
		double overallGain;

		if (!butterworth.hiPass((float64)m_SampleRate, cutoffFreq, 0.0, order, biquads, overallGain))
		{
			LOG_ERROR("[AudioDeviceLambda] butterworth::hiPass failed!");
			return nullptr;
		}

		IAudioFilter* pFinalFilter;

		if (biquads.size() > 1)
		{
			uint32 filterCount = biquads.size();
			std::vector<IAudioFilter*> filters;
			std::vector<FilterSystemConnection> filterConnections;
			filters.reserve(filterCount);
			filterConnections.reserve(filterCount + 1);

			for (int32 f = 0; f < filterCount; f++)
			{
				const Biquad* pBiquad = &biquads[f];

				float64 gain = f > 0 ? 1.0f : overallGain;

				float64 pFeedForwardWeights[3];
				pFeedForwardWeights[0]			= gain * pBiquad->b0;
				pFeedForwardWeights[1]			= gain * pBiquad->b1;
				pFeedForwardWeights[2]			= gain * pBiquad->b2;

				float64 pFeedBackWeights[2];
				pFeedBackWeights[0] = -pBiquad->a1;
				pFeedBackWeights[1] = -pBiquad->a2;

				IIRFilterDesc filterDesc = {};
				filterDesc.pName					= "Highpass IIR Filter Biquad";
				filterDesc.pFeedForwardWeights		= pFeedForwardWeights;
				filterDesc.FeedForwardWeightsCount	= 3;
				filterDesc.pFeedBackWeights			= pFeedBackWeights;
				filterDesc.FeedBackWeightsCount		= 2;
				
				filters.push_back(CreateIIRFilter(&filterDesc));
				
				FilterSystemConnection connection = {};
				connection.pPreviousFilters[0]		= f - 1;
				connection.PreviousFiltersCount		= 1;
				connection.NextFilter				= f;

				filterConnections.push_back(connection);
			}

			FilterSystemConnection outputConnection = {};
			outputConnection.pPreviousFilters[0]		= filterCount - 1;
			outputConnection.PreviousFiltersCount		= 1;
			outputConnection.NextFilter					= -1;

			filterConnections.push_back(outputConnection);

			FilterSystemDesc filterSystemDesc = {};
			filterSystemDesc.pName					= "Highpass IIR Filter System";
			filterSystemDesc.ppAudioFilters			= filters.data();
			filterSystemDesc.AudioFilterCount		= filters.size();
			filterSystemDesc.pFilterConnections		= filterConnections.data();
			filterSystemDesc.FilterConnectionsCount = filterConnections.size();

			pFinalFilter = CreateFilterSystem(&filterSystemDesc);

			for (uint32 f = 0; f < filters.size(); f++)
			{
				SAFEDELETE(filters[f]);
			}
		}
		else
		{
			const Biquad* pBiquad = &biquads[0];

			float64 pFeedForwardWeights[3];
			pFeedForwardWeights[0]			= overallGain * pBiquad->b0;
			pFeedForwardWeights[1]			= overallGain * pBiquad->b1;
			pFeedForwardWeights[2]			= overallGain * pBiquad->b2;

			float64 pFeedBackWeights[2];
			pFeedBackWeights[0] = -pBiquad->a1;
			pFeedBackWeights[1] = -pBiquad->a2;

			IIRFilterDesc filterDesc = {};
			filterDesc.pName					= "Highpass IIR Filter";
			filterDesc.pFeedForwardWeights		= pFeedForwardWeights;
			filterDesc.FeedForwardWeightsCount	= 3;
			filterDesc.pFeedBackWeights			= pFeedBackWeights;
			filterDesc.FeedBackWeightsCount		= 2;

			pFinalFilter = CreateIIRFilter(&filterDesc);
		}

		return pFinalFilter;
	}

	IAudioFilter* AudioDeviceLambda::CreateBandpassIIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order) const
	{
		Butterworth butterworth;
		std::vector<Biquad> biquads;
		double overallGain;

		if (!butterworth.bandPass((float64)m_SampleRate, lowCutoffFreq, highCutoffFreq, order, biquads, overallGain))
		{
			LOG_ERROR("[AudioDeviceLambda] butterworth::bandPass failed!");
			return nullptr;
		}

		IAudioFilter* pFinalFilter;

		if (biquads.size() > 1)
		{
			uint32 filterCount = biquads.size();
			std::vector<IAudioFilter*> filters;
			std::vector<FilterSystemConnection> filterConnections;
			filters.reserve(filterCount);
			filterConnections.reserve(filterCount + 1);

			for (int32 f = 0; f < filterCount; f++)
			{
				const Biquad* pBiquad = &biquads[f];

				float64 gain = f > 0 ? 1.0f : overallGain;

				float64 pFeedForwardWeights[3];
				pFeedForwardWeights[0]			= gain * pBiquad->b0;
				pFeedForwardWeights[1]			= gain * pBiquad->b1;
				pFeedForwardWeights[2]			= gain * pBiquad->b2;

				float64 pFeedBackWeights[2];
				pFeedBackWeights[0] = -pBiquad->a1;
				pFeedBackWeights[1] = -pBiquad->a2;

				IIRFilterDesc filterDesc = {};
				filterDesc.pName					= "Bandpass IIR Filter Biquad";
				filterDesc.pFeedForwardWeights		= pFeedForwardWeights;
				filterDesc.FeedForwardWeightsCount	= 3;
				filterDesc.pFeedBackWeights			= pFeedBackWeights;
				filterDesc.FeedBackWeightsCount		= 2;
				
				filters.push_back(CreateIIRFilter(&filterDesc));
				
				FilterSystemConnection connection = {};
				connection.pPreviousFilters[0]		= f - 1;
				connection.PreviousFiltersCount		= 1;
				connection.NextFilter				= f;

				filterConnections.push_back(connection);
			}

			FilterSystemConnection outputConnection = {};
			outputConnection.pPreviousFilters[0]		= filterCount - 1;
			outputConnection.PreviousFiltersCount		= 1;
			outputConnection.NextFilter					= -1;

			filterConnections.push_back(outputConnection);

			FilterSystemDesc filterSystemDesc = {};
			filterSystemDesc.pName					= "Bandpass IIR Filter System";
			filterSystemDesc.ppAudioFilters			= filters.data();
			filterSystemDesc.AudioFilterCount		= filters.size();
			filterSystemDesc.pFilterConnections		= filterConnections.data();
			filterSystemDesc.FilterConnectionsCount = filterConnections.size();

			pFinalFilter = CreateFilterSystem(&filterSystemDesc);

			for (uint32 f = 0; f < filters.size(); f++)
			{
				SAFEDELETE(filters[f]);
			}
		}
		else
		{
			const Biquad* pBiquad = &biquads[0];

			float64 pFeedForwardWeights[3];
			pFeedForwardWeights[0]			= overallGain * pBiquad->b0;
			pFeedForwardWeights[1]			= overallGain * pBiquad->b1;
			pFeedForwardWeights[2]			= overallGain * pBiquad->b2;

			float64 pFeedBackWeights[2];
			pFeedBackWeights[0] = -pBiquad->a1;
			pFeedBackWeights[1] = -pBiquad->a2;

			IIRFilterDesc filterDesc = {};
			filterDesc.pName					= "Bandpass IIR Filter";
			filterDesc.pFeedForwardWeights		= pFeedForwardWeights;
			filterDesc.FeedForwardWeightsCount	= 3;
			filterDesc.pFeedBackWeights			= pFeedBackWeights;
			filterDesc.FeedBackWeightsCount		= 2;

			pFinalFilter = CreateIIRFilter(&filterDesc);
		}

		return pFinalFilter;
	}

	IAudioFilter* AudioDeviceLambda::CreateBandstopIIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order) const
	{
		Butterworth butterworth;
		std::vector<Biquad> biquads;
		double overallGain;

		if (!butterworth.bandStop((float64)m_SampleRate, lowCutoffFreq, highCutoffFreq, order, biquads, overallGain))
		{
			LOG_ERROR("[AudioDeviceLambda] butterworth::bandStop failed!");
			return nullptr;
		}

		IAudioFilter* pFinalFilter;

		if (biquads.size() > 1)
		{
			uint32 filterCount = biquads.size();
			std::vector<IAudioFilter*> filters;
			std::vector<FilterSystemConnection> filterConnections;
			filters.reserve(filterCount);
			filterConnections.reserve(filterCount + 1);

			for (int32 f = 0; f < filterCount; f++)
			{
				const Biquad* pBiquad = &biquads[f];

				float64 gain = f > 0 ? 1.0f : overallGain;

				float64 pFeedForwardWeights[3];
				pFeedForwardWeights[0]			= gain * pBiquad->b0;
				pFeedForwardWeights[1]			= gain * pBiquad->b1;
				pFeedForwardWeights[2]			= gain * pBiquad->b2;

				float64 pFeedBackWeights[2];
				pFeedBackWeights[0] = -pBiquad->a1;
				pFeedBackWeights[1] = -pBiquad->a2;

				IIRFilterDesc filterDesc = {};
				filterDesc.pName					= "Bandstop IIR Filter Biquad";
				filterDesc.pFeedForwardWeights		= pFeedForwardWeights;
				filterDesc.FeedForwardWeightsCount	= 3;
				filterDesc.pFeedBackWeights			= pFeedBackWeights;
				filterDesc.FeedBackWeightsCount		= 2;
				
				filters.push_back(CreateIIRFilter(&filterDesc));
				
				FilterSystemConnection connection = {};
				connection.pPreviousFilters[0]		= f - 1;
				connection.PreviousFiltersCount		= 1;
				connection.NextFilter				= f;

				filterConnections.push_back(connection);
			}

			FilterSystemConnection outputConnection = {};
			outputConnection.pPreviousFilters[0]		= filterCount - 1;
			outputConnection.PreviousFiltersCount		= 1;
			outputConnection.NextFilter					= -1;

			filterConnections.push_back(outputConnection);

			FilterSystemDesc filterSystemDesc = {};
			filterSystemDesc.pName					= "Bandstop IIR Filter System";
			filterSystemDesc.ppAudioFilters			= filters.data();
			filterSystemDesc.AudioFilterCount		= filters.size();
			filterSystemDesc.pFilterConnections		= filterConnections.data();
			filterSystemDesc.FilterConnectionsCount = filterConnections.size();

			pFinalFilter = CreateFilterSystem(&filterSystemDesc);

			for (uint32 f = 0; f < filters.size(); f++)
			{
				SAFEDELETE(filters[f]);
			}
		}
		else
		{
			const Biquad* pBiquad = &biquads[0];

			float64 pFeedForwardWeights[3];
			pFeedForwardWeights[0]			= overallGain * pBiquad->b0;
			pFeedForwardWeights[1]			= overallGain * pBiquad->b1;
			pFeedForwardWeights[2]			= overallGain * pBiquad->b2;

			float64 pFeedBackWeights[2];
			pFeedBackWeights[0] = -pBiquad->a1;
			pFeedBackWeights[1] = -pBiquad->a2;

			IIRFilterDesc filterDesc = {};
			filterDesc.pName					= "Bandstop IIR Filter";
			filterDesc.pFeedForwardWeights		= pFeedForwardWeights;
			filterDesc.FeedForwardWeightsCount	= 3;
			filterDesc.pFeedBackWeights			= pFeedBackWeights;
			filterDesc.FeedBackWeightsCount		= 2;

			pFinalFilter = CreateIIRFilter(&filterDesc);
		}

		return pFinalFilter;
	}

	void AudioDeviceLambda::SetMasterFilter(const IAudioFilter* pAudioFilter)
	{
		for (uint32 c = 0; c < m_OutputChannelCount; c++)
		{
			SAFEDELETE(m_ppMasterFilters[c]);
			m_ppMasterFilters[c] = pAudioFilter->CreateCopy();
		}
	}

	void AudioDeviceLambda::SetMasterFilterEnabled(bool enabled)
	{
		for (uint32 c = 0; c < m_OutputChannelCount; c++)
		{
			IAudioFilter* pAudioFilter = m_ppMasterFilters[c];

			if (pAudioFilter != nullptr)
			{
				pAudioFilter->SetEnabled(enabled);
			}
		}
	}

	void AudioDeviceLambda::SetMasterVolume(float64 volume)
	{
		m_MasterVolume = glm::clamp(volume, -1.0, 1.0);
	}

	bool AudioDeviceLambda::GetMasterFilterEnabled() const
	{
		return m_ppMasterFilters[0] != nullptr && m_ppMasterFilters[0]->GetEnabled();
	}

	void AudioDeviceLambda::AddManagedSoundInstance(const ManagedSoundInstance3DDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		ManagedSoundInstance3DLambda* pManagedSoundInstance = DBG_NEW ManagedSoundInstance3DLambda(this);

		float64 endTime = pDesc->pSoundEffect->GetDuration() + EngineLoop::GetTimeSinceStart().AsSeconds();

		if (pManagedSoundInstance->Init(pDesc))
		{
			m_ManagedInstances.insert({ endTime, pManagedSoundInstance });
		}
		else
		{
			SAFEDELETE(pManagedSoundInstance);
		}
	}

	void AudioDeviceLambda::DeleteMusicInstance(MusicLambda* pMusic) const
	{
		m_MusicInstancesToDelete.insert(pMusic);
	}

	void AudioDeviceLambda::DeleteSoundEffect(SoundEffect3DLambda* pSoundEffect) const
	{
		m_SoundEffectsToDelete.insert(pSoundEffect);
	}

	void AudioDeviceLambda::DeleteSoundInstance(SoundInstance3DLambda* pSoundInstance) const
	{
		m_SoundInstancesToDelete.insert(pSoundInstance);
	}

	bool AudioDeviceLambda::InitStream()
	{
		m_WriteBufferSampleCount	= 2 * (uint32)glm::ceil(m_SampleRate / FIXED_TICK_RATE);
		m_WaveFormSampleCount		= 2 * m_WriteBufferSampleCount;
		m_ppWriteBuffers			= DBG_NEW float64*[m_OutputChannelCount];
		m_ppWaveForms				= DBG_NEW float32*[m_OutputChannelCount];
		m_ppMasterFilters			= DBG_NEW IAudioFilter*[m_OutputChannelCount];
		m_WaveFormReadIndex			= 0;
		m_WaveFormWriteIndex		= m_WriteBufferSampleCount;

		for (uint32 c = 0; c < m_OutputChannelCount; c++)
		{
			m_ppWriteBuffers[c]	= DBG_NEW float64[m_WriteBufferSampleCount];
			m_ppWaveForms[c]	= DBG_NEW float32[m_WaveFormSampleCount];
			memset(m_ppWaveForms[c], 0, m_WaveFormSampleCount * sizeof(float32));
			m_ppMasterFilters[c] = nullptr;
		}


		PaError paResult;

		PaStreamParameters outputParameters = {};
		outputParameters.device						= m_DeviceIndex;
		outputParameters.channelCount				= m_OutputChannelCount;
		outputParameters.sampleFormat				= paFloat32;
		outputParameters.suggestedLatency			= m_DefaultOutputLatency;
		outputParameters.hostApiSpecificStreamInfo	= nullptr;

		PaStreamFlags streamFlags = paNoFlag;

		/* Open an audio I/O stream. */
		paResult = Pa_OpenStream(
			&m_pStream,
			nullptr,
			&outputParameters,
			m_SampleRate,
			paFramesPerBufferUnspecified,
			streamFlags,
			PortAudioCallback,
			this);

		if (paResult != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not open PortAudio stream, error: \"%s\"", Pa_GetErrorText(paResult));
			return false;
		}

		paResult = Pa_StartStream(m_pStream);
		if (paResult != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not start PortAudio stream, error: \"%s\"", Pa_GetErrorText(paResult));
			return false;
		}

		return true;
	}

	void AudioDeviceLambda::UpdateWaveForm()
	{
		uint32 samplesToWriteCount = 0;

		if (m_WaveFormReadIndex == m_WaveFormWriteIndex)
		{
			return;
		}

		if (m_WaveFormReadIndex > m_WaveFormWriteIndex)
		{
			samplesToWriteCount = m_WaveFormReadIndex - m_WaveFormWriteIndex;
		}
		else
		{
			samplesToWriteCount = m_WaveFormSampleCount - m_WaveFormWriteIndex + m_WaveFormReadIndex;
		}

		samplesToWriteCount = glm::min(samplesToWriteCount, m_WriteBufferSampleCount);

		for (uint32 i = 0; i < m_OutputChannelCount; i++)
		{
			memset(m_ppWriteBuffers[i], 0, m_WriteBufferSampleCount * sizeof(float64));
		}

		for (auto it = m_MusicInstances.begin(); it != m_MusicInstances.end(); it++)
		{
			MusicLambda* pMusicInstance = *it;

			pMusicInstance->ProcessBuffer(m_ppWriteBuffers, m_OutputChannelCount, samplesToWriteCount);
		}

		for (auto it = m_SoundInstances.begin(); it != m_SoundInstances.end(); it++)
		{
			SoundInstance3DLambda* pSoundInstance = *it;

			pSoundInstance->UpdateVolume(m_MasterVolume, m_AudioListeners.data(), m_AudioListeners.size(), m_SpeakerSetup);

			pSoundInstance->ProcessBuffer(m_ppWriteBuffers, m_OutputChannelCount, samplesToWriteCount);
		}

		for (auto it = m_ManagedInstances.begin(); it != m_ManagedInstances.end(); it++)
		{
			ManagedSoundInstance3DLambda* pManagedSoundInstance = it->second;

			pManagedSoundInstance->UpdateVolume(m_MasterVolume, m_AudioListeners.data(), m_AudioListeners.size(), m_SpeakerSetup);

			pManagedSoundInstance->AddToBuffer(m_ppWriteBuffers, m_OutputChannelCount, samplesToWriteCount);
		}

		for (uint32 s = 0; s < samplesToWriteCount; s++)
		{
			for (uint32 c = 0; c < m_OutputChannelCount; c++)
			{
				float32 sample = m_MasterVolume * (float32)m_ppWriteBuffers[c][s];

				IAudioFilter* pAudioFilter = m_ppMasterFilters[c];

				if (pAudioFilter != nullptr) sample = pAudioFilter->ProcessSample(sample);

				m_ppWaveForms[c][m_WaveFormWriteIndex] = sample;
			}

			m_WaveFormWriteIndex++;

			if (m_WaveFormWriteIndex == m_WaveFormSampleCount)
				m_WaveFormWriteIndex = 0;
		}
	}

	int32 AudioDeviceLambda::LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer)
	{
		for (uint32 f = 0; f < framesPerBuffer; f++)
		{
			for (uint32 c = 0; c < m_OutputChannelCount; c++)
			{
				float32* pSample = &m_ppWaveForms[c][m_WaveFormReadIndex];
				(*(pOutputBuffer++)) = *pSample;
				(*pSample) = 0.0f;
			}

			m_WaveFormReadIndex++;
			if (m_WaveFormReadIndex == m_WaveFormSampleCount)
				m_WaveFormReadIndex = 0;
		}

		return paNoError;
	}

	int32 AudioDeviceLambda::PortAudioCallback(const void* pInputBuffer, void* pOutputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags, void* pUserData)
	{
		UNREFERENCED_VARIABLE(pInputBuffer);
		UNREFERENCED_VARIABLE(pTimeInfo);
		UNREFERENCED_VARIABLE(statusFlags);

		AudioDeviceLambda* pDevice = reinterpret_cast<AudioDeviceLambda*>(pUserData);
		VALIDATE(pDevice != nullptr);

		return pDevice->LocalAudioCallback(reinterpret_cast<float32*>(pOutputBuffer), framesPerBuffer);
	}
}
