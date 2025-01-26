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
#include <functional>
#include <string_view>

namespace Modules {
  //using Callback = void(*)(std::string_view obj_name, std::uintptr_t base_addr);
  using Callback = std::function<void(std::string_view obj_name, std::uintptr_t base_addr)>;

  // Iterate the current process linkmap and invoke a callback for each loaded
  // module.
  void forEach(Callback);

  // Return the executable file path for the current process.
  std::string getExecutablePath();
};
