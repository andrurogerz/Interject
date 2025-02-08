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

#include <fstream>
#include <format>
#include <iostream>
#include <sstream>

#include <sys/mman.h>

#include "memory_map.hxx"

namespace Interject {

bool MemoryMap::load(const std::string_view &file_name) {
  std::ifstream mapsFile(file_name.data());
  if (!mapsFile.is_open()) {
    std::cerr << std::format("Failed to open file {}", file_name) << std::endl;
    return false;
  }

  _regions.clear();
  std::string line;
  while (std::getline(mapsFile, line)) {
    std::string_view lineView(line);

    const auto addrEnd = lineView.find(' ');
    const std::string_view addrRange = lineView.substr(0, addrEnd);
    const std::string_view permissions = lineView.substr(addrEnd + 1, 4);

    const auto dashPos = addrRange.find('-');
    const std::string_view startAddr = addrRange.substr(0, dashPos);
    const std::string_view endAddr = addrRange.substr(dashPos + 1);

    const auto parseAddr = [](std::string_view str, std::uintptr_t &addr) -> bool {
      auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), addr, 16);
      return (ec == std::errc()) ? true : false;
    };

    std::uintptr_t start, end;
    if (!parseAddr(startAddr, start) || !parseAddr(endAddr, end)) {
      std::cerr << std::format("failed to parse address range {}", addrRange) << std::endl;
      return false;
    }

    _regions.emplace_back((Region){
      .start = start,
      .end = end,
      .permissions = (permissions[0] == 'r' ? PROT_READ : 0)
          | (permissions[1] == 'w' ? PROT_WRITE : 0)
          | (permissions[2] == 'x' ? PROT_EXEC : 0),
    });
  }

  return true;
}

const std::vector<MemoryMap::Region>& MemoryMap::regions() const {
  return _regions;
}

};
