#include "Resources/ResourceLoader.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Rendering/RenderSystem.h"

#include "Audio/AudioSystem.h"

#include "Resources/STB.h"

#include "Log/Log.h"

#include "Containers/THashTable.h"

#include <glslangStandAlone/DirStackFileIncluder.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/MachineIndependent/reflection.h>

#include <tiny_obj_loader.h>
#include <cstdio>

namespace LambdaEngine
{
	DeviceAllocator*		ResourceLoader::s_pAllocator				= nullptr;
	CommandAllocator*		ResourceLoader::s_pCopyCommandAllocator		= nullptr;
	CommandList*			ResourceLoader::s_pCopyCommandList			= nullptr;
	Fence*					ResourceLoader::s_pCopyFence				= nullptr;
	uint64					ResourceLoader::s_SignalValue				= 1;

	/*
	*  --------------------------glslang Helpers Begin---------------------------------
	*/

	static const TBuiltInResource* GetDefaultBuiltInResources()
	{
		static TBuiltInResource defaultBuiltInResources = {};

		defaultBuiltInResources.maxLights									= 32;
		defaultBuiltInResources.maxClipPlanes								= 6;
		defaultBuiltInResources.maxTextureUnits								= 32;
		defaultBuiltInResources.maxTextureCoords							= 32;
		defaultBuiltInResources.maxVertexAttribs							= 64;
		defaultBuiltInResources.maxVertexUniformComponents					= 4096;
		defaultBuiltInResources.maxVaryingFloats							= 64;
		defaultBuiltInResources.maxVertexTextureImageUnits					= 32;
		defaultBuiltInResources.maxCombinedTextureImageUnits				= 80;
		defaultBuiltInResources.maxTextureImageUnits						= 32;
		defaultBuiltInResources.maxFragmentUniformComponents				= 4096;
		defaultBuiltInResources.maxDrawBuffers								= 32;
		defaultBuiltInResources.maxVertexUniformVectors						= 128;
		defaultBuiltInResources.maxVaryingVectors							= 8;
		defaultBuiltInResources.maxFragmentUniformVectors					= 16;
		defaultBuiltInResources.maxVertexOutputVectors						= 16;
		defaultBuiltInResources.maxFragmentInputVectors						= 15;
		defaultBuiltInResources.minProgramTexelOffset						= -8;
		defaultBuiltInResources.maxProgramTexelOffset						= 7;
		defaultBuiltInResources.maxClipDistances							= 8;
		defaultBuiltInResources.maxComputeWorkGroupCountX					= 65535;
		defaultBuiltInResources.maxComputeWorkGroupCountY					= 65535;
		defaultBuiltInResources.maxComputeWorkGroupCountZ					= 65535;
		defaultBuiltInResources.maxComputeWorkGroupSizeX					= 1024;
		defaultBuiltInResources.maxComputeWorkGroupSizeY					= 1024;
		defaultBuiltInResources.maxComputeWorkGroupSizeZ					= 64;
		defaultBuiltInResources.maxComputeUniformComponents					= 1024;
		defaultBuiltInResources.maxComputeTextureImageUnits					= 16;
		defaultBuiltInResources.maxComputeImageUniforms						= 8;
		defaultBuiltInResources.maxComputeAtomicCounters					= 8;
		defaultBuiltInResources.maxComputeAtomicCounterBuffers				= 1;
		defaultBuiltInResources.maxVaryingComponents						= 60;
		defaultBuiltInResources.maxVertexOutputComponents					= 64;
		defaultBuiltInResources.maxGeometryInputComponents					= 64;
		defaultBuiltInResources.maxGeometryOutputComponents					= 128;
		defaultBuiltInResources.maxFragmentInputComponents					= 128;
		defaultBuiltInResources.maxImageUnits								= 8;
		defaultBuiltInResources.maxCombinedImageUnitsAndFragmentOutputs		= 8;
		defaultBuiltInResources.maxCombinedShaderOutputResources			= 8;
		defaultBuiltInResources.maxImageSamples								= 0;
		defaultBuiltInResources.maxVertexImageUniforms						= 0;
		defaultBuiltInResources.maxTessControlImageUniforms					= 0;
		defaultBuiltInResources.maxTessEvaluationImageUniforms				= 0;
		defaultBuiltInResources.maxGeometryImageUniforms					= 0;
		defaultBuiltInResources.maxFragmentImageUniforms					= 8;
		defaultBuiltInResources.maxCombinedImageUniforms					= 8;
		defaultBuiltInResources.maxGeometryTextureImageUnits				= 16;
		defaultBuiltInResources.maxGeometryOutputVertices					= 256;
		defaultBuiltInResources.maxGeometryTotalOutputComponents			= 1024;
		defaultBuiltInResources.maxGeometryUniformComponents				= 1024;
		defaultBuiltInResources.maxGeometryVaryingComponents				= 64;
		defaultBuiltInResources.maxTessControlInputComponents				= 128;
		defaultBuiltInResources.maxTessControlOutputComponents				= 128;
		defaultBuiltInResources.maxTessControlTextureImageUnits				= 16;
		defaultBuiltInResources.maxTessControlUniformComponents				= 1024;
		defaultBuiltInResources.maxTessControlTotalOutputComponents			= 4096;
		defaultBuiltInResources.maxTessEvaluationInputComponents			= 128;
		defaultBuiltInResources.maxTessEvaluationOutputComponents			= 128;
		defaultBuiltInResources.maxTessEvaluationTextureImageUnits			= 16;
		defaultBuiltInResources.maxTessEvaluationUniformComponents			= 1024;
		defaultBuiltInResources.maxTessPatchComponents						= 120;
		defaultBuiltInResources.maxPatchVertices							= 32;
		defaultBuiltInResources.maxTessGenLevel								= 64;
		defaultBuiltInResources.maxViewports								= 16;
		defaultBuiltInResources.maxVertexAtomicCounters						= 0;
		defaultBuiltInResources.maxTessControlAtomicCounters				= 0;
		defaultBuiltInResources.maxTessEvaluationAtomicCounters				= 0;
		defaultBuiltInResources.maxGeometryAtomicCounters					= 0;
		defaultBuiltInResources.maxFragmentAtomicCounters					= 8;
		defaultBuiltInResources.maxCombinedAtomicCounters					= 8;
		defaultBuiltInResources.maxAtomicCounterBindings					= 1;
		defaultBuiltInResources.maxVertexAtomicCounterBuffers				= 0;
		defaultBuiltInResources.maxTessControlAtomicCounterBuffers			= 0;
		defaultBuiltInResources.maxTessEvaluationAtomicCounterBuffers		= 0;
		defaultBuiltInResources.maxGeometryAtomicCounterBuffers				= 0;
		defaultBuiltInResources.maxFragmentAtomicCounterBuffers				= 1;
		defaultBuiltInResources.maxCombinedAtomicCounterBuffers				= 1;
		defaultBuiltInResources.maxAtomicCounterBufferSize					= 16384;
		defaultBuiltInResources.maxTransformFeedbackBuffers					= 4;
		defaultBuiltInResources.maxTransformFeedbackInterleavedComponents	= 64;
		defaultBuiltInResources.maxCullDistances							= 8;
		defaultBuiltInResources.maxCombinedClipAndCullDistances				= 8;
		defaultBuiltInResources.maxSamples									= 4;
		defaultBuiltInResources.limits.nonInductiveForLoops					= true;
		defaultBuiltInResources.limits.whileLoops							= true;
		defaultBuiltInResources.limits.doWhileLoops							= true;
		defaultBuiltInResources.limits.generalUniformIndexing				= true;
		defaultBuiltInResources.limits.generalAttributeMatrixVectorIndexing = true;
		defaultBuiltInResources.limits.generalVaryingIndexing				= true;
		defaultBuiltInResources.limits.generalSamplerIndexing				= true;
		defaultBuiltInResources.limits.generalVariableIndexing				= true;
		defaultBuiltInResources.limits.generalConstantMatrixVectorIndexing	= true;

		return &defaultBuiltInResources;
	}

	static EShLanguage ConvertShaderStageToEShLanguage(FShaderStageFlags shaderStage)
	{
		switch (shaderStage)
		{
		case FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER:			return EShLanguage::EShLangMeshNV;
		case FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER:			return EShLanguage::EShLangTaskNV;
		case FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER:		return EShLanguage::EShLangVertex;
		case FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER:		return EShLanguage::EShLangGeometry;
		case FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER:			return EShLanguage::EShLangTessControl;
		case FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER:		return EShLanguage::EShLangTessEvaluation;
		case FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER:			return EShLanguage::EShLangFragment;
		case FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER:		return EShLanguage::EShLangCompute;
		case FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER:		return EShLanguage::EShLangRayGen;
		case FShaderStageFlags::SHADER_STAGE_FLAG_INTERSECT_SHADER:		return EShLanguage::EShLangIntersect;
		case FShaderStageFlags::SHADER_STAGE_FLAG_ANY_HIT_SHADER:		return EShLanguage::EShLangAnyHit;
		case FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER:	return EShLanguage::EShLangClosestHit;
		case FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER:			return EShLanguage::EShLangMiss;

		case FShaderStageFlags::SHADER_STAGE_FLAG_NONE:
		default:
			return EShLanguage::EShLangCount;
		}
	}

	/*
	*  --------------------------glslang Helpers End---------------------------------
	*/

	bool ResourceLoader::Init()
	{
		s_pCopyCommandAllocator = RenderSystem::GetDevice()->CreateCommandAllocator("Resource Loader Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

		if (s_pCopyCommandAllocator == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Could not create Copy Command Allocator");
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName		= "Resource Loader Copy Command List";
		commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		s_pCopyCommandList = RenderSystem::GetDevice()->CreateCommandList(s_pCopyCommandAllocator, &commandListDesc);

		FenceDesc fenceDesc = {};
		fenceDesc.DebugName		= "Resource Loader Copy Fence";
		fenceDesc.InitalValue	= 0;
		s_pCopyFence = RenderSystem::GetDevice()->CreateFence(&fenceDesc);

		DeviceAllocatorDesc allocatorDesc = {};
		allocatorDesc.DebugName			= "Resource Allocator";
		allocatorDesc.PageSizeInBytes	= MEGA_BYTE(64);
		s_pAllocator = RenderSystem::GetDevice()->CreateDeviceAllocator(&allocatorDesc);

		glslang::InitializeProcess();

		return true;
	}

	bool ResourceLoader::Release()
	{
		SAFERELEASE(s_pAllocator);
		SAFERELEASE(s_pCopyCommandAllocator);
		SAFERELEASE(s_pCopyCommandList);
		SAFERELEASE(s_pCopyFence);

		glslang::FinalizeProcess();

		return true;
	}

	bool ResourceLoader::LoadSceneFromFile(const String& filepath, TArray<GameObject>& loadedGameObjects, TArray<Mesh*>& loadedMeshes, TArray<Material*>& loadedMaterials, TArray<Texture*>& loadedTextures)
	{
		size_t lastPathDivisor = filepath.find_last_of("/\\");

		if (lastPathDivisor == String::npos)
		{
			LOG_WARNING("[ResourceLoader]: Failed to load scene '%s'. No parent directory found...", filepath.c_str());
			return false;
		}

		std::string dirpath = filepath.substr(0, lastPathDivisor + 1);

		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn;
		std::string err;

		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, filepath.c_str(), dirpath.c_str(), true, false))
		{
			LOG_WARNING("[ResourceLoader]: Failed to load scene '%s'. Warning: %s Error: %s", filepath.c_str(), warn.c_str(), err.c_str());
			return false;
		}

		loadedMeshes.Resize(shapes.size());
		loadedMaterials.Resize(materials.size());

		std::unordered_map<std::string, Texture*> loadedTexturesMap;

		for (uint32 m = 0; m < materials.size(); m++)
		{
			tinyobj::material_t& material = materials[m];

			Material* pMaterial = DBG_NEW Material();

			if (material.diffuse_texname.length() > 0)
			{
				std::string texturePath = dirpath + material.diffuse_texname;
				ConvertBackslashes(texturePath);

				auto loadedTexture = loadedTexturesMap.find(texturePath);
				if (loadedTexture == loadedTexturesMap.end())
				{
					Texture* pTexture = LoadTextureArrayFromFile(material.diffuse_texname, dirpath, &material.diffuse_texname, 1, EFormat::FORMAT_R8G8B8A8_UNORM, false);
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pAlbedoMap			= pTexture;

					loadedTextures.PushBack(pTexture);
				}
				else
				{
					pMaterial->pAlbedoMap = loadedTexture->second;
				}
			}

			pMaterial->Properties.Albedo = glm::vec4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f);

			if (material.emission[0] > 0.0f || material.emission[1] > 0.0f || material.emission[2] > 0.0f)
			{
				pMaterial->Properties.Albedo.x *= material.emission[0];
				pMaterial->Properties.Albedo.y *= material.emission[1];
				pMaterial->Properties.Albedo.z *= material.emission[2];
				pMaterial->Properties.EmissionStrength = DEFAULT_EMISSIVE_EMISSION_STRENGTH;
			}
			else
			{
				pMaterial->Properties.EmissionStrength = 0.0f;
			}

			if (material.bump_texname.length() > 0)
			{
				std::string texturePath = dirpath + material.bump_texname;
				ConvertBackslashes(texturePath);
				
				auto loadedTexture = loadedTexturesMap.find(texturePath);
				if (loadedTexture == loadedTexturesMap.end())
				{
					Texture* pTexture = LoadTextureArrayFromFile(material.bump_texname, dirpath, &material.bump_texname, 1, EFormat::FORMAT_R8G8B8A8_UNORM, false);
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pNormalMap			= pTexture;

					loadedTextures.PushBack(pTexture);
				}
				else
				{
					pMaterial->pNormalMap = loadedTexture->second;
				}
			}

			if (material.reflection_texname.length() > 0)
			{
				std::string texturePath = dirpath + material.reflection_texname;
				ConvertBackslashes(texturePath);
				
				auto loadedTexture = loadedTexturesMap.find(texturePath);
				if (loadedTexture == loadedTexturesMap.end())
				{
					Texture* pTexture = LoadTextureArrayFromFile(material.reflection_texname, dirpath, &material.reflection_texname, 1, EFormat::FORMAT_R8G8B8A8_UNORM, false);
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pMetallicMap			= pTexture;

					loadedTextures.PushBack(pTexture);
				}
				else
				{
					pMaterial->pMetallicMap = loadedTexture->second;
				}

				pMaterial->Properties.Metallic = 1.0f;
			}
			else
			{
				pMaterial->Properties.Metallic = 0.0f;
			}

			if (material.specular_highlight_texname.length() > 0)
			{
				std::string texturePath = dirpath + material.specular_highlight_texname;
				ConvertBackslashes(texturePath);
				
				auto loadedTexture = loadedTexturesMap.find(texturePath);
				if (loadedTexture == loadedTexturesMap.end())
				{
					Texture* pTexture = LoadTextureArrayFromFile(material.specular_highlight_texname, dirpath, &material.specular_highlight_texname, 1, EFormat::FORMAT_R8G8B8A8_UNORM, false);
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pRoughnessMap		= pTexture;

					loadedTextures.PushBack(pTexture);
				}
				else
				{
					pMaterial->pRoughnessMap = loadedTexture->second;
				}
			}

			//Todo: We should check if a similar material already has been loaded
			loadedMaterials[m] = pMaterial;
		}

		bool hasNormals = false;
		bool hasTexCoords = false;

		for (uint32 s = 0; s < shapes.size(); s++)
		{
			tinyobj::shape_t& shape = shapes[s];

			TArray<Vertex> vertices = {};
			TArray<uint32> indices = {};
			std::unordered_map<Vertex, uint32> uniqueVertices = {};

			for (const tinyobj::index_t& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				//Normals and texcoords are optional, while positions are required
				ASSERT(index.vertex_index >= 0);

				vertex.Position =
				{
					attributes.vertices[3 * (size_t)index.vertex_index + 0],
					attributes.vertices[3 * (size_t)index.vertex_index + 1],
					attributes.vertices[3 * (size_t)index.vertex_index + 2]
				};

				if (index.normal_index >= 0)
				{
					//Assume that if one shape has normals, all have normals
					hasNormals = true; 

					vertex.Normal =
					{
						attributes.normals[3 * (size_t)index.normal_index + 0],
						attributes.normals[3 * (size_t)index.normal_index + 1],
						attributes.normals[3 * (size_t)index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0)
				{
					//Assume that if one shape has tex coords, all have tex coords
					hasTexCoords = true;

					vertex.TexCoord =
					{
						attributes.texcoords[2 * (size_t)index.texcoord_index + 0],
						1.0f - attributes.texcoords[2 * (size_t)index.texcoord_index + 1]
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32>(vertices.GetSize());
					vertices.PushBack(vertex);
				}

				indices.PushBack(uniqueVertices[vertex]);
			}

			//Calculate Tangents if Tex Coords exist
			if (hasNormals && hasTexCoords)
			{
				for (uint32 index = 0; index < indices.GetSize(); index += 3)
				{
					Vertex& v0 = vertices[indices[(size_t)index + 0]];
					Vertex& v1 = vertices[indices[(size_t)index + 1]];
					Vertex& v2 = vertices[indices[(size_t)index + 2]];

					v0.CalculateTangent(v1, v2);
					v1.CalculateTangent(v2, v0);
					v2.CalculateTangent(v0, v1);
				}
			}

			Mesh* pMesh = LoadMeshFromMemory(vertices.GetData(), uint32(vertices.GetSize()), indices.GetData(), uint32(indices.GetSize()));
			loadedMeshes[s] = pMesh;

			D_LOG_MESSAGE("[ResourceLoader]: Loaded Mesh \"%s\" \t for scene : \"%s\"", shape.name.c_str(), filepath.c_str());

			uint32 m = shape.mesh.material_ids[0];

			GameObject gameObject	= {};
			gameObject.Mesh			= s;
			gameObject.Material		= m;

			loadedGameObjects.PushBack(gameObject);
		}

		D_LOG_MESSAGE("[ResourceLoader]: Loaded Scene \"%s\"", filepath.c_str());

		return true;
	}

	Mesh* ResourceLoader::LoadMeshFromFile(const String& filepath)
	{
		//Start New Thread

		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, filepath.c_str(), nullptr, true, false))
		{
			LOG_WARNING("[ResourceLoader]: Failed to load mesh '%s'. Warning: %s Error: %s", filepath.c_str(), warn.c_str(), err.c_str());
			return nullptr;
		}

		std::vector<Vertex> vertices = {};
		std::vector<uint32> indices = {};
		std::unordered_map<Vertex, uint32> uniqueVertices = {};

		for (const tinyobj::shape_t& shape : shapes)
		{
			for (const tinyobj::index_t& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				//Normals and texcoords are optional, while positions are required
				ASSERT(index.vertex_index >= 0);

				vertex.Position =
				{
					attributes.vertices[3 * index.vertex_index + 0],
					attributes.vertices[3 * index.vertex_index + 1],
					attributes.vertices[3 * index.vertex_index + 2]
				};

				if (index.normal_index >= 0)
				{
					vertex.Normal =
					{
						attributes.normals[3 * index.normal_index + 0],
						attributes.normals[3 * index.normal_index + 1],
						attributes.normals[3 * index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0)
				{
					vertex.TexCoord =
					{
						attributes.texcoords[2 * index.texcoord_index + 0],
						1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		//Calculate tangents
		for (uint32 index = 0; index < indices.size(); index += 3)
		{
			Vertex& v0 = vertices[indices[index + 0]];
			Vertex& v1 = vertices[indices[index + 1]];
			Vertex& v2 = vertices[indices[index + 2]];

			v0.CalculateTangent(v1, v2);
			v1.CalculateTangent(v2, v0);
			v2.CalculateTangent(v0, v1);
		}

		Mesh* pMesh = LoadMeshFromMemory(vertices.data(), uint32(vertices.size()), indices.data(), uint32(indices.size()));

		D_LOG_MESSAGE("[ResourceLoader]: Loaded Mesh \"%s\"", filepath.c_str());

		return pMesh;
	}

	Mesh* ResourceLoader::LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		Vertex* pVertexArray = DBG_NEW Vertex[numVertices];
		memcpy(pVertexArray, pVertices, sizeof(Vertex) * numVertices);

		uint32* pIndexArray = DBG_NEW uint32[numIndices];
		memcpy(pIndexArray, pIndices, sizeof(uint32) * numIndices);

		Mesh* pMesh = DBG_NEW Mesh();
		pMesh->pVertexArray		= pVertexArray;
		pMesh->pIndexArray		= pIndexArray;
		pMesh->VertexCount		= numVertices;
		pMesh->IndexCount		= numIndices;

		return pMesh;
	}

	Texture* ResourceLoader::LoadTextureArrayFromFile(const String& name, const String& dir, const String* pFilenames, uint32 count, EFormat format, bool generateMips)
	{
		int texWidth = 0;
		int texHeight = 0;
		int bpp = 0;

		TArray<void*> stbi_pixels(count);

		for (uint32 i = 0; i < count; i++)
		{
			String filepath = dir + pFilenames[i];

			void* pPixels = nullptr;

			if (format == EFormat::FORMAT_R8G8B8A8_UNORM)
			{
				pPixels = (void*)stbi_load(filepath.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
			}
			else if (format == EFormat::FORMAT_R16_UNORM)
			{
				pPixels = (void*)stbi_load_16(filepath.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
			}
			else
			{
				LOG_ERROR("[ResourceLoader]: Texture format not supported for \"%s\"", filepath.c_str());
				return nullptr;
			}

			if (pPixels == nullptr)
			{
				LOG_ERROR("[ResourceLoader]: Failed to load texture file: \"%s\"", filepath.c_str());
				return nullptr;
			}

			stbi_pixels[i] = pPixels;
			D_LOG_MESSAGE("[ResourceLoader]: Loaded Texture \"%s\"", filepath.c_str());
		}

		Texture* pTexture = nullptr;

		if (format == EFormat::FORMAT_R8G8B8A8_UNORM)
		{
			pTexture = LoadTextureArrayFromMemory(name, stbi_pixels.GetData(), stbi_pixels.GetSize(), texWidth, texHeight, format, FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE, generateMips);
		}
		else if (format == EFormat::FORMAT_R16_UNORM)
		{
			TArray<void*> pixels(count * 4);

			for (uint32 i = 0; i < count; i++)
			{
				uint32 numPixels = texWidth * texHeight;
				uint16* pPixelsR = new uint16[numPixels];
				uint16* pPixelsG = new uint16[numPixels];
				uint16* pPixelsB = new uint16[numPixels];
				uint16* pPixelsA = new uint16[numPixels];

				uint16* pSTBIPixels = reinterpret_cast<uint16*>(stbi_pixels[i]);

				for (uint32 p = 0; p < numPixels; p++)
				{
					pPixelsR[p] = pSTBIPixels[4 * p + 0];
					pPixelsG[p] = pSTBIPixels[4 * p + 1];
					pPixelsB[p] = pSTBIPixels[4 * p + 2];
					pPixelsA[p] = pSTBIPixels[4 * p + 3];
				}

				pixels[4 * i + 0] = pPixelsR;
				pixels[4 * i + 1] = pPixelsG;
				pixels[4 * i + 2] = pPixelsB;
				pixels[4 * i + 3] = pPixelsA;
			}

			pTexture = LoadTextureArrayFromMemory(name, pixels.GetData(), pixels.GetSize(), texWidth, texHeight, format, FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE, generateMips);

			for (uint32 i = 0; i < pixels.GetSize(); i++)
			{
				uint16* pPixels = reinterpret_cast<uint16*>(pixels[i]);
				SAFEDELETE_ARRAY(pPixels);
			}
		}

		for (uint32 i = 0; i < count; i++)
		{
			stbi_image_free(stbi_pixels[i]);
		}

		return pTexture;
	}

	Texture* ResourceLoader::LoadTextureArrayFromMemory(const String& name, const void* const * ppData, uint32 arrayCount, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips)
	{
		uint32_t miplevels = 1u;

		if (generateMips)
		{
			miplevels = uint32(glm::floor(glm::log2((float)glm::max(width, height)))) + 1u;
		}

		TextureDesc textureDesc = {};
		textureDesc.DebugName	= name;
		textureDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		textureDesc.Format		= format;
		textureDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
		textureDesc.Flags		= FTextureFlags::TEXTURE_FLAG_COPY_SRC | FTextureFlags::TEXTURE_FLAG_COPY_DST | usageFlags;
		textureDesc.Width		= width;
		textureDesc.Height		= height;
		textureDesc.Depth		= 1;
		textureDesc.ArrayCount	= arrayCount;
		textureDesc.Miplevels	= miplevels;
		textureDesc.SampleCount = 1;

		Texture* pTexture = RenderSystem::GetDevice()->CreateTexture(&textureDesc, s_pAllocator);

		if (pTexture == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to create texture for \"%s\"", name.c_str());
			return nullptr;
		}

		uint32 pixelDataSize = width * height * TextureFormatStride(format);

		BufferDesc bufferDesc	= {};
		bufferDesc.DebugName	= "Texture Copy Buffer";
		bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
		bufferDesc.SizeInBytes	= uint64(arrayCount * pixelDataSize);

		Buffer* pTextureData = RenderSystem::GetDevice()->CreateBuffer(&bufferDesc, s_pAllocator);

		if (pTextureData == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to create copy buffer for \"%s\"", name.c_str());
			return nullptr;
		}

		const uint64 waitValue = s_SignalValue - 1;
		s_pCopyFence->Wait(waitValue, UINT64_MAX);

		s_pCopyCommandAllocator->Reset();
		s_pCopyCommandList->Begin(nullptr);

		PipelineTextureBarrierDesc transitionToCopyDstBarrier = {};
		transitionToCopyDstBarrier.pTexture					= pTexture;
		transitionToCopyDstBarrier.StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
		transitionToCopyDstBarrier.StateAfter				= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToCopyDstBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.SrcMemoryAccessFlags		= 0;
		transitionToCopyDstBarrier.DstMemoryAccessFlags		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToCopyDstBarrier.TextureFlags				= textureDesc.Flags;
		transitionToCopyDstBarrier.Miplevel					= 0;
		transitionToCopyDstBarrier.MiplevelCount			= textureDesc.Miplevels;
		transitionToCopyDstBarrier.ArrayIndex				= 0;
		transitionToCopyDstBarrier.ArrayCount				= textureDesc.ArrayCount;

		s_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, &transitionToCopyDstBarrier, 1);

		for (uint32 i = 0; i < arrayCount; i++)
		{
			uint64 bufferOffset = uint64(i) * pixelDataSize;

			void* pTextureDataDst = pTextureData->Map();
			const void* pTextureDataSrc = ppData[i];
			memcpy((void*)(uint64(pTextureDataDst) + bufferOffset), pTextureDataSrc, pixelDataSize);
			pTextureData->Unmap();

			CopyTextureFromBufferDesc copyDesc = {};
			copyDesc.SrcOffset		= bufferOffset;
			copyDesc.SrcRowPitch	= 0;
			copyDesc.SrcHeight		= 0;
			copyDesc.Width			= width;
			copyDesc.Height			= height;
			copyDesc.Depth			= 1;
			copyDesc.Miplevel		= 0;
			copyDesc.MiplevelCount  = miplevels;
			copyDesc.ArrayIndex		= i;
			copyDesc.ArrayCount		= 1;

			s_pCopyCommandList->CopyTextureFromBuffer(pTextureData, pTexture, copyDesc);
		}

		if (generateMips)
		{
			s_pCopyCommandList->GenerateMiplevels(pTexture, ETextureState::TEXTURE_STATE_COPY_DST, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);
		}
		else
		{
			PipelineTextureBarrierDesc transitionToShaderReadBarrier = {};
			transitionToShaderReadBarrier.pTexture				= pTexture;
			transitionToShaderReadBarrier.StateBefore			= ETextureState::TEXTURE_STATE_COPY_DST;
			transitionToShaderReadBarrier.StateAfter			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			transitionToShaderReadBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
			transitionToShaderReadBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
			transitionToShaderReadBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			transitionToShaderReadBarrier.DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
			transitionToShaderReadBarrier.TextureFlags			= textureDesc.Flags;
			transitionToShaderReadBarrier.Miplevel				= 0;
			transitionToShaderReadBarrier.MiplevelCount			= textureDesc.Miplevels;
			transitionToShaderReadBarrier.ArrayIndex			= 0;
			transitionToShaderReadBarrier.ArrayCount			= textureDesc.ArrayCount;

			s_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM, &transitionToShaderReadBarrier, 1);
		}

		s_pCopyCommandList->End();

		if (!RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&s_pCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, nullptr, 0, s_pCopyFence, s_SignalValue))
		{
			LOG_ERROR("[ResourceLoader]: Texture could not be created as command list could not be executed for \"%s\"", name.c_str());
			SAFERELEASE(pTextureData);

			return nullptr;
		}
		else
		{
			s_SignalValue++;
		}

		//Todo: Remove this wait after garbage collection works
		RenderSystem::GetGraphicsQueue()->Flush();

		SAFERELEASE(pTextureData);

		return pTexture;
	}

	Shader* ResourceLoader::LoadShaderFromFile(const String& filepath, FShaderStageFlags stage, EShaderLang lang, const String& entryPoint)
	{
		byte* pShaderRawSource = nullptr;
		uint32 shaderRawSourceSize = 0;

		TArray<uint32> sourceSPIRV;
		if (lang == EShaderLang::SHADER_LANG_GLSL)
		{
			if (!ReadDataFromFile(filepath, "r", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", filepath.c_str());
				return nullptr;
			}
			
			if (!CompileGLSLToSPIRV(filepath, reinterpret_cast<char*>(pShaderRawSource), shaderRawSourceSize, stage, &sourceSPIRV, nullptr))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV for \"%s\"", filepath.c_str());
				return nullptr;
			}
		}
		else if (lang == EShaderLang::SHADER_LANG_SPIRV)
		{
			if (!ReadDataFromFile(filepath, "rb", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", filepath.c_str());
				return nullptr;
			}
			
			sourceSPIRV.Resize(static_cast<uint32>(glm::ceil(static_cast<float32>(shaderRawSourceSize) / sizeof(uint32))));
			memcpy(sourceSPIRV.GetData(), pShaderRawSource, shaderRawSourceSize);
		}

		const uint32 sourceSize = static_cast<uint32>(sourceSPIRV.GetSize()) * sizeof(uint32);

		ShaderDesc shaderDesc = { };
		shaderDesc.DebugName	= filepath;
		shaderDesc.Source		= TArray<byte>(reinterpret_cast<byte*>(sourceSPIRV.GetData()), reinterpret_cast<byte*>(sourceSPIRV.GetData()) + sourceSize);
		shaderDesc.EntryPoint	= entryPoint;
		shaderDesc.Stage		= stage;
		shaderDesc.Lang			= lang;

		Shader* pShader = RenderSystem::GetDevice()->CreateShader(&shaderDesc);
		Malloc::Free(pShaderRawSource);

		return pShader;
	}

	Shader* ResourceLoader::LoadShaderFromMemory(const String& source, const String& name, FShaderStageFlags stage, EShaderLang lang, const String& entryPoint)
	{
		byte* pShaderRawSource = nullptr;
		uint32 shaderRawSourceSize = 0;

		TArray<uint32> sourceSPIRV;
		if (lang == EShaderLang::SHADER_LANG_GLSL)
		{
			if (!CompileGLSLToSPIRV("", source.c_str(), shaderRawSourceSize, stage, &sourceSPIRV, nullptr))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV");
				return nullptr;
			}
		}
		else if (lang == EShaderLang::SHADER_LANG_SPIRV)
		{
			sourceSPIRV.Resize(static_cast<uint32>(glm::ceil(static_cast<float32>(shaderRawSourceSize) / sizeof(uint32))));
			memcpy(sourceSPIRV.GetData(), pShaderRawSource, shaderRawSourceSize);
		}

		const uint32 sourceSize = static_cast<uint32>(sourceSPIRV.GetSize()) * sizeof(uint32);

		ShaderDesc shaderDesc = { };
		shaderDesc.DebugName	= name;
		shaderDesc.Source		= TArray<byte>(reinterpret_cast<byte*>(sourceSPIRV.GetData()), reinterpret_cast<byte*>(sourceSPIRV.GetData()) + sourceSize);
		shaderDesc.EntryPoint	= entryPoint;
		shaderDesc.Stage		= stage;
		shaderDesc.Lang			= lang;

		Shader* pShader = RenderSystem::GetDevice()->CreateShader(&shaderDesc);
		Malloc::Free(pShaderRawSource);

		return pShader;
	}

	bool ResourceLoader::CreateShaderReflection(const String& filepath, FShaderStageFlags stage, EShaderLang lang, ShaderReflection* pReflection)
	{
		byte* pShaderRawSource = nullptr;
		uint32 shaderRawSourceSize = 0;

		std::vector<uint32> sourceSPIRV;
		uint32 sourceSPIRVSize = 0;

		if (lang == EShaderLang::SHADER_LANG_GLSL)
		{
			if (!ReadDataFromFile(filepath, "r", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", filepath.c_str());
				return false;
			}

			if (!CompileGLSLToSPIRV(filepath, reinterpret_cast<char*>(pShaderRawSource), shaderRawSourceSize, stage, nullptr, pReflection))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV for \"%s\"", filepath.c_str());
				return false;
			}

			sourceSPIRVSize = sourceSPIRV.size() * sizeof(uint32);
		}
		else if (lang == EShaderLang::SHADER_LANG_SPIRV)
		{
			LOG_ERROR("[ResourceLoader]: CreateShaderReflection currently not supported for SPIRV source language");
			return false;
		}

		return true;
	}

	ISoundEffect3D* ResourceLoader::LoadSoundEffectFromFile(const String& filepath)
	{
		SoundEffect3DDesc soundDesc = {};
		soundDesc.Filepath = filepath;

		ISoundEffect3D* pSound = AudioSystem::GetDevice()->CreateSoundEffect(&soundDesc);
		if (pSound == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to initialize sound \"%s\"", filepath.c_str());
			return nullptr;
		}

		D_LOG_MESSAGE("[ResourceLoader]: Loaded Sound \"%s\"", filepath.c_str());

		return pSound;
	}

	bool ResourceLoader::ReadDataFromFile(const String& filepath, const char* pMode, byte** ppData, uint32* pDataSize)
	{
		FILE* pFile = fopen(filepath.c_str(), pMode);
		if (pFile == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to load file \"%s\"", filepath.c_str());
			return false;
		}

		fseek(pFile, 0, SEEK_END);
		int32 length = ftell(pFile) + 1;
		fseek(pFile, 0, SEEK_SET);

		byte* pData = reinterpret_cast<byte*>(Malloc::Allocate(length * sizeof(byte)));
		ZERO_MEMORY(pData, length * sizeof(byte));

		int32 read = fread(pData, 1, length, pFile);
		if (read == 0)
		{
			LOG_ERROR("[ResourceLoader]: Failed to read file \"%s\"", filepath.c_str());
			return false;
		}
		else
		{
			pData[read] = '\0';
		}
		
		(*ppData)		= pData;
		(*pDataSize)	= length;

		fclose(pFile);
		return true;
	}

	void ResourceLoader::ConvertBackslashes(std::string& string)
	{
		size_t pos = string.find_first_of('\\');
		while (pos != std::string::npos)
		{
			string.replace(pos, 1, 1, '/');
			pos = string.find_first_of('\\', pos + 1);
		}
	}

	bool ResourceLoader::CompileGLSLToSPIRV(const String& filepath, const char* pSource, int32 sourceSize, FShaderStageFlags stage, TArray<uint32>* pSourceSPIRV, ShaderReflection* pReflection)
	{
		EShLanguage shaderType = ConvertShaderStageToEShLanguage(stage);
		glslang::TShader shader(shaderType);

		std::string source			= std::string(pSource);
		int32 foundBracket			= source.find_last_of('}') + 1;
		source[foundBracket]		= '\0';
		const char* pFinalSource	= source.c_str();
		shader.setStringsWithLengths(&pFinalSource, &foundBracket, 1);

		//Todo: Fetch this
		int32 clientInputSemanticsVersion							    = 100;
		glslang::EShTargetClientVersion vulkanClientVersion				= glslang::EShTargetVulkan_1_2;
		glslang::EShTargetLanguageVersion targetVersion					= glslang::EShTargetSpv_1_5;

		shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, clientInputSemanticsVersion);
		shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
		shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

		const TBuiltInResource* pResources	= GetDefaultBuiltInResources();
		EShMessages messages				= static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
		const int defaultVersion			= 110;

		DirStackFileIncluder includer;

		//Get Directory Path of File
		size_t found				= filepath.find_last_of("/\\");
		std::string directoryPath	= filepath.substr(0, found);

		includer.pushExternalLocalDirectory(directoryPath);

		//std::string preprocessedGLSL;
		//if (!shader.preprocess(pResources, defaultVersion, ENoProfile, false, false, messages, &preprocessedGLSL, includer))
		//{
		//	LOG_ERROR("[ResourceLoader]: GLSL Preprocessing failed for: \"%s\"\n%s\n%s", filepath.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
		//	return false;
		//}

		//const char* pPreprocessedGLSL = preprocessedGLSL.c_str();
		//shader.setStrings(&pPreprocessedGLSL, 1);

		if (!shader.parse(pResources, defaultVersion, false, messages, includer))
		{
			LOG_ERROR("[ResourceLoader]: GLSL Parsing failed for: \"%s\"\n%s\n%s", filepath.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(messages))
		{
			LOG_ERROR("[ResourceLoader]: GLSL Linking failed for: \"%s\"\n%s\n%s", filepath.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}
		
		glslang::TIntermediate* pIntermediate = program.getIntermediate(shaderType);

		if (pSourceSPIRV != nullptr)
		{
			spv::SpvBuildLogger logger;
			glslang::SpvOptions spvOptions;
			std::vector<uint32> std_sourceSPIRV;
			glslang::GlslangToSpv(*pIntermediate, std_sourceSPIRV, &logger, &spvOptions);
			pSourceSPIRV->Assign(std_sourceSPIRV.data(), std_sourceSPIRV.data() + std_sourceSPIRV.size());
		}

		if (pReflection != nullptr)
		{
			if (!CreateShaderReflection(pIntermediate, stage, pReflection))
			{
				LOG_ERROR("[ResourceLoader]: Failed to Create Shader Reflection");
				return false;
			}
		}

		return true;
	}

	bool ResourceLoader::CreateShaderReflection(glslang::TIntermediate* pIntermediate, FShaderStageFlags stage, ShaderReflection* pReflection)
	{
		EShLanguage shaderType = ConvertShaderStageToEShLanguage(stage);
		glslang::TReflection glslangReflection(EShReflectionOptions::EShReflectionAllIOVariables, shaderType, shaderType);
		glslangReflection.addStage(shaderType, *pIntermediate);

		pReflection->NumAtomicCounters		= glslangReflection.getNumAtomicCounters();
		pReflection->NumBufferVariables		= glslangReflection.getNumBufferVariables();
		pReflection->NumPipeInputs			= glslangReflection.getNumPipeInputs();
		pReflection->NumPipeOutputs			= glslangReflection.getNumPipeOutputs();
		pReflection->NumStorageBuffers		= glslangReflection.getNumStorageBuffers();
		pReflection->NumUniformBlocks		= glslangReflection.getNumUniformBlocks();
		pReflection->NumUniforms			= glslangReflection.getNumUniforms();

		return true;
	}
}
