#pragma once
#include "Rendering/Core/API/ITopLevelAccelerationStructure.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class BufferVK;

	class TopLevelAccelerationStructureVK : public DeviceChildBase<GraphicsDeviceVK, ITopLevelAccelerationStructure>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ITopLevelAccelerationStructure>;

	public:
		TopLevelAccelerationStructureVK(const GraphicsDeviceVK* pDevice);
		~TopLevelAccelerationStructureVK();

		bool Init(const TopLevelAccelerationStructureDesc& desc);

		virtual void UpdateData(IBuffer* pBuffer) override;

		virtual void SetName(const char* pName) override;

	private:
		//INIT
		bool InitAccelerationStructure();
		bool InitScratchBuffer();

		//UPDATE
		void UpdateScratchBuffer();

		//UTIL
		VkMemoryRequirements GetMemoryRequirements();

	private:
		TopLevelAccelerationStructureDesc m_Desc;

		BufferVK* m_pScratchBuffer;
		VkDeviceOrHostAddressKHR m_ScratchBufferAddressUnion;

		VkAccelerationStructureKHR m_AccelerationStructure;
		VkDeviceMemory m_AccelerationStructureMemory;
		VkDeviceAddress m_AccelerationStructureDeviceAddress;

	};
}