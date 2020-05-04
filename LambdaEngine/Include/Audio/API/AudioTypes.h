#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class SoundEffect3DFMOD;

	enum FSoundModeFlags : uint32
	{
		SOUND_MODE_NONE		= FLAG(0),
		SOUND_MODE_LOOPING	= FLAG(1),
	};

	enum class ESpeakerSetup : uint32
	{ 
		NONE					= 0,
		STEREO_SOUND_SYSTEM		= 1,
		STEREO_HEADPHONES		= 2,
		SURROUND_5_1			= 3,
		SURROUND_7_1			= 4,
	};
}