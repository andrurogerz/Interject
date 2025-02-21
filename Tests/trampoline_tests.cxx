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

#include "functions.h"

#include <trampoline.hxx>

using namespace Interject;

TEST_CASE("trampoline correctness", "[trampoline]") {
  constexpr size_t (*functions[])(size_t) = {
    count_set_bits,
    fibonacci,
    isqrt,
    sum_of_digits,
    reverse_digits,
    factorial
  };

  constexpr std::string_view names[] = {
    "count_set_bits",
    "fibonacci",
    "isqrt",
    "sum_of_digits",
    "reverse_digits",
    "factorial"
  };

  Symbols::Descriptor descriptors[std::span(names).size()];
  Symbols::lookup(std::span(names), std::span(descriptors));

  for (size_t idx = 0; idx < std::span(names).size(); idx++) {
    auto &descriptor = descriptors[idx];
    REQUIRE(descriptor.addr != 0);
    REQUIRE(descriptor.size != 0);

    auto trampoline = Trampoline::create(descriptor);
    REQUIRE(trampoline);

    auto trampolineFn = reinterpret_cast<size_t (*)(size_t)>(trampoline->start());
    auto origFn = functions[idx];

    // Ensure the trampoline function behaves the same as the original.
    CHECK(trampolineFn(147) == origFn(147));
  }
}


