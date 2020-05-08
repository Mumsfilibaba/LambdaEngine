#pragma once

#include "Audio/API/IAudioFilter.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceLambda;

	class FIRFilterLambda : public IAudioFilter
	{
	public:
		FIRFilterLambda(const IAudioDevice* pAudioDevice);
		~FIRFilterLambda();

		bool Init(const FIRFilterDesc* pDesc);

		virtual IAudioFilter* CreateCopy() const override final;

		virtual float64 ProcessSample(float64 sample) override final;

		virtual void SetEnabled(bool enabled) override final;
		FORCEINLINE virtual bool GetEnabled() const override final { return m_Enabled; }

		FORCEINLINE virtual const char* GetName() const override final { return m_pName; }
		FORCEINLINE virtual EFilterType GetType() const override final { return EFilterType::FIR; }

	private:
		const AudioDeviceLambda*	m_pAudioDevice		= nullptr;

		const char*	m_pName					= "";
		bool		m_Enabled				= true;

		float64*	m_pSampleBuffer			= nullptr;
		int32		m_CurrentSampleIndex	= 0;

		float64*	m_pWeights				= nullptr;
		uint32		m_WeightsCount			= 0;
	};
}