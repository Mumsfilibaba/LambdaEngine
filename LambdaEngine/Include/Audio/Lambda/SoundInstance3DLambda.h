#pragma once

#include "PortAudio.h"
#include "Audio/API/ISoundInstance3D.h"

namespace LambdaEngine
{
	struct AudioListenerDesc;

	class AudioDeviceLambda;
	class SoundEffect3DLambda;
	
	class SoundInstance3DLambda : public ISoundInstance3D
	{
	public:
		SoundInstance3DLambda(AudioDeviceLambda* pAudioDevice, bool playOnce);
		~SoundInstance3DLambda();

		bool Init(const SoundInstance3DDesc* pDesc);
		
		// ISoundInstance3D interface
		virtual void Play()		override final;
		virtual void Pause()	override final;
		virtual void Stop()		override final;
		virtual void Toggle()	override final;

		virtual void SetPosition(const glm::vec3& position)				override final;
		virtual void SetVolume(float32 volume)							override final;
		virtual void SetPitch(float32 pitch)							override final;
		virtual void SetReferenceDistance(float32 referenceDistance)	override final;
		virtual void SetMaxDistance(float32 maxDistance)				override final;

		virtual const glm::vec3&	GetPosition()			const override final;
		virtual float32				GetVolume()				const override final;
		virtual float32				GetPitch()				const override final;
		virtual float32				GetMaxDistance()		const override final;
		virtual float32				GetReferenceDistance()	const override final;

	public:
		float32*	pWaveForm	= nullptr;
		bool		IsPlaying	= false;
		bool		PlayOnce	= false;
		
		uint32 TotalSampleCount		= 0;
		uint32 CurrentBufferIndex	= 0;

		ESoundMode	Mode				= ESoundMode::SOUND_MODE_NONE;
		float32		Pitch				= 1.0f;
		float32		Volume				= 1.0f;
		float32		ReferenceDistance	= 1.0f;
		float32		MaxDistance			= 10.0f;
		float32		RollOff				= 1.0f;
		glm::vec3	Position			= glm::vec3(0.0f);

		SoundDesc Desc;

	private:
		AudioDeviceLambda*		m_pDevice = nullptr;
		SoundEffect3DLambda*	m_pEffect = nullptr;
	};
}
