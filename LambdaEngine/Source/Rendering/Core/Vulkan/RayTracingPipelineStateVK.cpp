#include "Log/Log.h"

#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"

namespace LambdaEngine
{
	RayTracingPipelineStateVK::RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	RayTracingPipelineStateVK::~RayTracingPipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	bool RayTracingPipelineStateVK::Init(CommandQueue* pCommandQueue, const RayTracingPipelineStateDesc* pDesc)
	{
		// Define shader stage create infos
		TArray<VkPipelineShaderStageCreateInfo>			shaderStagesInfos;
		TArray<VkSpecializationInfo>					shaderStagesSpecializationInfos;
		TArray<TArray<VkSpecializationMapEntry>>		shaderStagesSpecializationMaps;
		TArray<VkRayTracingShaderGroupCreateInfoKHR>	shaderGroups;

		// Raygen Shader
		{
			CreateShaderStageInfo(&pDesc->RaygenShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader			= 0;
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= VK_SHADER_UNUSED_NV;
			shaderGroups.EmplaceBack(shaderGroupCreateInfo);
		}

		// Closest-Hit Shaders
		for (const ShaderModuleDesc& closetHitShader : pDesc->ClosestHitShaders)
		{
			CreateShaderStageInfo(&closetHitShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroupCreateInfo.generalShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= static_cast<uint32>(shaderStagesInfos.GetSize() - 1);
			shaderGroups.EmplaceBack(shaderGroupCreateInfo);
		}

		// Miss Shaders
		for (const ShaderModuleDesc& missShader : pDesc->MissShaders)
		{
			CreateShaderStageInfo(&missShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader			= static_cast<uint32>(shaderStagesInfos.GetSize() - 1);
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= VK_SHADER_UNUSED_NV;
			shaderGroups.EmplaceBack(shaderGroupCreateInfo);
		}

		VkPipelineLibraryCreateInfoKHR rayTracingPipelineLibrariesInfo = {};
		rayTracingPipelineLibrariesInfo.sType		 = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		rayTracingPipelineLibrariesInfo.libraryCount = 0;
		rayTracingPipelineLibrariesInfo.pLibraries	 = nullptr;

		const PipelineLayoutVK* pPipelineLayoutVk = reinterpret_cast<const PipelineLayoutVK*>(pDesc->pPipelineLayout);
		
		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo = {};
		rayTracingPipelineInfo.sType			 = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineInfo.flags			 = VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR | VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR;
		rayTracingPipelineInfo.stageCount		 = static_cast<uint32>(shaderStagesInfos.GetSize());
		rayTracingPipelineInfo.pStages			 = shaderStagesInfos.GetData();
		rayTracingPipelineInfo.groupCount		 = static_cast<uint32>(shaderGroups.GetSize());
		rayTracingPipelineInfo.pGroups			 = shaderGroups.GetData();
		rayTracingPipelineInfo.maxRecursionDepth = pDesc->MaxRecursionDepth;
		rayTracingPipelineInfo.layout			 = pPipelineLayoutVk->GetPipelineLayout();
		rayTracingPipelineInfo.libraries		 = rayTracingPipelineLibrariesInfo;

		VkResult result = m_pDevice->vkCreateRayTracingPipelinesKHR(m_pDevice->Device, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkCreateRayTracingPipelinesKHR failed for \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkCreateRayTracingPipelinesKHR failed");
			}
			
			return false;
		}

		const uint32 shaderGroupHandleSize	= m_pDevice->RayTracingProperties.shaderGroupHandleSize;
		const uint32 sbtSize				= shaderGroupHandleSize * static_cast<uint32>(shaderGroups.GetSize());

		BufferDesc shaderHandleStorageDesc = {};
		shaderHandleStorageDesc.DebugName	= "Shader Handle Storage";
		shaderHandleStorageDesc.Flags		= BUFFER_FLAG_COPY_SRC;
		shaderHandleStorageDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		shaderHandleStorageDesc.SizeInBytes	= sbtSize;

		m_ShaderHandleStorageBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&shaderHandleStorageDesc, nullptr));

		void* pMapped = m_ShaderHandleStorageBuffer->Map();
		result = m_pDevice->vkGetRayTracingShaderGroupHandlesKHR(m_pDevice->Device, m_Pipeline, 0, static_cast<uint32>(shaderGroups.GetSize()), sbtSize, pMapped);
		if (result!= VK_SUCCESS)
		{
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkGetRayTracingShaderGroupHandlesKHR failed for \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkGetRayTracingShaderGroupHandlesKHR failed");
			}
			
			return false;
		}

		m_ShaderHandleStorageBuffer->Unmap();

		CommandAllocator* pCommandAllocator = m_pDevice->CreateCommandAllocator("Ray Tracing Pipeline SBT Copy", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName		= "Ray Tracing Pipeline Command List";
		commandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		CommandList* pCommandList = m_pDevice->CreateCommandList(pCommandAllocator, &commandListDesc);

		BufferDesc sbtDesc = {};
		sbtDesc.DebugName	= "Shader Binding Table";
		sbtDesc.Flags		= BUFFER_FLAG_COPY_DST | BUFFER_FLAG_RAY_TRACING;
		sbtDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		sbtDesc.SizeInBytes	= sbtSize;

		m_ShaderBindingTable = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&sbtDesc, nullptr));

		pCommandAllocator->Reset();

		pCommandList->Begin(nullptr);
		pCommandList->CopyBuffer(m_ShaderHandleStorageBuffer.Get(), 0, m_ShaderBindingTable.Get(), 0, sbtSize);
		pCommandList->End();

		pCommandQueue->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN , nullptr, 0, nullptr, 0);

		m_BindingStride						= shaderGroupHandleSize;
		
		m_BindingOffsetRaygenShaderGroup	= 0;
		m_BindingSizeRaygenShaderGroup		= m_BindingStride;
		
		m_BindingOffsetHitShaderGroup		= m_BindingOffsetRaygenShaderGroup + m_BindingSizeRaygenShaderGroup;
		m_BindingSizeHitShaderGroup			= m_BindingStride * static_cast<uint32>(pDesc->ClosestHitShaders.GetSize());
		
		m_BindingOffsetMissShaderGroup		= m_BindingOffsetHitShaderGroup + m_BindingSizeHitShaderGroup;
		m_BindingSizeMissShaderGroup		= m_BindingStride * static_cast<uint32>(pDesc->MissShaders.GetSize());

		SetName(pDesc->DebugName);
		if (!pDesc->DebugName.empty())
		{
			D_LOG_MESSAGE("[RayTracingPipelineStateVK]: Created Pipeline for %s", pDesc->DebugName.c_str());
		}
		else
		{
			D_LOG_MESSAGE("[RayTracingPipelineStateVK]: Created Pipeline");
		}

		SAFERELEASE(pCommandAllocator);
		SAFERELEASE(pCommandList);

		return true;
	}

	void RayTracingPipelineStateVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_Pipeline), VK_OBJECT_TYPE_PIPELINE);
		m_DebugName = debugName;
	}
	
	void RayTracingPipelineStateVK::CreateShaderStageInfo(const ShaderModuleDesc* pShaderModule, TArray<VkPipelineShaderStageCreateInfo>& shaderStagesInfos, 
		TArray<VkSpecializationInfo>& shaderStagesSpecializationInfos, TArray<TArray<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps)
	{
		const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pShaderModule->pShader);

		// ShaderStageInfo
		VkPipelineShaderStageCreateInfo shaderCreateInfo = { };
		shaderCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.pNext	= nullptr;
		shaderCreateInfo.flags	= 0;
		shaderCreateInfo.stage	= ConvertShaderStageFlag(pShader->GetDesc().Stage);
		shaderCreateInfo.module	= pShader->GetShaderModule();
		shaderCreateInfo.pName	= pShader->GetEntryPoint().c_str();

		// Shader Constants
		if (!pShaderModule->ShaderConstants.IsEmpty())
		{
			TArray<VkSpecializationMapEntry> specializationEntires(pShaderModule->ShaderConstants.GetSize());
			for (uint32 i = 0; i < pShaderModule->ShaderConstants.GetSize(); i++)
			{
				VkSpecializationMapEntry specializationEntry = {};
				specializationEntry.constantID = i;
				specializationEntry.offset = i * sizeof(ShaderConstant);
				specializationEntry.size = sizeof(ShaderConstant);
				specializationEntires.EmplaceBack(specializationEntry);
			}

			shaderStagesSpecializationMaps.EmplaceBack(specializationEntires);

			VkSpecializationInfo specializationInfo = { };
			specializationInfo.mapEntryCount = static_cast<uint32>(specializationEntires.GetSize());
			specializationInfo.pMapEntries = specializationEntires.GetData();
			specializationInfo.dataSize = static_cast<uint32>(pShaderModule->ShaderConstants.GetSize()) * sizeof(ShaderConstant);
			specializationInfo.pData = pShaderModule->ShaderConstants.GetData();
			shaderStagesSpecializationInfos.EmplaceBack(specializationInfo);

			shaderCreateInfo.pSpecializationInfo = &shaderStagesSpecializationInfos.GetBack();
		}
		else
		{
			shaderCreateInfo.pSpecializationInfo = nullptr;
		}

		shaderStagesInfos.EmplaceBack(shaderCreateInfo);
	}
}
