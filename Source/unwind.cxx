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

#include "unwind.hxx"

#include <execinfo.h>

#if defined(__ANDROID__)
#include <unwind.h>
#endif

namespace Interject::Unwind {

#if defined(__GLIBC__)
// Use glibc backtrace() API when available.
std::size_t backtrace(std::span<void *> stackFrames) {
  return (std::size_t)::backtrace(stackFrames.data(), stackFrames.size());
}

#elif defined(__ANDROID__)
// Android doesn't have the glibc backtrace() API, but we can replicate its
// functionality using _Unwind_Backtrace from libunwind.
struct UnwindState {
  void **buffer_cursor;
  void **buffer_end;
};

static _Unwind_Reason_Code UnwindTrace(struct _Unwind_Context *context,
                                       void *arg) {
  UnwindState *state = reinterpret_cast<UnwindState *>(arg);
  uintptr_t ip = ::_Unwind_GetIP(context);
  if (ip != 0) {
    if (state->buffer_cursor >= state->buffer_end) {
      // Backtrace has more frames than we have space for in the buffer. It will
      // be truncated.
      return ::_URC_END_OF_STACK;
    }
    *(state->buffer_cursor++) = reinterpret_cast<void *>(ip);
  }
  return ::_URC_NO_REASON;
}

std::size_t backtrace(std::span<void *> stackFrames) {
  UnwindState state = {
      .buffer_cursor = stackFrames.data(),
      .buffer_end = stackFrames.data() + stackFrames.size(),
  };

  ::_Unwind_Backtrace(UnwindTrace, &state);
  return state.buffer_cursor - stackFrames.data();
}

#else
#warning No backtrace support found.
std::size_t backtrace(std::span<void *> stackFrames) { return 0; }
#endif

}; // namespace Interject::Unwind
