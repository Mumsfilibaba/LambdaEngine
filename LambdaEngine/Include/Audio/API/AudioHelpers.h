#pragma once

#include "AudioTypes.h"

namespace LambdaEngine
{
	FORCEINLINE int32 ConvertSpeakerSetupToChannelCount(ESpeakerSetup speakerSetup)
	{
		switch (speakerSetup)
		{
			case ESpeakerSetup::STEREO_SOUND_SYSTEM:
			case ESpeakerSetup::STEREO_HEADPHONES:
				return 2;
			case ESpeakerSetup::SURROUND_5_1:
				return 5;
			case ESpeakerSetup::SURROUND_7_1:	
				return 7;
			default:
				return INT32_MAX;
		}
	}
}