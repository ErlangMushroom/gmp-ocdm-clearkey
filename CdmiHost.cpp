#include "CdmiHost.h"

namespace CDMi {

StaticRefPtr<CdmiHost> CdmiHost::sInstance;

static std::string
GetCdmiLibName()
{
  std::string
#if defined(XP_MACOSX)
  name = std::string("libocdmi.dylib");
#elif defined(XP_UNIX)
  name = std::string("libocdmi.so");
#elif defined(XP_WIN)
  name = std::string("ocdmi.dll");
#else
#error Do not supported yet!!
#endif
  return name;
}

const CdmiHost*
CdmiHost::GetInstance()
{
  if (!sInstance) {
    CdmiHost* host = new CdmiHost(GetCdmiLibName());
    if (!host->Init()) {
      delete host;
      return nullptr;
    }
    sInstance = host;
  }
  return sInstance.get();
}

}
