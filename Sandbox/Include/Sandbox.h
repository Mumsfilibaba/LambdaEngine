#pragma once
#include "Game/Game.h"

#include "Application/API/EventHandler.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	struct GameObject;

	class RenderGraph;
	class Renderer;
	class ResourceManager;
	class IMusic;
	class ISoundEffect3D;
	class ISoundInstance3D;
	class IAudioGeometry;
	class IReverbSphere;
	class Scene;
	class Camera;
	class ISampler;
	class IAudioListener;
	class IMusicInstance;
}

class Sandbox : public LambdaEngine::Game, public LambdaEngine::EventHandler
{
public:
	Sandbox();
	~Sandbox();

	void InitTestAudio();

    // Inherited via IEventHandler
	virtual void KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat) override;
    
	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

private:
	bool InitRendererForDeferred();

private:
	GUID_Lambda						m_LaughSoundEffectGUID	= 0;
	LambdaEngine::ISoundEffect3D*	m_pLaughSoundEffect		= nullptr;
	LambdaEngine::ISoundInstance3D* m_pLaughInstance			= nullptr;

	GUID_Lambda						m_MusicGUID			= 0;
	LambdaEngine::IMusic*			m_pMusic			= nullptr;
	LambdaEngine::IMusicInstance*	m_pMusicInstance	= nullptr;

	LambdaEngine::IAudioListener* m_pAudioListener = nullptr;

	LambdaEngine::Scene*	m_pScene				= nullptr;
	LambdaEngine::Camera*	m_pCamera				= nullptr;
	LambdaEngine::ISampler*	m_pLinearSampler		= nullptr;
	LambdaEngine::ISampler*	m_pNearestSampler		= nullptr;

	LambdaEngine::RenderGraph*	m_pRenderGraph			= nullptr;
	LambdaEngine::Renderer*		m_pRenderer				= nullptr;

	GUID_Lambda	m_ImGuiPixelShaderNormalGUID		= GUID_NONE;
	GUID_Lambda	m_ImGuiPixelShaderDepthGUID			= GUID_NONE;
	GUID_Lambda	m_ImGuiPixelShaderRoughnessGUID		= GUID_NONE;

	float m_MasterVolume	= 0.5f;
	float m_MusicVolume		= 0.5f;
	float m_LaughVolume		= 0.5f;
};
