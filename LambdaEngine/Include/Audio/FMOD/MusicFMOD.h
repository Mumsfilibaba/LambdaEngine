#pragma once
#include "FMOD.h"

#include "Audio/API/IMusic.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class MusicFMOD : public IMusic
	{
	public:
		MusicFMOD(const IAudioDevice* pAudioDevice);
		~MusicFMOD();

		bool Init(const MusicDesc *pDesc);

		FORCEINLINE FMOD_SOUND*		GetHandle()		{ return m_pHandle; }
		FORCEINLINE FMOD_CHANNEL*	GetChannel()	{ return m_pChannel; }

		virtual SoundDesc	GetDesc()	const override final;

	private:
		//Engine
		const AudioDeviceFMOD* m_pAudioDevice	= nullptr;

		//FMOD
		FMOD_SOUND*		m_pHandle				= nullptr;
		FMOD_CHANNEL*	m_pChannel				= nullptr;

		//Local
		const char*		m_pName					= "";
		float			m_Volume				= 1.0f;
		float			m_Pitch					= 1.0f;
	};
}
