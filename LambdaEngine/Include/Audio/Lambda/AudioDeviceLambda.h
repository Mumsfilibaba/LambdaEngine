#pragma once
#include "Audio/API/IAudioDevice.h"

#include "Containers/TArray.h"

#include "Time/API/Clock.h"

#include "PortAudio.h"
#include "SoundInstance3DLambda.h"

namespace LambdaEngine
{
	class SoundEffect3DLambda;
	class SoundInstance3DLambda;
	class AudioListenerLambda;
	class MusicLambda;
	class MusicInstanceLambda;

#define BUFFER_SAMPLES (1 << 11)

	struct AudioBuffer
	{
		AudioBuffer*	pNext		= nullptr;
		uint32			BufferIndex = 0;

		float32	WaveForm[BUFFER_SAMPLES];
	};

	class AudioDeviceLambda : public IAudioDevice
	{		
	public:
		AudioDeviceLambda();
		~AudioDeviceLambda();

		bool Init(const AudioDeviceDesc* pDesc);
		
		void PlayOnce(const SoundInstance3DDesc* pDesc);

		void RemoveSoundEffect3D(SoundEffect3DLambda* pSoundEffect);
		void RemoveSoundInstance3D(SoundInstance3DLambda* pSoundInstance);
		void RemoveAudioListener(AudioListenerLambda* pAudioListener);
		void RemoveMusic(MusicLambda* pMusic);
		void RemoveMusicInstance(MusicInstanceLambda* pMusicInstance);

		// IAudioDevice interface
		virtual void Tick() override final;
			 
		virtual IMusic*				CreateMusic(const MusicDesc* pDesc)						override final;
		virtual IMusicInstance*		CreateMusicInstance(const MusicInstanceDesc* pDesc)		override final;
		virtual IAudioListener*		CreateAudioListener(const AudioListenerDesc* pDesc)		override final;
		virtual ISoundEffect3D*		CreateSoundEffect(const SoundEffect3DDesc* pDesc)		override final;
		virtual ISoundInstance3D*	CreateSoundInstance(const SoundInstance3DDesc* pDesc)	override final;
		virtual IAudioGeometry*		CreateAudioGeometry(const AudioGeometryDesc* pDesc)		override final;
		virtual IReverbSphere*		CreateReverbSphere(const ReverbSphereDesc* pDesc)		override final;

		virtual void SetMasterVolume(float32 volume) override final;

		virtual float GetMasterVolume() const override final;

	private:
		AudioBuffer*	CreateAudioBuffer();

		void ProcessBuffer(AudioBuffer* pBuffer);
		void ProcessSoundInstance(AudioListenerLambda* pAudioListener, SoundInstance3DLambda* pSoundInstance, float32* pBuffer);
		void ProcessMusicInstance(MusicInstanceLambda* pMusicInstance, float32* pBuffer);

		void RemoveResources();

		// Callback into portaudio
		int32 OutputAudioCallback(void* pOutputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags);

	private:
		/*
		*	This routine will be called by the PortAudio engine when audio is needed.
		*	It may called at interrupt level on some machines so avoid calling kernel 
		*	mode functions like calling malloc() or free().
		*/ 
		static int32 PortAudioCallback(const void* pInputBuffer, void* pOutputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags, void* pUserData);

	private:
		PaStream*		m_pStream				= nullptr;
		AudioBuffer*	m_pReadBuffer			= nullptr;
		AudioBuffer*	m_pWriteAudioBuffer		= nullptr;

		Clock m_Clock;

		float32 m_MasterVolume = 0.5f;

		const uint32 m_ChannelCount		= 2;
		const uint32 m_SampleRate		= 44100;

		TArray<SoundEffect3DLambda*>		m_SoundEffects;
		TArray<SoundInstance3DLambda*>		m_SoundInstances;
		TArray<AudioListenerLambda*>		m_AudioListeners;
		TArray<MusicLambda*>				m_Music;
		TArray<MusicInstanceLambda*>		m_MusicInstances;

		TArray<SoundEffect3DLambda*>		m_RemovableSoundEffects;
		TArray<SoundInstance3DLambda*>		m_RemovableSoundInstances;
		TArray<AudioListenerLambda*>		m_RemovableAudioListeners;
		TArray<MusicLambda*>				m_RemovableMusic;
		TArray<MusicInstanceLambda*>		m_RemovableMusicInstances;
	};
}