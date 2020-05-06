#pragma once

#include "Audio/API/IAudioFilter.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceLambda;

	class FilterSystemLambda : public IAudioFilter
	{
	public:
		FilterSystemLambda(const IAudioDevice* pAudioDevice);
		~FilterSystemLambda();

		bool Init(const FilterSystemDesc* pDesc);

		virtual IAudioFilter* CreateCopy() const override final;

		virtual float64 ProcessSample(float64 sample) override final;

		virtual void SetEnabled(bool enabled) override final;
		FORCEINLINE virtual bool GetEnabled() const override final { return m_Enabled; }

		FORCEINLINE virtual EFilterType GetType() const override final { return EFilterType::SYSTEM; }

	private:
		const AudioDeviceLambda* m_pAudioDevice = nullptr;

		const char*			m_pName						= "";
		bool				m_Enabled					= true;

		IAudioFilter**		m_ppAudioFilters			= nullptr;
		float64*			m_pFilterOutputs			= nullptr;
		uint32				m_AudioFilterCount			= 0;

		FilterSystemConnection*	m_pFilterConnections		= nullptr;
		uint32				m_FilterConnectionsCount	= 0;
	};
}