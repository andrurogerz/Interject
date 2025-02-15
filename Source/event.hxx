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
#include <time.h>

namespace Interject {

// A simple futex-based synchronization primitive similar to a Win32 manual-
// reset event.
class Event final {
public:
  Event();

  // Reset the event from set to unset. Noop if the event is not already set.
  void Reset() noexcept;

  // Set the event an unblock all waiters. Noop if the event is already set.
  void Set() noexcept;

  // Wait for the event to transition from unset to set. Returns immediately if
  // the event is already set.
  [[nodiscard]] bool Wait(struct timespec *timeout) const noexcept;

  // Wait with infinite timeout.
  void Wait() const noexcept { (void)Wait(nullptr); }

private:
  static constexpr std::uint32_t EVENT_VALUE_UNSET = 0;
  static constexpr std::uint32_t EVENT_VALUE_SET = 1;
  uint32_t _value;
};

}; // namespace Interject
