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

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

namespace Interject {

class MemoryMap {
public:
  struct Region {
    std::uintptr_t start;
    std::uintptr_t end;
    int permissions;
  };

  // load or re-load
  bool load(const std::string_view &file_name);

  const std::vector<Region>& regions() const;

private:
  std::vector<Region> _regions;
};

};
