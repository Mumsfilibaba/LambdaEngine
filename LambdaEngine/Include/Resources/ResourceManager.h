#pragma once
#include "ResourceLoader.h"

#include "Containers/THashTable.h"
#include "Containers/String.h"

namespace LambdaEngine
{
	union ShaderConstant;

	class GraphicsDevice;
	class IAudioDevice;

	//Meshes

	//Meshes
	constexpr GUID_Lambda GUID_MESH_QUAD					= 0;

	//Material
	constexpr GUID_Lambda GUID_MATERIAL_DEFAULT				= GUID_MESH_QUAD + 1;
	constexpr GUID_Lambda GUID_MATERIAL_DEFAULT_EMISSIVE	= GUID_MATERIAL_DEFAULT + 1;

	//Textures
	constexpr GUID_Lambda GUID_TEXTURE_DEFAULT_COLOR_MAP	= GUID_MATERIAL_DEFAULT_EMISSIVE + 1;
	constexpr GUID_Lambda GUID_TEXTURE_DEFAULT_NORMAL_MAP	= GUID_TEXTURE_DEFAULT_COLOR_MAP + 1;

	constexpr GUID_Lambda SMALLEST_UNRESERVED_GUID			= GUID_TEXTURE_DEFAULT_NORMAL_MAP + 1;

	constexpr const char* SCENE_DIR			= "../Assets/Scenes/";
	constexpr const char* MESH_DIR			= "../Assets/Meshes/";
	constexpr const char* TEXTURE_DIR		= "../Assets/Textures/";
	constexpr const char* SHADER_DIR		= "../Assets/Shaders/";
	constexpr const char* SOUND_DIR			= "../Assets/Sounds/";

	class LAMBDA_API ResourceManager
	{
		struct ShaderLoadDesc
		{
			String				Filepath	= "";
			FShaderStageFlags	Stage		= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			EShaderLang			Lang		= EShaderLang::SHADER_LANG_NONE;
			const char*			pEntryPoint	= nullptr;
		};

	public:
		DECL_STATIC_CLASS(ResourceManager);

		static bool Init();
		static bool Release();

		/*
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		*	pGraphicsDevice - A Graphics Device
		*	pDir - Path to the directory that holds the .obj file
		*	filename - The name of the .obj file
		*	result - A vector where all loaded GameObject(s) will be stored
		* return - true if the scene was loaded, false otherwise
		*/
		static bool LoadSceneFromFile(const String& filename, TArray<GameObject>& result);

		/*
		* Load a mesh from file
		*	filename - The name of the .obj file
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMeshFromFile(const String& filename);

		/*
		* Load a mesh from memory
		*	name - A name given to the mesh resource
		*	pVertices - An array of vertices
		*	numVertices - The vertexcount
		*	pIndices - An array of indices
		*	numIndices - The Indexcount
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMeshFromMemory(const String& name, const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		/*
		* Load a material from memory
		*	name - A name given to the material
		*	albedoMap, normalMap, ambientOcclusionMap, metallicMap, roughnessMap - The GUID of a valid Texture loaded with this ResourceManager, or GUID_NONE to use default maps
		*	properties - Material Properties which are to be used for this material
		* return - a valid GUID if the materials was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMaterialFromMemory(const String& name, GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambientOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties);

		/*
		* Load multiple textures from file and combine in a Texture Array
		*	name - A Name of given to the textureArray
		*	pFilenames - Names of the texture files
		*	count - number of elements in pFilenames
		*	format - The format of the pixeldata
		*	generateMips - If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureArrayFromFile(const String& name, const String* pFilenames, uint32 count, EFormat format, bool generateMips);

		/*
		* Load a texture from file
		*	filename - Name of the texture file
		*	format - The format of the pixeldata
		*	generateMips - If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureFromFile(const String& filename, EFormat format, bool generateMips);

		/*
		* Load a texture from memory
		*	name - A Name of given to the texture
		*	pData - The pixeldata
		*	width - The pixel width of the texture
		*	height - The pixel height of the texture
		*	format - The format of the pixeldata
		*	usageFlags - Usage flags
		*	generateMips - If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureFromMemory(const String& name, const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips);

		/*
		* Load sound from file
		*	filename - Name of the shader file
		*	stage - Which stage the shader belongs to
		*	lang - The language of the shader file
		*	pEntryPoint - The name of the shader entrypoint
		* return - a valid GUID if the shader was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadShaderFromFile(const String& filename, FShaderStageFlags stage, EShaderLang lang, const char* pEntryPoint = "main");

		/*
		* Load sound from file
		*	filename - Name of the audio file
		* return - a valid GUID if the sound was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadSoundEffectFromFile(const String& filename);

		static void ReloadAllShaders();

		static GUID_Lambda					GetMeshGUID(const String& name);
		static GUID_Lambda					GetMaterialGUID(const String& name);
		static GUID_Lambda					GetTextureGUID(const String& name);
		static GUID_Lambda					GetShaderGUID(const String& name);
		static GUID_Lambda					GetSoundEffectGUID(const String& name);

		static Mesh*						GetMesh(GUID_Lambda guid);
		static Material*					GetMaterial(GUID_Lambda guid);
		static Texture*						GetTexture(GUID_Lambda guid);
		static TextureView*					GetTextureView(GUID_Lambda guid);
		static Shader*						GetShader(GUID_Lambda guid);
		static ISoundEffect3D*				GetSoundEffect(GUID_Lambda guid);

		FORCEINLINE static std::unordered_map<String, GUID_Lambda>&				GetMeshNamesMap()			{ return s_MaterialNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>&				GetMaterialNamesMap()		{ return s_MaterialNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>&				GetTextureNamesMap()		{ return s_TextureNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>&				GetShaderNamesMap()			{ return s_ShaderNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>&				GetSoundEffectNamesMap()	{ return s_SoundEffectNamesToGUIDs; }

		FORCEINLINE static std::unordered_map<GUID_Lambda, Mesh*>&				GetMeshGUIDMap()			{ return s_Meshes; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Material*>&			GetMaterialGUIDMap()		{ return s_Materials; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Texture*>&			GetTextureGUIDMap()			{ return s_Textures; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, TextureView*>&		GetTextureViewGUIDMap()		{ return s_TextureViews; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Shader*>&			GetShaderGUIDMap()			{ return s_Shaders; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, ISoundEffect3D*>&	GetSoundEffectGUIDMap()		{ return s_SoundEffects; }

	private:
		static GUID_Lambda RegisterLoadedMesh(const String& name, Mesh* pMesh);
		static GUID_Lambda RegisterLoadedMaterial(const String& name, Material* pMaterial);
		static GUID_Lambda RegisterLoadedTexture(Texture* pTexture);

		static GUID_Lambda GetGUID(const std::unordered_map<String, GUID_Lambda>& namesToGUIDs, const String& name);

		static void InitDefaultResources();

	private:
		static GUID_Lambda										s_NextFreeGUID;

		static std::unordered_map<String, GUID_Lambda>			s_MeshNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_MaterialNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_TextureNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_ShaderNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_SoundEffectNamesToGUIDs;

		static std::unordered_map<GUID_Lambda, Mesh*>			s_Meshes;
		static std::unordered_map<GUID_Lambda, Material*>		s_Materials;
		static std::unordered_map<GUID_Lambda, Texture*>		s_Textures;
		static std::unordered_map<GUID_Lambda, TextureView*>	s_TextureViews;
		static std::unordered_map<GUID_Lambda, Shader*>			s_Shaders;
		static std::unordered_map<GUID_Lambda, ISoundEffect3D*>	s_SoundEffects;

		static std::unordered_map<GUID_Lambda, ShaderLoadDesc>	s_ShaderLoadConfigurations;
	};
}
