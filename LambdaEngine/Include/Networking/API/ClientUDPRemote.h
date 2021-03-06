#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClientUDP.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketManager.h"

namespace LambdaEngine
{
	class ServerUDP;
	class IClientUDPRemoteHandler;
	class PacketTransceiver;

	class LAMBDA_API ClientUDPRemote : 
		public IClientUDP,
		protected IPacketListener
	{
		friend class ServerUDP;
		
	public:
		~ClientUDPRemote();

		virtual void Disconnect() override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkPacket* packet) override;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener = nullptr) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkPacket* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;
		virtual const NetworkStatistics* GetStatistics() const override;

	protected:
		ClientUDPRemote(uint16 packetPoolSize, uint8 maximumTries, const IPEndPoint& ipEndPoint, ServerUDP* pServer);

		virtual PacketManager* GetPacketManager() override;

		virtual void OnPacketDelivered(NetworkPacket* pPacket) override;
		virtual void OnPacketResent(NetworkPacket* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkPacket* pPacket, uint8 tries) override;

	private:
		void OnDataReceived(PacketTransceiver* pTransciver);
		void SendPackets(PacketTransceiver* pTransciver);
		bool HandleReceivedPacket(NetworkPacket* pPacket);
		void Tick(Timestamp delta);

	private:
		ServerUDP* m_pServer;
		PacketManager m_PacketManager;
		SpinLock m_Lock;
		IClientUDPRemoteHandler* m_pHandler;
		EClientState m_State;
		std::atomic_bool m_Release;
		bool m_DisconnectedByRemote;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	};
}