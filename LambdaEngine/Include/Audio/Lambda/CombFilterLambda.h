#pragma once

#include "Audio/API/IAudioFilter.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceLambda;

	class CombFilterLambda : public IAudioFilter
	{
	public:
		CombFilterLambda(const IAudioDevice* pAudioDevice);
		~CombFilterLambda();

		bool Init(const CombFilterDesc* pDesc);

		virtual IAudioFilter* CreateCopy() const override final;

		virtual float64 ProcessSample(float64 sample) override final;

		virtual void SetEnabled(bool enabled) override final;
		FORCEINLINE virtual bool GetEnabled() const override final { return m_Enabled; }

		FORCEINLINE virtual const char* GetName() const override final { return m_pName; }
		FORCEINLINE virtual EFilterType GetType() const override final { return EFilterType::COMB; }

	private:
		const AudioDeviceLambda* m_pAudioDevice = nullptr;

		const char* m_pName					= "";
		bool		m_Enabled				= true;

		float64*	m_pPreviousSamples		= nullptr;
		int32		m_CurrentSampleIndex	= 0;
		uint32		m_Delay					= 0;

		float64		m_Multiplier			= 1.0;
		bool		m_FeedForward			= true;
	};
}