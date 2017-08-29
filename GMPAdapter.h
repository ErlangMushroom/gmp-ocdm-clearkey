/*
 * Copyright (C) 2015 Acadine Technologies. All rights reserved.
 */

#ifndef __GMPADAPTERMANAGER_H__
#define __GMPADAPTERMANAGER_H__

#include "gmp-api/gmp-decryption.h"
#include "OCDMi.h"
#include "CdmiHost.h"
#include "RefCounted.h"
#include "RefPtr.h"
#include <map>
#include <string>

using namespace CDMi;

namespace mozilla {

using namespace external;

class ocdm_MediaKeys final
  : public IMediaKeys
  , public AtomicRefCounted<ocdm_MediaKeys> {
public:
  ocdm_MediaKeys()
    : mImpl(nullptr)
  {
    CDMi_RESULT rv;
    IMediaKeys* mediaKeys = nullptr;

    rv = CdmiHost::GetInstance()->CreateMediaKeys(&mediaKeys);
    if (CDMi_SUCCEEDED(rv)) {
      mImpl = mediaKeys;
    }
  }

  ~ocdm_MediaKeys()
  {
    if (mImpl) {
      CdmiHost::GetInstance()->DestroyMediaKeys(mImpl);
      mImpl = nullptr;
    }
  }

  virtual bool
  IsTypeSupported(const char* aMimeType,
                  const char* aKeySystem) const
  {
    return mImpl && mImpl->IsTypeSupported(aMimeType, aKeySystem);
  }

  virtual CDMi_RESULT
  CreateMediaKeySession(const char* aMimeType,
                        const uint8_t* aInitData,
                        uint32_t aInitDataLength,
                        const uint8_t* aCDMData,
                        uint32_t aCDMDataLength,
                        IMediaKeySession** aMediaKeySession)
  {
    return mImpl ? mImpl->CreateMediaKeySession(aMimeType,
                                                aInitData,
                                                aInitDataLength,
                                                aCDMData,
                                                aCDMDataLength,
                                                aMediaKeySession)
                 : CDMi_E_FAIL;
  }

  virtual CDMi_RESULT
  DestroyMediaKeySession(IMediaKeySession* aMediaKeySession)
  {
    return mImpl ? mImpl->DestroyMediaKeySession(aMediaKeySession)
                 : CDMi_E_FAIL;
  }

private:
  IMediaKeys* mImpl;
};

class Decryptor;
class GMPAdapterManager final
  : public GMPDecryptor
  , public AtomicRefCounted<GMPAdapterManager>
{
public:
  GMPAdapterManager();

  virtual ~GMPAdapterManager();

  virtual void Init(GMPDecryptorCallback* aCallback) override;

  virtual void CreateSession(uint32_t aCreateSessionToken,
                             uint32_t aPromiseId,
                             const char* aInitDataType,
                             uint32_t aInitDataTypeSize,
                             const uint8_t* aInitData,
                             uint32_t aInitDataSize,
                             GMPSessionType aSessionType) override;

  virtual void LoadSession(uint32_t aPromiseId,
                           const char* aSessionId,
                           uint32_t aSessionIdLength) override;

  virtual void UpdateSession(uint32_t aPromiseId,
                             const char* aSessionId,
                             uint32_t aSessionIdLength,
                             const uint8_t* aResponse,
                             uint32_t aResponseSize) override;

  virtual void CloseSession(uint32_t aPromiseId,
                            const char* aSessionId,
                            uint32_t aSessionIdLength) override;
  
  virtual void RemoveSession(uint32_t aPromiseId,
                             const char* aSessionId,
                             uint32_t aSessionIdLength) override;

  virtual void SetServerCertificate(uint32_t aPromiseId,
                                    const uint8_t* aServerCert,
                                    uint32_t aServerCertSize) override;

  virtual void Decrypt(GMPBuffer* aBuffer,
                       GMPEncryptedBufferMetadata* aMetadata) override;

  virtual void DecryptingComplete() override;

private:
  RefPtr<ocdm_MediaKeys> mMediaKeys;
  GMPDecryptorCallback* mCallback;
  std::map<std::string, RefPtr<Decryptor>> mSessions;

  static StaticRefPtr<CdmiHost> sCdmiHost;
};

}

#endif // __GMPADAPTERMANAGER_H__
