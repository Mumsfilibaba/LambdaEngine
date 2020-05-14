#pragma once
#include "Game/Game.h"

#include "Application/API/EventHandler.h"
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
	class IAudioFilter;
}

class Sandbox : public LambdaEngine::Game, public LambdaEngine::EventHandler
{
public:
	Sandbox();
	~Sandbox();

	void InitTestAudio();

    // Inherited via IEventHandler
    virtual void FocusChanged(LambdaEngine::IWindow* pWindow, bool hasFocus)                                                 override;
    virtual void WindowMoved(LambdaEngine::IWindow* pWindow, int16 x, int16 y)                                               override;
    virtual void WindowResized(LambdaEngine::IWindow* pWindow, uint16 width, uint16 height, LambdaEngine::EResizeType type)  override;
    virtual void WindowClosed(LambdaEngine::IWindow* pWindow)                                                                override;
    virtual void MouseEntered(LambdaEngine::IWindow* pWindow)                                                                override;
    virtual void MouseLeft(LambdaEngine::IWindow* pWindow)                                                                   override;

	virtual void KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)     override;
	virtual void KeyReleased(LambdaEngine::EKey key)                                        override;
	virtual void KeyTyped(uint32 character)                                                 override;
	
	virtual void MouseMoved(int32 x, int32 y)                                               override;
	virtual void ButtonPressed(LambdaEngine::EMouseButton button, uint32 modifierMask)      override;
	virtual void ButtonReleased(LambdaEngine::EMouseButton button)                          override;
    virtual void MouseScrolled(int32 deltaX, int32 deltaY)                                  override;
    
	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

private:
	bool InitRendererForEmpty();
	bool InitRendererForDeferred();
	bool InitRendererForVisBuf();

private:
	uint32									m_AudioListenerIndex	= 0;

	GUID_Lambda								m_SoundEffectGUID0;
	LambdaEngine::ISoundEffect3D*			m_pSoundEffect0			= nullptr;
	LambdaEngine::ISoundInstance3D*			m_pSoundInstance0		= nullptr;

	GUID_Lambda								m_SoundEffectGUID1;
	LambdaEngine::ISoundEffect3D*			m_pSoundEffect1			= nullptr;
	LambdaEngine::ISoundInstance3D*			m_pSoundInstance1		= nullptr;

	GUID_Lambda								m_SoundEffectGUID2;
	LambdaEngine::ISoundEffect3D*			m_pSoundEffect2			= nullptr;
	LambdaEngine::ISoundInstance3D*			m_pSoundInstance2		= nullptr;

	GUID_Lambda								m_SoundEffectGUID3;
	LambdaEngine::ISoundEffect3D*			m_pSoundEffect3			= nullptr;
	LambdaEngine::ISoundInstance3D*			m_pSoundInstance3		= nullptr;

	LambdaEngine::IMusic*					m_pBGMusic				= nullptr;
	LambdaEngine::IMusic*					m_pBGTone				= nullptr;

	LambdaEngine::IReverbSphere*			m_pReverbSphere			= nullptr;
	LambdaEngine::IAudioGeometry*			m_pAudioGeometry		= nullptr;

	LambdaEngine::Scene*					m_pScene				= nullptr;
	LambdaEngine::Camera*					m_pCamera				= nullptr;
	LambdaEngine::ISampler*					m_pLinearSampler		= nullptr;
	LambdaEngine::ISampler*					m_pNearestSampler		= nullptr;

	LambdaEngine::RenderGraph*				m_pRenderGraph			= nullptr;
	LambdaEngine::Renderer*					m_pRenderer				= nullptr;

	LambdaEngine::IAudioFilter*				m_pLowpassIIRFilter		= nullptr;
	LambdaEngine::IAudioFilter*				m_pBandpassIIRFilter	= nullptr;

	LambdaEngine::IAudioFilter*				m_pF1Add0				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1AP0				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1AP1				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1AP2				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1A0					= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1A1					= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1A2					= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1Add1				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1Add2				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1LPF				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF1G					= nullptr;
	LambdaEngine::IAudioFilter*				m_pReverbSystem1		= nullptr;

	LambdaEngine::IAudioFilter*				m_pF2Comb0				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF2Comb1				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF2Comb2				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF2Comb3				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF2AP0				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF2AP1				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF2AP2				= nullptr;
	LambdaEngine::IAudioFilter*				m_pF2Add0				= nullptr;
	LambdaEngine::IAudioFilter*				m_pReverbSystem2		= nullptr;

	LambdaEngine::IAudioFilter*				m_pCombFilter			= nullptr;
	LambdaEngine::IAudioFilter*				m_pAllPassFilter		= nullptr;

	LambdaEngine::IAudioFilter*				m_pMasterFilter			= nullptr;

	GUID_Lambda								m_ImGuiPixelShaderNormalGUID		= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelShaderDepthGUID			= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelShaderRoughnessGUID		= GUID_NONE;

	bool									m_SpawnPlayAts;
	float									m_GunshotTimer;
	float									m_GunshotDelay;
	float									m_Timer;

};
