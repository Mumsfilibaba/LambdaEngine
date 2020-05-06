#pragma once
#include "Threading/API/SpinLock.h"

#ifdef LAMBDA_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4251) // Disable DLL linkage warning
#endif

namespace LambdaEngine
{
    class LAMBDA_API RefCountedObject
    {
    public:
        DECL_UNIQUE_CLASS(RefCountedObject);
        
        RefCountedObject();
        virtual ~RefCountedObject()  = default;
        
        uint64 AddRef();
        uint64 Release();
        
        FORCEINLINE uint64 GetRefCount() const
        {
            return m_StrongReferences;
        }
        
    private:
        uint64      m_StrongReferences = 0;
        SpinLock    m_Lock;
    };
}

#ifdef LAMBDA_VISUAL_STUDIO
    #pragma warning(pop)
#endif
