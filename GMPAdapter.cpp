/*
 * Copyright (C) 2015 Acadine Technologies. All rights reserved.
 */

#include "GMPAdapter.h"
#include "Decryptor.h"

#ifdef ANDROID
#define OCDM_LOG(x, ...) \
  __android_log_print(ANDROID_LOG_DEBUG, "[NFO-LOGGING]", "[GMPAdapterManager::%s][%d] " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define OCDM_LOG(x, ...) \
  printf("[NFO-LOGGING] [GMPAdapterManager::%s][%d] " x "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

namespace mozilla {

GMPAdapterManager::GMPAdapterManager()
  : mMediaKeys(new ocdm_MediaKeys())
{
}

GMPAdapterManager::~GMPAdapterManager()
{
}

void
GMPAdapterManager::Init(GMPDecryptorCallback* aCallback)
{
  OCDM_LOG("");
  if (aCallback) {
    mCallback = aCallback;
    if (mMediaKeys) {
      mCallback->SetCapabilities(GMP_EME_CAP_DECRYPT_AUDIO |
                                 GMP_EME_CAP_DECRYPT_VIDEO);
    }
  }
}

void
GMPAdapterManager::CreateSession(uint32_t aCreateSessionToken,
                                 uint32_t aPromiseId,
                                 const char* aInitDataType,
                                 uint32_t aInitDataTypeSize,
                                 const uint8_t* aInitData,
                                 uint32_t aInitDataSize,
                                 GMPSessionType aSessionType)
{
  OCDM_LOG("");
  MOZ_ASSERT(mCallback != nullptr);

  if (!mMediaKeys->IsTypeSupported(nullptr, aInitDataType)) {
    OCDM_LOG("Type(%s) not supported!", aInitDataType);
    mCallback->RejectPromise(aPromiseId,
                             kGMPInvalidStateError,
                             nullptr,
                             0);
    return;
  }

  RefPtr<Decryptor> decryptor = new Decryptor(mMediaKeys, mCallback);
  CDMi_RESULT rv = decryptor->Init(aCreateSessionToken,
                                   aInitDataType,
                                   aInitDataTypeSize,
                                   aInitData,
                                   aInitDataSize
                                   /* aSessionType */);
  if (CDMi_FAILED(rv)) {
    OCDM_LOG("Decryptor init failed!");
    mCallback->RejectPromise(aPromiseId,
                             kGMPInvalidStateError,
                             nullptr,
                             0);
    return;
  }

  std::string sessionId = decryptor->GetSessionId();
  OCDM_LOG("New session id(%s)", sessionId.c_str());

  MOZ_ASSERT(mSessions.find(sessionId) == mSessions.end());
  mSessions[sessionId] = decryptor;

  mCallback->ResolvePromise(aPromiseId);
}

void
GMPAdapterManager::LoadSession(uint32_t aPromiseId,
                               const char* aSessionId,
                               uint32_t aSessionIdLength)
{
  OCDM_LOG("");
  /*Dont support persistent session yet*/
  MOZ_ASSERT(mCallback != nullptr);
  mCallback->ResolveLoadSessionPromise(aPromiseId, false);
}

void
GMPAdapterManager::UpdateSession(uint32_t aPromiseId,
                                 const char* aSessionId,
                                 uint32_t aSessionIdLength,
                                 const uint8_t* aResponse,
                                 uint32_t aResponseSize)
{
  OCDM_LOG("aSessionId(%s)", aSessionId);
  MOZ_ASSERT(mCallback != nullptr);

  auto it = mSessions.find(std::string(aSessionId, aSessionIdLength));
  if (it == mSessions.end()) {
    OCDM_LOG("Cannot find session id(%s)", aSessionId);
    mCallback->ResolvePromise(aPromiseId);
  } else {
    CDMi_RESULT rv = it->second->Update(aResponse, aResponseSize);
    if (CDMi_FAILED(rv)) {
      OCDM_LOG("Update failed, session id(%s)", aSessionId);
      mCallback->RejectPromise(aPromiseId,
                               kGMPInvalidStateError,
                               nullptr,
                               0);
    }
  }
}

void
GMPAdapterManager::CloseSession(uint32_t aPromiseId,
                                const char* aSessionId,
                                uint32_t aSessionIdLength)
{
  OCDM_LOG("");
  MOZ_ASSERT(mCallback != nullptr);

  std::string sessionId = std::string(aSessionId, aSessionIdLength);
  auto it = mSessions.find(sessionId);
  if (it == mSessions.end()) {
    OCDM_LOG("Cannot find session id(%s)", aSessionId);
    mCallback->ResolveLoadSessionPromise(aPromiseId, false);
  } else {
    CDMi_RESULT rv = it->second->Close();
    if (CDMi_FAILED(rv)) {
      OCDM_LOG("Close failed, session id(%s)", aSessionId);
      mCallback->RejectPromise(aPromiseId,
                               kGMPInvalidStateError,
                               nullptr,
                               0);
    }
    mSessions.erase(sessionId);
  }
}

void
GMPAdapterManager::RemoveSession(uint32_t aPromiseId,
                                 const char* aSessionId,
                                 uint32_t aSessionIdLength)
{
  OCDM_LOG("");
  /*Doesnt support persistent session yet*/
  MOZ_ASSERT(mCallback);
  mCallback->ResolvePromise(aPromiseId);
}

void
GMPAdapterManager::SetServerCertificate(uint32_t aPromiseId,
                                        const uint8_t* aServerCert,
                                        uint32_t aServerCertSize)
{
  OCDM_LOG("");
  // FIXME : find out about how SetServerCertificate is implemented in OCDM
  /*
  if (!mMediaKeys->SetServerCertificate(aServerCert, aServerCertSize)) {
    mCallback->RejectPromise(aPromiseId,
                             kGMPNotSupportedError,
                             nullptr,
                             0);
    return;
  }
  */
  mCallback->ResolvePromise(aPromiseId);
}

void
GMPAdapterManager::Decrypt(GMPBuffer* aBuffer,
                           GMPEncryptedBufferMetadata* aMetadata)
{
  OCDM_LOG("");
  RefPtr<Decryptor> decryptor;
  const GMPStringList* stringList = aMetadata->SessionIds();

  for (uint32_t i = 0; i < stringList->Size(); i++) {
    const char* string = nullptr;
    uint32_t stringLen = 0;
    stringList->StringAt(i, &string, &stringLen);

    auto it = mSessions.find(std::string(string, stringLen));
    if (it != mSessions.end()) {
      decryptor = it->second;
      break;
    }
  }

  if (decryptor &&
      CDMi_SUCCEEDED(decryptor->Decrypt(aBuffer, aMetadata, nullptr))) {
    mCallback->Decrypted(aBuffer, GMPNoErr);
  } else {
    OCDM_LOG("Decrypt failed, decryptor(%p)", decryptor.get());
    mCallback->Decrypted(aBuffer, GMPGenericErr);
  }
}

void
GMPAdapterManager::DecryptingComplete()
{
  OCDM_LOG("");
  mSessions.clear();
}

}