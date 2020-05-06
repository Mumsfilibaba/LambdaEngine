#pragma once
#include "Core/RefCountedObject.h"

#include "Math/Math.h"

#include "AudioTypes.h"

namespace LambdaEngine
{
	struct SoundEffect3DDesc
	{
		const char* pFilepath	= "";
	};
	
	class ISoundEffect3D : public RefCountedObject
	{
	public:
		DECL_INTERFACE(ISoundEffect3D);

		virtual SoundDesc GetDesc() const = 0;

		virtual void PlayOnce(const SoundInstance3DDesc* pDesc) = 0;
	};
}