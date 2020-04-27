#pragma once
#include "Rendering/Core/API/IFence.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class FenceTimelineVK : public TDeviceChildBase<GraphicsDeviceVK, IFence>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IFence>;

	public:
		FenceTimelineVK(const GraphicsDeviceVK* pDevice);
		~FenceTimelineVK();

		bool Init(const FenceDesc* pDesc);

        FORCEINLINE VkSemaphore GetSemaphore() const
        {
            return m_Semaphore;
        }
        
        // IDeviceChild interface
        virtual void SetName(const char* pName) override final;
        
        // IFence interface
        virtual void Wait(uint64 waitValue, uint64 timeOut)	const override final;
        virtual void Reset(uint64 resetValue)                     override final;

        virtual uint64 GetValue() const override final;
        
        FORCEINLINE virtual FenceDesc GetDesc() const override final
        {
            return m_Desc;
        }
		
	private:
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
        FenceDesc   m_Desc;
	};
}