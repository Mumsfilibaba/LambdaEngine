#pragma once
#include "Core/RefCountedObject.h"

#include "Math/Math.h"

#include "AudioTypes.h"

namespace LambdaEngine
{
	class ISoundEffect3D;
	
	struct SoundInstance3DDesc
	{
		const char*		pName				= "";
		ISoundEffect3D* pSoundEffect		= nullptr;
		ESoundMode		Mode				= ESoundMode::SOUND_MODE_NONE;
		float32			ReferenceDistance	= 6.0f;		// The distance were the attenutation-volume is 1.0f
		float32			MaxDistance			= 20.0f;	// The distance were the attenutation reaches its lowest value
		float32			RollOff				= 1.0f;		// The speed of the attenutation [0.01, inf]
	};
	
	class ISoundInstance3D : public RefCountedObject
	{
	public:
		DECL_INTERFACE(ISoundInstance3D);

		/*
		* Play the sound instance
		*/
		virtual void Play() = 0;

		/*
		* Pause the sound instance
		*/
		virtual void Pause() = 0;

		/*
		* Stop the sound instance, releases some internal resources, a consecutive call to Play will restart the sound instance
		*/
		virtual void Stop() = 0;

		/*
		* Toggle the played/paused state of the sound instance
		*/
		virtual void Toggle() = 0;

		/*
		* Set the world position of the sound instance
		*	position - The world space position, should be given in meters
		*/
		virtual void SetPosition(const glm::vec3& position) = 0;

		/*
		* Set the volume of the sound instance in the range [-1.0, 1.0]
		*/
		virtual void SetVolume(float32 volume) = 0;

		/*
		* Set the distance at were the volume will be 1.0
		*/
		virtual void SetReferenceDistance(float32 referenceDistance) = 0;

		/*
		* Set the distance at were the volume will be 0.0
		*/
		virtual void SetMaxDistance(float32 maxDistance) = 0;

		/*
		* Set the pitch of the sound instance in the range [-Inf, Inf]
		*/
		virtual void SetPitch(float32 pitch) = 0;

		virtual const glm::vec3&	GetPosition()			const = 0;
		virtual float32				GetVolume()				const = 0;
		virtual float32				GetPitch()				const = 0;
		virtual float32				GetMaxDistance()		const = 0;
		virtual float32				GetReferenceDistance()	const = 0;
	};
}