#pragma once
#include "Core/RefCountedObject.h"

#include "AudioTypes.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	struct AudioListenerDesc
	{
		glm::vec3	Position	= glm::vec3(0.0f);
		glm::vec3	Forward		= glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3	Up			= glm::vec3(0.0f, 1.0f, 0.0f);
		float32		Volume		= 1.0f;
	};

	class IAudioListener : public RefCountedObject
	{
	public:
		DECL_INTERFACE(IAudioListener);

		virtual void SetVolume(float32 volume)											= 0;
		virtual void SetDirectionVectors(const glm::vec3& up, const glm::vec3& forward) = 0;
		virtual void SetPosition(const glm::vec3& position)								= 0;

		virtual float32				GetVolume()				const = 0;
		virtual glm::vec3			GetForwardVector()		const = 0;
		virtual glm::vec3			GetUpVector()			const = 0;
		virtual glm::vec3			GetPosition()			const = 0;
		virtual AudioListenerDesc	GetDesc()				const = 0;
	};
}