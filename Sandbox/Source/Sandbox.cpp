#include "Sandbox.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/ImGuiRenderer.h"
#include "Rendering/Renderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraphDescriptionParser.h"
#include "Rendering/Core/API/ITextureView.h"
#include "Rendering/Core/API/ISampler.h"
#include "Rendering/Core/API/ICommandQueue.h"

#include "Audio/AudioSystem.h"
#include "Audio/API/ISoundEffect3D.h"
#include "Audio/API/ISoundInstance3D.h"
#include "Audio/API/IAudioGeometry.h"
#include "Audio/API/IReverbSphere.h"
#include "Audio/API/IMusic.h"
#include "Audio/API/IAudioListener.h"
#include "Audio/API/IMusicInstance.h"

#include "Application/API/IWindow.h"
#include "Application/API/CommonApplication.h"

#include "Game/Scene.h"

#include "Time/API/Clock.h"

#include "Threading/API/Thread.h"

#include <imgui.h>

constexpr const uint32 BACK_BUFFER_COUNT = 3;
#ifdef LAMBDA_PLATFORM_MACOS
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 8;
#else
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 512;
#endif
constexpr const bool RAY_TRACING_ENABLED		= false;
constexpr const bool POST_PROCESSING_ENABLED	= false;

constexpr const bool RENDERING_DEBUG_ENABLED	= true;

static float goblinPos[3] = { 0.0f, 0.5f, 65.0f };
static float guardPos1[3] = { -5.0f, 0.3f, 30.0f };
static float guardPos2[3] = {  5.0f, 0.3f, 30.0f };
static float cricketPos[3] = { -4.5f, 2.0f, -12.0f };
static float camPos[3] = { 0.0f, 6.0f, -10.0f };
static float camRot[3] = { 0.0f, -90.0f, 0.0f };

Sandbox::Sandbox()
    : Game()
{
	using namespace LambdaEngine;

	CommonApplication::Get()->AddEventHandler(this);	
	m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice());

	SceneDesc sceneDesc = {};
	sceneDesc.pName				= "Test Scene";
	sceneDesc.RayTracingEnabled = RAY_TRACING_ENABLED;
	m_pScene->Init(sceneDesc);

	GUID_Lambda goblinGUID			= ResourceManager::LoadMeshFromFile("../Assets/Meshes/Goblin.obj");
	GUID_Lambda goblinDiffGUID		= ResourceManager::LoadTextureFromFile("../Assets/Textures/goblin_diffuse.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
	GUID_Lambda goblinNormalGUID	= ResourceManager::LoadTextureFromFile("../Assets/Textures/goblin_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
	GUID_Lambda goblinSpecularGUID	= ResourceManager::LoadTextureFromFile("../Assets/Textures/goblin_specular.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

	GUID_Lambda guardGUID			= ResourceManager::LoadMeshFromFile("../Assets/Meshes/guard.obj");
	GUID_Lambda guardDiffGUID		= ResourceManager::LoadTextureFromFile("../Assets/Textures/guard_diffuse.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
	GUID_Lambda guardNormalGUID		= ResourceManager::LoadTextureFromFile("../Assets/Textures/guard_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
	GUID_Lambda guardSpecularGUID	= ResourceManager::LoadTextureFromFile("../Assets/Textures/guard_specular.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

	MaterialProperties goblinProp = { };
	GUID_Lambda goblinMatGUID = ResourceManager::LoadMaterialFromMemory(goblinDiffGUID, goblinNormalGUID, GUID_NONE, GUID_NONE, goblinSpecularGUID, goblinProp);

	GameObject goblinObject = {};
	goblinObject.Mesh		= goblinGUID;
	goblinObject.Material	= goblinMatGUID;

	glm::mat4 transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(goblinPos[0], goblinPos[1], goblinPos[2])), glm::pi<float32>(), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.01f));
	m_pScene->AddDynamicGameObject(goblinObject, transform);

	MaterialProperties guardProp = { };
	GUID_Lambda guardMatGUID = ResourceManager::LoadMaterialFromMemory(guardDiffGUID, guardNormalGUID, GUID_NONE, GUID_NONE, guardSpecularGUID, guardProp);

	GameObject guardObject = {};
	guardObject.Mesh		= guardGUID;
	guardObject.Material	= guardMatGUID;

	transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(guardPos1[0], guardPos1[1], guardPos1[2])), glm::pi<float32>(), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.015f));
	m_pScene->AddDynamicGameObject(guardObject, transform);

	transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(guardPos2[0], guardPos2[1], guardPos2[2])), glm::pi<float32>(), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.015f));
	m_pScene->AddDynamicGameObject(guardObject, transform);

	TArray<GameObject> sceneGameObjects;
	ResourceManager::LoadSceneFromFile("../Assets/Scenes/suntemple/", "suntemple.obj", sceneGameObjects);

	for (GameObject& gameObject : sceneGameObjects)
	{
		m_pScene->AddDynamicGameObject(gameObject, glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)));
	}

	m_pScene->UpdateDirectionalLight(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(300.0f));	
	m_pScene->Finalize();

	m_pCamera = DBG_NEW Camera();

	IWindow* pWindow = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= 90.0f;
	cameraDesc.Width		= pWindow->GetWidth();
	cameraDesc.Height		= pWindow->GetHeight();
	cameraDesc.NearPlane	= 0.01f;
	cameraDesc.FarPlane		= 500.0f;

	m_pCamera->Init(cameraDesc);
	m_pCamera->SetPosition(glm::vec3(camPos[0], camPos[1], camPos[2]));
	m_pCamera->SetRotation(glm::vec3(camRot[0], camRot[1], camRot[2]));
	m_pCamera->Update();

	m_pScene->UpdateCamera(m_pCamera);

	SamplerDesc samplerLinearDesc = {};
	samplerLinearDesc.pName					= "Linear Sampler";
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
	samplerNearestDesc.pName				= "Nearest Sampler";
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

	//InitRendererForEmpty();

	InitRendererForDeferred();
	InitTestAudio();
}

Sandbox::~Sandbox()
{
    LambdaEngine::CommonApplication::Get()->RemoveEventHandler(this);
    
	SAFEDELETE(m_pScene);
	SAFEDELETE(m_pCamera);
	SAFERELEASE(m_pLinearSampler);
	SAFERELEASE(m_pNearestSampler);

	SAFEDELETE(m_pRenderGraph);
	SAFEDELETE(m_pRenderer);

	SAFERELEASE(m_pLaughInstance);
	SAFERELEASE(m_pMusicInstance);
	SAFERELEASE(m_pAudioListener);
}

void Sandbox::InitTestAudio()
{
	using namespace LambdaEngine;

	m_MusicGUID	= ResourceManager::LoadMusicFromFile("../Assets/Sounds/mountain.wav");
	m_pMusic	= ResourceManager::GetMusic(m_MusicGUID);

	m_LaughSoundEffectGUID	= ResourceManager::LoadSoundEffectFromFile("../Assets/Sounds/laugh.wav");
	m_pLaughSoundEffect		= ResourceManager::GetSoundEffect(m_LaughSoundEffectGUID);

	m_CricketSoundEffectGUID	= ResourceManager::LoadSoundEffectFromFile("../Assets/Sounds/crickets.wav");
	m_pCricketSoundEffect		= ResourceManager::GetSoundEffect(m_CricketSoundEffectGUID);

	m_ArrowKneeSoundEffectGUID	= ResourceManager::LoadSoundEffectFromFile("../Assets/Sounds/arrow_knee.wav");
	m_pArrowKneeSoundEffect		= ResourceManager::GetSoundEffect(m_ArrowKneeSoundEffectGUID);

	MusicInstanceDesc musicInstanceDesc = { };
	musicInstanceDesc.pMusic	= m_pMusic;
	musicInstanceDesc.Volume	= m_MusicVolume;
	musicInstanceDesc.Pitch		= 1.0f;

	m_pMusicInstance = AudioSystem::GetDevice()->CreateMusicInstance(&musicInstanceDesc);
	m_pMusicInstance->Play();

	SoundInstance3DDesc soundInstanceDesc = { };
	soundInstanceDesc.pSoundEffect		= m_pLaughSoundEffect;
	soundInstanceDesc.Mode				= ESoundMode::SOUND_MODE_LOOPING;
	soundInstanceDesc.ReferenceDistance = m_LaughReferenceDistance;
	soundInstanceDesc.MaxDistance		= m_LaughMaxDistance;
	soundInstanceDesc.RollOff			= m_LaughRollOff;
	soundInstanceDesc.Volume			= m_LaughVolume;
	soundInstanceDesc.Position			= glm::vec3(goblinPos[0], goblinPos[1], goblinPos[2]);

	m_pLaughInstance = AudioSystem::GetDevice()->CreateSoundInstance(&soundInstanceDesc);
	m_pLaughInstance->Play();

	soundInstanceDesc.pSoundEffect		= m_pCricketSoundEffect;
	soundInstanceDesc.Mode				= ESoundMode::SOUND_MODE_LOOPING;
	soundInstanceDesc.ReferenceDistance = m_CricketsReferenceDistance;
	soundInstanceDesc.MaxDistance		= m_CricketsMaxDistance;
	soundInstanceDesc.RollOff			= m_CricketsRollOff;
	soundInstanceDesc.Volume			= m_CricketsVolume;
	soundInstanceDesc.Position			= glm::vec3(cricketPos[0], cricketPos[1], cricketPos[2]);

	m_pCricketInstance = AudioSystem::GetDevice()->CreateSoundInstance(&soundInstanceDesc);
	m_pCricketInstance->Play();

	soundInstanceDesc.pSoundEffect		= m_pArrowKneeSoundEffect;
	soundInstanceDesc.Mode				= ESoundMode::SOUND_MODE_LOOPING;
	soundInstanceDesc.ReferenceDistance = m_ArrowKneeReferenceDistance;
	soundInstanceDesc.MaxDistance		= m_ArrowKneeMaxDistance;
	soundInstanceDesc.RollOff			= m_ArrowKneeRollOff;
	soundInstanceDesc.Volume			= m_ArrowKneeVolume;
	soundInstanceDesc.Position			= glm::vec3(guardPos1[0], guardPos1[1], guardPos1[2]);

	m_pArrowKneeInstance1 = AudioSystem::GetDevice()->CreateSoundInstance(&soundInstanceDesc);
	m_pArrowKneeInstance1->Play();

	soundInstanceDesc.Position = glm::vec3(guardPos2[0], guardPos2[1], guardPos2[2]);
	m_pArrowKneeInstance2 = AudioSystem::GetDevice()->CreateSoundInstance(&soundInstanceDesc);
	m_pArrowKneeInstance2->Play();

	AudioListenerDesc listenerDesc = { };
	listenerDesc.Position	= m_pCamera->GetPosition();
	listenerDesc.Volume		= 1.0f;
	listenerDesc.Forward	= m_pCamera->GetForwardVec();
	listenerDesc.Up			= m_pCamera->GetUpVec();
	m_pAudioListener = AudioSystem::GetDevice()->CreateAudioListener(&listenerDesc);

	AudioSystem::GetDevice()->SetMasterVolume(m_MasterVolume);
}

void Sandbox::KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)
{
	UNREFERENCED_VARIABLE(modifierMask);
	
    using namespace LambdaEngine;

	LOG_INFO("KeyPressed=%s", KeyToString(key));

   if (!isRepeat)
	{
		if (key == EKey::KEY_7)
		{
			float32 volume = m_pMusicInstance->GetVolume() - 0.025f;
			LOG_INFO("Music Volume: %.4f", volume);

			m_pMusicInstance->SetVolume(volume);
		}
		else if (key == EKey::KEY_8)
		{
			m_pMusicInstance->Toggle();
		}
		else if (key == EKey::KEY_9)
		{
			float32 volume = m_pMusicInstance->GetVolume() + 0.025f;
			LOG_INFO("Music Volume: %.4f", volume);

			m_pMusicInstance->SetVolume(volume);
		}
		else if (key == EKey::KEY_5)
		{
			m_pMusicInstance->Stop();
		}
		else if (key == EKey::KEY_1)
		{
			camPos[0] = 0.0f;
			camPos[1] = 6.0f;
			camPos[2] = -10.0f;
			m_pCamera->SetPosition(glm::vec3(camPos[0], camPos[1], camPos[2]));

			camRot[0] = 0.0f;
			camRot[1] = -90.0f;
			camRot[2] = 0.0f;
			m_pCamera->SetRotation(glm::vec3(camRot[0], camRot[1], camRot[2]));
		}
		else if (key == EKey::KEY_KEYPAD_5)
		{
			RenderSystem::GetGraphicsQueue()->Flush();
			RenderSystem::GetComputeQueue()->Flush();
			ResourceManager::ReloadAllShaders();
			PipelineStateManager::ReloadPipelineStates();
		}
		else if (key == EKey::KEY_ESCAPE)
		{
			CommonApplication::Terminate();
		}
	}
}

void Sandbox::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	constexpr float CAMERA_ROTATION_SPEED = 45.0f;

	float cameraMovementSpeed = 1.0f;
	if (Input::IsKeyDown(EKey::KEY_LEFT_SHIFT))
	{
		cameraMovementSpeed = 5.0f;
	}

	if (Input::IsKeyDown(EKey::KEY_W) && Input::IsKeyUp(EKey::KEY_S))
	{
		m_pCamera->Translate(glm::vec3(0.0f, 0.0f, cameraMovementSpeed * delta.AsSeconds()));
	}
	else if (Input::IsKeyDown(EKey::KEY_S) && Input::IsKeyUp(EKey::KEY_W))
	{
		m_pCamera->Translate(glm::vec3(0.0f, 0.0f, -cameraMovementSpeed * delta.AsSeconds()));
	}

	if (Input::IsKeyDown(EKey::KEY_A) && Input::IsKeyUp(EKey::KEY_D))
	{
		m_pCamera->Translate(glm::vec3(-cameraMovementSpeed * delta.AsSeconds(), 0.0f, 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_D) && Input::IsKeyUp(EKey::KEY_A))
	{
		m_pCamera->Translate(glm::vec3(cameraMovementSpeed * delta.AsSeconds(), 0.0f, 0.0f));
	}

	if (Input::IsKeyDown(EKey::KEY_Q) && Input::IsKeyUp(EKey::KEY_E))
	{
		m_pCamera->Translate(glm::vec3(0.0f, cameraMovementSpeed * delta.AsSeconds(), 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_E) && Input::IsKeyUp(EKey::KEY_Q))
	{
		m_pCamera->Translate(glm::vec3(0.0f, -cameraMovementSpeed * delta.AsSeconds(), 0.0f));
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

	m_pRenderer->Begin(delta);

	// ImGui Camera
	glm::vec3 cameraPos = m_pCamera->GetPosition();
	camPos[0] = cameraPos.x;
	camPos[1] = cameraPos.y;
	camPos[2] = cameraPos.z;

	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Camera", NULL))
	{
		if (ImGui::DragFloat3("Position", camPos, 0.1f))
		{
			m_pCamera->SetPosition(glm::vec3(camPos[0], camPos[1], camPos[2]));
		}

		if (ImGui::DragFloat3("Rotation", camRot, 0.1f))
		{
			m_pCamera->SetRotation(glm::vec3(camRot[0], camRot[1], camRot[2]));
		}
	}
	ImGui::End();

	// Update camera
	m_pCamera->Update();
	
	m_pAudioListener->SetDirectionVectors(m_pCamera->GetUpVec(), m_pCamera->GetForwardVec());
	m_pAudioListener->SetPosition(m_pCamera->GetPosition());

	m_pScene->UpdateCamera(m_pCamera);

	// Imgui
	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Settings", NULL))
	{
		// Settings
		ImGui::Text("Settings:");

		if (ImGui::SliderFloat("Master Volume", &m_MasterVolume, 0.0f, 1.0f, "%.2f"))
		{
			AudioSystem::GetDevice()->SetMasterVolume(m_MasterVolume);
		}

		// Music
		ImGui::PushID(0);
		{
			ImGui::Text("Music:");
			if (ImGui::SliderFloat("Volume", &m_MusicVolume, 0.0f, 1.0f, "%.2f"))
			{
				m_pMusicInstance->SetVolume(m_MusicVolume);
			}

			if (ImGui::Button("Play"))
			{
				m_pMusicInstance->Play();
			}

			ImGui::SameLine();

			if (ImGui::Button("Pause"))
			{
				m_pMusicInstance->Pause();
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop"))
			{
				m_pMusicInstance->Stop();
			}

			ImGui::SameLine();

			if (ImGui::Button("Toggle"))
			{
				m_pMusicInstance->Toggle();
			}
		}
		ImGui::PopID();

		// Laugh Sound Effect
		ImGui::PushID(1);
		{
			ImGui::Text("Laugh Sound Effect:");

			if (ImGui::SliderFloat("Volume", &m_LaughVolume, 0.0f, 1.0f, "%.2f"))
			{
				m_pLaughInstance->SetVolume(m_LaughVolume);
			}

			if (ImGui::DragFloat("Roll Off", &m_LaughRollOff, 0.1f, 1.0f, 20.0f, "%.1f"))
			{
				m_pLaughInstance->SetRollOff(m_LaughRollOff);
			}

			if (ImGui::DragFloat("Max Distance", &m_LaughMaxDistance, 0.1f, 1.0f, 100.0f, "%.1f"))
			{
				m_pLaughInstance->SetMaxDistance(m_LaughMaxDistance);
			}

			if (ImGui::DragFloat("Reference Distance", &m_LaughReferenceDistance, 0.1f, 0.0f, m_LaughMaxDistance, "%.1f"))
			{
				m_pLaughInstance->SetReferenceDistance(m_LaughReferenceDistance);
			}

			if (ImGui::Button("Play"))
			{
				m_pLaughInstance->Play();
			}

			ImGui::SameLine();

			if (ImGui::Button("Pause"))
			{
				m_pLaughInstance->Pause();
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop"))
			{
				m_pLaughInstance->Stop();
			}

			ImGui::SameLine();

			if (ImGui::Button("Toggle"))
			{
				m_pLaughInstance->Toggle();
			}
		}
		ImGui::PopID();

		// Cricket effect
		ImGui::PushID(2);
		{
			ImGui::Text("Cricket Sound Effect:");

			if (ImGui::SliderFloat("Volume", &m_CricketsVolume, 0.0f, 1.0f, "%.2f"))
			{
				m_pCricketInstance->SetVolume(m_CricketsVolume);
			}

			if (ImGui::DragFloat("Roll Off", &m_CricketsRollOff, 0.1f, 1.0f, 20.0f, "%.1f"))
			{
				m_pCricketInstance->SetRollOff(m_CricketsRollOff);
			}

			if (ImGui::DragFloat("Max Distance", &m_CricketsMaxDistance, 0.1f, 1.0f, 100.0f, "%.1f"))
			{
				m_pCricketInstance->SetMaxDistance(m_CricketsMaxDistance);
			}

			if (ImGui::DragFloat("Reference Distance", &m_CricketsReferenceDistance, 0.1f, 0.0f, m_CricketsMaxDistance, "%.1f"))
			{
				m_pCricketInstance->SetReferenceDistance(m_CricketsReferenceDistance);
			}

			if (ImGui::Button("Play"))
			{
				m_pCricketInstance->Play();
			}

			ImGui::SameLine();

			if (ImGui::Button("Pause"))
			{
				m_pCricketInstance->Pause();
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop"))
			{
				m_pCricketInstance->Stop();
			}

			ImGui::SameLine();

			if (ImGui::Button("Toggle"))
			{
				m_pCricketInstance->Toggle();
			}
		}
		ImGui::PopID();

		// Guard Sound Effect
		ImGui::PushID(3);
		{
			ImGui::Text("Guard Sound Effect:");

			if (ImGui::SliderFloat("Volume", &m_ArrowKneeVolume, 0.0f, 1.0f, "%.2f"))
			{
				m_pArrowKneeInstance1->SetVolume(m_ArrowKneeVolume);
				m_pArrowKneeInstance2->SetVolume(m_ArrowKneeVolume);
			}

			if (ImGui::DragFloat("Roll Off", &m_ArrowKneeRollOff, 0.1f, 1.0f, 20.0f, "%.1f"))
			{
				m_pArrowKneeInstance1->SetRollOff(m_ArrowKneeRollOff);
				m_pArrowKneeInstance2->SetRollOff(m_ArrowKneeRollOff);
			}

			if (ImGui::DragFloat("Max Distance", &m_ArrowKneeMaxDistance, 0.1f, 1.0f, 100.0f, "%.1f"))
			{
				m_pArrowKneeInstance1->SetMaxDistance(m_ArrowKneeMaxDistance);
				m_pArrowKneeInstance2->SetMaxDistance(m_ArrowKneeMaxDistance);
			}

			if (ImGui::DragFloat("Reference Distance", &m_ArrowKneeReferenceDistance, 0.1f, 0.0f, m_ArrowKneeMaxDistance, "%.1f"))
			{
				m_pArrowKneeInstance1->SetReferenceDistance(m_ArrowKneeReferenceDistance);
				m_pArrowKneeInstance2->SetReferenceDistance(m_ArrowKneeReferenceDistance);
			}

			if (ImGui::Button("Play"))
			{
				m_pArrowKneeInstance1->Play();
				m_pArrowKneeInstance2->Play();
			}

			ImGui::SameLine();

			if (ImGui::Button("Pause"))
			{
				m_pArrowKneeInstance1->Pause();
				m_pArrowKneeInstance2->Pause();
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop"))
			{
				m_pArrowKneeInstance1->Pause();
				m_pArrowKneeInstance2->Pause();
			}

			ImGui::SameLine();

			if (ImGui::Button("Toggle"))
			{
				m_pArrowKneeInstance1->Toggle();
				m_pArrowKneeInstance2->Toggle();
			}
		}
		ImGui::PopID();
	}
	ImGui::End();

#if 0
	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Test Window", NULL))
	{
		ImGui::Button("Test Button");

		uint32 modFrameIndex = m_pRenderer->GetModFrameIndex();

		ITextureView* const*	ppTextureViews		= nullptr;
		uint32					textureViewCount	= 0;

		static ImGuiTexture albedoTexture = {};
		static ImGuiTexture normalTexture = {};
		static ImGuiTexture depthStencilTexture = {};

		float windowWidth = ImGui::GetWindowWidth();
		if (ImGui::BeginTabBar("G-Buffer"))
		{
			if (ImGui::BeginTabItem("Albedo AO"))
			{
				if (m_pRenderGraph->GetResourceTextureViews("GEOMETRY_ALBEDO_AO_BUFFER", &ppTextureViews, &textureViewCount))
				{
					ITextureView* pTextureView = ppTextureViews[modFrameIndex];
					albedoTexture.pTextureView = pTextureView;

					float32 aspectRatio = (float32)pTextureView->GetDesc().pTexture->GetDesc().Width / (float32)pTextureView->GetDesc().pTexture->GetDesc().Height;

					ImGui::Image(&albedoTexture, ImVec2(windowWidth, windowWidth / aspectRatio));
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Normal Metallic Roughness"))
			{
				if (m_pRenderGraph->GetResourceTextureViews("GEOMETRY_NORM_MET_ROUGH_BUFFER", &ppTextureViews, &textureViewCount))
				{
					ITextureView* pTextureView = ppTextureViews[modFrameIndex];

					normalTexture.pTextureView = pTextureView;

					const char* items[] = { "ALL", "Normal", "Metallic", "Roughness" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), 4);

					if (currentItem == 0)
					{
						normalTexture.ReservedIncludeMask = 0x00008421;

						normalTexture.ChannelMult[0] = 0.5f;
						normalTexture.ChannelMult[1] = 0.5f;
						normalTexture.ChannelMult[2] = 0.5f;
						normalTexture.ChannelMult[3] = 0.5f;

						normalTexture.ChannelAdd[0] = 0.5f;
						normalTexture.ChannelAdd[1] = 0.5f;
						normalTexture.ChannelAdd[2] = 0.5f;
						normalTexture.ChannelAdd[3] = 0.5f;

						normalTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 1)
					{
						normalTexture.ReservedIncludeMask = 0x00008420;

						normalTexture.ChannelMult[0] = 1.0f;
						normalTexture.ChannelMult[1] = 1.0f;
						normalTexture.ChannelMult[2] = 1.0f;
						normalTexture.ChannelMult[3] = 0.0f;

						normalTexture.ChannelAdd[0] = 0.0f;
						normalTexture.ChannelAdd[1] = 0.0f;
						normalTexture.ChannelAdd[2] = 0.0f;
						normalTexture.ChannelAdd[3] = 1.0f;

						normalTexture.PixelShaderGUID = m_ImGuiPixelShaderNormalGUID;
					}
					else if (currentItem == 2)
					{
						normalTexture.ReservedIncludeMask = 0x00002220;

						normalTexture.ChannelMult[0] = 0.5f;
						normalTexture.ChannelMult[1] = 0.5f;
						normalTexture.ChannelMult[2] = 0.5f;
						normalTexture.ChannelMult[3] = 0.0f;

						normalTexture.ChannelAdd[0] = 0.5f;
						normalTexture.ChannelAdd[1] = 0.5f;
						normalTexture.ChannelAdd[2] = 0.5f;
						normalTexture.ChannelAdd[3] = 1.0f;

						normalTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 3)
					{
						normalTexture.ReservedIncludeMask = 0x00001110;

						normalTexture.ChannelMult[0] = 1.0f;
						normalTexture.ChannelMult[1] = 1.0f;
						normalTexture.ChannelMult[2] = 1.0f;
						normalTexture.ChannelMult[3] = 0.0f;

						normalTexture.ChannelAdd[0] = 0.0f;
						normalTexture.ChannelAdd[1] = 0.0f;
						normalTexture.ChannelAdd[2] = 0.0f;
						normalTexture.ChannelAdd[3] = 1.0f;

						normalTexture.PixelShaderGUID = m_ImGuiPixelShaderRoughnessGUID;
					}

					float32 aspectRatio = (float32)pTextureView->GetDesc().pTexture->GetDesc().Width / (float32)pTextureView->GetDesc().pTexture->GetDesc().Height;
					ImGui::Image(&normalTexture, ImVec2(windowWidth, windowWidth / aspectRatio));
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Depth Stencil"))
			{
				if (m_pRenderGraph->GetResourceTextureViews("GEOMETRY_DEPTH_STENCIL", &ppTextureViews, &textureViewCount))
				{
					ITextureView* pTextureView = ppTextureViews[modFrameIndex];
					depthStencilTexture.pTextureView = pTextureView;

					depthStencilTexture.ReservedIncludeMask = 0x00008880;

					depthStencilTexture.ChannelMult[0] = 1.0f;
					depthStencilTexture.ChannelMult[1] = 1.0f;
					depthStencilTexture.ChannelMult[2] = 1.0f;
					depthStencilTexture.ChannelMult[3] = 0.0f;

					depthStencilTexture.ChannelAdd[0] = 0.0f;
					depthStencilTexture.ChannelAdd[1] = 0.0f;
					depthStencilTexture.ChannelAdd[2] = 0.0f;
					depthStencilTexture.ChannelAdd[3] = 1.0f;

					depthStencilTexture.PixelShaderGUID = m_ImGuiPixelShaderDepthGUID;

					float32 aspectRatio = (float32)pTextureView->GetDesc().pTexture->GetDesc().Width / (float32)pTextureView->GetDesc().pTexture->GetDesc().Height;

					ImGui::Image(&depthStencilTexture, ImVec2(windowWidth, windowWidth / aspectRatio));
				}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
#endif

	m_pRenderer->Render(delta);
	m_pRenderer->End(delta);
}

void Sandbox::FixedTick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
        Sandbox* pSandbox = DBG_NEW Sandbox();        
        return pSandbox;
    }
}

bool Sandbox::InitRendererForDeferred()
{
	using namespace LambdaEngine;

	GUID_Lambda geometryVertexShaderGUID		= ResourceManager::LoadShaderFromFile("../Assets/Shaders/GeometryDefVertex.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::GLSL);
	GUID_Lambda geometryPixelShaderGUID			= ResourceManager::LoadShaderFromFile("../Assets/Shaders/GeometryDefPixel.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	GUID_Lambda fullscreenQuadShaderGUID		= ResourceManager::LoadShaderFromFile("../Assets/Shaders/FullscreenQuad.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::GLSL);
	GUID_Lambda shadingPixelShaderGUID			= ResourceManager::LoadShaderFromFile("../Assets/Shaders/ShadingDefPixel.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	GUID_Lambda raygenShaderGUID				= ResourceManager::LoadShaderFromFile("../Assets/Shaders/Raygen.glsl",					FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			EShaderLang::GLSL);
	GUID_Lambda closestHitShaderGUID			= ResourceManager::LoadShaderFromFile("../Assets/Shaders/ClosestHit.glsl",				FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::GLSL);
	GUID_Lambda missShaderGUID					= ResourceManager::LoadShaderFromFile("../Assets/Shaders/Miss.glsl",					FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::GLSL);

	GUID_Lambda postProcessShaderGUID			= ResourceManager::LoadShaderFromFile("../Assets/Shaders/PostProcess.glsl",				FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,		EShaderLang::GLSL);

	m_ImGuiPixelShaderNormalGUID				= ResourceManager::LoadShaderFromFile("../Assets/Shaders/ImGuiPixelNormal.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelShaderDepthGUID					= ResourceManager::LoadShaderFromFile("../Assets/Shaders/ImGuiPixelDepth.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);
	m_ImGuiPixelShaderRoughnessGUID				= ResourceManager::LoadShaderFromFile("../Assets/Shaders/ImGuiPixelRoughness.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	//GUID_Lambda geometryVertexShaderGUID		= ResourceManager::LoadShaderFromFile("../Assets/Shaders/geometryDefVertex.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	//GUID_Lambda geometryPixelShaderGUID			= ResourceManager::LoadShaderFromFile("../Assets/Shaders/geometryDefPixel.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);

	//GUID_Lambda fullscreenQuadShaderGUID		= ResourceManager::LoadShaderFromFile("../Assets/Shaders/fullscreenQuad.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	//GUID_Lambda shadingPixelShaderGUID			= ResourceManager::LoadShaderFromFile("../Assets/Shaders/shadingDefPixel.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);
	
	std::vector<RenderStageDesc> renderStages;

	const char*									pGeometryRenderStageName = "Geometry Render Stage";
	GraphicsManagedPipelineStateDesc			geometryPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			geometryRenderStageAttachments;

	{
		geometryRenderStageAttachments.push_back({ SCENE_VERTEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ SCENE_INDEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ SCENE_INSTANCE_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ SCENE_MESH_INDEX_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});

		geometryRenderStageAttachments.push_back({ PER_FRAME_BUFFER,							EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});

		geometryRenderStageAttachments.push_back({ SCENE_MAT_PARAM_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1 });
		geometryRenderStageAttachments.push_back({ SCENE_ALBEDO_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, EFormat::FORMAT_R8G8B8A8_UNORM });
		geometryRenderStageAttachments.push_back({ SCENE_NORMAL_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, EFormat::FORMAT_R8G8B8A8_UNORM });
		geometryRenderStageAttachments.push_back({ SCENE_AO_MAPS,								EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, EFormat::FORMAT_R8G8B8A8_UNORM });
		geometryRenderStageAttachments.push_back({ SCENE_METALLIC_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, EFormat::FORMAT_R8G8B8A8_UNORM });
		geometryRenderStageAttachments.push_back({ SCENE_ROUGHNESS_MAPS,						EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS, EFormat::FORMAT_R8G8B8A8_UNORM });

		
		geometryRenderStageAttachments.push_back({ "GEOMETRY_ALBEDO_AO_BUFFER",					EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R8G8B8A8_UNORM		});
		geometryRenderStageAttachments.push_back({ "GEOMETRY_NORM_MET_ROUGH_BUFFER",			EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R16G16B16A16_SFLOAT	});
		geometryRenderStageAttachments.push_back({ "GEOMETRY_DEPTH_STENCIL",					EAttachmentType::OUTPUT_DEPTH_STENCIL,								FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_D24_UNORM_S8_UINT	});

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName			= "Geometry Pass Push Constants";
		pushConstants.DataSize		= sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName						= pGeometryRenderStageName;
		renderStage.pAttachments				= geometryRenderStageAttachments.data();
		renderStage.AttachmentCount				= (uint32)geometryRenderStageAttachments.size();

		geometryPipelineStateDesc.pName				= "Geometry Pass Pipeline State";
		geometryPipelineStateDesc.VertexShader		= geometryVertexShaderGUID;
		geometryPipelineStateDesc.PixelShader		= geometryPixelShaderGUID;

		renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

		renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::SCENE_INDIRECT;
		renderStage.GraphicsPipeline.pIndexBufferName		= SCENE_INDEX_BUFFER;
		renderStage.GraphicsPipeline.pMeshIndexBufferName	= SCENE_MESH_INDEX_BUFFER;
		renderStage.GraphicsPipeline.pGraphicsDesc			= &geometryPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	const char*									pRayTracingRenderStageName = "Ray Tracing Render Stage";
	RayTracingManagedPipelineStateDesc			rayTracingPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			rayTracingRenderStageAttachments;

	if (RAY_TRACING_ENABLED)
	{
		rayTracingRenderStageAttachments.push_back({ "GEOMETRY_ALBEDO_AO_BUFFER",					EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R8G8B8A8_UNORM		 });
		rayTracingRenderStageAttachments.push_back({ "GEOMETRY_NORM_MET_ROUGH_BUFFER",				EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R16G16B16A16_SFLOAT	 });
		rayTracingRenderStageAttachments.push_back({ "GEOMETRY_DEPTH_STENCIL",						EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_D24_UNORM_S8_UINT	 });
	
		rayTracingRenderStageAttachments.push_back({ "SCENE_TLAS",									EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE,				FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,	1 });
		rayTracingRenderStageAttachments.push_back({ PER_FRAME_BUFFER,								EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER, 1 });

		rayTracingRenderStageAttachments.push_back({ "RADIANCE_TEXTURE",							EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE,					FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R8G8B8A8_UNORM });

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName			= "Ray Tracing Pass Push Constants";
		pushConstants.DataSize		= sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName						= pRayTracingRenderStageName;
		renderStage.pAttachments				= rayTracingRenderStageAttachments.data();
		renderStage.AttachmentCount				= (uint32)rayTracingRenderStageAttachments.size();

		rayTracingPipelineStateDesc.pName					= "Ray Tracing Pass Pipeline State";
		rayTracingPipelineStateDesc.RaygenShader			= raygenShaderGUID;
		rayTracingPipelineStateDesc.pClosestHitShaders[0]	= closestHitShaderGUID;
		rayTracingPipelineStateDesc.pMissShaders[0]			= missShaderGUID;
		rayTracingPipelineStateDesc.ClosestHitShaderCount	= 1;
		rayTracingPipelineStateDesc.MissShaderCount			= 1;

		renderStage.PipelineType							= EPipelineStateType::RAY_TRACING;

		renderStage.RayTracingPipeline.pRayTracingDesc		= &rayTracingPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	const char*									pShadingRenderStageName = "Shading Render Stage";
	GraphicsManagedPipelineStateDesc			shadingPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			shadingRenderStageAttachments;

	{
		shadingRenderStageAttachments.push_back({ "GEOMETRY_ALBEDO_AO_BUFFER",					EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R8G8B8A8_UNORM		 });
		shadingRenderStageAttachments.push_back({ "GEOMETRY_NORM_MET_ROUGH_BUFFER",				EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R16G16B16A16_SFLOAT	 });
		shadingRenderStageAttachments.push_back({ "GEOMETRY_DEPTH_STENCIL",						EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_D24_UNORM_S8_UINT	 });

		if (RAY_TRACING_ENABLED)
			shadingRenderStageAttachments.push_back({ "RADIANCE_TEXTURE",						EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R8G8B8A8_UNORM });

		shadingRenderStageAttachments.push_back({ "LIGHTS_BUFFER",								EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, 1 });
		shadingRenderStageAttachments.push_back({ PER_FRAME_BUFFER,								EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, 1 });

		if (POST_PROCESSING_ENABLED)
			shadingRenderStageAttachments.push_back({ "SHADED_TEXTURE",								EAttachmentType::OUTPUT_COLOR,									FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R8G8B8A8_UNORM });
		else
			shadingRenderStageAttachments.push_back({ RENDER_GRAPH_BACK_BUFFER_ATTACHMENT,			EAttachmentType::OUTPUT_COLOR,									FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_B8G8R8A8_UNORM });

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName			= "Shading Pass Push Constants";
		pushConstants.DataSize		= sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName						= pShadingRenderStageName;
		renderStage.pAttachments				= shadingRenderStageAttachments.data();
		renderStage.AttachmentCount				= (uint32)shadingRenderStageAttachments.size();

		shadingPipelineStateDesc.pName				= "Shading Pass Pipeline State";
		shadingPipelineStateDesc.VertexShader		= fullscreenQuadShaderGUID;
		shadingPipelineStateDesc.PixelShader		= shadingPixelShaderGUID;

		renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

		renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::FULLSCREEN_QUAD;
		renderStage.GraphicsPipeline.pIndexBufferName		= nullptr;
		renderStage.GraphicsPipeline.pMeshIndexBufferName	= nullptr;
		renderStage.GraphicsPipeline.pGraphicsDesc			= &shadingPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	const char*									pPostProcessRenderStageName = "Post-Process Render Stage";
	ComputeManagedPipelineStateDesc				postProcessPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			postProcessRenderStageAttachments;

	if (POST_PROCESSING_ENABLED)
	{
		postProcessRenderStageAttachments.push_back({ "SHADED_TEXTURE",								EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_R8G8B8A8_UNORM });

		postProcessRenderStageAttachments.push_back({ RENDER_GRAPH_BACK_BUFFER_ATTACHMENT,			EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE,					FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,	BACK_BUFFER_COUNT, EFormat::FORMAT_B8G8R8A8_UNORM });

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName			= "Post-Process Pass Push Constants";
		pushConstants.DataSize		= sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName						= pPostProcessRenderStageName;
		renderStage.pAttachments				= postProcessRenderStageAttachments.data();
		renderStage.AttachmentCount				= (uint32)postProcessRenderStageAttachments.size();

		postProcessPipelineStateDesc.pName		= "Post-Process Pass Pipeline State";
		postProcessPipelineStateDesc.Shader		= postProcessShaderGUID;

		renderStage.PipelineType						= EPipelineStateType::COMPUTE;

		renderStage.ComputePipeline.pComputeDesc			= &postProcessPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	RenderGraphDesc renderGraphDesc = {};
	renderGraphDesc.pName						= "Render Graph";
	renderGraphDesc.CreateDebugGraph			= RENDERING_DEBUG_ENABLED;
	renderGraphDesc.pRenderStages				= renderStages.data();
	renderGraphDesc.RenderStageCount			= (uint32)renderStages.size();
	renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;
	renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;
	renderGraphDesc.pScene						= m_pScene;

	LambdaEngine::Clock clock;
	clock.Reset();
	clock.Tick();

	m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());

	m_pRenderGraph->Init(renderGraphDesc);

	clock.Tick();
	LOG_INFO("Render Graph Build Time: %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

	IWindow* pWindow	= CommonApplication::Get()->GetMainWindow();
	uint32 renderWidth	= pWindow->GetWidth();
	uint32 renderHeight = pWindow->GetHeight();
	
	{
		RenderStageParameters geometryRenderStageParameters = {};
		geometryRenderStageParameters.pRenderStageName	= pGeometryRenderStageName;
		geometryRenderStageParameters.Graphics.Width	= renderWidth;
		geometryRenderStageParameters.Graphics.Height	= renderHeight;

		m_pRenderGraph->UpdateRenderStageParameters(geometryRenderStageParameters);
	}

	{
		RenderStageParameters shadingRenderStageParameters = {};
		shadingRenderStageParameters.pRenderStageName	= pShadingRenderStageName;
		shadingRenderStageParameters.Graphics.Width		= renderWidth;
		shadingRenderStageParameters.Graphics.Height	= renderHeight;

		m_pRenderGraph->UpdateRenderStageParameters(shadingRenderStageParameters);
	}

	if (RAY_TRACING_ENABLED)
	{
		RenderStageParameters rayTracingRenderStageParameters = {};
		rayTracingRenderStageParameters.pRenderStageName			= pRayTracingRenderStageName;
		rayTracingRenderStageParameters.RayTracing.RayTraceWidth	= renderWidth;
		rayTracingRenderStageParameters.RayTracing.RayTraceHeight	= renderHeight;
		rayTracingRenderStageParameters.RayTracing.RayTraceDepth	= 1;

		m_pRenderGraph->UpdateRenderStageParameters(rayTracingRenderStageParameters);
	}

	if (POST_PROCESSING_ENABLED)
	{
		GraphicsDeviceFeatureDesc features = { };
		RenderSystem::GetDevice()->QueryDeviceFeatures(&features);

		RenderStageParameters postProcessRenderStageParameters = {};
		postProcessRenderStageParameters.pRenderStageName				= pPostProcessRenderStageName;
		postProcessRenderStageParameters.Compute.WorkGroupCountX		= (renderWidth * renderHeight);
		postProcessRenderStageParameters.Compute.WorkGroupCountY		= 1;
		postProcessRenderStageParameters.Compute.WorkGroupCountZ		= 1;

		m_pRenderGraph->UpdateRenderStageParameters(postProcessRenderStageParameters);
	}

	{
		IBuffer* pBuffer = m_pScene->GetLightsBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= "LIGHTS_BUFFER";
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetPerFrameBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= PER_FRAME_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetMaterialProperties();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_MAT_PARAM_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetVertexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_VERTEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetIndexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_INDEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetInstanceBufer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_INSTANCE_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetMeshIndexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_MESH_INDEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IAccelerationStructure* pTLAS = m_pScene->GetTLAS();
		ResourceUpdateDesc resourceUpdateDesc					= {};
		resourceUpdateDesc.pResourceName						= "SCENE_TLAS";
		resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= pTLAS;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		TextureDesc textureDesc	= {};
		textureDesc.pName				= "Albedo-AO G-Buffer Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R8G8B8A8_UNORM;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height				= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc = { };
		textureViewDesc.pName			= "Albedo-AO G-Buffer Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "Nearest Sampler";
		samplerDesc.MinFilter			= EFilter::NEAREST;
		samplerDesc.MagFilter			= EFilter::NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerDesc.AddressModeU		= EAddressMode::REPEAT;
		samplerDesc.AddressModeV		= EAddressMode::REPEAT;
		samplerDesc.AddressModeW		= EAddressMode::REPEAT;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> textureDescriptions(BACK_BUFFER_COUNT, &textureDesc);
		std::vector<TextureViewDesc*> textureViewDescriptions(BACK_BUFFER_COUNT, &textureViewDesc);
		std::vector<SamplerDesc*> samplerDescriptions(BACK_BUFFER_COUNT, &samplerDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "GEOMETRY_ALBEDO_AO_BUFFER";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= textureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= textureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= samplerDescriptions.data();

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		TextureDesc textureDesc	= {};
		textureDesc.pName				= "Norm-Met-Rough G-Buffer Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R16G16B16A16_SFLOAT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height				= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureView = { };
		textureView.pName				= "Norm-Met-Rough G-Buffer Texture View";
		textureView.Flags				= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureView.Type				= ETextureViewType::TEXTURE_VIEW_2D;
		textureView.Miplevel			= 0;
		textureView.MiplevelCount		= 1;
		textureView.ArrayIndex			= 0;
		textureView.ArrayCount			= 1;
		textureView.Format				= textureDesc.Format;

		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "Nearest Sampler";
		samplerDesc.MinFilter			= EFilter::NEAREST;
		samplerDesc.MagFilter			= EFilter::NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerDesc.AddressModeU		= EAddressMode::REPEAT;
		samplerDesc.AddressModeV		= EAddressMode::REPEAT;
		samplerDesc.AddressModeW		= EAddressMode::REPEAT;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> textureDescriptions(BACK_BUFFER_COUNT, &textureDesc);
		std::vector<TextureViewDesc*> textureViewDescriptions(BACK_BUFFER_COUNT, &textureView);
		std::vector<SamplerDesc*> samplerDescriptions(BACK_BUFFER_COUNT, &samplerDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "GEOMETRY_NORM_MET_ROUGH_BUFFER";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= textureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= textureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= samplerDescriptions.data();

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		TextureDesc textureDesc = {};
		textureDesc.pName				= "Geometry Pass Depth Stencil Texture";
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
		textureViewDesc.pName			= "Geometry Pass Depth Stencil Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_DEPTH_STENCIL | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "Nearest Sampler";
		samplerDesc.MinFilter			= EFilter::NEAREST;
		samplerDesc.MagFilter			= EFilter::NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerDesc.AddressModeU		= EAddressMode::REPEAT;
		samplerDesc.AddressModeV		= EAddressMode::REPEAT;
		samplerDesc.AddressModeW		= EAddressMode::REPEAT;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> textureDescriptions(BACK_BUFFER_COUNT, &textureDesc);
		std::vector<TextureViewDesc*> textureViewDescriptions(BACK_BUFFER_COUNT, &textureViewDesc);
		std::vector<SamplerDesc*> samplerDescriptions(BACK_BUFFER_COUNT, &samplerDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "GEOMETRY_DEPTH_STENCIL";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= textureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= textureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= samplerDescriptions.data();

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	if (RAY_TRACING_ENABLED)
	{
		TextureDesc textureDesc	= {};
		textureDesc.pName				= "Radiance Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R8G8B8A8_UNORM;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height				= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc = { };
		textureViewDesc.pName			= "Radiance Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "Nearest Sampler";
		samplerDesc.MinFilter			= EFilter::NEAREST;
		samplerDesc.MagFilter			= EFilter::NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerDesc.AddressModeU		= EAddressMode::REPEAT;
		samplerDesc.AddressModeV		= EAddressMode::REPEAT;
		samplerDesc.AddressModeW		= EAddressMode::REPEAT;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> textureDescriptions(BACK_BUFFER_COUNT, &textureDesc);
		std::vector<TextureViewDesc*> textureViewDescriptions(BACK_BUFFER_COUNT, &textureViewDesc);
		std::vector<SamplerDesc*> samplerDescriptions(BACK_BUFFER_COUNT, &samplerDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "RADIANCE_TEXTURE";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= textureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= textureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= samplerDescriptions.data();

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	if (POST_PROCESSING_ENABLED)
	{
		TextureDesc textureDesc	= {};
		textureDesc.pName				= "Shaded Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R8G8B8A8_UNORM;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= CommonApplication::Get()->GetMainWindow()->GetWidth();
		textureDesc.Height				= CommonApplication::Get()->GetMainWindow()->GetHeight();
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc = { };
		textureViewDesc.pName			= "Shaded Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "Nearest Sampler";
		samplerDesc.MinFilter			= EFilter::NEAREST;
		samplerDesc.MagFilter			= EFilter::NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerDesc.AddressModeU		= EAddressMode::REPEAT;
		samplerDesc.AddressModeV		= EAddressMode::REPEAT;
		samplerDesc.AddressModeW		= EAddressMode::REPEAT;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> textureDescriptions(BACK_BUFFER_COUNT, &textureDesc);
		std::vector<TextureViewDesc*> textureViewDescriptions(BACK_BUFFER_COUNT, &textureViewDesc);
		std::vector<SamplerDesc*> samplerDescriptions(BACK_BUFFER_COUNT, &samplerDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "SHADED_TEXTURE";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= textureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= textureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= samplerDescriptions.data();

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

		std::vector<ISampler*> samplers(MAX_UNIQUE_MATERIALS, m_pLinearSampler);
		ResourceUpdateDesc albedoMapsUpdateDesc = {};
		albedoMapsUpdateDesc.pResourceName								= SCENE_ALBEDO_MAPS;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppAlbedoMaps;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppAlbedoMapViews;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc normalMapsUpdateDesc = {};
		normalMapsUpdateDesc.pResourceName								= SCENE_NORMAL_MAPS;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppNormalMaps;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppNormalMapViews;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc aoMapsUpdateDesc = {};
		aoMapsUpdateDesc.pResourceName									= SCENE_AO_MAPS;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= ppAmbientOcclusionMaps;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= ppAmbientOcclusionMapViews;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= samplers.data();

		ResourceUpdateDesc metallicMapsUpdateDesc = {};
		metallicMapsUpdateDesc.pResourceName							= SCENE_METALLIC_MAPS;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppMetallicMaps;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppMetallicMapViews;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc roughnessMapsUpdateDesc = {};
		roughnessMapsUpdateDesc.pResourceName							= SCENE_ROUGHNESS_MAPS;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= ppRoughnessMaps;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= ppRoughnessMapViews;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= samplers.data();

		m_pRenderGraph->UpdateResource(albedoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(normalMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(aoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(metallicMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(roughnessMapsUpdateDesc);
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
