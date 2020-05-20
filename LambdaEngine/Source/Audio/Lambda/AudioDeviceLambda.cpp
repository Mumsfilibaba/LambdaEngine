#include "Log/Log.h"

#include "PortAudio.h"

#include "Audio/Lambda/AudioDeviceLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/SoundInstance3DLambda.h"
#include "Audio/Lambda/MusicLambda.h"
#include "Audio/Lambda/AudioListenerLambda.h"
#include "Audio/Lambda/MusicInstanceLambda.h"

#include <immintrin.h>

namespace LambdaEngine
{	
	AudioDeviceLambda::AudioDeviceLambda()
		: m_SoundEffects(),	
		m_SoundInstances(),
		m_AudioListeners(),
		m_Music(),
		m_MusicInstances(),
		m_RemovableSoundEffects(),
		m_RemovableSoundInstances(),
		m_RemovableAudioListeners(),
		m_RemovableMusic(),
		m_RemovableMusicInstances()
	{
	}

	AudioDeviceLambda::~AudioDeviceLambda()
	{
		// Terminate portaudio
		PaError result = Pa_StopStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not stop stream, error: \"%s\"", Pa_GetErrorText(result));
		}
		
		result = Pa_Terminate();
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not terminate PortAudio, error: \"%s\"", Pa_GetErrorText(result));
		}

		// Cleanup		
		for (SoundInstance3DLambda* pSoundInstance : m_SoundInstances)
		{
			SAFERELEASE(pSoundInstance);
		}
		m_SoundInstances.clear();

		RemoveResources();

		AudioBuffer* pBuffer = m_pWriteAudioBuffer->pNext;
		while (pBuffer != m_pWriteAudioBuffer)
		{
			AudioBuffer* pDelete = pBuffer;
			pBuffer = pBuffer->pNext;

			SAFEDELETE(pDelete);
		}

		SAFEDELETE(m_pWriteAudioBuffer);
	}

	bool AudioDeviceLambda::Init(const AudioDeviceDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		// Init buffer
		m_pWriteAudioBuffer					= CreateAudioBuffer();
		m_pReadBuffer						= CreateAudioBuffer();
		m_pReadBuffer->pNext				= m_pWriteAudioBuffer;
		m_pWriteAudioBuffer->pNext			= CreateAudioBuffer();
		m_pWriteAudioBuffer->pNext->pNext	= m_pReadBuffer;

		// Init portaudio
		PaError result = Pa_Initialize();
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not initialize PortAudio, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		result = Pa_OpenDefaultStream(&m_pStream, 0, m_ChannelCount, paFloat32, m_SampleRate, paFramesPerBufferUnspecified, PortAudioCallback, this);
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not open PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		result = Pa_StartStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not start PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
		}

		m_Clock.Reset();

		return true;
	}

	void AudioDeviceLambda::PlayOnce(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundInstance3DLambda* pSoundInstance = DBG_NEW SoundInstance3DLambda(this, true);
		if (!pSoundInstance->Init(pDesc))
		{
			SAFERELEASE(pSoundInstance);
		}
		else
		{
			m_SoundInstances.emplace_back(pSoundInstance);
			pSoundInstance->Play();

			LOG_INFO("NumSoundInstances=%u", m_SoundInstances.size());
		}
	}

	void AudioDeviceLambda::RemoveSoundEffect3D(SoundEffect3DLambda* pSoundEffect)
	{
		m_RemovableSoundEffects.emplace_back(pSoundEffect);
	}

	void AudioDeviceLambda::RemoveSoundInstance3D(SoundInstance3DLambda* pSoundInstance)
	{
		m_RemovableSoundInstances.emplace_back(pSoundInstance);
	}

	void AudioDeviceLambda::RemoveAudioListener(AudioListenerLambda* pAudioListener)
	{
		m_RemovableAudioListeners.emplace_back(pAudioListener);
	}

	void AudioDeviceLambda::RemoveMusic(MusicLambda* pMusic)
	{
		m_RemovableMusic.emplace_back(pMusic);
	}

	void AudioDeviceLambda::RemoveMusicInstance(MusicInstanceLambda* pMusicInstance)
	{
		m_RemovableMusicInstances.emplace_back(pMusicInstance);
	}

	void AudioDeviceLambda::Tick()
	{
		RemoveResources();

		// Setup next free buffer
		AudioBuffer* pBuffer = m_pWriteAudioBuffer;
		if (m_pReadBuffer != pBuffer)
		{
			m_Clock.Tick();
			
			// Work on the next free buffer
			ProcessBuffer(pBuffer);

			// Get next buffer to process
			m_pWriteAudioBuffer = m_pWriteAudioBuffer->pNext;
			
			m_Clock.Tick();
			//LOG_INFO("AudioProcessing=%llu ns", m_Clock.GetDeltaTime().AsNanoSeconds());
		}
	}

	void AudioDeviceLambda::ProcessBuffer(AudioBuffer* pBuffer)
	{
		float32* pProcessedData = pBuffer->WaveForm;
		memset(pProcessedData, 0, sizeof(float32) * BUFFER_SAMPLES);

		const uint32 SAMPLES_PER_CHANNEL = BUFFER_SAMPLES / m_ChannelCount;
		for (AudioListenerLambda* pAudioListener : m_AudioListeners)
		{
			for (SoundInstance3DLambda* pSoundInstance : m_SoundInstances)
			{
				if (pSoundInstance->IsPlaying)
				{
					ProcessSoundInstance(pAudioListener, pSoundInstance, pProcessedData);
				}
			}
		}

		for (MusicInstanceLambda* pMusicInstance : m_MusicInstances)
		{
			if (pMusicInstance->IsPlaying)
			{
				ProcessMusicInstance(pMusicInstance, pProcessedData);
			}
		}
	}

	void AudioDeviceLambda::ProcessSoundInstance(AudioListenerLambda* pAudioListener, SoundInstance3DLambda* pSoundInstance, float32* pBuffer)
	{
		const uint32 SAMPLES_PER_CHANNEL = BUFFER_SAMPLES / m_ChannelCount;

		// Reset buffer index for each instance
		uint32 bufferIndex = 0;

        // Calculate panning
        glm::vec3 listPos		= pAudioListener->Desc.Position;
        glm::vec3 sourcePos		= pSoundInstance->Position;
        glm::vec3 forward		= glm::normalize(pAudioListener->Desc.Forward);
		glm::vec3 right			= glm::normalize(pAudioListener->Right);
        glm::vec3 sourceDir		= listPos - sourcePos;
        
        float32 sourceDirLen    = glm::length(sourceDir);
        float32 angle           = 0.0f; //Horizontal angle
        float32 dot             = 0.0f;
        if (sourceDirLen > 0)
        {
            sourceDir = sourceDir / sourceDirLen;
            
            dot     = glm::dot(right, sourceDir);
            angle   = glm::acos(dot) - glm::half_pi<float32>();
        }
        else
        {
            angle = 0.0f;
        }
        
        float32 theta1  = -glm::half_pi<float32>();     // Angle to left speaker
        float32 theta2  = glm::half_pi<float32>();      // Angle to right speaker
        float32 b       = (angle - theta1) / (theta2 - theta1);
        
		// Calculate attenutation
		float32 refDist		= pSoundInstance->ReferenceDistance;
		float32 distance	= glm::length(pAudioListener->Desc.Position - pSoundInstance->Position);
		distance			= glm::min(pSoundInstance->MaxDistance, glm::max(refDist, distance)) - refDist;
		float32 attenuation = refDist / (refDist + (pSoundInstance->RollOff * distance));

		// Calculate volume
		float32 volume	= pAudioListener->Desc.Volume * pSoundInstance->Volume;
		volume			= volume * attenuation;
        
        float32 volumeLeft  = glm::abs(glm::cos(glm::half_pi<float32>() * b) * volume);
        float32 volumeRight = glm::abs(glm::sin(glm::half_pi<float32>() * b) * volume);

        // LOG_INFO("right: x=%.3f y=%.3f z=%.3f sourceDir: x=%.3f y=%.3f z=%.3f, angle=%.3f, dot=%.3f, VL=%.3f, VR=%.3f", right.x, right.y, right.z, sourceDir.x, sourceDir.y, sourceDir.z, angle, dot, volumeLeft, volumeRight);
        
		// Add samples to buffer
		for (uint32_t i = 0; i < SAMPLES_PER_CHANNEL; i++)
		{
            float32 sample  = pSoundInstance->pWaveForm[pSoundInstance->CurrentBufferIndex];
            pBuffer[bufferIndex++] += sample * volumeLeft;
            pBuffer[bufferIndex++] += sample * volumeRight;

			pSoundInstance->CurrentBufferIndex++;
			if (pSoundInstance->CurrentBufferIndex >= pSoundInstance->TotalSampleCount)
			{
				if (pSoundInstance->PlayOnce)
				{
					SAFERELEASE(pSoundInstance);
					break;
				}
				else
				{
					if (pSoundInstance->Mode == ESoundMode::SOUND_MODE_LOOPING)
					{
						pSoundInstance->CurrentBufferIndex = 0;
					}
					else
					{
						pSoundInstance->Stop();
						break;
					}
				}
			}
		}
	}

	void AudioDeviceLambda::ProcessMusicInstance(MusicInstanceLambda* pMusicInstance, float32* pBuffer)
	{
		const uint32 SAMPLES_PER_CHANNEL = BUFFER_SAMPLES / m_ChannelCount;

		// Reset buffer index for each instance
		uint32 bufferIndex = 0;

		// Calculate volume
		float32 volume = pMusicInstance->Volume;

		// Add samples to buffer
		for (uint32_t i = 0; i < SAMPLES_PER_CHANNEL; i++)
		{
			for (uint32_t c = 0; c < m_ChannelCount; c++)
			{
				float32 sample = pMusicInstance->pWaveForm[pMusicInstance->CurrentBufferIndex];
				pBuffer[bufferIndex] += volume * sample;
				bufferIndex++;
			}

			pMusicInstance->CurrentBufferIndex++;
			if (pMusicInstance->CurrentBufferIndex >= pMusicInstance->TotalSampleCount)
			{
				pMusicInstance->CurrentBufferIndex = 0;
			}
		}
	}

	IMusic* AudioDeviceLambda::CreateMusic(const MusicDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		MusicLambda* pMusic = DBG_NEW MusicLambda(this);
		if (!pMusic->Init(pDesc))
		{
			SAFERELEASE(pMusic);
			return nullptr;
		}
		else
		{
			m_Music.emplace_back(pMusic);
			return pMusic;
		}
	}

	IMusicInstance* AudioDeviceLambda::CreateMusicInstance(const MusicInstanceDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		MusicInstanceLambda* pMusicInstance = DBG_NEW MusicInstanceLambda(this);
		if (!pMusicInstance->Init(pDesc))
		{
			SAFERELEASE(pMusicInstance);
			return nullptr;
		}
		else
		{
			m_MusicInstances.emplace_back(pMusicInstance);
			return pMusicInstance;
		}
	}

	IAudioListener* AudioDeviceLambda::CreateAudioListener(const AudioListenerDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		AudioListenerLambda* pListener = DBG_NEW AudioListenerLambda(this);
		if (!pListener->Init(pDesc))
		{
			SAFERELEASE(pListener);
			return nullptr;
		}
		else
		{
			m_AudioListeners.emplace_back(pListener);
			return pListener;
		}
	}

	ISoundEffect3D* AudioDeviceLambda::CreateSoundEffect(const SoundEffect3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundEffect3DLambda* pSoundEffect = DBG_NEW SoundEffect3DLambda(this);
		if (!pSoundEffect->Init(pDesc))
		{
			SAFERELEASE(pSoundEffect);
			return nullptr;
		}
		else
		{
			m_SoundEffects.emplace_back(pSoundEffect);
			return pSoundEffect;
		}
	}

	ISoundInstance3D* AudioDeviceLambda::CreateSoundInstance(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundInstance3DLambda* pSoundInstance = DBG_NEW SoundInstance3DLambda(this, false);
		if (!pSoundInstance->Init(pDesc))
		{
			SAFERELEASE(pSoundInstance);
			return nullptr;
		}
		else
		{
			m_SoundInstances.emplace_back(pSoundInstance);
			pSoundInstance->AddRef();

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

	void AudioDeviceLambda::SetMasterVolume(float32 volume)
	{
		m_MasterVolume = volume;
	}

	float AudioDeviceLambda::GetMasterVolume() const
	{
		return m_MasterVolume;
	}

	AudioBuffer* AudioDeviceLambda::CreateAudioBuffer()
	{
		static uint32_t numBuffers = 0;
		numBuffers++;

		D_LOG_INFO("[AudioDeviceLambda]: Created AudioBuffer. NumBuffers=%u", numBuffers);
		return DBG_NEW AudioBuffer();
	}

	void AudioDeviceLambda::RemoveResources()
	{
		// SoundEffects
		for (SoundEffect3DLambda* pSoundEffect : m_RemovableSoundEffects)
		{
			for (TArray<SoundEffect3DLambda*>::iterator it = m_SoundEffects.begin(); it != m_SoundEffects.end(); it++)
			{
				if ((*it) == pSoundEffect)
				{
					m_SoundEffects.erase(it);
					break;
				}
			}
		}
		m_RemovableSoundEffects.clear();

		// SoundInstances
		for (SoundInstance3DLambda* pSoundInstance: m_RemovableSoundInstances)
		{
			for (TArray<SoundInstance3DLambda*>::iterator it = m_SoundInstances.begin(); it != m_SoundInstances.end(); it++)
			{
				if ((*it) == pSoundInstance)
				{
					m_SoundInstances.erase(it);
					break;
				}
			}
		}
		m_RemovableSoundInstances.clear();

		// AudioListeners
		for (AudioListenerLambda* pAudioListener : m_RemovableAudioListeners)
		{
			for (TArray<AudioListenerLambda*>::iterator it = m_AudioListeners.begin(); it != m_AudioListeners.end(); it++)
			{
				if ((*it) == pAudioListener)
				{
					m_AudioListeners.erase(it);
					break;
				}
			}
		}
		m_RemovableAudioListeners.clear();

		// Music
		for (MusicLambda* pMusic : m_RemovableMusic)
		{
			for (TArray<MusicLambda*>::iterator it = m_Music.begin(); it != m_Music.end(); it++)
			{
				if ((*it) == pMusic)
				{
					m_Music.erase(it);
					break;
				}
			}
		}
		m_RemovableMusic.clear();

		// MusicInstances
		for (MusicInstanceLambda* pMusicInstance : m_RemovableMusicInstances)
		{
			for (TArray<MusicInstanceLambda*>::iterator it = m_MusicInstances.begin(); it != m_MusicInstances.end(); it++)
			{
				if ((*it) == pMusicInstance)
				{
					m_MusicInstances.erase(it);
					break;
				}
			}
		}
		m_RemovableMusicInstances.clear();
	}

	int32 AudioDeviceLambda::OutputAudioCallback(void* pOutputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags)
	{
		float32*		pOutput = (float32*)pOutputBuffer;
		AudioBuffer*	pBuffer = m_pReadBuffer;

		__m128 masterVolume = _mm_load1_ps(&m_MasterVolume);

		const uint32 totalSamples = framesPerBuffer * m_ChannelCount;		
		for (uint32 f = 0; f < totalSamples; f += 4)
		{
			__m128 samples = _mm_loadu_ps(pBuffer->WaveForm + pBuffer->BufferIndex);
			pBuffer->BufferIndex += 4;

			__m128 output = _mm_mul_ps(samples, masterVolume); 		
			_mm_storeu_ps(pOutput, output);

			pOutput += 4;

			// Get next buffer
			if (pBuffer->BufferIndex >= BUFFER_SAMPLES)
			{
				AudioBuffer* pNext = m_pReadBuffer->pNext;
				if (m_pWriteAudioBuffer != pNext)
				{
					pBuffer->BufferIndex = 0;
					
					m_pReadBuffer = pNext;
					pBuffer = m_pReadBuffer;
				}
				else
				{
					pBuffer->BufferIndex -= 4;

					float	zero	= 0.0f;
					__m128	zeros	= _mm_load1_ps(&zero);
					_mm_storeu_ps(pBuffer->WaveForm + pBuffer->BufferIndex, zeros);
				}
			}
		}

		return paNoError;
	}
	
	int32 AudioDeviceLambda::PortAudioCallback(const void* pInputBuffer, void* pOutputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags, void* pUserData)
	{
		AudioDeviceLambda* pDevice = reinterpret_cast<AudioDeviceLambda*>(pUserData);
		VALIDATE(pDevice != nullptr);

		return pDevice->OutputAudioCallback(pOutputBuffer, framesPerBuffer, pTimeInfo, statusFlags);
	}
}
