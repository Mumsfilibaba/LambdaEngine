#pragma once
#include "Containers/THashTable.h"

#include "Utilities/HashUtilities.h"

#include "Threading/API/Spinlock.h"

#include "Rendering/Core/API/RenderPass.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	struct FrameBufferCacheKey
	{
		VkImageView		ColorAttachmentsViews[MAX_COLOR_ATTACHMENTS];
		uint32			ColorAttachmentViewCount	= 0;
		VkImageView		DepthStencilView			= VK_NULL_HANDLE;
		VkRenderPass	RenderPass					= VK_NULL_HANDLE;
		mutable size_t	Hash						= 0;

		size_t GetHash() const
		{
			if (Hash == 0)
			{
				Hash = std::hash<uint64>()(uint64(RenderPass));
				for (uint32 i = 0; i < ColorAttachmentViewCount; i++)
				{
					HashCombine<uint64>(Hash, uint64(ColorAttachmentsViews[i]));
				}

				HashCombine<uint64>(Hash, uint64(DepthStencilView));
			}

			return Hash;
		}

		bool Contains(VkImageView imageView) const
		{
			for (uint32 i = 0; i < ColorAttachmentViewCount; i++)
			{
				if (ColorAttachmentsViews[i] == imageView)
				{
					return true;
				}
			}

			return (DepthStencilView == imageView);
		}

		bool Contains(VkRenderPass renderPass) const
		{
			return (renderPass == RenderPass);
		}

		bool operator==(const FrameBufferCacheKey& other) const
		{
			if (ColorAttachmentViewCount != other.ColorAttachmentViewCount)
			{
				return false;
			}

			for (uint32 i = 0; i < ColorAttachmentViewCount; i++)
			{
				if (ColorAttachmentsViews[i] != other.ColorAttachmentsViews[i])
				{
					return false;
				}
			}

			if (DepthStencilView != other.DepthStencilView)
			{
				return false;
			}

			return true;
		}
	};

	struct FrameBufferCacheKeyHasher
	{
		size_t operator()(const FrameBufferCacheKey& key) const
		{
			return key.GetHash();
		}
	};

	class FrameBufferCacheVK
	{
		using FrameBufferMap		= std::unordered_map<FrameBufferCacheKey, VkFramebuffer, FrameBufferCacheKeyHasher>;
		using FrameBufferMapEntry	= std::pair<const FrameBufferCacheKey, VkFramebuffer>;

	public:
		DECL_UNIQUE_CLASS(FrameBufferCacheVK);

		FrameBufferCacheVK(const GraphicsDeviceVK* pDevice);
		~FrameBufferCacheVK();

		void DestroyRenderPass(VkRenderPass renderPass) const;
		void DestroyImageView(VkImageView imageView) const;
		
		VkFramebuffer GetFrameBuffer(const FrameBufferCacheKey& key, uint32 width, uint32 height);

	private:
		const GraphicsDeviceVK* const	m_pDevice = nullptr;
		mutable FrameBufferMap			m_FrameBufferMap;
		mutable SpinLock 				m_Lock;
	};
}
