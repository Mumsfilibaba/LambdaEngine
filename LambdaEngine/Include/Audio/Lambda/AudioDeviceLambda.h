#pragma once

#include "PortAudio.h"
#include "Audio/API/IAudioDevice.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include <map>

namespace LambdaEngine
{
	struct ManagedSoundInstance3DDesc;

	class SoundEffect3DLambda;
	class SoundInstance3DLambda;
	class ManagedSoundInstance3DLambda;

	class AudioDeviceLambda : public IAudioDevice
	{		
		struct MusicLambda
		{
			PaStream* pStream			= nullptr;

			float32* pWaveForm			= nullptr;
			uint32 SampleCount			= 0;
			uint32 CurrentBufferIndex	= 0;
			uint32 ChannelCount			= 0;
			uint32 TotalSampleCount		= 0;

			float Volume				= 1.0f;
			float OutputVolume			= 1.0f;

			bool Playing				= false;
		};

	public:
		AudioDeviceLambda();
		~AudioDeviceLambda();

		virtual bool Init(const AudioDeviceDesc* pDesc) override final;

		virtual void Tick() override final;
			 
		virtual void UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc) override final;

		virtual uint32				CreateAudioListener()									override final;
		virtual IMusic*				CreateMusic(const MusicDesc* pDesc)						override final;
		virtual ISoundEffect3D*		CreateSoundEffect(const SoundEffect3DDesc* pDesc)		override final;
		virtual ISoundInstance3D*	CreateSoundInstance(const SoundInstance3DDesc* pDesc)	override final;
		virtual IAudioGeometry*		CreateAudioGeometry(const AudioGeometryDesc* pDesc)		override final;
		virtual IReverbSphere*		CreateReverbSphere(const ReverbSphereDesc* pDesc)		override final;

		void AddManagedSoundInstance(const ManagedSoundInstance3DDesc* pDesc) const;

		void DeleteSoundEffect(SoundEffect3DLambda* pSoundEffect) const;
		void DeleteSoundInstance(SoundInstance3DLambda* pSoundInstance) const;

		virtual void SetMasterVolume(float volume) override final;

		FORCEINLINE virtual float GetMasterVolume() const override final { return m_MasterVolume; }

	private:
		int32 LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer);

	private:
		static int32 PortAudioCallback(
			const void* pInputBuffer,
			void* pOutputBuffer,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* pTimeInfo,
			PaStreamCallbackFlags statusFlags,
			void* pUserData);

	private:
		const char* m_pName;

		float	m_MasterVolume			= 1.0f;

		uint32	m_MaxNumAudioListeners	= 0;
		uint32	m_NumAudioListeners		= 0;

		MusicLambda m_Music;

		THashTable<uint32, uint32>	m_AudioListenerMap;
		TArray<AudioListenerDesc>	m_AudioListeners;

		std::set<SoundEffect3DLambda*>			m_SoundEffects;
		std::set<SoundInstance3DLambda*>		m_SoundInstances;

		mutable std::set<SoundEffect3DLambda*>		m_SoundEffectsToDelete;
		mutable std::set<SoundInstance3DLambda*>	m_SoundInstancesToDelete;

		mutable std::multimap<float64, ManagedSoundInstance3DLambda*>		m_ManagedInstances;
	};
}