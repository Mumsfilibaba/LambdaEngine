#include "Log/Log.h"

#include "Rendering/Core/Vulkan/QueryHeapVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	QueryHeapVK::QueryHeapVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	QueryHeapVK::~QueryHeapVK()
	{
		if (m_QueryPool != VK_NULL_HANDLE)
		{
			vkDestroyQueryPool(m_pDevice->Device, m_QueryPool, nullptr);
			m_QueryPool = VK_NULL_HANDLE;
		}
	}

	bool QueryHeapVK::Init(const QueryHeapDesc* pDesc)
	{
		VkQueryPoolCreateInfo createInfo = {};
		createInfo.sType				= VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		createInfo.pNext				= nullptr;
		createInfo.flags				= 0;
		createInfo.queryType			= ConvertQueryType(pDesc->Type);
		createInfo.queryCount			= pDesc->QueryCount;
		createInfo.pipelineStatistics	= ConvertPipelineStageMask(pDesc->PipelineStatisticsFlags);

		VkResult result = vkCreateQueryPool(m_pDevice->Device, &createInfo, nullptr, &m_QueryPool);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[QueryHeapVK]: Failed to create QueryPool");
			return false;
		}
		else
		{
			m_Desc = *pDesc;
			SetName(pDesc->DebugName);

			D_LOG_MESSAGE("[QueryHeapVK]: Created QueryPool");
			return true;
		}
	}

	void QueryHeapVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_QueryPool), VK_OBJECT_TYPE_QUERY_POOL);
		m_Desc.DebugName = debugName;
	}
	
	bool QueryHeapVK::GetResults(uint32 firstQuery, uint32 queryCount, uint64* pData) const
	{
		const uint64 dataSize = queryCount * sizeof(uint64);
		VkResult result = vkGetQueryPoolResults(m_pDevice->Device, m_QueryPool, firstQuery, queryCount, dataSize, reinterpret_cast<void*>(pData), sizeof(uint64), VK_QUERY_RESULT_64_BIT);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[QueryHeapVK]: Failed to retrive query results");
			return false;
		}
		else
		{
			return true;
		}
	}
}