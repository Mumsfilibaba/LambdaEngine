#include "Rendering/Renderer.h"
#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/ISwapChain.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/RenderGraph.h"

#include "Log/Log.h"
#include "Rendering/Core/API/ITextureView.h"

namespace LambdaEngine
{
	Renderer::Renderer(const IGraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice),
		m_pSwapChain(nullptr),
		m_pRenderGraph(nullptr),
		m_ppBackBufferViews(nullptr)
	{
	}

	Renderer::~Renderer()
	{
		SAFERELEASE(m_pSwapChain);
	}

	bool Renderer::Init(const RendererDesc& desc)
	{
		m_pName			= desc.pName;
		m_pRenderGraph	= desc.pRenderGraph;

		SwapChainDesc swapChainDesc = {};
		swapChainDesc.pName			= "Renderer Swap Chain";
		swapChainDesc.Format		= EFormat::FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Width			= 0;
		swapChainDesc.Height		= 0;
		swapChainDesc.BufferCount	= 3;
		swapChainDesc.SampleCount	= 1;
		swapChainDesc.VerticalSync	= false;
		
		m_pSwapChain = m_pGraphicsDevice->CreateSwapChain(desc.pWindow, RenderSystem::GetGraphicsQueue(), swapChainDesc);

		if (m_pSwapChain == nullptr)
		{
			LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton for \"%s\"", m_pName);
			return false;
		}

		uint32 backBufferCount	= m_pSwapChain->GetDesc().BufferCount;
		m_ppBackBuffers			= new ITexture*[backBufferCount];
		m_ppBackBufferViews		= new ITextureView*[backBufferCount];

		for (uint32 v = 0; v < backBufferCount; v++)
		{
			ITexture* pBackBuffer	= m_pSwapChain->GetBuffer(v);
			m_ppBackBuffers[v]		= pBackBuffer;

			TextureViewDesc textureViewDesc = {};
			textureViewDesc.pName			= "Renderer Back Buffer Texture View";
			textureViewDesc.pTexture		= pBackBuffer;
			textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET;
			textureViewDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
			textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
			textureViewDesc.MiplevelCount	= 1;
			textureViewDesc.ArrayCount		= 1;
			textureViewDesc.Miplevel		= 0;
			textureViewDesc.ArrayIndex		= 0;
			
			ITextureView* pBackBufferView	= m_pGraphicsDevice->CreateTextureView(textureViewDesc);

			if (pBackBufferView == nullptr)
			{
				LOG_ERROR("[Renderer]: Could not create Back Buffer View of Back Buffer Index %u in Renderer \"%s\"", v, m_pName);
				return false;
			}
			
			m_ppBackBufferViews[v]			= pBackBufferView;
		}
		
		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName						= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
		resourceUpdateDesc.ExternalTextureUpdate.ppTextures		= m_ppBackBuffers;
		resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_ppBackBufferViews;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		
		return true;
	}
	
	void Renderer::Render()
	{
		m_pRenderGraph->Render(m_FrameIndex, m_pSwapChain->GetCurrentBackBufferIndex());
		
		m_pSwapChain->Present();

		m_FrameIndex++;
	}
}