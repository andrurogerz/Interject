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

#include <cassert>

#include <signal.h>

namespace Interject {

class SignalAction {
public:
  using Action = void (*)(int, siginfo_t *, void *);

  SignalAction(int signal, Action action, int flags) : _signal(signal) {
    if (::sigaction(signal, nullptr, &_origAction) == -1) {
      assert(!"sigaction failed");
    }

    struct sigaction newAction;
    newAction.sa_sigaction = action;
    newAction.sa_flags = flags;
    sigemptyset(&newAction.sa_mask);

    if (::sigaction(_signal, &newAction, nullptr) == -1) {
      assert(!"sigaction failed");
    }
  }

  ~SignalAction() {
    if (::sigaction(_signal, &_origAction, nullptr) == -1) {
      assert(!"sigaction failed");
    }
  }

private:
  int _signal;
  struct sigaction _origAction;
};

}; // namespace Interject
