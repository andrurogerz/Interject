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
#include <optional>
#include <span>
#include <string_view>

namespace Symbols {

struct Descriptor {
  struct Info {
    std::uintptr_t addr;
    std::size_t size;
    void *module_handle;
    ~Info();
  };
  std::string_view symbol_name;
  std::optional<Info> info;
};

void lookup(std::span<Descriptor> descriptors);

};
