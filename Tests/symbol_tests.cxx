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

#include <dlfcn.h>

#include <symbols.hxx>

TEST_CASE("Lookup exported symbols", "[symbol]") {
  Symbols::Descriptor descriptors[] =  {
      { "malloc" },
      { "snprintf" },
  };

  Symbols::lookup(std::span(descriptors));

  for (auto& desc : std::span(descriptors)) {
    REQUIRE(desc.info);
    REQUIRE(desc.info->addr != 0);
    REQUIRE(desc.info->size > 0);
    REQUIRE(desc.info->module_handle != nullptr);
    const uintptr_t sym = (uintptr_t)dlsym(desc.info->module_handle, desc.symbol_name.data());
    CHECK(sym == desc.info->addr);
  }
}

extern "C" {
  size_t test_function_1() { return 0; }
  void test_function_2(size_t value) {}
  size_t test_array_1[] = { 0 };
  size_t test_array_2[] = { 0, 1, 2, 3 };
}

TEST_CASE("Lookup private symbols", "[symbol]") {
  Symbols::Descriptor descriptors[] =  {
      { "test_function_1" },
      { "test_function_2" },
      { "test_array_1" },
      { "test_array_2" },
  };

  uintptr_t local_functions[] = {
    reinterpret_cast<uintptr_t>(test_function_1),
    reinterpret_cast<uintptr_t>(test_function_2),
    reinterpret_cast<uintptr_t>(test_array_1),
    reinterpret_cast<uintptr_t>(test_array_2),
  };

  Symbols::lookup(std::span(descriptors));

  size_t i = 0;
  for (auto& desc : std::span(descriptors)) {
    REQUIRE(desc.info);
    REQUIRE(desc.info->addr != 0);
    REQUIRE(desc.info->size > 0);
    CHECK(local_functions[i++] == desc.info->addr);
  }
}

TEST_CASE("Lookup non-existent symbols", "[symbol]") {
  Symbols::Descriptor descriptors[] =  {
      { "kwyjibo" },
  };

  Symbols::lookup(std::span(descriptors));

  auto& desc = descriptors[0];
  REQUIRE_FALSE(desc.info);
}
