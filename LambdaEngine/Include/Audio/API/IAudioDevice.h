#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct MusicDesc;
	struct AudioGeometryDesc;
	struct ReverbSphereDesc;
	struct SoundEffect3DDesc;
	struct SoundInstance3DDesc;
	struct AudioListenerDesc;
	struct MusicInstanceDesc;

	class IMusic;
	class ISoundEffect3D;
	class ISoundInstance3D;
	class IAudioGeometry;
	class IReverbSphere;
	class IAudioListener;
	class IMusicInstance;

	enum class EAudioAPI
	{
		FMOD,
		LAMBDA
	};
	
	struct AudioDeviceDesc
	{
		const char*		pName					= "";
		float32			MasterVolume			= 1.0f;
		bool			Debug					= true;
		uint32			MaxNumAudioListeners	= 1;
		float32			MaxWorldSize			= 100.0f;
	};
	
	class IAudioDevice
	{
	public:
		DECL_INTERFACE(IAudioDevice);
		
		/*
		* Tick the audio device
		*/
		virtual void Tick() = 0;

		virtual IMusic*				CreateMusic(const MusicDesc* pDesc)						= 0;
		virtual IMusicInstance*		CreateMusicInstance(const MusicInstanceDesc* pDesc)		= 0;
		virtual IAudioListener*		CreateAudioListener(const AudioListenerDesc* pDesc)		= 0;
		virtual ISoundEffect3D*		CreateSoundEffect(const SoundEffect3DDesc* pDesc)		= 0;
		virtual ISoundInstance3D*	CreateSoundInstance(const SoundInstance3DDesc* pDesc)	= 0;
		virtual IAudioGeometry*		CreateAudioGeometry(const AudioGeometryDesc* pDesc)		= 0;
		virtual IReverbSphere*		CreateReverbSphere(const ReverbSphereDesc* pDesc)		= 0;

		virtual void SetMasterVolume(float32 volume) = 0;

		virtual float GetMasterVolume() const = 0;
	};

	LAMBDA_API IAudioDevice* CreateAudioDevice(EAudioAPI api, const AudioDeviceDesc& desc);
}
