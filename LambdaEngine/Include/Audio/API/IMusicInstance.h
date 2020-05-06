#pragma once
#include "Core/RefCountedObject.h"

#include "AudioTypes.h"

namespace LambdaEngine
{
	class IMusic;

	struct MusicInstanceDesc
	{
		IMusic* pMusic	= nullptr;
		float32 Volume	= 1.0f;
		float32 Pitch	= 1.0f;
	};

	class IMusicInstance : public RefCountedObject
	{
	public:
		DECL_INTERFACE(IMusicInstance);

		/*
		* Play the music
		*/
		virtual void Play() = 0;

		/*
		* Pause the music
		*/
		virtual void Pause() = 0;

		/*
		* Pause the music
		*/
		virtual void Stop() = 0;

		/*
		* Toggle the played/paused state of the music
		*/
		virtual void Toggle() = 0;

		/*
		* Set the volume of the music in the range [-Inf, Inf]
		*/
		virtual void SetVolume(float32 volume) = 0;

		/*
		* Set the pitch of the music in the range [-Inf, Inf]
		*/
		virtual void SetPitch(float32 pitch) = 0;

		virtual float GetVolume() const = 0;
		virtual float GetPitch()  const = 0;
	};
}