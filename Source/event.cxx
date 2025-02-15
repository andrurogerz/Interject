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

#include "event.hxx"

#include <cassert>
#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace Interject {

Event::Event() {
  __atomic_store_n(&_value, EVENT_VALUE_UNSET, __ATOMIC_RELEASE);
}

void Event::Reset() noexcept {
  __atomic_store_n(&this->_value, EVENT_VALUE_UNSET, __ATOMIC_RELEASE);
}

void Event::Set() noexcept {
  const uint32_t value =
      __atomic_exchange_n(&this->_value, EVENT_VALUE_SET, __ATOMIC_RELEASE);
  if (value == EVENT_VALUE_UNSET) {
    // If the value was previously unset, wake any waiters.
    if (::syscall(SYS_futex, &this->_value, FUTEX_WAKE_PRIVATE, INT_MAX,
                  nullptr, nullptr, 0) == -1) {
      assert(!"FUTEX_WAKE_PRIVATE failed unexpectedly");
    }
  }
}

[[nodiscard]]
bool Event::Wait(struct timespec *timeout) const noexcept {
  for (;;) {
    const uint32_t value = __atomic_load_n(&this->_value, __ATOMIC_ACQUIRE);
    if (value == EVENT_VALUE_SET) {
      break;
    }

    assert(value == EVENT_VALUE_UNSET);

    if (::syscall(SYS_futex, &this->_value, FUTEX_WAIT_PRIVATE, value, timeout,
                  nullptr, 0) == -1) {
      switch (errno) {
      case EAGAIN:
        continue;
      case ETIMEDOUT:
        return false;
      default:
        assert(!"FUTEX_WAIT_PRIVATE failed unexpectedly");
      }
    }
  }
  return true;
}

}; // namespace Interject
