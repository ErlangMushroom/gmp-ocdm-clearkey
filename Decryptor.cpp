/*
 * Copyright (C) 2015 Acadine Technologies. All rights reserved.
 */

#include "Decryptor.h"
#include "OCDMiFix.h"
#include "CdmiHost.h"
#include <string.h>

#ifdef ANDROID
#define OCDM_LOG(x, ...) \
  __android_log_print(ANDROID_LOG_DEBUG, "[NFO-LOGGING]", "[Decryptor::%s][%d] " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define OCDM_LOG(x, ...) \
  printf("[NFO-LOGGING] [Decryptor::%s][%d] " x "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#define RETURN_IF_FAILED(rv) do { if (CDMi_FAILED(rv)) return (rv); } while (0)

namespace mozilla {

static GMPDOMException
MapGMPError(int16_t aError)
{
  // FIXME
  return kGMPNotSupportedError;
}

static uint32_t
MapGMPSystemCode(CDMi_RESULT aSysError)
{
  // FIXME
  return 0;
}

Decryptor::Decryptor(const RefPtr<ocdm_MediaKeys>& aMediaKeys,
                     GMPDecryptorCallback* aCallback)
  : mMediaKeys(aMediaKeys)
  , mGMPCallback(aCallback)
  , mMediaKeySesson(nullptr)
  , mMediaKeyEngine(nullptr)
{
  MOZ_ASSERT(mMediaKeys != nullptr && mGMPCallback != nullptr);
}

Decryptor::~Decryptor()
{
  Finalize();
}

void
Decryptor::OnKeyMessage(const uint8_t* aKeyMessage,
                        uint32_t aKeyMessageLength,
                        char* aUrl)
{
  OCDM_LOG("aKeyMessage(%s)", aKeyMessage);
  if (aKeyMessage) {
    mGMPCallback->SessionMessage(mSessionId.c_str(),
                                 mSessionId.length(),
                                 kGMPLicenseRequest,
                                 aKeyMessage,
                                 aKeyMessageLength);
  }
} 

void
Decryptor::OnKeyReady()
{
  OCDM_LOG("");
}

void
Decryptor::OnKeyError(int16_t aError,
                      CDMi_RESULT aSysError)
{
  OCDM_LOG("");
  mGMPCallback->SessionError(mSessionId.c_str(),
                             mSessionId.length(),
                             MapGMPError(aError),/*GMPDOMException aException*/
                             MapGMPSystemCode(aSysError),
                             nullptr, /* aMessage */
                             0 /* aMessageLength*/);
}

void
Decryptor::OnKeyStatusUpdate(const char* aKeyMessage)
{
  OCDM_LOG("aKeyMessage(%s)", aKeyMessage);
  OCDMi::fix::KeyMessageParser parser;
  if (!parser.Parse(reinterpret_cast<const uint8_t*>(aKeyMessage),
                    strlen(aKeyMessage))) {
    return;
  }

  std::vector<uint8_t> keyId = parser.GetKeyID();
  mGMPCallback->KeyStatusChanged(mSessionId.c_str(),
                                 mSessionId.length(),
                                 &keyId[0],
                                 keyId.size(),
                                 kGMPUsable);
}

CDMi_RESULT
Decryptor::Init(uint32_t aCreateSessionToken,
                const char* aInitDataType,
                uint32_t aInitDataTypeSize,
                const uint8_t* aInitData,
                uint32_t aInitDataSize)
{
  CDMi_RESULT rv;
  IMediaKeySession* keySession = nullptr;
  IMediaEngineSession* keyEngine = nullptr;

  // FIXME : make sure of arguments
  rv = mMediaKeys->CreateMediaKeySession(nullptr, // MimeType
                                         aInitData,
                                         aInitDataSize,
                                         nullptr, // CDMData
                                         0,       // CDMDataLen
                                         &keySession);
  RETURN_IF_FAILED(rv);
  MOZ_ASSERT(keySession);

  rv = CdmiHost::GetInstance()->
         CreateMediaEngineSession(keySession, &keyEngine);
  if (CDMi_FAILED(rv)) {
    mMediaKeys->DestroyMediaKeySession(keySession);
    return rv;
  }
  MOZ_ASSERT(keyEngine);

  mSessionId = keySession->GetSessionId();
  mGMPCallback->SetSessionId(aCreateSessionToken,
                             mSessionId.c_str(),
                             mSessionId.length());

  mMediaKeySesson = keySession;
  mMediaKeyEngine = keyEngine;

  mMediaKeySesson->RunAndGetLicenceChallange(this);

  /* OCDMi(clearkey) really need to do following things */
  OCDMi::fix::RequestComposer(this).
    GenerateRequest(aInitData, aInitDataSize);

  return CDMi_SUCCESS;
}

CDMi_RESULT
Decryptor::Finalize()
{
  if (mMediaKeyEngine) {
    CdmiHost::GetInstance()->DestroyMediaEngineSession(mMediaKeyEngine);
    mMediaKeyEngine = nullptr;
  }
  if (mMediaKeySesson) {
    mMediaKeys->DestroyMediaKeySession(mMediaKeySesson);
    mMediaKeySesson = nullptr;
  }
  return CDMi_SUCCESS;
}

CDMi_RESULT
Decryptor::Decrypt(GMPBuffer* aBufferIn,
                   GMPEncryptedBufferMetadata* aMetadata,
                   GMPBuffer* aBufferOut)
{
  OCDM_LOG("this(%p) aBuffer(%d) IV(%d) NumSubsamples(%d)",
    this, aBufferIn->Size(), aMetadata->IVSize(), aMetadata->NumSubsamples());

  if (!mMediaKeyEngine) {
    return CDMi_E_FAIL;
  }

  std::vector<uint8_t> encBytes(aBufferIn->Size());

  if (aMetadata->NumSubsamples()) {
    unsigned char* data = aBufferIn->Data();
    unsigned char* iter = &encBytes[0];
    for (size_t i = 0; i < aMetadata->NumSubsamples(); i++) {
      data += aMetadata->ClearBytes()[i];
      uint32_t cipherBytes = aMetadata->CipherBytes()[i];

      memcpy(iter, data, cipherBytes);

      data += cipherBytes;
      iter += cipherBytes;
    }

    encBytes.resize((size_t)(iter - &encBytes[0]));
  } else {
    memcpy(&encBytes[0], aBufferIn->Data(), aBufferIn->Size());
  }

  uint8_t* decrypted = nullptr;
  uint32_t decryptedLen = 0;

  if (encBytes.size()) {
    mMediaKeyEngine->Decrypt(aMetadata->NumSubsamples(),
                             aMetadata->CipherBytes(),
                             aMetadata->IVSize(),
                             aMetadata->IV(),
                             encBytes.size(),
                             &encBytes[0],
                             &decryptedLen,
                             &decrypted);
  }

  GMPBuffer* out = aBufferOut ? aBufferOut : aBufferIn;

  if (aMetadata->NumSubsamples()) {
    unsigned char* data = out->Data();
    unsigned char* iter = decrypted;
    for (size_t i = 0; i < aMetadata->NumSubsamples(); i++) {
      data += aMetadata->ClearBytes()[i];
      uint32_t cipherBytes = aMetadata->CipherBytes()[i];

      memcpy(data, iter, cipherBytes);

      data += cipherBytes;
      iter += cipherBytes;
    }
  } else {
    memcpy(out->Data(), decrypted, out->Size());
  }

  if (decrypted) {
    free(decrypted);
  }

  return CDMi_SUCCESS;
}

CDMi_RESULT
Decryptor::Update(const uint8_t* aResponse,
                  uint32_t aResponseSize)
{
  if (mMediaKeySesson) {
    mMediaKeySesson->Update(aResponse, aResponseSize);
    return CDMi_SUCCESS;
  }

  return CDMi_E_FAIL;
}

CDMi_RESULT
Decryptor::Close()
{
  if (mMediaKeySesson) {
    mMediaKeySesson->Close();
    return CDMi_SUCCESS;
  }

  return CDMi_E_FAIL;
}

std::string
Decryptor::GetSessionId()
{
  return mSessionId;
}

SessionType
Decryptor::GetSessionType()
{
  return SessionType_Tempory;
}

}