#include "Networking/API/ClientUDP.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IClientUDPHandler.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/PacketPool.h"
#include "Networking/API/NetworkChallenge.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	std::set<ClientUDP*> ClientUDP::s_Clients;
	SpinLock ClientUDP::s_Lock;

	ClientUDP::ClientUDP(IClientUDPHandler* pHandler, uint16 packetPoolSize, uint8 maximumTries) :
		m_pSocket(nullptr),
		m_PacketManager(packetPoolSize, maximumTries),
		m_pHandler(pHandler), 
		m_State(STATE_DISCONNECTED),
		m_pSendBuffer()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Clients.insert(this);
	}

	ClientUDP::~ClientUDP()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Clients.erase(this);
		LOG_INFO("[ClientUDP]: Released");
	}

	void ClientUDP::OnPacketDelivered(NetworkPacket* pPacket)
	{
		LOG_INFO("ClientUDP::OnPacketDelivered() | %s", pPacket->ToString().c_str());
	}

	void ClientUDP::OnPacketResent(NetworkPacket* pPacket, uint8 tries)
	{
		LOG_INFO("ClientUDP::OnPacketResent(%d) | %s", tries, pPacket->ToString().c_str());
	}

	void ClientUDP::OnPacketMaxTriesReached(NetworkPacket* pPacket, uint8 tries)
	{
		LOG_INFO("ClientUDP::OnPacketMaxTriesReached(%d) | %s", tries, pPacket->ToString().c_str());
		Disconnect();
	}

	bool ClientUDP::Connect(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				LOG_WARNING("[ClientUDP]: Connecting...");
				m_PacketManager.SetEndPoint(ipEndPoint);
				return true;
			}
		}
		return false;
	}

	void ClientUDP::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateReceivingPacketLoss(lossRatio);
	}

	void ClientUDP::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateTransmittingPacketLoss(lossRatio);
	}

	void ClientUDP::Disconnect()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ClientUDP::Release()
	{
		NetWorker::TerminateAndRelease();
	}

	bool ClientUDP::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	bool ClientUDP::SendUnreliable(NetworkPacket* packet)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientUDP]: Can not send packet before a connection has been established");
			return false;
		}

		m_PacketManager.EnqueuePacketUnreliable(packet);
		return true;
	}

	bool ClientUDP::SendReliable(NetworkPacket* packet, IPacketListener* listener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientUDP]: Can not send packet before a connection has been established");
			return false;
		}
			
		m_PacketManager.EnqueuePacketReliable(packet, listener);
		return true;
	}

	const IPEndPoint& ClientUDP::GetEndPoint() const
	{
		return m_PacketManager.GetEndPoint();
	}

	NetworkPacket* ClientUDP::GetFreePacket(uint16 packetType)
	{
		return m_PacketManager.GetPacketPool()->RequestFreePacket()->SetType(packetType);
	}

	EClientState ClientUDP::GetState() const
	{
		return m_State;
	}

	const NetworkStatistics* ClientUDP::GetStatistics() const
	{
		return m_PacketManager.GetStatistics();
	}

	PacketManager* ClientUDP::GetPacketManager()
	{
		return &m_PacketManager;
	}

	bool ClientUDP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(IPEndPoint(IPAddress::ANY, 0)))
			{
				m_Transciver.SetSocket(m_pSocket);
				m_PacketManager.Reset();
				m_State = STATE_CONNECTING;
				m_pHandler->OnConnectingUDP(this);
				m_SendDisconnectPacket = true;
				SendConnectRequest();
				return true;
			}
			LOG_ERROR("[ClientUDP]: Failed To Bind socket");
			return false;
		}
		LOG_ERROR("[ClientUDP]: Failed To Create socket");
		return false;
	}

	void ClientUDP::RunReceiver()
	{
		IPEndPoint sender;
		while (!ShouldTerminate())
		{
			if (!m_Transciver.ReceiveBegin(sender))
				continue;

			TArray<NetworkPacket*> packets;
			m_PacketManager.QueryBegin(&m_Transciver, packets);
			for (NetworkPacket* pPacket : packets)
			{
				HandleReceivedPacket(pPacket);
			}
			m_PacketManager.QueryEnd(packets);
		}
	}

	void ClientUDP::RunTranmitter()
	{
		while (!ShouldTerminate())
		{
			TransmitPackets();
			YieldTransmitter();
		}
	}

	void ClientUDP::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ClientUDP]: Disconnected");
		m_State = STATE_DISCONNECTED;
		if(m_pHandler)
			m_pHandler->OnDisconnectedUDP(this);
	}

	void ClientUDP::OnTerminationRequested()
	{
		LOG_WARNING("[ClientUDP]: Disconnecting...");
		m_State = STATE_DISCONNECTING;
		m_pHandler->OnDisconnectingUDP(this);

		if(m_SendDisconnectPacket)
			SendDisconnectRequest();
	}

	void ClientUDP::OnReleaseRequested()
	{
		Disconnect();
		m_pHandler = nullptr;
	}

	void ClientUDP::SendConnectRequest()
	{
		m_PacketManager.EnqueuePacketReliable(GetFreePacket(NetworkPacket::TYPE_CONNNECT), this);
		TransmitPackets();
	}

	void ClientUDP::SendDisconnectRequest()
	{
		m_PacketManager.EnqueuePacketReliable(GetFreePacket(NetworkPacket::TYPE_DISCONNECT), this);
		TransmitPackets();
	}

	void ClientUDP::HandleReceivedPacket(NetworkPacket* pPacket)
	{
		LOG_MESSAGE("PING %fms", GetStatistics()->GetPing().AsMilliSeconds());
		uint16 packetType = pPacket->GetType();

		if (packetType == NetworkPacket::TYPE_CHALLENGE)
		{
			uint64 answer = NetworkChallenge::Compute(GetStatistics()->GetSalt(), pPacket->GetRemoteSalt());
			ASSERT(answer != 0);

			NetworkPacket* pResponse = GetFreePacket(NetworkPacket::TYPE_CHALLENGE);
			BinaryEncoder encoder(pResponse);
			encoder.WriteUInt64(answer);
			m_PacketManager.EnqueuePacketReliable(pResponse, this);
		}
		else if (packetType == NetworkPacket::TYPE_ACCEPTED)
		{
			if (m_State == STATE_CONNECTING)
			{
				LOG_INFO("[ClientUDP]: Connected");
				m_State = STATE_CONNECTED;
				m_pHandler->OnConnectedUDP(this);
			}
		}
		else if (packetType == NetworkPacket::TYPE_DISCONNECT)
		{
			m_SendDisconnectPacket = false;
			Disconnect();
		}
		else if (packetType == NetworkPacket::TYPE_SERVER_FULL)
		{
			m_SendDisconnectPacket = false;
			m_pHandler->OnServerFullUDP(this);
			Disconnect();
		}
		else
		{
			m_pHandler->OnPacketReceivedUDP(this, pPacket);
		}
	}

	void ClientUDP::TransmitPackets()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_PacketManager.Flush(&m_Transciver);
	}

	void ClientUDP::Tick(Timestamp delta)
	{
		if (m_State != STATE_DISCONNECTED)
		{
			m_PacketManager.Tick(delta);
		}
		
		Flush();
	}

	ClientUDP* ClientUDP::Create(IClientUDPHandler* pHandler, uint16 packetPoolSize, uint8 maximumTries)
	{
		return DBG_NEW ClientUDP(pHandler, packetPoolSize, maximumTries);
	}

	void ClientUDP::FixedTickStatic(Timestamp timestamp)
	{
		UNREFERENCED_VARIABLE(timestamp);

		if (!s_Clients.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (ClientUDP* client : s_Clients)
			{
				client->Tick(timestamp);
			}
		}
	}
}