// Copyright (c) 2012, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "platform/globals.h"
#if defined(TARGET_OS_WINDOWS)

#include "bin/platform.h"
#include "bin/log.h"
#include "bin/socket.h"
#include "bin/utils.h"


namespace dart {
namespace bin {

bool Platform::Initialize() {
  // Nothing to do on Windows.
  return true;
}


int Platform::NumberOfProcessors() {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}


const char* Platform::OperatingSystem() {
  return "windows";
}


const char* Platform::LibraryExtension() {
  return "dll";
}


bool Platform::LocalHostname(char *buffer, intptr_t buffer_length) {
#if defined(PLATFORM_DISABLE_SOCKET)
  return false;
#else
  if (!Socket::Initialize()) return false;
  return gethostname(buffer, buffer_length) == 0;
#endif
}


char** Platform::Environment(intptr_t* count) {
  wchar_t* strings = GetEnvironmentStringsW();
  if (strings == NULL) return NULL;
  wchar_t* tmp = strings;
  intptr_t i = 0;
  while (*tmp != '\0') {
    // Skip environment strings starting with "=".
    // These are synthetic variables corresponding to dynamic environment
    // variables like %=C:% and %=ExitCode%, and the Dart environment does
    // not include these.
    if (*tmp != '=') i++;
    tmp += (wcslen(tmp) + 1);
  }
  *count = i;
  char** result = new char*[i];
  tmp = strings;
  for (intptr_t current = 0; current < i;) {
    // Skip the strings that were not counted above.
    if (*tmp != '=') result[current++] = StringUtils::WideToUtf8(tmp);
    tmp += (wcslen(tmp) + 1);
  }
  FreeEnvironmentStringsW(strings);
  return result;
}


void Platform::FreeEnvironment(char** env, intptr_t count) {
  for (intptr_t i = 0; i < count; i++) {
    free(env[i]);
  }
  delete[] env;
}

}  // namespace bin
}  // namespace dart

#endif  // defined(TARGET_OS_WINDOWS)
