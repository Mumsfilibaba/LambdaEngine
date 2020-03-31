#pragma once
#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Utilities/StringHash.h"
#include "Vulkan.h"

#include <optional>
#include <vector>

namespace LambdaEngine
{
	class CommandBufferVK;
	
	typedef ConstString ValidationLayer;
	typedef ConstString Extension;

	class GraphicsDeviceVK : public IGraphicsDevice
	{
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> GraphicsFamily;
			std::optional<uint32_t> ComputeFamily;
			std::optional<uint32_t> TransferFamily;
			std::optional<uint32_t> PresentFamily;

			bool IsComplete()
			{
				return GraphicsFamily.has_value() && ComputeFamily.has_value() && TransferFamily.has_value() && PresentFamily.has_value();
			}
		};

	public:
		GraphicsDeviceVK();
		~GraphicsDeviceVK();

		virtual bool Init(const GraphicsDeviceDesc& desc) override;
		virtual void Release() override;

		//CREATE
		virtual IRenderPass*	CreateRenderPass()						                            override;
		virtual IFence*			CreateFence()							                            override;
		virtual ICommandList*	CreateCommandList()						                            override;
		virtual IBuffer*		CreateBuffer(const BufferDesc& desc)	                            override;
		virtual ITexture*		CreateTexture(const TextureDesc& desc)	                            override;
		virtual ITextureView*	CreateTextureView()						                            override;
        virtual ISwapChain*     CreateSwapChain(const Window* pWindow, const SwapChainDesc& desc)   override;
		virtual IPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) 		override;
		virtual IPipelineState* CreateComputePipelineState(const ComputePipelineDesc& desc) 		override;
		virtual IPipelineState* CreateRayTracingPipelineState(const RayTracingPipelineDesc& desc)   override;
		

		//EXECUTE
		void ExecuteGraphics(CommandBufferVK* pCommandBuffer, const VkSemaphore* pWaitSemaphore, const VkPipelineStageFlags* pWaitStages,
			uint32_t waitSemaphoreCount, const VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount);
		void ExecuteCompute(CommandBufferVK* pCommandBuffer, const VkSemaphore* pWaitSemaphore, const VkPipelineStageFlags* pWaitStages,
			uint32_t waitSemaphoreCount, const VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount);
		void ExecuteTransfer(CommandBufferVK* pCommandBuffer, const VkSemaphore* pWaitSemaphore, const VkPipelineStageFlags* pWaitStages,
			uint32_t waitSemaphoreCount, const VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount);

		//UTIL
		void SetVulkanObjectName(const char* pName, uint64 objectHandle, VkObjectType type) const;

	private:
		//INIT
		bool InitInstance(const GraphicsDeviceDesc& desc);

		bool InitDevice(const GraphicsDeviceDesc& desc);
		bool InitPhysicalDevice();
		bool InitLogicalDevice(const GraphicsDeviceDesc& desc);

		//UTIL
		bool SetEnabledValidationLayers();
		bool SetEnabledInstanceExtensions();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		int32 RatePhysicalDevice(VkPhysicalDevice physicalDevice);
		void CheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice, bool& requiredExtensionsSupported, uint32_t& numOfOptionalExtensionsSupported);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice);
		uint32 GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies);
		void SetEnabledDeviceExtensions();

		bool IsInstanceExtensionEnabled(const char* pExtensionName);
		bool IsDeviceExtensionEnabled(const char* pExtensionName);

		void RegisterExtensionData();

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	public:
		VkInstance Instance;

		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;

		VkQueue m_GraphicsQueue;
		VkQueue m_ComputeQueue;
		VkQueue m_TransferQueue;
		VkQueue m_PresentQueue;

		//Extension Data
		PFN_vkSetDebugUtilsObjectNameEXT	vkSetDebugUtilsObjectNameEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
		PFN_vkCreateDebugUtilsMessengerEXT	vkCreateDebugUtilsMessengerEXT;

		PFN_vkCreateAccelerationStructureKHR					vkCreateAccelerationStructureKHR;
		PFN_vkDestroyAccelerationStructureKHR					vkDestroyAccelerationStructureKHR;
		PFN_vkBindAccelerationStructureMemoryKHR				vkBindAccelerationStructureMemoryKHR;
		PFN_vkGetAccelerationStructureDeviceAddressKHR			vkGetAccelerationStructureDeviceAddressKHR;
		PFN_vkGetAccelerationStructureMemoryRequirementsKHR		vkGetAccelerationStructureMemoryRequirementsKHR;
		PFN_vkCmdBuildAccelerationStructureKHR					vkCmdBuildAccelerationStructureKHR;
		PFN_vkCreateRayTracingPipelinesKHR						vkCreateRayTracingPipelinesKHR;
		PFN_vkGetRayTracingShaderGroupHandlesKHR				vkGetRayTracingShaderGroupHandlesKHR;
		PFN_vkCmdTraceRaysKHR									vkCmdTraceRaysKHR;

		VkPhysicalDeviceRayTracingPropertiesNV					RayTracingProperties;

	private:
		VkDebugUtilsMessengerEXT m_DebugMessenger;

		QueueFamilyIndices m_DeviceQueueFamilyIndices;
		VkPhysicalDeviceLimits m_DeviceLimits;

		std::vector<const char*> m_EnabledValidationLayers;
		std::vector<const char*> m_EnabledInstanceExtensions;
		std::vector<const char*> m_EnabledDeviceExtensions;
	};
}