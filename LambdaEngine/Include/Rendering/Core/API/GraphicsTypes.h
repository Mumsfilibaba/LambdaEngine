#pragma once
#include "LambdaEngine.h"

#include <vector>

namespace LambdaEngine
{
    enum class EMemoryType : uint8
    {
        NONE        = 0,
        CPU_MEMORY  = 1,
        GPU_MEMORY  = 2,
    };

    enum class EFormat : uint8
    {
        NONE            = 0,
        R8G8B8A8_UNORM  = 1,
        B8G8R8A8_UNORM  = 2,
    };

	enum class EShaderType : uint32
	{
		NONE				= 0,
		MESH_SHADER			= (1 << 0),
		VERTEX_SHADER		= (1 << 1),
		GEOMETRY_SHADER		= (1 << 2),
		HULL_SHADER			= (1 << 3),
		DOMAIN_SHADER		= (1 << 4),
		PIXEL_SHADER		= (1 << 5),
		COMPUTE_SHADER		= (1 << 6),
		RAYGEN_SHADER		= (1 << 7),
		INTERSECT_SHADER	= (1 << 8),
		ANY_HIT_SHADER		= (1 << 9),
		CLOSEST_HIT_SHADER	= (1 << 10),
		MISS_SHADER			= (1 << 11),
	};

	enum class EQueueType : uint8
	{
		QUEUE_UNKNOWN	= 0,
		QUEUE_COMPUTE	= 1,
		QUEUE_GRAPHICS	= 2,
		QUEUE_COPY		= 3,
	};

	enum class ECommandListType : uint8
	{
		COMMANDLIST_PRIMARY		= 1,
		COMMANDLIST_SECONDARY	= 2
	};

	enum class EShaderLang : uint32
	{
		NONE	= 0,
		SPIRV	= (1 << 0),
	}; 

	union ShaderConstant
	{
		byte Data[4];
		float Float;
		int32 Integer;
	};

	struct ShaderDesc
	{
		const char* pSource = nullptr;
		uint32 SourceSize = 0;
		const char* pEntryPoint = "main";
		EShaderType Type = EShaderType::NONE;
		EShaderLang Lang = EShaderLang::NONE;
		std::vector<ShaderConstant> Constants;
	};

	struct Viewport
	{
		float MinDepth	= 0.0f;
		float MaxDepth	= 0.0f;
		float Width		= 0.0f;
		float Height	= 0.0f;
		float TopX		= 0.0f;
		float TopY		= 0.0f;
	};

	struct ScissorRect
	{
		uint32 Width	= 0;
		uint32 Height	= 0;
		uint32 TopX		= 0;
		uint32 TopY		= 0;
	};
	
	struct GraphicsObject
	{
		GUID_Lambda Mesh;
		GUID_Lambda Material;
	};
}
