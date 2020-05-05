#include "Log/Log.h"

#include "PortAudio.h"

#include "Audio/Lambda/AudioDeviceLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/SoundInstance3DLambda.h"
#include "Audio/Lambda/MusicLambda.h"
#include "Audio/Lambda/AudioListenerLambda.h"

#include <immintrin.h>

namespace LambdaEngine
{	
	AudioDeviceLambda::AudioDeviceLambda()
	{
	}

	AudioDeviceLambda::~AudioDeviceLambda()
	{
		PaError result = Pa_Terminate();
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not terminate PortAudio, error: \"%s\"", Pa_GetErrorText(result));
		}
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

		return true;
	}

	void AudioDeviceLambda::Tick()
	{
		// Setup next free buffer
		AudioBuffer* pBuffer = m_pWriteAudioBuffer;
		if (m_pReadBuffer != pBuffer)
		{
			// Work on the next free buffer
			ProcessBuffer(pBuffer);

			// Get next buffer to process
			m_pWriteAudioBuffer = m_pWriteAudioBuffer->pNext;
		}
	}

	void AudioDeviceLambda::ProcessBuffer(AudioBuffer* pBuffer)
	{
		float32* pProcessedData = pBuffer->WaveForm;
		memset(pProcessedData, 0, sizeof(float32) * BUFFER_SAMPLES);

		uint32			bufferIndex			= 0;
		const uint32	SAMPLES_PER_CHANNEL	= BUFFER_SAMPLES / m_ChannelCount;
		for (AudioListenerLambda* pListener : m_AudioListeners)
		{
			for (SoundInstance3DLambda* pInstance : m_SoundInstances)
			{
				if (pInstance->IsPlaying)
				{
					// Calculate volume
					float32 volume		= pListener->Desc.Volume * pInstance->Volume;
					float32 distance	= glm::length(pListener->Desc.Position - pInstance->Position);
					distance			= glm::max(distance - pInstance->ReferenceDistance, 0.0f);
					float32 maxDistance = glm::max(pInstance->MaxDistance - pInstance->ReferenceDistance, 0.01f);
					
					float32 attenuation = glm::max(1.0f - (distance / maxDistance), 0.0f);
					volume				= volume * attenuation;

					// Add samples to buffer
					for (uint32_t i = 0; i < SAMPLES_PER_CHANNEL; i++)
					{
						for (uint32_t c = 0; c < m_ChannelCount; c++)
						{
							float32 sample = pInstance->pWaveForm[pInstance->CurrentBufferIndex];
							pProcessedData[bufferIndex] += volume * sample;
							bufferIndex++;
						}

						pInstance->CurrentBufferIndex++;
						if (pInstance->CurrentBufferIndex == pInstance->TotalSampleCount)
						{
							pInstance->CurrentBufferIndex = 0;
						}
					}
				}
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

		SoundInstance3DLambda* pSoundInstance = DBG_NEW SoundInstance3DLambda(this);
		if (!pSoundInstance->Init(pDesc))
		{
			SAFERELEASE(pSoundInstance);
			return nullptr;
		}
		else
		{
			m_SoundInstances.emplace_back(pSoundInstance);
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

	void AudioDeviceLambda::SetMasterVolume(float volume)
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
