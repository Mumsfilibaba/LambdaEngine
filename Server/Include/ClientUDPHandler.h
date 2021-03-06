#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IClientUDPRemoteHandler.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class IClientUDP;
}

class ClientUDPHandler : public LambdaEngine::IClientUDPRemoteHandler
{
public:
	ClientUDPHandler();
	~ClientUDPHandler();

	virtual void OnConnectingUDP(LambdaEngine::IClientUDP* pClient) override;
	virtual void OnConnectedUDP(LambdaEngine::IClientUDP* pClient) override;
	virtual void OnDisconnectingUDP(LambdaEngine::IClientUDP* pClient) override;
	virtual void OnDisconnectedUDP(LambdaEngine::IClientUDP* pClient) override;
	virtual void OnPacketReceivedUDP(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkPacket* pPacket) override;

private:
	int counter = 0;
};