#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class IClientUDP;

	class LAMBDA_API IClientUDPRemoteHandler
	{
	public:
		DECL_INTERFACE(IClientUDPRemoteHandler);

		virtual void OnConnectingUDP(IClientUDP* pClient) = 0;
		virtual void OnConnectedUDP(IClientUDP* pClient) = 0;
		virtual void OnDisconnectingUDP(IClientUDP* pClient) = 0;
		virtual void OnDisconnectedUDP(IClientUDP* pClient) = 0;
		virtual void OnPacketReceivedUDP(IClientUDP* pClient, NetworkPacket* pPacket) = 0;
	};
}