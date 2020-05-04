#include "Audio/Lambda/MusicLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

#include <WavLib.h>

namespace LambdaEngine
{
	MusicLambda::MusicLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	MusicLambda::~MusicLambda()
	{
		m_pAudioDevice->DeleteMusicInstance(this);

		PaError result;

		result = Pa_CloseStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[MusicLambda]: Could not close PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
		}

		SAFEDELETE_ARRAY(m_pWaveForm);
	}

	bool MusicLambda::Init(const MusicDesc* pDesc)
	{
		VALIDATE(pDesc);

		WaveFile waveHeader = {};

		int32 result = WavLibLoadFileFloat(pDesc->pFilepath, &m_pWaveForm, &waveHeader, WAV_LIB_FLAG_NONE);
		if (result != WAVE_SUCCESS)
		{
			const char* pError = WavLibGetError(result);

			LOG_ERROR("[MusicLambda]: Failed to load file '%s'. Error: %s", pDesc->pFilepath, pError);
			return false;
		}

		PaError paResult;

		m_CurrentBufferIndex	= 0;
		m_SampleCount			= waveHeader.SampleCount;
		m_ChannelCount			= waveHeader.ChannelCount; //For Music, open up as many channels as the music has, assume device can support it, Todo: What if device can't?
		m_TotalSampleCount		= m_SampleCount * m_ChannelCount;


		PaStreamParameters outputParameters = {};
		outputParameters.device						= m_pAudioDevice->GetDeviceIndex();
		outputParameters.channelCount				= m_ChannelCount;
		outputParameters.sampleFormat				= paFloat32;
		outputParameters.suggestedLatency			= m_pAudioDevice->GetDefaultHighOutputLatency();
		outputParameters.hostApiSpecificStreamInfo	= nullptr;

		PaStreamFlags streamFlags = paNoFlag;

		/* Open an audio I/O stream. */
		paResult = Pa_OpenStream(
			&m_pStream,
			nullptr,
			&outputParameters,
			waveHeader.SampleRate,
			paFramesPerBufferUnspecified,
			streamFlags,
			PortAudioCallback,
			this);

		if (paResult != paNoError)
		{
			LOG_ERROR("[MusicLambda]: Could not open PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		paResult = Pa_StartStream(m_pStream);
		if (paResult != paNoError)
		{
			LOG_ERROR("[MusicLambda]: Could not start PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		m_Playing = true;

		return true;
	}

	void MusicLambda::Play()
	{
		if (m_pStream == nullptr)
			return;

		if (!m_Playing)
		{
			PaError paResult;
			paResult = Pa_StartStream(m_pStream);
			if (paResult != paNoError)
			{
				LOG_ERROR("[SoundInstance3DLambda]: Could not start PortAudio stream, error: \"%s\"", Pa_GetErrorText(paResult));
			}

			m_Playing = true;
		}
	}

	void MusicLambda::Pause()
	{
		if (m_pStream == nullptr)
			return;

		if (m_Playing)
		{
			PaError paResult;
			paResult = Pa_StopStream(m_pStream);
			if (paResult != paNoError)
			{
				LOG_ERROR("[SoundInstance3DLambda]: Could not pause PortAudio stream, error: \"%s\"", Pa_GetErrorText(paResult));
			}

			m_Playing = false;
		}
	}

	void MusicLambda::Toggle()
	{
		if (m_pStream == nullptr)
			return;

		if (m_Playing)
		{
			Pause();
		}
		else
		{
			Play();
		}
	}

	void MusicLambda::SetVolume(float volume)
	{
		m_Volume = volume;
	}

	void MusicLambda::SetPitch(float pitch)
	{
		UNREFERENCED_VARIABLE(pitch);
	}

	void MusicLambda::UpdateVolume(float masterVolume)
	{
		m_OutputVolume = masterVolume * m_Volume;
	}

	int32 MusicLambda::LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer)
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

	int32 MusicLambda::PortAudioCallback(const void* pInputBuffer, void* pOutputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags, void* pUserData)
	{
		UNREFERENCED_VARIABLE(pInputBuffer);
		UNREFERENCED_VARIABLE(pTimeInfo);
		UNREFERENCED_VARIABLE(statusFlags);

		MusicLambda* pMusic = reinterpret_cast<MusicLambda*>(pUserData);
		VALIDATE(pMusic != nullptr);

		return pMusic->LocalAudioCallback(reinterpret_cast<float32*>(pOutputBuffer), framesPerBuffer);
	}
}