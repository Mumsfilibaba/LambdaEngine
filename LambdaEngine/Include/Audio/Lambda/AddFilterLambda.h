#pragma once

#include "Audio/API/IAudioFilter.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceLambda;

	class AddFilterLambda : public IAudioFilter
	{
	public:
		AddFilterLambda(const IAudioDevice* pAudioDevice);
		~AddFilterLambda();

		bool Init(const AddFilterDesc* pDesc);

		virtual IAudioFilter* CreateCopy() const override final;

		virtual float64 ProcessSample(float64 sample) override final;

		virtual void SetEnabled(bool enabled) override final;
		FORCEINLINE virtual bool GetEnabled() const override final { return m_Enabled; }

		FORCEINLINE virtual EFilterType GetType() const override final { return EFilterType::ADD; }

	private:
		const AudioDeviceLambda* m_pAudioDevice = nullptr;

		const char* m_pName			= "";
		bool		m_Enabled		= true;
	};
}