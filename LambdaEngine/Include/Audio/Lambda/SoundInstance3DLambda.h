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
		SoundInstance3DLambda(AudioDeviceLambda* pAudioDevice);
		~SoundInstance3DLambda();

		bool Init(const SoundInstance3DDesc* pDesc);

		void UpdateVolume(float32 masterVolume, const AudioListenerDesc* pAudioListeners, uint32 count);
		
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
		
		uint32 TotalSampleCount		= 0;
		uint32 CurrentBufferIndex	= 0;

		float32	Volume				= 1.0f;
		float32 MaxDistance			= 1.0f;
		float32 ReferenceDistance	= 1.0f;
		float32 RollOff				= 1.0f;

		glm::vec3 Position;
		SoundDesc Desc;

	private:
		AudioDeviceLambda*		m_pAudioDevice	= nullptr;
		SoundEffect3DLambda*	m_pEffect		= nullptr;
	};
}
