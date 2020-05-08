#pragma once

#include "LambdaEngine.h"
#include "AudioTypes.h"
#include "Time/API/Timestamp.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct MusicDesc;
	struct AudioGeometryDesc;
	struct ReverbSphereDesc;
	struct SoundEffect3DDesc;
	struct SoundInstance3DDesc;
	struct FIRFilterDesc;
	struct IIRFilterDesc;
	struct AddFilterDesc;
	struct MulFilterDesc;
	struct CombFilterDesc;
	struct AllPassFilterDesc;
	struct FilterSystemDesc;

	class IMusic;
	class ISoundEffect3D;
	class ISoundInstance3D;
	class IAudioGeometry;
	class IReverbSphere;
	class IAudioFilter;
	
	enum class EAudioAPI
	{
		FMOD,
		LAMBDA
	};
	
	struct AudioDeviceDesc
	{
		const char*		pName					= "Audio Device";
		bool			Debug					= true;
		ESpeakerSetup	SpeakerSetup			= ESpeakerSetup::NONE;
		float64			MasterVolume			= 1.0f;
		uint32			MaxNumAudioListeners	= 1;
		float			MaxWorldSize			= 100.0f;
	};
	
	struct AudioListenerDesc
	{
		glm::vec3	Position					= glm::vec3(0.0f);
		glm::vec3	Forward						= glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3	Right						= glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3	Up							= glm::vec3(0.0f, 1.0f, 0.0f);
		float64		Volume						= 1.0f;
		float		AttenuationRollOffFactor	= 1.0f;
		float		AttenuationStartDistance	= 1.0f;
	};

	class IAudioDevice
	{
	public:
		DECL_INTERFACE(IAudioDevice);
		
		/*
		* Initialize this AudioDeviceFMOD
		*	pDesc - A description of initialization parameters
		* return - true if the initialization was successful, otherwise returns false
		*/
		virtual bool Init(const AudioDeviceDesc* pDesc) = 0;

		/*
		* Tick the audio device
		*/
		virtual void FixedTick(Timestamp delta) = 0;

		virtual void UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc) = 0;

		virtual uint32				CreateAudioListener()										= 0;
		virtual IMusic*				CreateMusic(const MusicDesc* pDesc)							= 0;
		virtual ISoundEffect3D*		CreateSoundEffect(const SoundEffect3DDesc* pDesc)			= 0;
		virtual ISoundInstance3D*	CreateSoundInstance(const SoundInstance3DDesc* pDesc)		= 0;
		virtual IAudioGeometry*		CreateAudioGeometry(const AudioGeometryDesc* pDesc)			= 0;
		virtual IReverbSphere*		CreateReverbSphere(const ReverbSphereDesc* pDesc)			= 0;

		virtual IAudioFilter*		CreateFIRFilter(const FIRFilterDesc* pDesc)					const = 0;
		virtual IAudioFilter*		CreateIIRFilter(const IIRFilterDesc* pDesc)					const = 0;
		virtual IAudioFilter*		CreateAddFilter(const AddFilterDesc* pDesc)					const = 0;
		virtual IAudioFilter*		CreateMulFilter(const MulFilterDesc* pDesc)					const = 0;
		virtual IAudioFilter*		CreateCombFilter(const CombFilterDesc* pDesc)				const = 0;
		virtual IAudioFilter*		CreateAllPassFilter(const AllPassFilterDesc* pDesc)			const = 0;
		virtual IAudioFilter*		CreateFilterSystem(const FilterSystemDesc* pDesc)			const = 0;

		virtual IAudioFilter*		CreateLowpassFIRFilter(float64 cutoffFreq, uint32 order)	const = 0;
		virtual IAudioFilter*		CreateHighpassFIRFilter(float64 cutoffFreq, uint32 order)	const = 0;
		virtual IAudioFilter*		CreateBandpassFIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order)	const = 0;
		virtual IAudioFilter*		CreateBandstopFIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order)	const = 0;

		virtual IAudioFilter*		CreateLowpassIIRFilter(float64 cutoffFreq, uint32 order)	const = 0;
		virtual IAudioFilter*		CreateHighpassIIRFilter(float64 cutoffFreq, uint32 order)	const = 0;
		virtual IAudioFilter*		CreateBandpassIIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order)	const = 0;
		virtual IAudioFilter*		CreateBandstopIIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order)	const = 0;


		virtual void				SetMasterFilter(const IAudioFilter* pAudioFilter)			= 0;

		virtual void				SetMasterVolume(float64 volume)								= 0;
		virtual void				SetMasterFilterEnabled(bool enabled)						= 0;

		virtual float64				GetMasterVolume()											const = 0;
		virtual bool				GetMasterFilterEnabled()									const = 0;

		virtual float64				GetSampleRate()												const = 0;
	};

	LAMBDA_API IAudioDevice* CreateAudioDevice(EAudioAPI api, const AudioDeviceDesc& desc);
}
