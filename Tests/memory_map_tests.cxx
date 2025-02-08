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

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include <sys/mman.h>

#include <memory_map.hxx>

using namespace Interject;

TEST_CASE("Parse non-existent file", "[memory_map]") {
  MemoryMap map;
  REQUIRE_FALSE(map.load("/does/not/exist"));
}

TEST_CASE("Parse /proc/self/maps", "[memory_map]") {
  MemoryMap map;
  REQUIRE(map.load("/proc/self/maps"));

  // Try to locate an executable section that contains our return addr.
  bool found_addr = false;
  const std::uintptr_t return_addr = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));

  for (auto &region : map.regions()) {
    CHECK(region.start > 0);
    CHECK(region.end > region.start);

    // Expect at least one permission bit is set on every region.
    CHECK(region.permissions != 0);
    CHECK_FALSE(region.permissions & ~(PROT_READ | PROT_WRITE | PROT_EXEC));

    if (return_addr >= region.start && return_addr <= region.end) {
      CHECK_FALSE(found_addr);
      CHECK(region.permissions & PROT_EXEC);
      found_addr = true;
    }
  }

  CHECK(found_addr);
}
