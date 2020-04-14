#pragma once
#include "IDeviceChild.h"

#include "Threading/API/SpinLock.h"

#include <mutex>

#include <string.h>

#define MAX_DEVICE_CHILD_NAME_LENGTH 256

namespace LambdaEngine
{
	template <typename TGraphicsDevice, typename IBase>
	class TDeviceChildBase : public IBase
	{
	public:
		TDeviceChildBase(const TGraphicsDevice* pDevice)
			: IBase(),
			m_pDevice(pDevice)
		{
			ZERO_MEMORY(m_DebugName, sizeof(m_DebugName));

			AddRef();
		}

		virtual ~TDeviceChildBase()
		{
			m_StrongReferences	= 0;
		}

		virtual uint64 Release() override
		{
            uint64 strongReferences;
            {
                std::scoped_lock<SpinLock> lock(m_Lock);
                strongReferences = --m_StrongReferences;
            }
            
			if (strongReferences < 1)
			{
				delete this;
			}

			return strongReferences;
		}

		virtual uint64 AddRef() override
		{
            std::scoped_lock<SpinLock> lock(m_Lock);
			return ++m_StrongReferences;
		}
        
		virtual void SetName(const char* pName) override
		{
			strncpy(m_DebugName, pName, sizeof(m_DebugName));
		}

        FORCEINLINE virtual const IGraphicsDevice* GetDevice() const override
        {
            //Cast the device to the correct type, this way we do not actually need to include any implementation.
            //Not the prettiest solution but it works
            return reinterpret_cast<const IGraphicsDevice*>(m_pDevice);
        }

	protected:
		const TGraphicsDevice* const	m_pDevice = nullptr;
		char							m_DebugName[MAX_DEVICE_CHILD_NAME_LENGTH];

	private:
        SpinLock    m_Lock;
		uint64      m_StrongReferences = 0;
	};
}
