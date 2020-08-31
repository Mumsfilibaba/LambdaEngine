#include "Sandbox.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/ImGuiRenderer.h"
#include "Rendering/Renderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraphEditor.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/Core/API/ITextureView.h"
#include "Rendering/Core/API/ISampler.h"
#include "Rendering/Core/API/ICommandQueue.h"

#include "Audio/AudioSystem.h"
#include "Audio/API/ISoundEffect3D.h"
#include "Audio/API/ISoundInstance3D.h"
#include "Audio/API/IAudioGeometry.h"
#include "Audio/API/IReverbSphere.h"
#include "Audio/API/IMusic.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Game/Scene.h"

#include "Time/API/Clock.h"

#include "Threading/API/Thread.h"

#include <imgui.h>

constexpr const uint32 BACK_BUFFER_COUNT = 3;
#ifdef LAMBDA_PLATFORM_MACOS
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 8;
#else
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 256;
#endif
constexpr const bool RAY_TRACING_ENABLED		= true;
constexpr const bool SVGF_ENABLED				= true;
constexpr const bool POST_PROCESSING_ENABLED	= false;

constexpr const bool RENDER_GRAPH_IMGUI_ENABLED	= true;
constexpr const bool RENDERING_DEBUG_ENABLED	= false;

constexpr const float DEFAULT_DIR_LIGHT_R			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_G			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_B			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_STRENGTH	= 0.0f;

constexpr const uint32 NUM_BLUE_NOISE_LUTS = 128;

enum class EScene
{
	SPONZA,
	CORNELL,
	TESTING
};

Sandbox::Sandbox()
    : Game()
{
	using namespace LambdaEngine;

	CommonApplication::Get()->AddEventHandler(this);

	const uint32 size = 128;
	byte* pMem = DBG_NEW byte[size];
	
	for (uint32 i = 0; i < size; i++)
	{
		pMem[i] = 'a';
	}
	
	SAFEDELETE_ARRAY(pMem);
	
	m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice());

	SceneDesc sceneDesc = {};
	sceneDesc.Name				= "Test Scene";
	sceneDesc.RayTracingEnabled = RAY_TRACING_ENABLED;
	m_pScene->Init(sceneDesc);

	m_DirectionalLightAngle	= glm::half_pi<float>();
	m_DirectionalLightStrength[0] = DEFAULT_DIR_LIGHT_R;
	m_DirectionalLightStrength[1] = DEFAULT_DIR_LIGHT_G;
	m_DirectionalLightStrength[2] = DEFAULT_DIR_LIGHT_B;
	m_DirectionalLightStrength[3] = DEFAULT_DIR_LIGHT_STRENGTH;

	DirectionalLight directionalLight;
	directionalLight.Direction			= glm::vec4(glm::normalize(glm::vec3(glm::cos(m_DirectionalLightAngle), glm::sin(m_DirectionalLightAngle), 0.0f)), 0.0f);
	directionalLight.EmittedRadiance	= glm::vec4(glm::vec3(m_DirectionalLightStrength[0], m_DirectionalLightStrength[1], m_DirectionalLightStrength[2]) * m_DirectionalLightStrength[3], 0.0f);

	EScene scene = EScene::TESTING;

	m_pScene->SetDirectionalLight(directionalLight);

	AreaLightObject areaLight;
	areaLight.Type = EAreaLightType::QUAD;
	areaLight.Material = GUID_MATERIAL_DEFAULT_EMISSIVE;

	if (scene == EScene::SPONZA)
	{
		//Lights
		{
			glm::vec3 position(0.0f, 6.0f, 0.0f);
			glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			glm::vec3 scale(1.5f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			InstanceIndexAndTransform instanceIndexAndTransform;
			instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			instanceIndexAndTransform.Position		= position;
			instanceIndexAndTransform.Rotation		= rotation;
			instanceIndexAndTransform.Scale			= scale;

			m_LightInstanceIndicesAndTransforms.push_back(instanceIndexAndTransform);
		}

		//Scene
		{
			std::vector<GameObject>	sceneGameObjects;
			ResourceManager::LoadSceneFromFile("sponza/sponza.obj", sceneGameObjects);

			glm::vec3 position(0.0f, 0.0f, 0.0f);
			glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec3 scale(0.01f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			for (GameObject& gameObject : sceneGameObjects)
			{
				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = m_pScene->AddDynamicGameObject(gameObject, transform);
				instanceIndexAndTransform.Position = position;
				instanceIndexAndTransform.Rotation = rotation;
				instanceIndexAndTransform.Scale = scale;

				m_InstanceIndicesAndTransforms.push_back(instanceIndexAndTransform);
			}
		}
	}
	else if (scene == EScene::CORNELL)
	{
		//Lights
		{
			glm::vec3 position(0.0f, 1.95f, 0.0f);
			glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			glm::vec3 scale(0.2f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			InstanceIndexAndTransform instanceIndexAndTransform;
			instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			instanceIndexAndTransform.Position		= position;
			instanceIndexAndTransform.Rotation		= rotation;
			instanceIndexAndTransform.Scale			= scale;

			m_LightInstanceIndicesAndTransforms.push_back(instanceIndexAndTransform);
		}

		//Scene
		{
			std::vector<GameObject>	sceneGameObjects;
			ResourceManager::LoadSceneFromFile("CornellBox/CornellBox-Original-No-Light.obj", sceneGameObjects);

			glm::vec3 position(0.0f, 0.0f, 0.0f);
			glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec3 scale(1.0f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			for (GameObject& gameObject : sceneGameObjects)
			{
				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = m_pScene->AddDynamicGameObject(gameObject, transform);
				instanceIndexAndTransform.Position = position;
				instanceIndexAndTransform.Rotation = rotation;
				instanceIndexAndTransform.Scale = scale;

				m_InstanceIndicesAndTransforms.push_back(instanceIndexAndTransform);
			}
		}
	}
	else if (scene == EScene::TESTING)
	{
		//Lights
		{
			glm::vec3 position(0.0f, 6.0f, 0.0f);
			glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			glm::vec3 scale(1.5f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			InstanceIndexAndTransform instanceIndexAndTransform;
			instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			instanceIndexAndTransform.Position		= position;
			instanceIndexAndTransform.Rotation		= rotation;
			instanceIndexAndTransform.Scale			= scale;

			m_LightInstanceIndicesAndTransforms.push_back(instanceIndexAndTransform);
		}

		//Scene
		{
			std::vector<GameObject>	sceneGameObjects;
			ResourceManager::LoadSceneFromFile("Testing/Testing.obj", sceneGameObjects);

			glm::vec3 position(0.0f, 0.0f, 0.0f);
			glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec3 scale(1.0f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			for (GameObject& gameObject : sceneGameObjects)
			{
				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = m_pScene->AddDynamicGameObject(gameObject, transform);
				instanceIndexAndTransform.Position = position;
				instanceIndexAndTransform.Rotation = rotation;
				instanceIndexAndTransform.Scale = scale;

				m_InstanceIndicesAndTransforms.push_back(instanceIndexAndTransform);
			}
		}

		//Sphere Grid
		{
			uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
		
			uint32 gridRadius = 5;

			for (uint32 y = 0; y < gridRadius; y++)
			{
				float32 roughness = y / float32(gridRadius - 1);

				for (uint32 x = 0; x < gridRadius; x++)
				{
					float32 metallic = x / float32(gridRadius - 1);

					MaterialProperties materialProperties;
					materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
					materialProperties.Roughness	= roughness;
					materialProperties.Metallic		= metallic;

					GameObject sphereGameObject = {};
					sphereGameObject.Mesh		= sphereMeshGUID;
					sphereGameObject.Material	= ResourceManager::LoadMaterialFromMemory(
						"Default r: " + std::to_string(roughness) + " m: " + std::to_string(metallic),
						GUID_TEXTURE_DEFAULT_COLOR_MAP,
						GUID_TEXTURE_DEFAULT_NORMAL_MAP,
						GUID_TEXTURE_DEFAULT_COLOR_MAP,
						GUID_TEXTURE_DEFAULT_COLOR_MAP,
						GUID_TEXTURE_DEFAULT_COLOR_MAP,
						materialProperties);

					glm::vec3 position(-float32(gridRadius) * 0.5f + x, 1.0f + y, 5.0f);
					glm::vec3 scale(1.0f);

					glm::mat4 transform(1.0f);
					transform = glm::translate(transform, position);
					transform = glm::scale(transform, scale);

					InstanceIndexAndTransform instanceIndexAndTransform;
					instanceIndexAndTransform.InstanceIndex = m_pScene->AddDynamicGameObject(sphereGameObject, transform);
					instanceIndexAndTransform.Position		= position;
					instanceIndexAndTransform.Rotation		= glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
					instanceIndexAndTransform.Scale			= scale;

					m_InstanceIndicesAndTransforms.push_back(instanceIndexAndTransform);
				}
			}
		}
	}

	m_pScene->Finalize();

	m_pCamera = DBG_NEW Camera();

	Window* pWindow = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= 90.0f;
	cameraDesc.Width		= pWindow->GetWidth();
	cameraDesc.Height		= pWindow->GetHeight();
	cameraDesc.NearPlane	= 0.001f;
	cameraDesc.FarPlane		= 1000.0f;

	m_pCamera->Init(cameraDesc);
	m_pCamera->Update();

	SamplerDesc samplerLinearDesc = {};
	samplerLinearDesc.Name					= "Linear Sampler";
	samplerLinearDesc.MinFilter				= EFilter::LINEAR;
	samplerLinearDesc.MagFilter				= EFilter::LINEAR;
	samplerLinearDesc.MipmapMode			= EMipmapMode::LINEAR;
	samplerLinearDesc.AddressModeU			= EAddressMode::REPEAT;
	samplerLinearDesc.AddressModeV			= EAddressMode::REPEAT;
	samplerLinearDesc.AddressModeW			= EAddressMode::REPEAT;
	samplerLinearDesc.MipLODBias			= 0.0f;
	samplerLinearDesc.AnisotropyEnabled		= false;
	samplerLinearDesc.MaxAnisotropy			= 16;
	samplerLinearDesc.MinLOD				= 0.0f;
	samplerLinearDesc.MaxLOD				= 1.0f;

	m_pLinearSampler = RenderSystem::GetDevice()->CreateSampler(&samplerLinearDesc);

	SamplerDesc samplerNearestDesc = {};
	samplerNearestDesc.Name					= "Nearest Sampler";
	samplerNearestDesc.MinFilter			= EFilter::NEAREST;
	samplerNearestDesc.MagFilter			= EFilter::NEAREST;
	samplerNearestDesc.MipmapMode			= EMipmapMode::NEAREST;
	samplerNearestDesc.AddressModeU			= EAddressMode::REPEAT;
	samplerNearestDesc.AddressModeV			= EAddressMode::REPEAT;
	samplerNearestDesc.AddressModeW			= EAddressMode::REPEAT;
	samplerNearestDesc.MipLODBias			= 0.0f;
	samplerNearestDesc.AnisotropyEnabled	= false;
	samplerNearestDesc.MaxAnisotropy		= 16;
	samplerNearestDesc.MinLOD				= 0.0f;
	samplerNearestDesc.MaxLOD				= 1.0f;

	m_pNearestSampler = RenderSystem::GetDevice()->CreateSampler(&samplerNearestDesc);

	m_pRenderGraphEditor = DBG_NEW RenderGraphEditor();

	//InitRendererForEmpty();

	//InitRendererForRayTracingOnly();

	InitRendererForDeferred();

	//InitRendererForVisBuf(BACK_BUFFER_COUNT, MAX_TEXTURES_PER_DESCRIPTOR_SET);

	ICommandList* pGraphicsCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();
	ICommandList* pComputeCopyCommandList = m_pRenderer->AcquireComputeCopyCommandList();

	m_pScene->UpdateCamera(m_pCamera);

	if (RENDER_GRAPH_IMGUI_ENABLED)
	{
		m_pRenderGraphEditor->InitGUI();	//Must Be called after Renderer is initialized
	}

	//InitTestAudio();
}

Sandbox::~Sandbox()
{
    LambdaEngine::CommonApplication::Get()->RemoveEventHandler(this);
    
	SAFEDELETE(m_pAudioGeometry);

	SAFEDELETE(m_pScene);
	SAFEDELETE(m_pCamera);
	SAFERELEASE(m_pLinearSampler);
	SAFERELEASE(m_pNearestSampler);

	SAFEDELETE(m_pRenderGraph);
	SAFEDELETE(m_pRenderer);

	SAFEDELETE(m_pRenderGraphEditor);
}

void Sandbox::InitTestAudio()
{
	using namespace LambdaEngine;

	//m_AudioListenerIndex = AudioSystem::GetDevice()->CreateAudioListener();

	//m_ToneSoundEffectGUID = ResourceManager::LoadSoundEffectFromFile("../Assets/Sounds/noise.wav");
	//m_GunSoundEffectGUID = ResourceManager::LoadSoundEffectFromFile("../Assets/Sounds/GUN_FIRE-GoodSoundForYou.wav");

	//m_pToneSoundEffect = ResourceManager::GetSoundEffect(m_ToneSoundEffectGUID);
	//m_pGunSoundEffect = ResourceManager::GetSoundEffect(m_GunSoundEffectGUID);

	//SoundInstance3DDesc soundInstanceDesc = {};
	//soundInstanceDesc.pSoundEffect = m_pGunSoundEffect;
	//soundInstanceDesc.Flags = FSoundModeFlags::SOUND_MODE_LOOPING;

	//m_pToneSoundInstance = AudioSystem::GetDevice()->CreateSoundInstance(&soundInstanceDesc);
	//m_pToneSoundInstance->SetVolume(0.5f);

	MusicDesc musicDesc = {};
	musicDesc.pFilepath		= "../Assets/Sounds/halo_theme.ogg";
	musicDesc.Volume		= 0.5f;
	musicDesc.Pitch			= 1.0f;

	AudioSystem::GetDevice()->CreateMusic(&musicDesc);

	/*m_SpawnPlayAts = false;
	m_GunshotTimer = 0.0f;
	m_GunshotDelay = 1.0f;
	m_Timer = 0.0f;


	m_pAudioListener = AudioSystem::GetDevice()->CreateAudioListener();
	m_pAudioListener->Update(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	m_pReverbSphere = AudioSystem::GetDevice()->CreateReverbSphere();

	ReverbSphereDesc reverbSphereDesc = {};
	reverbSphereDesc.Position = glm::vec3(0.0f, 0.0f, 5.0f);
	reverbSphereDesc.MinDistance = 20.0f;
	reverbSphereDesc.MaxDistance = 40.0f;
	reverbSphereDesc.ReverbSetting = EReverbSetting::SEWERPIPE;

	m_pReverbSphere->Init(reverbSphereDesc);

	m_pAudioGeometry = AudioSystem::GetDevice()->CreateAudioGeometry();

	GUID_Lambda sphereGUID = ResourceManager::LoadMeshFromFile("../Assets/Meshes/sphere.obj");
	Mesh* sphereMesh = ResourceManager::GetMesh(sphereGUID);
	glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
	AudioMeshParameters audioMeshParameters = {};
	audioMeshParameters.DirectOcclusion = 0.0f;
	audioMeshParameters.ReverbOcclusion = 0.25f;
	audioMeshParameters.DoubleSided = true;

	AudioGeometryDesc audioGeometryDesc = {};
	audioGeometryDesc.NumMeshes = 1;
	audioGeometryDesc.ppMeshes = &sphereMesh;
	audioGeometryDesc.pTransforms = &transform;
	audioGeometryDesc.pAudioMeshParameters = &audioMeshParameters;

	m_pAudioGeometry->Init(audioGeometryDesc);*/

	/*std::vector<GraphicsObject> sponzaGraphicsObjects;
	ResourceManager::LoadSceneFromFile("../Assets/Scenes/sponza/", "sponza.obj", sponzaGraphicsObjects);

	std::vector<Mesh*> sponzaMeshes;
	std::vector<glm::mat4> sponzaMeshTransforms;
	std::vector<LambdaEngine::AudioMeshParameters> sponzaAudioMeshParameters;

	for (GraphicsObject& graphicsObject : sponzaGraphicsObjects)
	{
		sponzaMeshes.push_back(ResourceManager::GetMesh(graphicsObject.Mesh));
		sponzaMeshTransforms.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)));

		LambdaEngine::AudioMeshParameters audioMeshParameters = {};
		audioMeshParameters.DirectOcclusion = 1.0f;
		audioMeshParameters.ReverbOcclusion = 1.0f;
		audioMeshParameters.DoubleSided = true;
		sponzaAudioMeshParameters.push_back(audioMeshParameters);
	}

	AudioGeometryDesc audioGeometryDesc = {};
	audioGeometryDesc.pName = "Test";
	audioGeometryDesc.NumMeshes = sponzaMeshes.size();
	audioGeometryDesc.ppMeshes = sponzaMeshes.data();
	audioGeometryDesc.pTransforms = sponzaMeshTransforms.data();
	audioGeometryDesc.pAudioMeshParameters = sponzaAudioMeshParameters.data();

	m_pAudioGeometry->Init(audioGeometryDesc);*/
}

void Sandbox::OnFocusChanged(LambdaEngine::Window* pWindow, bool hasFocus)
{
	UNREFERENCED_VARIABLE(pWindow);
	
    //LOG_MESSAGE("Window Moved: hasFocus=%s", hasFocus ? "true" : "false");
}

void Sandbox::OnWindowMoved(LambdaEngine::Window* pWindow, int16 x, int16 y)
{
	UNREFERENCED_VARIABLE(pWindow);
	
    //LOG_MESSAGE("Window Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnWindowResized(LambdaEngine::Window* pWindow, uint16 width, uint16 height, LambdaEngine::EResizeType type)
{
	UNREFERENCED_VARIABLE(pWindow);
	
    //LOG_MESSAGE("Window Resized: width=%u, height=%u, type=%u", width, height, uint32(type));
}

void Sandbox::OnWindowClosed(LambdaEngine::Window* pWindow)
{
	UNREFERENCED_VARIABLE(pWindow);
	
   // LOG_MESSAGE("Window closed");
}

void Sandbox::OnMouseEntered(LambdaEngine::Window* pWindow)
{
	UNREFERENCED_VARIABLE(pWindow);
	
    //LOG_MESSAGE("Mouse Entered");
}

void Sandbox::OnMouseLeft(LambdaEngine::Window* pWindow)
{
	UNREFERENCED_VARIABLE(pWindow);
	
    //LOG_MESSAGE("Mouse Left");
}

void Sandbox::OnKeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)
{
	UNREFERENCED_VARIABLE(modifierMask);
	
    using namespace LambdaEngine;
    
	//LOG_MESSAGE("Key Pressed: %s, isRepeat=%s", KeyToString(key), isRepeat ? "true" : "false");

    if (isRepeat)
    {
        return;
    }
    
	Window* pMainWindow = CommonApplication::Get()->GetMainWindow();
    if (key == EKey::KEY_ESCAPE)
    {
		pMainWindow->Close();
    }
    if (key == EKey::KEY_1)
    {
		pMainWindow->Minimize();
    }
    if (key == EKey::KEY_2)
    {
		pMainWindow->Maximize();
    }
    if (key == EKey::KEY_3)
    {
		pMainWindow->Restore();
    }
    if (key == EKey::KEY_4)
    {
		pMainWindow->ToggleFullscreen();
    }
	if (key == EKey::KEY_5)
	{
		if (CommonApplication::Get()->GetInputMode(pMainWindow) == EInputMode::INPUT_MODE_STANDARD)
		{
			CommonApplication::Get()->SetInputMode(pMainWindow, EInputMode::INPUT_MODE_RAW);
		}
		else
		{
			CommonApplication::Get()->SetInputMode(pMainWindow, EInputMode::INPUT_MODE_STANDARD);
		}
	}
	if (key == EKey::KEY_6)
	{
		pMainWindow->SetPosition(0, 0);
	}
    
	static bool geometryAudioActive = true;
	static bool reverbSphereActive = true;

	if (key == EKey::KEY_KEYPAD_5)
	{
		RenderSystem::GetGraphicsQueue()->Flush();
		RenderSystem::GetComputeQueue()->Flush();
		ResourceManager::ReloadAllShaders();
		PipelineStateManager::ReloadPipelineStates();
	}
	/*if (key == EKey::KEY_KEYPAD_1)
	{
		m_pToneSoundInstance->Toggle();
	}
	else if (key == EKey::KEY_KEYPAD_2)
	{
		m_SpawnPlayAts = !m_SpawnPlayAts;
	}
	else if (key == EKey::KEY_KEYPAD_3)
	{
		m_pGunSoundEffect->PlayOnceAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), 1.0f);
	}
	else if (key == EKey::KEY_KEYPAD_ADD)
	{
		m_GunshotDelay += 0.05f;
	}
	else if (key == EKey::KEY_KEYPAD_SUBTRACT)
	{
		m_GunshotDelay = glm::max(m_GunshotDelay - 0.05f, 0.125f);
	}
	else if (key == EKey::KEY_KEYPAD_5)
	{
		AudioSystem::GetDevice()->ToggleMusic();
	}
	else if (key == EKey::KEY_KEYPAD_7)
	{
		if (m_pAudioGeometry != nullptr)
		{
			geometryAudioActive = !geometryAudioActive;
			LOG_MESSAGE("AudioGeometry %s", geometryAudioActive ? "Enabled" : "Disabled");
			m_pAudioGeometry->SetActive(geometryAudioActive);
		}
	}
	else if (key == EKey::KEY_KEYPAD_8)
	{
		if (m_pReverbSphere != nullptr)
		{
			reverbSphereActive = !reverbSphereActive;
			LOG_MESSAGE("ReverbSphere %s", reverbSphereActive ? "Enabled" : "Disabled");
			m_pReverbSphere->SetActive(reverbSphereActive);
		}
	}*/
}

void Sandbox::OnKeyReleased(LambdaEngine::EKey key)
{
    using namespace LambdaEngine;
    
	UNREFERENCED_VARIABLE(key);
	
	//LOG_MESSAGE("Key Released: %s", KeyToString(key));
}

void Sandbox::OnKeyTyped(uint32 character)
{
    using namespace LambdaEngine;
    
    UNREFERENCED_VARIABLE(character);
    
	//LOG_MESSAGE("Key Text: %c", char(character));
}

void Sandbox::OnMouseMoved(int32 x, int32 y)
{
	UNREFERENCED_VARIABLE(x);
	UNREFERENCED_VARIABLE(y);
    
	//LOG_MESSAGE("Mouse Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnMouseMovedRaw(int32 deltaX, int32 deltaY)
{
	UNREFERENCED_VARIABLE(deltaX);
	UNREFERENCED_VARIABLE(deltaY);
    
	//LOG_MESSAGE("Mouse Delta: x=%d, y=%d", deltaX, deltaY);
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button, uint32 modifierMask)
{
	UNREFERENCED_VARIABLE(button);
    UNREFERENCED_VARIABLE(modifierMask);
    
	//LOG_MESSAGE("Mouse Button Pressed: %d", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	UNREFERENCED_VARIABLE(button);
	//LOG_MESSAGE("Mouse Button Released: %d", button);
}

void Sandbox::OnMouseScrolled(int32 deltaX, int32 deltaY)
{
	UNREFERENCED_VARIABLE(deltaX);
    UNREFERENCED_VARIABLE(deltaY);
    
	//LOG_MESSAGE("Mouse Scrolled: x=%d, y=%d", deltaX, deltaY);
}


void Sandbox::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	Render(delta);
}

void Sandbox::FixedTick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	float dt = (float)delta.AsSeconds();
	m_Timer += dt;

	if (m_pGunSoundEffect != nullptr)
	{
		if (m_SpawnPlayAts)
		{
			m_GunshotTimer += dt;

			if (m_GunshotTimer > m_GunshotDelay)
			{

				glm::vec3 gunPosition(glm::cos(m_Timer), 0.0f, glm::sin(m_Timer));
				m_pGunSoundEffect->PlayOnceAt(gunPosition, glm::vec3(0.0f), 0.5f);
				m_GunshotTimer = 0.0f;
			}
		}
	}

	if (m_pToneSoundInstance != nullptr)
	{
		glm::vec3 tonePosition(glm::cos(m_Timer), 0.0f, glm::sin(m_Timer));
		m_pToneSoundInstance->SetPosition(tonePosition);
	}

	constexpr float CAMERA_MOVEMENT_SPEED = 1.4f;
	constexpr float CAMERA_ROTATION_SPEED = 45.0f;

	if (Input::IsKeyDown(EKey::KEY_W) && Input::IsKeyUp(EKey::KEY_S))
	{
		m_pCamera->Translate(glm::vec3(0.0f, 0.0f, CAMERA_MOVEMENT_SPEED * delta.AsSeconds()));
	}
	else if (Input::IsKeyDown(EKey::KEY_S) && Input::IsKeyUp(EKey::KEY_W))
	{
		m_pCamera->Translate(glm::vec3(0.0f, 0.0f, -CAMERA_MOVEMENT_SPEED * delta.AsSeconds()));
	}

	if (Input::IsKeyDown(EKey::KEY_A) && Input::IsKeyUp(EKey::KEY_D))
	{
		m_pCamera->Translate(glm::vec3(-CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_D) && Input::IsKeyUp(EKey::KEY_A))
	{
		m_pCamera->Translate(glm::vec3(CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}

	if (Input::IsKeyDown(EKey::KEY_Q) && Input::IsKeyUp(EKey::KEY_E))
	{
		m_pCamera->Translate(glm::vec3(0.0f, CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_E) && Input::IsKeyUp(EKey::KEY_Q))
	{
		m_pCamera->Translate(glm::vec3(0.0f, -CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f));
	}

	if (Input::IsKeyDown(EKey::KEY_UP) && Input::IsKeyUp(EKey::KEY_DOWN))
	{
		m_pCamera->Rotate(glm::vec3(-CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_DOWN) && Input::IsKeyUp(EKey::KEY_UP))
	{
		m_pCamera->Rotate(glm::vec3(CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}

	if (Input::IsKeyDown(EKey::KEY_LEFT) && Input::IsKeyUp(EKey::KEY_RIGHT))
	{
		m_pCamera->Rotate(glm::vec3(0.0f, -CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_RIGHT) && Input::IsKeyUp(EKey::KEY_LEFT))
	{
		m_pCamera->Rotate(glm::vec3(0.0f, CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f));
	}

	m_pCamera->Update();
	m_pScene->UpdateCamera(m_pCamera);

	AudioListenerDesc listenerDesc = {};
	listenerDesc.Position = m_pCamera->GetPosition();
	listenerDesc.Forward = m_pCamera->GetForwardVec();
	listenerDesc.Up = m_pCamera->GetUpVec();

	AudioSystem::GetDevice()->UpdateAudioListener(m_AudioListenerIndex, &listenerDesc);
}

void Sandbox::Render(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	m_pRenderer->NewFrame(delta);

	ICommandList* pGraphicsCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();
	ICommandList* pComputeCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();

	Window* pWindow = CommonApplication::Get()->GetMainWindow();
	float32 renderWidth = (float32)pWindow->GetWidth();
	float32 renderHeight = (float32)pWindow->GetHeight();
	float32 renderAspectRatio = renderWidth / renderHeight;

	if (RENDER_GRAPH_IMGUI_ENABLED)
	{
		m_pRenderGraphEditor->RenderGUI();

		ImGui::ShowDemoWindow();

		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Debugging Window", NULL))
		{
			uint32 modFrameIndex = m_pRenderer->GetModFrameIndex();

			ITextureView* const* ppTextureViews = nullptr;
			uint32			textureViewCount = 0;

			static ImGuiTexture albedoTexture = {};
			static ImGuiTexture normalTexture = {};
			static ImGuiTexture emissiveMetallicRoughness = {};
			static ImGuiTexture compactNormalsTexture = {};
			static ImGuiTexture motionTexture = {};
			static ImGuiTexture linearZTexture = {};
			static ImGuiTexture depthStencilTexture = {};
			static ImGuiTexture momentsTexture = {};
			static ImGuiTexture radianceTexture = {};
			static ImGuiTexture compNormalTexture = {};

			float windowWidth = ImGui::GetWindowWidth();

			ImGui::Text("FPS: %f", 1.0f / delta.AsSeconds());
			ImGui::Text("Frametime (ms): %f", delta.AsMilliSeconds());

			if (ImGui::BeginTabBar("Debugging Tabs"))
			{
				if (ImGui::BeginTabItem("Scene"))
				{
					bool dirLightChanged = false;

					if (ImGui::SliderFloat("Dir. Light Angle", &m_DirectionalLightAngle, 0.0f, glm::two_pi<float>()))
					{
						dirLightChanged = true;
					}

					if (ImGui::ColorEdit3("Dir. Light Color", m_DirectionalLightStrength))
					{
						dirLightChanged = true;
					}

					if (ImGui::SliderFloat("Dir. Light Strength", &m_DirectionalLightStrength[3], 0.0f, 10000.0f, "%.3f", 10.0f))
					{
						dirLightChanged = true;
					}

					if (ImGui::Button("Reset Dir. Light"))
					{
						m_DirectionalLightStrength[0] = DEFAULT_DIR_LIGHT_R;
						m_DirectionalLightStrength[1] = DEFAULT_DIR_LIGHT_G;
						m_DirectionalLightStrength[2] = DEFAULT_DIR_LIGHT_B;
						m_DirectionalLightStrength[3] = DEFAULT_DIR_LIGHT_STRENGTH;

						dirLightChanged = true;
					}

					if (dirLightChanged)
					{
						DirectionalLight directionalLight;
						directionalLight.Direction			= glm::vec4(glm::normalize(glm::vec3(glm::cos(m_DirectionalLightAngle), glm::sin(m_DirectionalLightAngle), 0.0f)), 0.0f);
						directionalLight.EmittedRadiance	= glm::vec4(glm::vec3(m_DirectionalLightStrength[0], m_DirectionalLightStrength[1], m_DirectionalLightStrength[2]) * m_DirectionalLightStrength[3], 0.0f);

						m_pScene->SetDirectionalLight(directionalLight);
					}

					if (ImGui::CollapsingHeader("Game Objects"))
					{
						for (InstanceIndexAndTransform& instanceIndexAndTransform : m_InstanceIndicesAndTransforms)
						{
							if (ImGui::TreeNode(("Game Object" + std::to_string(instanceIndexAndTransform.InstanceIndex)).c_str()))
							{
								bool updated = false;

								if (ImGui::SliderFloat3("Position", glm::value_ptr(instanceIndexAndTransform.Position), -50.0f, 50.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat4("Rotation", glm::value_ptr(instanceIndexAndTransform.Rotation), 0.0f, glm::two_pi<float32>()))
								{
									updated = true;
								}

								if (ImGui::SliderFloat3("Scale", glm::value_ptr(instanceIndexAndTransform.Scale), 0.0f, 10.0f))
								{
									updated = true;
								}

								float lockedScale = instanceIndexAndTransform.Scale.x;
								if (ImGui::SliderFloat("Locked Scale", &lockedScale, 0.0f, 10.0f))
								{
									instanceIndexAndTransform.Scale.x = lockedScale;
									instanceIndexAndTransform.Scale.y = lockedScale;
									instanceIndexAndTransform.Scale.z = lockedScale;
									updated = true;
								}

								if (updated)
								{
									glm::mat4 transform(1.0f);
									transform = glm::translate(transform, instanceIndexAndTransform.Position);
									transform = glm::rotate(transform, instanceIndexAndTransform.Rotation.w, glm::vec3(instanceIndexAndTransform.Rotation));
									transform = glm::scale(transform, instanceIndexAndTransform.Scale);

									m_pScene->UpdateTransform(instanceIndexAndTransform.InstanceIndex, transform);
								}

								ImGui::TreePop();
							}
						}
					}

					if (ImGui::CollapsingHeader("Lights"))
					{
						for (InstanceIndexAndTransform& instanceIndexAndTransform : m_LightInstanceIndicesAndTransforms)
						{
							if (ImGui::TreeNode(("Light" + std::to_string(instanceIndexAndTransform.InstanceIndex)).c_str()))
							{
								bool updated = false;

								if (ImGui::SliderFloat3("Position", glm::value_ptr(instanceIndexAndTransform.Position), -50.0f, 50.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat4("Rotation", glm::value_ptr(instanceIndexAndTransform.Rotation), 0.0f, glm::two_pi<float32>()))
								{
									updated = true;
								}

								if (ImGui::SliderFloat3("Scale", glm::value_ptr(instanceIndexAndTransform.Scale), 0.0f, 10.0f))
								{
									updated = true;
								}

								float lockedScale = instanceIndexAndTransform.Scale.x;
								if (ImGui::SliderFloat("Locked Scale",&lockedScale, 0.0f, 10.0f))
								{
									instanceIndexAndTransform.Scale.x = lockedScale;
									instanceIndexAndTransform.Scale.y = lockedScale;
									instanceIndexAndTransform.Scale.z = lockedScale;
									updated = true;
								}

								if (updated)
								{
									glm::mat4 transform(1.0f);
									transform = glm::translate(transform, instanceIndexAndTransform.Position);
									transform = glm::rotate(transform, instanceIndexAndTransform.Rotation.w, glm::vec3(instanceIndexAndTransform.Rotation));
									transform = glm::scale(transform, instanceIndexAndTransform.Scale);

									m_pScene->UpdateTransform(instanceIndexAndTransform.InstanceIndex, transform);
								}

								ImGui::TreePop();
							}
						}
					}

					if (ImGui::CollapsingHeader("Materials"))
					{
						std::unordered_map<String, GUID_Lambda>& materialNamesMap = ResourceManager::GetMaterialNamesMap();;

						for (auto materialIt = materialNamesMap.begin(); materialIt != materialNamesMap.end(); materialIt++)
						{
							if (ImGui::TreeNode(materialIt->first.c_str()))
							{
								Material* pMaterial = ResourceManager::GetMaterial(materialIt->second);

								bool updated = false;

								if (ImGui::ColorEdit3("Albedo", glm::value_ptr(pMaterial->Properties.Albedo)))
								{
									updated = true;
								}

								if (ImGui::SliderFloat("AO", &pMaterial->Properties.Ambient, 0.0f, 1.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat("Metallic", &pMaterial->Properties.Metallic, 0.0f, 1.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat("Roughness", &pMaterial->Properties.Roughness, 0.0f, 1.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat("Emission Strength", &pMaterial->Properties.EmissionStrength, 0.0f, 1000.0f))
								{
									updated = true;
								}

								if (updated)
								{
									m_pScene->UpdateMaterialProperties(materialIt->second);
								}

								ImGui::TreePop();
							}
						}
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Albedo AO"))
				{
					albedoTexture.ResourceName = "G_BUFFER_ALBEDO_AO";
					ImGui::Image(&albedoTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Emission Metallic Roughness"))
				{
					emissiveMetallicRoughness.ResourceName = "G_BUFFER_EMISSION_METALLIC_ROUGHNESS";

					const char* items[] = { "Emission", "Metallic", "Roughness" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						emissiveMetallicRoughness.ReservedIncludeMask = 0x00008420;

						emissiveMetallicRoughness.ChannelMul[0] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[1] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[2] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[3] = 0.0f;

						emissiveMetallicRoughness.ChannelAdd[0] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[1] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[2] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[3] = 1.0f;

						emissiveMetallicRoughness.PixelShaderGUID = m_ImGuiPixelShaderEmissionGUID;
					}
					else if (currentItem == 1)
					{
						emissiveMetallicRoughness.ReservedIncludeMask = 0x00001110;

						emissiveMetallicRoughness.ChannelMul[0] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[1] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[2] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[3] = 0.0f;

						emissiveMetallicRoughness.ChannelAdd[0] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[1] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[2] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[3] = 1.0f;

						emissiveMetallicRoughness.PixelShaderGUID = m_ImGuiPixelPackedMetallicGUID;
					}
					else if (currentItem == 2)
					{
						emissiveMetallicRoughness.ReservedIncludeMask = 0x00001110;

						emissiveMetallicRoughness.ChannelMul[0] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[1] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[2] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[3] = 0.0f;

						emissiveMetallicRoughness.ChannelAdd[0] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[1] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[2] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[3] = 1.0f;

						emissiveMetallicRoughness.PixelShaderGUID = m_ImGuiPixelPackedRoughnessGUID;
					}

					ImGui::Image(&emissiveMetallicRoughness, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Compact Normals"))
				{
					compactNormalsTexture.ResourceName = "G_BUFFER_COMPACT_NORMALS";

					const char* items[] = { "Shading", "Geometric" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						compactNormalsTexture.ReservedIncludeMask = 0x00008880;

						compactNormalsTexture.ChannelMul[0] = 1.0f;
						compactNormalsTexture.ChannelMul[1] = 1.0f;
						compactNormalsTexture.ChannelMul[2] = 1.0f;
						compactNormalsTexture.ChannelMul[3] = 0.0f;

						compactNormalsTexture.ChannelAdd[0] = 0.0f;
						compactNormalsTexture.ChannelAdd[1] = 0.0f;
						compactNormalsTexture.ChannelAdd[2] = 0.0f;
						compactNormalsTexture.ChannelAdd[3] = 1.0f;

						compactNormalsTexture.PixelShaderGUID = m_ImGuiPixelCompactNormalFloatGUID;
					}
					else if (currentItem == 1)
					{
						compactNormalsTexture.ReservedIncludeMask = 0x00004440;

						compactNormalsTexture.ChannelMul[0] = 1.0f;
						compactNormalsTexture.ChannelMul[1] = 1.0f;
						compactNormalsTexture.ChannelMul[2] = 1.0f;
						compactNormalsTexture.ChannelMul[3] = 0.0f;

						compactNormalsTexture.ChannelAdd[0] = 0.0f;
						compactNormalsTexture.ChannelAdd[1] = 0.0f;
						compactNormalsTexture.ChannelAdd[2] = 0.0f;
						compactNormalsTexture.ChannelAdd[3] = 1.0f;

						compactNormalsTexture.PixelShaderGUID = m_ImGuiPixelCompactNormalFloatGUID;
					}

					ImGui::Image(&compactNormalsTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Motion"))
				{
					motionTexture.ResourceName = "G_BUFFER_MOTION";

					motionTexture.ReservedIncludeMask = 0x00008400;

					motionTexture.ChannelMul[0] = 10.0f;
					motionTexture.ChannelMul[1] = 10.0f;
					motionTexture.ChannelMul[2] = 0.0f;
					motionTexture.ChannelMul[3] = 0.0f;

					motionTexture.ChannelAdd[0] = 0.0f;
					motionTexture.ChannelAdd[1] = 0.0f;
					motionTexture.ChannelAdd[2] = 0.0f;
					motionTexture.ChannelAdd[3] = 1.0f;

					ImGui::Image(&motionTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Linear Z"))
				{
					linearZTexture.ResourceName = "G_BUFFER_LINEAR_Z";

					const char* items[] = { "ALL", "Linear Z", "Max Change Z", "Prev Linear Z", "Packed Local Normal" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						linearZTexture.ReservedIncludeMask = 0x00008421;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 1.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 0.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelLinearZGUID;
					}
					else if (currentItem == 1)
					{
						linearZTexture.ReservedIncludeMask = 0x00008880;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 0.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 1.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelLinearZGUID;
					}
					else if (currentItem == 2)
					{
						linearZTexture.ReservedIncludeMask = 0x00004440;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 0.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 1.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelLinearZGUID;
					}
					else if (currentItem == 3)
					{
						linearZTexture.ReservedIncludeMask = 0x00002220;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 0.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 1.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelLinearZGUID;
					}
					else if (currentItem == 4)
					{
						linearZTexture.ReservedIncludeMask = 0x00001110;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 0.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 1.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelShaderPackedLocalNormalGUID;
					}

					ImGui::Image(&linearZTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Compact Normal"))
				{
					compNormalTexture.ResourceName = "G_BUFFER_COMPACT_NORM_DEPTH";

					const char* items[] = { "Packed World Normal" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						compNormalTexture.ReservedIncludeMask = 0x00008880;

						compNormalTexture.ChannelMul[0] = 1.0f;
						compNormalTexture.ChannelMul[1] = 1.0f;
						compNormalTexture.ChannelMul[2] = 1.0f;
						compNormalTexture.ChannelMul[3] = 0.0f;

						compNormalTexture.ChannelAdd[0] = 0.0f;
						compNormalTexture.ChannelAdd[1] = 0.0f;
						compNormalTexture.ChannelAdd[2] = 0.0f;
						compNormalTexture.ChannelAdd[3] = 1.0f;

						compNormalTexture.PixelShaderGUID = m_ImGuiPixelShaderPackedLocalNormalGUID;
					}

					ImGui::Image(&compNormalTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Depth Stencil"))
				{
					depthStencilTexture.ResourceName = "G_BUFFER_DEPTH_STENCIL";

					depthStencilTexture.ReservedIncludeMask = 0x00008880;

					depthStencilTexture.ChannelMul[0] = 1.0f;
					depthStencilTexture.ChannelMul[1] = 1.0f;
					depthStencilTexture.ChannelMul[2] = 1.0f;
					depthStencilTexture.ChannelMul[3] = 0.0f;

					depthStencilTexture.ChannelAdd[0] = 0.0f;
					depthStencilTexture.ChannelAdd[1] = 0.0f;
					depthStencilTexture.ChannelAdd[2] = 0.0f;
					depthStencilTexture.ChannelAdd[3] = 1.0f;

					depthStencilTexture.PixelShaderGUID = m_ImGuiPixelShaderDepthGUID;

					ImGui::Image(&depthStencilTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Ray Tracing / SVGF"))
				{
					const char* items[] = { 
						"Direct Albedo",
						"Direct Radiance", 
						"Direct Radiance Reproj.", 
						"Direct Radiance Variance Est.",
						"Direct Radiance Feedback",
						"Indirect Albedo ",
						"Indirect Radiance ", 
						"Indirect Radiance Reproj.", 
						"Indirect Radiance Variance Est.",
						"Indirect Radiance Feedback",
						"Direct Variance", 
						"Indirect Variance", 
						"Accumulation" 
					};
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						radianceTexture.ResourceName = "DIRECT_ALBEDO";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					if (currentItem == 1)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 2)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE_REPROJECTED";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 3)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE_VARIANCE_ESTIMATED";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 4)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE_FEEDBACK";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 5)
					{
						radianceTexture.ResourceName		= "INDIRECT_ALBEDO";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 6)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 7)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE_REPROJECTED";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 8)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE_VARIANCE_ESTIMATED";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 9)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE_FEEDBACK";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 10)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE_REPROJECTED";
						radianceTexture.ReservedIncludeMask = 0x00001110;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 11)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE_REPROJECTED";
						radianceTexture.ReservedIncludeMask = 0x00001110;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 12)
					{
						radianceTexture.ResourceName		= "SVGF_HISTORY";
						radianceTexture.ReservedIncludeMask = 0x00008880;

						const float32 maxHistoryLength = 32.0f;

						radianceTexture.ChannelMul[0] = 1.0f / maxHistoryLength;
						radianceTexture.ChannelMul[1] = 1.0f / maxHistoryLength;
						radianceTexture.ChannelMul[2] = 1.0f / maxHistoryLength;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}

					ImGui::Image(&radianceTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Moments"))
				{
					momentsTexture.ResourceName = "MOMENTS";

					momentsTexture.ReservedIncludeMask = 0x00008421;

					momentsTexture.ChannelMul[0] = 1.0f;
					momentsTexture.ChannelMul[1] = 1.0f;
					momentsTexture.ChannelMul[2] = 1.0f;
					momentsTexture.ChannelMul[3] = 0.0f;

					momentsTexture.ChannelAdd[0] = 0.0f;
					momentsTexture.ChannelAdd[1] = 0.0f;
					momentsTexture.ChannelAdd[2] = 0.0f;
					momentsTexture.ChannelAdd[3] = 1.0f;

					momentsTexture.PixelShaderGUID = GUID_NONE;

					ImGui::Image(&momentsTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	m_pScene->PrepareRender(pGraphicsCopyCommandList, pComputeCopyCommandList, m_pRenderer->GetFrameIndex(), delta);
	m_pRenderer->PrepareRender(delta);

	m_pRenderer->Render();
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
        Sandbox* pSandbox = DBG_NEW Sandbox();        
        return pSandbox;
    }
}

bool Sandbox::InitRendererForEmpty()
{
	//using namespace LambdaEngine;

	//GUID_Lambda fullscreenQuadShaderGUID	= ResourceManager::LoadShaderFromFile("../Assets/Shaders/FullscreenQuad.glsl",	FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,	EShaderLang::GLSL);
	//GUID_Lambda shadingPixelShaderGUID		= ResourceManager::LoadShaderFromFile("../Assets/Shaders/StaticPixel.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	EShaderLang::GLSL);

	//std::vector<RenderStageDesc> renderStages;

	//const char*							pShadingRenderStageName = "Shading Render Stage";
	//GraphicsManagedPipelineStateDesc	shadingPipelineStateDesc = {};
	//std::vector<RenderStageAttachment>	shadingRenderStageAttachments;

	//{
	//	shadingRenderStageAttachments.push_back({ RENDER_GRAPH_BACK_BUFFER_ATTACHMENT, EAttachmentType::OUTPUT_COLOR, FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, BACK_BUFFER_COUNT, false, EFormat::FORMAT_B8G8R8A8_UNORM });

	//	RenderStagePushConstants pushConstants = {};
	//	pushConstants.pName			= "Shading Pass Push Constants";
	//	pushConstants.DataSize		= sizeof(int32) * 2;

	//	RenderStageDesc renderStage = {};
	//	renderStage.pName						= pShadingRenderStageName;
	//	renderStage.pAttachments				= shadingRenderStageAttachments.data();
	//	renderStage.AttachmentCount				= (uint32)shadingRenderStageAttachments.size();

	//	shadingPipelineStateDesc.pName				= "Shading Pass Pipeline State";
	//	shadingPipelineStateDesc.VertexShader		= fullscreenQuadShaderGUID;
	//	shadingPipelineStateDesc.PixelShader		= shadingPixelShaderGUID;

	//	renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

	//	renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::FULLSCREEN_QUAD;
	//	renderStage.GraphicsPipeline.pIndexBufferName		= nullptr;
	//	renderStage.GraphicsPipeline.pMeshIndexBufferName	= nullptr;
	//	renderStage.GraphicsPipeline.pGraphicsDesc			= &shadingPipelineStateDesc;

	//	renderStages.push_back(renderStage);
	//}

	//RenderGraphDesc renderGraphDesc = {};
	//renderGraphDesc.pName						= "Render Graph";
	//renderGraphDesc.CreateDebugGraph			= RENDER_GRAPH_DEBUG_ENABLED;
	//renderGraphDesc.CreateDebugStages			= RENDERING_DEBUG_ENABLED;
	//renderGraphDesc.pRenderStages				= renderStages.data();
	//renderGraphDesc.RenderStageCount			= (uint32)renderStages.size();
	//renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;
	//renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;
	//renderGraphDesc.pScene						= nullptr;

	//m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());

	//m_pRenderGraph->Init(&renderGraphDesc);

	//Window* pWindow = CommonApplication::Get()->GetMainWindow();
	//uint32 renderWidth	= pWindow->GetWidth();
	//uint32 renderHeight = pWindow->GetHeight();

	//{
	//	RenderStageParameters shadingRenderStageParameters = {};
	//	shadingRenderStageParameters.pRenderStageName	= pShadingRenderStageName;
	//	shadingRenderStageParameters.Graphics.Width		= renderWidth;
	//	shadingRenderStageParameters.Graphics.Height	= renderHeight;

	//	m_pRenderGraph->UpdateRenderStageParameters(shadingRenderStageParameters);
	//}

	//m_pRenderer = DBG_NEW Renderer(RenderSystem::GetDevice());

	//RendererDesc rendererDesc = {};
	//rendererDesc.pName				= "Renderer";
	//rendererDesc.Debug				= RENDERING_DEBUG_ENABLED;
	//rendererDesc.pRenderGraph		= m_pRenderGraph;
	//rendererDesc.pWindow			= CommonApplication::Get()->GetMainWindow();
	//rendererDesc.BackBufferCount	= BACK_BUFFER_COUNT;
	//
	//m_pRenderer->Init(&rendererDesc);

	//if (RENDERING_DEBUG_ENABLED)
	//{
	//	ImGui::SetCurrentContext(ImGuiRenderer::GetImguiContext());
	//}

	return true;
}

bool Sandbox::InitRendererForDeferred()
{
	using namespace LambdaEngine;

	//GUID_Lambda geometryVertexShaderGUID		= ResourceManager::LoadShaderFromFile("GeometryDefVertex.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda geometryPixelShaderGUID			= ResourceManager::LoadShaderFromFile("GeometryDefPixel.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	//GUID_Lambda fullscreenQuadShaderGUID		= ResourceManager::LoadShaderFromFile("FullscreenQuad.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda shadingPixelShaderGUID			= ResourceManager::LoadShaderFromFile("ShadingDefPixel.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	//GUID_Lambda raygenShaderGUID				= ResourceManager::LoadShaderFromFile("Raygen.glsl",				FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda closestHitRadianceShaderGUID	= ResourceManager::LoadShaderFromFile("ClosestHitRadiance.glsl",	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::GLSL);
	//GUID_Lambda missRadianceShaderGUID			= ResourceManager::LoadShaderFromFile("MissRadiance.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda closestHitShadowShaderGUID		= ResourceManager::LoadShaderFromFile("ClosestHitShadow.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::GLSL);
	//GUID_Lambda missShadowShaderGUID			= ResourceManager::LoadShaderFromFile("MissShadow.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::GLSL);

	//GUID_Lambda postProcessShaderGUID			= ResourceManager::LoadShaderFromFile("PostProcess.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,		EShaderLang::GLSL);

	m_ImGuiPixelShaderNormalGUID				= ResourceManager::LoadShaderFromFile("ImGuiPixelNormal.frag",				FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelShaderDepthGUID					= ResourceManager::LoadShaderFromFile("ImGuiPixelDepth.frag",				FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelShaderMetallicGUID				= ResourceManager::LoadShaderFromFile("ImGuiPixelMetallic.frag",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelShaderRoughnessGUID				= ResourceManager::LoadShaderFromFile("ImGuiPixelRoughness.frag",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelShaderEmissiveGUID				= ResourceManager::LoadShaderFromFile("ImGuiPixelEmissive.frag",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelShaderPackedLocalNormalGUID		= ResourceManager::LoadShaderFromFile("ImGuiPixelPackedLocalNormal.frag",	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelLinearZGUID						= ResourceManager::LoadShaderFromFile("ImGuiPixelLinearZ.frag",				FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelCompactNormalFloatGUID			= ResourceManager::LoadShaderFromFile("ImGuiPixelCompactNormalFloat.frag",	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelShaderEmissionGUID				= ResourceManager::LoadShaderFromFile("ImGuiPixelEmission.frag",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelPackedMetallicGUID				= ResourceManager::LoadShaderFromFile("ImGuiPixelPackedMetallic.frag",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelPackedRoughnessGUID				= ResourceManager::LoadShaderFromFile("ImGuiPixelPackedRoughness.frag",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	//ResourceManager::LoadShaderFromFile("ForwardVertex.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::GLSL);
	//ResourceManager::LoadShaderFromFile("ForwardPixel.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	EShaderLang::GLSL);

	//ResourceManager::LoadShaderFromFile("ShadingSimpleDefPixel.glsl",	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	EShaderLang::GLSL);

	String renderGraphFile = "";

	if (!RAY_TRACING_ENABLED && !POST_PROCESSING_ENABLED)
	{
		renderGraphFile = "../Assets/RenderGraphs/DEFERRED.lrg";
	}
	else if (RAY_TRACING_ENABLED && !SVGF_ENABLED && !POST_PROCESSING_ENABLED)
	{
		renderGraphFile = "../Assets/RenderGraphs/TRT_DEFERRED_SIMPLE.lrg";
	}
	else if (RAY_TRACING_ENABLED && SVGF_ENABLED && !POST_PROCESSING_ENABLED)
	{
		renderGraphFile = "../Assets/RenderGraphs/TRT_DEFERRED_SVGF.lrg";
	}
	else if (RAY_TRACING_ENABLED && POST_PROCESSING_ENABLED)
	{
		renderGraphFile = "../Assets/RenderGraphs/TRT_PP_DEFERRED.lrg";
	}

	RenderGraphStructureDesc renderGraphStructure = m_pRenderGraphEditor->CreateRenderGraphStructure(renderGraphFile, RENDER_GRAPH_IMGUI_ENABLED);

	RenderGraphDesc renderGraphDesc = {};
	renderGraphDesc.pRenderGraphStructureDesc	= &renderGraphStructure;
	renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;
	renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;
	renderGraphDesc.pScene						= m_pScene;

	LambdaEngine::Clock clock;
	clock.Reset();
	clock.Tick();

	m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());
	m_pRenderGraph->Init(&renderGraphDesc);

	clock.Tick();
	LOG_INFO("Render Graph Build Time: %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

	SamplerDesc nearestSamplerDesc		= m_pNearestSampler->GetDesc();
	SamplerDesc linearSamplerDesc		= m_pLinearSampler->GetDesc();

	SamplerDesc* pNearestSamplerDesc	= &nearestSamplerDesc;
	SamplerDesc* pLinearSamplerDesc		= &linearSamplerDesc;

	Window* pWindow	= CommonApplication::Get()->GetMainWindow();
	uint32 renderWidth	= pWindow->GetWidth();
	uint32 renderHeight = pWindow->GetHeight();

	{
		IBuffer* pBuffer = m_pScene->GetLightsBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_LIGHTS_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetPerFrameBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= PER_FRAME_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetMaterialProperties();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_MAT_PARAM_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetVertexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_VERTEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetIndexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_INDEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetPrimaryInstanceBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_PRIMARY_INSTANCE_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetSecondaryInstanceBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_SECONDARY_INSTANCE_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetIndirectArgsBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_INDIRECT_ARGS_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		ITexture** ppAlbedoMaps						= m_pScene->GetAlbedoMaps();
		ITexture** ppNormalMaps						= m_pScene->GetNormalMaps();
		ITexture** ppAmbientOcclusionMaps			= m_pScene->GetAmbientOcclusionMaps();
		ITexture** ppMetallicMaps					= m_pScene->GetMetallicMaps();
		ITexture** ppRoughnessMaps					= m_pScene->GetRoughnessMaps();

		ITextureView** ppAlbedoMapViews				= m_pScene->GetAlbedoMapViews();
		ITextureView** ppNormalMapViews				= m_pScene->GetNormalMapViews();
		ITextureView** ppAmbientOcclusionMapViews	= m_pScene->GetAmbientOcclusionMapViews();
		ITextureView** ppMetallicMapViews			= m_pScene->GetMetallicMapViews();
		ITextureView** ppRoughnessMapViews			= m_pScene->GetRoughnessMapViews();

		std::vector<ISampler*> linearSamplers(MAX_UNIQUE_MATERIALS, m_pLinearSampler);
		std::vector<ISampler*> nearestSamplers(MAX_UNIQUE_MATERIALS, m_pNearestSampler);

		ResourceUpdateDesc albedoMapsUpdateDesc = {};
		albedoMapsUpdateDesc.ResourceName								= SCENE_ALBEDO_MAPS;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppAlbedoMaps;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppAlbedoMapViews;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

		ResourceUpdateDesc normalMapsUpdateDesc = {};
		normalMapsUpdateDesc.ResourceName								= SCENE_NORMAL_MAPS;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppNormalMaps;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppNormalMapViews;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

		ResourceUpdateDesc aoMapsUpdateDesc = {};
		aoMapsUpdateDesc.ResourceName									= SCENE_AO_MAPS;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= ppAmbientOcclusionMaps;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= ppAmbientOcclusionMapViews;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= nearestSamplers.data();

		ResourceUpdateDesc metallicMapsUpdateDesc = {};
		metallicMapsUpdateDesc.ResourceName								= SCENE_METALLIC_MAPS;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppMetallicMaps;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppMetallicMapViews;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

		ResourceUpdateDesc roughnessMapsUpdateDesc = {};
		roughnessMapsUpdateDesc.ResourceName							= SCENE_ROUGHNESS_MAPS;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= ppRoughnessMaps;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= ppRoughnessMapViews;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= nearestSamplers.data();

		m_pRenderGraph->UpdateResource(albedoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(normalMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(aoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(metallicMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(roughnessMapsUpdateDesc);
	}

	if (RAY_TRACING_ENABLED)
	{
		const IAccelerationStructure* pTLAS = m_pScene->GetTLAS();
		ResourceUpdateDesc resourceUpdateDesc					= {};
		resourceUpdateDesc.ResourceName							= SCENE_TLAS;
		resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= pTLAS;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		String blueNoiseLUTFileNames[NUM_BLUE_NOISE_LUTS];

		for (uint32 i = 0; i < NUM_BLUE_NOISE_LUTS; i++)
		{
			char str[5];
			snprintf(str, 5, "%04d", i);
			blueNoiseLUTFileNames[i] = "LUTs/BlueNoise/256_256/HDR_RGBA_" + std::string(str) + ".png";
		}

		GUID_Lambda blueNoiseID = ResourceManager::LoadTextureArrayFromFile("Blue Noise Texture", blueNoiseLUTFileNames, NUM_BLUE_NOISE_LUTS, EFormat::FORMAT_R16_UNORM, false);

		ITexture* pBlueNoiseTexture				= ResourceManager::GetTexture(blueNoiseID);
		ITextureView* pBlueNoiseTextureView		= ResourceManager::GetTextureView(blueNoiseID);

		ResourceUpdateDesc blueNoiseUpdateDesc = {};
		blueNoiseUpdateDesc.ResourceName								= "BLUE_NOISE_LUT";
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures			= &pBlueNoiseTexture;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews		= &pBlueNoiseTextureView;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers			= &m_pNearestSampler;

		m_pRenderGraph->UpdateResource(blueNoiseUpdateDesc);
	}

	{
		TextureDesc textureDesc	= {};
		textureDesc.Name					= "G-Buffer Albedo-AO Texture";
		textureDesc.Type					= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType				= EMemoryType::MEMORY_GPU;
		textureDesc.Format					= EFormat::FORMAT_R32G32B32A32_SFLOAT;
		textureDesc.Flags					= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width					= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height					= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth					= 1;
		textureDesc.SampleCount				= 1;
		textureDesc.Miplevels				= 1;
		textureDesc.ArrayCount				= 1;

		TextureViewDesc textureViewDesc		= { };
		textureViewDesc.Name				= "G-Buffer Albedo-AO Texture View";
		textureViewDesc.Flags				= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type				= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel			= 0;
		textureViewDesc.MiplevelCount		= 1;
		textureViewDesc.ArrayIndex			= 0;
		textureViewDesc.ArrayCount			= 1;
		textureViewDesc.Format				= textureDesc.Format;

		{
			TextureDesc* pTextureDesc			= &textureDesc;
			TextureViewDesc* pTextureViewDesc	= &textureViewDesc;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "G_BUFFER_ALBEDO_AO";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			textureDesc.Name		= "Prev " + textureDesc.Name;
			textureViewDesc.Name	= "Prev " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &textureDesc;
			TextureViewDesc* pTextureViewDesc	= &textureViewDesc;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "PREV_G_BUFFER_ALBEDO_AO";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	{
		TextureDesc textureDesc	= {};
		textureDesc.Name					= "G-Buffer Motion Texture";
		textureDesc.Type					= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType				= EMemoryType::MEMORY_GPU;
		textureDesc.Format					= EFormat::FORMAT_R32G32B32A32_SFLOAT;
		textureDesc.Flags					= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width					= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height					= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth					= 1;
		textureDesc.SampleCount				= 1;
		textureDesc.Miplevels				= 1;
		textureDesc.ArrayCount				= 1;

		TextureViewDesc textureViewDesc		= { };
		textureViewDesc.Name				= "G-Buffer Motion Texture View";
		textureViewDesc.Flags				= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type				= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel			= 0;
		textureViewDesc.MiplevelCount		= 1;
		textureViewDesc.ArrayIndex			= 0;
		textureViewDesc.ArrayCount			= 1;
		textureViewDesc.Format				= textureDesc.Format;

		{
			TextureDesc* pTextureDesc			= &textureDesc;
			TextureViewDesc* pTextureViewDesc	= &textureViewDesc;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "G_BUFFER_MOTION";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	{
		TextureDesc textureDesc	= {};
		textureDesc.Name					= "G-Buffer Compact Normals Texture";
		textureDesc.Type					= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType				= EMemoryType::MEMORY_GPU;
		textureDesc.Format					= EFormat::FORMAT_R32G32_SFLOAT;
		textureDesc.Flags					= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width					= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height					= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth					= 1;
		textureDesc.SampleCount				= 1;
		textureDesc.Miplevels				= 1;
		textureDesc.ArrayCount				= 1;

		TextureViewDesc textureViewDesc		= { };
		textureViewDesc.Name				= "G-Buffer Compact Normals Texture View";
		textureViewDesc.Flags				= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type				= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel			= 0;
		textureViewDesc.MiplevelCount		= 1;
		textureViewDesc.ArrayIndex			= 0;
		textureViewDesc.ArrayCount			= 1;
		textureViewDesc.Format				= textureDesc.Format;

		{
			TextureDesc* pTextureDesc			= &textureDesc;
			TextureViewDesc* pTextureViewDesc	= &textureViewDesc;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "G_BUFFER_COMPACT_NORMALS";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	{
		TextureDesc textureDesc	= {};
		textureDesc.Name					= "G-Buffer Emissive Metallic Roughness Texture";
		textureDesc.Type					= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType				= EMemoryType::MEMORY_GPU;
		textureDesc.Format					= EFormat::FORMAT_R32G32B32A32_SFLOAT;
		textureDesc.Flags					= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width					= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height					= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth					= 1;
		textureDesc.SampleCount				= 1;
		textureDesc.Miplevels				= 1;
		textureDesc.ArrayCount				= 1;

		TextureViewDesc textureViewDesc		= { };
		textureViewDesc.Name				= "G-Buffer Emissive Metallic Roughness Texture View";
		textureViewDesc.Flags				= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type				= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel			= 0;
		textureViewDesc.MiplevelCount		= 1;
		textureViewDesc.ArrayIndex			= 0;
		textureViewDesc.ArrayCount			= 1;
		textureViewDesc.Format				= textureDesc.Format;

		{
			TextureDesc* pTextureDesc			= &textureDesc;
			TextureViewDesc* pTextureViewDesc	= &textureViewDesc;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "G_BUFFER_EMISSION_METALLIC_ROUGHNESS";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	{
		TextureDesc textureDesc;
		textureDesc.Name				= "G-Buffer Linear Z Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R32G32B32A32_UINT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= renderWidth;
		textureDesc.Height				= renderHeight;
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc;
		textureViewDesc.Name			= "G-Buffer Linear Z Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS | FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "G_BUFFER_LINEAR_Z";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Prev " + textureDesc.Name;
			tvd.Name	= "Prev " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "PREV_G_BUFFER_LINEAR_Z";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

		{
		TextureDesc textureDesc;
		textureDesc.Name				= "G-Buffer Compact Norm Depth Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R32G32B32A32_UINT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= renderWidth;
		textureDesc.Height				= renderHeight;
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc;
		textureViewDesc.Name			= "G-Buffer Compact Norm Depth Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS | FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "G_BUFFER_COMPACT_NORM_DEPTH";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}


	{
		TextureDesc textureDesc = {};
		textureDesc.Name				= "G-Buffer Depth Stencil Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_D24_UNORM_S8_UINT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_DEPTH_STENCIL | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height				= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc = { };
		textureViewDesc.Name			= "G-Buffer Depth Stencil Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_DEPTH_STENCIL | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		{
			TextureDesc* pTextureDesc			= &textureDesc;
			TextureViewDesc* pTextureViewDesc	= &textureViewDesc;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "G_BUFFER_DEPTH_STENCIL";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			textureDesc.Name		= "Prev " + textureDesc.Name;
			textureViewDesc.Name	= "Prev " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &textureDesc;
			TextureViewDesc* pTextureViewDesc	= &textureViewDesc;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "PREV_G_BUFFER_DEPTH_STENCIL";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	{
		TextureDesc textureDesc;
		textureDesc.Name				= "Radiance Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R32G32B32A32_SFLOAT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= renderWidth;
		textureDesc.Height				= renderHeight;
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc;
		textureViewDesc.Name			= "Radiance Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Direct " + textureDesc.Name;
			tvd.Name	= "Direct " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "DIRECT_RADIANCE";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Reproj. Direct " + textureDesc.Name;
			tvd.Name	= "Reproj. Direct " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "DIRECT_RADIANCE_REPROJECTED";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Variance Est. Direct " + textureDesc.Name;
			tvd.Name	= "Variance Est. Direct " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "DIRECT_RADIANCE_VARIANCE_ESTIMATED";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Variance Est. Direct " + textureDesc.Name;
			tvd.Name	= "Variance Est. Direct " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "DIRECT_RADIANCE_VARIANCE_ESTIMATED";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Direct " + textureDesc.Name + " Atrous Ping";
			tvd.Name	= "Direct " + textureDesc.Name + " Atrous Ping";

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "DIRECT_RADIANCE_ATROUS_PING";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Direct " + textureDesc.Name + " Atrous Pong";
			tvd.Name	= "Direct " + textureDesc.Name + " Atrous Pong";

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "DIRECT_RADIANCE_ATROUS_PONG";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Direct " + textureDesc.Name + " Feedback";
			tvd.Name	= "Direct " + textureDesc.Name + " Feedback";

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "DIRECT_RADIANCE_FEEDBACK";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Indirect " + textureDesc.Name;
			tvd.Name	= "Indirect " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "INDIRECT_RADIANCE";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Reproj. Indirect " + textureDesc.Name;
			tvd.Name	= "Reproj. Indirect " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "INDIRECT_RADIANCE_REPROJECTED";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Variance Est. Indirect " + textureDesc.Name;
			tvd.Name	= "Variance Est. Indirect " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "INDIRECT_RADIANCE_VARIANCE_ESTIMATED";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Indirect " + textureDesc.Name + " Atrous Ping";
			tvd.Name	= "Indirect " + textureDesc.Name + " Atrous Ping";

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "INDIRECT_RADIANCE_ATROUS_PING";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Indirect " + textureDesc.Name + " Atrous Pong";
			tvd.Name	= "Indirect " + textureDesc.Name + " Atrous Pong";

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "INDIRECT_RADIANCE_ATROUS_PONG";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Prev Indirect " + textureDesc.Name + " Feedback";
			tvd.Name	= "Prev Indirect " + textureDesc.Name + " Feedback";

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "INDIRECT_RADIANCE_FEEDBACK";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	{
		TextureDesc textureDesc;
		textureDesc.Name				= "Albedo Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R32G32B32A32_SFLOAT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= renderWidth;
		textureDesc.Height				= renderHeight;
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc;
		textureViewDesc.Name			= "Albedo Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Direct " + textureDesc.Name;
			tvd.Name	= "Direct " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "DIRECT_ALBEDO";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Indirect " + textureDesc.Name;
			tvd.Name	= "Indirect " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "INDIRECT_ALBEDO";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	{
		TextureDesc textureDesc;
		textureDesc.Name				= "Moments Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R32G32B32A32_SFLOAT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= renderWidth;
		textureDesc.Height				= renderHeight;
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc;
		textureViewDesc.Name			= "Moments Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "MOMENTS";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Prev " + textureDesc.Name;
			tvd.Name	= "Prev " + textureDesc.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "PREV_MOMENTS";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	{
		TextureDesc textureDesc;
		textureDesc.Name				= "History Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R16_SFLOAT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= renderWidth;
		textureDesc.Height				= renderHeight;
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc;
		textureViewDesc.Name			= "History Texture View";
		textureViewDesc.Flags			= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "SVGF_HISTORY";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			td.Name		= "Prev " + td.Name;
			tvd.Name	= "Prev " + tvd.Name;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "PREV_SVGF_HISTORY";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}

		{
			TextureDesc td		= textureDesc;
			TextureViewDesc tvd = textureViewDesc;

			TextureDesc* pTextureDesc			= &td;
			TextureViewDesc* pTextureViewDesc	= &tvd;

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName								= "HISTORY";
			resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pTextureDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pTextureViewDesc;
			resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= &pNearestSamplerDesc;
			m_pRenderGraph->UpdateResource(resourceUpdateDesc);
		}
	}

	m_pRenderer = DBG_NEW Renderer(RenderSystem::GetDevice());

	RendererDesc rendererDesc = {};
	rendererDesc.pName				= "Renderer";
	rendererDesc.Debug				= RENDERING_DEBUG_ENABLED;
	rendererDesc.pRenderGraph		= m_pRenderGraph;
	rendererDesc.pWindow			= CommonApplication::Get()->GetMainWindow();
	rendererDesc.BackBufferCount	= BACK_BUFFER_COUNT;
	
	m_pRenderer->Init(&rendererDesc);

	if (RENDERING_DEBUG_ENABLED)
	{
		ImGui::SetCurrentContext(ImGuiRenderer::GetImguiContext());
	}

	m_pRenderGraph->Update();

	return true;
}

bool Sandbox::InitRendererForVisBuf()
{
	//using namespace LambdaEngine;

	//GUID_Lambda geometryVertexShaderGUID		= ResourceManager::LoadShaderFromFile("../Assets/Shaders/GeometryVisVertex.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda geometryPixelShaderGUID			= ResourceManager::LoadShaderFromFile("../Assets/Shaders/GeometryVisPixel.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	//GUID_Lambda fullscreenQuadShaderGUID		= ResourceManager::LoadShaderFromFile("../Assets/Shaders/FullscreenQuad.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda shadingPixelShaderGUID			= ResourceManager::LoadShaderFromFile("../Assets/Shaders/ShadingVisPixel.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	//std::vector<RenderStageDesc> renderStages;

	//const char*									pGeometryRenderStageName = "Geometry Render Stage";
	//GraphicsManagedPipelineStateDesc			geometryPipelineStateDesc = {};
	//std::vector<RenderStageAttachment>			geometryRenderStageAttachments;

	//{
	//	geometryRenderStageAttachments.push_back({ SCENE_VERTEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1, false});
	//	geometryRenderStageAttachments.push_back({ SCENE_INSTANCE_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1, false});

	//	geometryRenderStageAttachments.push_back({ PER_FRAME_BUFFER,							EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1, false });

	//	geometryRenderStageAttachments.push_back({ "GEOMETRY_VISIBILITY_BUFFER",				EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, false });
	//	geometryRenderStageAttachments.push_back({ "GEOMETRY_DEPTH_STENCIL",					EAttachmentType::OUTPUT_DEPTH_STENCIL,								FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1, false });

	//	RenderStagePushConstants pushConstants = {};
	//	pushConstants.pName			= "Geometry Pass Push Constants";
	//	pushConstants.DataSize		= sizeof(int32) * 2;

	//	RenderStageDesc renderStage = {};
	//	renderStage.pName						= pGeometryRenderStageName;
	//	renderStage.pAttachments				= geometryRenderStageAttachments.data();
	//	renderStage.AttachmentCount				= (uint32)geometryRenderStageAttachments.size();
	//	//renderStage.PushConstants				= pushConstants;

	//	geometryPipelineStateDesc.pName				= "Geometry Pass Pipeline State";
	//	geometryPipelineStateDesc.VertexShader		= geometryVertexShaderGUID;
	//	geometryPipelineStateDesc.PixelShader		= geometryPixelShaderGUID;

	//	renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

	//	renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::SCENE_INDIRECT;
	//	renderStage.GraphicsPipeline.pIndexBufferName		= SCENE_INDEX_BUFFER;
	//	renderStage.GraphicsPipeline.pMeshIndexBufferName	= SCENE_MESH_INDEX_BUFFER;
	//	renderStage.GraphicsPipeline.pGraphicsDesc			= &geometryPipelineStateDesc;

	//	renderStages.push_back(renderStage);
	//}

	//const char*									pShadingRenderStageName = "Shading Render Stage";
	//GraphicsManagedPipelineStateDesc			shadingPipelineStateDesc = {};
	//std::vector<RenderStageAttachment>			shadingRenderStageAttachments;

	//{
	//	shadingRenderStageAttachments.push_back({ "GEOMETRY_VISIBILITY_BUFFER",					EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, false });

	//	shadingRenderStageAttachments.push_back({ SCENE_VERTEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1, false});
	//	shadingRenderStageAttachments.push_back({ SCENE_INDEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1, false});
	//	shadingRenderStageAttachments.push_back({ SCENE_INSTANCE_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1, false});
	//	shadingRenderStageAttachments.push_back({ SCENE_MESH_INDEX_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1, false});

	//	shadingRenderStageAttachments.push_back({ PER_FRAME_BUFFER,								EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1, false });

	//	shadingRenderStageAttachments.push_back({ SCENE_MAT_PARAM_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1, false });
	//	shadingRenderStageAttachments.push_back({ SCENE_ALBEDO_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, false});
	//	shadingRenderStageAttachments.push_back({ SCENE_NORMAL_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, false});
	//	shadingRenderStageAttachments.push_back({ SCENE_AO_MAPS,								EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, false});
	//	shadingRenderStageAttachments.push_back({ SCENE_ROUGHNESS_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, false});
	//	shadingRenderStageAttachments.push_back({ SCENE_METALLIC_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, false});

	//	shadingRenderStageAttachments.push_back({ RENDER_GRAPH_BACK_BUFFER_ATTACHMENT,			EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, false });

	//	RenderStagePushConstants pushConstants = {};
	//	pushConstants.pName			= "Shading Pass Push Constants";
	//	pushConstants.DataSize		= sizeof(int32) * 2;

	//	RenderStageDesc renderStage = {};
	//	renderStage.pName						= pShadingRenderStageName;
	//	renderStage.pAttachments				= shadingRenderStageAttachments.data();
	//	renderStage.AttachmentCount				= (uint32)shadingRenderStageAttachments.size();
	//	//renderStage.PushConstants				= pushConstants;

	//	shadingPipelineStateDesc.pName				= "Shading Pass Pipeline State";
	//	shadingPipelineStateDesc.VertexShader		= fullscreenQuadShaderGUID;
	//	shadingPipelineStateDesc.PixelShader		= shadingPixelShaderGUID;

	//	renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

	//	renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::FULLSCREEN_QUAD;
	//	renderStage.GraphicsPipeline.pIndexBufferName		= nullptr;
	//	renderStage.GraphicsPipeline.pMeshIndexBufferName	= nullptr;
	//	renderStage.GraphicsPipeline.pGraphicsDesc			= &shadingPipelineStateDesc;

	//	renderStages.push_back(renderStage);
	//}

	//RenderGraphDesc renderGraphDesc = {};
	//renderGraphDesc.pName						= "Render Graph";
	//renderGraphDesc.CreateDebugGraph			= RENDER_GRAPH_DEBUG_ENABLED;
	//renderGraphDesc.CreateDebugStages			= RENDERING_DEBUG_ENABLED;
	//renderGraphDesc.pRenderStages				= renderStages.data();
	//renderGraphDesc.RenderStageCount			= (uint32)renderStages.size();
	//renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;
	//renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;
	//renderGraphDesc.pScene						= m_pScene;

	//LambdaEngine::Clock clock;
	//clock.Reset();
	//clock.Tick();

	//m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());

	//m_pRenderGraph->Init(&renderGraphDesc);

	//clock.Tick();
	//LOG_INFO("Render Graph Build Time: %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

	//uint32 renderWidth	= CommonApplication::Get()->GetMainWindow()->GetWidth();
	//uint32 renderHeight = CommonApplication::Get()->GetMainWindow()->GetHeight();
	//
	//{
	//	RenderStageParameters geometryRenderStageParameters = {};
	//	geometryRenderStageParameters.pRenderStageName	= pGeometryRenderStageName;
	//	geometryRenderStageParameters.Graphics.Width	= renderWidth;
	//	geometryRenderStageParameters.Graphics.Height	= renderHeight;

	//	m_pRenderGraph->UpdateRenderStageParameters(geometryRenderStageParameters);
	//}

	//{
	//	RenderStageParameters shadingRenderStageParameters = {};
	//	shadingRenderStageParameters.pRenderStageName	= pShadingRenderStageName;
	//	shadingRenderStageParameters.Graphics.Width	= renderWidth;
	//	shadingRenderStageParameters.Graphics.Height	= renderHeight;

	//	m_pRenderGraph->UpdateRenderStageParameters(shadingRenderStageParameters);
	//}

	//{
	//	IBuffer* pBuffer = m_pScene->GetPerFrameBuffer();
	//	ResourceUpdateDesc resourceUpdateDesc				= {};
	//	resourceUpdateDesc.pResourceName					= PER_FRAME_BUFFER;
	//	resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

	//	m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	//}

	//{
	//	IBuffer* pBuffer = m_pScene->GetMaterialProperties();
	//	ResourceUpdateDesc resourceUpdateDesc				= {};
	//	resourceUpdateDesc.pResourceName					= SCENE_MAT_PARAM_BUFFER;
	//	resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

	//	m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	//}

	//{
	//	IBuffer* pBuffer = m_pScene->GetVertexBuffer();
	//	ResourceUpdateDesc resourceUpdateDesc				= {};
	//	resourceUpdateDesc.pResourceName					= SCENE_VERTEX_BUFFER;
	//	resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

	//	m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	//}

	//{
	//	IBuffer* pBuffer = m_pScene->GetIndexBuffer();
	//	ResourceUpdateDesc resourceUpdateDesc				= {};
	//	resourceUpdateDesc.pResourceName					= SCENE_INDEX_BUFFER;
	//	resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

	//	m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	//}

	//{
	//	IBuffer* pBuffer = m_pScene->GetInstanceBufer();
	//	ResourceUpdateDesc resourceUpdateDesc				= {};
	//	resourceUpdateDesc.pResourceName					= SCENE_INSTANCE_BUFFER;
	//	resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

	//	m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	//}

	//{
	//	IBuffer* pBuffer = m_pScene->GetMeshIndexBuffer();
	//	ResourceUpdateDesc resourceUpdateDesc				= {};
	//	resourceUpdateDesc.pResourceName					= SCENE_MESH_INDEX_BUFFER;
	//	resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

	//	m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	//}

	//{
	//	TextureDesc visibilityBufferDesc	= {};
	//	visibilityBufferDesc.pName			= "Visibility Buffer Texture";
	//	visibilityBufferDesc.Type			= ETextureType::TEXTURE_2D;
	//	visibilityBufferDesc.MemoryType		= EMemoryType::MEMORY_GPU;
	//	visibilityBufferDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
	//	visibilityBufferDesc.Flags			= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
	//	visibilityBufferDesc.Width			= CommonApplication::Get()->GetMainWindow()->GetWidth();
	//	visibilityBufferDesc.Height			= CommonApplication::Get()->GetMainWindow()->GetHeight();
	//	visibilityBufferDesc.Depth			= 1;
	//	visibilityBufferDesc.SampleCount	= 1;
	//	visibilityBufferDesc.Miplevels		= 1;
	//	visibilityBufferDesc.ArrayCount		= 1;

	//	TextureViewDesc visibilityBufferViewDesc = { };
	//	visibilityBufferViewDesc.pName			= "Visibility Buffer Texture View";
	//	visibilityBufferViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
	//	visibilityBufferViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
	//	visibilityBufferViewDesc.Miplevel		= 0;
	//	visibilityBufferViewDesc.MiplevelCount	= 1;
	//	visibilityBufferViewDesc.ArrayIndex		= 0;
	//	visibilityBufferViewDesc.ArrayCount		= 1;
	//	visibilityBufferViewDesc.Format			= visibilityBufferDesc.Format;

	//	SamplerDesc samplerNearestDesc = {};
	//	samplerNearestDesc.pName				= "Nearest Sampler";
	//	samplerNearestDesc.MinFilter			= EFilter::NEAREST;
	//	samplerNearestDesc.MagFilter			= EFilter::NEAREST;
	//	samplerNearestDesc.MipmapMode			= EMipmapMode::NEAREST;
	//	samplerNearestDesc.AddressModeU			= EAddressMode::REPEAT;
	//	samplerNearestDesc.AddressModeV			= EAddressMode::REPEAT;
	//	samplerNearestDesc.AddressModeW			= EAddressMode::REPEAT;
	//	samplerNearestDesc.MipLODBias			= 0.0f;
	//	samplerNearestDesc.AnisotropyEnabled	= false;
	//	samplerNearestDesc.MaxAnisotropy		= 16;
	//	samplerNearestDesc.MinLOD				= 0.0f;
	//	samplerNearestDesc.MaxLOD				= 1.0f;

	//	std::vector<TextureDesc*> visibilityBufferTextureDescriptions(BACK_BUFFER_COUNT, &visibilityBufferDesc);
	//	std::vector<TextureViewDesc*> visibilityBufferTextureViewDescriptions(BACK_BUFFER_COUNT, &visibilityBufferViewDesc);
	//	std::vector<SamplerDesc*> visibilityBufferSamplerDescriptions(BACK_BUFFER_COUNT, &samplerNearestDesc);

	//	ResourceUpdateDesc resourceUpdateDesc = {};
	//	resourceUpdateDesc.pResourceName							= "GEOMETRY_VISIBILITY_BUFFER";
	//	resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= visibilityBufferTextureDescriptions.data();
	//	resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= visibilityBufferTextureViewDescriptions.data();
	//	resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= visibilityBufferSamplerDescriptions.data();

	//	m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	//}

	//{
	//	TextureDesc depthStencilDesc = {};
	//	depthStencilDesc.pName			= "Geometry Pass Depth Stencil Texture";
	//	depthStencilDesc.Type			= ETextureType::TEXTURE_2D;
	//	depthStencilDesc.MemoryType		= EMemoryType::MEMORY_GPU;
	//	depthStencilDesc.Format			= EFormat::FORMAT_D24_UNORM_S8_UINT;
	//	depthStencilDesc.Flags			= TEXTURE_FLAG_DEPTH_STENCIL;
	//	depthStencilDesc.Width			= CommonApplication::Get()->GetMainWindow()->GetWidth();
	//	depthStencilDesc.Height			= CommonApplication::Get()->GetMainWindow()->GetHeight();
	//	depthStencilDesc.Depth			= 1;
	//	depthStencilDesc.SampleCount	= 1;
	//	depthStencilDesc.Miplevels		= 1;
	//	depthStencilDesc.ArrayCount		= 1;

	//	TextureViewDesc depthStencilViewDesc = { };
	//	depthStencilViewDesc.pName			= "Geometry Pass Depth Stencil Texture View";
	//	depthStencilViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_DEPTH_STENCIL;
	//	depthStencilViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
	//	depthStencilViewDesc.Miplevel		= 0;
	//	depthStencilViewDesc.MiplevelCount	= 1;
	//	depthStencilViewDesc.ArrayIndex		= 0;
	//	depthStencilViewDesc.ArrayCount		= 1;
	//	depthStencilViewDesc.Format			= depthStencilDesc.Format;

	//	TextureDesc*		pDepthStencilDesc		= &depthStencilDesc;
	//	TextureViewDesc*	pDepthStencilViewDesc	= &depthStencilViewDesc;

	//	ResourceUpdateDesc resourceUpdateDesc = {};
	//	resourceUpdateDesc.pResourceName							= "GEOMETRY_DEPTH_STENCIL";
	//	resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pDepthStencilDesc;
	//	resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pDepthStencilViewDesc;

	//	m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	//}

	//{
	//	ITexture** ppAlbedoMaps						= m_pScene->GetAlbedoMaps();
	//	ITexture** ppNormalMaps						= m_pScene->GetNormalMaps();
	//	ITexture** ppAmbientOcclusionMaps			= m_pScene->GetAmbientOcclusionMaps();
	//	ITexture** ppMetallicMaps					= m_pScene->GetMetallicMaps();
	//	ITexture** ppRoughnessMaps					= m_pScene->GetRoughnessMaps();

	//	ITextureView** ppAlbedoMapViews				= m_pScene->GetAlbedoMapViews();
	//	ITextureView** ppNormalMapViews				= m_pScene->GetNormalMapViews();
	//	ITextureView** ppAmbientOcclusionMapViews	= m_pScene->GetAmbientOcclusionMapViews();
	//	ITextureView** ppMetallicMapViews			= m_pScene->GetMetallicMapViews();
	//	ITextureView** ppRoughnessMapViews			= m_pScene->GetRoughnessMapViews();

	//	std::vector<ISampler*> samplers(MAX_UNIQUE_MATERIALS, m_pLinearSampler);

	//	ResourceUpdateDesc albedoMapsUpdateDesc = {};
	//	albedoMapsUpdateDesc.pResourceName								= SCENE_ALBEDO_MAPS;
	//	albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppAlbedoMaps;
	//	albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppAlbedoMapViews;
	//	albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

	//	ResourceUpdateDesc normalMapsUpdateDesc = {};
	//	normalMapsUpdateDesc.pResourceName								= SCENE_NORMAL_MAPS;
	//	normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppNormalMaps;
	//	normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppNormalMapViews;
	//	normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

	//	ResourceUpdateDesc aoMapsUpdateDesc = {};
	//	aoMapsUpdateDesc.pResourceName									= SCENE_AO_MAPS;
	//	aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= ppAmbientOcclusionMaps;
	//	aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= ppAmbientOcclusionMapViews;
	//	aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= samplers.data();

	//	ResourceUpdateDesc metallicMapsUpdateDesc = {};
	//	metallicMapsUpdateDesc.pResourceName							= SCENE_METALLIC_MAPS;
	//	metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppMetallicMaps;
	//	metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppMetallicMapViews;
	//	metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

	//	ResourceUpdateDesc roughnessMapsUpdateDesc = {};
	//	roughnessMapsUpdateDesc.pResourceName							= SCENE_ROUGHNESS_MAPS;
	//	roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= ppRoughnessMaps;
	//	roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= ppRoughnessMapViews;
	//	roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= samplers.data();

	//	m_pRenderGraph->UpdateResource(albedoMapsUpdateDesc);
	//	m_pRenderGraph->UpdateResource(normalMapsUpdateDesc);
	//	m_pRenderGraph->UpdateResource(aoMapsUpdateDesc);
	//	m_pRenderGraph->UpdateResource(metallicMapsUpdateDesc);
	//	m_pRenderGraph->UpdateResource(roughnessMapsUpdateDesc);
	//}

	//m_pRenderGraph->Update();

	//m_pRenderer = DBG_NEW Renderer(RenderSystem::GetDevice());

	//RendererDesc rendererDesc = {};
	//rendererDesc.pName				= "Renderer";
	//rendererDesc.pRenderGraph		= m_pRenderGraph;
	//rendererDesc.pWindow			= CommonApplication::Get()->GetMainWindow();
	//rendererDesc.BackBufferCount	= BACK_BUFFER_COUNT;
	//
	//m_pRenderer->Init(&rendererDesc);

	return true;
}
