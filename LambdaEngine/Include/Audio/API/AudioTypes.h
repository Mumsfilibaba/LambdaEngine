#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	enum class FSoundMode : uint32
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
}