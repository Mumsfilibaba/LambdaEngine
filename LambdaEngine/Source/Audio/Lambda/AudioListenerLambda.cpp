#include "Audio/Lambda/AudioListenerLambda.h"

namespace LambdaEngine
{
	AudioListenerLambda::AudioListenerLambda(AudioDeviceLambda* pDevice)
		: m_pDevice(pDevice)
	{
	}

	AudioListenerLambda::~AudioListenerLambda()
	{
	}

	bool AudioListenerLambda::Init(const AudioListenerDesc* pDesc)
	{
		memcpy(&Desc, pDesc, sizeof(AudioListenerDesc));
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