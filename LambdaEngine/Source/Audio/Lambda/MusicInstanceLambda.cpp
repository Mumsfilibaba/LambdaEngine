#include "Audio/Lambda/MusicInstanceLambda.h"
#include "Audio/Lambda/MusicLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

namespace LambdaEngine
{
	MusicInstanceLambda::MusicInstanceLambda(AudioDeviceLambda* pDevice)
		: m_pDevice(pDevice),
		Desc()
	{
	}

	MusicInstanceLambda::~MusicInstanceLambda()
	{
		m_pDevice->RemoveMusicInstance(this);
		SAFERELEASE(m_pMusic);
	}

	bool MusicInstanceLambda::Init(const MusicInstanceDesc* pDesc)
	{
		VALIDATE(pDesc			!= nullptr);
		VALIDATE(pDesc->pMusic	!= nullptr);

		MusicLambda* pMusic = reinterpret_cast<MusicLambda*>(pDesc->pMusic);
		pMusic->AddRef();

		m_pMusic			= pMusic;
		Volume				= pDesc->Volume;
		Pitch				= pDesc->Pitch;
		pWaveForm			= m_pMusic->GetWaveForm();
		Desc				= m_pMusic->GetDesc();
		TotalSampleCount	= Desc.SampleCount * Desc.ChannelCount;

		return true;
	}

	void MusicInstanceLambda::Play()
	{
		IsPlaying = true;
	}

	void MusicInstanceLambda::Pause()
	{
		IsPlaying = false;
	}

	void MusicInstanceLambda::Toggle()
	{
		IsPlaying = !IsPlaying;
	}

	void MusicInstanceLambda::Stop()
	{
		Pause();
		CurrentBufferIndex = 0;
	}

	void MusicInstanceLambda::SetVolume(float32 volume)
	{
		Volume = glm::max(glm::min(volume, 1.0f), 0.0f);
	}

	void MusicInstanceLambda::SetPitch(float32 pitch)
	{
		Pitch = pitch;
	}

	float MusicInstanceLambda::GetVolume() const
	{
		return Volume;
	}

	float MusicInstanceLambda::GetPitch() const
	{
		return Pitch;
	}
}