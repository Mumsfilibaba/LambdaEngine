#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class BufferVK;

	class RayTracingPipelineStateVK : public TDeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice);
		~RayTracingPipelineStateVK();

		bool Init(const RayTracingPipelineStateDesc& desc);

		FORCEINLINE VkDeviceSize GetBindingOffsetRaygenGroup()	const { return m_BindingOffsetRaygenShaderGroup; }
		FORCEINLINE VkDeviceSize GetBindingOffsetHitGroup()		const { return m_BindingOffsetHitShaderGroup; }
		FORCEINLINE VkDeviceSize GetBindingOffsetMissGroup()	const { return m_BindingOffsetMissShaderGroup; }
		FORCEINLINE VkDeviceSize GetBindingStride()				const { return m_BindingStride; }

		FORCEINLINE VkPipeline GetPipeline() const
        {
            return m_Pipeline;
        }
        
        //IDeviceChild interface
		virtual void SetName(const char* pName) override;

        //IPipelineState interface
		FORCEINLINE virtual EPipelineStateType GetType() const override
        {
            return EPipelineStateType::RAY_TRACING;
        }

	private:
		bool CreateShaderData(std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos,
			std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps,
			std::vector<VkRayTracingShaderGroupCreateInfoKHR>& shaderGroups,
			const RayTracingPipelineStateDesc& desc);

	private:
		VkPipeline	m_Pipeline;
		BufferVK*	m_pSBT;

		VkDeviceSize m_BindingOffsetRaygenShaderGroup;
		VkDeviceSize m_BindingOffsetHitShaderGroup;
		VkDeviceSize m_BindingOffsetMissShaderGroup;
		VkDeviceSize m_BindingStride;
	};
}
