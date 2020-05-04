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
		PaError result;

		result = Pa_CloseStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[ManagedSoundInstance3DLambda]: Could not close PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
		}

		SAFEDELETE_ARRAY(m_pWaveForm);
		SAFEDELETE_ARRAY(m_pOutputVolumes);
	}

	bool ManagedSoundInstance3DLambda::Init(const ManagedSoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc);

		const SoundEffect3DLambda* pSoundEffect = reinterpret_cast<const SoundEffect3DLambda*>(pDesc->pSoundEffect);

		m_CurrentBufferIndex	= 0;
		m_SampleCount			= pSoundEffect->GetSampleCount();
		m_ChannelCount			= m_pAudioDevice->GetOutputChannelCount(); //For 3D Sounds, open up max Output Channels, assume WaveForm is mono
		m_pWaveForm				= DBG_NEW float32[m_SampleCount];
		memcpy(m_pWaveForm, pSoundEffect->GetWaveform(), sizeof(float32) * m_SampleCount);

		m_pOutputVolumes		= DBG_NEW float32[m_ChannelCount];

		PaError paResult;

		PaStreamParameters outputParameters = {};
		outputParameters.device						= m_pAudioDevice->GetDeviceIndex();
		outputParameters.channelCount				= m_ChannelCount;
		outputParameters.sampleFormat				= paFloat32;
		outputParameters.suggestedLatency			= m_pAudioDevice->GetDefaultLowOutputLatency();
		outputParameters.hostApiSpecificStreamInfo	= nullptr;

		PaStreamFlags streamFlags = paNoFlag;

		/* Open an audio I/O stream. */
		paResult = Pa_OpenStream(
			&m_pStream,
			nullptr,
			&outputParameters,
			pSoundEffect->GetSampleRate(),
			paFramesPerBufferUnspecified,
			streamFlags,
			PortAudioCallback,
			this);

		if (paResult != paNoError)
		{
			LOG_ERROR("[ManagedSoundInstance3DLambda]: Could not open PortAudio stream, error: \"%s\"", Pa_GetErrorText(paResult));
			return false;
		}

		paResult = Pa_StartStream(m_pStream);
		if (paResult != paNoError)
		{
			LOG_ERROR("[ManagedSoundInstance3DLambda]: Could not start PortAudio stream, error: \"%s\"", Pa_GetErrorText(paResult));
			return false;
		}

		return true;
	}

	void ManagedSoundInstance3DLambda::UpdateVolume(float masterVolume, const AudioListenerDesc* pAudioListeners, uint32 audioListenerCount, ESpeakerSetup speakerSetup)
	{
		//Todo: How to deal with multiple listeners?

		if (audioListenerCount > 1)
		{
			LOG_WARNING("[ManagedSoundInstance3DLambda]: Update3D called with multiple AudioListeners, this is currently not supported!");
			return;
		}

		float32 localVolume = masterVolume * m_Volume;

		for (uint32 i = 0; i < audioListenerCount; i++)
		{
			const AudioListenerDesc* pAudioListener = &pAudioListeners[i];

			glm::vec3 deltaPosition			= m_Position - pAudioListener->Position;
			float32 distance				= glm::length(deltaPosition);
			float32 clampedDistance			= glm::max(pAudioListener->AttenuationStartDistance, distance);
			glm::vec3 normalizedDeltaPos	= deltaPosition / clampedDistance;

			glm::vec2 forwardDir			= glm::normalize(glm::vec2(pAudioListener->Forward.x, pAudioListener->Forward.z));
			glm::vec2 soundDir				= distance > 0.0f ? glm::normalize(glm::vec2(normalizedDeltaPos.x, normalizedDeltaPos.z)) : forwardDir;
			
			float32 soundAngle				= glm::atan(soundDir.y, soundDir.x);
			float32 rightAngle				= glm::atan(pAudioListener->Right.z, pAudioListener->Right.x);
			float32 theta					= soundAngle - rightAngle;

			float32 attenuation				= pAudioListener->AttenuationStartDistance / (pAudioListener->AttenuationStartDistance + pAudioListener->AttenuationRollOffFactor * (clampedDistance - pAudioListener->AttenuationStartDistance));
			float32 globalVolume			= attenuation * pAudioListener->Volume * localVolume;

			switch (speakerSetup)
			{
				case ESpeakerSetup::STEREO_SOUND_SYSTEM:
				{
					float leftSpeakerAngle	= 3.0f * glm::quarter_pi<float>();
					float rightSpeakerAngle = glm::quarter_pi<float>();

					float panning = (theta - leftSpeakerAngle) / (rightSpeakerAngle - leftSpeakerAngle);

					m_pOutputVolumes[0] = globalVolume * glm::abs(sin(glm::half_pi<float>() * panning));
					m_pOutputVolumes[1] = globalVolume * glm::abs(cos(glm::half_pi<float>() * panning));

					break;
				}
				case ESpeakerSetup::STEREO_HEADPHONES:
				{
					float leftSpeakerAngle = glm::pi<float>();
					float rightSpeakerAngle = 0.0f;

					float panning = (theta - rightSpeakerAngle) / (leftSpeakerAngle - rightSpeakerAngle);

					m_pOutputVolumes[0] = globalVolume * glm::abs(sin(glm::half_pi<float>() * panning));
					m_pOutputVolumes[1] = globalVolume * glm::abs(cos(glm::half_pi<float>() * panning));

					break;
				}
				case ESpeakerSetup::SURROUND_5_1:
				case ESpeakerSetup::SURROUND_7_1:
				default:
				{
					LOG_ERROR("[ManagedSoundInstance3DLambda]: Speaker setup not implemented!");

					for (uint32 c = 0; c < m_ChannelCount; c++)
					{
						m_pOutputVolumes[c] = globalVolume;
					}

					break;
				}
			}
		}
	}

	int32 ManagedSoundInstance3DLambda::LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer)
	{
		for (uint32 f = 0; f < framesPerBuffer; f++)
		{
			float sample = m_pWaveForm[m_CurrentBufferIndex++];

			for (uint32 c = 0; c < m_ChannelCount; c++)
			{
				(*(pOutputBuffer++)) = m_pOutputVolumes[c] * sample;
			}

			if (m_CurrentBufferIndex == m_SampleCount)
			{
				m_CurrentBufferIndex = 0;
				m_Completed = true;
				return paComplete;
			}
		}

		return paNoError;
	}


	int32 ManagedSoundInstance3DLambda::PortAudioCallback(const void* pInputBuffer, void* pOutputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags, void* pUserData)
	{
		UNREFERENCED_VARIABLE(pInputBuffer);
		UNREFERENCED_VARIABLE(pTimeInfo);
		UNREFERENCED_VARIABLE(statusFlags);

		ManagedSoundInstance3DLambda* pInstance = reinterpret_cast<ManagedSoundInstance3DLambda*>(pUserData);
		VALIDATE(pInstance != nullptr);

		return pInstance->LocalAudioCallback(reinterpret_cast<float32*>(pOutputBuffer), framesPerBuffer);
	}
}