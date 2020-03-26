#include "Engine/EngineLoop.h"

extern LambdaEngine::Game* CreateGame();

#ifdef LAMBDA_PLATFORM_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
#else
int main(int, const char*[])
#endif 
{
	using namespace LambdaEngine;

#ifdef LAMBDA_PLATFORM_WINDOWS
	if (!EngineLoop::PreInit(hInstance))
#else
	if (!EngineLoop::PreInit())
#endif
	{
		return -1;
	}

	if (!EngineLoop::Init())
	{
		return -1;
	}

	LambdaEngine::Game* pGame = CreateGame();
	EngineLoop::Run(pGame);

	SAFEDELETE(pGame);

	if (!EngineLoop::Release())
	{
		return -1;
	}

	if (!EngineLoop::PostRelease())
	{
		return -1;
	}

	return 0;
}
