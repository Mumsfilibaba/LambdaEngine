#pragma once

#include "Audio/API/IAudioFilter.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceLambda;

	class AllPassFilterLambda : public IAudioFilter
	{
	public:
		AllPassFilterLambda(const IAudioDevice* pAudioDevice);
		~AllPassFilterLambda();

		bool Init(const AllPassFilterDesc* pDesc);

		virtual IAudioFilter* CreateCopy() const override final;

		virtual float64 ProcessSample(float64 sample) override final;

		virtual void SetEnabled(bool enabled) override final;
		FORCEINLINE virtual bool GetEnabled() const override final { return m_Enabled; }

		FORCEINLINE virtual EFilterType GetType() const override final { return EFilterType::ALL_PASS; }

	private:
		const AudioDeviceLambda* m_pAudioDevice = nullptr;

		const char* m_pName					= "";
		bool		m_Enabled				= true;

		float64*	m_pPreviousSamples		= nullptr;
		int32		m_CurrentSampleIndex	= 0;
		uint32		m_Delay					= 0;

		float64		m_Multiplier			= 1.0;
	};
}