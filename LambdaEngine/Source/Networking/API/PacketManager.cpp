#include "Networking/API/PacketManager.h"

#include "Networking/API/NetworkPacket.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiver.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PacketManager::PacketManager(uint16 poolSize, int32 maxRetries, float32 resendRTTMultiplier) :
		m_PacketPool(poolSize),
		m_QueueIndex(0),
		m_MaxRetries(maxRetries),
		m_ResendRTTMultiplier(resendRTTMultiplier)
	{

	}

	PacketManager::~PacketManager()
	{

	}

	uint32 PacketManager::EnqueuePacketReliable(NetworkPacket* pPacket, IPacketListener* pListener)
	{
		std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);
		uint32 UID = EnqueuePacket(pPacket, m_Statistics.RegisterReliableMessageSent());
		m_MessagesWaitingForAck.insert({ UID, MessageInfo{ pPacket, pListener, EngineLoop::GetTimeSinceStart()} });
		return UID;
	}

	uint32 PacketManager::EnqueuePacketUnreliable(NetworkPacket* pPacket)
	{
		std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);
		return EnqueuePacket(pPacket, 0);
	}

	uint32 PacketManager::EnqueuePacket(NetworkPacket* pPacket, uint32 reliableUID)
	{
		pPacket->GetHeader().UID = m_Statistics.RegisterMessageSent();
		pPacket->GetHeader().ReliableUID = reliableUID;
		m_MessagesToSend[m_QueueIndex].push(pPacket);
		return pPacket->GetHeader().UID;
	}

	void PacketManager::Flush(PacketTransceiver* pTransceiver)
	{
		int32 indexToUse = m_QueueIndex;
		m_QueueIndex = (m_QueueIndex + 1) % 2;
		std::queue<NetworkPacket*>& packets = m_MessagesToSend[indexToUse];

		Timestamp timestamp = EngineLoop::GetTimeSinceStart();

		while (!packets.empty())
		{
			Bundle bundle;
			uint32 bundleUID = pTransceiver->Transmit(&m_PacketPool, packets, bundle.ReliableUIDs, m_IPEndPoint, &m_Statistics);

			if (!bundle.ReliableUIDs.empty())
			{
				bundle.Timestamp = timestamp;

				std::scoped_lock<SpinLock> lock(m_LockBundles);
				m_Bundles.insert({ bundleUID, bundle });
			}
		}
	}

	void PacketManager::QueryBegin(PacketTransceiver* pTransceiver, TArray<NetworkPacket*>& packetsReturned)
	{
		TArray<NetworkPacket*> packets;
		IPEndPoint ipEndPoint;
		TArray<uint32> acks;

		if (!pTransceiver->ReceiveEnd(&m_PacketPool, packets, acks, &m_Statistics))
			return;

		packetsReturned.Clear();
		packetsReturned.Reserve(packets.GetSize());

		HandleAcks(acks);
		FindPacketsToReturn(packets, packetsReturned);
	}

	void PacketManager::QueryEnd(TArray<NetworkPacket*>& packetsReceived)
	{
		m_PacketPool.FreePackets(packetsReceived);
	}

	void PacketManager::Tick(Timestamp delta)
	{
		static const Timestamp delay = Timestamp::Seconds(1);

		m_Timer += delta;
		if (m_Timer >= delay)
		{
			m_Timer -= delay;
			DeleteOldBundles();
		}

		ResendOrDeleteMessages();
	}

	PacketPool* PacketManager::GetPacketPool()
	{
		return &m_PacketPool;
	}

	const NetworkStatistics* PacketManager::GetStatistics() const
	{
		return &m_Statistics;
	}

	const IPEndPoint& PacketManager::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void PacketManager::SetEndPoint(const IPEndPoint& ipEndPoint)
	{
		m_IPEndPoint = ipEndPoint;
	}

	void PacketManager::Reset()
	{
		std::scoped_lock<SpinLock> lock1(m_LockMessagesToSend);
		std::scoped_lock<SpinLock> lock2(m_LockBundles);
		m_MessagesToSend[0] = {};
		m_MessagesToSend[1] = {};
		m_MessagesWaitingForAck.clear();
		m_ReliableMessagesReceived.clear();
		m_Bundles.clear();

		m_PacketPool.Reset();
		m_Statistics.Reset();
		m_QueueIndex = 0;
	}

	void PacketManager::FindPacketsToReturn(const TArray<NetworkPacket*>& packetsReceived, TArray<NetworkPacket*>& packetsReturned)
	{
		bool reliableMessagesInserted = false;
		bool hasReliableMessage = false;

		TArray<NetworkPacket*> packetsToFree;
		packetsToFree.Reserve(32);

		for (NetworkPacket* pPacket : packetsReceived)
		{
			if (!pPacket->IsReliable())																//Unreliable Packet
			{
				if (pPacket->GetType() == NetworkPacket::TYPE_NETWORK_ACK)
					packetsToFree.PushBack(pPacket);
				else
					packetsReturned.PushBack(pPacket);
			}
			else
			{
				hasReliableMessage = true;

				if (pPacket->GetReliableUID() == m_Statistics.GetLastReceivedReliableUID() + 1)		//Reliable Packet in correct order
				{
					packetsReturned.PushBack(pPacket);
					m_Statistics.RegisterReliableMessageReceived();
				}
				else if (pPacket->GetReliableUID() > m_Statistics.GetLastReceivedReliableUID())		//Reliable Packet in incorrect order
				{
					m_ReliableMessagesReceived.insert(pPacket);
					reliableMessagesInserted = true;
				}
				else																				//Reliable Packet already received before
				{
					packetsToFree.PushBack(pPacket);
				}
			}
		}

		m_PacketPool.FreePackets(packetsToFree);

		if (reliableMessagesInserted)
			UntangleReliablePackets(packetsReturned);

		if (hasReliableMessage && m_MessagesToSend[m_QueueIndex].empty())
			EnqueuePacketUnreliable(m_PacketPool.RequestFreePacket()->SetType(NetworkPacket::TYPE_NETWORK_ACK));
	}

	void PacketManager::UntangleReliablePackets(TArray<NetworkPacket*>& packetsReturned)
	{
		TArray<NetworkPacket*> packetsToErase;

		for (NetworkPacket* pPacket : m_ReliableMessagesReceived)
		{
			if (pPacket->GetReliableUID() == m_Statistics.GetLastReceivedReliableUID() + 1)
			{
				packetsReturned.PushBack(pPacket);
				packetsToErase.PushBack(pPacket);
				m_Statistics.RegisterReliableMessageReceived();
			}
			else
			{
				break;
			}
		}

		for (NetworkPacket* pPacket : packetsToErase)
		{
			m_ReliableMessagesReceived.erase(pPacket);
		}
	}

	/*
	* Finds packets that have been sent erlier and are now acked.
	* Notifies the listener that the packet was succesfully delivered.
	* Removes the packet and returns it to the pool.
	*/
	void PacketManager::HandleAcks(const TArray<uint32>& acks)
	{
		TArray<uint32> ackedReliableUIDs;
		GetReliableUIDsFromAcks(acks, ackedReliableUIDs);

		TArray<MessageInfo> messagesAcked;
		GetReliableMessageInfosFromUIDs(ackedReliableUIDs, messagesAcked);

		TArray<NetworkPacket*> packetsToFree;
		packetsToFree.Reserve(messagesAcked.GetSize());

		for (MessageInfo& messageInfo : messagesAcked)
		{
			if (messageInfo.Listener)
			{
				messageInfo.Listener->OnPacketDelivered(messageInfo.Packet);
			}
			packetsToFree.PushBack(messageInfo.Packet);
		}

		m_PacketPool.FreePackets(packetsToFree);
	}

	void PacketManager::GetReliableUIDsFromAcks(const TArray<uint32>& acks, TArray<uint32>& ackedReliableUIDs)
	{
		ackedReliableUIDs.Reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockBundles);

		Timestamp timestamp = 0;

		for (uint32 ack : acks)
		{
			auto iterator = m_Bundles.find(ack);
			if (iterator != m_Bundles.end())
			{
				Bundle& bundle = iterator->second;
				for (uint32 UID : bundle.ReliableUIDs)
					ackedReliableUIDs.PushBack(UID);

				timestamp = bundle.Timestamp;
				m_Bundles.erase(iterator);
			}
		}

		if (timestamp != 0)
		{
			RegisterRTT(EngineLoop::GetTimeSinceStart() - timestamp);
		}
	}

	void PacketManager::GetReliableMessageInfosFromUIDs(const TArray<uint32>& ackedReliableUIDs, TArray<MessageInfo>& ackedReliableMessages)
	{
		ackedReliableMessages.Reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);

		for (uint32 UID : ackedReliableUIDs)
		{
			auto iterator = m_MessagesWaitingForAck.find(UID);
			if (iterator != m_MessagesWaitingForAck.end())
			{
				ackedReliableMessages.PushBack(iterator->second);
				m_MessagesWaitingForAck.erase(iterator);
			}
		}
	}

	void PacketManager::RegisterRTT(Timestamp rtt)
	{
		static const double scalar1 = 1.0f / 10.0f;
		static const double scalar2 = 1.0f - scalar1;
		m_Statistics.m_Ping = (uint64)((rtt.AsNanoSeconds() * scalar1) + (m_Statistics.GetPing().AsNanoSeconds() * scalar2));
	}

	void PacketManager::DeleteOldBundles()
	{
		Timestamp maxAllowedTime = m_Statistics.GetPing() * 100;
		Timestamp currentTime = EngineLoop::GetTimeSinceStart();

		TArray<uint32> bundlesToDelete;

		std::scoped_lock<SpinLock> lock(m_LockBundles);
		for (auto& pair : m_Bundles)
		{
			if (currentTime - pair.second.Timestamp > maxAllowedTime)
			{
				bundlesToDelete.PushBack(pair.first);
				m_Statistics.RegisterPacketLoss();
			}
		}

		for (uint32 UID : bundlesToDelete)
			m_Bundles.erase(UID);
	}

	void PacketManager::ResendOrDeleteMessages()
	{
		static Timestamp minTime = Timestamp::MilliSeconds(5);
		Timestamp maxAllowedTime = (uint64)((float64)m_Statistics.GetPing().AsNanoSeconds() * m_ResendRTTMultiplier);
		if (maxAllowedTime < minTime)
			maxAllowedTime = minTime;

		Timestamp currentTime = EngineLoop::GetTimeSinceStart();

		TArray<std::pair<const uint32, MessageInfo>> messagesToDelete;

		{
			std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);

			for (auto& pair : m_MessagesWaitingForAck)
			{
				MessageInfo& messageInfo = pair.second;
				if (currentTime - messageInfo.LastSent > maxAllowedTime)
				{
					messageInfo.Retries++;

					if (messageInfo.Retries < m_MaxRetries)
					{
						m_MessagesToSend[m_QueueIndex].push(messageInfo.Packet);
						messageInfo.LastSent = currentTime;

						if (messageInfo.Listener)
							messageInfo.Listener->OnPacketResent(messageInfo.Packet, messageInfo.Retries);
					}
					else
					{
						messagesToDelete.PushBack(pair);
					}
				}
			}

			for (auto& pair : messagesToDelete)
				m_MessagesWaitingForAck.erase(pair.first);
		}
		
		TArray<NetworkPacket*> packetsToFree;
		packetsToFree.Reserve(messagesToDelete.GetSize());

		for (auto& pair : messagesToDelete)
		{
			MessageInfo& messageInfo = pair.second;
			packetsToFree.PushBack(messageInfo.Packet);
			if (messageInfo.Listener)
				messageInfo.Listener->OnPacketMaxTriesReached(messageInfo.Packet, messageInfo.Retries);
		}

		m_PacketPool.FreePackets(packetsToFree);
	}
}
