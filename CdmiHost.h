#ifndef __CDMI_HOST_H__
#define __CDMI_HOST_H__

#include "RefCounted.h"
#include "OCDMi.h"
#include "StaticPtr.h"
#include <string>
#include <dlfcn.h>

namespace CDMi {

typedef CDMi_RESULT (*ocdm_CreateMediaKeys) (IMediaKeys **);
typedef CDMi_RESULT (*ocdm_DestroyMediaKeys) (IMediaKeys *);
typedef CDMi_RESULT (*ocdm_CreateMediaEngineSession) (IMediaKeySession *, IMediaEngineSession **);
typedef CDMi_RESULT (*ocdm_DestroyMediaEngineSession) (IMediaEngineSession *);

using mozilla::external::AtomicRefCounted;
using mozilla::StaticRefPtr;

class CdmiHost final : public AtomicRefCounted<CdmiHost> {
public:

  static StaticRefPtr<CdmiHost> sInstance;

  static const CdmiHost* GetInstance();

  bool Init()
  {
  #define RETURN_IF_FALSE(expr) do { if(!(expr)) return false; } while (0)
    RETURN_IF_FALSE(mHandle.HookFn(mFnCreateMediaKeys, "CreateMediaKeys"));
    RETURN_IF_FALSE(mHandle.HookFn(mFnDestroyMediaKeys, "DestroyMediaKeys"));
    RETURN_IF_FALSE(mHandle.HookFn(mFnCreateMediaEngineSession, "CreateMediaEngineSession"));
    RETURN_IF_FALSE(mHandle.HookFn(mFnDestroyMediaEngineSession, "DestroyMediaEngineSession"));
  #undef RETURN_IF_FALSE
    return true;
  }

  CDMi_RESULT
  CreateMediaKeys(IMediaKeys** aMediaKeys) const
  {
    return mHandle.IsLoaded() ? mFnCreateMediaKeys(aMediaKeys)
                              : CDMi_E_FAIL;
  }

  CDMi_RESULT
  DestroyMediaKeys(IMediaKeys* aMediaKeys) const
  {
    return mHandle.IsLoaded() ? mFnDestroyMediaKeys(aMediaKeys)
                              : CDMi_E_FAIL;
  }

  CDMi_RESULT
  CreateMediaEngineSession(IMediaKeySession* aMediaKeySession,
                           IMediaEngineSession** aMediaEngineSession) const
  {
    return mHandle.IsLoaded() ? mFnCreateMediaEngineSession(aMediaKeySession,
                                                            aMediaEngineSession)
                              : CDMi_E_FAIL;
  }

  CDMi_RESULT
  DestroyMediaEngineSession(IMediaEngineSession* aMediaEngineSession) const
  {
    return mHandle.IsLoaded() ? mFnDestroyMediaEngineSession(aMediaEngineSession)
                              : CDMi_E_FAIL;
  }

protected:
  CdmiHost(const std::string& path)
    : mHandle(path)
    , mFnCreateMediaKeys(nullptr)
    , mFnDestroyMediaKeys(nullptr)
    , mFnCreateMediaEngineSession(nullptr)
    , mFnDestroyMediaEngineSession(nullptr)
  {
  }

private:
  CdmiHost(const CdmiHost&) = delete;
  CdmiHost& operator=(const CdmiHost&) = delete;

  class DynamicLibHandle {
  public:
    DynamicLibHandle(const std::string& aPath)
      : handle(nullptr)
    {
      handle = dlopen(aPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
    }

    ~DynamicLibHandle()
    {
      if (handle) {
        dlclose(handle);
      }
    }

    bool IsLoaded() const { return !!handle; }

    template <typename T>
    bool HookFn(T& aFn, const std::string& aFnName) const
    {
      if (handle) {
        void* ret = dlsym(handle, aFnName.c_str());
        if (ret && !dlerror()) {
          aFn = reinterpret_cast<T>(ret);
          return true;
        }
      }
      return false;
    }

  private:
    void* handle;
  };

  DynamicLibHandle mHandle;

  ocdm_CreateMediaKeys mFnCreateMediaKeys;
  ocdm_DestroyMediaKeys mFnDestroyMediaKeys;
  ocdm_CreateMediaEngineSession mFnCreateMediaEngineSession;
  ocdm_DestroyMediaEngineSession mFnDestroyMediaEngineSession;
};

}

#endif // __CDMI_HOST_H__