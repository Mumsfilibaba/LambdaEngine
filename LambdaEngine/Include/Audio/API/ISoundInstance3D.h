#pragma once
#include "Core/RefCountedObject.h"

#include "Math/Math.h"

#include "AudioTypes.h"

namespace LambdaEngine
{
	class ISoundEffect3D;
	
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
		* Set how quickly the audio fades off
		*/
		virtual void SetRollOff(float32 rollOff) = 0;

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