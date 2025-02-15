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

#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>

#include "threads.hxx"

namespace Interject::Threads {

bool forEach(Callback callback) {
  DIR *dir;
  if ((dir = opendir("/proc/self/task")) == nullptr) {
    return false;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.') {
      continue;
    }

    if (entry->d_type == DT_DIR) {
      pid_t tid = atoi(entry->d_name);
      callback(tid);
    }
  }

  closedir(dir);
  return true;
}

std::optional<std::vector<pid_t>> all() {
  std::vector<pid_t> threads;
  if (!forEach([&](pid_t tid) { threads.push_back(tid); })) {
    return std::nullopt;
  }

  return std::move(threads);
}

}; // namespace Interject::Threads
