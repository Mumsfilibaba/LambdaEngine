#pragma once
#include "LambdaEngine.h"

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
}