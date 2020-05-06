#pragma once

#include "Audio/API/IAudioFilter.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceLambda;

	class IIRFilterLambda : public IAudioFilter
	{
	public:
		IIRFilterLambda(const IAudioDevice* pAudioDevice);
		~IIRFilterLambda();

		bool Init(const IIRFilterDesc* pDesc);

		virtual IAudioFilter* CreateCopy() const override final;

		virtual float64 ProcessSample(float64 sample) override final;

		virtual void SetEnabled(bool enabled) override final;
		FORCEINLINE virtual bool GetEnabled() const override final { return m_Enabled; }

		FORCEINLINE virtual EFilterType GetType() const override final { return EFilterType::IIR; }

	private:
		const AudioDeviceLambda* m_pAudioDevice = nullptr;

		const char* m_pName								= "";
		bool		m_Enabled							= true;

		float64*	m_pFeedForwardSampleBuffer			= nullptr;
		int32		m_CurrentFeedForwardSampleIndex		= 0;

		float64*	m_pFeedBackSampleBuffer				= nullptr;
		int32		m_CurrentFeedBackSampleIndex		= 0;

		float64*	m_pFeedForwardWeights				= nullptr;
		uint32		m_FeedForwardWeightsCount			= 0;

		float64*	m_pFeedBackWeights					= nullptr;
		uint32		m_FeedBackWeightsCount				= 0;

		float64		m_InputGain							= 1.0;
	};
}