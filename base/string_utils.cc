/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "string_utils.h"

namespace rtc {

size_t strcpyn(char* buffer,
               size_t buflen,
               const char* source,
               size_t srclen /* = SIZE_UNKNOWN */) {
  if (buflen <= 0)
    return 0;

  if (srclen == SIZE_UNKNOWN) {
    srclen = strlen(source);
  }
  if (srclen >= buflen) {
    srclen = buflen - 1;
  }
  memcpy(buffer, source, srclen);
  buffer[srclen] = 0;
  return srclen;
}

static const char kWhitespace[] = " \n\r\t";

std::string string_trim(const std::string& s) {
  std::string::size_type first = s.find_first_not_of(kWhitespace);
  std::string::size_type last = s.find_last_not_of(kWhitespace);

  if (first == std::string::npos || last == std::string::npos) {
    return std::string("");
  }

  return s.substr(first, last - first + 1);
}

std::string ToHex(const int i) {
  char buffer[50];
  snprintf(buffer, sizeof(buffer), "%x", i);

  return std::string(buffer);
}

std::string LeftPad(char padding, unsigned length, std::string s) {
  if (s.length() >= length)
    return s;
  return std::string(length - s.length(), padding) + s;
}

#if defined(WEBRTC_WIN)
std::string Utf8ToString(const std::string& str)
{
  int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
  wchar_t* pwBuf = new wchar_t[nwLen + 1]; 
  memset(pwBuf, 0, nwLen * 2 + 2);
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), pwBuf, nwLen);
  int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
  char* pBuf = new char[nLen + 1];
  memset(pBuf, 0, nLen + 1);
  WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

  std::string strRet = pBuf;

  delete[]pBuf;
  delete[]pwBuf;
  pBuf = NULL;
  pwBuf = NULL;

  return strRet;
}

std::string StringToUtf8(const std::string& str)
{
  int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
  wchar_t* pwBuf = new wchar_t[nwLen + 1];
  ZeroMemory(pwBuf, nwLen * 2 + 2);
  ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);
  int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
  char* pBuf = new char[nLen + 1];
  ZeroMemory(pBuf, nLen + 1);
  ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

  std::string strRet(pBuf);

  delete[]pwBuf;
  delete[]pBuf;
  pwBuf = NULL;
  pBuf = NULL;

  return strRet;
}

#endif  // WEBRTC_WIN

}  // namespace rtc
