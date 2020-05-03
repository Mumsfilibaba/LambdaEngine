#include "Audio/Lambda/ManagedSoundInstance3DLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ManagedSoundInstance3DLambda::ManagedSoundInstance3DLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	ManagedSoundInstance3DLambda::~ManagedSoundInstance3DLambda()
	{
		SAFEDELETE_ARRAY(m_pWaveForm);

		PaError result;

		result = Pa_CloseStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[ManagedSoundInstance3DLambda]: Could not close PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
		}
	}

	bool ManagedSoundInstance3DLambda::Init(const ManagedSoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc);

		const SoundEffect3DLambda* pSoundEffect = reinterpret_cast<const SoundEffect3DLambda*>(pDesc->pSoundEffect);

		m_CurrentBufferIndex	= 0;
		m_SampleCount			= pSoundEffect->GetSampleCount();
		m_ChannelCount			= pSoundEffect->GetChannelCount();
		m_TotalSampleCount		= m_SampleCount * m_ChannelCount;
		m_pWaveForm				= DBG_NEW float32[m_TotalSampleCount];
		memcpy(m_pWaveForm, pSoundEffect->GetWaveform(), sizeof(float32) * m_TotalSampleCount);

		PaError result;

		/* Open an audio I/O stream. */
		result = Pa_OpenDefaultStream(
			&m_pStream,
			0,          /* no input channels */
			m_ChannelCount,          /* stereo output */
			paFloat32,  /* 32 bit floating point output */
			pSoundEffect->GetSampleRate(),
			paFramesPerBufferUnspecified,	/* frames per buffer, i.e. the number
							   of sample frames that PortAudio will
							   request from the callback. Many apps
							   may want to use
							   paFramesPerBufferUnspecified, which
							   tells PortAudio to pick the best,
							   possibly changing, buffer size.*/
			PortAudioCallback, /* this is your callback function */
			this); /*This is a pointer that will be passed to your callback*/

		if (result != paNoError)
		{
			LOG_ERROR("[ManagedSoundInstance3DLambda]: Could not open PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		result = Pa_StartStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[ManagedSoundInstance3DLambda]: Could not start PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		return true;
	}

	void ManagedSoundInstance3DLambda::UpdateVolume(float masterVolume, const AudioListenerDesc* pAudioListeners, uint32 count)
	{
		//Todo: How to deal with multiple listeners?

		if (count > 1)
		{
			LOG_WARNING("[ManagedSoundInstance3DLambda]: Update3D called with multiple AudioListeners, this is currently not supported!");
			return;
		}

		float localVolume = masterVolume * m_Volume;

		m_OutputVolume = 0.0f;

		for (uint32 i = 0; i < count; i++)
		{
			const AudioListenerDesc* pAudioListener = &pAudioListeners[i];

			float distance = glm::max(pAudioListener->AttenuationStartDistance, glm::distance(pAudioListener->Position, m_Position));
			float attenuation = pAudioListener->AttenuationStartDistance / (pAudioListener->AttenuationStartDistance + pAudioListener->AttenuationRollOffFactor * (distance - pAudioListener->AttenuationStartDistance));

			float globalVolume = localVolume * pAudioListener->Volume;

			m_OutputVolume += globalVolume * attenuation;
		}
	}

	int32 ManagedSoundInstance3DLambda::LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer)
	{
		for (uint32 f = 0; f < framesPerBuffer; f++)
		{
			for (uint32 c = 0; c < m_ChannelCount; c++)
			{
				float sample = m_pWaveForm[m_CurrentBufferIndex++];
				(*(pOutputBuffer++)) = m_OutputVolume * sample;
			}

			if (m_CurrentBufferIndex == m_TotalSampleCount)
				m_CurrentBufferIndex = 0;
		}

		return paNoError;
	}

	int32 ManagedSoundInstance3DLambda::PortAudioCallback(
		const void* pInputBuffer,
		void* pOutputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* pTimeInfo,
		PaStreamCallbackFlags statusFlags,
		void* pUserData)
	{
		UNREFERENCED_VARIABLE(pInputBuffer);
		UNREFERENCED_VARIABLE(pTimeInfo);
		UNREFERENCED_VARIABLE(statusFlags);

		ManagedSoundInstance3DLambda* pInstance = reinterpret_cast<ManagedSoundInstance3DLambda*>(pUserData);
		VALIDATE(pInstance != nullptr);

		return pInstance->LocalAudioCallback(reinterpret_cast<float32*>(pOutputBuffer), framesPerBuffer);
	}
}