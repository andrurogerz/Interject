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

using namespace Interject;

TEST_CASE("Lookup exported symbols", "[symbol]") {
  constexpr std::string_view names[] = {
    "malloc",
    "snprintf",
  };
  std::optional<Symbols::Descriptor> descriptors[std::span(names).size()];

  Symbols::lookup(std::span(names), std::span(descriptors));

  for (size_t idx = 0; idx < std::span(names).size(); idx++) {
    auto& name = names[idx];
    auto& desc = descriptors[idx];
    REQUIRE(desc);
    CHECK(desc->size > 0);
    CHECK(desc->module_handle != nullptr);
    const uintptr_t sym = (uintptr_t)dlsym(desc->module_handle, name.data());
    CHECK(sym == desc->addr);
  }
}

extern "C" {
  size_t test_function_1() { return 0; }
  void test_function_2(size_t value) {}
  size_t test_array_1[] = { 0 };
  size_t test_array_2[] = { 0, 1, 2, 3 };
}

TEST_CASE("Lookup private symbols", "[symbol]") {
  constexpr std::string_view names[] = {
    "test_function_1",
    "test_function_2",
    "test_array_1",
    "test_array_2",
  };
  std::optional<Symbols::Descriptor> descriptors[std::span(names).size()];

  uintptr_t local_functions[] = {
    reinterpret_cast<uintptr_t>(test_function_1),
    reinterpret_cast<uintptr_t>(test_function_2),
    reinterpret_cast<uintptr_t>(test_array_1),
    reinterpret_cast<uintptr_t>(test_array_2),
  };

  Symbols::lookup(std::span(names), std::span(descriptors));

  for (size_t idx = 0; idx < std::span(names).size(); idx++) {
    auto& name = names[idx];
    auto& desc = descriptors[idx];
    REQUIRE(desc);
    CHECK(desc->size > 0);
    CHECK(local_functions[idx] == desc->addr);
  }
}

TEST_CASE("Lookup non-existent symbols", "[symbol]") {
  constexpr std::string_view names[] {
    "kwyjibo",
  };
  std::optional<Symbols::Descriptor> descriptors[std::span(names).size()];

  Symbols::lookup(std::span(names), std::span(descriptors));

  auto& desc = descriptors[0];
  REQUIRE_FALSE(desc);
}
