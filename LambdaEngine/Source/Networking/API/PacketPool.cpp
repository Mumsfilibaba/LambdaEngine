#include "Networking/API/PacketPool.h"
#include "Networking/API/NetworkPacket.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	PacketPool::PacketPool(uint16 size)
	{
		m_Packets.Reserve(size);
		m_PacketsFree.Reserve(size);

		for (uint16 i = 0; i < size; i++)
		{
			NetworkPacket* pPacket = new NetworkPacket();
			m_Packets.PushBack(pPacket);
			m_PacketsFree.PushBack(pPacket);
		}		
	}

	PacketPool::~PacketPool()
	{
		for (uint16 i = 0; i < m_Packets.GetSize(); i++)
			delete m_Packets[i];

		m_Packets.Clear();
	}

	NetworkPacket* PacketPool::RequestFreePacket()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		NetworkPacket* pPacket = nullptr;
		if (!m_PacketsFree.IsEmpty())
		{
			pPacket = m_PacketsFree[m_PacketsFree.GetSize() - 1];
			m_PacketsFree.PopBack();

#ifndef LAMBDA_CONFIG_PRODUCTION
			Request(pPacket);
#endif
		}
		else
		{
			LOG_ERROR("[PacketPool]: No more free packets!, delta = -1");
		}
		return pPacket;
	}

	bool PacketPool::RequestFreePackets(uint16 nrOfPackets, TArray<NetworkPacket*>& packetsReturned)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		int32 delta = (int32)m_PacketsFree.GetSize() - nrOfPackets;

		if (delta < 0)
		{
			LOG_ERROR("[PacketPool]: No more free packets!, delta = %d", delta);
			return false;
		}

		packetsReturned = TArray<NetworkPacket*>(m_PacketsFree.begin() + delta, m_PacketsFree.end());
		m_PacketsFree = TArray<NetworkPacket*>(m_PacketsFree.begin(), m_PacketsFree.begin() + delta);

#ifndef LAMBDA_CONFIG_PRODUCTION
		for (int32 i = 0; i < nrOfPackets; i++)
		{
			Request(packetsReturned[i]);
		}
#endif

		return true;
	}

	void PacketPool::FreePacket(NetworkPacket* pPacket)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		Free(pPacket);
	}

	void PacketPool::FreePackets(TArray<NetworkPacket*>& packets)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		for (NetworkPacket* pPacket : packets)
		{
			Free(pPacket);
		}
		packets.Clear();
	}

	void PacketPool::Request(NetworkPacket* pPacket)
	{
		pPacket->m_IsBorrowed = true;

#ifdef DEBUG_PACKET_POOL
		LOG_MESSAGE("[PacketPool]: Lending [%x]", pPacket);
#endif
	}

	void PacketPool::Free(NetworkPacket* pPacket)
	{
#ifdef DEBUG_PACKET_POOL
		LOG_MESSAGE("[PacketPool]: Freeing [%x]%s", pPacket, pPacket->ToString().c_str());
#endif

#ifndef LAMBDA_CONFIG_PRODUCTION
		if (pPacket->m_IsBorrowed)
		{
			pPacket->m_IsBorrowed = false;
		}
		else
		{
			LOG_ERROR("[PacketPool]: Packet was returned multiple times!");
			DEBUGBREAK();
		}
#endif

		pPacket->m_SizeOfBuffer = 0;
		m_PacketsFree.PushBack(pPacket);
	}

	void PacketPool::Reset()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_PacketsFree.Clear();
		m_PacketsFree.Reserve(m_Packets.GetSize());

		for (NetworkPacket* pPacket : m_Packets)
		{
#ifndef LAMBDA_CONFIG_PRODUCTION
			pPacket->m_IsBorrowed = false;
#endif
			pPacket->m_SizeOfBuffer = 0;
			m_PacketsFree.PushBack(pPacket);
		}
	}

	uint16 PacketPool::GetSize() const
	{
		return (uint16)m_Packets.GetSize();
	}

	uint16 PacketPool::GetFreePackets() const
	{
		return (uint16)m_PacketsFree.GetSize();
	}
}