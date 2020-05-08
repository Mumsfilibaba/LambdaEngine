#pragma once

#include "FMOD.h"
#include "Audio/API/IAudioDevice.h"

namespace LambdaEngine
{
	class SoundEffect3DFMOD;
	class SoundInstance3DFMOD;
	class AudioGeometryFMOD;
	class ReverbSphereFMOD;

	class AudioDeviceFMOD : public IAudioDevice
	{
	public:
		AudioDeviceFMOD();
		~AudioDeviceFMOD();

		virtual bool Init(const AudioDeviceDesc* pDesc) override final;

		virtual void FixedTick(Timestamp delta) override final;

		virtual void UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc) override final;

		virtual uint32				CreateAudioListener()										override final;
		virtual IMusic*				CreateMusic(const MusicDesc* pDesc)							override final;
		virtual ISoundEffect3D*		CreateSoundEffect(const SoundEffect3DDesc* pDesc)			override final;
		virtual ISoundInstance3D*	CreateSoundInstance(const SoundInstance3DDesc* pDesc)		override final;
		virtual IAudioGeometry*		CreateAudioGeometry(const AudioGeometryDesc* pDesc)			override final;
		virtual IReverbSphere*		CreateReverbSphere(const ReverbSphereDesc* pDesc)			override final;

		virtual IAudioFilter*		CreateFIRFilter(const FIRFilterDesc* pDesc)					const override final;
		virtual IAudioFilter*		CreateIIRFilter(const IIRFilterDesc* pDesc)					const override final;
		virtual IAudioFilter*		CreateAddFilter(const AddFilterDesc* pDesc)					const override final;
		virtual IAudioFilter*		CreateMulFilter(const MulFilterDesc* pDesc)					const override final;
		virtual IAudioFilter*		CreateCombFilter(const CombFilterDesc* pDesc)				const override final;
		virtual IAudioFilter*		CreateAllPassFilter(const AllPassFilterDesc* pDesc)			const override final;
		virtual IAudioFilter*		CreateFilterSystem(const FilterSystemDesc* pDesc)			const override final;

		virtual IAudioFilter*		CreateLowpassFIRFilter(float64 cutoffFreq, uint32 order)	const override final;
		virtual IAudioFilter*		CreateHighpassFIRFilter(float64 cutoffFreq, uint32 order)	const override final;
		virtual IAudioFilter*		CreateBandpassFIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order)	const override final;
		virtual IAudioFilter*		CreateBandstopFIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order)	const override final;

		virtual IAudioFilter*		CreateLowpassIIRFilter(float64 cutoffFreq, uint32 order)	const override final;
		virtual IAudioFilter*		CreateHighpassIIRFilter(float64 cutoffFreq, uint32 order)	const override final;
		virtual IAudioFilter*		CreateBandpassIIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order)	const override final;
		virtual IAudioFilter*		CreateBandstopIIRFilter(float64 lowCutoffFreq, float64 highCutoffFreq, uint32 order)	const override final;

		virtual void				SetMasterFilter(const IAudioFilter* pAudioFilter)			override final;

		virtual void				SetMasterVolume(float64 volume)								override final;
		virtual void				SetMasterFilterEnabled(bool enabled)						override final;

		virtual float64				GetMasterVolume()											const override final;
		virtual bool				GetMasterFilterEnabled()									const override final;

		virtual float64				GetSampleRate()												const override final;

	public:
		FMOD_SYSTEM* pSystem					= nullptr;

	private:
		const char*		m_pName					= "";

		uint32			m_MaxNumAudioListeners	= 0;
		uint32			m_NumAudioListeners		= 0;
	};
}