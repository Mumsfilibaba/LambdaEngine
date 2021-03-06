#include "Rendering/ImGuiRenderer.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraph.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/DeviceAllocator.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/Buffer.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Resources/ResourceManager.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>
#include <imnodes.h>

namespace LambdaEngine
{
	ImGuiRenderer::ImGuiRenderer(const GraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice)
	{
	}

	ImGuiRenderer::~ImGuiRenderer()
	{
		imnodes::Shutdown();
		ImGui::DestroyContext();

		SAFERELEASE(m_pCopyCommandAllocator);
		SAFERELEASE(m_pCopyCommandList);
		SAFERELEASE(m_pAllocator);
		SAFERELEASE(m_pPipelineLayout);
		SAFERELEASE(m_pDescriptorHeap);
		SAFERELEASE(m_pDescriptorSet);
		SAFERELEASE(m_pRenderPass);
		SAFERELEASE(m_pVertexBuffer);
		SAFERELEASE(m_pIndexBuffer);

		for (auto textureIt = m_TextureResourceNameDescriptorSetsMap.begin(); textureIt != m_TextureResourceNameDescriptorSetsMap.end(); textureIt++)
		{
			TArray<DescriptorSet*>& descriptorSets = textureIt->second;
			for (uint32 d = 0; d < descriptorSets.GetSize(); d++)
			{
				SAFERELEASE(descriptorSets[d]);
			}
		}
		
		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppVertexCopyBuffers[b]);
			SAFERELEASE(m_ppIndexCopyBuffers[b]);
		}

		SAFEDELETE_ARRAY(m_ppVertexCopyBuffers);
		SAFEDELETE_ARRAY(m_ppIndexCopyBuffers);
		SAFERELEASE(m_pFontTexture);
		SAFERELEASE(m_pFontTextureView);
		SAFERELEASE(m_pSampler);

		SAFEDELETE_ARRAY(m_ppBackBuffers);
	}

	bool ImGuiRenderer::Init(const ImGuiRendererDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_BackBufferCount = pDesc->BackBufferCount;
		m_ppBackBuffers = DBG_NEW TextureView*[m_BackBufferCount];

		uint32 allocatorPageSize = 2 * (4 * pDesc->VertexBufferSize + 4 * pDesc->IndexBufferSize) + MEGA_BYTE(64);

		if (!InitImGui())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to initialize ImGui");
			return false;
		}

		if (!CreateCopyCommandList())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create copy command list");
			return false;
		}

		if (!CreateAllocator(allocatorPageSize))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create allocator");
			return false;
		}

		if (!CreateBuffers(pDesc->VertexBufferSize, pDesc->IndexBufferSize))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create buffers");
			return false;
		}

		if (!CreateTextures())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create textures");
			return false;
		}

		if (!CreateSamplers())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create samplers");
			return false;
		}

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSet())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create Shaders");
			return false;
		}

		m_pDescriptorSet->WriteTextureDescriptors(&m_pFontTextureView, &m_pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);

		CommonApplication::Get()->AddEventHandler(this);

		return true;
	}

	bool ImGuiRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);

		VALIDATE(pPreInitDesc->ColorAttachmentCount == 1);

		if (!CreateRenderPass(&pPreInitDesc->pColorAttachmentDesc[0]))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create RenderPass");
			return false;
		}

		if (!CreatePipelineState())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create PipelineState");
			return false;
		}
		
		return true;
	}

	void ImGuiRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void ImGuiRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	//void ImGuiRenderer::UpdateParameters(void* pData)
	//{
	//	UNREFERENCED_VARIABLE(pData);
	//}

	/*void ImGuiRenderer::UpdatePushConstants(void* pData, uint32 dataSize)
	{
		UNREFERENCED_VARIABLE(pData);
		UNREFERENCED_VARIABLE(dataSize);
	}*/

	void ImGuiRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound)
	{
		if (count == 1 || backBufferBound)
		{
			if (resourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
			{
				memcpy(m_ppBackBuffers, ppTextureViews, m_BackBufferCount * sizeof(TextureView*));
			}
			else
			{
				auto textureIt = m_TextureResourceNameDescriptorSetsMap.find(resourceName);

				if (textureIt == m_TextureResourceNameDescriptorSetsMap.end())
				{
					TArray<DescriptorSet*>& descriptorSets = m_TextureResourceNameDescriptorSetsMap[resourceName];

					if (backBufferBound)
					{
						descriptorSets.Resize(m_BackBufferCount);

						for (uint32 b = 0; b < m_BackBufferCount; b++)
						{
							DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("ImGui Custom Texture Descriptor Set", m_pPipelineLayout, 0, m_pDescriptorHeap);
							descriptorSets[b] = pDescriptorSet;

							pDescriptorSet->WriteTextureDescriptors(&ppTextureViews[b], &m_pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
						}
					}
					else
					{
						descriptorSets.Resize(count);

						for (uint32 b = 0; b < count; b++)
						{
							DescriptorSet* pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("ImGui Custom Texture Descriptor Set", m_pPipelineLayout, 0, m_pDescriptorHeap);
							descriptorSets[b] = pDescriptorSet;

							pDescriptorSet->WriteTextureDescriptors(&ppTextureViews[b], &m_pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
						}
					}
				}
				else
				{
					TArray<DescriptorSet*>& descriptorSets = m_TextureResourceNameDescriptorSetsMap[resourceName];

					if (backBufferBound)
					{
						if (descriptorSets.GetSize() == m_BackBufferCount)
						{
							for (uint32 b = 0; b < m_BackBufferCount; b++)
							{
								DescriptorSet* pDescriptorSet = descriptorSets[b];
								pDescriptorSet->WriteTextureDescriptors(&ppTextureViews[b], &m_pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
							}
						}
					}
					else
					{
						if (descriptorSets.GetSize() == count)
						{
							for (uint32 b = 0; b < count; b++)
							{
								DescriptorSet* pDescriptorSet = descriptorSets[b];
								pDescriptorSet->WriteTextureDescriptors(&ppTextureViews[b], &m_pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
							}
						}
						else
						{
							LOG_ERROR("[ImGuiRenderer]: Texture count changed between calls to UpdateTextureResource for resource \"%s\"", resourceName.c_str());
						}
					}
				}
			}
		}
		else
		{
			LOG_WARNING("[ImGuiRenderer]: Textures with count > 1 and not BackBufferBound is not implemented");
		}
	}

	void ImGuiRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppBuffers);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(pSizesInBytes);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(backBufferBound);
	}

	void ImGuiRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void ImGuiRenderer::NewFrame(Timestamp delta)
	{
		TSharedRef<Window> window	= CommonApplication::Get()->GetMainWindow();
		uint32 windowWidth	= window->GetWidth();
		uint32 windowHeight = window->GetHeight();

		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = float32(delta.AsSeconds());

		io.DisplaySize = ImVec2((float32)windowWidth, (float32)windowHeight);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

		ImGui::NewFrame();
	}

	void ImGuiRenderer::PrepareRender(Timestamp delta)
	{
		UNREFERENCED_VARIABLE(delta);

		ImGui::EndFrame();
		ImGui::Render();
	}

	void ImGuiRenderer::Render(CommandAllocator* pCommandAllocator, CommandList* pCommandList, uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppExecutionStage)
	{
		pCommandAllocator->Reset();
		pCommandList->Begin(nullptr);

		//Start drawing
		ImDrawData* pDrawData = ImGui::GetDrawData();
		TextureView* pBackBuffer = m_ppBackBuffers[backBufferIndex];
		uint32 width	= pBackBuffer->GetDesc().Texture->GetDesc().Width;
		uint32 height	= pBackBuffer->GetDesc().Texture->GetDesc().Height;

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass			= m_pRenderPass;
		beginRenderPassDesc.ppRenderTargets		= &pBackBuffer;
		beginRenderPassDesc.RenderTargetCount	= 1;
		beginRenderPassDesc.pDepthStencil		= nullptr;
		beginRenderPassDesc.Width				= width;
		beginRenderPassDesc.Height				= height;
		beginRenderPassDesc.Flags				= FRenderPassBeginFlags::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors		= nullptr;
		beginRenderPassDesc.ClearColorCount		= 0;
		beginRenderPassDesc.Offset.x			= 0;
		beginRenderPassDesc.Offset.y			= 0;

		if (pDrawData == nullptr || pDrawData->CmdListsCount == 0)
		{
			//Begin and End RenderPass to transition Texture State (Lazy)
			pCommandList->BeginRenderPass(&beginRenderPassDesc);
			pCommandList->EndRenderPass();

			pCommandList->End();

			(*ppExecutionStage) = pCommandList;
			return;
		}

		{
			Buffer* pVertexCopyBuffer	= m_ppVertexCopyBuffers[modFrameIndex];
			Buffer* pIndexCopyBuffer	= m_ppIndexCopyBuffers[modFrameIndex];

			uint32 vertexBufferSize		= 0;
			uint32 indexBufferSize		= 0;
			byte* pVertexMapping		= reinterpret_cast<byte*>(pVertexCopyBuffer->Map());
			byte* pIndexMapping			= reinterpret_cast<byte*>(pIndexCopyBuffer->Map());

			for (int n = 0; n < pDrawData->CmdListsCount; n++)
			{
				const ImDrawList* pDrawList = pDrawData->CmdLists[n];

				memcpy(pVertexMapping + vertexBufferSize,	pDrawList->VtxBuffer.Data, pDrawList->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(pIndexMapping + indexBufferSize,		pDrawList->IdxBuffer.Data, pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx));

				vertexBufferSize	+= pDrawList->VtxBuffer.Size * sizeof(ImDrawVert);
				indexBufferSize		+= pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx);
			}

			pVertexCopyBuffer->Unmap();
			pIndexCopyBuffer->Unmap();

			pCommandList->CopyBuffer(pVertexCopyBuffer, 0, m_pVertexBuffer, 0, vertexBufferSize);
			pCommandList->CopyBuffer(pIndexCopyBuffer, 0, m_pIndexBuffer, 0, indexBufferSize);
		}

		pCommandList->BeginRenderPass(&beginRenderPassDesc);
	
		Viewport viewport = {};
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float32)width;
		viewport.Height		= (float32)height;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;

		pCommandList->SetViewports(&viewport, 0, 1);

		uint64 offset = 0;
		pCommandList->BindVertexBuffers(&m_pVertexBuffer, 0, &offset, 1);
		pCommandList->BindIndexBuffer(m_pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT16);

		// Setup scale and translation:
		// Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
		{
			float32 pScale[2];
			pScale[0] = 2.0f / pDrawData->DisplaySize.x;
			pScale[1] = 2.0f / pDrawData->DisplaySize.y;
			float pTranslate[2];
			pTranslate[0] = -1.0f - pDrawData->DisplayPos.x * pScale[0];
			pTranslate[1] = -1.0f - pDrawData->DisplayPos.y * pScale[1];

			pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, pScale,		2 * sizeof(float32), 0 * sizeof(float32));
			pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, pTranslate,	2 * sizeof(float32), 2 * sizeof(float32));
		}

		// Will project scissor/clipping rectangles into framebuffer space
		ImVec2 clipOffset		= pDrawData->DisplayPos;       // (0,0) unless using multi-viewports
		ImVec2 clipScale		= pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		// Render command lists
		// (Because we merged all buffers into a single one, we maintain our own offset into them)
		int32 globalVertexOffset = 0;
		int32 globalIndexOffset = 0;

		for (int32 n = 0; n < pDrawData->CmdListsCount; n++)
		{
			const ImDrawList* pCmdList = pDrawData->CmdLists[n];
			for (int32 i = 0; i < pCmdList->CmdBuffer.Size; i++)
			{
				const ImDrawCmd* pCmd = &pCmdList->CmdBuffer[i];
				// Project scissor/clipping rectangles into framebuffer space
				ImVec4 clipRect;
				clipRect.x = (pCmd->ClipRect.x - clipOffset.x) * clipScale.x;
				clipRect.y = (pCmd->ClipRect.y - clipOffset.y) * clipScale.y;
				clipRect.z = (pCmd->ClipRect.z - clipOffset.x) * clipScale.x;
				clipRect.w = (pCmd->ClipRect.w - clipOffset.y) * clipScale.y;

				if (clipRect.x < viewport.Width && clipRect.y < viewport.Height && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
				{
					// Negative offsets are illegal for vkCmdSetScissor
					if (clipRect.x < 0.0f)
						clipRect.x = 0.0f;
					if (clipRect.y < 0.0f)
						clipRect.y = 0.0f;

					// Apply scissor/clipping rectangle
					ScissorRect scissorRect = {};
					scissorRect.x				= (int32)clipRect.x;
					scissorRect.y				= (int32)clipRect.y;
					scissorRect.Width			= uint32(clipRect.z - clipRect.x);
					scissorRect.Height			= uint32(clipRect.w - clipRect.y);

					pCommandList->SetScissorRects(&scissorRect, 0, 1);

					if (pCmd->TextureId)
					{
						ImGuiTexture*		pImGuiTexture	= reinterpret_cast<ImGuiTexture*>(pCmd->TextureId);
						auto textureIt = m_TextureResourceNameDescriptorSetsMap.find(pImGuiTexture->ResourceName);

						if (textureIt == m_TextureResourceNameDescriptorSetsMap.end()) continue;

						GUID_Lambda vertexShaderGUID	= pImGuiTexture->VertexShaderGUID == GUID_NONE	? m_VertexShaderGUID	: pImGuiTexture->VertexShaderGUID;
						GUID_Lambda pixelShaderGUID		= pImGuiTexture->PixelShaderGUID == GUID_NONE	? m_PixelShaderGUID		: pImGuiTexture->PixelShaderGUID;

						auto vertexShaderIt = m_ShadersIDToPipelineStateIDMap.find(vertexShaderGUID);

						if (vertexShaderIt != m_ShadersIDToPipelineStateIDMap.end())
						{
							auto pixelShaderIt = vertexShaderIt->second.find(pixelShaderGUID);

							if (pixelShaderIt != vertexShaderIt->second.end())
							{
								PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(pixelShaderIt->second);
								pCommandList->BindGraphicsPipeline(pPipelineState);
							}
							else
							{
								uint64 pipelineGUID = InternalCreatePipelineState(vertexShaderGUID, pixelShaderGUID);

								vertexShaderIt->second.insert({ pixelShaderGUID, pipelineGUID });

								PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(pipelineGUID);
								pCommandList->BindGraphicsPipeline(pPipelineState);
							}
						}
						else
						{
							uint64 pipelineGUID = InternalCreatePipelineState(vertexShaderGUID, pixelShaderGUID);

							THashTable<GUID_Lambda, uint64> pixelShaderToPipelineStateMap;
							pixelShaderToPipelineStateMap.insert({ pixelShaderGUID, pipelineGUID });
							m_ShadersIDToPipelineStateIDMap.insert({ vertexShaderGUID, pixelShaderToPipelineStateMap });

							PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(pipelineGUID);
							pCommandList->BindGraphicsPipeline(pPipelineState);
						}

						pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, pImGuiTexture->ChannelMul,				4 * sizeof(float32),	4 * sizeof(float32));
						pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, pImGuiTexture->ChannelAdd,				4 * sizeof(float32),	8 * sizeof(float32));
						pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, &pImGuiTexture->ReservedIncludeMask,		sizeof(uint32),		12 * sizeof(float32));

						const TArray<DescriptorSet*>& descriptorSets = textureIt->second;

						//Todo: Allow other sizes than 1
						if (descriptorSets.GetSize() == 1)
						{
							pCommandList->BindDescriptorSetGraphics(descriptorSets[0], m_pPipelineLayout, 0);
						}
						else
						{
							pCommandList->BindDescriptorSetGraphics(descriptorSets[backBufferIndex], m_pPipelineLayout, 0);
						}
					}
					else
					{
						constexpr const float32 DEFAULT_CHANNEL_MUL[4]					= { 1.0f, 1.0f, 1.0f, 1.0f };
						constexpr const float32 DEFAULT_CHANNEL_ADD[4]					= { 0.0f, 0.0f, 0.0f, 0.0f };
						constexpr const uint32 DEFAULT_CHANNEL_RESERVED_INCLUDE_MASK	= 0x00008421;  //0000 0000 0000 0000 1000 0100 0010 0001

						PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(m_PipelineStateID);
						pCommandList->BindGraphicsPipeline(pPipelineState);

						pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, DEFAULT_CHANNEL_MUL,						4 * sizeof(float32),	4 * sizeof(float32));
						pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, DEFAULT_CHANNEL_ADD,						4 * sizeof(float32),	8 * sizeof(float32));
						pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, &DEFAULT_CHANNEL_RESERVED_INCLUDE_MASK,		sizeof(uint32),		12 * sizeof(float32));

						pCommandList->BindDescriptorSetGraphics(m_pDescriptorSet, m_pPipelineLayout, 0);
					}

					// Draw
					pCommandList->DrawIndexInstanced(pCmd->ElemCount, 1, pCmd->IdxOffset + globalIndexOffset, pCmd->VtxOffset + globalVertexOffset, 0);
				}
			}

			globalIndexOffset	+= pCmdList->IdxBuffer.Size;
			globalVertexOffset	+= pCmdList->VtxBuffer.Size;
		}

		pCommandList->EndRenderPass();
		pCommandList->End();

		(*ppExecutionStage) = pCommandList;
	}

	void ImGuiRenderer::OnMouseMoved(int32 x, int32 y)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2(float32(x), float32(y));
	}

	void ImGuiRenderer::OnButtonPressed(EMouseButton button, uint32 modifierMask)
	{
		UNREFERENCED_VARIABLE(modifierMask);

		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[button - 1] = true;
	}

	void ImGuiRenderer::OnButtonReleased(EMouseButton button)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[button - 1] = false;
	}

	void ImGuiRenderer::OnMouseScrolled(int32 deltaX, int32 deltaY)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseWheelH	+= (float32)deltaX;
		io.MouseWheel	+= (float32)deltaY;
	}

	void ImGuiRenderer::OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		UNREFERENCED_VARIABLE(isRepeat);
		UNREFERENCED_VARIABLE(modifierMask);

		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[key] = true;
		io.KeyCtrl	= io.KeysDown[EKey::KEY_LEFT_CONTROL]	|| io.KeysDown[EKey::KEY_RIGHT_CONTROL];
		io.KeyShift	= io.KeysDown[EKey::KEY_LEFT_SHIFT]		|| io.KeysDown[EKey::KEY_RIGHT_SHIFT];
		io.KeyAlt	= io.KeysDown[EKey::KEY_LEFT_ALT]		|| io.KeysDown[EKey::KEY_RIGHT_ALT];
		io.KeySuper	= io.KeysDown[EKey::KEY_LEFT_SUPER]		|| io.KeysDown[EKey::KEY_RIGHT_SUPER];
	}

	void ImGuiRenderer::OnKeyReleased(EKey key)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[key] = false;
		io.KeyCtrl	= io.KeysDown[EKey::KEY_LEFT_CONTROL]	|| io.KeysDown[EKey::KEY_RIGHT_CONTROL];
		io.KeyShift	= io.KeysDown[EKey::KEY_LEFT_SHIFT]		|| io.KeysDown[EKey::KEY_RIGHT_SHIFT];
		io.KeyAlt	= io.KeysDown[EKey::KEY_LEFT_ALT]		|| io.KeysDown[EKey::KEY_RIGHT_ALT];
		io.KeySuper	= io.KeysDown[EKey::KEY_LEFT_SUPER]		|| io.KeysDown[EKey::KEY_RIGHT_SUPER];
	}

	void ImGuiRenderer::OnKeyTyped(uint32 character)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.AddInputCharacter(character);
	}

	ImGuiContext* ImGuiRenderer::GetImguiContext()
	{
		return ImGui::GetCurrentContext();
	}

	bool ImGuiRenderer::InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		imnodes::Initialize();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendPlatformName = "Lambda Engine";
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

		io.KeyMap[ImGuiKey_Tab]			= EKey::KEY_TAB;
		io.KeyMap[ImGuiKey_LeftArrow]	= EKey::KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow]	= EKey::KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow]		= EKey::KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow]	= EKey::KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp]		= EKey::KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown]	= EKey::KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home]		= EKey::KEY_HOME;
		io.KeyMap[ImGuiKey_End]			= EKey::KEY_END;
		io.KeyMap[ImGuiKey_Insert]		= EKey::KEY_INSERT;
		io.KeyMap[ImGuiKey_Delete]		= EKey::KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace]	= EKey::KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Space]		= EKey::KEY_SPACE;
		io.KeyMap[ImGuiKey_Enter]		= EKey::KEY_ENTER;
		io.KeyMap[ImGuiKey_Escape]		= EKey::KEY_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = EKey::KEY_KEYPAD_ENTER;
		io.KeyMap[ImGuiKey_A]			= EKey::KEY_A;
		io.KeyMap[ImGuiKey_C]			= EKey::KEY_C;
		io.KeyMap[ImGuiKey_V]			= EKey::KEY_V;
		io.KeyMap[ImGuiKey_X]			= EKey::KEY_X;
		io.KeyMap[ImGuiKey_Y]			= EKey::KEY_Y;
		io.KeyMap[ImGuiKey_Z]			= EKey::KEY_Z;

#ifdef LAMBDA_PLATFORM_WINDOWS
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		io.ImeWindowHandle = window->GetHandle();
#endif

		//Todo: Implement clipboard handling
		//io.SetClipboardTextFn = ImGuiSetClipboardText;
		//io.GetClipboardTextFn = ImGuiGetClipboardText;
		//io.ClipboardUserData = pWindow;

		ImGui::StyleColorsDark();
		ImGui::GetStyle().WindowRounding	= 0.0f;
		ImGui::GetStyle().ChildRounding		= 0.0f;
		ImGui::GetStyle().FrameRounding		= 0.0f;
		ImGui::GetStyle().GrabRounding		= 0.0f;
		ImGui::GetStyle().PopupRounding		= 0.0f;
		ImGui::GetStyle().ScrollbarRounding = 0.0f;

		return true;
	}

	bool ImGuiRenderer::CreateCopyCommandList()
	{
		m_pCopyCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("ImGui Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

		if (m_pCopyCommandAllocator == nullptr)
		{
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName			= "ImGui Copy Command List";
		commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_pCopyCommandList = m_pGraphicsDevice->CreateCommandList(m_pCopyCommandAllocator, &commandListDesc);

		return m_pCopyCommandList != nullptr;
	}

	bool ImGuiRenderer::CreateAllocator(uint32 pageSize)
	{
		DeviceAllocatorDesc allocatorDesc = {};
		allocatorDesc.DebugName			= "ImGui Allocator";
		allocatorDesc.PageSizeInBytes	= pageSize;

		m_pAllocator = m_pGraphicsDevice->CreateDeviceAllocator(&allocatorDesc);

		return m_pAllocator != nullptr;
	}

	bool ImGuiRenderer::CreateBuffers(uint32 vertexBufferSize, uint32 indexBufferSize)
	{
		BufferDesc vertexBufferDesc = {};
		vertexBufferDesc.DebugName		= "ImGui Vertex Buffer";
		vertexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
		vertexBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_VERTEX_BUFFER;
		vertexBufferDesc.SizeInBytes	= vertexBufferSize;

		m_pVertexBuffer = m_pGraphicsDevice->CreateBuffer(&vertexBufferDesc, m_pAllocator);
		if (!m_pVertexBuffer)
		{
			return false;
		}

		BufferDesc indexBufferDesc = {};
		indexBufferDesc.DebugName	= "ImGui Index Buffer";
		indexBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		indexBufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_INDEX_BUFFER;
		indexBufferDesc.SizeInBytes	= vertexBufferSize;
		
		m_pIndexBuffer = m_pGraphicsDevice->CreateBuffer(&indexBufferDesc, m_pAllocator);
		if (!m_pIndexBuffer)
		{
			return false;
		}

		BufferDesc vertexCopyBufferDesc = {};
		vertexCopyBufferDesc.DebugName		= "ImGui Vertex Copy Buffer";
		vertexCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		vertexCopyBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_COPY_SRC;
		vertexCopyBufferDesc.SizeInBytes	= vertexBufferSize;

		BufferDesc indexCopyBufferDesc = {};
		indexCopyBufferDesc.DebugName	= "ImGui Index Copy Buffer";
		indexCopyBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		indexCopyBufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
		indexCopyBufferDesc.SizeInBytes	= indexBufferSize;

		m_ppVertexCopyBuffers = DBG_NEW Buffer * [m_BackBufferCount];
		m_ppIndexCopyBuffers = DBG_NEW Buffer * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			Buffer* pVertexBuffer = m_pGraphicsDevice->CreateBuffer(&vertexCopyBufferDesc, m_pAllocator);
			Buffer* pIndexBuffer = m_pGraphicsDevice->CreateBuffer(&indexCopyBufferDesc, m_pAllocator);

			if (pVertexBuffer != nullptr && pIndexBuffer != nullptr)
			{
				m_ppVertexCopyBuffers[b] = pVertexBuffer;
				m_ppIndexCopyBuffers[b] = pIndexBuffer;
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	bool ImGuiRenderer::CreateTextures()
	{
		ImGuiIO& io = ImGui::GetIO();

		uint8* pPixels = nullptr;
		int32 width = 0;
		int32 height = 0;
		io.Fonts->GetTexDataAsRGBA32(&pPixels, &width, &height);

		int64 textureSize = 4 * width * height;

		BufferDesc fontBufferDesc = {};
		fontBufferDesc.DebugName	= "ImGui Font Buffer";
		fontBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		fontBufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
		fontBufferDesc.SizeInBytes	= textureSize;

		Buffer* pFontBuffer = m_pGraphicsDevice->CreateBuffer(&fontBufferDesc, m_pAllocator);
		if (pFontBuffer == nullptr)
		{
			return false;
		}

		void* pMapped = pFontBuffer->Map();
		memcpy(pMapped, pPixels, textureSize);
		pFontBuffer->Unmap();

		TextureDesc fontTextureDesc = {};
		fontTextureDesc.DebugName	= "ImGui Font Texture";
		fontTextureDesc.MemoryType  = EMemoryType::MEMORY_TYPE_GPU;
		fontTextureDesc.Format		= EFormat::FORMAT_R8G8B8A8_UNORM;
		fontTextureDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
		fontTextureDesc.Flags		= FTextureFlags::TEXTURE_FLAG_COPY_DST | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		fontTextureDesc.Width		= width;
		fontTextureDesc.Height		= height;
		fontTextureDesc.Depth		= 1;
		fontTextureDesc.ArrayCount	= 1;
		fontTextureDesc.Miplevels	= 1;
		fontTextureDesc.SampleCount = 1;

		m_pFontTexture = m_pGraphicsDevice->CreateTexture(&fontTextureDesc, m_pAllocator);
		if (m_pFontTexture == nullptr)
		{
			return false;
		}

		CopyTextureFromBufferDesc copyDesc = {};
		copyDesc.SrcOffset		= 0;
		copyDesc.SrcRowPitch	= 0;
		copyDesc.SrcHeight		= 0;
		copyDesc.Width			= width;
		copyDesc.Height			= height;
		copyDesc.Depth			= 1;
		copyDesc.Miplevel		= 0;
		copyDesc.MiplevelCount  = 1;
		copyDesc.ArrayIndex		= 0;
		copyDesc.ArrayCount		= 1;

		m_pCopyCommandAllocator->Reset();
		m_pCopyCommandList->Begin(nullptr);

		PipelineTextureBarrierDesc transitionToCopyDstBarrier = {};
		transitionToCopyDstBarrier.pTexture				= m_pFontTexture;
		transitionToCopyDstBarrier.StateBefore			= ETextureState::TEXTURE_STATE_UNKNOWN;
		transitionToCopyDstBarrier.StateAfter			= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToCopyDstBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.SrcMemoryAccessFlags	= 0;
		transitionToCopyDstBarrier.DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToCopyDstBarrier.TextureFlags			= fontTextureDesc.Flags;
		transitionToCopyDstBarrier.Miplevel				= 0;
		transitionToCopyDstBarrier.MiplevelCount		= fontTextureDesc.Miplevels;
		transitionToCopyDstBarrier.ArrayIndex			= 0;
		transitionToCopyDstBarrier.ArrayCount			= fontTextureDesc.ArrayCount;

		m_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, &transitionToCopyDstBarrier, 1);
		m_pCopyCommandList->CopyTextureFromBuffer(pFontBuffer, m_pFontTexture, copyDesc);
		
		PipelineTextureBarrierDesc transitionToShaderReadBarrier = {};
		transitionToShaderReadBarrier.pTexture				= m_pFontTexture;
		transitionToShaderReadBarrier.StateBefore			= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToShaderReadBarrier.StateAfter			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		transitionToShaderReadBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToShaderReadBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToShaderReadBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToShaderReadBarrier.DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		transitionToShaderReadBarrier.TextureFlags			= fontTextureDesc.Flags;
		transitionToShaderReadBarrier.Miplevel				= 0;
		transitionToShaderReadBarrier.MiplevelCount			= fontTextureDesc.Miplevels;
		transitionToShaderReadBarrier.ArrayIndex			= 0;
		transitionToShaderReadBarrier.ArrayCount			= fontTextureDesc.ArrayCount;

		m_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM, &transitionToShaderReadBarrier, 1);

		m_pCopyCommandList->End();

		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&m_pCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, nullptr, 0, nullptr, 0);
		RenderSystem::GetGraphicsQueue()->Flush();

		SAFEDELETE(pFontBuffer);

		TextureViewDesc fontTextureViewDesc = {};
		fontTextureViewDesc.DebugName		= "ImGui Font Texture View";
		fontTextureViewDesc.Texture			= m_pFontTexture;
		fontTextureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		fontTextureViewDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		fontTextureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		fontTextureViewDesc.MiplevelCount	= 1;
		fontTextureViewDesc.ArrayCount		= 1;
		fontTextureViewDesc.Miplevel		= 0;
		fontTextureViewDesc.ArrayIndex		= 0;

		m_pFontTextureView = m_pGraphicsDevice->CreateTextureView(&fontTextureViewDesc);
		if (!m_pFontTextureView)
		{
			return false;
		}

		return true;
	}

	bool ImGuiRenderer::CreateSamplers()
	{
		SamplerDesc samplerDesc = {};
		samplerDesc.DebugName			= "ImGui Sampler";
		samplerDesc.MinFilter			= EFilterType::FILTER_TYPE_NEAREST;
		samplerDesc.MagFilter			= EFilterType::FILTER_TYPE_NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::MIPMAP_MODE_NEAREST;
		samplerDesc.AddressModeU		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		samplerDesc.AddressModeV		= samplerDesc.AddressModeU;
		samplerDesc.AddressModeW		= samplerDesc.AddressModeU;
		samplerDesc.MipLODBias			= 0.0f; 
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		m_pSampler = m_pGraphicsDevice->CreateSampler(&samplerDesc);

		return m_pSampler != nullptr;
	}

	bool ImGuiRenderer::CreatePipelineLayout()
	{
		DescriptorBindingDesc descriptorBindingDesc = {};
		descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		descriptorBindingDesc.DescriptorCount	= 1;
		descriptorBindingDesc.Binding			= 0;
		descriptorBindingDesc.ShaderStageMask	= FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc = {};
		descriptorSetLayoutDesc.DescriptorBindings		= { descriptorBindingDesc };

		ConstantRangeDesc constantRangeVertexDesc = { };
		constantRangeVertexDesc.ShaderStageFlags	= FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER;
		constantRangeVertexDesc.SizeInBytes			= 4 * sizeof(float32);
		constantRangeVertexDesc.OffsetInBytes		= 0;

		ConstantRangeDesc constantRangePixelDesc = { };
		constantRangePixelDesc.ShaderStageFlags	= FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER;
		constantRangePixelDesc.SizeInBytes		= 8 * sizeof(float32) + sizeof(uint32);
		constantRangePixelDesc.OffsetInBytes	= constantRangeVertexDesc.SizeInBytes;

		ConstantRangeDesc pConstantRanges[2] = { constantRangeVertexDesc, constantRangePixelDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName			= "ImGui Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts	= { descriptorSetLayoutDesc };
		pipelineLayoutDesc.ConstantRanges		= { pConstantRanges[0], pConstantRanges[1] };

		m_pPipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_pPipelineLayout != nullptr;
	}

	bool ImGuiRenderer::CreateDescriptorSet()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount					= 1;
		descriptorCountDesc.TextureDescriptorCount					= 1;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount	= 64;
		descriptorCountDesc.ConstantBufferDescriptorCount			= 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount	= 1;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount	= 1;
		descriptorCountDesc.AccelerationStructureDescriptorCount	= 1;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName			= "ImGui Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount	= 64;
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_pDescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);

		if (m_pDescriptorHeap == nullptr)
			return false;

		m_pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("ImGui Descriptor Set", m_pPipelineLayout, 0, m_pDescriptorHeap);

		return m_pDescriptorSet != nullptr;
	}

	bool ImGuiRenderer::CreateShaders()
	{
		m_VertexShaderGUID		= ResourceManager::LoadShaderFromFile("ImGuiVertex.vert", FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderGUID		= ResourceManager::LoadShaderFromFile("ImGuiPixel.frag", FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderGUID != GUID_NONE && m_PixelShaderGUID != GUID_NONE;
	}

	bool ImGuiRenderer::CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc)
	{
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= pBackBufferAttachmentDesc->InitialState;
		colorAttachmentDesc.FinalState		= pBackBufferAttachmentDesc->FinalState;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates			= { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		subpassDesc.DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_DONT_CARE;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass	= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass	= 0;
		subpassDependencyDesc.SrcAccessMask	= 0;
		subpassDependencyDesc.DstAccessMask	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName			= "ImGui Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_pRenderPass = m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool ImGuiRenderer::CreatePipelineState()
	{
		m_PipelineStateID = InternalCreatePipelineState(m_VertexShaderGUID, m_PixelShaderGUID);

		THashTable<GUID_Lambda, uint64> pixelShaderToPipelineStateMap;
		pixelShaderToPipelineStateMap.insert({ m_PixelShaderGUID, m_PipelineStateID });
		m_ShadersIDToPipelineStateIDMap.insert({ m_VertexShaderGUID, pixelShaderToPipelineStateMap });

		return true;
	}

	uint64 ImGuiRenderer::InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader)
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName			= "ImGui Pipeline State";
		pipelineStateDesc.RenderPass		= m_pRenderPass;
		pipelineStateDesc.PipelineLayout	= m_pPipelineLayout;

		pipelineStateDesc.DepthStencilState.CompareOp			= ECompareOp::COMPARE_OP_NEVER;
		pipelineStateDesc.DepthStencilState.DepthTestEnable		= false;
		pipelineStateDesc.DepthStencilState.DepthWriteEnable	= false;

		pipelineStateDesc.BlendState.BlendAttachmentStates =
		{
			{
				EBlendOp::BLEND_OP_ADD,
				EBlendFactor::BLEND_FACTOR_SRC_ALPHA,
				EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
				EBlendOp::BLEND_OP_ADD,
				EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
				EBlendFactor::BLEND_FACTOR_SRC_ALPHA,
				COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A,
				true
			}
		};

		pipelineStateDesc.InputLayout =
		{
			{ "POSITION",	0, sizeof(ImDrawVert), EVertexInputRate::VERTEX_INPUT_PER_VERTEX, 0, IM_OFFSETOF(ImDrawVert, pos),	EFormat::FORMAT_R32G32_SFLOAT },
			{ "TEXCOORD",	0, sizeof(ImDrawVert), EVertexInputRate::VERTEX_INPUT_PER_VERTEX, 1, IM_OFFSETOF(ImDrawVert, uv),	EFormat::FORMAT_R32G32_SFLOAT },
			{ "COLOR",		0, sizeof(ImDrawVert), EVertexInputRate::VERTEX_INPUT_PER_VERTEX, 2, IM_OFFSETOF(ImDrawVert, col),	EFormat::FORMAT_R8G8B8A8_UNORM },
		};

		pipelineStateDesc.VertexShader.ShaderGUID	= vertexShader;
		pipelineStateDesc.PixelShader.ShaderGUID	= pixelShader;

		return PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
	}
}
