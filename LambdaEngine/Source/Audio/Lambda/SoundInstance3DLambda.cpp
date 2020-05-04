#include "Audio/Lambda/SoundInstance3DLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundInstance3DLambda::SoundInstance3DLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	SoundInstance3DLambda::~SoundInstance3DLambda()
	{
		m_pAudioDevice->DeleteSoundInstance(this);
	
		SAFEDELETE_ARRAY(m_pOutputVolumes);
	}

	bool SoundInstance3DLambda::Init(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc);

		const SoundEffect3DLambda* pSoundEffect = reinterpret_cast<const SoundEffect3DLambda*>(pDesc->pSoundEffect);

		m_pSoundEffect			= pSoundEffect;
		m_CurrentWaveFormIndex	= 0;
		m_SampleCount			= pSoundEffect->GetResampledSampleCount();
		m_ChannelCount			= m_pAudioDevice->GetOutputChannelCount(); //For 3D Sounds, open up max Output Channels, assume WaveForm is mono
		
		m_pOutputVolumes		= DBG_NEW float64[m_ChannelCount];

		if (pDesc->Flags & FSoundModeFlags::SOUND_MODE_LOOPING)
		{
			m_Looping = true;
		}

		m_Playing = true;

		return true;
	}

	void SoundInstance3DLambda::Play()
	{
		m_Playing = true;
	}

	void SoundInstance3DLambda::Pause()
	{
		m_Playing = false;
	}

	void SoundInstance3DLambda::Stop()
	{
		m_Playing = false;
		m_CurrentWaveFormIndex = 0;
	}

	void SoundInstance3DLambda::Toggle()
	{
		if (m_Playing)
		{
			Pause();
		}
		else
		{
			Play();
		}
	}

	void SoundInstance3DLambda::SetPosition(const glm::vec3& position)
	{
		m_Position = position;
	}

	void SoundInstance3DLambda::SetVolume(float64 volume)
	{
		m_Volume = glm::clamp(volume, -1.0, 1.0);
	}

	void SoundInstance3DLambda::SetPitch(float32 pitch)
	{
		UNREFERENCED_VARIABLE(pitch);
	}

	void SoundInstance3DLambda::UpdateVolume(float masterVolume, const AudioListenerDesc* pAudioListeners, uint32 count, ESpeakerSetup speakerSetup)
	{
		//Todo: How to deal with multiple listeners?

		if (count > 1)
		{
			LOG_WARNING("[SoundInstance3DLambda]: Update3D called with multiple AudioListeners, this is currently not supported!");
			return;
		}

		float localVolume = masterVolume * m_Volume;

		for (uint32 i = 0; i < count; i++)
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
			float64 globalVolume			= (float64)attenuation * pAudioListener->Volume * localVolume;

			switch (speakerSetup)
			{
				case ESpeakerSetup::STEREO_SOUND_SYSTEM:
				{
					float leftSpeakerAngle	= 3.0f * glm::quarter_pi<float>();
					float rightSpeakerAngle = glm::quarter_pi<float>();

					float64 panning = (float64)(theta - rightSpeakerAngle) / (leftSpeakerAngle - rightSpeakerAngle);

					m_pOutputVolumes[0] = globalVolume * glm::abs(sin(glm::half_pi<double>() * panning));
					m_pOutputVolumes[1] = globalVolume * glm::abs(cos(glm::half_pi<double>() * panning));

					break;
				}
				case ESpeakerSetup::STEREO_HEADPHONES:
				{
					float leftSpeakerAngle = glm::pi<float>();
					float rightSpeakerAngle = 0.0f;

					float64 panning = (float64)(theta - rightSpeakerAngle) / (leftSpeakerAngle - rightSpeakerAngle);

					m_pOutputVolumes[0] = globalVolume * glm::abs(sin(glm::half_pi<double>() * panning));
					m_pOutputVolumes[1] = globalVolume * glm::abs(cos(glm::half_pi<double>() * panning));

					break;
				}
				case ESpeakerSetup::SURROUND_5_1:
				case ESpeakerSetup::SURROUND_7_1:
				default:
				{
					LOG_ERROR("[ManagedSoundInstance3DLambda]: Speaker setup not implemented!");

					for (uint32 c = 0; c < m_ChannelCount; c++)
					{
						m_pOutputVolumes[c] = (float64)globalVolume;
					}

					break;
				}
			}
		}
	}

	void SoundInstance3DLambda::AddToBuffer(double** ppOutputChannels, uint32 channelCount, uint32 outputSampleCount)
	{
		if (m_Playing)
		{
			const float64* pResampledWaveForm = m_pSoundEffect->GetWaveform();

			for (uint32 s = 0; s < outputSampleCount; s++)
			{
				for (uint32 c = 0; c < channelCount; c++)
				{
					double* pOutputChannel = ppOutputChannels[c];
					pOutputChannel[s] += m_pOutputVolumes[c] * pResampledWaveForm[m_CurrentWaveFormIndex];
				}

				m_CurrentWaveFormIndex++;
				if (m_CurrentWaveFormIndex == m_SampleCount)
				{
					m_CurrentWaveFormIndex = 0;
					
					if (!m_Looping)
					{
						m_Playing = false;
						return;
					}
				}
			}
		}
	}
}