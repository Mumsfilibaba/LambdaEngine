#include "Rendering/RenderGraph.h"
#include "Rendering/ICustomRenderer.h"
#include "Rendering/ImGuiRenderer.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/GraphicsHelpers.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/Shader.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/PipelineStateManager.h"

#include "Game/Scene.h"

#include "Log/Log.h"

#include "Application/API/CommonApplication.h"

namespace LambdaEngine
{
	constexpr const uint32 SAME_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX	= 0;
	constexpr const uint32 SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX				= 1;
	constexpr const uint32 OTHER_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX	= 2;
	constexpr const uint32 OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX			= 3;

	constexpr const uint32 SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX			= 0;
	constexpr const uint32 OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX			= 1;

	RenderGraph::RenderGraph(const GraphicsDevice* pGraphicsDevice) 
		: m_pGraphicsDevice(pGraphicsDevice)
	{
		CommonApplication::Get()->AddEventHandler(this);
	}

	RenderGraph::~RenderGraph()
	{
        m_pFence->Wait(m_SignalValue - 1, UINT64_MAX);
        SAFERELEASE(m_pFence);

        SAFERELEASE(m_pDescriptorHeap);
		SAFEDELETE_ARRAY(m_ppExecutionStages);

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppGraphicsCopyCommandAllocators[b]);
			SAFERELEASE(m_ppGraphicsCopyCommandLists[b]);

			SAFERELEASE(m_ppComputeCopyCommandAllocators[b]);
			SAFERELEASE(m_ppComputeCopyCommandLists[b]);
		}

		SAFEDELETE_ARRAY(m_ppGraphicsCopyCommandAllocators);
		SAFEDELETE_ARRAY(m_ppGraphicsCopyCommandLists);

		SAFEDELETE_ARRAY(m_ppComputeCopyCommandAllocators);
		SAFEDELETE_ARRAY(m_ppComputeCopyCommandLists);

		for (auto it = m_ResourceMap.begin(); it != m_ResourceMap.end(); it++)
		{
			Resource* pResource = &it->second;

			if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
			{
				if (pResource->Type == ERenderGraphResourceType::BUFFER)
				{
					for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
					{
						SAFERELEASE(pResource->Buffer.Buffers[sr]);
					}
				}
				else if (pResource->Type == ERenderGraphResourceType::TEXTURE)
				{
					for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
					{
						SAFERELEASE(pResource->Texture.Textures[sr]);
						SAFERELEASE(pResource->Texture.TextureViews[sr]);
						SAFERELEASE(pResource->Texture.Samplers[sr]);
					}
				}
			}
		}

		for (uint32 i = 0; i < m_PipelineStageCount; i++)
		{
			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(pPipelineStage->ppComputeCommandAllocators[b]);
				SAFERELEASE(pPipelineStage->ppGraphicsCommandAllocators[b]);
				SAFERELEASE(pPipelineStage->ppComputeCommandLists[b]);
				SAFERELEASE(pPipelineStage->ppGraphicsCommandLists[b]);
			}

			SAFEDELETE_ARRAY(pPipelineStage->ppComputeCommandAllocators);
			SAFEDELETE_ARRAY(pPipelineStage->ppGraphicsCommandAllocators);
			SAFEDELETE_ARRAY(pPipelineStage->ppComputeCommandLists);
			SAFEDELETE_ARRAY(pPipelineStage->ppGraphicsCommandLists);

			if (pPipelineStage->Type == ERenderGraphPipelineStageType::RENDER)
			{
				RenderStage* pRenderStage = &m_pRenderStages[pPipelineStage->StageIndex];

				for (uint32 b = 0; b < m_BackBufferCount; b++)
				{
					if (pRenderStage->ppTextureDescriptorSets != nullptr)
					{
						for (uint32 s = 0; s < pRenderStage->TextureSubDescriptorSetCount; s++)
						{
							SAFERELEASE(pRenderStage->ppTextureDescriptorSets[b * pRenderStage->TextureSubDescriptorSetCount + s]);
						}
					}

					if (pRenderStage->ppBufferDescriptorSets != nullptr)
						SAFERELEASE(pRenderStage->ppBufferDescriptorSets[b]);
				}
				
				SAFEDELETE_ARRAY(pRenderStage->ppTextureDescriptorSets);
				SAFEDELETE_ARRAY(pRenderStage->ppBufferDescriptorSets);
				SAFERELEASE(pRenderStage->pPipelineLayout);
				SAFERELEASE(pRenderStage->pRenderPass);
				PipelineStateManager::ReleasePipelineState(pRenderStage->PipelineStateID);
			}
			else if (pPipelineStage->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
			{
				SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pPipelineStage->StageIndex];

				UNREFERENCED_VARIABLE(pSynchronizationStage);
			}
		}

		SAFEDELETE_ARRAY(m_pRenderStages);
		SAFEDELETE_ARRAY(m_pSynchronizationStages);
		SAFEDELETE_ARRAY(m_pPipelineStages);

		for (uint32 r = 0; r < m_DebugRenderers.GetSize(); r++)
		{
			SAFEDELETE(m_DebugRenderers[r]);
		}

		m_DebugRenderers.Clear();
	}

	bool RenderGraph::Init(const RenderGraphDesc* pDesc)
	{
		m_pScene						= pDesc->pScene;
		m_BackBufferCount				= pDesc->BackBufferCount;
		m_MaxTexturesPerDescriptorSet	= pDesc->MaxTexturesPerDescriptorSet;

		if (!CreateFence())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Fence", pDesc->Name.c_str());
			return false;
		}

		if (!CreateDescriptorHeap())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Descriptor Heap", pDesc->Name.c_str());
			return false;
		}

		if (!CreateCopyCommandLists())
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Copy Command Lists", pDesc->Name.c_str());
			return false;
		}

		if (!CreateResources(pDesc->pRenderGraphStructureDesc->ResourceDescriptions))
		{ 
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Resources", pDesc->Name.c_str());
			return false;
		}

		if (!CreateRenderStages(pDesc->pRenderGraphStructureDesc->RenderStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Render Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CreateSynchronizationStages(pDesc->pRenderGraphStructureDesc->SynchronizationStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Synchronization Stages", pDesc->Name.c_str());
			return false;
		}

		if (!CreatePipelineStages(pDesc->pRenderGraphStructureDesc->PipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraph]: Render Graph \"%s\" failed to create Pipeline Stages", pDesc->Name.c_str());
			return false;
		}

		return true;
	}

	void RenderGraph::UpdateResource(const ResourceUpdateDesc& desc)
	{
		auto it = m_ResourceMap.find(desc.ResourceName);

		if (it != m_ResourceMap.end())
		{
			Resource* pResource = &it->second;

			switch (pResource->Type)
			{
				case ERenderGraphResourceType::TEXTURE:					UpdateResourceTexture(pResource, desc);					break;
				case ERenderGraphResourceType::BUFFER:					UpdateResourceBuffer(pResource, desc);					break;
				case ERenderGraphResourceType::ACCELERATION_STRUCTURE:	UpdateResourceAccelerationStructure(pResource, desc);	break;
				default:
				{
					LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph has unsupported Type", desc.ResourceName.c_str());
					return;
				}
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph could not be found in Resource Map", desc.ResourceName.c_str());
			return;
		}
	}

	void RenderGraph::UpdateRenderStageDimensions(const String& renderStageName, uint32 x, uint32 y, uint32 z)
	{
		auto it = m_RenderStageMap.find(renderStageName);

		if (it != m_RenderStageMap.end())
		{
			RenderStage* pRenderStage = &m_pRenderStages[it->second];

			if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::EXTERNAL) pRenderStage->Dimensions.x = x;
			if (pRenderStage->Parameters.YDimType == ERenderGraphDimensionType::EXTERNAL) pRenderStage->Dimensions.y = y;
			if (pRenderStage->Parameters.ZDimType == ERenderGraphDimensionType::EXTERNAL) pRenderStage->Dimensions.z = z;
		}
		else
		{
			LOG_WARNING("[RenderGraph]: UpdateRenderStageParameters failed, render stage with name \"%s\" could not be found", renderStageName.c_str());
			return;
		}
	}

	void RenderGraph::UpdateResourceDimensions(const String& resourceName, uint32 x, uint32 y)
	{
		auto resourceDescIt = m_InternalResourceUpdateDescriptions.find(resourceName);

		if (resourceDescIt != m_InternalResourceUpdateDescriptions.end())
		{
			InternalResourceUpdateDesc* pResourceUpdateDesc = &resourceDescIt->second;

			switch (pResourceUpdateDesc->Type)
			{
				case ERenderGraphResourceType::TEXTURE:
				{
					if (pResourceUpdateDesc->TextureUpdate.XDimType == ERenderGraphDimensionType::EXTERNAL) pResourceUpdateDesc->TextureUpdate.TextureDesc.Width = x;
					if (pResourceUpdateDesc->TextureUpdate.YDimType == ERenderGraphDimensionType::EXTERNAL) pResourceUpdateDesc->TextureUpdate.TextureDesc.Height = y;
					m_DirtyInternalResources.insert(resourceName);
					break;
				}
				case ERenderGraphResourceType::BUFFER:
				{
					if (pResourceUpdateDesc->BufferUpdate.SizeType == ERenderGraphDimensionType::EXTERNAL) pResourceUpdateDesc->BufferUpdate.BufferDesc.SizeInBytes = x;
					m_DirtyInternalResources.insert(resourceName);
					break;
				}
				default:
				{
					LOG_WARNING("[RenderGraph]: UpdateResourceDimensions failed, resource \"%s\" has an unsupported type", resourceName.c_str());
					break;
				}
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: UpdateResourceDimensions failed, resource with name \"%s\" could not be found", resourceName.c_str());
			return;
		}
	}

	void RenderGraph::GetAndIncrementFence(Fence** ppFence, uint64* pSignalValue)
	{
		(*pSignalValue) = m_SignalValue++;
		(*ppFence) = m_pFence;
	}

	void RenderGraph::Update()
	{
		if (m_DirtyInternalResources.size() > 0)
		{
			for (const String& dirtyInternalResourceDescName : m_DirtyInternalResources)
			{
				UpdateInternalResource(m_InternalResourceUpdateDescriptions[dirtyInternalResourceDescName]);
			}

			m_DirtyInternalResources.clear();
		}

		if (m_DirtyDescriptorSetBuffers.size()					> 0 || 
			m_DirtyDescriptorSetAccelerationStructures.size()	> 0)
		{
			//Copy old descriptor set and replace old with copy, then write into the new copy
			for (uint32 r = 0; r < m_RenderStageCount; r++)
			{
				RenderStage* pRenderStage		= &m_pRenderStages[r];

				if (pRenderStage->ppBufferDescriptorSets != nullptr)
				{
					for (uint32 b = 0; b < m_BackBufferCount; b++)
					{
						DescriptorSet* pSrcDescriptorSet	= pRenderStage->ppBufferDescriptorSets[b];
						DescriptorSet* pDescriptorSet		= m_pGraphicsDevice->CreateDescriptorSet(pSrcDescriptorSet->GetName(), pRenderStage->pPipelineLayout, 0, m_pDescriptorHeap);
						m_pGraphicsDevice->CopyDescriptorSet(pSrcDescriptorSet, pDescriptorSet);
						SAFERELEASE(pSrcDescriptorSet);
						pRenderStage->ppBufferDescriptorSets[b] = pDescriptorSet;
					}
				}
				else if (pRenderStage->UsesCustomRenderer)
				{
					pRenderStage->pCustomRenderer->PreBuffersDescriptorSetWrite();
				}
			}

			if (m_DirtyDescriptorSetBuffers.size() > 0)
			{
				for (Resource* pResource : m_DirtyDescriptorSetBuffers)
				{
					for (uint32 rb = 0; rb < pResource->ResourceBindings.GetSize(); rb++)
					{
						ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
						RenderStage* pRenderStage = pResourceBinding->pRenderStage;

						if (pRenderStage->UsesCustomRenderer)
						{
							pRenderStage->pCustomRenderer->UpdateBufferResource(
								pResource->Name,
								pResource->Buffer.Buffers.GetData(),
								pResource->Buffer.Offsets.GetData(),
								pResource->Buffer.SizesInBytes.GetData(),
								pResource->SubResourceCount,
								pResource->BackBufferBound);
						}
						else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
						{
							if (pResource->BackBufferBound)
							{
								for (uint32 b = 0; b < m_BackBufferCount; b++)
								{
									pResourceBinding->pRenderStage->ppBufferDescriptorSets[b]->WriteBufferDescriptors(
										&pResource->Buffer.Buffers[b],
										&pResource->Buffer.Offsets[b],
										&pResource->Buffer.SizesInBytes[b],
										pResourceBinding->Binding,
										1,
										pResourceBinding->DescriptorType);
								}
							}
							else
							{
								for (uint32 b = 0; b < m_BackBufferCount; b++)
								{
									pResourceBinding->pRenderStage->ppBufferDescriptorSets[b]->WriteBufferDescriptors(
										pResource->Buffer.Buffers.GetData(),
										pResource->Buffer.Offsets.GetData(),
										pResource->Buffer.SizesInBytes.GetData(),
										pResourceBinding->Binding,
										pResource->SubResourceCount,
										pResourceBinding->DescriptorType);
								}
							}
						}
					}
				}

				m_DirtyDescriptorSetBuffers.clear();
			}

			//Acceleration Structures
			if (m_DirtyDescriptorSetAccelerationStructures.size() > 0)
			{
				for (Resource* pResource : m_DirtyDescriptorSetAccelerationStructures)
				{
					if (!pResource->ResourceBindings.IsEmpty())
					{
						ResourceBinding* pResourceBinding = &pResource->ResourceBindings[0]; //Assume only one acceleration structure
						RenderStage* pRenderStage = pResourceBinding->pRenderStage;

						if (pRenderStage->UsesCustomRenderer)
						{
							pRenderStage->pCustomRenderer->UpdateAccelerationStructureResource(
								pResource->Name,
								pResource->AccelerationStructure.pTLAS);
						}
						else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pResourceBinding->pRenderStage->ppBufferDescriptorSets[b]->WriteAccelerationStructureDescriptors(
									&pResource->AccelerationStructure.pTLAS,
									pResourceBinding->Binding,
									1);
							}
						}
					}
				}

				m_DirtyDescriptorSetAccelerationStructures.clear();
			}
		}

		if (m_DirtyDescriptorSetTextures.size() > 0)
		{
			//Copy old descriptor set and replace old with copy, then write into the new copy
			for (uint32 r = 0; r < m_RenderStageCount; r++)
			{
				RenderStage* pRenderStage = &m_pRenderStages[r];

				if (pRenderStage->ppTextureDescriptorSets != nullptr)
				{
					for (uint32 b = 0; b < m_BackBufferCount; b++)
					{
						for (uint32 s = 0; s < pRenderStage->TextureSubDescriptorSetCount; s++)
						{
							uint32 descriptorSetIndex = b * pRenderStage->TextureSubDescriptorSetCount + s;

							DescriptorSet* pSrcDescriptorSet	= pRenderStage->ppTextureDescriptorSets[descriptorSetIndex];
							DescriptorSet* pDescriptorSet		= m_pGraphicsDevice->CreateDescriptorSet(pSrcDescriptorSet->GetName(), pRenderStage->pPipelineLayout, pRenderStage->ppBufferDescriptorSets != nullptr ? 1 : 0, m_pDescriptorHeap);
							m_pGraphicsDevice->CopyDescriptorSet(pSrcDescriptorSet, pDescriptorSet);
							SAFERELEASE(pSrcDescriptorSet);
							pRenderStage->ppTextureDescriptorSets[descriptorSetIndex] = pDescriptorSet;
						}
					}
				}
				else if (pRenderStage->UsesCustomRenderer)
				{
					pRenderStage->pCustomRenderer->PreTexturesDescriptorSetWrite();
				}
			}

			for (Resource* pResource : m_DirtyDescriptorSetTextures)
			{
				for (uint32 rb = 0; rb < pResource->ResourceBindings.GetSize(); rb++)
				{
					ResourceBinding* pResourceBinding = &pResource->ResourceBindings[rb];
					RenderStage* pRenderStage = pResourceBinding->pRenderStage;

					if (pRenderStage->UsesCustomRenderer)
					{
						pRenderStage->pCustomRenderer->UpdateTextureResource(
							pResource->Name,
							pResource->Texture.TextureViews.GetData(),
							pResource->Texture.IsOfArrayType ? 1 : pResource->SubResourceCount,
							pResource->BackBufferBound);
					}
					else if (pResourceBinding->DescriptorType != EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
					{
						if (pResource->BackBufferBound)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								pRenderStage->ppTextureDescriptorSets[b]->WriteTextureDescriptors(
									&pResource->Texture.TextureViews[b],
									&pResource->Texture.Samplers[b],
									pResourceBinding->TextureState,
									pResourceBinding->Binding,
									1,
									pResourceBinding->DescriptorType);
							}
						}
						else
						{
							uint32 actualSubResourceCount = (uint32)glm::ceil(pResource->Texture.IsOfArrayType ? 1.0f : float32(pResource->SubResourceCount) / float32(pRenderStage->TextureSubDescriptorSetCount));

							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								for (uint32 s = 0; s < pRenderStage->TextureSubDescriptorSetCount; s++)
								{
									uint32 descriptorSetIndex = b * pRenderStage->TextureSubDescriptorSetCount + s;
									uint32 subResourceIndex = s * actualSubResourceCount;

									pRenderStage->ppTextureDescriptorSets[descriptorSetIndex]->WriteTextureDescriptors(
										&pResource->Texture.TextureViews[subResourceIndex],
										&pResource->Texture.Samplers[subResourceIndex],
										pResourceBinding->TextureState,
										pResourceBinding->Binding,
										actualSubResourceCount,
										pResourceBinding->DescriptorType);
								}
							}
						}
					}
				}
			}

			m_DirtyDescriptorSetTextures.clear();
		}
	}

	void RenderGraph::NewFrame(uint64 modFrameIndex, uint32 backBufferIndex, Timestamp delta)
	{
		m_ModFrameIndex = modFrameIndex;
		m_BackBufferIndex = backBufferIndex;

		for (uint32 r = 0; r < (uint32)m_CustomRenderers.GetSize(); r++)
		{
			ICustomRenderer* pCustomRenderer = m_CustomRenderers[r];
			pCustomRenderer->NewFrame(delta);
		}
	}

	void RenderGraph::PrepareRender(Timestamp delta)
	{
		for (uint32 r = 0; r < (uint32)m_CustomRenderers.GetSize(); r++)
		{
			ICustomRenderer* pCustomRenderer = m_CustomRenderers[r];
			pCustomRenderer->PrepareRender(delta);
		}
	}

	void RenderGraph::Render()
	{
		ZERO_MEMORY(m_ppExecutionStages, m_ExecutionStageCount * sizeof(CommandList*));

		uint32 currentExecutionStage = 0;

		if (m_SignalValue > 3)
			m_pFence->Wait(m_SignalValue - 3, UINT64_MAX);

		for (uint32 p = 0; p < m_PipelineStageCount; p++)
		{
			//Seperate Thread
			{
				PipelineStage* pPipelineStage = &m_pPipelineStages[p];

				if (pPipelineStage->Type == ERenderGraphPipelineStageType::RENDER)
				{
					RenderStage* pRenderStage = &m_pRenderStages[pPipelineStage->StageIndex];
					PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(pRenderStage->PipelineStateID);
					EPipelineStateType stateType = pPipelineState->GetType();

					if (pRenderStage->UsesCustomRenderer)
					{
						ICustomRenderer* pCustomRenderer = pRenderStage->pCustomRenderer;
						switch (stateType)
						{
						case EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS:		pCustomRenderer->Render(pPipelineStage->ppGraphicsCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppGraphicsCommandLists[m_ModFrameIndex],m_ModFrameIndex, m_BackBufferIndex, &m_ppExecutionStages[currentExecutionStage]); break;
						case EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE:		pCustomRenderer->Render(pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],	m_ModFrameIndex, m_BackBufferIndex, &m_ppExecutionStages[currentExecutionStage]); break;
						case EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING:	pCustomRenderer->Render(pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],	pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],	m_ModFrameIndex, m_BackBufferIndex, &m_ppExecutionStages[currentExecutionStage]); break;
						}
					}
					else
					{
						switch (stateType)
						{
						case EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS:		ExecuteGraphicsRenderStage(pRenderStage,	pPipelineState, pPipelineStage->ppGraphicsCommandAllocators[m_ModFrameIndex],		pPipelineStage->ppGraphicsCommandLists[m_ModFrameIndex],	&m_ppExecutionStages[currentExecutionStage]);	break;
						case EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE:		ExecuteComputeRenderStage(pRenderStage,		pPipelineState, pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],		pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],		&m_ppExecutionStages[currentExecutionStage]);	break;
						case EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING:	ExecuteRayTracingRenderStage(pRenderStage,	pPipelineState, pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],		pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],		&m_ppExecutionStages[currentExecutionStage]);	break;
						}
					}
					currentExecutionStage++;
				}
				else if (pPipelineStage->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
				{
					SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pPipelineStage->StageIndex];

					ExecuteSynchronizationStage(
						pSynchronizationStage,
						pPipelineStage->ppGraphicsCommandAllocators[m_ModFrameIndex],
						pPipelineStage->ppGraphicsCommandLists[m_ModFrameIndex],
						pPipelineStage->ppComputeCommandAllocators[m_ModFrameIndex],
						pPipelineStage->ppComputeCommandLists[m_ModFrameIndex],
						&m_ppExecutionStages[currentExecutionStage],
						&m_ppExecutionStages[currentExecutionStage + 1]);

					currentExecutionStage += 2;
				}
			}
		}

		//Execute Copy Command Lists
		{
			if (m_ExecuteGraphicsCopy)
			{
				m_ExecuteGraphicsCopy = false;

				CommandList* pGraphicsCopyCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

				pGraphicsCopyCommandList->End();
				RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&pGraphicsCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				m_SignalValue++;
			}

			if (m_ExecuteComputeCopy)
			{
				m_ExecuteComputeCopy = false;

				CommandList* pComputeCopyCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

				pComputeCopyCommandList->End();
				RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pComputeCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				m_SignalValue++;
			}
		}

		//Wait Threads

		//Execute the recorded Command Lists, we do this in a Batched mode where we batch as many "same queue" command lists that execute in succession together. This reduced the overhead caused by QueueSubmit
		{
			//This is safe since the first Execution Stage should never be nullptr
			ECommandQueueType currentBatchType = m_ppExecutionStages[0]->GetType();

			static TArray<CommandList*> currentBatch;

			for (uint32 e = 0; e < m_ExecutionStageCount; e++)
			{
				CommandList* pCommandList = m_ppExecutionStages[e];

				if (pCommandList != nullptr)
				{
					ECommandQueueType currentType = pCommandList->GetType();

					if (currentType != currentBatchType)
					{
						if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
						{
							RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(currentBatch.GetData(), currentBatch.GetSize(), FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
						}
						else if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
						{
							RenderSystem::GetComputeQueue()->ExecuteCommandLists(currentBatch.GetData(), currentBatch.GetSize(), FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
						}

						m_SignalValue++;
						currentBatch.Clear();

						currentBatchType = currentType;
					}

					currentBatch.PushBack(pCommandList);
				}
			}

			if (!currentBatch.IsEmpty())
			{
				if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
				{
					RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(currentBatch.GetData(), currentBatch.GetSize(), FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				}
				else if (currentBatchType == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
				{
					RenderSystem::GetComputeQueue()->ExecuteCommandLists(currentBatch.GetData(), currentBatch.GetSize(), FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, m_pFence, m_SignalValue - 1, m_pFence, m_SignalValue);
				}

				m_SignalValue++;
				currentBatch.Clear();
			}
		}
	}

	CommandList* RenderGraph::AcquireGraphicsCopyCommandList()
	{
		CommandList* pCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

		if (!m_ExecuteGraphicsCopy)
		{
			m_ExecuteGraphicsCopy = true;

			m_ppGraphicsCopyCommandAllocators[m_ModFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
		}

		return pCommandList;
	}

	CommandList* RenderGraph::AcquireComputeCopyCommandList()
	{
		CommandList* pCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

		if (!m_ExecuteComputeCopy)
		{
			m_ExecuteComputeCopy = true;

			m_ppComputeCopyCommandAllocators[m_ModFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
		}

		return pCommandList;
	}

	bool RenderGraph::GetResourceTextures(const char* pResourceName, Texture* const ** pppTexture, uint32* pTextureView) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppTexture)		= it->second.Texture.Textures.GetData();
			(*pTextureView)		= (uint32)it->second.Texture.Textures.GetSize();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourceTextureViews(const char* pResourceName, TextureView* const ** pppTextureViews, uint32* pTextureViewCount) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppTextureViews)		= it->second.Texture.TextureViews.GetData();
			(*pTextureViewCount)	= (uint32)it->second.Texture.TextureViews.GetSize();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourceBuffers(const char* pResourceName, Buffer* const ** pppBuffers, uint32* pBufferCount) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*pppBuffers)			= it->second.Buffer.Buffers.GetData();
			(*pBufferCount)			= (uint32)it->second.Buffer.Buffers.GetSize();
			return true;
		}

		return false;
	}

	bool RenderGraph::GetResourceAccelerationStructure(const char* pResourceName, AccelerationStructure const ** ppAccelerationStructure) const
	{
		auto it = m_ResourceMap.find(pResourceName);

		if (it != m_ResourceMap.end())
		{
			(*ppAccelerationStructure) = it->second.AccelerationStructure.pTLAS;
			return true;
		}

		return false;
	}

	void RenderGraph::OnWindowResized(TSharedRef<Window> window, uint16 width, uint16 height, EResizeType type)
	{
		m_WindowWidth	= (float32)width;
		m_WindowHeight	= (float32)height;

		for (uint32 renderStageIndex : m_WindowRelativeRenderStages)
		{
			RenderStage* pRenderStage = &m_pRenderStages[renderStageIndex];

			UpdateRelativeRenderStageDimensions(pRenderStage);
		}

		for (const String& resourceName : m_WindowRelativeResources)
		{
			InternalResourceUpdateDesc* pInternalResourceUpdateDesc = &m_InternalResourceUpdateDescriptions[resourceName];

			UpdateRelativeResourceDimensions(pInternalResourceUpdateDesc);
		}
	}

	bool RenderGraph::CreateFence()
	{
		FenceDesc fenceDesc = {};
		fenceDesc.DebugName		= "Render Stage Fence";
		fenceDesc.InitalValue	= 0;

		m_pFence = m_pGraphicsDevice->CreateFence(&fenceDesc);

		if (m_pFence == nullptr)
		{
			LOG_ERROR("[RenderGraph]: Could not create RenderGraph fence");
			return false;
		}

		return true;
	}

	bool RenderGraph::CreateDescriptorHeap()
	{
		constexpr uint32 DESCRIPTOR_COUNT = 1024;

		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.ConstantBufferDescriptorCount				= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.AccelerationStructureDescriptorCount		= DESCRIPTOR_COUNT;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName			= "Render Graph Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount	= DESCRIPTOR_COUNT;
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_pDescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);

		return m_pDescriptorHeap != nullptr;
	}

	bool RenderGraph::CreateCopyCommandLists()
	{
		m_ppGraphicsCopyCommandAllocators		= DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppGraphicsCopyCommandLists			= DBG_NEW CommandList*[m_BackBufferCount];
		m_ppComputeCopyCommandAllocators		= DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppComputeCopyCommandLists				= DBG_NEW CommandList *[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			//Graphics
			{
				m_ppGraphicsCopyCommandAllocators[b]		= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Graphics Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

				if (m_ppGraphicsCopyCommandAllocators[b] == nullptr)
				{
					return false;
				}

				CommandListDesc graphicsCopyCommandListDesc = {};
				graphicsCopyCommandListDesc.DebugName				= "Render Graph Graphics Copy Command List";
				graphicsCopyCommandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				graphicsCopyCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				m_ppGraphicsCopyCommandLists[b]			= m_pGraphicsDevice->CreateCommandList(m_ppGraphicsCopyCommandAllocators[b], &graphicsCopyCommandListDesc);

				if (m_ppGraphicsCopyCommandLists[b] == nullptr)
				{
					return false;
				}
			}

			//Compute
			{
				m_ppComputeCopyCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("Render Graph Compute Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

				if (m_ppComputeCopyCommandAllocators[b] == nullptr)
				{
					return false;
				}

				CommandListDesc computeCopyCommandListDesc = {};
				computeCopyCommandListDesc.DebugName				= "Render Graph Compute Copy Command List";
				computeCopyCommandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				computeCopyCommandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				m_ppComputeCopyCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ppComputeCopyCommandAllocators[b], &computeCopyCommandListDesc);

				if (m_ppComputeCopyCommandLists[b] == nullptr)
				{
					return false;
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreateResources(const TArray<RenderGraphResourceDesc>& resources)
	{
		m_ResourceMap.reserve(resources.GetSize());

		for (uint32 i = 0; i < resources.GetSize(); i++)
		{
			const RenderGraphResourceDesc* pResourceDesc = &resources[i];

			Resource* pResource = &m_ResourceMap[pResourceDesc->Name];

			pResource->Name				= pResourceDesc->Name;
			pResource->IsBackBuffer		= pResourceDesc->Name == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			pResource->BackBufferBound	= pResource->IsBackBuffer || pResourceDesc->BackBufferBound;

			if (pResource->BackBufferBound)
			{
				pResource->SubResourceCount = m_BackBufferCount;
			}
			else
			{
				pResource->SubResourceCount = pResourceDesc->SubResourceCount;
			}
					
			if (!pResourceDesc->External)
			{
				//Internal
				if (pResourceDesc->Type == ERenderGraphResourceType::TEXTURE)
				{
					pResource->Type						= ERenderGraphResourceType::TEXTURE;
					pResource->OwnershipType			= pResource->IsBackBuffer ? EResourceOwnershipType::EXTERNAL : EResourceOwnershipType::INTERNAL;
					pResource->Texture.IsOfArrayType	= pResourceDesc->TextureParams.IsOfArrayType;
					pResource->Texture.Format			= pResourceDesc->TextureParams.TextureFormat;

					uint32 actualSubResourceCount = pResourceDesc->TextureParams.IsOfArrayType ? 1 : pResource->SubResourceCount;
					pResource->Texture.Textures.Resize(actualSubResourceCount);
					pResource->Texture.TextureViews.Resize(actualSubResourceCount);
					pResource->Texture.Samplers.Resize(actualSubResourceCount);

					if (!pResource->IsBackBuffer)
					{
						uint32 arrayCount = pResourceDesc->TextureParams.IsOfArrayType ? pResource->SubResourceCount : 1;

						TextureDesc		textureDesc		= {};
						TextureViewDesc textureViewDesc = {};
						SamplerDesc		samplerDesc		= {};

						textureDesc.DebugName			= pResourceDesc->Name + " Texture";
						textureDesc.MemoryType			= pResourceDesc->MemoryType;
						textureDesc.Format				= pResourceDesc->TextureParams.TextureFormat;
						textureDesc.Type				= ETextureType::TEXTURE_TYPE_2D;
						textureDesc.Flags				= pResourceDesc->TextureParams.TextureFlags;
						textureDesc.Width				= pResourceDesc->TextureParams.XDimVariable;
						textureDesc.Height				= pResourceDesc->TextureParams.YDimVariable;
						textureDesc.Depth				= 1;
						textureDesc.ArrayCount			= arrayCount;
						textureDesc.Miplevels			= pResourceDesc->TextureParams.MiplevelCount;
						textureDesc.SampleCount			= pResourceDesc->TextureParams.SampleCount;

						textureViewDesc.DebugName		= pResourceDesc->Name + " Texture View";
						textureViewDesc.Texture			= nullptr;
						textureViewDesc.Flags			= pResourceDesc->TextureParams.TextureViewFlags;
						textureViewDesc.Format			= pResourceDesc->TextureParams.TextureFormat;
						textureViewDesc.Type			= pResourceDesc->TextureParams.IsOfArrayType ? ETextureViewType::TEXTURE_VIEW_TYPE_2D_ARRAY : ETextureViewType::TEXTURE_VIEW_TYPE_2D;
						textureViewDesc.MiplevelCount	= pResourceDesc->TextureParams.MiplevelCount;
						textureViewDesc.ArrayCount		= arrayCount;
						textureViewDesc.Miplevel		= 0;
						textureViewDesc.ArrayIndex		= 0;

						samplerDesc.DebugName			= pResourceDesc->Name + " Sampler";
						samplerDesc.MinFilter			= RenderGraphSamplerToFilter(pResourceDesc->TextureParams.SamplerType);
						samplerDesc.MagFilter			= RenderGraphSamplerToFilter(pResourceDesc->TextureParams.SamplerType);
						samplerDesc.MipmapMode			= RenderGraphSamplerToMipmapMode(pResourceDesc->TextureParams.SamplerType);
						samplerDesc.AddressModeU		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
						samplerDesc.AddressModeV		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
						samplerDesc.AddressModeW		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
						samplerDesc.MipLODBias			= 0.0f;
						samplerDesc.AnisotropyEnabled	= false;
						samplerDesc.MaxAnisotropy		= 16;
						samplerDesc.MinLOD				= 0.0f;
						samplerDesc.MaxLOD				= 1.0f;

						InternalResourceUpdateDesc internalResourceUpdateDesc = {};
						internalResourceUpdateDesc.ResourceName						= pResourceDesc->Name;
						internalResourceUpdateDesc.Type								= ERenderGraphResourceType::TEXTURE;
						internalResourceUpdateDesc.TextureUpdate.XDimType			= pResourceDesc->TextureParams.XDimType;
						internalResourceUpdateDesc.TextureUpdate.YDimType			= pResourceDesc->TextureParams.YDimType;
						internalResourceUpdateDesc.TextureUpdate.XDimVariable		= pResourceDesc->TextureParams.XDimVariable;
						internalResourceUpdateDesc.TextureUpdate.YDimVariable		= pResourceDesc->TextureParams.YDimVariable;
						internalResourceUpdateDesc.TextureUpdate.TextureDesc		= textureDesc;
						internalResourceUpdateDesc.TextureUpdate.TextureViewDesc	= textureViewDesc;
						internalResourceUpdateDesc.TextureUpdate.SamplerDesc		= samplerDesc;

						m_InternalResourceUpdateDescriptions[pResourceDesc->Name]	= internalResourceUpdateDesc;
						m_DirtyInternalResources.insert(pResourceDesc->Name);

						if (pResourceDesc->TextureParams.XDimType == ERenderGraphDimensionType::RELATIVE ||
							pResourceDesc->TextureParams.YDimType == ERenderGraphDimensionType::RELATIVE)
						{
							m_WindowRelativeResources.PushBack(pResourceDesc->Name);
						}
					}
				}
				else if (pResourceDesc->Type == ERenderGraphResourceType::BUFFER)
				{
					pResource->Type				= ERenderGraphResourceType::BUFFER;
					pResource->OwnershipType	= EResourceOwnershipType::INTERNAL;
					pResource->Buffer.Buffers.Resize(pResource->SubResourceCount);
					pResource->Buffer.Offsets.Resize(pResource->SubResourceCount);
					pResource->Buffer.SizesInBytes.Resize(pResource->SubResourceCount);

					BufferDesc bufferDesc = {};
					//bufferDesc.pName		
					bufferDesc.MemoryType		= pResourceDesc->MemoryType;
					bufferDesc.Flags			= pResourceDesc->BufferParams.BufferFlags;
					bufferDesc.SizeInBytes		= pResourceDesc->BufferParams.Size;

					InternalResourceUpdateDesc internalResourceUpdateDesc = {};
					internalResourceUpdateDesc.ResourceName						= pResourceDesc->Name;
					internalResourceUpdateDesc.Type								= ERenderGraphResourceType::BUFFER;
					internalResourceUpdateDesc.BufferUpdate.SizeType			= pResourceDesc->BufferParams.SizeType;
					internalResourceUpdateDesc.BufferUpdate.BufferDesc			= bufferDesc;

					m_InternalResourceUpdateDescriptions[pResourceDesc->Name]	= internalResourceUpdateDesc;
					m_DirtyInternalResources.insert(pResourceDesc->Name);

					if (pResourceDesc->BufferParams.SizeType == ERenderGraphDimensionType::RELATIVE)
					{
						m_WindowRelativeResources.PushBack(pResourceDesc->Name);
					}
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Unsupported resource type for internal resource \"%s\"", pResource->Name.c_str());
					return false;
				}
			}
			else
			{
				//External
				if (pResourceDesc->Type == ERenderGraphResourceType::TEXTURE)
				{
					pResource->Type						= ERenderGraphResourceType::TEXTURE;
					pResource->OwnershipType			= EResourceOwnershipType::EXTERNAL;
					pResource->Texture.IsOfArrayType	= pResourceDesc->TextureParams.IsOfArrayType;
					pResource->Texture.Format			= pResourceDesc->TextureParams.TextureFormat;

					uint32 actualSubResourceCount = pResourceDesc->TextureParams.IsOfArrayType ? 1 : pResource->SubResourceCount;
					pResource->Texture.Textures.Resize(actualSubResourceCount);
					pResource->Texture.TextureViews.Resize(actualSubResourceCount);
					pResource->Texture.Samplers.Resize(actualSubResourceCount);
				}
				else if (pResourceDesc->Type == ERenderGraphResourceType::BUFFER)
				{
					pResource->Type				= ERenderGraphResourceType::BUFFER;
					pResource->OwnershipType	= EResourceOwnershipType::EXTERNAL;
					pResource->Buffer.Buffers.Resize(pResource->SubResourceCount);
					pResource->Buffer.Offsets.Resize(pResource->SubResourceCount);
					pResource->Buffer.SizesInBytes.Resize(pResource->SubResourceCount);
				}
				else if (pResourceDesc->Type == ERenderGraphResourceType::ACCELERATION_STRUCTURE)
				{ 
					pResource->Type				= ERenderGraphResourceType::ACCELERATION_STRUCTURE;
					pResource->OwnershipType	= EResourceOwnershipType::EXTERNAL;
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Unsupported resource type for external resource \"%s\"", pResource->Name.c_str());
					return false;
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreateRenderStages(const TArray<RenderStageDesc>& renderStages)
	{
		m_RenderStageCount = (uint32)renderStages.GetSize();
		m_RenderStageMap.reserve(m_RenderStageCount);
		m_pRenderStages = DBG_NEW RenderStage[m_RenderStageCount];

		for (uint32 renderStageIndex = 0; renderStageIndex < m_RenderStageCount; renderStageIndex++)
		{
			const RenderStageDesc* pRenderStageDesc = &renderStages[renderStageIndex];

			RenderStage* pRenderStage = &m_pRenderStages[renderStageIndex];
			m_RenderStageMap[pRenderStageDesc->Name] = renderStageIndex;

			pRenderStage->Name			= pRenderStageDesc->Name ;
			pRenderStage->Parameters	= pRenderStageDesc->Parameters;

			if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::RELATIVE ||
				pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::RELATIVE_1D ||
				pRenderStage->Parameters.YDimType == ERenderGraphDimensionType::RELATIVE)
			{
				m_WindowRelativeRenderStages.insert(renderStageIndex);

				UpdateRelativeRenderStageDimensions(pRenderStage);
			}

			if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.x = pRenderStageDesc->Parameters.XDimVariable;
			}

			if (pRenderStage->Parameters.YDimType == ERenderGraphDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.y = pRenderStageDesc->Parameters.YDimVariable;
			}

			if (pRenderStage->Parameters.ZDimType == ERenderGraphDimensionType::CONSTANT)
			{
				pRenderStage->Dimensions.z = pRenderStageDesc->Parameters.ZDimVariable;
			}

			pRenderStage->Dimensions.x = glm::max<uint32>(1, pRenderStage->Dimensions.x);
			pRenderStage->Dimensions.y = glm::max<uint32>(1, pRenderStage->Dimensions.y);
			pRenderStage->Dimensions.z = glm::max<uint32>(1, pRenderStage->Dimensions.z);

			//Calculate the total number of textures we want to bind
			uint32 textureSlots = 0;
			uint32 totalNumberOfTextures = 0;
			uint32 totalNumberOfNonMaterialTextures = 0;
			uint32 textureSubResourceCount = 0;
			bool textureSubResourceCountSame = true;
			for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.GetSize(); rs++)
			{
				const RenderGraphResourceState* pResourceStateDesc = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = m_ResourceMap.find(pResourceStateDesc->ResourceName);

				if (resourceIt == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying Resource", pResourceStateDesc->ResourceName.c_str());
					return false;
				}

				const Resource* pResource = &resourceIt->second;

				if (ResourceStateNeedsDescriptor(pResourceStateDesc->BindingType) && pResource->Type == ERenderGraphResourceType::TEXTURE)
				{
					textureSlots++;

					//Todo: Review this, this seems retarded
					if (pResourceStateDesc->BindingType == ERenderGraphResourceBindingType::COMBINED_SAMPLER)
					{
						//Samplers which are of array type only take up one slot, same for back buffer bound resources
						uint32 actualResourceSubResourceCount = (pResource->BackBufferBound || pResource->Texture.IsOfArrayType) ? 1 : pResource->SubResourceCount; 

						if (textureSubResourceCount > 0 && actualResourceSubResourceCount != textureSubResourceCount)
							textureSubResourceCountSame = false;

						textureSubResourceCount = actualResourceSubResourceCount;
						totalNumberOfTextures += textureSubResourceCount;

						if (pResource->Name != SCENE_ALBEDO_MAPS	 &&
							pResource->Name != SCENE_NORMAL_MAPS	 &&
							pResource->Name != SCENE_AO_MAPS		 &&
							pResource->Name != SCENE_ROUGHNESS_MAPS	 &&
							pResource->Name != SCENE_METALLIC_MAPS)
						{
							totalNumberOfNonMaterialTextures += textureSubResourceCount;
						}
					}
					else
					{
						totalNumberOfTextures++;
						totalNumberOfNonMaterialTextures++;
					}
				}
			}

			if (textureSlots > m_MaxTexturesPerDescriptorSet)
			{
				LOG_ERROR("[RenderGraph]: Number of required texture slots %u for render stage %s is more than MaxTexturesPerDescriptorSet %u", textureSlots, pRenderStageDesc->Name.c_str(), m_MaxTexturesPerDescriptorSet);
				return false;
			}
			else if (totalNumberOfTextures > m_MaxTexturesPerDescriptorSet && !textureSubResourceCountSame)
			{
				//If all texture have either 1 or the same subresource count we can divide the Render Stage into multiple passes, is this correct?
				LOG_ERROR("[RenderGraph]: Total number of required texture slots %u for render stage %s is more than MaxTexturesPerDescriptorSet %u. This only works if all texture bindings have either 1 or the same subresource count", totalNumberOfTextures, pRenderStageDesc->Name.c_str(), m_MaxTexturesPerDescriptorSet);
				return false;
			}
			pRenderStage->MaterialsRenderedPerPass = (m_MaxTexturesPerDescriptorSet - totalNumberOfNonMaterialTextures) / 5; //5 textures per material
			pRenderStage->TextureSubDescriptorSetCount = (uint32)glm::ceil((float)totalNumberOfTextures / float(pRenderStage->MaterialsRenderedPerPass * 5));

			TArray<DescriptorBindingDesc> textureDescriptorSetDescriptions;
			textureDescriptorSetDescriptions.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			uint32 textureDescriptorBindingIndex = 0;

			TArray<DescriptorBindingDesc> bufferDescriptorSetDescriptions;
			bufferDescriptorSetDescriptions.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			uint32 bufferDescriptorBindingIndex = 0;

			TArray<RenderPassAttachmentDesc>								renderPassAttachmentDescriptions;
			RenderPassAttachmentDesc										renderPassDepthStencilDescription;
			TArray<ETextureState>											renderPassRenderTargetStates;
			TArray<BlendAttachmentStateDesc>								renderPassBlendAttachmentStates;
			TArray<std::pair<Resource*, ETextureState>>						renderStageRenderTargets;
			Resource*														pDepthStencilResource = nullptr;
			TArray<std::tuple<Resource*, ETextureState, EDescriptorType>>	renderStageTextureResources;
			TArray<std::tuple<Resource*, ETextureState, EDescriptorType>>	renderStageBufferResources;
			renderPassAttachmentDescriptions.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderPassRenderTargetStates.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderPassBlendAttachmentStates.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderStageRenderTargets.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderStageTextureResources.Reserve(pRenderStageDesc->ResourceStates.GetSize());
			renderStageBufferResources.Reserve(pRenderStageDesc->ResourceStates.GetSize());

			//Create Descriptors and RenderPass Attachments from RenderStage Resource States
			for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.GetSize(); rs++)
			{
				const RenderGraphResourceState* pResourceStateDesc = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = m_ResourceMap.find(pResourceStateDesc->ResourceName);

				if (resourceIt == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource State with name \"%s\" has no accompanying Resource", pResourceStateDesc->ResourceName.c_str());
					return false;
				}

				Resource* pResource = &resourceIt->second;

				//Descriptors
				if (ResourceStateNeedsDescriptor(pResourceStateDesc->BindingType))
				{
					EDescriptorType descriptorType		= CalculateResourceStateDescriptorType(pResource->Type, pResourceStateDesc->BindingType);

					if (descriptorType == EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN)
					{
						LOG_ERROR("[RenderGraph]: Descriptor Type for Resource State with name \"%s\" could not be found", pResourceStateDesc->ResourceName.c_str());
						return false;
					}

					DescriptorBindingDesc descriptorBinding = {};
					descriptorBinding.DescriptorType		= descriptorType;
					descriptorBinding.ShaderStageMask		= CreateShaderStageMask(pRenderStageDesc);

					if (pResource->Type == ERenderGraphResourceType::TEXTURE)
					{
						ETextureState textureState = CalculateResourceTextureState(pResource->Type, pResourceStateDesc->BindingType, pResource->Texture.Format);

						uint32 actualSubResourceCount		= (pResource->BackBufferBound || pResource->Texture.IsOfArrayType) ? 1 : pResource->SubResourceCount;

						descriptorBinding.DescriptorCount	= (uint32)glm::ceil((float)actualSubResourceCount / pRenderStage->TextureSubDescriptorSetCount);
						descriptorBinding.Binding			= textureDescriptorBindingIndex++;

						textureDescriptorSetDescriptions.PushBack(descriptorBinding);
						renderStageTextureResources.PushBack(std::make_tuple(pResource, textureState, descriptorType));
					}
					else
					{
						descriptorBinding.DescriptorCount	= pResource->SubResourceCount;
						descriptorBinding.Binding			= bufferDescriptorBindingIndex++;

						bufferDescriptorSetDescriptions.PushBack(descriptorBinding);
						renderStageBufferResources.PushBack(std::make_tuple(pResource, ETextureState::TEXTURE_STATE_UNKNOWN, descriptorType));
					}
				}
				//RenderPass Attachments
				else if (pResourceStateDesc->BindingType == ERenderGraphResourceBindingType::ATTACHMENT)
				{
					if (pResource->SubResourceCount > 1 && pResource->SubResourceCount != m_BackBufferCount)
					{
						LOG_ERROR("[RenderGraph]: Resource \"%s\" is bound as RenderPass Attachment but does not have a subresource count equal to 1 or Back buffer Count: %u", pResource->Name.c_str(), pResource->SubResourceCount);
						return false;
					}

					bool isColorAttachment = pResource->Texture.Format != EFormat::FORMAT_D24_UNORM_S8_UINT;

					ETextureState initialState	= CalculateResourceTextureState(pResource->Type, pResourceStateDesc->AttachmentSynchronizations.PrevBindingType, pResource->Texture.Format);
					ETextureState finalState	= CalculateResourceTextureState(pResource->Type, pResourceStateDesc->AttachmentSynchronizations.NextBindingType, pResource->Texture.Format);

					ELoadOp loadOp = ELoadOp::LOAD_OP_LOAD;

					if (initialState == ETextureState::TEXTURE_STATE_DONT_CARE ||
						initialState == ETextureState::TEXTURE_STATE_UNKNOWN ||
						!pResourceStateDesc->AttachmentSynchronizations.PrevSameFrame)
					{
						loadOp = ELoadOp::LOAD_OP_CLEAR;
					}

					if (isColorAttachment)
					{
						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= pResource->Texture.Format;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= loadOp;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE_OP_STORE;
						renderPassAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
						renderPassAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
						renderPassAttachmentDesc.InitialState	= initialState;
						renderPassAttachmentDesc.FinalState		= finalState;

						renderPassAttachmentDescriptions.PushBack(renderPassAttachmentDesc);

						renderPassRenderTargetStates.PushBack(ETextureState::TEXTURE_STATE_RENDER_TARGET);

						BlendAttachmentStateDesc blendAttachmentState = {};
						blendAttachmentState.BlendEnabled			= false;
						blendAttachmentState.RenderTargetComponentMask	= COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A;

						renderPassBlendAttachmentStates.PushBack(blendAttachmentState);
						renderStageRenderTargets.PushBack(std::make_pair(pResource, finalState));
					}
					else
					{
						RenderPassAttachmentDesc renderPassAttachmentDesc = {};
						renderPassAttachmentDesc.Format			= pResource->Texture.Format;
						renderPassAttachmentDesc.SampleCount	= 1;
						renderPassAttachmentDesc.LoadOp			= loadOp;
						renderPassAttachmentDesc.StoreOp		= EStoreOp::STORE_OP_STORE;
						renderPassAttachmentDesc.StencilLoadOp	= loadOp;
						renderPassAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_STORE;
						renderPassAttachmentDesc.InitialState	= initialState;
						renderPassAttachmentDesc.FinalState		= finalState;

						renderPassDepthStencilDescription = renderPassAttachmentDesc;
						pDepthStencilResource = pResource;
					}
				}
				else if (pResourceStateDesc->BindingType == ERenderGraphResourceBindingType::DRAW_RESOURCE)
				{
					ASSERT(false); //Todo: What todo here? Is this just error?
				}
			}

			if (pRenderStageDesc->CustomRenderer)
			{
				ICustomRenderer* pCustomRenderer = nullptr;

				if (pRenderStageDesc->Name == RENDER_GRAPH_IMGUI_STAGE_NAME)
				{
					ImGuiRenderer* pImGuiRenderer = DBG_NEW ImGuiRenderer(m_pGraphicsDevice);

					ImGuiRendererDesc imguiRendererDesc = {};
					imguiRendererDesc.BackBufferCount	= m_BackBufferCount;
					imguiRendererDesc.VertexBufferSize	= MEGA_BYTE(8);
					imguiRendererDesc.IndexBufferSize	= MEGA_BYTE(8);

					if (!pImGuiRenderer->Init(&imguiRendererDesc))
					{
						LOG_ERROR("[RenderGraph] Could not initialize ImGui Custom Renderer");
						return false;
					}

					pCustomRenderer					= pImGuiRenderer;

					m_CustomRenderers.PushBack(pImGuiRenderer);
					m_DebugRenderers.PushBack(pImGuiRenderer);
				}
				else
				{
					//Todo: Implement Custom Custom Renderer
					/*pCustomRenderer = pRenderStageDesc->CustomRenderer.pCustomRenderer;
					m_CustomRenderers.PushBack(pRenderStageDesc->CustomRenderer.pCustomRenderer);*/
				}

				CustomRendererRenderGraphInitDesc customRendererInitDesc = {};
				customRendererInitDesc.pColorAttachmentDesc			= renderPassAttachmentDescriptions.GetData();
				customRendererInitDesc.ColorAttachmentCount			= (uint32)renderPassAttachmentDescriptions.GetSize();
				customRendererInitDesc.pDepthStencilAttachmentDesc	= renderPassDepthStencilDescription.Format != EFormat::FORMAT_NONE ? &renderPassDepthStencilDescription : nullptr;

				if (!pCustomRenderer->RenderGraphInit(&customRendererInitDesc))
				{
					LOG_ERROR("[RenderGraph] Could not initialize Custom Renderer");
					return false;
				}

				pRenderStage->UsesCustomRenderer	= true;
				pRenderStage->pCustomRenderer		= pCustomRenderer;
				pRenderStage->FirstPipelineStage	= pCustomRenderer->GetFirstPipelineStage();
				pRenderStage->LastPipelineStage		= pCustomRenderer->GetLastPipelineStage();
			}
			else
			{
				pRenderStage->FirstPipelineStage	= FindEarliestPipelineStage(pRenderStageDesc);
				pRenderStage->LastPipelineStage		= FindLastPipelineStage(pRenderStageDesc);

				//Todo: Implement Constant Range
				//ConstantRangeDesc constantRangeDesc = {};
				//constantRangeDesc.OffsetInBytes			= 0;
				//constantRangeDesc.ShaderStageFlags		= CreateShaderStageMask(pRenderStageDesc);
				//constantRangeDesc.SizeInBytes			= pRenderStageDesc->PushConstants.DataSize;

				//Create Pipeline Layout
				{
					TArray<DescriptorSetLayoutDesc> descriptorSetLayouts;
					descriptorSetLayouts.Reserve(2);

					if (bufferDescriptorSetDescriptions.GetSize() > 0)
					{
						DescriptorSetLayoutDesc descriptorSetLayout = {};
						descriptorSetLayout.DescriptorBindings		= bufferDescriptorSetDescriptions;
						descriptorSetLayouts.PushBack(descriptorSetLayout);
					}

					if (textureDescriptorSetDescriptions.GetSize() > 0)
					{
						DescriptorSetLayoutDesc descriptorSetLayout = {};
						descriptorSetLayout.DescriptorBindings		= textureDescriptorSetDescriptions;
						descriptorSetLayouts.PushBack(descriptorSetLayout);
					}

					PipelineLayoutDesc pipelineLayoutDesc = {};
					pipelineLayoutDesc.DescriptorSetLayouts	= descriptorSetLayouts;
					//pipelineLayoutDesc.pConstantRanges			= &constantRangeDesc;
					//pipelineLayoutDesc.ConstantRangeCount		= constantRangeDesc.SizeInBytes > 0 ? 1 : 0;

					pRenderStage->pPipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);
				}

				//Create Descriptor Set
				{
					if (bufferDescriptorSetDescriptions.GetSize() > 0)
					{
						pRenderStage->ppBufferDescriptorSets = DBG_NEW DescriptorSet*[m_BackBufferCount];

						for (uint32 i = 0; i < m_BackBufferCount; i++)
						{
							DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet(pRenderStageDesc->Name + " Buffer Descriptor Set " + std::to_string(i), pRenderStage->pPipelineLayout, 0, m_pDescriptorHeap);
							pRenderStage->ppBufferDescriptorSets[i] = pDescriptorSet;
						}
					}

					if (textureDescriptorSetDescriptions.GetSize() > 0)
					{
						uint32 textureDescriptorSetCount = m_BackBufferCount * pRenderStage->TextureSubDescriptorSetCount;
						pRenderStage->ppTextureDescriptorSets = DBG_NEW DescriptorSet*[textureDescriptorSetCount];

						for (uint32 i = 0; i < textureDescriptorSetCount; i++)
						{
							DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet(pRenderStageDesc->Name + " Texture Descriptor Set " + std::to_string(i), pRenderStage->pPipelineLayout, pRenderStage->ppBufferDescriptorSets != nullptr ? 1 : 0, m_pDescriptorHeap);
							pRenderStage->ppTextureDescriptorSets[i] = pDescriptorSet;
						}
					}
				}

				//Create Pipeline State
				if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
				{
					ManagedGraphicsPipelineStateDesc pipelineDesc = {};
					pipelineDesc.DebugName							= pRenderStageDesc->Name;
					pipelineDesc.PipelineLayout						= pRenderStage->pPipelineLayout;
					pipelineDesc.DepthStencilState.DepthTestEnable	= pRenderStageDesc->Graphics.DepthTestEnabled;
					pipelineDesc.TaskShader.ShaderGUID				= pRenderStageDesc->Graphics.Shaders.TaskShaderName.empty()		? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.TaskShaderName,		FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.MeshShader.ShaderGUID				= pRenderStageDesc->Graphics.Shaders.MeshShaderName.empty()		? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.MeshShaderName,		FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.VertexShader.ShaderGUID			= pRenderStageDesc->Graphics.Shaders.VertexShaderName.empty()	? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.VertexShaderName,		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.GeometryShader.ShaderGUID			= pRenderStageDesc->Graphics.Shaders.GeometryShaderName.empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.GeometryShaderName,	FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER,	EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.HullShader.ShaderGUID				= pRenderStageDesc->Graphics.Shaders.HullShaderName.empty()		? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.HullShaderName,		FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.DomainShader.ShaderGUID			= pRenderStageDesc->Graphics.Shaders.DomainShaderName.empty()	? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.DomainShaderName,		FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.PixelShader.ShaderGUID				= pRenderStageDesc->Graphics.Shaders.PixelShaderName.empty()	? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Graphics.Shaders.PixelShaderName,		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,		EShaderLang::SHADER_LANG_GLSL);
					pipelineDesc.BlendState.BlendAttachmentStates	= renderPassBlendAttachmentStates;

					//Create RenderPass
					{
						RenderPassSubpassDesc renderPassSubpassDesc = { };
						renderPassSubpassDesc.RenderTargetStates			= renderPassRenderTargetStates;
						renderPassSubpassDesc.DepthStencilAttachmentState	= pDepthStencilResource != nullptr ? ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT : ETextureState::TEXTURE_STATE_DONT_CARE;

						RenderPassSubpassDependencyDesc renderPassSubpassDependencyDesc = {};
						renderPassSubpassDependencyDesc.SrcSubpass		= EXTERNAL_SUBPASS;
						renderPassSubpassDependencyDesc.DstSubpass		= 0;
						renderPassSubpassDependencyDesc.SrcAccessMask	= 0;
						renderPassSubpassDependencyDesc.DstAccessMask	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
						renderPassSubpassDependencyDesc.SrcStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
						renderPassSubpassDependencyDesc.DstStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

						if (renderPassDepthStencilDescription.Format != EFormat::FORMAT_NONE) 
							renderPassAttachmentDescriptions.PushBack(renderPassDepthStencilDescription);

						RenderPassDesc renderPassDesc = {};
						renderPassDesc.DebugName			= "";
						renderPassDesc.Attachments			= renderPassAttachmentDescriptions;
						renderPassDesc.Subpasses			= { renderPassSubpassDesc };
						renderPassDesc.SubpassDependencies	= { renderPassSubpassDependencyDesc };

						RenderPass* pRenderPass		= m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);
						pipelineDesc.RenderPass		= pRenderPass;

						pRenderStage->pRenderPass		= pRenderPass;
					}

					//Set Draw Type and Draw Resource
					{
						pRenderStage->DrawType = pRenderStageDesc->Graphics.DrawType;

						if (pRenderStageDesc->Graphics.IndexBufferName.size() > 0)
						{
							auto indexBufferIt = m_ResourceMap.find(pRenderStageDesc->Graphics.IndexBufferName);

							if (indexBufferIt == m_ResourceMap.end())
							{
								LOG_ERROR("[RenderGraph]: Resource \"%s\" is referenced as index buffer resource by render stage, but it cannot be found in Resource Map", pRenderStageDesc->Graphics.IndexBufferName.c_str());
								return false;
							}

							pRenderStage->pIndexBufferResource = &indexBufferIt->second;
						}

						if (pRenderStageDesc->Graphics.IndirectArgsBufferName.size() > 0)
						{
							auto indirectArgsBufferIt = m_ResourceMap.find(pRenderStageDesc->Graphics.IndirectArgsBufferName);

							if (indirectArgsBufferIt == m_ResourceMap.end())
							{
								LOG_ERROR("[RenderGraph]: Resource \"%s\" is referenced as mesh index buffer resource by render stage, but it cannot be found in Resource Map", pRenderStageDesc->Graphics.IndirectArgsBufferName.c_str());
								return false;
							}

							pRenderStage->pIndirectArgsBufferResource = &indirectArgsBufferIt->second;
						}
					}

					pRenderStage->PipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineDesc);
				}
				else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
				{
					ManagedComputePipelineStateDesc pipelineDesc = { };
					pipelineDesc.DebugName		= pRenderStageDesc->Name;
					pipelineDesc.PipelineLayout	= pRenderStage->pPipelineLayout;
					pipelineDesc.Shader.ShaderGUID = pRenderStageDesc->Compute.ShaderName.empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->Compute.ShaderName, FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);

					pRenderStage->PipelineStateID = PipelineStateManager::CreateComputePipelineState(&pipelineDesc);
				}
				else if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
				{
					ManagedRayTracingPipelineStateDesc pipelineDesc = {};
					pipelineDesc.PipelineLayout			= pRenderStage->pPipelineLayout;
					pipelineDesc.MaxRecursionDepth		= 1;
					pipelineDesc.RaygenShader.ShaderGUID = pRenderStageDesc->RayTracing.Shaders.RaygenShaderName.empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->RayTracing.Shaders.RaygenShaderName, FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER, EShaderLang::SHADER_LANG_GLSL );

					pipelineDesc.ClosestHitShaders.Resize(pRenderStageDesc->RayTracing.Shaders.ClosestHitShaderCount);
					for (uint32 ch = 0; ch < pRenderStageDesc->RayTracing.Shaders.ClosestHitShaderCount; ch++)
					{
						pipelineDesc.ClosestHitShaders[ch].ShaderGUID = pRenderStageDesc->RayTracing.Shaders.pClosestHitShaderNames[ch].empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->RayTracing.Shaders.pClosestHitShaderNames[ch], FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER, EShaderLang::SHADER_LANG_GLSL );
					}

					pipelineDesc.MissShaders.Resize(pRenderStageDesc->RayTracing.Shaders.MissShaderCount);
					for (uint32 m = 0; m < pRenderStageDesc->RayTracing.Shaders.MissShaderCount; m++)
					{
						pipelineDesc.MissShaders[m].ShaderGUID = pRenderStageDesc->RayTracing.Shaders.pMissShaderNames[m].empty() ? GUID_NONE : ResourceManager::LoadShaderFromFile(pRenderStageDesc->RayTracing.Shaders.pMissShaderNames[m], FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER, EShaderLang::SHADER_LANG_GLSL );
					}

					pRenderStage->PipelineStateID = PipelineStateManager::CreateRayTracingPipelineState(&pipelineDesc);
				}
			}

			//Create Resource Bindings to Render Stage
			{
				if (renderStageRenderTargets.GetSize() > 0)
				{
					if (pRenderStageDesc->Type != EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
					{
						LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", renderStageIndex);
						return false;
					}

					for (uint32 r = 0; r < renderStageRenderTargets.GetSize(); r++)
					{
						auto resourcePair = renderStageRenderTargets[r];
						Resource* pResource = resourcePair.first;

						ResourceBinding resourceBinding = {};
						resourceBinding.pRenderStage	= pRenderStage;
						resourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
						resourceBinding.Binding			= UINT32_MAX;
						resourceBinding.TextureState	= resourcePair.second;

						pResource->ResourceBindings.PushBack(resourceBinding);		//Create Binding to notify Custom Renderers
						pRenderStage->RenderTargetResources.PushBack(pResource);
					}
				}

				if (pDepthStencilResource != nullptr)
				{
					if (pRenderStageDesc->Type != EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
					{
						LOG_ERROR("[RenderGraph]: There are resources that a RenderPass should be linked to, but Render Stage %u is not a Graphics Pipeline State", renderStageIndex);
						return false;
					}

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
					resourceBinding.Binding			= UINT32_MAX;
					resourceBinding.TextureState	= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

					pDepthStencilResource->ResourceBindings.PushBack(resourceBinding); //Create Binding to notify Custom Renderers
					pRenderStage->pDepthStencilAttachment = pDepthStencilResource;
				}

				for (uint32 r = 0; r < renderStageTextureResources.GetSize(); r++)
				{
					auto& resourceTuple = renderStageTextureResources[r];
					Resource* pResource = std::get<0>(resourceTuple);

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= std::get<2>(resourceTuple);
					resourceBinding.Binding			= r;
					resourceBinding.TextureState	= std::get<1>(resourceTuple);

					pResource->ResourceBindings.PushBack(resourceBinding);
				}

				for (uint32 r = 0; r < renderStageBufferResources.GetSize(); r++)
				{
					auto& resourceTuple = renderStageBufferResources[r];
					Resource* pResource = std::get<0>(resourceTuple);

					ResourceBinding resourceBinding = {};
					resourceBinding.pRenderStage	= pRenderStage;
					resourceBinding.DescriptorType	= std::get<2>(resourceTuple);
					resourceBinding.Binding			= r;
					resourceBinding.TextureState	= std::get<1>(resourceTuple);

					pResource->ResourceBindings.PushBack(resourceBinding);
				}
			}

			//Link Render Stage to Push Constant Resource
			/*if (pRenderStageDesc->PushConstants.DataSize > 0)
			{
				auto it = m_ResourceMap.find(pRenderStageDesc->PushConstants.DebugName);

				if (it != m_ResourceMap.end())
				{
					pRenderStage->pPushConstantsResource = &it->second;
				}
				else
				{
					LOG_ERROR("[RenderGraph]: Push Constants resource found in Render Stage but not in Resource Map \"%s\"", pRenderStageDesc->PushConstants.DebugName);
					return false;
				}
			}*/
		}

		return true;
	}

	bool RenderGraph::CreateSynchronizationStages(const TArray<SynchronizationStageDesc>& synchronizationStageDescriptions)
	{
		m_pSynchronizationStages = DBG_NEW SynchronizationStage[synchronizationStageDescriptions.GetSize()];

		bool firstTimeEnvounteringBackBuffer = false;

		for (uint32 s = 0; s < synchronizationStageDescriptions.GetSize(); s++)
		{
			const SynchronizationStageDesc* pSynchronizationStageDesc = &synchronizationStageDescriptions[s];

			SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[s];
			ECommandQueueType otherQueue = ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;

			for (auto synchronizationIt = pSynchronizationStageDesc->Synchronizations.begin(); synchronizationIt != pSynchronizationStageDesc->Synchronizations.end(); synchronizationIt++)
			{
				const RenderGraphResourceSynchronizationDesc* pResourceSynchronizationDesc = &(*synchronizationIt);

				//En massa skit kommer nog beh�va g�ras om h�r, nu n�r Parsern tar hand om Back Buffer States korrekt.

				auto it = m_ResourceMap.find(pResourceSynchronizationDesc->ResourceName);

				if (it == m_ResourceMap.end())
				{
					LOG_ERROR("[RenderGraph]: Resource found in Synchronization Stage but not in Resource Map \"%s\"", pResourceSynchronizationDesc->ResourceName.c_str());
					return false;
				}

				Resource* pResource = &it->second;

				auto prevRenderStageIt = m_RenderStageMap.find(pResourceSynchronizationDesc->PrevRenderStage);
				auto nextRenderStageIt = m_RenderStageMap.find(pResourceSynchronizationDesc->NextRenderStage);

				if (prevRenderStageIt == m_RenderStageMap.end())
				{
					LOG_ERROR("[RenderGraph]: Render Stage found in Synchronization but not in Render Stage Map \"%s\"", pResourceSynchronizationDesc->PrevRenderStage.c_str());
					return false;
				}

				if (nextRenderStageIt == m_RenderStageMap.end())
				{
					LOG_ERROR("[RenderGraph]: Render Stage found in Synchronization but not in Render Stage Map \"%s\"", pResourceSynchronizationDesc->NextRenderStage.c_str());
					return false;
				}

				const RenderStage* pPrevRenderStage = &m_pRenderStages[prevRenderStageIt->second];
				const RenderStage* pNextRenderStage = &m_pRenderStages[nextRenderStageIt->second];

				ECommandQueueType prevQueue 	= pResourceSynchronizationDesc->PrevQueue;
				ECommandQueueType nextQueue		= pResourceSynchronizationDesc->NextQueue;
				uint32 srcMemoryAccessFlags		= CalculateResourceAccessFlags(pResourceSynchronizationDesc->PrevBindingType);
				uint32 dstMemoryAccessFlags		= CalculateResourceAccessFlags(pResourceSynchronizationDesc->NextBindingType);

				if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_NONE)
				{
					pSynchronizationStage->ExecutionQueue = prevQueue;
					otherQueue = pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS ? ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE : ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
				}
				else if (pSynchronizationStage->ExecutionQueue != prevQueue)
				{
					LOG_ERROR("[RenderGraph]: SynchronizationStage \"%s\" contains synchronizations that have different Previous Queues");
					return false;
				}

				pSynchronizationStage->SrcPipelineStage				= FindLastPipelineStage(pSynchronizationStage->SrcPipelineStage | pPrevRenderStage->LastPipelineStage);
				pSynchronizationStage->SameQueueDstPipelineStage	= FindEarliestCompatiblePipelineStage(pSynchronizationStage->SameQueueDstPipelineStage | pNextRenderStage->FirstPipelineStage, pSynchronizationStage->ExecutionQueue);
				pSynchronizationStage->OtherQueueDstPipelineStage	= FindEarliestCompatiblePipelineStage(pSynchronizationStage->OtherQueueDstPipelineStage | pNextRenderStage->FirstPipelineStage, otherQueue);

				if (pResource->Type == ERenderGraphResourceType::TEXTURE)
				{
					PipelineTextureBarrierDesc textureBarrier = {};
					textureBarrier.QueueBefore			= prevQueue;
					textureBarrier.QueueAfter			= nextQueue;
					textureBarrier.SrcMemoryAccessFlags = srcMemoryAccessFlags;
					textureBarrier.DstMemoryAccessFlags = dstMemoryAccessFlags;
					textureBarrier.StateBefore			= CalculateResourceTextureState(pResource->Type, pResourceSynchronizationDesc->PrevBindingType, pResource->Texture.Format);
					textureBarrier.StateAfter			= CalculateResourceTextureState(pResource->Type, pResourceSynchronizationDesc->NextBindingType, pResource->Texture.Format);
					textureBarrier.TextureFlags			= pResource->Texture.Format == EFormat::FORMAT_D24_UNORM_S8_UINT ? FTextureFlags::TEXTURE_FLAG_DEPTH_STENCIL : 0;

					uint32 targetSynchronizationIndex = 0;

					if (prevQueue == nextQueue)
					{
						if (pResource->BackBufferBound)
						{
							targetSynchronizationIndex = SAME_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX;
						}
						else
						{
							targetSynchronizationIndex = SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX;
						}
					}
					else
					{
						if (pResource->BackBufferBound)
						{
							targetSynchronizationIndex = OTHER_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX;
						}
						else
						{
							targetSynchronizationIndex = OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX;
						}
					}

					uint32 actualSubResourceCount = pResource->Texture.IsOfArrayType ? 1 : pResource->SubResourceCount;
					for (uint32 sr = 0; sr < actualSubResourceCount; sr++)
					{
						TArray<PipelineTextureBarrierDesc>& targetArray = pSynchronizationStage->TextureBarriers[targetSynchronizationIndex];
						targetArray.PushBack(textureBarrier);
						uint32 barrierIndex = targetArray.GetSize() - 1;

						ResourceBarrierInfo barrierInfo = {};
						barrierInfo.SynchronizationStageIndex	= s;
						barrierInfo.SynchronizationTypeIndex	= targetSynchronizationIndex;
						barrierInfo.BarrierIndex				= barrierIndex;

						pResource->Texture.BarriersPerSynchronizationStage.PushBack(barrierInfo);
					}
				}
				else if (pResource->Type == ERenderGraphResourceType::BUFFER)
				{
					PipelineBufferBarrierDesc bufferBarrier = {};
					bufferBarrier.QueueBefore			= prevQueue;
					bufferBarrier.QueueAfter			= nextQueue;
					bufferBarrier.SrcMemoryAccessFlags	= srcMemoryAccessFlags;
					bufferBarrier.DstMemoryAccessFlags	= dstMemoryAccessFlags;

					uint32 targetSynchronizationIndex = 0;

					if (prevQueue == nextQueue)
					{
						targetSynchronizationIndex = SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX;
					}
					else
					{
						targetSynchronizationIndex = OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX;
					}

					for (uint32 sr = 0; sr < pResource->SubResourceCount; sr++)
					{
						TArray<PipelineBufferBarrierDesc>& targetArray = pSynchronizationStage->BufferBarriers[targetSynchronizationIndex];
						targetArray.PushBack(bufferBarrier);
						uint32 barrierIndex = targetArray.GetSize() - 1;

						ResourceBarrierInfo barrierInfo = {};
						barrierInfo.SynchronizationStageIndex	= s;
						barrierInfo.SynchronizationTypeIndex	= targetSynchronizationIndex;
						barrierInfo.BarrierIndex				= barrierIndex;

						pResource->Buffer.BarriersPerSynchronizationStage.PushBack(barrierInfo);
					}
				}
			}
		}

		return true;
	}

	bool RenderGraph::CreatePipelineStages(const TArray<PipelineStageDesc>& pipelineStageDescriptions)
	{
		m_PipelineStageCount = (uint32)pipelineStageDescriptions.GetSize();
		m_pPipelineStages = DBG_NEW PipelineStage[m_PipelineStageCount];

		for (uint32 i = 0; i < m_PipelineStageCount; i++)
		{
			const PipelineStageDesc* pPipelineStageDesc = &pipelineStageDescriptions[i];

			PipelineStage* pPipelineStage = &m_pPipelineStages[i];

			m_ExecutionStageCount += pPipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION ? 2 : 1;

			pPipelineStage->Type		= pPipelineStageDesc->Type;
			pPipelineStage->StageIndex	= pPipelineStageDesc->StageIndex;

			pPipelineStage->ppGraphicsCommandAllocators		= DBG_NEW CommandAllocator*[m_BackBufferCount];
			pPipelineStage->ppComputeCommandAllocators		= DBG_NEW CommandAllocator*[m_BackBufferCount];
			pPipelineStage->ppGraphicsCommandLists			= DBG_NEW CommandList*[m_BackBufferCount];
			pPipelineStage->ppComputeCommandLists			= DBG_NEW CommandList*[m_BackBufferCount];
			
			for (uint32 f = 0; f < m_BackBufferCount; f++)
			{
				pPipelineStage->ppGraphicsCommandAllocators[f]	= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Graphics Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
				pPipelineStage->ppComputeCommandAllocators[f]	= m_pGraphicsDevice->CreateCommandAllocator("Render Graph Compute Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

				CommandListDesc graphicsCommandListDesc = {};
				graphicsCommandListDesc.DebugName					= "Render Graph Graphics Command List";
				graphicsCommandListDesc.CommandListType			= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				graphicsCommandListDesc.Flags					= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				pPipelineStage->ppGraphicsCommandLists[f]		= m_pGraphicsDevice->CreateCommandList(pPipelineStage->ppGraphicsCommandAllocators[f], &graphicsCommandListDesc);

				CommandListDesc computeCommandListDesc = {};
				computeCommandListDesc.DebugName					= "Render Graph Compute Command List";
				computeCommandListDesc.CommandListType			= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				computeCommandListDesc.Flags					= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				pPipelineStage->ppComputeCommandLists[f]		= m_pGraphicsDevice->CreateCommandList(pPipelineStage->ppComputeCommandAllocators[f], &computeCommandListDesc);
			}
		}

		m_ppExecutionStages = DBG_NEW CommandList*[m_ExecutionStageCount];

		return true;
	}

	void RenderGraph::UpdateInternalResource(InternalResourceUpdateDesc& desc)
	{
		auto it = m_ResourceMap.find(desc.ResourceName);

		if (it != m_ResourceMap.end())
		{
			Resource* pResource = &it->second;
			ResourceUpdateDesc resourceUpdateDesc;
			resourceUpdateDesc.ResourceName = desc.ResourceName;

			switch (desc.Type)
			{
				case ERenderGraphResourceType::TEXTURE:					
				{
					resourceUpdateDesc.InternalTextureUpdate.pTextureDesc		= &desc.TextureUpdate.TextureDesc;
					resourceUpdateDesc.InternalTextureUpdate.pTextureViewDesc	= &desc.TextureUpdate.TextureViewDesc;
					resourceUpdateDesc.InternalTextureUpdate.pSamplerDesc		= &desc.TextureUpdate.SamplerDesc;
					UpdateResourceTexture(pResource, resourceUpdateDesc);
					break;
				}
				case ERenderGraphResourceType::BUFFER:					
				{
					resourceUpdateDesc.InternalBufferUpdate.pBufferDesc			= &desc.BufferUpdate.BufferDesc;
					UpdateResourceBuffer(pResource, resourceUpdateDesc);
					break;
				}
				default:
				{
					LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph has unsupported Type", desc.ResourceName.c_str());
					return;
				}
			}
		}
		else
		{
			LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph could not be found in Resource Map", desc.ResourceName.c_str());
			return;
		}
	}

	void RenderGraph::UpdateResourceTexture(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		uint32 actualSubResourceCount = 0;

		if (pResource->BackBufferBound)
		{
			actualSubResourceCount = m_BackBufferCount;
		}
		else if (pResource->Texture.IsOfArrayType)
		{
			actualSubResourceCount = 1;
		}
		else
		{
			actualSubResourceCount = pResource->SubResourceCount;
		}


		for (uint32 sr = 0; sr < actualSubResourceCount; sr++)
		{
			Texture** ppTexture = &pResource->Texture.Textures[sr];
			TextureView** ppTextureView = &pResource->Texture.TextureViews[sr];
			Sampler** ppSampler = &pResource->Texture.Samplers[sr];

			Texture* pTexture			= nullptr;
			TextureView* pTextureView	= nullptr;
			Sampler* pSampler			= nullptr;

			if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
			{
				const TextureDesc* pTextureDesc	= desc.InternalTextureUpdate.pTextureDesc;
				TextureViewDesc textureViewDesc = *desc.InternalTextureUpdate.pTextureViewDesc; //Make a copy so we can change TextureViewDesc::pTexture
				
				SAFERELEASE(*ppTexture);
				SAFERELEASE(*ppTextureView);

				pTexture			= m_pGraphicsDevice->CreateTexture(pTextureDesc, nullptr);

				textureViewDesc.Texture = pTexture;
				pTextureView	= m_pGraphicsDevice->CreateTextureView(&textureViewDesc);

				//Update Sampler
				if (desc.InternalTextureUpdate.pSamplerDesc != nullptr)
				{
					SAFERELEASE(*ppSampler);
					pSampler = m_pGraphicsDevice->CreateSampler(desc.InternalTextureUpdate.pSamplerDesc);
				}
			}
			else if (pResource->OwnershipType == EResourceOwnershipType::EXTERNAL)
			{
				pTexture			= desc.ExternalTextureUpdate.ppTextures[sr];
				pTextureView		= desc.ExternalTextureUpdate.ppTextureViews[sr];

				//Update Sampler
				if (desc.ExternalTextureUpdate.ppSamplers != nullptr)
				{
					pSampler = desc.ExternalTextureUpdate.ppSamplers[sr];
				}
			}
			else
			{
				LOG_ERROR("[RenderGraph]: UpdateResourceTexture called for resource with invalid OwnershipType");
				return;
			}

			TextureDesc textureDesc = pTexture->GetDesc();

			if (pResource->Texture.IsOfArrayType)
			{
				if (textureDesc.ArrayCount != pResource->SubResourceCount)
				{
					LOG_ERROR("[RenderGraph]: UpdateResourceTexture for resource of array type with length %u but ArrayCount was %u", pResource->SubResourceCount, textureDesc.ArrayCount);
					return;
				}
			}

			(*ppTexture)		= pTexture;
			(*ppTextureView)	= pTextureView;
			(*ppSampler)		= pSampler;

			if (pResource->Texture.BarriersPerSynchronizationStage.GetSize() > 0)
			{
				PipelineTextureBarrierDesc* pFirstBarrier = nullptr;

				for (uint32 b = sr; b < pResource->Texture.BarriersPerSynchronizationStage.GetSize(); b += pResource->SubResourceCount)
				{
					const ResourceBarrierInfo* pBarrierInfo = &pResource->Texture.BarriersPerSynchronizationStage[b];
					SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

					PipelineTextureBarrierDesc* pTextureBarrier = &pSynchronizationStage->TextureBarriers[pBarrierInfo->SynchronizationTypeIndex][pBarrierInfo->BarrierIndex];

					pTextureBarrier->pTexture		= pTexture;
					pTextureBarrier->Miplevel		= 0;
					pTextureBarrier->MiplevelCount	= textureDesc.Miplevels;
					pTextureBarrier->ArrayIndex		= 0;
					pTextureBarrier->ArrayCount		= textureDesc.ArrayCount;

					if (pFirstBarrier == nullptr)
						pFirstBarrier = pTextureBarrier;
				}

				//Transfer to Initial State
				if (pFirstBarrier != nullptr)
				{
					PipelineTextureBarrierDesc  initialBarrier = {};

					initialBarrier.pTexture						= pTexture;
					initialBarrier.StateBefore					= ETextureState::TEXTURE_STATE_DONT_CARE;
					initialBarrier.StateAfter					= pFirstBarrier->StateBefore;
					initialBarrier.QueueBefore					= pFirstBarrier->QueueBefore;
					initialBarrier.QueueAfter					= pFirstBarrier->QueueBefore;
					initialBarrier.SrcMemoryAccessFlags			= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
					initialBarrier.DstMemoryAccessFlags			= pFirstBarrier->SrcMemoryAccessFlags;
					initialBarrier.TextureFlags					= pFirstBarrier->TextureFlags;
					initialBarrier.Miplevel						= 0;
					initialBarrier.MiplevelCount				= pFirstBarrier->MiplevelCount;
					initialBarrier.ArrayIndex					= 0;
					initialBarrier.ArrayCount					= pFirstBarrier->ArrayCount;

					FPipelineStageFlags srcPipelineStage = pResource->ResourceBindings[0].pRenderStage->FirstPipelineStage;
					FPipelineStageFlags dstPipelineStage = pResource->ResourceBindings[0].pRenderStage->LastPipelineStage;

					if (initialBarrier.QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
					{
						CommandList* pCommandList = m_ppGraphicsCopyCommandLists[m_ModFrameIndex];

						if (!m_ExecuteGraphicsCopy)
						{
							m_ExecuteGraphicsCopy = true;
								
							m_ppGraphicsCopyCommandAllocators[m_ModFrameIndex]->Reset();
							pCommandList->Begin(nullptr);
						}

						pCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, &initialBarrier, 1);
					}
					else if (initialBarrier.QueueAfter == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
					{
						CommandList* pCommandList = m_ppComputeCopyCommandLists[m_ModFrameIndex];

						if (!m_ExecuteComputeCopy)
						{
							m_ExecuteComputeCopy = true;

							m_ppComputeCopyCommandAllocators[m_ModFrameIndex]->Reset();
							pCommandList->Begin(nullptr);
						}

						pCommandList->PipelineTextureBarriers(srcPipelineStage, dstPipelineStage, &initialBarrier, 1);
					}
				}
			}
		}

		if (pResource->ResourceBindings.GetSize() > 0)
			m_DirtyDescriptorSetTextures.insert(pResource);
	}

	void RenderGraph::UpdateResourceBuffer(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		uint32 actualSubResourceCount = pResource->BackBufferBound ? m_BackBufferCount : pResource->SubResourceCount;

		//Update Buffer
		for (uint32 sr = 0; sr < actualSubResourceCount; sr++)
		{
			Buffer** ppBuffer		= &pResource->Buffer.Buffers[sr];
			uint64* pOffset			= &pResource->Buffer.Offsets[sr];
			uint64* pSizeInBytes	= &pResource->Buffer.SizesInBytes[sr];

			Buffer* pBuffer = nullptr;

			if (pResource->OwnershipType == EResourceOwnershipType::INTERNAL)
			{
				SAFERELEASE(*ppBuffer);
				pBuffer = m_pGraphicsDevice->CreateBuffer(desc.InternalBufferUpdate.pBufferDesc, nullptr);
			}
			else if (pResource->OwnershipType == EResourceOwnershipType::EXTERNAL)
			{
				pBuffer = desc.ExternalBufferUpdate.ppBuffer[sr];
			}
			else
			{
				LOG_ERROR("[RenderGraph]: UpdateResourceBuffer called for Resource with unknown OwnershipType, \"%s\"", pResource->Name.c_str());
				return;
			}

			BufferDesc origBufferDesc = pBuffer->GetDesc();

			(*ppBuffer)		= pBuffer;
			(*pSizeInBytes) = origBufferDesc.SizeInBytes;
			(*pOffset)		= 0;

			for (uint32 b = sr; b < pResource->Buffer.BarriersPerSynchronizationStage.GetSize(); b += pResource->SubResourceCount)
			{
				const ResourceBarrierInfo* pBarrierInfo = &pResource->Buffer.BarriersPerSynchronizationStage[b];
				SynchronizationStage* pSynchronizationStage = &m_pSynchronizationStages[pBarrierInfo->SynchronizationStageIndex];

				PipelineBufferBarrierDesc* pBufferBarrier = &pSynchronizationStage->BufferBarriers[pBarrierInfo->SynchronizationTypeIndex][pBarrierInfo->BarrierIndex];

				pBufferBarrier->pBuffer		= pBuffer;
				pBufferBarrier->SizeInBytes = origBufferDesc.SizeInBytes;
				pBufferBarrier->Offset		= 0;
			}
		}

		if (pResource->ResourceBindings.GetSize() > 0)
			m_DirtyDescriptorSetBuffers.insert(pResource);
	}

	void RenderGraph::UpdateResourceAccelerationStructure(Resource* pResource, const ResourceUpdateDesc& desc)
	{
		//Update Acceleration Structure
		pResource->AccelerationStructure.pTLAS = desc.ExternalAccelerationStructure.pTLAS;

		m_DirtyDescriptorSetAccelerationStructures.insert(pResource);
	}

	void RenderGraph::UpdateRelativeRenderStageDimensions(RenderStage* pRenderStage)
	{
		if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::RELATIVE_1D)
		{
			pRenderStage->Dimensions.x = uint32(pRenderStage->Parameters.XDimVariable * m_WindowWidth * m_WindowHeight);
		}
		else
		{
			if (pRenderStage->Parameters.XDimType == ERenderGraphDimensionType::RELATIVE)
			{
				pRenderStage->Dimensions.x = uint32(pRenderStage->Parameters.XDimVariable * m_WindowWidth);
			}

			if (pRenderStage->Parameters.YDimType == ERenderGraphDimensionType::RELATIVE)
			{
				pRenderStage->Dimensions.y = uint32(pRenderStage->Parameters.YDimVariable * m_WindowHeight);
			}
		}
	}

	void RenderGraph::UpdateRelativeResourceDimensions(InternalResourceUpdateDesc* pResourceUpdateDesc)
	{
		switch (pResourceUpdateDesc->Type)
		{
			case ERenderGraphResourceType::TEXTURE:
			{
				if (pResourceUpdateDesc->TextureUpdate.XDimType == ERenderGraphDimensionType::RELATIVE)
				{
					pResourceUpdateDesc->TextureUpdate.TextureDesc.Width = uint32(pResourceUpdateDesc->TextureUpdate.XDimVariable * m_WindowWidth);
					m_DirtyInternalResources.insert(pResourceUpdateDesc->ResourceName);
				}

				if (pResourceUpdateDesc->TextureUpdate.YDimType == ERenderGraphDimensionType::RELATIVE)
				{
					pResourceUpdateDesc->TextureUpdate.TextureDesc.Height = uint32(pResourceUpdateDesc->TextureUpdate.YDimVariable * m_WindowHeight);
					m_DirtyInternalResources.insert(pResourceUpdateDesc->ResourceName);
				}

				break;
			}
			default:
			{
				LOG_WARNING("[RenderGraph]: Resource \"%s\" in Render Graph has unsupported Type for relative dimensions", pResourceUpdateDesc->ResourceName.c_str());
				return;
			}
		}
	}

	void RenderGraph::ExecuteSynchronizationStage(
		SynchronizationStage*	pSynchronizationStage, 
		CommandAllocator*		pGraphicsCommandAllocator, 
		CommandList*			pGraphicsCommandList, 
		CommandAllocator*		pComputeCommandAllocator, 
		CommandList*			pComputeCommandList,
		CommandList**			ppFirstExecutionStage,
		CommandList**			ppSecondExecutionStage)
	{
		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Begin(nullptr);

		pComputeCommandAllocator->Reset();
		pComputeCommandList->Begin(nullptr);

		CommandList* pFirstExecutionCommandList = nullptr;
		CommandList* pSecondExecutionCommandList = nullptr;

		if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
		{
			(*ppFirstExecutionStage) = pGraphicsCommandList;
			pFirstExecutionCommandList = pGraphicsCommandList;
			pSecondExecutionCommandList = pComputeCommandList;
		}
		else if (pSynchronizationStage->ExecutionQueue == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
		{
			(*ppFirstExecutionStage) = pComputeCommandList;
			pFirstExecutionCommandList = pComputeCommandList;
			pSecondExecutionCommandList = pGraphicsCommandList;
		}

		//Texture Synchronizations
		{
			const TArray<PipelineTextureBarrierDesc>& sameQueueBackBufferBarriers	= pSynchronizationStage->TextureBarriers[SAME_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& sameQueueTextureBarriers		= pSynchronizationStage->TextureBarriers[SAME_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& otherQueueBackBufferBarriers	= pSynchronizationStage->TextureBarriers[OTHER_QUEUE_BACK_BUFFER_BOUND_SYNCHRONIZATION_INDEX];
			const TArray<PipelineTextureBarrierDesc>& otherQueueTextureBarriers		= pSynchronizationStage->TextureBarriers[OTHER_QUEUE_TEXTURE_SYNCHRONIZATION_INDEX];

			if (sameQueueBackBufferBarriers.GetSize() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage, &otherQueueBackBufferBarriers[m_BackBufferIndex], 1);
			}

			if (sameQueueTextureBarriers.GetSize() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage, sameQueueTextureBarriers.GetData(), sameQueueTextureBarriers.GetSize());
			}

			if (otherQueueBackBufferBarriers.GetSize() > 0)
			{
				const PipelineTextureBarrierDesc* pTextureBarrier = &otherQueueBackBufferBarriers[m_BackBufferIndex];
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, pTextureBarrier, 1);
				pSecondExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, pTextureBarrier, 1);
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}

			if (otherQueueTextureBarriers.GetSize() > 0)
			{
				pFirstExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, otherQueueTextureBarriers.GetData(), otherQueueTextureBarriers.GetSize());
				pSecondExecutionCommandList->PipelineTextureBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, otherQueueTextureBarriers.GetData(), otherQueueTextureBarriers.GetSize());
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}
		}

		//Buffer Synchronization
		{
			const TArray<PipelineBufferBarrierDesc>& sameQueueBufferBarriers		= pSynchronizationStage->BufferBarriers[SAME_QUEUE_BUFFER_SYNCHRONIZATION_INDEX];
			const TArray<PipelineBufferBarrierDesc>& otherQueueBufferBarriers		= pSynchronizationStage->BufferBarriers[OTHER_QUEUE_BUFFER_SYNCHRONIZATION_INDEX];

			if (sameQueueBufferBarriers.GetSize() > 0)
			{
				pFirstExecutionCommandList->PipelineBufferBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->SameQueueDstPipelineStage, sameQueueBufferBarriers.GetData(), sameQueueBufferBarriers.GetSize());
			}

			if (otherQueueBufferBarriers.GetSize() > 0)
			{
				pFirstExecutionCommandList->PipelineBufferBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, otherQueueBufferBarriers.GetData(), otherQueueBufferBarriers.GetSize());
				pSecondExecutionCommandList->PipelineBufferBarriers(pSynchronizationStage->SrcPipelineStage, pSynchronizationStage->OtherQueueDstPipelineStage, otherQueueBufferBarriers.GetData(), otherQueueBufferBarriers.GetSize());
				(*ppSecondExecutionStage) = pSecondExecutionCommandList;
			}
		}

		pGraphicsCommandList->End();
		pComputeCommandList->End();
	}

	void RenderGraph::ExecuteGraphicsRenderStage(
		RenderStage*		pRenderStage, 
		PipelineState*		pPipelineState,
		CommandAllocator*	pGraphicsCommandAllocator, 
		CommandList*		pGraphicsCommandList, 
		CommandList**		ppExecutionStage)
	{
		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Begin(nullptr);

		uint32 flags = FRenderPassBeginFlags::RENDER_PASS_BEGIN_FLAG_INLINE;

		TextureView* ppTextureViews[MAX_COLOR_ATTACHMENTS];
		TextureView* pDepthStencilTextureView = nullptr;
		ClearColorDesc clearColorDescriptions[MAX_COLOR_ATTACHMENTS + 1];
		
		uint32 textureViewCount = 0;
		uint32 clearColorCount = 0;
		for (Resource* pResource : pRenderStage->RenderTargetResources)
		{
			//Assume resource is Back Buffer Bound if there is more than 1 Texture View
			ppTextureViews[textureViewCount++] = pResource->Texture.TextureViews.GetSize() > 1 ? pResource->Texture.TextureViews[m_BackBufferIndex] : pResource->Texture.TextureViews[0];

			clearColorDescriptions[clearColorCount].Color[0] = 0.0f;
			clearColorDescriptions[clearColorCount].Color[1] = 0.0f;
			clearColorDescriptions[clearColorCount].Color[2] = 0.0f;
			clearColorDescriptions[clearColorCount].Color[3] = 0.0f;

			clearColorCount++;
		}

		if (pRenderStage->pDepthStencilAttachment != nullptr)
		{
			pDepthStencilTextureView = pRenderStage->pDepthStencilAttachment->Texture.TextureViews.GetSize() > 1 ? pRenderStage->pDepthStencilAttachment->Texture.TextureViews[m_BackBufferIndex] : pRenderStage->pDepthStencilAttachment->Texture.TextureViews[0];

			clearColorDescriptions[clearColorCount].Depth		= 1.0f;
			clearColorDescriptions[clearColorCount].Stencil		= 0;

			clearColorCount++;
		}
		
		BeginRenderPassDesc beginRenderPassDesc = { };
		beginRenderPassDesc.pRenderPass			= pRenderStage->pRenderPass;
		beginRenderPassDesc.ppRenderTargets		= ppTextureViews;
		beginRenderPassDesc.RenderTargetCount	= textureViewCount;
		beginRenderPassDesc.pDepthStencil		= pDepthStencilTextureView;
		beginRenderPassDesc.Width				= pRenderStage->Dimensions.x;
		beginRenderPassDesc.Height				= pRenderStage->Dimensions.y;
		beginRenderPassDesc.Flags				= flags;
		beginRenderPassDesc.pClearColors		= clearColorDescriptions;
		beginRenderPassDesc.ClearColorCount		= clearColorCount;
		beginRenderPassDesc.Offset.x			= 0;
		beginRenderPassDesc.Offset.y			= 0;

		pGraphicsCommandList->BeginRenderPass(&beginRenderPassDesc);

		Viewport viewport = { };
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float)pRenderStage->Dimensions.x;
		viewport.Height		= (float)pRenderStage->Dimensions.y;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;

		pGraphicsCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = { };
		scissorRect.Width	= pRenderStage->Dimensions.x;
		scissorRect.Height	= pRenderStage->Dimensions.y;
		scissorRect.x		= 0;
		scissorRect.y		= 0;

		pGraphicsCommandList->SetScissorRects(&scissorRect, 0, 1);

		pGraphicsCommandList->BindGraphicsPipeline(pPipelineState);

		uint32 textureDescriptorSetBindingIndex = 0;
		if (pRenderStage->ppBufferDescriptorSets != nullptr)
		{
			pGraphicsCommandList->BindDescriptorSetGraphics(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, 0);
			textureDescriptorSetBindingIndex = 1;
		}

		if (pRenderStage->DrawType == ERenderStageDrawType::SCENE_INDIRECT)
		{
			pGraphicsCommandList->BindIndexBuffer(pRenderStage->pIndexBufferResource->Buffer.Buffers[0], 0, EIndexType::INDEX_TYPE_UINT32);

			Buffer* pDrawBuffer			= pRenderStage->pIndirectArgsBufferResource->Buffer.Buffers[0];
			uint32 totalDrawCount		= uint32(pDrawBuffer->GetDesc().SizeInBytes / sizeof(IndexedIndirectMeshArgument));
			uint32 indirectArgStride	= sizeof(IndexedIndirectMeshArgument);

			uint32 drawOffset = 0;
			for (uint32 i = 0; i < pRenderStage->TextureSubDescriptorSetCount; i++)
			{
				uint32 newBaseMaterialIndex	= (i + 1) * pRenderStage->MaterialsRenderedPerPass;
				uint32 newDrawOffset		= m_pScene->GetIndirectArgumentOffset(newBaseMaterialIndex);
				uint32 drawCount			= newDrawOffset - drawOffset;

				if (pRenderStage->ppTextureDescriptorSets != nullptr)
					pGraphicsCommandList->BindDescriptorSetGraphics(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex * pRenderStage->TextureSubDescriptorSetCount + i], pRenderStage->pPipelineLayout, textureDescriptorSetBindingIndex);

				pGraphicsCommandList->DrawIndexedIndirect(pDrawBuffer, drawOffset * indirectArgStride, drawCount, indirectArgStride);
				drawOffset = newDrawOffset;

				if (newDrawOffset >= totalDrawCount)
					break;
			}
		}
		else if (pRenderStage->DrawType == ERenderStageDrawType::FULLSCREEN_QUAD)
		{
			if (pRenderStage->TextureSubDescriptorSetCount > 1)
			{
				LOG_WARNING("[RenderGraph]: Render Stage has TextureSubDescriptor > 1 and DrawType FULLSCREEN_QUAD, this is currently not supported");
			}

			if (pRenderStage->ppTextureDescriptorSets != nullptr)
				pGraphicsCommandList->BindDescriptorSetGraphics(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, textureDescriptorSetBindingIndex);

			pGraphicsCommandList->DrawInstanced(3, 1, 0, 0);
		}

		pGraphicsCommandList->EndRenderPass();
		pGraphicsCommandList->End();

		(*ppExecutionStage) = pGraphicsCommandList;
	}

	void RenderGraph::ExecuteComputeRenderStage(
		RenderStage*		pRenderStage, 
		PipelineState*		pPipelineState,
		CommandAllocator*	pComputeCommandAllocator,
		CommandList*		pComputeCommandList,
		CommandList**		ppExecutionStage)
	{
		pComputeCommandAllocator->Reset();
		pComputeCommandList->Begin(nullptr);

		pComputeCommandList->BindComputePipeline(pPipelineState);

		uint32 textureDescriptorSetBindingIndex = 0;

		if (pRenderStage->ppBufferDescriptorSets != nullptr)
		{
			pComputeCommandList->BindDescriptorSetCompute(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, 0);
			textureDescriptorSetBindingIndex = 1;
		}

		if (pRenderStage->TextureSubDescriptorSetCount > 1)
		{
			LOG_WARNING("[RenderGraph]: Render Stage has TextureSubDescriptor > 1 and is Compute, this is currently not supported");
		}

		if (pRenderStage->ppTextureDescriptorSets != nullptr)
			pComputeCommandList->BindDescriptorSetCompute(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, textureDescriptorSetBindingIndex);

		pComputeCommandList->Dispatch(pRenderStage->Dimensions.x, pRenderStage->Dimensions.y, pRenderStage->Dimensions.z);

		pComputeCommandList->End();

		(*ppExecutionStage) = pComputeCommandList;
	}

	void RenderGraph::ExecuteRayTracingRenderStage(
		RenderStage*		pRenderStage, 
		PipelineState*		pPipelineState,
		CommandAllocator*	pComputeCommandAllocator,
		CommandList*		pComputeCommandList,
		CommandList**		ppExecutionStage)
	{
		pComputeCommandAllocator->Reset();
		pComputeCommandList->Begin(nullptr);

		pComputeCommandList->BindRayTracingPipeline(pPipelineState);

		uint32 textureDescriptorSetBindingIndex = 0;

		if (pRenderStage->ppBufferDescriptorSets != nullptr)
		{
			pComputeCommandList->BindDescriptorSetRayTracing(pRenderStage->ppBufferDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, 0);
			textureDescriptorSetBindingIndex = 1;
		}

		if (pRenderStage->TextureSubDescriptorSetCount > 1)
		{
			LOG_WARNING("[RenderGraph]: Render Stage has TextureSubDescriptor > 1 and is Ray Tracing, this is currently not supported");
		}

		if (pRenderStage->ppTextureDescriptorSets != nullptr)
			pComputeCommandList->BindDescriptorSetRayTracing(pRenderStage->ppTextureDescriptorSets[m_BackBufferIndex], pRenderStage->pPipelineLayout, textureDescriptorSetBindingIndex);

		pComputeCommandList->TraceRays(pRenderStage->Dimensions.x, pRenderStage->Dimensions.y, pRenderStage->Dimensions.z);

		pComputeCommandList->End();

		(*ppExecutionStage) = pComputeCommandList;
	}
}
