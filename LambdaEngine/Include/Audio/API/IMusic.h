#pragma once
#include "Core/RefCountedObject.h"

#include "Math/Math.h"

#include "AudioTypes.h"

namespace LambdaEngine
{
	struct MusicDesc
	{
		const char*	pFilepath = "";
	};

	class IMusic : public RefCountedObject
	{
	public:
		DECL_INTERFACE(IMusic);

		virtual SoundDesc GetDesc() const = 0;
	};
}