#pragma once

#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

#include <atomic>

namespace LambdaEngine
{
	class LAMBDA_API NetworkStatistics
	{
		friend class PacketTransceiver;
		friend class PacketManager;

	public:
		NetworkStatistics();
		~NetworkStatistics();

		/*
		* return - The number of physical packets sent
		*/
		uint32 GetPacketsSent()	const;

		/*
		* return - The number of packets (NetworkPacket) sent
		*/
		uint32 GetMessagesSent() const;

		/*
		* return - The number of reliable packets (NetworkPacket) sent
		*/
		uint32 GetReliableMessagesSent() const;

		/*
		* return - The number of physical packets received
		*/
		uint32 GetPacketsReceived()	const;

		/*
		* return - The number of packets (NetworkPacket) received
		*/
		uint32 GetMessagesReceived() const;

		/*
		* return - The number of physical packets lost
		*/
		uint32 GetPacketsLost()	const;

		/*
		* return - The percentage of physical packets lost 
		*/
		float64 GetPacketLossRate()	const;

		/*
		* return - The total number of bytes sent
		*/
		uint32 GetBytesSent() const;

		/*
		* return - The total number of bytes received
		*/
		uint32 GetBytesReceived() const;

		/*
		* return - The avarage roun trip time of the 10 latest physical packets
		*/
		const Timestamp& GetPing() const;

		/*
		* return - The unique salt representing this side of the connection
		*/
		uint64 GetSalt() const;

		/*
		* return - The unique salt representing the remote side of the connection
		*/
		uint64 GetRemoteSalt() const;

		/*
		* return - The timestamp of when the last physical packet was sent
		*/
		Timestamp GetTimestapLastSent() const;

		/*
		* return - The timestamp of when the last physical packet was received
		*/
		Timestamp GetTimestapLastReceived()	const;

		

		uint32 GetLastReceivedSequenceNr()	const;
		uint32 GetReceivedSequenceBits()	const;
		uint32 GetLastReceivedAckNr()		const;
		uint32 GetReceivedAckBits()			const;
		uint32 GetLastReceivedReliableUID()	const;

	private:
		void Reset();

		uint32 RegisterPacketSent();
		uint32 RegisterMessageSent();
		uint32 RegisterReliableMessageSent();
		void RegisterPacketReceived(uint32 messages, uint32 bytes);
		void RegisterReliableMessageReceived();
		void RegisterPacketLoss();
		void RegisterBytesSent(uint32 bytes);
		void SetRemoteSalt(uint64 salt);

		void SetLastReceivedSequenceNr(uint32 sequence);
		void SetReceivedSequenceBits(uint32 sequenceBits);
		void SetLastReceivedAckNr(uint32 ack);
		void SetReceivedAckBits(uint32 ackBits);

	private:
		uint32 m_PacketsLost;
		uint32 m_PacketsSent;
		uint32 m_MessagesSent;
		uint32 m_ReliableMessagesSent;
		uint32 m_PacketsReceived;
		uint32 m_MessagesReceived;
		uint32 m_BytesSent;
		uint32 m_BytesReceived;

		Timestamp m_Ping;
		Timestamp m_TimestampLastSent;
		Timestamp m_TimestampLastReceived;

		std::atomic_uint64_t m_Salt;
		std::atomic_uint64_t m_SaltRemote;

		std::atomic_uint32_t m_LastReceivedSequenceNr;
		std::atomic_uint32_t m_ReceivedSequenceBits;

		std::atomic_uint32_t m_LastReceivedAckNr;
		std::atomic_uint32_t m_ReceivedAckBits;

		std::atomic_uint32_t m_LastReceivedReliableUID;
	};
}