#pragma once

#include "PortAudio.h"
#include "Audio/API/IAudioDevice.h"
#include "Audio/API/AudioTypes.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include <map>

namespace LambdaEngine
{
	struct ManagedSoundInstance3DDesc;

	class MusicLambda;
	class SoundEffect3DLambda;
	class SoundInstance3DLambda;
	class ManagedSoundInstance3DLambda;

	class AudioDeviceLambda : public IAudioDevice
	{		
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

		virtual void SetMasterVolume(float volume) override final;

		FORCEINLINE virtual float GetMasterVolume() const override final { return m_MasterVolume; }

		void AddManagedSoundInstance(const ManagedSoundInstance3DDesc* pDesc) const;

		void DeleteMusicInstance(MusicLambda* pMusic) const;
		void DeleteSoundEffect(SoundEffect3DLambda* pSoundEffect) const;
		void DeleteSoundInstance(SoundInstance3DLambda* pSoundInstance) const;

		FORCEINLINE PaDeviceIndex	GetDeviceIndex()				const { return m_DeviceIndex;				}
		FORCEINLINE int				GetOutputChannelCount()			const { return m_OutputChannelCount;		}
		FORCEINLINE PaTime			GetDefaultLowOutputLatency()	const { return m_DefaultLowOutputLatency;	}
		FORCEINLINE PaTime			GetDefaultHighOutputLatency()	const { return m_DefaultHighOutputLatency;	}

	private:
		const char*		m_pName;

		PaDeviceIndex	m_DeviceIndex;
		ESpeakerSetup	m_SpeakerSetup;
		int				m_OutputChannelCount;
		PaTime			m_DefaultLowOutputLatency;
		PaTime			m_DefaultHighOutputLatency;

		float32			m_MasterVolume			= 1.0f;

		uint32			m_MaxNumAudioListeners	= 0;
		uint32			m_NumAudioListeners		= 0;

		THashTable<uint32, uint32>	m_AudioListenerMap;
		TArray<AudioListenerDesc>	m_AudioListeners;

		std::set<MusicLambda*>					m_MusicInstances;
		std::set<SoundEffect3DLambda*>			m_SoundEffects;
		std::set<SoundInstance3DLambda*>		m_SoundInstances;

		mutable std::set<MusicLambda*>				m_MusicInstancesToDelete;
		mutable std::set<SoundEffect3DLambda*>		m_SoundEffectsToDelete;
		mutable std::set<SoundInstance3DLambda*>	m_SoundInstancesToDelete;

		mutable std::multimap<float64, ManagedSoundInstance3DLambda*>		m_ManagedInstances;
	};
}