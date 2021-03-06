#include "Log/Log.h"

#include "Rendering/RenderSystem.h"

#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/Core/API/DeviceAllocator.h"

#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
	GraphicsDevice*		RenderSystem::s_pGraphicsDevice = nullptr;
	
	TSharedRef<CommandQueue>	RenderSystem::s_GraphicsQueue	= nullptr;
	TSharedRef<CommandQueue>	RenderSystem::s_ComputeQueue	= nullptr;
	TSharedRef<CommandQueue>	RenderSystem::s_CopyQueue		= nullptr;

	/*
	* RenderSystem
	*/
	bool RenderSystem::Init()
	{
		GraphicsDeviceDesc deviceDesc = { };
#ifdef LAMBDA_DEVELOPMENT
		deviceDesc.Debug = true;
#else
		deviceDesc.Debug = false;
#endif

		s_pGraphicsDevice = CreateGraphicsDevice(EGraphicsAPI::VULKAN, &deviceDesc);
		if (!s_pGraphicsDevice)
		{
			return false;
		}

		s_GraphicsQueue = s_pGraphicsDevice->CreateCommandQueue("Graphics Queue", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
		if (!s_GraphicsQueue)
		{
			return false;
		}

		s_ComputeQueue	= s_pGraphicsDevice->CreateCommandQueue("Compute Queue", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
		if (!s_ComputeQueue)
		{
			return false;
		}

		s_CopyQueue = s_pGraphicsDevice->CreateCommandQueue("Copy Queue", ECommandQueueType::COMMAND_QUEUE_TYPE_COPY);
		if (!s_CopyQueue)
		{
			return false;
		}

		return true;
	}

	bool RenderSystem::Release()
	{
		s_GraphicsQueue->Flush();
		s_ComputeQueue->Flush();
		s_CopyQueue->Flush();

		SAFERELEASE(s_pGraphicsDevice);
		return true;
	}
	
	void RenderSystem::Tick()
	{
	}
}
