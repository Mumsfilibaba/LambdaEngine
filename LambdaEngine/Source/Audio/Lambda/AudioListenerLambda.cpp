#include "Audio/Lambda/AudioListenerLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

namespace LambdaEngine
{
	AudioListenerLambda::AudioListenerLambda(AudioDeviceLambda* pDevice)
		: m_pDevice(pDevice)
	{
	}

	AudioListenerLambda::~AudioListenerLambda()
	{
		m_pDevice->RemoveAudioListener(this);
	}

	bool AudioListenerLambda::Init(const AudioListenerDesc* pDesc)
	{
		Desc.Position	= pDesc->Position;
		Desc.Volume		= pDesc->Volume;

		SetDirectionVectors(pDesc->Up, pDesc->Forward);
		return true;
	}

	void AudioListenerLambda::SetVolume(float32 volume)
	{
		Desc.Volume = volume;
	}

	void AudioListenerLambda::SetDirectionVectors(const glm::vec3& up, const glm::vec3& forward)
	{
		Desc.Up			= up;
		Desc.Forward	= forward;

		glm::mat4 rotationMat	= glm::rotate(glm::mat4(1.0f), glm::half_pi<float32>(), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec4 RightVec		= rotationMat * glm::vec4(forward, 0.0f);
		Right = RightVec;
	}

	void AudioListenerLambda::SetPosition(const glm::vec3& position)
	{
		Desc.Position = position;
	}

	float32 AudioListenerLambda::GetVolume() const
	{
		return Desc.Volume;
	}

	glm::vec3 AudioListenerLambda::GetForwardVector() const
	{
		return Desc.Forward;
	}

	glm::vec3 AudioListenerLambda::GetUpVector() const
	{
		return Desc.Up;
	}

	glm::vec3 AudioListenerLambda::GetPosition() const
	{
		return Desc.Position;
	}

	AudioListenerDesc AudioListenerLambda::GetDesc() const
	{
		return Desc;
	}
}