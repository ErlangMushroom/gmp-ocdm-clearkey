/*
 * Copyright (C) 2015 Acadine Technologies. All rights reserved.
 */

#ifndef __MOZILLA_OCDMIFIX_H__
#define __MOZILLA_OCDMIFIX_H__

#include "gmp-api/gmp-decryption.h"
#include "OCDMi.h"
#include "ArrayUtils.h"
#include <string.h>
#include <vector>
#include <memory>

namespace OCDMi {
namespace fix {

#define FOURCC(a,b,c,d) ((a << 24) + (b << 16) + (c << 8) + d)
#define CLEARKEY_KEY_LEN ((size_t)16)

typedef std::vector<uint8_t> Bytes;

const uint8_t kSystemID[] = {
  0x10, 0x77, 0xef, 0xec, 0xc0, 0xb2, 0x4d, 0x02,
  0xac, 0xe3, 0x3c, 0x1e, 0x52, 0xe2, 0xfb, 0x4b
};

class BigEndian {
public:
  static uint32_t readUint32(const void* aPtr) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(aPtr);
    return uint32_t(p[0]) << 24 |
           uint32_t(p[1]) << 16 |
           uint32_t(p[2]) << 8 |
           uint32_t(p[3]);
  }

  static uint16_t readUint16(const void* aPtr) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(aPtr);
    return uint32_t(p[0]) << 8 |
           uint32_t(p[1]);
  }

  static uint64_t readUint64(const void* aPtr) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(aPtr);
    return uint64_t(p[0]) << 56 |
           uint64_t(p[1]) << 48 |
           uint64_t(p[2]) << 40 |
           uint64_t(p[3]) << 32 |
           uint64_t(p[4]) << 24 |
           uint64_t(p[5]) << 16 |
           uint64_t(p[6]) << 8 |
           uint64_t(p[7]);
  }

  static void writeUint64(void* aPtr, uint64_t aValue) {
    uint8_t* p = reinterpret_cast<uint8_t*>(aPtr);
    p[0] = uint8_t(aValue >> 56) & 0xff;
    p[1] = uint8_t(aValue >> 48) & 0xff;
    p[2] = uint8_t(aValue >> 40) & 0xff;
    p[3] = uint8_t(aValue >> 32) & 0xff;
    p[4] = uint8_t(aValue >> 24) & 0xff;
    p[5] = uint8_t(aValue >> 16) & 0xff;
    p[6] = uint8_t(aValue >> 8) & 0xff;
    p[7] = uint8_t(aValue) & 0xff;
  }
};

class RequestComposer {
public:
  RequestComposer(IMediaKeySessionCallback* aCallback)
    : mCallback(aCallback)
  {
    MOZ_ASSERT(mCallback != nullptr);
  }

  static const char*
  SessionTypeToString(GMPSessionType aSessionType)
  {
    switch (aSessionType) {
      case kGMPTemporySession: return "temporary";
      case kGMPPersistentSession: return "persistent";
      default: {
        MOZ_ASSERT(false); // Should not reach here.
        return "invalid";
      }
    }
  }

  static bool
  EncodeBase64Web(std::vector<uint8_t> aBinary, std::string& aEncoded)
  {
    const char sAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    const uint8_t sMask = 0x3f;

    aEncoded.resize((aBinary.size() * 8 + 5) / 6);

    // Pad binary data in case there's rubbish past the last byte.
    aBinary.push_back(0);

    // Number of bytes not consumed in the previous character
    uint32_t shift = 0;

    auto out = aEncoded.begin();
    auto data = aBinary.begin();
    for (std::string::size_type i = 0; i < aEncoded.length(); i++) {
      if (shift) {
        out[i] = (*data << (6 - shift)) & sMask;
        data++;
      } else {
        out[i] = 0;
      }

      out[i] += (*data >> (shift + 2)) & sMask;
      shift = (shift + 2) % 8;

      // Cast idx to size_t before using it as an array-index,
      // to pacify clang 'Wchar-subscripts' warning:
      size_t idx = static_cast<size_t>(out[i]);
      MOZ_ASSERT(idx < MOZ_ARRAY_LENGTH(sAlphabet)); // out of bounds index for 'sAlphabet'
      out[i] = sAlphabet[idx];
    }

    return true;
  }

  static void
  ParseInitData(const uint8_t* aInitData,
                uint32_t aInitDataSize,
                std::vector<Bytes>& aOutKeys)
  {
    uint32_t size = 0;
    for (uint32_t offset = 0; offset + sizeof(uint32_t) < aInitDataSize; offset += size) {
      const uint8_t* data = aInitData + offset;
      size = BigEndian::readUint32(data); data += sizeof(uint32_t);

      if (size + offset > aInitDataSize) {
        return;
      }

      if (size < 36) {
        continue;
      }

      uint32_t box = BigEndian::readUint32(data); data += sizeof(uint32_t);
      if (box != FOURCC('p','s','s','h')) {
        return;
      }

      uint32_t head = BigEndian::readUint32(data); data += sizeof(uint32_t);
      if ((head >> 24) != 1) {
        continue;
      }

      if (memcmp(kSystemID, data, sizeof(kSystemID))) {
        continue;
      }
      data += sizeof(kSystemID);

      uint32_t kidCount = BigEndian::readUint32(data); data += sizeof(uint32_t);
      if (data + kidCount * CLEARKEY_KEY_LEN > aInitData + aInitDataSize) {
        return;
      }

      for (uint32_t i = 0; i < kidCount; i++) {
        aOutKeys.push_back(Bytes(data, data + CLEARKEY_KEY_LEN));
        data += CLEARKEY_KEY_LEN;
      }
    }
  }

  static void
  MakeKeyRequest(const std::vector<Bytes>& aKeyIDs,
                 std::string& aOutRequest,
                 GMPSessionType aSessionType)
  {
    MOZ_ASSERT(aKeyIDs.size() && aOutRequest.empty());

    aOutRequest.append("{ \"kids\":[");
    for (size_t i = 0; i < aKeyIDs.size(); i++) {
      if (i) {
        aOutRequest.append(",");
      }
      aOutRequest.append("\"");

      std::string base64key;
      EncodeBase64Web(aKeyIDs[i], base64key);
      aOutRequest.append(base64key);

      aOutRequest.append("\"");
    }
    aOutRequest.append("], \"type\":");

    aOutRequest.append("\"");
    aOutRequest.append(SessionTypeToString(aSessionType));
    aOutRequest.append("\"}");
  }

  void
  GenerateRequest(const uint8_t* aInitData,
                  uint32_t aInitDataSize)
  {
    std::string licRequest;
    std::vector<Bytes> keyIDs;
    ParseInitData(aInitData, aInitDataSize, keyIDs);

    if (keyIDs.size()) {
      MakeKeyRequest(keyIDs, licRequest, kGMPTemporySession);
    }

    if (!licRequest.empty()) {
      mCallback->OnKeyMessage(
        reinterpret_cast<const uint8_t*>(licRequest.c_str()),
        licRequest.length(),
        nullptr);
    }
  }

private:
  IMediaKeySessionCallback* mCallback;
};

class KeyMessageParser {
public:
  bool Parse(const uint8_t* aKeyMessage, size_t aKeyMessageLength)
  {
    if (!aKeyMessage || !aKeyMessageLength) {
      return false;
    }

    std::auto_ptr<ParserImpl> innerParser(
      new ParserImpl(aKeyMessage, aKeyMessageLength));
    int32_t value;
    return innerParser->Parse(mKeyID, value);
  }

  Bytes GetKeyID() { return mKeyID; }

private:

  class ParserImpl {
  public:
    ParserImpl(const uint8_t* aMessage, size_t aLength)
      : mIter(aMessage)
      , mEnd(aMessage + aLength)
    {
    }

    bool
    Parse(Bytes& aKeyId, int32_t& aValue)
    {
      bool finished = false;
      enum class State {
        None, ParseKeyID, ParseValue, Finish
      } state = State::None;

      for (; mIter < mEnd && !finished; mIter++) {
        if (IsSpace(*mIter)) {
          continue;
        }
        switch (state) {
          case State::None:
            if (*mIter == '{') {
              state = State::ParseKeyID;
            }
            break;
          case State::ParseKeyID:
            if (!GetNextElem(aKeyId)) {
              return false;
            }
            state = State::ParseValue;
            break;
          case State::ParseValue:
            if (!GetNextInt32(aValue)) {
              return false;
            }
            state = State::Finish;
            break;
          case State::Finish:
            if (*mIter == '}') {
              finished = true;
            }
            break;
        }
      }
      return finished;
    }

  protected:
    bool IsSpace(uint8_t c)
    {
      return c <= ' '  ||
             c == '\n' ||
             c == '\t' ||
             c == ':'  ||
             c == ';'  ||
             c == ',';
    }

    template <class T>
    bool GetNextElem(T& aOut)
    {
      MOZ_ASSERT(*mIter == '"');

      const uint8_t* start = ++mIter;
      for (; mIter < mEnd; mIter++) {
        if (*mIter == '"') {
          aOut.assign(start, mIter);
          return true;
        }
      }

      return false;
    }

    bool GetNextInt32(int32_t& aOut)
    {
      std::string intString;
      if (GetNextElem(intString)) {
        aOut = atoi(intString.c_str());
        return true;
      }
      return false;
    }

  private:
    const uint8_t* mIter;
    const uint8_t* mEnd;
  };

  Bytes mKeyID;
};

}
}

#endif // __MOZILLA_OCDMIFIX_H__
