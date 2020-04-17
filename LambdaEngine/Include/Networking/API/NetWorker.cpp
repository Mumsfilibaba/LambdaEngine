#include "NetWorker.h"

#include "Log/Log.h"

#include "Threading/API/Thread.h"

namespace LambdaEngine
{
	NetWorker::NetWorker() : 
		m_pThreadReceiver(nullptr),
		m_pThreadTransmitter(nullptr),
		m_Run(false),
		m_ThreadsStarted(false),
		m_ReceiverStopped(false),
		m_Initiated(false),
		m_ThreadsTerminated(false),
		m_Release(false),
		m_pReceiveBuffer(),
		m_pSendBuffer()
	{

	}

	NetWorker::~NetWorker()
	{
		if (!m_Release)
			LOG_ERROR("[NetWorker]: Do not use delete on a NetWorker object. Use the Release() function!");
		else
			LOG_INFO("[NetWorker]: Released");
	}

	void NetWorker::Flush()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pThreadTransmitter)
			m_pThreadTransmitter->Notify();
	}

	void NetWorker::Release()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (!m_Release)
		{
			m_Release = true;
			TerminateThreads();
			OnReleaseRequested();
		}

		if (m_ThreadsTerminated)
			delete this;
	}

	void NetWorker::TerminateThreads()
	{
		if (m_Run)
		{
			m_Run = false;
			OnTerminationRequested();
		}
	}

	bool NetWorker::ThreadsAreRunning() const
	{
		return m_pThreadReceiver != nullptr && m_pThreadTransmitter != nullptr;
	}

	bool NetWorker::ShouldTerminate() const
	{
		return !m_Run;
	}

	void NetWorker::YieldTransmitter()
	{
		if (m_pThreadTransmitter)
			m_pThreadTransmitter->Wait();
	}

	bool NetWorker::StartThreads()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (!ThreadsAreRunning())
		{
			m_Run = true;
			m_ThreadsStarted = false;
			m_Initiated = false;
			m_ThreadsTerminated = false;

			m_pThreadTransmitter = Thread::Create(
				std::bind(&NetWorker::ThreadTransmitter, this),
				std::bind(&NetWorker::ThreadTransmitterDeleted, this)
			);

			m_pThreadReceiver = Thread::Create(
				std::bind(&NetWorker::ThreadReceiver, this),
				std::bind(&NetWorker::ThreadReceiverDeleted, this)
			);
			m_ThreadsStarted = true;
			return true;
		}
		return false;
	}

	void NetWorker::ThreadTransmitter()
	{
		while (!m_ThreadsStarted);
		if (!OnThreadsStarted())
			TerminateThreads();

		m_Initiated = true;

		RunTranmitter();

		while (!m_ReceiverStopped);
	}

	void NetWorker::ThreadReceiver()
	{
		while (!m_Initiated);

		RunReceiver();

		Flush();
		m_ReceiverStopped = true;
	}

	void NetWorker::ThreadTransmitterDeleted()
	{
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			m_pThreadTransmitter = nullptr;
		}

		if (!m_pThreadReceiver)
			ThreadsDeleted();
	}

	void NetWorker::ThreadReceiverDeleted()
	{
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			m_pThreadReceiver = nullptr;
		}

		if (!m_pThreadTransmitter)
			ThreadsDeleted();
	}

	void NetWorker::ThreadsDeleted()
	{
		OnThreadsTurminated();
		m_ThreadsTerminated = true;

		if (m_Release)
			Release();
	}
}