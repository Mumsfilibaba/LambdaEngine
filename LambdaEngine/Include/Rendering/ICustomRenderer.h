#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	struct RenderPassAttachmentDesc;

	class CommandAllocator;
	class CommandList;
	class TextureView;
	class Buffer;
	class AccelerationStructure;

	struct CustomRendererRenderGraphInitDesc
	{
		RenderPassAttachmentDesc*	pColorAttachmentDesc			= nullptr;
		uint32						ColorAttachmentCount			= 0;
		RenderPassAttachmentDesc*	pDepthStencilAttachmentDesc		= nullptr;
	};

	class ICustomRenderer
	{
	public:
		DECL_INTERFACE(ICustomRenderer);

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) = 0;

		/*
		* Called before when a/multiple buffer descriptor write(s) are discovered.
		* Allows the callee to recreate Descriptor Sets if needed.
		*/
		virtual void PreBuffersDescriptorSetWrite()		= 0;
		/*
		* Called before when a/multiple texture descriptor write(s) are discovered.
		* Allows the callee to recreate Descriptor Sets if needed.
		*/
		virtual void PreTexturesDescriptorSetWrite()	= 0;

		/*
		* Called when the Render Graph discovers a parameter update for the custom renderer.
		*	pData - The data passed to RenderGraph::UpdateRenderStageParameters
		*/
		//virtual void UpdateParameters(void* pData)		= 0;

		//virtual void UpdatePushConstants(void* pData, uint32 dataSize)	= 0;

		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given texture
		*	resourceName - Name of the resource being updated
		*	ppTextureViews - An array of textureviews that represent the update data
		*	count - Size of ppTextureViews
		*	backBufferBound - Describes if subresources are bound in array or 1 / Back buffer
		*/
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const * ppTextureViews, uint32 count, bool backBufferBound)	= 0;
		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given buffer
		*	resourceName - Name of the resource being updated
		*	ppBuffers - An array of buffers that represent the update data
		*	pOffsets - An array of offsets into each ppBuffers entry
		*	pSizesInBytes - An array of sizes representing the data size of each target sub entry in each ppBuffers entry
		*	count - size of ppBuffers, pOffsets and pSizesInBytes
		*	backBufferBound - Describes if subresources are bound in array or 1 / Back buffer
		*/
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const * ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)	= 0;
		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given acceleration structure
		*	resourceName - Name of the resource being updated
		*	pAccelerationStructure - The acceleration structure that represent the update data
		*/
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)	= 0;

		/*
		* Called when beginning a new frame to allow for initial setup
		*	delta - The frame deltatime
		*/
		virtual void NewFrame(Timestamp delta)		= 0;
		/*
		* Called when just before rendering the frame to allow for final setup
		*	delta - The frame deltatime
		*/
		virtual void PrepareRender(Timestamp delta)		= 0;

		/*
		* Called at rendertime to allow recording device commands
		*	pCommandAllocator - The command allocator to be reset
		*	pCommandList - The command list corresponding to pCommandAllocator and which to record device commands to
		*	modFrameIndex - The current Frame Index % #BackBufferImages
		*	backBufferIndex - The current Back Buffer index
		*	ppExecutionStage - A pointer to a variable which should contain the recorded command buffer if this rendering should be executed.
		*		Example:	
		*		if (executeRecording)	(*ppExecutionStage) = pCommandList;
		*		else					(*ppExecutionStage) = nullptr;
		*/
		virtual void Render(CommandAllocator* pCommandAllocator, CommandList* pCommandList, uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppExecutionStage)		= 0;

		virtual FPipelineStageFlags GetFirstPipelineStage()	= 0;
		virtual FPipelineStageFlags GetLastPipelineStage()	= 0;
	};
}

