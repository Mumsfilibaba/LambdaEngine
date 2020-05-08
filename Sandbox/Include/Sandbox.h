#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Application/API/IWindowHandler.h"
#include "Application/API/PlatformApplication.h"

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

class Sandbox : public LambdaEngine::Game, public LambdaEngine::IKeyboardHandler, public LambdaEngine::IMouseHandler
{
public:
	Sandbox();
	~Sandbox();

	void InitTestAudio();
   
	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	// Inherited via IKeyboardHandler
	virtual void KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)     override;
	virtual void KeyReleased(LambdaEngine::EKey key)                                        override;
	virtual void KeyTyped(uint32 character)                                                 override;

	// Inherited via IMouseHandler
	virtual void MouseMoved(int32 x, int32 y)                                               override;
	virtual void ButtonPressed(LambdaEngine::EMouseButton button, uint32 modifierMask)      override;
	virtual void ButtonReleased(LambdaEngine::EMouseButton button)                          override;
    virtual void MouseScrolled(int32 deltaX, int32 deltaY)                                  override;

private:
	bool InitRendererForDeferred();

private:
	GUID_Lambda						m_ToneSoundEffectGUID	= 0;
	LambdaEngine::ISoundEffect3D*	m_pToneSoundEffect		= nullptr;
	LambdaEngine::ISoundInstance3D*	m_pToneSoundInstance	= nullptr;

	GUID_Lambda						m_GunSoundEffectGUID	= 0;
	LambdaEngine::ISoundEffect3D*	m_pGunSoundEffect		= nullptr;
	LambdaEngine::ISoundInstance3D* m_pGunInstance			= nullptr;
    
    GUID_Lambda                     m_MusicEffectGUID       = 0;
    LambdaEngine::ISoundEffect3D*   m_pMusicEffect          = nullptr;
    LambdaEngine::ISoundInstance3D* m_pMusicEffectInstance  = nullptr;

	GUID_Lambda						m_MusicGUID			= 0;
	LambdaEngine::IMusic*			m_pMusic			= nullptr;
	LambdaEngine::IMusicInstance*	m_pMusicInstance	= nullptr;

	LambdaEngine::IAudioListener* m_pAudioListener = nullptr;

	LambdaEngine::IReverbSphere*  m_pReverbSphere			= nullptr;
	LambdaEngine::IAudioGeometry* m_pAudioGeometry		= nullptr;

	LambdaEngine::Scene*	m_pScene				= nullptr;
	LambdaEngine::Camera*	m_pCamera				= nullptr;
	LambdaEngine::ISampler*	m_pLinearSampler		= nullptr;
	LambdaEngine::ISampler*	m_pNearestSampler		= nullptr;

	LambdaEngine::RenderGraph*	m_pRenderGraph			= nullptr;
	LambdaEngine::Renderer*		m_pRenderer				= nullptr;

	bool  m_SpawnPlayAts;
	float m_GunshotTimer;
	float m_GunshotDelay;
	float m_Timer;

};
