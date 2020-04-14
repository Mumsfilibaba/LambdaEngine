#include "Log/Log.h"

#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	CommandAllocatorVK::CommandAllocatorVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}
	
	CommandAllocatorVK::~CommandAllocatorVK()
	{
		if (m_CommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_pDevice->Device, m_CommandPool, nullptr);
			m_CommandPool = VK_NULL_HANDLE;
		}
	}
	
	bool CommandAllocatorVK::Init(const char* pName, ECommandQueueType queueType)
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.pNext			= nullptr;
		createInfo.flags			= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		createInfo.queueFamilyIndex = m_pDevice->GetQueueFamilyIndexFromQueueType(queueType);

		VkResult result = vkCreateCommandPool(m_pDevice->Device, &createInfo, nullptr, &m_CommandPool);
		if (result != VK_SUCCESS)
		{
			if (pName)
			{
				LOG_VULKAN_ERROR(result, "[CommandAllocatorVK]: Failed to create commandpool \"%s\"", pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[CommandAllocatorVK]: Failed to create commandpool");
			}

			return false;
		}
		else
		{
			m_Type = queueType;

			if (pName)
			{
				LOG_MESSAGE("[CommandAllocatorVK]: Created commandpool \"%s\"", pName);
				SetName(pName);
			}
			else
			{
				LOG_MESSAGE("[CommandAllocatorVK]: Created commandpool");
			}

			return true;
		}
	}

	VkCommandBuffer CommandAllocatorVK::AllocateCommandBuffer(VkCommandBufferLevel level)
	{
		VkCommandBufferAllocateInfo allocateInfo = { };
		allocateInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.pNext				= nullptr;
		allocateInfo.level				= level;
		allocateInfo.commandPool		= m_CommandPool;
		allocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		VkResult result = vkAllocateCommandBuffers(m_pDevice->Device, &allocateInfo, &commandBuffer);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandAllocatorVK]: Failed to allocate commandbuffer");
			commandBuffer = VK_NULL_HANDLE;
		}

		return commandBuffer;
	}

	void CommandAllocatorVK::FreeCommandBuffer(VkCommandBuffer commandBuffer)
	{
		vkFreeCommandBuffers(m_pDevice->Device, m_CommandPool, 1, &commandBuffer);
	}
	
	bool CommandAllocatorVK::Reset()
	{
		VkResult result = vkResetCommandPool(m_pDevice->Device, m_CommandPool, 0);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandAllocatorVK]: Failed to reset commandpool");
			return false; 
		}

		return true;
	}

	void CommandAllocatorVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_CommandPool, VK_OBJECT_TYPE_COMMAND_POOL);
		}
	}
}