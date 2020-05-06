#pragma once
#include "LambdaEngine.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	enum class ESoundMode : uint32
	{
		SOUND_MODE_NONE		= 0,
		SOUND_MODE_LOOPING	= 1,
	};

	enum class ESoundFormat
	{
		SOUND_FORMAT_NONE	= 0,
		SOUND_FORMAT_INT8	= 1,
		SOUND_FORMAT_INT16	= 2,
		SOUND_FORMAT_INT32	= 3,
		SOUND_FORMAT_FLOAT	= 4,
	};

	struct SoundDesc
	{
		uint32 SampleCount	= 0;
		uint32 SampleRate	= 0;
		uint32 ChannelCount = 0;
	};

	struct SoundInstance3DDesc
	{
		const char*				pName				= "";
		class ISoundEffect3D*	pSoundEffect		= nullptr;
		ESoundMode				Mode				= ESoundMode::SOUND_MODE_NONE;
		float32					Volume				= 1.0f;
		glm::vec3				Position			= glm::vec3(0.0f);
		float32					ReferenceDistance	= 6.0f;		// The distance were the attenutation-volume is 1.0f
		float32					MaxDistance			= 20.0f;	// The distance were the attenutation reaches its lowest value
		float32					RollOff				= 1.0f;		// The speed of the attenutation [0.01, inf]
	};
}