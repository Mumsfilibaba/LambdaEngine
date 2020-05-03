#include "Audio/Lambda/AudioDeviceLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/SoundInstance3DLambda.h"
#include "Audio/Lambda/ManagedSoundInstance3DLambda.h"

#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "portaudio.h"

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

		for (auto it = m_SoundEffects.begin(); it != m_SoundEffects.end(); it++)
		{
			m_SoundEffects.erase(it);
		}
		m_SoundEffects.clear();

		for (auto it = m_SoundInstances.begin(); it != m_SoundInstances.end(); it++)
		{
			m_SoundInstances.erase(it);
		}
		m_SoundInstances.clear();

		SAFEDELETE_ARRAY(m_Music.pWaveForm);

		if (m_Music.pStream != nullptr)
		{
			paResult = Pa_CloseStream(m_Music.pStream);

			if (paResult != paNoError)
			{
				LOG_ERROR("[AudioDeviceLambda]: Could not close PortAudio stream, error: \"%s\"", Pa_GetErrorText(paResult));
			}

			m_Music.pStream = nullptr;
		}

		paResult = Pa_Terminate();
		if (paResult != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not terminate PortAudio, error: \"%s\"", Pa_GetErrorText(paResult));
		}
	}

	bool AudioDeviceLambda::Init(const AudioDeviceDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;
		m_MaxNumAudioListeners = pDesc->MaxNumAudioListeners;

		if (pDesc->MaxNumAudioListeners > 1)
		{
			LOG_ERROR("[AudioDeviceLambda]: MaxNumAudioListeners can not be greater than 1 in the current version of the Audio Engine");
			return false;
		}

		PaError result;

		result = Pa_Initialize();
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not initialize PortAudio, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		return true;
	}

	void AudioDeviceLambda::Tick()
	{
		m_Music.OutputVolume = m_MasterVolume * m_Music.Volume;

		float64 elapsedSinceStart = EngineLoop::GetTimeSinceStart().AsSeconds();

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
				pManagedSoundInstance->UpdateVolume(m_MasterVolume, m_AudioListeners.data(), m_AudioListeners.size());
				it++;
			}
		}

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

		for (auto it = m_SoundInstances.begin(); it != m_SoundInstances.end(); it++)
		{
			SoundInstance3DLambda* pSoundInstance = *it;

			pSoundInstance->UpdateVolume(m_MasterVolume, m_AudioListeners.data(), m_AudioListeners.size());
		}
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
		return nullptr;
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

	void AudioDeviceLambda::DeleteSoundEffect(SoundEffect3DLambda* pSoundEffect) const
	{
		m_SoundEffectsToDelete.insert(pSoundEffect);
	}

	void AudioDeviceLambda::DeleteSoundInstance(SoundInstance3DLambda* pSoundInstance) const
	{
		m_SoundInstancesToDelete.insert(pSoundInstance);
	}

	void AudioDeviceLambda::SetMasterVolume(float volume)
	{
		m_MasterVolume = volume;
	}

	int32 AudioDeviceLambda::LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer)
	{
		for (uint32 f = 0; f < framesPerBuffer; f++)
		{
			for (uint32 c = 0; c < m_Music.ChannelCount; c++)
			{
				float sample = m_Music.pWaveForm[m_Music.CurrentBufferIndex++];
				(*(pOutputBuffer++)) = m_Music.OutputVolume * sample;
			}

			if (m_Music.CurrentBufferIndex == m_Music.TotalSampleCount)
				m_Music.CurrentBufferIndex = 0;
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
