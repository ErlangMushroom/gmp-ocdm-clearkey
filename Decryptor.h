/*
 * Copyright (C) 2015 Acadine Technologies. All rights reserved.
 */

#ifndef __DECRYPTOR_H__
#define __DECRYPTOR_H__

#include "gmp-api/gmp-decryption.h"
#include "OCDMi.h"
#include "GMPAdapter.h"
#include <string>

using namespace CDMi;

namespace mozilla {

using namespace external;

enum SessionType {
  SessionType_Tempory,
  SessionType_Persistent
};

class Decryptor
  : public IMediaKeySessionCallback
  , public AtomicRefCounted<Decryptor> {
public:
  Decryptor(const RefPtr<ocdm_MediaKeys>& aMediaKeys,
            GMPDecryptorCallback* aCallback);

  virtual ~Decryptor();

  virtual void OnKeyMessage(const uint8_t* aKeyMessage,
                            uint32_t aKeyMessageLength,
                            char* aUrl) override;

  virtual void OnKeyReady() override;

  virtual void OnKeyError(int16_t aError,
                          CDMi_RESULT aSysError) override;

  virtual void OnKeyStatusUpdate(const char* aKeyMessage) override;

  CDMi_RESULT Init(uint32_t aCreateSessionToken,
                   const char* aInitDataType,
                   uint32_t aInitDataTypeSize,
                   const uint8_t* aInitData,
                   uint32_t aInitDataSize);

  CDMi_RESULT Finalize();

  CDMi_RESULT Decrypt(GMPBuffer* aBufferIn,
                      GMPEncryptedBufferMetadata* aMetadata,
                      GMPBuffer* aBufferOut);

  CDMi_RESULT Update(const uint8_t* aResponse,
                     uint32_t aResponseSize);

  CDMi_RESULT Close();

  std::string GetSessionId();

  SessionType GetSessionType();

private:
  RefPtr<ocdm_MediaKeys> mMediaKeys;
  GMPDecryptorCallback* mGMPCallback;
  IMediaKeySession* mMediaKeySesson;
  IMediaEngineSession* mMediaKeyEngine;
  std::string mSessionId;
};
 
} // mozilla

#endif // __DECRYPTOR_H__