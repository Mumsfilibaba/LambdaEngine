#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	constexpr const uint32 MAX_PREVIOUS_FILTERS = 8;

	class IAudioFilter;

	struct AddFilterDesc
	{
		const char*		pName			= "ADD Filter";
	};

	struct MulFilterDesc
	{
		const char* pName				= "MUL Filter";
		float64		Multiplier			= 1.0;
	};

	struct CombFilterDesc
	{
		const char* pName				= "Comb Filter";
		uint32		Delay				= 1;
		float64		Multiplier			= 1.0;
	};

	struct AllPassFilterDesc
	{
		const char* pName				= "All Pass Filter";
		uint32		Delay				= 1;
		float64		Multiplier			= 0.5;
	};

	struct FIRFilterDesc
	{
		const char*		pName			= "FIR Filter";
		const float64*	pWeights		= nullptr;
		uint32			WeightsCount	= 0;
	};

	struct IIRFilterDesc
	{
		const char*		pName						= "IIR Filter";
		const float64*	pFeedForwardWeights			= nullptr;
		uint32			FeedForwardWeightsCount		= 0;
		const float64*	pFeedBackWeights			= nullptr;
		uint32			FeedBackWeightsCount		= 0;
	};

	struct FilterSystemConnection
	{
		int32	pPreviousFilters[MAX_PREVIOUS_FILTERS];
		uint32	PreviousFiltersCount	= 0;
		int32	NextFilter				= -1;
	};

	struct FilterSystemDesc
	{
		const char*				pName					= "Filter System";
		IAudioFilter**			ppAudioFilters			= nullptr;
		uint32					AudioFilterCount		= 0;
		FilterSystemConnection*	pFilterConnections		= nullptr;
		uint32					FilterConnectionsCount	= 0;
	};

	enum class EFilterType
	{
		NONE		= 0,
		ADD			= 1,
		MUL			= 2,
		FIR			= 3,
		IIR			= 4,
		COMB		= 5,
		ALL_PASS	= 6,
		SYSTEM		= 7,
	};

	class IAudioFilter
	{
	public:
		DECL_INTERFACE(IAudioFilter);

		virtual IAudioFilter* CreateCopy() const = 0;

		virtual float64 ProcessSample(float64 sample) = 0;

		virtual void SetEnabled(bool enabled) = 0;
		virtual bool GetEnabled() const = 0;

		virtual EFilterType GetType() const = 0;
	};
}