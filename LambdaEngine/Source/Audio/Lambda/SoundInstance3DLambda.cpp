#include "Audio/Lambda/SoundInstance3DLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	SoundInstance3DLambda::SoundInstance3DLambda(AudioDeviceLambda* pAudioDevice) 
		: m_pAudioDevice(pAudioDevice)
	{
	}

	SoundInstance3DLambda::~SoundInstance3DLambda()
	{
		SAFEDELETE_ARRAY(pWaveForm);
	}

	bool SoundInstance3DLambda::Init(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc					!= nullptr);
		VALIDATE(pDesc->pSoundEffect	!= nullptr);

		SoundEffect3DLambda* pSoundEffect = reinterpret_cast<SoundEffect3DLambda*>(pDesc->pSoundEffect);
		pSoundEffect->AddRef();

		m_pEffect			= pSoundEffect;
		Desc				= m_pEffect->GetDesc();
		ReferenceDistance	= pDesc->ReferenceDistance;
		MaxDistance			= pDesc->MaxDistance;
		TotalSampleCount	= Desc.SampleCount * Desc.ChannelCount;
		pWaveForm			= new float32[TotalSampleCount];
		memcpy(pWaveForm, pSoundEffect->GetWaveform(), sizeof(float32) * TotalSampleCount);

		return true;
	}

	void SoundInstance3DLambda::Play()
	{
		IsPlaying = true;
	}

	void SoundInstance3DLambda::Pause()
	{
		IsPlaying = false;
	}

	void SoundInstance3DLambda::Stop()
	{
		Pause();
		CurrentBufferIndex = 0;
	}

	void SoundInstance3DLambda::Toggle()
	{
		IsPlaying = !IsPlaying;
	}

	void SoundInstance3DLambda::SetPosition(const glm::vec3& position)
	{
		Position = position;
	}

	void SoundInstance3DLambda::SetVolume(float32 volume)
	{
		Volume = glm::max(glm::min(volume, 1.0f), 0.0f);
	}

	void SoundInstance3DLambda::SetPitch(float32 pitch)
	{
		UNREFERENCED_VARIABLE(pitch);
	}

	void SoundInstance3DLambda::SetReferenceDistance(float32 referenceDistance)
	{
		ReferenceDistance = referenceDistance;
	}

	void SoundInstance3DLambda::SetMaxDistance(float32 maxDistance)
	{
		MaxDistance = maxDistance;
	}

	const glm::vec3& SoundInstance3DLambda::GetPosition() const
	{
		return Position;
	}

	float SoundInstance3DLambda::GetVolume() const
	{
		return Volume;
	}

	float SoundInstance3DLambda::GetPitch() const
	{
		return 1.0f;
	}

	float32 SoundInstance3DLambda::GetMaxDistance() const
	{
		return MaxDistance;
	}

	float32 SoundInstance3DLambda::GetReferenceDistance() const
	{
		return ReferenceDistance;
	}

	void SoundInstance3DLambda::UpdateVolume(float32 masterVolume, const AudioListenerDesc* pAudioListeners, uint32 count)
	{
	}
}