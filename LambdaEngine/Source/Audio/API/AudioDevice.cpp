#include "Audio/API/IAudioDevice.h"

#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

namespace LambdaEngine
{
	IAudioDevice* CreateAudioDevice(EAudioAPI api, const AudioDeviceDesc& desc)
	{
		if (api == EAudioAPI::FMOD)
		{
			AudioDeviceFMOD* pFMODDevice = DBG_NEW AudioDeviceFMOD();
			if (pFMODDevice->Init(&desc))
			{
				return pFMODDevice;
			}
			else
			{
				SAFEDELETE(pFMODDevice);
				return nullptr;
			}
		}
		else if (api == EAudioAPI::LAMBDA)
		{
			AudioDeviceLambda* pDevice = DBG_NEW AudioDeviceLambda();
			if (pDevice->Init(&desc))
			{
				return pDevice;
			}
			else
			{
				SAFEDELETE(pDevice);
				return nullptr;
			}
		}
		else
		{
			return nullptr;
		}
	}
}
