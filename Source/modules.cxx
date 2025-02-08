/*
 * Copyright 2025 Andrew Rogers
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <iostream>
#include <vector>

#include <limits.h>
#include <link.h>
#include <unistd.h>

#include "modules.hxx"

namespace Interject::Modules {

std::string getExecutablePath() {
  constexpr auto SELF_EXE = "/proc/self/exe";
  std::vector<char> buffer(PATH_MAX);
  ssize_t len = ::readlink(SELF_EXE, buffer.data(), buffer.size() - 1);
  while (len == -1 && errno == ENAMETOOLONG) {
    buffer.resize(buffer.size() << 1);
    len = ::readlink(SELF_EXE, buffer.data(), buffer.size() - 1);
  }

  if (len == -1) {
    return std::string();
  }

  buffer[len] = '\0';
  return std::string(buffer.data());
}

static int dl_iterate_phdr_callback(struct dl_phdr_info *info, size_t size,
                                    void *context) {
  if (info->dlpi_phnum == 0 || info->dlpi_phdr == nullptr) {
    // Entry has no ELF headers so skip it.
    return 0;
  }

  auto &func = *static_cast<Callback *>(context);
  if (info->dlpi_name == nullptr || info->dlpi_name[0] == '\0') {
    // The name of the entry is not populated, indicating this is the main
    // executable.
    func(getExecutablePath(), static_cast<uintptr_t>(info->dlpi_addr));
  } else {
    func(static_cast<std::string_view>(info->dlpi_name),
         static_cast<uintptr_t>(info->dlpi_addr));
  }

  return 0;
}

void forEach(Callback callback) {
  dl_iterate_phdr(dl_iterate_phdr_callback, &callback);
}

}; // namespace Interject::Modules
