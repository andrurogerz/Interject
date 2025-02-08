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

#pragma once

#include <charconv>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Interject {

class MemoryMap {
public:
  struct Region {
    std::uintptr_t start;
    std::uintptr_t end;
    int permissions;
  };

  // load or re-load memory mapping for the current process
  bool load() { return load("/proc/self/maps"); }

  // load or re-load the specified memory mapping
  bool load(const std::string_view &file_name);

  std::span<const Region> regions() const {
    return {_regions.data(), _regions.size()};
  }

private:
  std::vector<Region> _regions;
};

}; // namespace Interject
