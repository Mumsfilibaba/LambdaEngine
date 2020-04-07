#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FenceLegacyVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
	FenceLegacyVK::FenceLegacyVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}
	
	FenceLegacyVK::~FenceLegacyVK()
	{
	}
	
	bool FenceLegacyVK::Init(const FenceDesc& desc)
	{
		LOG_ERROR("[FenceLegacyVK]: FUNCTION NOT IMPLEMENTED");
		return false;
	}
	
	void FenceLegacyVK::SetName(const char* pName)
	{
		LOG_ERROR("[FenceLegacyVK]: FUNCTION NOT IMPLEMENTED");
	}
	
	void FenceLegacyVK::Wait(uint64 signalValue, uint64 timeOut) const
	{
		LOG_ERROR("[FenceLegacyVK]: FUNCTION NOT IMPLEMENTED");
	}
	
	void FenceLegacyVK::Signal(uint64 signalValue)
	{
		LOG_ERROR("[FenceLegacyVK]: FUNCTION NOT IMPLEMENTED");
	}

	uint64 FenceLegacyVK::GetValue() const
	{
		LOG_ERROR("[FenceLegacyVK]: FUNCTION NOT IMPLEMENTED");
		return uint64(0);
	}
}