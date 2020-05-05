#pragma once
#include "Audio/API/IAudioListener.h"

namespace LambdaEngine
{
	class AudioDeviceLambda;

	class AudioListenerLambda final : public IAudioListener
	{
	public:
		AudioListenerLambda(AudioDeviceLambda* pDevice);
		~AudioListenerLambda();

		bool Init(const AudioListenerDesc* pDesc);

		//  IAudioListener interface
		virtual void SetVolume(float32 volume)											override final;
		virtual void SetDirectionVectors(const glm::vec3& up, const glm::vec3& forward) override final;
		virtual void SetPosition(const glm::vec3& position)								override final;

		virtual float32				GetVolume()				const override final;
		virtual glm::vec3			GetForwardVector()		const override final;
		virtual glm::vec3			GetUpVector()			const override final;
		virtual glm::vec3			GetPosition()			const override final;
		virtual AudioListenerDesc	GetDesc()				const override final;

	public:
		AudioListenerDesc Desc;
	
	private:
		AudioDeviceLambda* m_pDevice = nullptr;
	};
}