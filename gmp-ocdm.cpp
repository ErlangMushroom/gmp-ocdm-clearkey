/*
 * Copyright (C) 2015 Acadine Technologies. All rights reserved.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "GMPAdapter.h"
#include "gmp-api/gmp-async-shutdown.h"
#include "gmp-api/gmp-decryption.h"
#include "gmp-api/gmp-platform.h"

#define GMP_EXPORT __attribute__((visibility("default")))

using namespace mozilla;

static GMPPlatformAPI* sPlatform = nullptr;

GMPPlatformAPI*
GetPlatform()
{
  return sPlatform;
}

extern "C" {

GMP_EXPORT GMPErr
GMPInit(GMPPlatformAPI* aPlatformAPI)
{
  printf("[NFO-LOGGING] OCDM GMPInit\n");
  sPlatform = aPlatformAPI;
  CDMi::CdmiHost::GetInstance();
  return GMPNoErr;
}

GMP_EXPORT GMPErr
GMPGetAPI(const char* aApiName, void* aHostAPI, void** aPluginAPI)
{
  assert(!*aPluginAPI);

  if (!strcmp(aApiName, GMP_API_DECRYPTOR)) {
    *aPluginAPI = new GMPAdapterManager();
  }

  printf("[NFO-LOGGING] OCDM GMPGetAPI(%p)\n", *aPluginAPI);
  return *aPluginAPI ? GMPNoErr : GMPNotImplementedErr;
}

GMP_EXPORT GMPErr
GMPShutdown(void)
{
  printf("[NFO-LOGGING] OCDM GMPShutdown\n");
  sPlatform = nullptr;
  return GMPNoErr;
}

}
